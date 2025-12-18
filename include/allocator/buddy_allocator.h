#ifndef MEMSIM_ALLOCATOR_BUDDY_ALLOCATOR_H
#define MEMSIM_ALLOCATOR_BUDDY_ALLOCATOR_H

#include "allocator/allocator_interface.h"
#include "memory/physical_memory.h"
#include <map>
#include <list>
#include <unordered_map>

namespace memsim {

/**
 * @brief Represents a buddy memory block
 */
struct BuddyBlock {
    Address start_address;  // Starting address in physical memory
    size_t size;            // Size of this block (always power of 2)
    bool is_free;           // true if block is free
    BlockId id;             // Unique identifier for allocated blocks (0 for free blocks)

    BuddyBlock(Address addr, size_t sz, bool free)
        : start_address(addr), size(sz), is_free(free), id(0) {}

    Address endAddress() const {
        return start_address + size;
    }
};

/**
 * @brief Buddy memory allocator implementing power-of-two allocation
 *
 * The buddy allocator maintains free lists for each power-of-two block size.
 * When allocating, it rounds the requested size up to the nearest power of two
 * and recursively splits larger blocks if necessary. When freeing, it attempts
 * to coalesce with its buddy block to form larger free blocks.
 *
 * Key properties:
 * - Memory size must be a power of 2
 * - All block sizes are powers of 2
 * - Buddy address can be computed using XOR: buddy_addr = addr XOR size
 * - Coalescing is recursive (merge up to largest possible block)
 */
class BuddyAllocator : public IAllocator {
public:
    /**
     * @brief Construct a BuddyAllocator
     * @param memory Pointer to physical memory (size must be power of 2)
     * @param min_block_size Minimum allocatable block size (power of 2)
     *
     * @throws std::invalid_argument if memory size is not power of 2
     */
    BuddyAllocator(PhysicalMemory* memory, size_t min_block_size = 32);

    /**
     * @brief Destructor - cleans up all buddy blocks
     */
    ~BuddyAllocator() override;

    // Disable copy and move
    BuddyAllocator(const BuddyAllocator&) = delete;
    BuddyAllocator& operator=(const BuddyAllocator&) = delete;
    BuddyAllocator(BuddyAllocator&&) = delete;
    BuddyAllocator& operator=(BuddyAllocator&&) = delete;

    // IAllocator interface implementation
    Result<BlockId> allocate(size_t size) override;
    Result<void> deallocate(BlockId block_id) override;
    Result<void> deallocateByAddress(Address address) override;
    void dump() const override;
    std::string getStats() const override;
    double getInternalFragmentation() const override;
    double getExternalFragmentation() const override;
    double getUtilization() const override;
    AllocatorType getType() const override { return AllocatorType::BUDDY; }

private:
    PhysicalMemory* physical_memory_;
    size_t min_block_size_;  // Minimum allocatable block size
    size_t max_block_size_;  // Total memory size (power of 2)

    // Free lists: free_lists_[size] contains list of free blocks of that size
    std::map<size_t, std::list<BuddyBlock*>> free_lists_;

    // Quick lookup maps
    std::unordered_map<BlockId, BuddyBlock*> allocated_blocks_;
    std::unordered_map<Address, BuddyBlock*> address_to_block_;

    BlockId next_block_id_;

    // Metrics
    size_t total_allocations_;
    size_t failed_allocations_;
    size_t total_deallocations_;

    // Track requested vs allocated sizes for internal fragmentation
    std::unordered_map<BlockId, size_t> requested_sizes_;

    /**
     * @brief Round size up to nearest power of 2
     * @param size Size to round
     * @return Rounded size (power of 2)
     */
    size_t roundUpToPowerOfTwo(size_t size) const;

    /**
     * @brief Check if a number is a power of 2
     * @param n Number to check
     * @return true if n is a power of 2
     */
    bool isPowerOfTwo(size_t n) const;

    /**
     * @brief Calculate log2 of a power-of-2 number
     * @param n Power-of-2 number
     * @return log2(n)
     */
    size_t log2(size_t n) const;

    /**
     * @brief Calculate buddy address using XOR
     * @param addr Block address
     * @param size Block size
     * @return Address of buddy block
     */
    Address getBuddyAddress(Address addr, size_t size) const;

    /**
     * @brief Split a block recursively to get desired size
     * @param current_size Size of block to split
     * @param target_size Desired size
     * @return Pointer to block of target size, or nullptr if not available
     */
    BuddyBlock* splitBlock(size_t current_size, size_t target_size);

    /**
     * @brief Try to coalesce a block with its buddy
     * @param block Block to coalesce
     *
     * Recursively merges the block with its buddy if the buddy is free
     * and has the same size.
     */
    void coalesceBlock(BuddyBlock* block);

    /**
     * @brief Find a free block in the free list
     * @param size Size of block to find
     * @return Pointer to free block, or nullptr if not found
     */
    BuddyBlock* findFreeBlock(size_t size);

    /**
     * @brief Remove a block from the free list
     * @param block Block to remove
     */
    void removeFromFreeList(BuddyBlock* block);

    /**
     * @brief Add a block to the free list
     * @param block Block to add
     */
    void addToFreeList(BuddyBlock* block);

    /**
     * @brief Calculate total free memory
     * @return Total free memory in bytes
     */
    size_t getTotalFreeMemory() const;

    /**
     * @brief Find the largest free block size
     * @return Size of largest free block
     */
    size_t getLargestFreeBlock() const;
};

} // namespace memsim

#endif // MEMSIM_ALLOCATOR_BUDDY_ALLOCATOR_H
