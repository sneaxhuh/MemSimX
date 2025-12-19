# Memory Management Simulator - Architecture Documentation

## Overview

This project implements a comprehensive memory management simulator with multiple allocation strategies, cache hierarchy, and virtual memory system. The simulator is designed for educational purposes to demonstrate how modern operating system memory management works.

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                         User/CLI                            │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                    Memory Manager                           │
│              (Orchestrates all subsystems)                  │
└─────┬──────────────┬──────────────┬─────────────────────────┘
      │              │              │
      ▼              ▼              ▼
┌───────────┐  ┌────────────┐  ┌──────────────┐
│ Allocator │  │   Cache    │  │   Virtual    │
│           │  │ Hierarchy  │  │   Memory     │
└─────┬─────┘  └─────┬──────┘  └──────┬───────┘
      │              │                 │
      └──────────────┴─────────────────┘
                     │
                     ▼
           ┌──────────────────┐
           │ Physical Memory  │
           └──────────────────┘
```

## Component Architecture

### 1. Physical Memory (`memory/`)

**Purpose**: Simulates physical RAM as a contiguous byte array.

**Key Classes**:
- `PhysicalMemory`: Manages raw memory storage

**Features**:
- Read/write operations (buffer-based and single-byte)
- Bounds checking
- Memory size tracking
- Clear/zero operations

**Design Pattern**: Singleton-like (one instance per system)

### 2. Memory Allocators (`allocator/`)

**Purpose**: Manages allocation and deallocation of memory blocks.

**Key Classes**:
- `IAllocator`: Strategy pattern interface
- `StandardAllocator`: Implements First/Best/Worst Fit
- `BuddyAllocator`: Implements Buddy System allocation

**Allocation Strategies**:

#### Standard Allocation
- **First Fit**: Allocates first block large enough
- **Best Fit**: Allocates smallest block that fits
- **Worst Fit**: Allocates largest available block

#### Buddy Allocation
- Power-of-2 sized blocks
- Recursive splitting and coalescing
- XOR-based buddy address calculation: `buddy_addr = addr ^ size`

**Data Structures**:
- Standard: Doubly-linked list of blocks
- Buddy: Free lists indexed by block size

**Metrics**:
- Internal fragmentation (wasted space within blocks)
- External fragmentation (unusable gaps between blocks)
- Memory utilization percentage

### 3. Cache Hierarchy (`cache/`)

**Purpose**: Simulates CPU cache for fast memory access.

**Key Classes**:
- `CacheLine`: Individual cache entry
- `CacheLevel`: Single cache level (L1 or L2)
- `CacheHierarchy`: Manages L1 and L2 coordination

**Cache Architecture**:
```
Virtual Address: | Tag | Set Index | Block Offset |
                   └──────────────────────────────┘
                     Used for cache lookup
```

**Replacement Policies**:
- **FIFO**: First-In-First-Out (queue-based)
- **LRU**: Least Recently Used (timestamp-based)
- **LFU**: Least Frequently Used (access count-based)

**Write Policy**: Write-through (write to cache and memory)

**Structure**:
- N-way set-associative cache
- Configurable number of sets, associativity, block size
- Two-level hierarchy (L1 → L2 → Memory)

### 4. Virtual Memory (`virtual_memory/`)

**Purpose**: Implements paging with address translation.

**Key Classes**:
- `PageTableEntry`: Page table entry with metadata
- `VirtualMemory`: Manages virtual-to-physical translation

**Address Translation**:
```
Virtual Address:  | Page Number | Page Offset |
                         ↓
                   Page Table Lookup
                         ↓
Physical Address: | Frame Number | Page Offset |
```

**Page Replacement Policies**:
- **FIFO**: Queue of page load times
- **LRU**: Timestamp of last access
- **Clock**: Second-chance algorithm with reference bit

**Page Fault Handling**:
1. Parse virtual address → (page_number, offset)
2. Check page table valid bit
3. If invalid: page fault
   - Find free frame or evict victim
   - Load page from "disk"
   - Update page table
4. Construct physical address
5. Return to caller

**Metadata per Page**:
- Valid bit (in physical memory?)
- Dirty bit (modified?)
- Referenced bit (accessed? for Clock)
- Load time (for FIFO)
- Last access time (for LRU)

### 5. CLI and Manager (`cli/`, `manager/`)

**Purpose**: User interface and system orchestration.

**Key Classes**:
- `CommandParser`: Parses user commands
- `CLI`: REPL interface
- `MemoryManager`: Orchestrates all subsystems

**Commands**:
- `init memory <size>`: Initialize physical memory
- `set allocator <type>`: Choose allocation strategy
- `malloc <size>`: Allocate memory block
- `free <id>`: Deallocate by block ID
- `free_addr <address>`: Deallocate by address
- `dump memory`: Visualize memory layout
- `stats`: Show statistics
- `help`: Display command help
- `exit`: Exit simulator

## Design Patterns Used

### 1. Strategy Pattern
**Where**: `IAllocator` interface
**Why**: Allows runtime selection of allocation strategy
**Implementation**: All allocators implement common interface

### 2. Template Pattern
**Where**: `Result<T>` error handling
**Why**: Type-safe error handling without exceptions
**Implementation**: Generic result type with success/error states

### 3. Factory Pattern (implicit)
**Where**: `MemoryManager` creates allocators
**Why**: Centralized creation logic
**Implementation**: Switch statement on allocator type

### 4. RAII (Resource Acquisition Is Initialization)
**Where**: Throughout codebase
**Why**: Automatic resource management
**Implementation**: Smart pointers, constructors/destructors

## Data Flow

### Memory Allocation Flow
```
User → CLI → MemoryManager → Allocator → PhysicalMemory
                                  ↓
                          Update metadata
                                  ↓
                          Return block info
```

### Cache Read Flow
```
User Request → CacheHierarchy → L1 Cache
                                   ↓ (miss)
                                L2 Cache
                                   ↓ (miss)
                              PhysicalMemory
                                   ↓
                          Load into L2 and L1
                                   ↓
                              Return data
```

### Virtual Memory Access Flow
```
Virtual Address → VirtualMemory → Page Table Lookup
                                        ↓ (valid)
                                   Frame Number
                                        ↓
                                Physical Address
                                        ↓ (invalid = page fault)
                               Find Free Frame / Evict
                                        ↓
                               Load Page from Disk
                                        ↓
                              Update Page Table
                                        ↓
                               Return Physical Address
```

## Memory Layout

### Standard Allocator Memory Layout
```
┌──────────┬──────────┬──────────┬──────────┐
│  Block 1 │  Block 2 │   Free   │  Block 3 │
│ (alloc)  │ (alloc)  │  Space   │ (alloc)  │
└──────────┴──────────┴──────────┴──────────┘
     ↕            ↕                     ↕
   prev/next   prev/next             prev/next
   pointers    pointers              pointers
```

### Buddy Allocator Memory Layout
```
┌────────────────────────────────┐
│          1024 bytes            │  Initial block
└────────────────────────────────┘
         Split ↓
┌───────────────┬────────────────┐
│   512 bytes   │   512 bytes    │  Buddies
└───────────────┴────────────────┘
   Split ↓
┌──────┬───────┬────────────────┐
│ 256  │  256  │   512 bytes    │
└──────┴───────┴────────────────┘
```

### Cache Structure
```
Set 0: [Line 0] [Line 1] ... [Line N-1]  (N-way)
Set 1: [Line 0] [Line 1] ... [Line N-1]
Set 2: [Line 0] [Line 1] ... [Line N-1]
...
```

### Page Table Structure
```
Virtual Page 0 → [Valid|Frame|Dirty|Ref|LoadTime|LastAccess]
Virtual Page 1 → [Valid|Frame|Dirty|Ref|LoadTime|LastAccess]
...
Virtual Page N → [Valid|Frame|Dirty|Ref|LoadTime|LastAccess]
```

## Key Algorithms

### 1. Block Coalescing (Standard Allocator)
```
When freeing block:
1. Mark block as free
2. Check previous block: if free, merge
3. Check next block: if free, merge
4. Recursively coalesce until no adjacent free blocks
```

### 2. Buddy Splitting
```
To allocate size S:
1. Round S up to power of 2
2. Find smallest free block ≥ S
3. If no exact match:
   - Take next larger block
   - Split into two buddies
   - Repeat until exact size
```

### 3. Buddy Coalescing
```
When freeing block:
1. Calculate buddy address: buddy_addr = addr ^ size
2. Check if buddy is free
3. If free:
   - Merge buddies into larger block
   - Recursively coalesce parent
```

### 4. Cache Address Parsing
```
Given address and cache parameters:
offset_bits = log2(block_size)
index_bits = log2(num_sets)

offset = address & ((1 << offset_bits) - 1)
set_index = (address >> offset_bits) & ((1 << index_bits) - 1)
tag = address >> (offset_bits + index_bits)
```

### 5. LRU Replacement
```
For each cache access:
1. Update last_access_time = global_time++
2. On eviction:
   - Scan all lines in set
   - Find line with minimum last_access_time
   - Evict that line
```

### 6. Clock Page Replacement
```
Circular scan with reference bit:
1. Start at clock_hand position
2. If current page referenced:
   - Clear reference bit (second chance)
   - Move to next page
3. If current page not referenced:
   - Evict this page
   - Move clock_hand to next position
```

## Testing Strategy

### Test Pyramid
```
        /\
       /  \      Integration Tests (31)
      /----\     - Full system integration
     /      \    - Component interaction
    /--------\   - Workload simulations
   /          \
  /------------\ Unit Tests (126)
 /              \ - Individual components
/________________\ - Edge cases, stress tests
```

### Test Coverage

**Unit Tests (126 total)**:
- Physical Memory: 14 tests
- Standard Allocator: 27 tests
- Buddy Allocator: 27 tests
- Cache Level: 26 tests
- Virtual Memory: 32 tests

**Integration Tests (31 total)**:
- Cache Hierarchy: 18 tests
- Full System: 13 tests

**Total: 157 tests, all passing**

### Test Categories
1. **Constructor validation**: Invalid parameters
2. **Basic operations**: Read, write, allocate, free
3. **Algorithm correctness**: Replacement policies, coalescing
4. **Edge cases**: Boundary conditions, out of bounds
5. **Stress tests**: Heavy load, many operations
6. **Workload tests**: Realistic usage patterns
7. **Integration**: Component interaction

## Performance Characteristics

### Time Complexity

**Standard Allocator**:
- Allocate: O(n) where n = number of blocks
- Deallocate: O(n) for coalescing
- Best Fit/Worst Fit: O(n) scan

**Buddy Allocator**:
- Allocate: O(log n) for splitting
- Deallocate: O(log n) for coalescing
- Free list lookup: O(1) average

**Cache**:
- Lookup: O(associativity) scan within set
- Replacement: O(associativity)

**Virtual Memory**:
- Translation: O(1) page table lookup
- Page fault: O(n) for LRU, O(1) for FIFO

### Space Complexity

**Standard Allocator**:
- Overhead: O(n) for block metadata

**Buddy Allocator**:
- Overhead: O(m) where m = number of free lists

**Cache**:
- Storage: O(sets × associativity × block_size)

**Virtual Memory**:
- Page table: O(virtual_pages)

## Build System

### CMake Structure
```
CMakeLists.txt (root)
├── src/CMakeLists.txt (library)
└── tests/CMakeLists.txt (tests)
```

### Build Process
1. Fetch Google Test via CMake FetchContent
2. Build static library (memsim_lib)
3. Build main executable
4. Build test executables
5. Discover and register tests with CTest

### Compiler Flags
- C++17 standard
- Release mode optimization (-O3)
- All warnings enabled
- No exceptions (using Result<T> pattern)

## Future Enhancements

Possible extensions to the simulator:

1. **Write Policies**: Write-back cache
2. **TLB**: Translation Lookaside Buffer for faster address translation
3. **Multi-level Page Tables**: Hierarchical page tables
4. **Memory-Mapped I/O**: Simulate device memory
5. **Defragmentation**: Automatic memory compaction
6. **Parallel Access**: Multi-threaded memory access simulation
7. **Performance Metrics**: Detailed timing simulation
8. **GUI**: Graphical visualization of memory state

## References

- Operating System Concepts (Silberschatz, Galvin, Gagne)
- Computer Architecture: A Quantitative Approach (Hennessy, Patterson)
- Modern Operating Systems (Tanenbaum, Bos)
