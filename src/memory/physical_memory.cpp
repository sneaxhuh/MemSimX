#include "memory/physical_memory.h"
#include <cstring>
#include <algorithm>

namespace memsim {

PhysicalMemory::PhysicalMemory(size_t size)
    : memory_(size, 0),
      total_size_(size),
      used_size_(0) {
}

bool PhysicalMemory::write(Address addr, const void* data, size_t size) {
    if (!isValidRange(addr, size)) {
        return false;
    }

    std::memcpy(memory_.data() + addr, data, size);
    return true;
}

bool PhysicalMemory::read(Address addr, void* buffer, size_t size) const {
    if (!isValidRange(addr, size)) {
        return false;
    }

    std::memcpy(buffer, memory_.data() + addr, size);
    return true;
}

void PhysicalMemory::clear() {
    std::fill(memory_.begin(), memory_.end(), 0);
    used_size_ = 0;
}

bool PhysicalMemory::isValidRange(Address addr, size_t size) const {
    if (addr >= total_size_) {
        return false;
    }
    if (size == 0) {
        return true;
    }
    if (addr + size < addr) return false;  // overflow
    return (addr + size) <= total_size_;
}

Result<void> PhysicalMemory::write(Address addr, uint8_t data) {
    if (addr >= total_size_) {
        return Result<void>::Err("Address out of bounds");
    }
    memory_[addr] = data;
    return Result<void>::Ok();
}

Result<uint8_t> PhysicalMemory::read(Address addr) const {
    if (addr >= total_size_) {
        return Result<uint8_t>::Err("Address out of bounds");
    }
    return Result<uint8_t>::Ok(memory_[addr]);
}

} // namespace memsim
