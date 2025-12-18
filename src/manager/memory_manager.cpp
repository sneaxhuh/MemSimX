#include "manager/memory_manager.h"
#include <iostream>
#include <iomanip>

namespace memsim {

MemoryManager::MemoryManager()
    : physical_memory_(nullptr),
      allocator_(nullptr),
      current_allocator_type_(AllocatorType::FIRST_FIT) {
}

Result<void> MemoryManager::initMemory(size_t size) {
    if (size == 0) {
        return Result<void>::Err("Memory size must be greater than zero");
    }

    try {
        physical_memory_ = std::make_unique<PhysicalMemory>(size);

        // If we already have an allocator, recreate it with the new memory
        if (allocator_ != nullptr) {
            allocator_ = std::make_unique<StandardAllocator>(
                physical_memory_.get(),
                current_allocator_type_
            );
        }

        std::cout << "Memory initialized: " << size << " bytes" << std::endl;
        return Result<void>::Ok();
    } catch (const std::exception& e) {
        return Result<void>::Err(std::string("Failed to initialize memory: ") + e.what());
    }
}

Result<void> MemoryManager::setAllocator(AllocatorType type) {
    if (!isMemoryInitialized()) {
        return Result<void>::Err("Memory not initialized. Use 'init memory <size>' first.");
    }

    try {
        current_allocator_type_ = type;
        allocator_ = std::make_unique<StandardAllocator>(
            physical_memory_.get(),
            type
        );

        std::string type_name;
        switch (type) {
            case AllocatorType::FIRST_FIT: type_name = "First Fit"; break;
            case AllocatorType::BEST_FIT: type_name = "Best Fit"; break;
            case AllocatorType::WORST_FIT: type_name = "Worst Fit"; break;
            default: type_name = "Unknown"; break;
        }

        std::cout << "Allocator set to: " << type_name << std::endl;
        return Result<void>::Ok();
    } catch (const std::exception& e) {
        return Result<void>::Err(std::string("Failed to set allocator: ") + e.what());
    }
}

Result<BlockId> MemoryManager::malloc(size_t size) {
    if (!isMemoryInitialized()) {
        return Result<BlockId>::Err("Memory not initialized");
    }

    if (!isAllocatorSet()) {
        return Result<BlockId>::Err("Allocator not set");
    }

    auto result = allocator_->allocate(size);
    if (result.success) {
        std::cout << "Allocated block id=" << result.value
                  << " at address=0x" << std::hex << std::setfill('0') << std::setw(4)
                  << 0 << std::dec << std::endl; // We'll get actual address later
    }

    return result;
}

Result<void> MemoryManager::free(BlockId block_id) {
    if (!isAllocatorSet()) {
        return Result<void>::Err("Allocator not set");
    }

    auto result = allocator_->deallocate(block_id);
    if (result.success) {
        std::cout << "Block " << block_id << " freed" << std::endl;
    }

    return result;
}

Result<void> MemoryManager::freeByAddress(Address address) {
    if (!isAllocatorSet()) {
        return Result<void>::Err("Allocator not set");
    }

    auto result = allocator_->deallocateByAddress(address);
    if (result.success) {
        std::cout << "Block at address 0x" << std::hex << address << std::dec
                  << " freed" << std::endl;
    }

    return result;
}

void MemoryManager::dumpMemory() const {
    if (!isMemoryInitialized()) {
        std::cout << "Memory not initialized" << std::endl;
        return;
    }

    if (!isAllocatorSet()) {
        std::cout << "Allocator not set" << std::endl;
        return;
    }

    allocator_->dump();
}

void MemoryManager::printStats() const {
    if (!isMemoryInitialized()) {
        std::cout << "Memory not initialized" << std::endl;
        return;
    }

    if (!isAllocatorSet()) {
        std::cout << "Allocator not set" << std::endl;
        return;
    }

    std::cout << allocator_->getStats();
}

} // namespace memsim
