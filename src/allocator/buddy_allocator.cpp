#include "allocator/buddy_allocator.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <cmath>

namespace memsim {

BuddyAllocator::BuddyAllocator(PhysicalMemory* memory, size_t min_block_size)
    : physical_memory_(memory),
      min_block_size_(min_block_size),
      max_block_size_(memory->getTotalSize()),
      next_block_id_(1),
      total_allocations_(0),
      failed_allocations_(0),
      total_deallocations_(0) {

    // Validate that memory size is a power of 2
    if (!isPowerOfTwo(max_block_size_)) {
        throw std::invalid_argument("Memory size must be a power of 2 for buddy allocation");
    }

    // Validate that min_block_size is a power of 2
    if (!isPowerOfTwo(min_block_size_)) {
        throw std::invalid_argument("Minimum block size must be a power of 2");
    }

    // Initialize with one large free block covering all memory
    BuddyBlock* initial_block = new BuddyBlock(0, max_block_size_, true);
    free_lists_[max_block_size_].push_back(initial_block);
}

BuddyAllocator::~BuddyAllocator() {
    // Clean up all blocks in free lists
    for (auto& pair : free_lists_) {
        for (BuddyBlock* block : pair.second) {
            delete block;
        }
    }

    // Clean up any remaining allocated blocks
    for (auto& pair : allocated_blocks_) {
        delete pair.second;
    }
}

Result<BlockId> BuddyAllocator::allocate(size_t size) {
    total_allocations_++;

    if (size == 0) {
        failed_allocations_++;
        return Result<BlockId>::Err("Cannot allocate zero bytes");
    }

    // Round up to power of 2, but at least min_block_size
    size_t actual_size = roundUpToPowerOfTwo(size);
    if (actual_size < min_block_size_) {
        actual_size = min_block_size_;
    }

    // Can't allocate more than total memory
    if (actual_size > max_block_size_) {
        failed_allocations_++;
        return Result<BlockId>::Err("Requested size exceeds total memory");
    }

    // Try to find or split to get a block of the required size
    BuddyBlock* block = splitBlock(actual_size, actual_size);
    if (block == nullptr) {
        failed_allocations_++;
        return Result<BlockId>::Err("No suitable block found (out of memory)");
    }

    // Mark as allocated
    block->is_free = false;
    block->id = next_block_id_++;

    // Track for quick lookups
    allocated_blocks_[block->id] = block;
    address_to_block_[block->start_address] = block;
    requested_sizes_[block->id] = size;

    // Update physical memory used size
    size_t total_used = 0;
    for (const auto& pair : allocated_blocks_) {
        total_used += pair.second->size;
    }
    physical_memory_->updateUsedSize(total_used);

    return Result<BlockId>::Ok(block->id);
}

Result<void> BuddyAllocator::deallocate(BlockId block_id) {
    total_deallocations_++;

    // Find the block
    auto it = allocated_blocks_.find(block_id);
    if (it == allocated_blocks_.end()) {
        return Result<void>::Err("Block ID not found");
    }

    BuddyBlock* block = it->second;

    // Mark as free
    block->is_free = true;
    block->id = 0;

    // Remove from tracking maps
    allocated_blocks_.erase(block_id);
    address_to_block_.erase(block->start_address);
    requested_sizes_.erase(block_id);

    // Add to free list and try to coalesce
    addToFreeList(block);
    coalesceBlock(block);

    // Update physical memory used size
    size_t total_used = 0;
    for (const auto& pair : allocated_blocks_) {
        total_used += pair.second->size;
    }
    physical_memory_->updateUsedSize(total_used);

    return Result<void>::Ok();
}

Result<void> BuddyAllocator::deallocateByAddress(Address address) {
    auto it = address_to_block_.find(address);
    if (it == address_to_block_.end()) {
        return Result<void>::Err("No allocated block found at this address");
    }

    BuddyBlock* block = it->second;
    return deallocate(block->id);
}

BuddyBlock* BuddyAllocator::splitBlock(size_t current_size, size_t target_size) {
    // Base case: found exact size
    if (current_size == target_size) {
        BuddyBlock* block = findFreeBlock(current_size);
        if (block != nullptr) {
            removeFromFreeList(block);
            return block;
        }

        // No block of this size, try larger size
        if (current_size >= max_block_size_) {
            return nullptr;
        }

        return splitBlock(current_size * 2, target_size);
    }

    // Recursive case: need to split
    BuddyBlock* larger_block = findFreeBlock(current_size);

    if (larger_block == nullptr) {
        // Try to get even larger block and split it
        if (current_size >= max_block_size_) {
            return nullptr;
        }
        return splitBlock(current_size * 2, target_size);
    }

    // Split the larger block into two buddies
    removeFromFreeList(larger_block);

    size_t half_size = current_size / 2;

    // Create two buddy blocks
    BuddyBlock* left_buddy = new BuddyBlock(larger_block->start_address, half_size, true);
    BuddyBlock* right_buddy = new BuddyBlock(larger_block->start_address + half_size, half_size, true);

    // Add both to free list
    addToFreeList(left_buddy);
    addToFreeList(right_buddy);

    // Delete the original larger block
    delete larger_block;

    // Continue splitting if needed
    return splitBlock(half_size, target_size);
}

void BuddyAllocator::coalesceBlock(BuddyBlock* block) {
    if (block == nullptr || !block->is_free) {
        return;
    }

    // Can't coalesce blocks of maximum size
    if (block->size >= max_block_size_) {
        return;
    }

    // Calculate buddy address
    Address buddy_addr = getBuddyAddress(block->start_address, block->size);

    // Find buddy in the free list
    auto& free_list = free_lists_[block->size];
    BuddyBlock* buddy = nullptr;

    for (BuddyBlock* candidate : free_list) {
        if (candidate->start_address == buddy_addr && candidate->is_free) {
            buddy = candidate;
            break;
        }
    }

    // If buddy not found or not free, can't coalesce
    if (buddy == nullptr) {
        return;
    }

    // Determine which block comes first
    BuddyBlock* left = (block->start_address < buddy->start_address) ? block : buddy;
    BuddyBlock* right = (block->start_address < buddy->start_address) ? buddy : block;

    // Remove both blocks from free list
    removeFromFreeList(left);
    removeFromFreeList(right);

    // Create merged block
    BuddyBlock* merged = new BuddyBlock(left->start_address, left->size * 2, true);

    // Add merged block to free list
    addToFreeList(merged);

    // Delete old blocks
    delete left;
    delete right;

    // Recursively try to coalesce the merged block
    coalesceBlock(merged);
}

BuddyBlock* BuddyAllocator::findFreeBlock(size_t size) {
    auto it = free_lists_.find(size);
    if (it == free_lists_.end() || it->second.empty()) {
        return nullptr;
    }
    return it->second.front();
}

void BuddyAllocator::removeFromFreeList(BuddyBlock* block) {
    auto& free_list = free_lists_[block->size];
    free_list.remove(block);
}

void BuddyAllocator::addToFreeList(BuddyBlock* block) {
    free_lists_[block->size].push_back(block);
}

size_t BuddyAllocator::roundUpToPowerOfTwo(size_t size) const {
    if (size == 0) return 1;
    if (isPowerOfTwo(size)) return size;

    size_t power = 1;
    while (power < size) {
        power *= 2;
    }
    return power;
}

bool BuddyAllocator::isPowerOfTwo(size_t n) const {
    return n > 0 && (n & (n - 1)) == 0;
}

size_t BuddyAllocator::log2(size_t n) const {
    size_t result = 0;
    while (n > 1) {
        n >>= 1;
        result++;
    }
    return result;
}

Address BuddyAllocator::getBuddyAddress(Address addr, size_t size) const {
    return addr ^ size;
}

void BuddyAllocator::dump() const {
    std::cout << "\n=== Buddy Memory Layout (" << physical_memory_->getTotalSize()
              << " bytes) ===" << std::endl;
    std::cout << "Min block size: " << min_block_size_ << " bytes" << std::endl;
    std::cout << "Max block size: " << max_block_size_ << " bytes" << std::endl;
    std::cout << "\nFree Lists:" << std::endl;

    for (const auto& pair : free_lists_) {
        if (!pair.second.empty()) {
            std::cout << "  Size " << pair.first << ": " << pair.second.size()
                      << " block(s)" << std::endl;
        }
    }

    std::cout << "\nAllocated Blocks:" << std::endl;
    for (const auto& pair : allocated_blocks_) {
        BuddyBlock* block = pair.second;
        std::cout << "  [0x" << std::hex << std::setfill('0') << std::setw(4)
                  << block->start_address << " - 0x" << std::setw(4)
                  << (block->endAddress() - 1) << std::dec << "] id=" << block->id
                  << ", size=" << block->size << " bytes" << std::endl;
    }
    std::cout << std::endl;
}

std::string BuddyAllocator::getStats() const {
    std::ostringstream oss;

    oss << "\n=== Buddy Allocator Statistics ===" << std::endl;
    oss << "Strategy: Buddy Allocation (Power-of-Two)" << std::endl;
    oss << "Min block size: " << min_block_size_ << " bytes" << std::endl;
    oss << "Max block size: " << max_block_size_ << " bytes" << std::endl;

    oss << "\nTotal memory: " << physical_memory_->getTotalSize() << " bytes" << std::endl;
    oss << "Used memory: " << physical_memory_->getUsedSize() << " bytes" << std::endl;
    oss << "Free memory: " << physical_memory_->getFreeSize() << " bytes" << std::endl;
    oss << "Utilization: " << std::fixed << std::setprecision(2)
        << getUtilization() << "%" << std::endl;

    oss << "\nAllocated blocks: " << allocated_blocks_.size() << std::endl;

    size_t total_free_blocks = 0;
    for (const auto& pair : free_lists_) {
        total_free_blocks += pair.second.size();
    }
    oss << "Free blocks: " << total_free_blocks << std::endl;
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

double BuddyAllocator::getInternalFragmentation() const {
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

double BuddyAllocator::getExternalFragmentation() const {
    size_t total_free = getTotalFreeMemory();
    if (total_free == 0) {
        return 0.0;
    }

    size_t largest_free = getLargestFreeBlock();
    return 100.0 * (total_free - largest_free) / static_cast<double>(total_free);
}

double BuddyAllocator::getUtilization() const {
    if (physical_memory_->getTotalSize() == 0) {
        return 0.0;
    }
    return 100.0 * physical_memory_->getUsedSize() /
           static_cast<double>(physical_memory_->getTotalSize());
}

size_t BuddyAllocator::getTotalFreeMemory() const {
    size_t total = 0;
    for (const auto& pair : free_lists_) {
        for (const BuddyBlock* block : pair.second) {
            total += block->size;
        }
    }
    return total;
}

size_t BuddyAllocator::getLargestFreeBlock() const {
    size_t largest = 0;
    for (const auto& pair : free_lists_) {
        if (!pair.second.empty() && pair.first > largest) {
            largest = pair.first;
        }
    }
    return largest;
}

} // namespace memsim
