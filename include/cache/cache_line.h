#ifndef MEMSIM_CACHE_CACHE_LINE_H
#define MEMSIM_CACHE_CACHE_LINE_H

#include "common/types.h"
#include <cstdint>
#include <vector>

namespace memsim {

/**
 * @brief Represents a single line in a cache
 *
 * A cache line stores a block of data from memory along with metadata
 * for replacement policies (FIFO, LRU, LFU).
 */
struct CacheLine {
    bool valid;              // Valid bit (is this line occupied?)
    Address tag;             // Tag bits from address
    std::vector<uint8_t> data; // Data block

    // Metadata for replacement policies
    uint64_t insertion_order;  // For FIFO (lower = older)
    uint64_t last_access_time; // For LRU (lower = older)
    uint64_t access_count;     // For LFU (lower = less frequently used)

    /**
     * @brief Construct an invalid cache line
     */
    CacheLine(size_t block_size)
        : valid(false),
          tag(0),
          data(block_size, 0),
          insertion_order(0),
          last_access_time(0),
          access_count(0) {}

    /**
     * @brief Reset the cache line to invalid state
     */
    void invalidate() {
        valid = false;
        tag = 0;
        insertion_order = 0;
        last_access_time = 0;
        access_count = 0;
    }

    /**
     * @brief Update access metadata for LRU and LFU
     */
    void recordAccess(uint64_t current_time) {
        last_access_time = current_time;
        access_count++;
    }
};

} // namespace memsim

#endif // MEMSIM_CACHE_CACHE_LINE_H
