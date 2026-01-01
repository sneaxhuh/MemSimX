#ifndef MEMSIM_VIRTUAL_MEMORY_PAGE_TABLE_ENTRY_H
#define MEMSIM_VIRTUAL_MEMORY_PAGE_TABLE_ENTRY_H

#include "common/types.h"
#include <cstdint>

namespace memsim {

/**
 * @brief Entry in a page table
 *
 * Each page table entry maps a virtual page number to a physical frame number
 * and stores metadata for page replacement policies.
 */
struct PageTableEntry {
    bool valid;              // Is this page currently in physical memory?
    Address frame_number;    // Physical frame number (if valid)
    bool dirty;              // Has this page been modified?
    bool referenced;         // Has this page been accessed?

    // Metadata for page replacement policies
    uint64_t load_time;      // When was this page loaded? (for FIFO)
    uint64_t last_access;    // When was this page last accessed? (for LRU)

    /**
     * @brief Construct an invalid page table entry
     */
    PageTableEntry()
        : valid(false),
          frame_number(0),
          dirty(false),
          referenced(false),
          load_time(0),
          last_access(0) {}

    /**
     * @brief Reset entry to invalid state
     */
    void invalidate() {
        valid = false;
        frame_number = 0;
        dirty = false;
        referenced = false;
        load_time = 0;
        last_access = 0;
    }

    /**
     * @brief Mark page as accessed (update metadata)
     */
    void recordAccess(uint64_t current_time) {
        referenced = true;
        last_access = current_time;
    }
};

} // namespace memsim

#endif // MEMSIM_VIRTUAL_MEMORY_PAGE_TABLE_ENTRY_H
