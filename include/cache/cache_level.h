#ifndef MEMSIM_CACHE_CACHE_LEVEL_H
#define MEMSIM_CACHE_CACHE_LEVEL_H

#include "common/types.h"
#include "common/result.h"
#include "cache/cache_line.h"
#include "memory/physical_memory.h"
#include <vector>
#include <string>
#include <cstdint>

namespace memsim {

/**
 * @brief Statistics for a cache level
 */
struct CacheStats {
    uint64_t hits;
    uint64_t misses;
    uint64_t accesses;

    CacheStats() : hits(0), misses(0), accesses(0) {}

    double getHitRatio() const {
        if (accesses == 0) return 0.0;
        return (static_cast<double>(hits) / accesses) * 100.0;
    }

    double getMissRatio() const {
        if (accesses == 0) return 0.0;
        return (static_cast<double>(misses) / accesses) * 100.0;
    }
};

/**
 * @brief Represents a single level of cache (L1 or L2)
 *
 * Supports direct-mapped and N-way set-associative caches
 * with FIFO, LRU, and LFU replacement policies.
 *
 * Address breakdown:
 * | Tag | Set Index | Block Offset |
 */
class CacheLevel {
public:
    /**
     * @brief Construct a cache level
     *
     * @param level Cache level (1 for L1, 2 for L2)
     * @param num_sets Number of sets in the cache
     * @param associativity Number of lines per set (1 = direct-mapped)
     * @param block_size Size of each cache line in bytes
     * @param policy Replacement policy (FIFO, LRU, LFU)
     * @param memory Pointer to physical memory (for fetching on miss)
     */
    CacheLevel(int level,
               size_t num_sets,
               size_t associativity,
               size_t block_size,
               CachePolicy policy,
               PhysicalMemory* memory);

    ~CacheLevel() = default;

    /**
     * @brief Read data from cache
     *
     * If data is not in cache (miss), fetch from memory.
     *
     * @param address Physical address to read
     * @return Result containing the data byte, or error
     */
    Result<uint8_t> read(Address address);

    /**
     * @brief Write data to cache
     *
     * Uses write-through policy (write to cache and memory).
     *
     * @param address Physical address to write
     * @param data Data byte to write
     * @return Result indicating success or error
     */
    Result<void> write(Address address, uint8_t data);

    /**
     * @brief Check if address is in cache (without updating stats)
     *
     * @param address Physical address to check
     * @return true if in cache, false otherwise
     */
    bool contains(Address address) const;

    /**
     * @brief Invalidate all cache lines
     */
    void flush();

    /**
     * @brief Get cache statistics
     */
    CacheStats getStats() const { return stats_; }

    /**
     * @brief Get formatted statistics string
     */
    std::string getStatsString() const;

    /**
     * @brief Dump cache contents (for visualization)
     */
    void dump() const;

    /**
     * @brief Get cache configuration info
     */
    std::string getConfigString() const;

private:
    int level_;                    // Cache level (1 or 2)
    size_t num_sets_;              // Number of sets
    size_t associativity_;         // Lines per set
    size_t block_size_;            // Bytes per line
    CachePolicy policy_;           // Replacement policy
    PhysicalMemory* memory_;       // Physical memory reference

    // Cache storage: sets[set_index][way] = CacheLine
    std::vector<std::vector<CacheLine>> sets_;

    // Statistics
    CacheStats stats_;
    uint64_t global_time_;         // For LRU timestamps

    // Address parsing bit counts
    size_t offset_bits_;           // Block offset bits
    size_t index_bits_;            // Set index bits

    /**
     * @brief Parse address into tag, set index, and block offset
     */
    void parseAddress(Address address, Address& tag, size_t& set_index, size_t& offset) const;

    /**
     * @brief Find line in set matching tag
     *
     * @return Pointer to line if found, nullptr otherwise
     */
    CacheLine* findLine(size_t set_index, Address tag);

    /**
     * @brief Select victim line for replacement
     *
     * Uses the configured replacement policy (FIFO, LRU, LFU).
     *
     * @return Index of victim line in the set
     */
    size_t selectVictim(size_t set_index);

    /**
     * @brief Load block from memory into cache
     *
     * @param address Address to load (will be aligned to block boundary)
     * @param tag Tag to store in cache line
     * @param set_index Set to load into
     * @param way_index Way (line) within set
     */
    void loadBlock(Address address, Address tag, size_t set_index, size_t way_index);

    /**
     * @brief Calculate number of bits needed to represent a value
     */
    static size_t calculateBits(size_t value);

    /**
     * @brief Check if value is power of 2
     */
    static bool isPowerOfTwo(size_t value);
};

} // namespace memsim

#endif // MEMSIM_CACHE_CACHE_LEVEL_H
