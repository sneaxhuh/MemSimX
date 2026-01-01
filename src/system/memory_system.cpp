#include "system/memory_system.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

namespace memsim {

MemorySystem::MemorySystem(size_t memory_size, bool enable_vm, bool enable_cache)
    : memory_(std::make_unique<PhysicalMemory>(memory_size)),
      vm_enabled_(enable_vm),
      cache_enabled_(enable_cache),
      verbose_logging_(false),
      memory_size_(memory_size) {

    // Default cache configuration (can be changed before first use)
    l1_config_ = {8, 2, 64, CachePolicy::LRU};  // 8 sets, 2-way, 64B blocks
    l2_config_ = {16, 4, 64, CachePolicy::LRU}; // 16 sets, 4-way, 64B blocks

    // Default VM configuration
    vm_config_ = {64, 16, 512, PageReplacementPolicy::LRU}; // 64 pages, 16 frames, 512B pages

    // Create allocator (always needed)
    allocator_ = std::make_unique<StandardAllocator>(
        memory_.get(),
        AllocatorType::BEST_FIT
    );

    // Initialize cache if enabled
    if (cache_enabled_) {
        initializeCache();
    }

    // Initialize VM if enabled
    if (vm_enabled_) {
        initializeVM();
    }
}

void MemorySystem::initializeCache() {
    cache_ = std::make_unique<CacheHierarchy>(
        memory_.get(),
        l1_config_.sets, l1_config_.associativity,
        l1_config_.block_size, l1_config_.policy,
        l2_config_.sets, l2_config_.associativity,
        l2_config_.block_size, l2_config_.policy
    );
}

void MemorySystem::initializeVM() {
    vm_ = std::make_unique<VirtualMemory>(
        memory_.get(),
        vm_config_.num_virtual_pages,
        vm_config_.num_physical_frames,
        vm_config_.page_size,
        vm_config_.policy
    );
}

void MemorySystem::configureCacheL1(size_t sets, size_t associativity,
                                     size_t block_size, CachePolicy policy) {
    l1_config_ = {sets, associativity, block_size, policy};
    if (cache_enabled_) {
        initializeCache();
    }
}

void MemorySystem::configureCacheL2(size_t sets, size_t associativity,
                                     size_t block_size, CachePolicy policy) {
    l2_config_ = {sets, associativity, block_size, policy};
    if (cache_enabled_) {
        initializeCache();
    }
}

void MemorySystem::configureVM(size_t num_virtual_pages, size_t num_physical_frames,
                                size_t page_size, PageReplacementPolicy policy) {
    vm_config_ = {num_virtual_pages, num_physical_frames, page_size, policy};
    if (vm_enabled_) {
        initializeVM();
    }
}

AccessLevel MemorySystem::determineAccessLevel(Address phys_addr, bool /* is_write */) {
    if (!cache_enabled_) {
        return AccessLevel::MEMORY;
    }

    // Check L1 cache
    auto l1_contains = cache_->containsInL1(phys_addr);
    if (l1_contains) {
        return AccessLevel::L1_CACHE;
    }

    // Check L2 cache
    auto l2_contains = cache_->containsInL2(phys_addr);
    if (l2_contains) {
        return AccessLevel::L2_CACHE;
    }

    return AccessLevel::MEMORY;
}

AccessResult MemorySystem::read(Address address) {
    AccessResult result;
    result.virtual_address = address;
    result.used_virtual_memory = vm_enabled_;

    session_stats_.total_accesses++;
    session_stats_.total_reads++;

    Address physical_addr = address;

    // Step 1: Virtual memory translation (if enabled)
    if (vm_enabled_) {
        auto vm_stats_before = vm_->getStats();
        auto translate_result = vm_->translate(address);

        if (!translate_result.success) {
            result.success = false;
            result.level = AccessLevel::PAGE_FAULT;
            session_stats_.page_faults++;
            recordAccess(result);
            return result;
        }

        physical_addr = translate_result.value;
        result.physical_address = physical_addr;

        // Check if page fault occurred
        auto vm_stats_after = vm_->getStats();
        if (vm_stats_after.page_faults > vm_stats_before.page_faults) {
            result.level = AccessLevel::PAGE_FAULT;
            session_stats_.page_faults++;
        }
    } else {
        result.physical_address = physical_addr;
    }

    // Step 2: Access through cache hierarchy
    if (cache_enabled_) {
        // Check cache state before access
        auto cache_stats_before = cache_->getStats();

        // Perform the cache read
        auto cache_result = cache_->read(physical_addr);

        if (!cache_result.success) {
            result.success = false;
            recordAccess(result);
            return result;
        }

        result.value = cache_result.value;

        // Determine which level served the request
        auto cache_stats_after = cache_->getStats();

        if (cache_stats_after.l1_stats.hits > cache_stats_before.l1_stats.hits) {
            // L1 hit
            result.level = AccessLevel::L1_CACHE;
            session_stats_.l1_hits++;
        } else if (cache_stats_after.l2_stats.hits > cache_stats_before.l2_stats.hits) {
            // L1 miss, L2 hit
            result.level = AccessLevel::L2_CACHE;
            session_stats_.l2_hits++;
        } else {
            // L1 miss, L2 miss -> Memory access
            result.level = AccessLevel::MEMORY;
            session_stats_.memory_accesses++;
        }

        result.success = true;
    } else {
        // No cache - direct memory access
        auto mem_result = memory_->read(physical_addr);
        result.success = mem_result.success;
        result.value = mem_result.value;
        result.level = AccessLevel::MEMORY;
        session_stats_.memory_accesses++;
    }

    // Override with page fault if VM detected one
    if (vm_enabled_ && result.level == AccessLevel::PAGE_FAULT) {
        // Already handled above
    }

    recordAccess(result);

    if (verbose_logging_) {
        std::cout << "READ  [0x" << std::hex << std::setw(8) << std::setfill('0')
                  << address << std::dec << "] → "
                  << getAccessLevelColor(result.level)
                  << std::setw(12) << std::left << accessLevelToString(result.level)
                  << "\033[0m"
                  << " (value: 0x" << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(result.value) << std::dec << ")"
                  << std::endl;
    }

    return result;
}

AccessResult MemorySystem::write(Address address, uint8_t data) {
    AccessResult result;
    result.virtual_address = address;
    result.value = data;
    result.used_virtual_memory = vm_enabled_;

    session_stats_.total_accesses++;
    session_stats_.total_writes++;

    Address physical_addr = address;

    // Step 1: Virtual memory translation (if enabled)
    if (vm_enabled_) {
        auto vm_stats_before = vm_->getStats();
        auto write_result = vm_->write(address, data);

        if (!write_result.success) {
            result.success = false;
            result.level = AccessLevel::PAGE_FAULT;
            session_stats_.page_faults++;
            recordAccess(result);
            return result;
        }

        // Get physical address for cache check
        auto translate_result = vm_->translate(address);
        if (translate_result.success) {
            physical_addr = translate_result.value;
            result.physical_address = physical_addr;
        }

        // Check if page fault occurred
        auto vm_stats_after = vm_->getStats();
        if (vm_stats_after.page_faults > vm_stats_before.page_faults) {
            result.level = AccessLevel::PAGE_FAULT;
            session_stats_.page_faults++;
        }
    } else {
        result.physical_address = physical_addr;
    }

    // Step 2: Cache access (if enabled)
    if (cache_enabled_) {
        auto cache_stats_before = cache_->getStats();

        // Perform cache write (write-through)
        auto cache_result = cache_->write(physical_addr, data);

        if (!cache_result.success) {
            result.success = false;
            recordAccess(result);
            return result;
        }

        // Determine which level served the request
        auto cache_stats_after = cache_->getStats();

        if (cache_stats_after.l1_stats.hits > cache_stats_before.l1_stats.hits) {
            result.level = AccessLevel::L1_CACHE;
            session_stats_.l1_hits++;
        } else if (cache_stats_after.l2_stats.hits > cache_stats_before.l2_stats.hits) {
            result.level = AccessLevel::L2_CACHE;
            session_stats_.l2_hits++;
        } else {
            result.level = AccessLevel::MEMORY;
            session_stats_.memory_accesses++;
        }

        result.success = true;
    } else {
        // No cache - direct memory access
        auto mem_result = memory_->write(physical_addr, data);
        result.success = mem_result.success;
        result.level = AccessLevel::MEMORY;
        session_stats_.memory_accesses++;
    }

    // Override with page fault if VM detected one
    if (vm_enabled_ && result.level == AccessLevel::PAGE_FAULT) {
        // Already handled above
    }

    recordAccess(result);

    if (verbose_logging_) {
        std::cout << "WRITE [0x" << std::hex << std::setw(8) << std::setfill('0')
                  << address << std::dec << "] → "
                  << getAccessLevelColor(result.level)
                  << std::setw(12) << std::left << accessLevelToString(result.level)
                  << "\033[0m"
                  << " (value: 0x" << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(data) << std::dec << ")"
                  << std::endl;
    }

    return result;
}

void MemorySystem::recordAccess(const AccessResult& result) {
    access_history_.push_back(result);

    // Keep history size bounded
    if (access_history_.size() > MAX_HISTORY_SIZE) {
        access_history_.erase(access_history_.begin());
    }
}

Result<BlockId> MemorySystem::allocate(size_t size) {
    return allocator_->allocate(size);
}

Result<void> MemorySystem::deallocate(BlockId block_id) {
    return allocator_->deallocate(block_id);
}

void MemorySystem::resetSessionStats() {
    session_stats_ = SessionStats();
    access_history_.clear();
}

void MemorySystem::flushCaches() {
    if (cache_) {
        cache_->flush();
    }
}

std::vector<AccessResult> MemorySystem::getRecentAccesses(size_t count) const {
    if (access_history_.size() <= count) {
        return access_history_;
    }

    return std::vector<AccessResult>(
        access_history_.end() - count,
        access_history_.end()
    );
}

std::string MemorySystem::getSessionReport() const {
    std::ostringstream oss;

    oss << "\n";
    oss << "═══════════════════════════════════════════════════════════════\n";
    oss << "                    SESSION REPORT                             \n";
    oss << "═══════════════════════════════════════════════════════════════\n";
    oss << "\n";

    // Access Summary
    oss << "Access Summary:\n";
    oss << "───────────────────────────────────────────────────────────────\n";
    oss << "  Total Accesses:     " << std::setw(10) << session_stats_.total_accesses << "\n";
    oss << "  Total Reads:        " << std::setw(10) << session_stats_.total_reads << "\n";
    oss << "  Total Writes:       " << std::setw(10) << session_stats_.total_writes << "\n";
    oss << "\n";

    // Per-Level Statistics (Current Session)
    oss << "Access Distribution (Current Session):\n";
    oss << "───────────────────────────────────────────────────────────────\n";
    oss << "  L1 Cache Hits:      " << std::setw(10) << session_stats_.l1_hits
        << "  (" << std::fixed << std::setprecision(1)
        << session_stats_.getL1HitRate() << "%)\n";
    oss << "  L2 Cache Hits:      " << std::setw(10) << session_stats_.l2_hits
        << "  (" << std::fixed << std::setprecision(1)
        << session_stats_.getL2HitRate() << "%)\n";
    oss << "  Memory Accesses:    " << std::setw(10) << session_stats_.memory_accesses
        << "  (" << std::fixed << std::setprecision(1)
        << session_stats_.getMemoryAccessRate() << "%)\n";

    if (vm_enabled_) {
        oss << "  Page Faults:        " << std::setw(10) << session_stats_.page_faults
            << "  (" << std::fixed << std::setprecision(1)
            << session_stats_.getPageFaultRate() << "%)\n";
    }
    oss << "\n";

    // Component Statistics (Cumulative)
    if (cache_) {
        auto cache_stats = cache_->getStats();
        oss << "Cache Hierarchy (Cumulative):\n";
        oss << "───────────────────────────────────────────────────────────────\n";
        oss << "  L1: " << cache_stats.l1_stats.hits << " hits, "
            << cache_stats.l1_stats.misses << " misses "
            << "(" << std::fixed << std::setprecision(1)
            << cache_stats.l1_stats.getHitRatio() << "% hit ratio)\n";
        oss << "  L2: " << cache_stats.l2_stats.hits << " hits, "
            << cache_stats.l2_stats.misses << " misses "
            << "(" << std::fixed << std::setprecision(1)
            << cache_stats.l2_stats.getHitRatio() << "% hit ratio)\n";
        oss << "  Overall: " << std::fixed << std::setprecision(1)
            << cache_stats.getOverallHitRatio() << "% hit ratio\n";
        oss << "\n";
    }

    if (vm_) {
        auto vm_stats = vm_->getStats();
        oss << "Virtual Memory (Cumulative):\n";
        oss << "───────────────────────────────────────────────────────────────\n";
        oss << "  Page Faults:        " << vm_stats.page_faults << "\n";
        oss << "  Page Hits:          " << vm_stats.page_hits << "\n";
        oss << "  Page Fault Rate:    " << std::fixed << std::setprecision(1)
            << vm_stats.getPageFaultRate() << "%\n";
        oss << "\n";
    }

    // Allocator Statistics
    oss << "Memory Allocator:\n";
    oss << "───────────────────────────────────────────────────────────────\n";
    oss << "  Utilization:        " << std::fixed << std::setprecision(1)
        << allocator_->getUtilization() << "%\n";
    oss << "  Internal Frag:      " << std::fixed << std::setprecision(1)
        << allocator_->getInternalFragmentation() << "%\n";
    oss << "  External Frag:      " << std::fixed << std::setprecision(1)
        << allocator_->getExternalFragmentation() << "%\n";

    oss << "═══════════════════════════════════════════════════════════════\n";

    return oss.str();
}

std::string MemorySystem::getVisualStats() const {
    std::ostringstream oss;

    oss << "\n";
    oss << "┌─────────────────────────────────────────────────────────────┐\n";
    oss << "│           MEMORY ACCESS VISUALIZATION                      │\n";
    oss << "└─────────────────────────────────────────────────────────────┘\n";
    oss << "\n";

    if (session_stats_.total_accesses == 0) {
        oss << "  No accesses recorded yet.\n\n";
        return oss.str();
    }

    // Calculate percentages
    double l1_pct = session_stats_.getL1HitRate();
    double l2_pct = session_stats_.getL2HitRate();
    double mem_pct = session_stats_.getMemoryAccessRate();
    double pf_pct = session_stats_.getPageFaultRate();

    // Visual bar chart
    auto make_bar = [](double percentage, int width = 40) -> std::string {
        int filled = static_cast<int>((percentage / 100.0) * width);
        std::string bar;
        for (int i = 0; i < filled; i++) bar += "█";
        for (int i = filled; i < width; i++) bar += "░";
        return bar;
    };

    oss << "Access Distribution:\n\n";

    oss << "  \033[32m█\033[0m L1 Cache    "
        << make_bar(l1_pct)
        << " " << std::setw(5) << std::fixed << std::setprecision(1)
        << l1_pct << "%  (" << session_stats_.l1_hits << ")\n";

    oss << "  \033[33m█\033[0m L2 Cache    "
        << make_bar(l2_pct)
        << " " << std::setw(5) << std::fixed << std::setprecision(1)
        << l2_pct << "%  (" << session_stats_.l2_hits << ")\n";

    oss << "  \033[31m█\033[0m Memory      "
        << make_bar(mem_pct)
        << " " << std::setw(5) << std::fixed << std::setprecision(1)
        << mem_pct << "%  (" << session_stats_.memory_accesses << ")\n";

    if (vm_enabled_ && session_stats_.page_faults > 0) {
        oss << "  \033[35m█\033[0m Page Faults "
            << make_bar(pf_pct)
            << " " << std::setw(5) << std::fixed << std::setprecision(1)
            << pf_pct << "%  (" << session_stats_.page_faults << ")\n";
    }

    oss << "\n";
    oss << "Total Accesses: " << session_stats_.total_accesses
        << " (Reads: " << session_stats_.total_reads
        << ", Writes: " << session_stats_.total_writes << ")\n";
    oss << "\n";

    // Recent accesses
    oss << "Recent Accesses (last 10):\n";
    oss << "┌──────────┬────────────┬──────────────┬────────┐\n";
    oss << "│ Address  │   Type     │    Level     │ Value  │\n";
    oss << "├──────────┼────────────┼──────────────┼────────┤\n";

    auto recent = getRecentAccesses(10);
    for (const auto& access : recent) {
        oss << "│ 0x" << std::hex << std::setw(6) << std::setfill('0')
            << access.virtual_address << std::dec << " │ ";
        oss << std::setw(10) << (access.success ? "SUCCESS" : "FAIL") << " │ ";

        std::string level_str = accessLevelToString(access.level);
        oss << std::setw(12) << level_str << " │ ";
        oss << "0x" << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(access.value) << std::dec << " │\n";
    }

    oss << "└──────────┴────────────┴──────────────┴────────┘\n";

    return oss.str();
}

std::string MemorySystem::getAllStats() const {
    std::ostringstream oss;

    oss << getSessionReport();
    oss << "\n";
    oss << getVisualStats();

    return oss.str();
}

} // namespace memsim
