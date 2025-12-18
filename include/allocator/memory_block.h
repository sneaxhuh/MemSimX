#ifndef MEMSIM_ALLOCATOR_MEMORY_BLOCK_H
#define MEMSIM_ALLOCATOR_MEMORY_BLOCK_H

#include "common/types.h"
#include <cstddef>

namespace memsim {

/**
 * @brief Represents a block of memory in the allocator
 *
 * This structure is used to maintain a doubly-linked list of memory blocks.
 * Each block can be either free or allocated.
 */
struct MemoryBlock {
    Address start_address;  // Starting address in physical memory
    size_t size;            // Size of this block in bytes
    bool is_free;           // true if block is free, false if allocated
    BlockId id;             // Unique identifier for allocated blocks (0 for free blocks)
    MemoryBlock* next;      // Next block in the list
    MemoryBlock* prev;      // Previous block in the list

    /**
     * @brief Construct a new MemoryBlock
     * @param addr Starting address
     * @param sz Size in bytes
     * @param free Whether the block is free
     */
    MemoryBlock(Address addr, size_t sz, bool free)
        : start_address(addr),
          size(sz),
          is_free(free),
          id(0),
          next(nullptr),
          prev(nullptr) {
    }

    /**
     * @brief Get the ending address of this block (exclusive)
     * @return End address
     */
    Address endAddress() const {
        return start_address + size;
    }

    /**
     * @brief Check if this block is adjacent to another block
     * @param other The other block to check
     * @return true if blocks are adjacent
     */
    bool isAdjacentTo(const MemoryBlock* other) const {
        if (!other) return false;
        return (this->endAddress() == other->start_address) ||
               (other->endAddress() == this->start_address);
    }

    /**
     * @brief Check if this block comes immediately before another block
     * @param other The other block to check
     * @return true if this block is immediately before other
     */
    bool comesBeforeAdjacent(const MemoryBlock* other) const {
        if (!other) return false;
        return this->endAddress() == other->start_address;
    }
};

} // namespace memsim

#endif // MEMSIM_ALLOCATOR_MEMORY_BLOCK_H
