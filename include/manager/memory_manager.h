#ifndef MEMSIM_MANAGER_MEMORY_MANAGER_H
#define MEMSIM_MANAGER_MEMORY_MANAGER_H

#include "memory/physical_memory.h"
#include "allocator/allocator_interface.h"
#include "allocator/standard_allocator.h"
#include "common/types.h"
#include "common/result.h"
#include <memory>

namespace memsim {

/**
 * @brief Central orchestrator for the memory management system
 *
 * The MemoryManager coordinates all subsystems including physical memory,
 * allocators, cache (future), and virtual memory (future). It provides a
 * high-level interface for the CLI.
 */
class MemoryManager {
public:
    /**
     * @brief Construct a MemoryManager
     */
    MemoryManager();

    /**
     * @brief Destructor
     */
    ~MemoryManager() = default;

    // Disable copy and move
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    MemoryManager(MemoryManager&&) = delete;
    MemoryManager& operator=(MemoryManager&&) = delete;

    /**
     * @brief Initialize physical memory
     * @param size Size of physical memory in bytes
     * @return Result indicating success or failure
     */
    Result<void> initMemory(size_t size);

    /**
     * @brief Set the allocator type
     * @param type Allocator type to use
     * @return Result indicating success or failure
     */
    Result<void> setAllocator(AllocatorType type);

    /**
     * @brief Allocate memory
     * @param size Size to allocate in bytes
     * @return Result containing BlockId on success
     */
    Result<BlockId> malloc(size_t size);

    /**
     * @brief Free memory by block ID
     * @param block_id Block ID to free
     * @return Result indicating success or failure
     */
    Result<void> free(BlockId block_id);

    /**
     * @brief Free memory by address
     * @param address Address to free
     * @return Result indicating success or failure
     */
    Result<void> freeByAddress(Address address);

    /**
     * @brief Dump memory layout
     */
    void dumpMemory() const;

    /**
     * @brief Print statistics
     */
    void printStats() const;

    /**
     * @brief Check if memory is initialized
     * @return true if memory is initialized
     */
    bool isMemoryInitialized() const { return physical_memory_ != nullptr; }

    /**
     * @brief Check if allocator is set
     * @return true if allocator is set
     */
    bool isAllocatorSet() const { return allocator_ != nullptr; }

    /**
     * @brief Get current allocator type
     * @return Current allocator type, or FIRST_FIT if not set
     */
    AllocatorType getCurrentAllocatorType() const {
        return current_allocator_type_;
    }

private:
    std::unique_ptr<PhysicalMemory> physical_memory_;
    std::unique_ptr<IAllocator> allocator_;
    AllocatorType current_allocator_type_;
};

} // namespace memsim

#endif // MEMSIM_MANAGER_MEMORY_MANAGER_H
