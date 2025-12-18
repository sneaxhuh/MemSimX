#ifndef MEMSIM_ALLOCATOR_STANDARD_ALLOCATOR_H
#define MEMSIM_ALLOCATOR_STANDARD_ALLOCATOR_H

#include "allocator/allocator_interface.h"
#include "allocator/memory_block.h"
#include "memory/physical_memory.h"
#include <unordered_map>

namespace memsim {

/**
 * @brief Standard allocator implementing First Fit, Best Fit, and Worst Fit
 *
 * This allocator maintains a doubly-linked list of memory blocks and implements
 * three classic allocation strategies. It supports block splitting when an
 * allocated block is larger than requested, and automatic coalescing of adjacent
 * free blocks during deallocation.
 */
class StandardAllocator : public IAllocator {
public:
    /**
     * @brief Construct a StandardAllocator
     * @param memory Pointer to physical memory
     * @param type Allocation strategy (FIRST_FIT, BEST_FIT, or WORST_FIT)
     */
    StandardAllocator(PhysicalMemory* memory, AllocatorType type);

    /**
     * @brief Destructor - cleans up all memory blocks
     */
    ~StandardAllocator() override;

    // Disable copy and move
    StandardAllocator(const StandardAllocator&) = delete;
    StandardAllocator& operator=(const StandardAllocator&) = delete;
    StandardAllocator(StandardAllocator&&) = delete;
    StandardAllocator& operator=(StandardAllocator&&) = delete;

    // IAllocator interface implementation
    Result<BlockId> allocate(size_t size) override;
    Result<void> deallocate(BlockId block_id) override;
    Result<void> deallocateByAddress(Address address) override;
    void dump() const override;
    std::string getStats() const override;
    double getInternalFragmentation() const override;
    double getExternalFragmentation() const override;
    double getUtilization() const override;
    AllocatorType getType() const override { return strategy_; }

private:
    PhysicalMemory* physical_memory_;  // Pointer to physical memory
    MemoryBlock* head_;                 // Head of doubly-linked list
    AllocatorType strategy_;            // Allocation strategy
    BlockId next_block_id_;             // Next available block ID

    // Maps for quick lookups
    std::unordered_map<BlockId, MemoryBlock*> allocated_blocks_;
    std::unordered_map<Address, MemoryBlock*> address_to_block_;

    // Metrics tracking
    size_t total_allocations_;
    size_t failed_allocations_;
    size_t total_deallocations_;

    // Track requested vs allocated sizes for internal fragmentation
    std::unordered_map<BlockId, size_t> requested_sizes_;

    /**
     * @brief Find a suitable free block for allocation
     * @param size Size of block needed
     * @return Pointer to suitable block, or nullptr if none found
     */
    MemoryBlock* findBlock(size_t size);

    /**
     * @brief Split a block if it's larger than needed
     * @param block Block to potentially split
     * @param size Size needed
     *
     * If the block is larger than size, splits it into two blocks:
     * one of the requested size and one free block for the remainder.
     */
    void splitBlock(MemoryBlock* block, size_t size);

    /**
     * @brief Coalesce a block with adjacent free blocks
     * @param block Block to coalesce
     *
     * Merges the block with its free neighbors to reduce fragmentation.
     */
    void coalesceBlock(MemoryBlock* block);

    /**
     * @brief Remove a block from the linked list
     * @param block Block to remove
     */
    void removeBlock(MemoryBlock* block);

    /**
     * @brief Insert a block after another block in the list
     * @param after Insert after this block (or nullptr to insert at head)
     * @param block Block to insert
     */
    void insertBlockAfter(MemoryBlock* after, MemoryBlock* block);

    /**
     * @brief Calculate total free memory
     * @return Total free memory in bytes
     */
    size_t getTotalFreeMemory() const;

    /**
     * @brief Find the largest free block
     * @return Size of largest free block
     */
    size_t getLargestFreeBlock() const;

    /**
     * @brief Count number of free blocks
     * @return Number of free blocks
     */
    size_t countFreeBlocks() const;

    /**
     * @brief Count number of allocated blocks
     * @return Number of allocated blocks
     */
    size_t countAllocatedBlocks() const;
};

} // namespace memsim

#endif // MEMSIM_ALLOCATOR_STANDARD_ALLOCATOR_H
