#ifndef MEMSIM_SYSTEM_MEMORY_SYSTEM_H
#define MEMSIM_SYSTEM_MEMORY_SYSTEM_H

#include "common/types.h"
#include "common/result.h"
#include "memory/physical_memory.h"
#include "cache/cache_hierarchy.h"
#include "virtual_memory/virtual_memory.h"
#include "allocator/standard_allocator.h"
#include "allocator/buddy_allocator.h"
#include <memory>
#include <string>
#include <vector>
#include <iomanip>

namespace memsim {

/**
 * @brief Access level enumeration - where data was found
 */
enum class AccessLevel {
    L1_CACHE,       // Data found in L1 cache
    L2_CACHE,       // Data found in L2 cache (L1 miss)
    MEMORY,         // Data found in memory (L1 and L2 miss)
    PAGE_FAULT      // Page fault occurred (VM miss)
};

/**
 * @brief Result of a memory access with detailed tracking
 */
struct AccessResult {
    bool success;
    uint8_t value;              // Data read (if success)
    AccessLevel level;          // Where data was found
    Address physical_address;   // Physical address accessed
    Address virtual_address;    // Virtual address (if using VM)
    bool used_virtual_memory;   // Whether VM translation occurred

    AccessResult()
        : success(false), value(0), level(AccessLevel::MEMORY),
          physical_address(0), virtual_address(0),
          used_virtual_memory(false) {}
};

/**
 * @brief Session statistics for all accesses
 */
struct SessionStats {
    // Total access counts
    uint64_t total_accesses;

    // Per-level access counts
    uint64_t l1_hits;
    uint64_t l2_hits;
    uint64_t memory_accesses;
    uint64_t page_faults;

    // Running totals
    uint64_t total_reads;
    uint64_t total_writes;

    SessionStats()
        : total_accesses(0), l1_hits(0), l2_hits(0),
          memory_accesses(0), page_faults(0),
          total_reads(0), total_writes(0) {}

    double getL1HitRate() const {
        if (total_accesses == 0) return 0.0;
        return (static_cast<double>(l1_hits) / total_accesses) * 100.0;
    }

    double getL2HitRate() const {
        if (total_accesses == 0) return 0.0;
        return (static_cast<double>(l2_hits) / total_accesses) * 100.0;
    }

    double getMemoryAccessRate() const {
        if (total_accesses == 0) return 0.0;
        return (static_cast<double>(memory_accesses) / total_accesses) * 100.0;
    }

    double getPageFaultRate() const {
        if (total_accesses == 0) return 0.0;
        return (static_cast<double>(page_faults) / total_accesses) * 100.0;
    }
};

/**
 * @brief Integrated memory system with cache, VM, and allocation
 *
 * This class provides a unified interface for memory operations where:
 * - All accesses go through cache hierarchy (L1 -> L2 -> Memory)
 * - Virtual memory can optionally translate addresses
 * - Detailed logging shows exactly where each access is served from
 * - Session statistics track hits/misses at each level
 */
class MemorySystem {
public:
    /**
     * @brief Construct integrated memory system
     *
     * @param memory_size Size of physical memory in bytes
     * @param enable_vm Whether to enable virtual memory
     * @param enable_cache Whether to enable cache hierarchy
     */
    MemorySystem(size_t memory_size,
                 bool enable_vm = true,
                 bool enable_cache = true);

    ~MemorySystem() = default;

    // Disable copy/move
    MemorySystem(const MemorySystem&) = delete;
    MemorySystem& operator=(const MemorySystem&) = delete;

    /**
     * @brief Read data with full tracking
     *
     * @param address Address to read (virtual if VM enabled, physical otherwise)
     * @return AccessResult with data and detailed access information
     */
    AccessResult read(Address address);

    /**
     * @brief Write data with full tracking
     *
     * @param address Address to write
     * @param data Data to write
     * @return AccessResult with detailed access information
     */
    AccessResult write(Address address, uint8_t data);

    /**
     * @brief Allocate memory block
     *
     * @param size Size in bytes
     * @return Result containing BlockId or error
     */
    Result<BlockId> allocate(size_t size);

    /**
     * @brief Deallocate memory block
     *
     * @param block_id Block to deallocate
     * @return Result indicating success or error
     */
    Result<void> deallocate(BlockId block_id);

    /**
     * @brief Get session statistics
     */
    const SessionStats& getSessionStats() const { return session_stats_; }

    /**
     * @brief Reset session statistics
     */
    void resetSessionStats();

    /**
     * @brief Print detailed session report
     */
    std::string getSessionReport() const;

    /**
     * @brief Print visual representation of current session
     */
    std::string getVisualStats() const;

    /**
     * @brief Enable/disable verbose access logging
     */
    void setVerboseLogging(bool enabled) { verbose_logging_ = enabled; }

    /**
     * @brief Get last N access results (for display)
     */
    std::vector<AccessResult> getRecentAccesses(size_t count = 10) const;

    /**
     * @brief Flush all caches
     */
    void flushCaches();

    /**
     * @brief Get component statistics
     */
    std::string getAllStats() const;

    /**
     * @brief Configure cache hierarchy
     */
    void configureCacheL1(size_t sets, size_t associativity,
                          size_t block_size, CachePolicy policy);
    void configureCacheL2(size_t sets, size_t associativity,
                          size_t block_size, CachePolicy policy);

    /**
     * @brief Configure virtual memory
     */
    void configureVM(size_t num_virtual_pages, size_t num_physical_frames,
                     size_t page_size, PageReplacementPolicy policy);

private:
    // Core components
    std::unique_ptr<PhysicalMemory> memory_;
    std::unique_ptr<CacheHierarchy> cache_;
    std::unique_ptr<VirtualMemory> vm_;
    std::unique_ptr<StandardAllocator> allocator_;

    // Configuration
    bool vm_enabled_;
    bool cache_enabled_;
    bool verbose_logging_;
    size_t memory_size_;

    // Session tracking
    SessionStats session_stats_;
    std::vector<AccessResult> access_history_;
    static constexpr size_t MAX_HISTORY_SIZE = 1000;

    // Cache configuration (for lazy initialization)
    struct CacheConfig {
        size_t sets;
        size_t associativity;
        size_t block_size;
        CachePolicy policy;
    };
    CacheConfig l1_config_;
    CacheConfig l2_config_;

    // VM configuration
    struct VMConfig {
        size_t num_virtual_pages;
        size_t num_physical_frames;
        size_t page_size;
        PageReplacementPolicy policy;
    };
    VMConfig vm_config_;

    /**
     * @brief Initialize cache with current configuration
     */
    void initializeCache();

    /**
     * @brief Initialize VM with current configuration
     */
    void initializeVM();

    /**
     * @brief Record an access in history
     */
    void recordAccess(const AccessResult& result);

    /**
     * @brief Determine access level based on cache/memory state
     */
    AccessLevel determineAccessLevel(Address phys_addr, bool is_write);
};

/**
 * @brief Helper function to convert AccessLevel to string
 */
inline std::string accessLevelToString(AccessLevel level) {
    switch (level) {
        case AccessLevel::L1_CACHE: return "L1 Cache";
        case AccessLevel::L2_CACHE: return "L2 Cache";
        case AccessLevel::MEMORY: return "Memory";
        case AccessLevel::PAGE_FAULT: return "Page Fault";
        default: return "Unknown";
    }
}

/**
 * @brief Helper function to get color code for access level
 */
inline std::string getAccessLevelColor(AccessLevel level) {
    switch (level) {
        case AccessLevel::L1_CACHE: return "\033[32m";    // Green
        case AccessLevel::L2_CACHE: return "\033[33m";    // Yellow
        case AccessLevel::MEMORY: return "\033[31m";      // Red
        case AccessLevel::PAGE_FAULT: return "\033[35m";  // Magenta
        default: return "\033[0m";
    }
}

} // namespace memsim

#endif // MEMSIM_SYSTEM_MEMORY_SYSTEM_H
