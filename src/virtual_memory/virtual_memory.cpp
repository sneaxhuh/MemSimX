#include "virtual_memory/virtual_memory.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace memsim {

VirtualMemory::VirtualMemory(PhysicalMemory* memory,
                             size_t num_virtual_pages,
                             size_t num_physical_frames,
                             size_t page_size,
                             PageReplacementPolicy policy)
    : memory_(memory),
      num_virtual_pages_(num_virtual_pages),
      num_physical_frames_(num_physical_frames),
      page_size_(page_size),
      policy_(policy),
      clock_hand_(0),
      global_time_(0) {

    // Validate parameters
    if (!isPowerOfTwo(page_size)) {
        throw std::invalid_argument("Page size must be power of 2");
    }
    if (num_virtual_pages == 0) {
        throw std::invalid_argument("Number of virtual pages must be > 0");
    }
    if (num_physical_frames == 0) {
        throw std::invalid_argument("Number of physical frames must be > 0");
    }
    if (num_physical_frames > num_virtual_pages) {
        throw std::invalid_argument("Physical frames cannot exceed virtual pages");
    }
    if (memory == nullptr) {
        throw std::invalid_argument("Memory pointer cannot be null");
    }

    // Check if physical memory is large enough
    size_t required_size = num_physical_frames * page_size;
    if (required_size > memory->getTotalSize()) {
        throw std::invalid_argument("Physical memory too small for requested frames");
    }

    // Calculate bit counts
    offset_bits_ = calculateBits(page_size - 1);
    page_number_bits_ = calculateBits(num_virtual_pages - 1);

    // Initialize page table
    page_table_.resize(num_virtual_pages);

    // Initialize frame allocation tracker
    frame_allocated_.resize(num_physical_frames, false);
}

Result<Address> VirtualMemory::translate(Address virtual_addr) {
    stats_.total_accesses++;
    global_time_++;

    // Parse virtual address
    size_t page_number, offset;
    parseAddress(virtual_addr, page_number, offset);

    // Check if page number is valid
    if (page_number >= num_virtual_pages_) {
        return Result<Address>::Err("Invalid virtual address: page number out of range");
    }

    auto& pte = page_table_[page_number];

    if (pte.valid) {
        // Page hit
        stats_.page_hits++;
        pte.recordAccess(global_time_);

        // Construct physical address
        Address physical_addr = constructPhysicalAddress(pte.frame_number, offset);
        return Result<Address>::Ok(physical_addr);
    }

    // Page fault - need to load page
    stats_.page_faults++;
    auto frame_result = handlePageFault(page_number);
    if (!frame_result.success) {
        return Result<Address>::Err(frame_result.error_message);
    }

    // Construct physical address
    Address physical_addr = constructPhysicalAddress(frame_result.value, offset);
    return Result<Address>::Ok(physical_addr);
}

Result<uint8_t> VirtualMemory::read(Address virtual_addr) {
    auto translate_result = translate(virtual_addr);
    if (!translate_result.success) {
        return Result<uint8_t>::Err(translate_result.error_message);
    }

    return memory_->read(translate_result.value);
}

Result<void> VirtualMemory::write(Address virtual_addr, uint8_t data) {
    auto translate_result = translate(virtual_addr);
    if (!translate_result.success) {
        return Result<void>::Err(translate_result.error_message);
    }

    // Mark page as dirty
    size_t page_number, offset;
    parseAddress(virtual_addr, page_number, offset);
    page_table_[page_number].dirty = true;

    return memory_->write(translate_result.value, data);
}

void VirtualMemory::flush() {
    for (auto& pte : page_table_) {
        pte.invalidate();
    }
    std::fill(frame_allocated_.begin(), frame_allocated_.end(), false);
    while (!fifo_queue_.empty()) {
        fifo_queue_.pop();
    }
    clock_hand_ = 0;
}

std::string VirtualMemory::getStatsString() const {
    std::ostringstream oss;
    oss << "=== Virtual Memory Statistics ===\n";
    oss << "Configuration: " << getConfigString() << "\n";
    oss << "Page Faults: " << stats_.page_faults << "\n";
    oss << "Page Hits: " << stats_.page_hits << "\n";
    oss << "Total Accesses: " << stats_.total_accesses << "\n";
    oss << "Page Fault Rate: " << std::fixed << std::setprecision(2)
        << stats_.getPageFaultRate() << "%\n";
    oss << "Page Hit Rate: " << std::fixed << std::setprecision(2)
        << stats_.getPageHitRate() << "%\n";
    return oss.str();
}

void VirtualMemory::dump() const {
    std::cout << "=== Page Table ===\n";
    std::cout << getConfigString() << "\n\n";

    for (size_t i = 0; i < num_virtual_pages_; i++) {
        const auto& pte = page_table_[i];
        if (!pte.valid) continue;  // Skip invalid entries

        std::cout << "Page " << std::setw(4) << i << ": ";
        std::cout << "Valid=" << pte.valid << ", ";
        std::cout << "Frame=" << std::setw(4) << pte.frame_number << ", ";
        std::cout << "Dirty=" << pte.dirty << ", ";
        std::cout << "Ref=" << pte.referenced;

        // Show replacement metadata
        switch (policy_) {
            case PageReplacementPolicy::FIFO:
                std::cout << ", LoadTime=" << pte.load_time;
                break;
            case PageReplacementPolicy::LRU:
                std::cout << ", LastAccess=" << pte.last_access;
                break;
            case PageReplacementPolicy::CLOCK:
                // Referenced bit already shown
                break;
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}

std::string VirtualMemory::getConfigString() const {
    std::ostringstream oss;
    oss << num_virtual_pages_ << " virtual pages, "
        << num_physical_frames_ << " physical frames, "
        << page_size_ << " bytes/page, ";

    switch (policy_) {
        case PageReplacementPolicy::FIFO: oss << "FIFO"; break;
        case PageReplacementPolicy::LRU: oss << "LRU"; break;
        case PageReplacementPolicy::CLOCK: oss << "Clock"; break;
    }

    return oss.str();
}

// Private helper methods

void VirtualMemory::parseAddress(Address virtual_addr, size_t& page_number, size_t& offset) const {
    // Extract page offset (lowest bits)
    offset = virtual_addr & ((1ULL << offset_bits_) - 1);

    // Extract page number (remaining bits)
    page_number = virtual_addr >> offset_bits_;
}

Address VirtualMemory::constructPhysicalAddress(Address frame_number, size_t offset) const {
    return (frame_number << offset_bits_) | offset;
}

Result<Address> VirtualMemory::handlePageFault(size_t page_number) {
    // Try to find free frame first
    auto free_frame = findFreeFrame();

    Address frame_number;
    if (free_frame.success) {
        // Use free frame
        frame_number = free_frame.value;
    } else {
        // No free frames - must evict a page
        size_t victim_page = selectVictimPage();
        evictPage(victim_page);

        // Now try to find free frame again
        free_frame = findFreeFrame();
        if (!free_frame.success) {
            return Result<Address>::Err("Failed to find free frame after eviction");
        }
        frame_number = free_frame.value;
    }

    // Mark frame as allocated
    frame_allocated_[frame_number] = true;

    // Load page from "disk"
    loadPageFromDisk(page_number, frame_number);

    // Update page table entry
    auto& pte = page_table_[page_number];
    pte.valid = true;
    pte.frame_number = frame_number;
    pte.dirty = false;
    pte.referenced = true;  // Set reference bit on page load
    pte.load_time = global_time_;
    pte.last_access = global_time_;

    // Update replacement policy data structures
    if (policy_ == PageReplacementPolicy::FIFO) {
        fifo_queue_.push(page_number);
    }

    return Result<Address>::Ok(frame_number);
}

size_t VirtualMemory::selectVictimPage() {
    switch (policy_) {
        case PageReplacementPolicy::FIFO: {
            if (fifo_queue_.empty()) {
                for (size_t i = 0; i < num_virtual_pages_; i++) {
                    if (page_table_[i].valid) return i;
                }
                return 0;
            }
            return fifo_queue_.front();
        }

        case PageReplacementPolicy::LRU: {
            // LRU: find page with smallest last_access time
            size_t victim = 0;
            uint64_t min_time = UINT64_MAX;
            for (size_t i = 0; i < num_virtual_pages_; i++) {
                if (page_table_[i].valid && page_table_[i].last_access < min_time) {
                    min_time = page_table_[i].last_access;
                    victim = i;
                }
            }
            return victim;
        }

        case PageReplacementPolicy::CLOCK: {
            // Clock algorithm: circular scan with reference bit
            // Scan only valid pages (those currently in memory)
            size_t scanned = 0;
            size_t max_scans = num_virtual_pages_ * 2;  // Prevent infinite loop

            while (scanned < max_scans) {
                auto& pte = page_table_[clock_hand_];

                if (pte.valid) {
                    if (!pte.referenced) {
                        // Found victim - page with ref bit = 0
                        size_t victim = clock_hand_;
                        clock_hand_ = (clock_hand_ + 1) % num_virtual_pages_;
                        return victim;
                    } else {
                        // Give second chance - clear reference bit
                        pte.referenced = false;
                    }
                }

                // Move to next page
                clock_hand_ = (clock_hand_ + 1) % num_virtual_pages_;
                scanned++;
            }

            // Fallback: return first valid page (shouldn't reach here)
            for (size_t i = 0; i < num_virtual_pages_; i++) {
                if (page_table_[i].valid) return i;
            }
            return 0;
        }

        default:
            return 0;
    }
}

void VirtualMemory::evictPage(size_t page_number) {
    auto& pte = page_table_[page_number];

    if (!pte.valid) {
        return;  // Already evicted
    }

    // If page is dirty, write back to "disk"
    if (pte.dirty) {
        writePageToDisk(page_number, pte.frame_number);
    }

    // Free the frame
    frame_allocated_[pte.frame_number] = false;

    // Invalidate page table entry
    pte.invalidate();

    // Update FIFO queue if needed
    if (policy_ == PageReplacementPolicy::FIFO && !fifo_queue_.empty()) {
        fifo_queue_.pop();  // Remove evicted page from front
    }
}

Result<Address> VirtualMemory::findFreeFrame() {
    for (size_t i = 0; i < num_physical_frames_; i++) {
        if (!frame_allocated_[i]) {
            return Result<Address>::Ok(i);
        }
    }
    return Result<Address>::Err("No free frames available");
}

void VirtualMemory::loadPageFromDisk(size_t page_number, Address frame_number) {
    // Simulate disk load with deterministic pattern
    Address frame_start = frame_number * page_size_;
    for (size_t i = 0; i < page_size_; i++) {
        uint8_t value = static_cast<uint8_t>((page_number * page_size_ + i) % 256);
        memory_->write(frame_start + i, value);
    }
}

void VirtualMemory::writePageToDisk(size_t page_number, Address frame_number) {
    // Disk write simulation (no-op)
    (void)page_number;
    (void)frame_number;
}

bool VirtualMemory::isPowerOfTwo(size_t value) {
    return value > 0 && (value & (value - 1)) == 0;
}

size_t VirtualMemory::calculateBits(size_t value) {
    if (value == 0) return 0;
    size_t bits = 0;
    while (value > 0) {
        bits++;
        value >>= 1;
    }
    return bits;
}

} // namespace memsim
