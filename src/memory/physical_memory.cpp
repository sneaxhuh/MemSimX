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
    // Check for overflow and bounds
    if (addr >= total_size_) {
        return false;
    }
    if (size == 0) {
        return true;
    }
    // Check if addr + size would overflow or exceed total_size_
    if (addr + size < addr) { // Overflow check
        return false;
    }
    return (addr + size) <= total_size_;
}

} // namespace memsim
