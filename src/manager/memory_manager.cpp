#include "manager/memory_manager.h"
#include <iostream>
#include <iomanip>

namespace memsim {

MemoryManager::MemoryManager()
    : physical_memory_(nullptr),
      allocator_(nullptr),
      virtual_memory_(nullptr),
      cache_(nullptr),
      current_allocator_type_(AllocatorType::FIRST_FIT) {
}

Result<void> MemoryManager::initMemory(size_t size) {
    if (size == 0) {
        return Result<void>::Err("Memory size must be greater than zero");
    }

    try {
        physical_memory_ = std::make_unique<PhysicalMemory>(size);

        if (current_allocator_type_ == AllocatorType::BUDDY) {
            allocator_ = std::make_unique<BuddyAllocator>(
                physical_memory_.get(),
                32  // min block size
            );
        } else {
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
    try {
        current_allocator_type_ = type;

        if (isMemoryInitialized()) {
            // Warn user that switching allocators clears all allocations
            if (allocator_) {
                std::cout << "Warning: Switching allocator. All previous allocations invalidated." << std::endl;
            }

            // Create appropriate allocator based on type
            if (type == AllocatorType::BUDDY) {
                allocator_ = std::make_unique<BuddyAllocator>(
                    physical_memory_.get(),
                    32  // min block size
                );
            } else {
                allocator_ = std::make_unique<StandardAllocator>(
                    physical_memory_.get(),
                    type
                );
            }
        }

        std::string type_name;
        switch (type) {
            case AllocatorType::FIRST_FIT: type_name = "First Fit"; break;
            case AllocatorType::BEST_FIT: type_name = "Best Fit"; break;
            case AllocatorType::WORST_FIT: type_name = "Worst Fit"; break;
            case AllocatorType::BUDDY: type_name = "Buddy Allocation"; break;
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
        auto addr_result = allocator_->getBlockAddress(result.value);
        if (addr_result.success) {
            std::cout << "Allocated block id=" << result.value
                      << " at address=0x" << std::hex << std::setfill('0') << std::setw(4)
                      << addr_result.value << std::dec << std::endl;
        } else {
            std::cout << "Allocated block id=" << result.value << std::endl;
        }
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

Result<void> MemoryManager::initVirtualMemory(size_t num_virtual_pages,
                                               size_t num_physical_frames,
                                               size_t page_size,
                                               PageReplacementPolicy policy) {
    if (!isMemoryInitialized()) {
        return Result<void>::Err("Physical memory must be initialized first");
    }

    try {
        virtual_memory_ = std::make_unique<VirtualMemory>(
            physical_memory_.get(),
            num_virtual_pages,
            num_physical_frames,
            page_size,
            policy
        );

        std::string policy_name;
        switch (policy) {
            case PageReplacementPolicy::FIFO: policy_name = "FIFO"; break;
            case PageReplacementPolicy::LRU: policy_name = "LRU"; break;
            default: policy_name = "Unknown"; break;
        }

        std::cout << "Virtual memory initialized: "
                  << num_virtual_pages << " virtual pages, "
                  << num_physical_frames << " physical frames, "
                  << page_size << " bytes/page, "
                  << policy_name << " policy" << std::endl;

        return Result<void>::Ok();
    } catch (const std::exception& e) {
        return Result<void>::Err(std::string("Failed to initialize virtual memory: ") + e.what());
    }
}

Result<uint8_t> MemoryManager::vmRead(Address virtual_addr) {
    if (!isVMInitialized()) {
        return Result<uint8_t>::Err("Virtual memory not initialized");
    }

    return virtual_memory_->read(virtual_addr);
}

Result<void> MemoryManager::vmWrite(Address virtual_addr, uint8_t data) {
    if (!isVMInitialized()) {
        return Result<void>::Err("Virtual memory not initialized");
    }

    return virtual_memory_->write(virtual_addr, data);
}

Result<Address> MemoryManager::vmTranslate(Address virtual_addr) {
    if (!isVMInitialized()) {
        return Result<Address>::Err("Virtual memory not initialized");
    }

    return virtual_memory_->translate(virtual_addr);
}

void MemoryManager::printVMStats() const {
    if (!isVMInitialized()) {
        std::cout << "Virtual memory not initialized" << std::endl;
        return;
    }

    std::cout << virtual_memory_->getStatsString();
}

void MemoryManager::dumpVM() const {
    if (!isVMInitialized()) {
        std::cout << "Virtual memory not initialized" << std::endl;
        return;
    }

    virtual_memory_->dump();
}

Result<void> MemoryManager::initCache(size_t l1_sets, size_t l1_assoc, size_t l1_block_size, CachePolicy l1_policy,
                                       size_t l2_sets, size_t l2_assoc, size_t l2_block_size, CachePolicy l2_policy) {
    if (!isMemoryInitialized()) {
        return Result<void>::Err("Physical memory must be initialized first");
    }

    try {
        cache_ = std::make_unique<CacheHierarchy>(
            physical_memory_.get(),
            l1_sets, l1_assoc, l1_block_size, l1_policy,
            l2_sets, l2_assoc, l2_block_size, l2_policy
        );

        std::string l1_policy_name, l2_policy_name;
        switch (l1_policy) {
            case CachePolicy::FIFO: l1_policy_name = "FIFO"; break;
            case CachePolicy::LRU: l1_policy_name = "LRU"; break;
            case CachePolicy::LFU: l1_policy_name = "LFU"; break;
            default: l1_policy_name = "Unknown"; break;
        }
        switch (l2_policy) {
            case CachePolicy::FIFO: l2_policy_name = "FIFO"; break;
            case CachePolicy::LRU: l2_policy_name = "LRU"; break;
            case CachePolicy::LFU: l2_policy_name = "LFU"; break;
            default: l2_policy_name = "Unknown"; break;
        }

        std::cout << "Cache hierarchy initialized:" << std::endl;
        std::cout << "  L1: " << l1_sets << " sets, " << l1_assoc << "-way, "
                  << l1_block_size << " bytes/block, " << l1_policy_name << std::endl;
        std::cout << "  L2: " << l2_sets << " sets, " << l2_assoc << "-way, "
                  << l2_block_size << " bytes/block, " << l2_policy_name << std::endl;

        return Result<void>::Ok();
    } catch (const std::exception& e) {
        return Result<void>::Err(std::string("Failed to initialize cache: ") + e.what());
    }
}

Result<uint8_t> MemoryManager::cacheRead(Address address) {
    if (!isCacheInitialized()) {
        return Result<uint8_t>::Err("Cache not initialized");
    }

    return cache_->read(address);
}

Result<void> MemoryManager::cacheWrite(Address address, uint8_t data) {
    if (!isCacheInitialized()) {
        return Result<void>::Err("Cache not initialized");
    }

    return cache_->write(address, data);
}

void MemoryManager::printCacheStats() const {
    if (!isCacheInitialized()) {
        std::cout << "Cache not initialized" << std::endl;
        return;
    }

    std::cout << cache_->getStatsString();
}

void MemoryManager::dumpCache() const {
    if (!isCacheInitialized()) {
        std::cout << "Cache not initialized" << std::endl;
        return;
    }

    cache_->dump();
}

void MemoryManager::flushCache() {
    if (!isCacheInitialized()) {
        std::cout << "Cache not initialized" << std::endl;
        return;
    }

    cache_->flush();
    std::cout << "Cache flushed" << std::endl;
}

} // namespace memsim
