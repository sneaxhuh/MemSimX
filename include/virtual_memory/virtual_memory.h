#ifndef MEMSIM_VIRTUAL_MEMORY_VIRTUAL_MEMORY_H
#define MEMSIM_VIRTUAL_MEMORY_VIRTUAL_MEMORY_H

#include "common/types.h"
#include "common/result.h"
#include "virtual_memory/page_table_entry.h"
#include "memory/physical_memory.h"
#include <vector>
#include <queue>
#include <string>
#include <cstdint>

namespace memsim {

/**
 * @brief Statistics for virtual memory system
 */
struct VirtualMemoryStats {
    uint64_t page_faults;
    uint64_t page_hits;
    uint64_t total_accesses;

    VirtualMemoryStats() : page_faults(0), page_hits(0), total_accesses(0) {}

    double getPageFaultRate() const {
        if (total_accesses == 0) return 0.0;
        return (static_cast<double>(page_faults) / total_accesses) * 100.0;
    }

    double getPageHitRate() const {
        if (total_accesses == 0) return 0.0;
        return (static_cast<double>(page_hits) / total_accesses) * 100.0;
    }
};

/**
 * @brief Virtual memory system with paging and page replacement
 *
 * Provides address translation from virtual addresses to physical addresses
 * using a page table. Implements page replacement policies (FIFO, LRU, Clock)
 * when physical memory is full.
 *
 * Virtual Address format:
 * | Page Number | Page Offset |
 *
 * Physical Address format:
 * | Frame Number | Page Offset |
 */
class VirtualMemory {
public:
    /**
     * @brief Construct virtual memory system
     *
     * @param memory Pointer to physical memory
     * @param num_virtual_pages Total number of virtual pages
     * @param num_physical_frames Number of physical frames available
     * @param page_size Size of each page in bytes (must be power of 2)
     * @param policy Page replacement policy
     */
    VirtualMemory(PhysicalMemory* memory,
                  size_t num_virtual_pages,
                  size_t num_physical_frames,
                  size_t page_size,
                  PageReplacementPolicy policy);

    ~VirtualMemory() = default;

    /**
     * @brief Translate virtual address to physical address
     *
     * If page is not in memory (page fault), load it using replacement policy.
     *
     * @param virtual_addr Virtual address to translate
     * @return Result containing physical address, or error
     */
    Result<Address> translate(Address virtual_addr);

    /**
     * @brief Read data through virtual memory
     *
     * Translates virtual address to physical, then reads from physical memory.
     *
     * @param virtual_addr Virtual address to read from
     * @return Result containing data byte, or error
     */
    Result<uint8_t> read(Address virtual_addr);

    /**
     * @brief Write data through virtual memory
     *
     * Translates virtual address to physical, then writes to physical memory.
     * Marks page as dirty.
     *
     * @param virtual_addr Virtual address to write to
     * @param data Data byte to write
     * @return Result indicating success or error
     */
    Result<void> write(Address virtual_addr, uint8_t data);

    /**
     * @brief Flush all pages (mark all as invalid)
     */
    void flush();

    /**
     * @brief Get virtual memory statistics
     */
    VirtualMemoryStats getStats() const { return stats_; }

    /**
     * @brief Get formatted statistics string
     */
    std::string getStatsString() const;

    /**
     * @brief Dump page table contents
     */
    void dump() const;

    /**
     * @brief Get configuration string
     */
    std::string getConfigString() const;

private:
    PhysicalMemory* memory_;
    size_t num_virtual_pages_;
    size_t num_physical_frames_;
    size_t page_size_;
    PageReplacementPolicy policy_;

    // Page table: virtual page number -> PageTableEntry
    std::vector<PageTableEntry> page_table_;

    // Frame tracking: which frames are currently free?
    std::vector<bool> frame_allocated_;  // true if frame is in use

    // Page replacement data structures
    std::queue<size_t> fifo_queue_;      // For FIFO: queue of page numbers
    size_t clock_hand_;                   // For Clock: current position

    // Statistics and time tracking
    VirtualMemoryStats stats_;
    uint64_t global_time_;

    // Address parsing
    size_t offset_bits_;                  // Number of bits for page offset
    size_t page_number_bits_;             // Number of bits for page number

    /**
     * @brief Parse virtual address into page number and offset
     */
    void parseAddress(Address virtual_addr, size_t& page_number, size_t& offset) const;

    /**
     * @brief Construct physical address from frame number and offset
     */
    Address constructPhysicalAddress(Address frame_number, size_t offset) const;

    /**
     * @brief Handle page fault - load page into physical memory
     *
     * @param page_number Virtual page number to load
     * @return Result containing frame number where page was loaded
     */
    Result<Address> handlePageFault(size_t page_number);

    /**
     * @brief Select victim page for eviction
     *
     * Uses configured page replacement policy (FIFO, LRU, Clock).
     *
     * @return Page number to evict
     */
    size_t selectVictimPage();

    /**
     * @brief Evict a page from physical memory
     *
     * @param page_number Page to evict
     */
    void evictPage(size_t page_number);

    /**
     * @brief Find a free physical frame
     *
     * @return Frame number if available, or error if all frames allocated
     */
    Result<Address> findFreeFrame();

    /**
     * @brief Load page data from "disk" into physical frame
     *
     * Simulates loading page from secondary storage.
     *
     * @param page_number Virtual page to load
     * @param frame_number Physical frame to load into
     */
    void loadPageFromDisk(size_t page_number, Address frame_number);

    /**
     * @brief Write page data to "disk"
     *
     * Simulates writing dirty page back to secondary storage.
     *
     * @param page_number Virtual page to write
     * @param frame_number Physical frame to read from
     */
    void writePageToDisk(size_t page_number, Address frame_number);

    /**
     * @brief Check if value is power of 2
     */
    static bool isPowerOfTwo(size_t value);

    /**
     * @brief Calculate number of bits needed to represent a value
     */
    static size_t calculateBits(size_t value);
};

} // namespace memsim

#endif // MEMSIM_VIRTUAL_MEMORY_VIRTUAL_MEMORY_H
