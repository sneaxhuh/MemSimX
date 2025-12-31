#include "cache/cache_level.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace memsim {

CacheLevel::CacheLevel(int level,
                       size_t num_sets,
                       size_t associativity,
                       size_t block_size,
                       CachePolicy policy,
                       PhysicalMemory* memory)
    : level_(level),
      num_sets_(num_sets),
      associativity_(associativity),
      block_size_(block_size),
      policy_(policy),
      memory_(memory),
      global_time_(0) {

    // Validate parameters
    if (!isPowerOfTwo(num_sets)) {
        throw std::invalid_argument("Number of sets must be power of 2");
    }
    if (!isPowerOfTwo(block_size)) {
        throw std::invalid_argument("Block size must be power of 2");
    }
    if (associativity == 0) {
        throw std::invalid_argument("Associativity must be at least 1");
    }
    if (memory == nullptr) {
        throw std::invalid_argument("Memory pointer cannot be null");
    }

    // Calculate bit counts for address parsing
    offset_bits_ = calculateBits(block_size - 1);
    index_bits_ = calculateBits(num_sets - 1);

    // Initialize cache structure
    sets_.resize(num_sets);
    for (auto& set : sets_) {
        set.reserve(associativity);
        for (size_t i = 0; i < associativity; i++) {
            set.emplace_back(block_size);
        }
    }
}

Result<uint8_t> CacheLevel::read(Address address) {
    stats_.accesses++;
    global_time_++;

    // Parse address
    Address tag;
    size_t set_index, offset;
    parseAddress(address, tag, set_index, offset);

    // Look for matching line in set
    CacheLine* line = findLine(set_index, tag);

    if (line != nullptr) {
        // Cache hit
        stats_.hits++;
        line->recordAccess(global_time_);
        return Result<uint8_t>::Ok(line->data[offset]);
    }

    // Cache miss - select victim and load from memory
    stats_.misses++;
    size_t victim_way = selectVictim(set_index);

    // Load block from memory
    loadBlock(address, tag, set_index, victim_way);

    // Return requested byte
    CacheLine& new_line = sets_[set_index][victim_way];
    return Result<uint8_t>::Ok(new_line.data[offset]);
}

Result<void> CacheLevel::write(Address address, uint8_t data) {
    stats_.accesses++;
    global_time_++;

    // Parse address
    Address tag;
    size_t set_index, offset;
    parseAddress(address, tag, set_index, offset);

    // Write-through: always write to memory
    auto write_result = memory_->write(address, data);
    if (!write_result.success) {
        return write_result;
    }

    // Look for matching line in set
    CacheLine* line = findLine(set_index, tag);

    if (line != nullptr) {
        // Cache hit - update cache line
        stats_.hits++;
        line->data[offset] = data;
        line->recordAccess(global_time_);
    } else {
        // Cache miss - load block and update
        stats_.misses++;
        size_t victim_way = selectVictim(set_index);
        loadBlock(address, tag, set_index, victim_way);
        sets_[set_index][victim_way].data[offset] = data;
    }

    return Result<void>::Ok();
}

bool CacheLevel::contains(Address address) const {
    Address tag;
    size_t set_index, offset;
    parseAddress(address, tag, set_index, offset);

    const auto& set = sets_[set_index];
    for (const auto& line : set) {
        if (line.valid && line.tag == tag) {
            return true;
        }
    }
    return false;
}

void CacheLevel::flush() {
    for (auto& set : sets_) {
        for (auto& line : set) {
            line.invalidate();
        }
    }
}

std::string CacheLevel::getStatsString() const {
    std::ostringstream oss;
    oss << "=== L" << level_ << " Cache Statistics ===\n";
    oss << "Configuration: " << getConfigString() << "\n";
    oss << "Hits: " << stats_.hits << "\n";
    oss << "Misses: " << stats_.misses << "\n";
    oss << "Total Accesses: " << stats_.accesses << "\n";
    oss << "Hit Ratio: " << std::fixed << std::setprecision(2)
        << stats_.getHitRatio() << "%\n";
    oss << "Miss Ratio: " << std::fixed << std::setprecision(2)
        << stats_.getMissRatio() << "%\n";
    return oss.str();
}

void CacheLevel::dump() const {
    std::cout << "=== L" << level_ << " Cache Contents ===\n";
    std::cout << getConfigString() << "\n\n";

    for (size_t set_idx = 0; set_idx < num_sets_; set_idx++) {
        bool has_valid = false;
        for (const auto& line : sets_[set_idx]) {
            if (line.valid) {
                has_valid = true;
                break;
            }
        }

        if (!has_valid) continue;

        std::cout << "Set " << set_idx << ": ";
        for (size_t way = 0; way < associativity_; way++) {
            const auto& line = sets_[set_idx][way];
            if (line.valid) {
                std::cout << "[V:1 Tag:0x" << std::hex << std::setw(4)
                          << std::setfill('0') << line.tag << std::dec;

                // Show replacement metadata
                switch (policy_) {
                    case CachePolicy::FIFO:
                        std::cout << " Order:" << line.insertion_order;
                        break;
                    case CachePolicy::LRU:
                        std::cout << " LastUse:" << line.last_access_time;
                        break;
                    case CachePolicy::LFU:
                        std::cout << " AccessCnt:" << line.access_count;
                        break;
                }
                std::cout << "] ";
            } else {
                std::cout << "[V:0 Tag:----] ";
            }
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}

std::string CacheLevel::getConfigString() const {
    std::ostringstream oss;
    oss << num_sets_ << " sets, " << associativity_ << "-way, "
        << block_size_ << " bytes/block, ";

    switch (policy_) {
        case CachePolicy::FIFO: oss << "FIFO"; break;
        case CachePolicy::LRU: oss << "LRU"; break;
        case CachePolicy::LFU: oss << "LFU"; break;
    }

    return oss.str();
}

// Private helper methods

void CacheLevel::parseAddress(Address address, Address& tag, size_t& set_index, size_t& offset) const {
    // Extract block offset (lowest bits)
    offset = address & ((1ULL << offset_bits_) - 1);

    // Extract set index (middle bits)
    set_index = (address >> offset_bits_) & ((1ULL << index_bits_) - 1);

    // Extract tag (remaining upper bits)
    tag = address >> (offset_bits_ + index_bits_);
}

CacheLine* CacheLevel::findLine(size_t set_index, Address tag) {
    auto& set = sets_[set_index];
    for (auto& line : set) {
        if (line.valid && line.tag == tag) {
            return &line;
        }
    }
    return nullptr;
}

size_t CacheLevel::selectVictim(size_t set_index) {
    auto& set = sets_[set_index];

    // First, check for invalid (empty) lines
    for (size_t i = 0; i < associativity_; i++) {
        if (!set[i].valid) {
            return i;
        }
    }

    // No empty lines, use replacement policy
    switch (policy_) {
        case CachePolicy::FIFO: {
            // Find line with smallest insertion_order (oldest)
            size_t victim = 0;
            uint64_t min_order = set[0].insertion_order;
            for (size_t i = 1; i < associativity_; i++) {
                if (set[i].insertion_order < min_order) {
                    min_order = set[i].insertion_order;
                    victim = i;
                }
            }
            return victim;
        }

        case CachePolicy::LRU: {
            // Find line with smallest last_access_time (least recently used)
            size_t victim = 0;
            uint64_t min_time = set[0].last_access_time;
            for (size_t i = 1; i < associativity_; i++) {
                if (set[i].last_access_time < min_time) {
                    min_time = set[i].last_access_time;
                    victim = i;
                }
            }
            return victim;
        }

        case CachePolicy::LFU: {
            // Find line with smallest access_count (least frequently used)
            size_t victim = 0;
            uint64_t min_count = set[0].access_count;
            for (size_t i = 1; i < associativity_; i++) {
                if (set[i].access_count < min_count) {
                    min_count = set[i].access_count;
                    victim = i;
                }
            }
            return victim;
        }

        default:
            return 0;
    }
}

void CacheLevel::loadBlock(Address address, Address tag, size_t set_index, size_t way_index) {
    // Align address to block boundary
    Address block_address = (address >> offset_bits_) << offset_bits_;

    auto& line = sets_[set_index][way_index];

    // Load entire block from memory
    for (size_t i = 0; i < block_size_; i++) {
        auto read_result = memory_->read(block_address + i);
        if (read_result.success) {
            line.data[i] = read_result.value;
        } else {
            line.data[i] = 0;
        }
    }

    // Update cache line metadata
    line.valid = true;
    line.tag = tag;
    line.insertion_order = global_time_;
    line.last_access_time = global_time_;
    line.access_count = 1;
}

size_t CacheLevel::calculateBits(size_t value) {
    if (value == 0) return 0;
    size_t bits = 0;
    while (value > 0) {
        bits++;
        value >>= 1;
    }
    return bits;
}

bool CacheLevel::isPowerOfTwo(size_t value) {
    return value > 0 && (value & (value - 1)) == 0;
}

} // namespace memsim
