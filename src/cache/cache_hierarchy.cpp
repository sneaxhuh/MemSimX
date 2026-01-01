#include "cache/cache_hierarchy.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace memsim {

CacheHierarchy::CacheHierarchy(PhysicalMemory* memory,
                               size_t l1_sets, size_t l1_associativity,
                               size_t l1_block_size, CachePolicy l1_policy,
                               size_t l2_sets, size_t l2_associativity,
                               size_t l2_block_size, CachePolicy l2_policy)
    : memory_(memory),
      memory_access_count_(0) {

    // Create L1 and L2 caches
    l1_cache_ = std::make_unique<CacheLevel>(
        1, l1_sets, l1_associativity, l1_block_size, l1_policy, memory
    );

    l2_cache_ = std::make_unique<CacheLevel>(
        2, l2_sets, l2_associativity, l2_block_size, l2_policy, memory
    );
}

Result<uint8_t> CacheHierarchy::read(Address address) {
    // Try L1 first
    if (l1_cache_->contains(address)) {
        return l1_cache_->read(address);
    }

    // L1 miss - try L2
    if (l2_cache_->contains(address)) {
        auto result = l2_cache_->read(address);
        if (result.success) {
            // Load into L1 as well
            l1_cache_->write(address, result.value);
        }
        return result;
    }

    // L2 miss - access memory
    memory_access_count_++;
    auto result = memory_->read(address);
    if (result.success) {
        // Load into both L2 and L1
        l2_cache_->write(address, result.value);
        l1_cache_->write(address, result.value);
    }
    return result;
}

Result<void> CacheHierarchy::write(Address address, uint8_t data) {
    // Write-through: write to memory first
    auto mem_result = memory_->write(address, data);
    if (!mem_result.success) {
        return mem_result;
    }

    // Update L1 if present
    if (l1_cache_->contains(address)) {
        l1_cache_->write(address, data);
    }

    // Update L2 if present
    if (l2_cache_->contains(address)) {
        l2_cache_->write(address, data);
    }

    return Result<void>::Ok();
}

void CacheHierarchy::flush() {
    l1_cache_->flush();
    l2_cache_->flush();
}

HierarchyStats CacheHierarchy::getStats() const {
    HierarchyStats stats;
    stats.l1_stats = l1_cache_->getStats();
    stats.l2_stats = l2_cache_->getStats();
    stats.total_accesses = stats.l1_stats.accesses + stats.l2_stats.accesses;
    stats.memory_accesses = memory_access_count_;
    return stats;
}

std::string CacheHierarchy::getStatsString() const {
    std::ostringstream oss;
    auto stats = getStats();

    oss << "=== Cache Hierarchy Statistics ===\n\n";

    // L1 stats
    oss << l1_cache_->getStatsString() << "\n";

    // L2 stats
    oss << l2_cache_->getStatsString() << "\n";

    // Overall stats
    oss << "=== Overall Statistics ===\n";
    oss << "Total Accesses: " << stats.total_accesses << "\n";
    oss << "L1 Hits: " << stats.l1_stats.hits << "\n";
    oss << "L2 Hits: " << stats.l2_stats.hits << "\n";
    oss << "Memory Accesses: " << stats.memory_accesses << "\n";
    oss << "Overall Hit Ratio: " << std::fixed << std::setprecision(2)
        << stats.getOverallHitRatio() << "%\n";

    return oss.str();
}

void CacheHierarchy::dump() const {
    l1_cache_->dump();
    std::cout << "\n";
    l2_cache_->dump();
}

bool CacheHierarchy::containsInL1(Address address) const {
    return l1_cache_->contains(address);
}

bool CacheHierarchy::containsInL2(Address address) const {
    return l2_cache_->contains(address);
}

} // namespace memsim
