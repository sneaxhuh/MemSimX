#ifndef MEMSIM_MEMORY_PHYSICAL_MEMORY_H
#define MEMSIM_MEMORY_PHYSICAL_MEMORY_H

#include <vector>
#include <cstddef>
#include "common/types.h"
#include "common/result.h"

namespace memsim {

/**
 * @brief Simulates physical memory as a contiguous block of bytes
 *
 * This class represents the physical memory of the system. It provides
 * basic read/write operations and tracks memory usage statistics.
 */
class PhysicalMemory {
public:
    /**
     * @brief Construct a PhysicalMemory object with specified size
     * @param size Total size of physical memory in bytes
     */
    explicit PhysicalMemory(size_t size);

    /**
     * @brief Destructor
     */
    ~PhysicalMemory() = default;

    // Non-copyable, non-movable
    PhysicalMemory(const PhysicalMemory&) = delete;
    PhysicalMemory& operator=(const PhysicalMemory&) = delete;
    PhysicalMemory(PhysicalMemory&&) = delete;
    PhysicalMemory& operator=(PhysicalMemory&&) = delete;

    /**
     * @brief Write data to physical memory
     * @param addr Starting address to write to
     * @param data Pointer to data to write
     * @param size Number of bytes to write
     * @return true if write was successful, false if out of bounds
     */
    bool write(Address addr, const void* data, size_t size);

    /**
     * @brief Read data from physical memory
     * @param addr Starting address to read from
     * @param buffer Pointer to buffer to read into
     * @param size Number of bytes to read
     * @return true if read was successful, false if out of bounds
     */
    bool read(Address addr, void* buffer, size_t size) const;

    /**
     * @brief Write single byte to physical memory (convenience method for cache)
     * @param addr Address to write to
     * @param data Byte value to write
     * @return Result indicating success or error
     */
    Result<void> write(Address addr, uint8_t data);

    /**
     * @brief Read single byte from physical memory (convenience method for cache)
     * @param addr Address to read from
     * @return Result containing byte value, or error
     */
    Result<uint8_t> read(Address addr) const;

    /**
     * @brief Get total size of physical memory
     * @return Total memory size in bytes
     */
    size_t getTotalSize() const { return total_size_; }

    /**
     * @brief Get currently used memory size
     * @return Used memory size in bytes
     */
    size_t getUsedSize() const { return used_size_; }

    /**
     * @brief Get free memory size
     * @return Free memory size in bytes
     */
    size_t getFreeSize() const { return total_size_ - used_size_; }

    /**
     * @brief Update the used memory size
     * @param size New used memory size (updated by allocator, not I/O)
     */
    void updateUsedSize(size_t size) { used_size_ = size; }

    /**
     * @brief Zero out all memory
     */
    void clear();

    /**
     * @brief Check if an address range is valid
     * @param addr Starting address
     * @param size Size of the range
     * @return true if the entire range is within memory bounds
     */
    bool isValidRange(Address addr, size_t size) const;

private:
    std::vector<uint8_t> memory_;  // The actual memory storage
    size_t total_size_;             // Total memory size
    size_t used_size_;              // Currently used memory (tracked by allocator)
};

} // namespace memsim

#endif // MEMSIM_MEMORY_PHYSICAL_MEMORY_H
