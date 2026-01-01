#include "allocator/standard_allocator.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>
#include <algorithm>

namespace memsim {

StandardAllocator::StandardAllocator(PhysicalMemory* memory, AllocatorType type)
    : physical_memory_(memory),
      head_(nullptr),
      strategy_(type),
      next_block_id_(1),
      total_allocations_(0),
      failed_allocations_(0),
      total_deallocations_(0) {

    // Initialize with one large free block covering all memory
    head_ = new MemoryBlock(0, memory->getTotalSize(), true);
}

StandardAllocator::~StandardAllocator() {
    // Clean up all blocks in the linked list
    MemoryBlock* current = head_;
    while (current != nullptr) {
        MemoryBlock* next = current->next;
        delete current;
        current = next;
    }
}

Result<BlockId> StandardAllocator::allocate(size_t size) {
    total_allocations_++;

    // Validate input
    if (size == 0) {
        failed_allocations_++;
        return Result<BlockId>::Err("Cannot allocate zero bytes");
    }

    // Find a suitable free block
    MemoryBlock* block = findBlock(size);
    if (block == nullptr) {
        failed_allocations_++;
        return Result<BlockId>::Err("No suitable block found (out of memory)");
    }

    // Split the block if it's larger than needed
    splitBlock(block, size);

    // Mark the block as allocated
    block->is_free = false;
    block->id = next_block_id_++;

    // Track for quick lookups
    allocated_blocks_[block->id] = block;
    address_to_block_[block->start_address] = block;
    requested_sizes_[block->id] = size;

    // Update physical memory used size
    size_t total_used = 0;
    MemoryBlock* current = head_;
    while (current != nullptr) {
        if (!current->is_free) {
            total_used += current->size;
        }
        current = current->next;
    }
    physical_memory_->updateUsedSize(total_used);

    return Result<BlockId>::Ok(block->id);
}

Result<void> StandardAllocator::deallocate(BlockId block_id) {
    // Find the block
    auto it = allocated_blocks_.find(block_id);
    if (it == allocated_blocks_.end()) {
        return Result<void>::Err("Block ID not found (allocator may have been reset or invalid ID)");
    }

    MemoryBlock* block = it->second;

    // Mark as free
    block->is_free = true;
    block->id = 0;

    // Remove from tracking maps
    allocated_blocks_.erase(block_id);
    address_to_block_.erase(block->start_address);
    requested_sizes_.erase(block_id);

    // Coalesce with adjacent free blocks
    coalesceBlock(block);

    // Update physical memory used size
    size_t total_used = 0;
    MemoryBlock* current = head_;
    while (current != nullptr) {
        if (!current->is_free) {
            total_used += current->size;
        }
        current = current->next;
    }
    physical_memory_->updateUsedSize(total_used);

    total_deallocations_++;
    return Result<void>::Ok();
}

Result<void> StandardAllocator::deallocateByAddress(Address address) {
    // Find the block by address
    auto it = address_to_block_.find(address);
    if (it == address_to_block_.end()) {
        return Result<void>::Err("No allocated block found at this address");
    }

    MemoryBlock* block = it->second;
    return deallocate(block->id);
}

MemoryBlock* StandardAllocator::findBlock(size_t size) {
    MemoryBlock* best_block = nullptr;

    switch (strategy_) {
        case AllocatorType::FIRST_FIT: {
            // Return the first block that fits
            MemoryBlock* current = head_;
            while (current != nullptr) {
                if (current->is_free && current->size >= size) {
                    return current;
                }
                current = current->next;
            }
            break;
        }

        case AllocatorType::BEST_FIT: {
            // Find the smallest block that fits
            size_t min_size = std::numeric_limits<size_t>::max();
            MemoryBlock* current = head_;
            while (current != nullptr) {
                if (current->is_free && current->size >= size && current->size < min_size) {
                    best_block = current;
                    min_size = current->size;
                }
                current = current->next;
            }
            break;
        }

        case AllocatorType::WORST_FIT: {
            // Find the largest block that fits
            size_t max_size = 0;
            MemoryBlock* current = head_;
            while (current != nullptr) {
                if (current->is_free && current->size >= size && current->size > max_size) {
                    best_block = current;
                    max_size = current->size;
                }
                current = current->next;
            }
            break;
        }

        default:
            return nullptr;
    }

    return best_block;
}

void StandardAllocator::splitBlock(MemoryBlock* block, size_t size) {
    const size_t MIN_SPLIT_SIZE = 1;

    if (block->size > size + MIN_SPLIT_SIZE) {
        // Create a new free block for the remaining space
        MemoryBlock* new_block = new MemoryBlock(
            block->start_address + size,
            block->size - size,
            true
        );

        // Insert the new block after the current block
        new_block->next = block->next;
        new_block->prev = block;

        if (block->next != nullptr) {
            block->next->prev = new_block;
        }
        block->next = new_block;

        // Resize the current block
        block->size = size;
    }
}

void StandardAllocator::coalesceBlock(MemoryBlock* block) {
    if (block == nullptr || !block->is_free) {
        return;
    }

    // Try to merge with next block
    while (block->next != nullptr && block->next->is_free) {
        MemoryBlock* next = block->next;

        // Merge
        block->size += next->size;
        block->next = next->next;

        if (next->next != nullptr) {
            next->next->prev = block;
        }

        delete next;
    }

    // Try to merge with previous block
    if (block->prev != nullptr && block->prev->is_free) {
        MemoryBlock* prev = block->prev;

        // Merge
        prev->size += block->size;
        prev->next = block->next;

        if (block->next != nullptr) {
            block->next->prev = prev;
        }

        delete block;
    }
}

void StandardAllocator::dump() const {
    std::cout << "\n=== Memory Layout (" << physical_memory_->getTotalSize()
              << " bytes) ===" << std::endl;

    MemoryBlock* current = head_;
    while (current != nullptr) {
        if (current->is_free) {
            std::cout << "[0x" << std::hex << std::setfill('0') << std::setw(4)
                      << current->start_address << " - 0x" << std::setw(4)
                      << (current->endAddress() - 1) << std::dec << "] FREE ("
                      << current->size << " bytes)" << std::endl;
        } else {
            std::cout << "[0x" << std::hex << std::setfill('0') << std::setw(4)
                      << current->start_address << " - 0x" << std::setw(4)
                      << (current->endAddress() - 1) << std::dec << "] USED (id="
                      << current->id << ", " << current->size << " bytes)" << std::endl;
        }
        current = current->next;
    }
    std::cout << std::endl;
}

std::string StandardAllocator::getStats() const {
    std::ostringstream oss;

    oss << "\n=== Allocator Statistics ===" << std::endl;
    oss << "Strategy: ";
    switch (strategy_) {
        case AllocatorType::FIRST_FIT: oss << "First Fit"; break;
        case AllocatorType::BEST_FIT: oss << "Best Fit"; break;
        case AllocatorType::WORST_FIT: oss << "Worst Fit"; break;
        default: oss << "Unknown"; break;
    }
    oss << std::endl;

    oss << "Total memory: " << physical_memory_->getTotalSize() << " bytes" << std::endl;
    oss << "Used memory: " << physical_memory_->getUsedSize() << " bytes" << std::endl;
    oss << "Free memory: " << physical_memory_->getFreeSize() << " bytes" << std::endl;
    oss << "Utilization: " << std::fixed << std::setprecision(2)
        << getUtilization() << "%" << std::endl;

    oss << "\nAllocated blocks: " << countAllocatedBlocks() << std::endl;
    oss << "Free blocks: " << countFreeBlocks() << std::endl;
    oss << "Largest free block: " << getLargestFreeBlock() << " bytes" << std::endl;

    oss << "\nTotal allocations: " << total_allocations_ << std::endl;
    oss << "Failed allocations: " << failed_allocations_ << std::endl;
    oss << "Total deallocations: " << total_deallocations_ << std::endl;

    double success_rate = total_allocations_ > 0
        ? (100.0 * (total_allocations_ - failed_allocations_) / total_allocations_)
        : 0.0;
    oss << "Success rate: " << std::fixed << std::setprecision(2)
        << success_rate << "%" << std::endl;

    oss << "\nInternal fragmentation: " << std::fixed << std::setprecision(2)
        << getInternalFragmentation() << "%" << std::endl;
    oss << "External fragmentation: " << std::fixed << std::setprecision(2)
        << getExternalFragmentation() << "%" << std::endl;

    return oss.str();
}

double StandardAllocator::getInternalFragmentation() const {
    if (requested_sizes_.empty()) {
        return 0.0;
    }

    size_t total_allocated = 0;
    size_t total_requested = 0;

    for (const auto& pair : requested_sizes_) {
        BlockId block_id = pair.first;
        size_t requested = pair.second;

        auto it = allocated_blocks_.find(block_id);
        if (it != allocated_blocks_.end()) {
            total_requested += requested;
            total_allocated += it->second->size;
        }
    }

    if (total_allocated == 0) {
        return 0.0;
    }

    return 100.0 * (total_allocated - total_requested) / static_cast<double>(total_allocated);
}

double StandardAllocator::getExternalFragmentation() const {
    size_t total_free = getTotalFreeMemory();
    if (total_free == 0) {
        return 0.0;
    }

    size_t largest_free = getLargestFreeBlock();

    // External fragmentation = unusable free memory / total free memory
    // = (total_free - largest_free) / total_free
    return 100.0 * (total_free - largest_free) / static_cast<double>(total_free);
}

double StandardAllocator::getUtilization() const {
    if (physical_memory_->getTotalSize() == 0) {
        return 0.0;
    }
    return 100.0 * physical_memory_->getUsedSize() /
           static_cast<double>(physical_memory_->getTotalSize());
}

size_t StandardAllocator::getTotalFreeMemory() const {
    size_t total = 0;
    MemoryBlock* current = head_;
    while (current != nullptr) {
        if (current->is_free) {
            total += current->size;
        }
        current = current->next;
    }
    return total;
}

size_t StandardAllocator::getLargestFreeBlock() const {
    size_t largest = 0;
    MemoryBlock* current = head_;
    while (current != nullptr) {
        if (current->is_free && current->size > largest) {
            largest = current->size;
        }
        current = current->next;
    }
    return largest;
}

size_t StandardAllocator::countFreeBlocks() const {
    size_t count = 0;
    MemoryBlock* current = head_;
    while (current != nullptr) {
        if (current->is_free) {
            count++;
        }
        current = current->next;
    }
    return count;
}

size_t StandardAllocator::countAllocatedBlocks() const {
    return allocated_blocks_.size();
}

Result<Address> StandardAllocator::getBlockAddress(BlockId block_id) const {
    auto it = allocated_blocks_.find(block_id);
    if (it == allocated_blocks_.end()) {
        return Result<Address>::Err("Block ID not found");
    }
    return Result<Address>::Ok(it->second->start_address);
}

} // namespace memsim
