#ifndef MEMSIM_CACHE_CACHE_HIERARCHY_H
#define MEMSIM_CACHE_CACHE_HIERARCHY_H

#include "common/types.h"
#include "common/result.h"
#include "cache/cache_level.h"
#include "memory/physical_memory.h"
#include <memory>
#include <string>

namespace memsim {

/**
 * @brief Combined statistics for the entire cache hierarchy
 */
struct HierarchyStats {
    CacheStats l1_stats;
    CacheStats l2_stats;
    uint64_t total_accesses;
    uint64_t memory_accesses;  // Number of times we went to main memory

    HierarchyStats()
        : total_accesses(0), memory_accesses(0) {}

    double getOverallHitRatio() const {
        if (total_accesses == 0) return 0.0;
        uint64_t total_hits = l1_stats.hits + l2_stats.hits;
        return (static_cast<double>(total_hits) / total_accesses) * 100.0;
    }
};

/**
 * @brief Manages a two-level cache hierarchy (L1 + L2)
 *
 * Access flow:
 * 1. Check L1 cache
 * 2. On L1 miss, check L2 cache
 * 3. On L2 miss, access main memory
 *
 * Both L1 and L2 use write-through policy.
 */
class CacheHierarchy {
public:
    /**
     * @brief Construct cache hierarchy with L1 and L2
     *
     * @param memory Pointer to physical memory
     * @param l1_sets Number of sets in L1
     * @param l1_associativity Associativity of L1
     * @param l1_block_size Block size of L1
     * @param l1_policy Replacement policy for L1
     * @param l2_sets Number of sets in L2
     * @param l2_associativity Associativity of L2
     * @param l2_block_size Block size of L2
     * @param l2_policy Replacement policy for L2
     */
    CacheHierarchy(PhysicalMemory* memory,
                   size_t l1_sets, size_t l1_associativity,
                   size_t l1_block_size, CachePolicy l1_policy,
                   size_t l2_sets, size_t l2_associativity,
                   size_t l2_block_size, CachePolicy l2_policy);

    ~CacheHierarchy() = default;

    /**
     * @brief Read data through cache hierarchy
     *
     * Checks L1, then L2, then memory on misses.
     *
     * @param address Physical address to read
     * @return Result containing data byte, or error
     */
    Result<uint8_t> read(Address address);

    /**
     * @brief Write data through cache hierarchy
     *
     * Writes to memory and updates caches (write-through).
     *
     * @param address Physical address to write
     * @param data Data byte to write
     * @return Result indicating success or error
     */
    Result<void> write(Address address, uint8_t data);

    /**
     * @brief Flush all caches
     */
    void flush();

    /**
     * @brief Get combined statistics
     */
    HierarchyStats getStats() const;

    /**
     * @brief Get formatted statistics string
     */
    std::string getStatsString() const;

    /**
     * @brief Dump all cache levels
     */
    void dump() const;

    /**
     * @brief Get L1 cache (for direct testing)
     */
    CacheLevel* getL1() { return l1_cache_.get(); }

    /**
     * @brief Get L2 cache (for direct testing)
     */
    CacheLevel* getL2() { return l2_cache_.get(); }

    /**
     * @brief Check if address is in L1 cache
     */
    bool containsInL1(Address address) const;

    /**
     * @brief Check if address is in L2 cache
     */
    bool containsInL2(Address address) const;

private:
    PhysicalMemory* memory_;
    std::unique_ptr<CacheLevel> l1_cache_;
    std::unique_ptr<CacheLevel> l2_cache_;
    uint64_t memory_access_count_;
};

} // namespace memsim

#endif // MEMSIM_CACHE_CACHE_HIERARCHY_H
