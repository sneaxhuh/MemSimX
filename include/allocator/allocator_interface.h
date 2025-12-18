#ifndef MEMSIM_ALLOCATOR_ALLOCATOR_INTERFACE_H
#define MEMSIM_ALLOCATOR_ALLOCATOR_INTERFACE_H

#include "common/types.h"
#include "common/result.h"
#include <string>

namespace memsim {

/**
 * @brief Interface for memory allocators
 *
 * This interface defines the contract that all memory allocators must implement.
 * It uses the Strategy pattern to allow different allocation algorithms to be
 * used interchangeably.
 */
class IAllocator {
public:
    virtual ~IAllocator() = default;

    /**
     * @brief Allocate a block of memory
     * @param size Size of memory to allocate in bytes
     * @return Result containing BlockId on success, or error message on failure
     */
    virtual Result<BlockId> allocate(size_t size) = 0;

    /**
     * @brief Deallocate a block of memory by block ID
     * @param block_id The ID of the block to deallocate
     * @return Result indicating success or failure
     */
    virtual Result<void> deallocate(BlockId block_id) = 0;

    /**
     * @brief Deallocate a block of memory by address
     * @param address The starting address of the block to deallocate
     * @return Result indicating success or failure
     */
    virtual Result<void> deallocateByAddress(Address address) = 0;

    /**
     * @brief Dump the current state of memory allocation
     *
     * Prints a visualization of allocated and free blocks
     */
    virtual void dump() const = 0;

    /**
     * @brief Get statistics as a formatted string
     * @return String containing allocation statistics
     */
    virtual std::string getStats() const = 0;

    /**
     * @brief Calculate internal fragmentation
     * @return Internal fragmentation as a percentage (0-100)
     *
     * Internal fragmentation = wasted space within allocated blocks
     */
    virtual double getInternalFragmentation() const = 0;

    /**
     * @brief Calculate external fragmentation
     * @return External fragmentation as a percentage (0-100)
     *
     * External fragmentation = largest free block / total free memory
     */
    virtual double getExternalFragmentation() const = 0;

    /**
     * @brief Calculate memory utilization
     * @return Memory utilization as a percentage (0-100)
     *
     * Utilization = used memory / total memory
     */
    virtual double getUtilization() const = 0;

    /**
     * @brief Get the type of this allocator
     * @return AllocatorType enum value
     */
    virtual AllocatorType getType() const = 0;
};

} // namespace memsim

#endif // MEMSIM_ALLOCATOR_ALLOCATOR_INTERFACE_H
