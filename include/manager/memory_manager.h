#ifndef MEMSIM_MANAGER_MEMORY_MANAGER_H
#define MEMSIM_MANAGER_MEMORY_MANAGER_H

#include "memory/physical_memory.h"
#include "allocator/allocator_interface.h"
#include "allocator/standard_allocator.h"
#include "allocator/buddy_allocator.h"
#include "virtual_memory/virtual_memory.h"
#include "cache/cache_hierarchy.h"
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

    /**
     * @brief Initialize virtual memory system
     * @param num_virtual_pages Number of virtual pages
     * @param num_physical_frames Number of physical frames
     * @param page_size Size of each page in bytes
     * @param policy Page replacement policy
     * @return Result indicating success or failure
     */
    Result<void> initVirtualMemory(size_t num_virtual_pages,
                                    size_t num_physical_frames,
                                    size_t page_size,
                                    PageReplacementPolicy policy);

    /**
     * @brief Read from virtual address
     * @param virtual_addr Virtual address to read from
     * @return Result containing data byte
     */
    Result<uint8_t> vmRead(Address virtual_addr);

    /**
     * @brief Write to virtual address
     * @param virtual_addr Virtual address to write to
     * @param data Data byte to write
     * @return Result indicating success or failure
     */
    Result<void> vmWrite(Address virtual_addr, uint8_t data);

    /**
     * @brief Translate virtual address to physical
     * @param virtual_addr Virtual address to translate
     * @return Result containing physical address
     */
    Result<Address> vmTranslate(Address virtual_addr);

    /**
     * @brief Print virtual memory statistics
     */
    void printVMStats() const;

    /**
     * @brief Dump virtual memory page table
     */
    void dumpVM() const;

    /**
     * @brief Check if virtual memory is initialized
     * @return true if VM is initialized
     */
    bool isVMInitialized() const { return virtual_memory_ != nullptr; }

    /**
     * @brief Initialize cache hierarchy
     * @param l1_sets Number of sets in L1 cache
     * @param l1_assoc Associativity of L1 cache
     * @param l1_block_size Block size of L1 cache
     * @param l1_policy Replacement policy for L1
     * @param l2_sets Number of sets in L2 cache
     * @param l2_assoc Associativity of L2 cache
     * @param l2_block_size Block size of L2 cache
     * @param l2_policy Replacement policy for L2
     * @return Result indicating success or failure
     */
    Result<void> initCache(size_t l1_sets, size_t l1_assoc, size_t l1_block_size, CachePolicy l1_policy,
                           size_t l2_sets, size_t l2_assoc, size_t l2_block_size, CachePolicy l2_policy);

    /**
     * @brief Read from cache
     * @param address Physical address to read from
     * @return Result containing data byte
     */
    Result<uint8_t> cacheRead(Address address);

    /**
     * @brief Write to cache
     * @param address Physical address to write to
     * @param data Data byte to write
     * @return Result indicating success or failure
     */
    Result<void> cacheWrite(Address address, uint8_t data);

    /**
     * @brief Print cache statistics
     */
    void printCacheStats() const;

    /**
     * @brief Dump cache contents
     */
    void dumpCache() const;

    /**
     * @brief Flush cache
     */
    void flushCache();

    /**
     * @brief Check if cache is initialized
     * @return true if cache is initialized
     */
    bool isCacheInitialized() const { return cache_ != nullptr; }

private:
    std::unique_ptr<PhysicalMemory> physical_memory_;
    std::unique_ptr<IAllocator> allocator_;
    std::unique_ptr<VirtualMemory> virtual_memory_;
    std::unique_ptr<CacheHierarchy> cache_;
    AllocatorType current_allocator_type_;
};

} // namespace memsim

#endif // MEMSIM_MANAGER_MEMORY_MANAGER_H
