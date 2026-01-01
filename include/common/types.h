#ifndef MEMSIM_COMMON_TYPES_H
#define MEMSIM_COMMON_TYPES_H

#include <cstdint>

namespace memsim {

// Type aliases for clarity
using Address = uint64_t;
using BlockId = uint32_t;
using PageNumber = uint32_t;
using FrameNumber = uint32_t;

// Allocator types
enum class AllocatorType {
    FIRST_FIT,
    BEST_FIT,
    WORST_FIT,
    BUDDY
};

// Cache replacement policies
enum class CachePolicy {
    FIFO,  // First-In-First-Out
    LRU,   // Least Recently Used
    LFU    // Least Frequently Used
};

// Page replacement policies
enum class PageReplacementPolicy {
    FIFO,   // First-In-First-Out
    LRU     // Least Recently Used
};

} // namespace memsim

#endif // MEMSIM_COMMON_TYPES_H
