# Memory Management Simulator

A comprehensive memory management simulator that models OS-level memory management with dynamic allocation strategies, multilevel caching, and virtual memory with paging.

## Features

### Core Features
- **Physical Memory Simulation**: Configurable-size contiguous memory block
- **Multiple Allocation Strategies**:
  - First Fit
  - Best Fit
  - Worst Fit
  - Buddy Allocation System (power-of-two)
- **Multilevel Cache**: L1/L2 cache with FIFO, LRU, and LFU replacement policies
- **Virtual Memory**: Paging with FIFO, LRU, and Clock page replacement policies
- **Interactive CLI**: Command-line interface with ASCII visualization
- **Comprehensive Testing**: Unit and integration tests with Google Test

## Building the Project

### Requirements
- CMake 3.14 or higher
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Git (for fetching Google Test)

### Build Instructions

```bash
# Clone or navigate to the project directory
cd memory-simulator

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc)

# Run tests
ctest --output-on-failure

# Run the simulator
./memsim
```

### Debug Build

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

## Project Structure

```
memory-simulator/
├── CMakeLists.txt           # Root build configuration
├── README.md                # This file
├── src/                     # Source files
│   ├── memory/              # Physical memory implementation
│   ├── allocator/           # Allocation algorithms
│   ├── cache/               # Cache hierarchy
│   ├── virtual_memory/      # Virtual memory and paging
│   ├── manager/             # Memory manager (orchestrator)
│   └── cli/                 # Command-line interface
├── include/                 # Header files
│   ├── common/              # Common types and utilities
│   ├── memory/
│   ├── allocator/
│   ├── cache/
│   ├── virtual_memory/
│   ├── manager/
│   └── cli/
├── tests/                   # Test files
│   ├── unit/                # Unit tests
│   ├── integration/         # Integration tests
│   ├── workloads/           # Test workloads
│   └── scripts/             # Test scripts
└── docs/                    # Documentation
```

## Usage

The simulator provides an interactive command-line interface:

```bash
./memsim
```

### Example Session

```
> init memory 1024
Memory initialized: 1024 bytes

> set allocator first_fit
Allocator set to: First Fit

> malloc 100
Allocated block id=1 at address=0x0000

> malloc 200
Allocated block id=2 at address=0x0000

> dump memory
=== Memory Layout (1024 bytes) ===
[0x0000 - 0x0063] USED (id=1, 100 bytes)
[0x0064 - 0x012b] USED (id=2, 200 bytes)
[0x012c - 0x03ff] FREE (724 bytes)

> free 1
Block 1 freed

> stats
=== Allocator Statistics ===
Strategy: First Fit
Total memory: 1024 bytes
Used memory: 200 bytes
Free memory: 824 bytes
Utilization: 19.53%
...
```

### Available Commands

- **`init memory <size>`** - Initialize physical memory
- **`set allocator <type>`** - Set allocation strategy (first_fit, best_fit, worst_fit, buddy)
- **`malloc <size>`** - Allocate memory block
- **`free <block_id>`** - Free block by ID
- **`free_addr <address>`** - Free block by address
- **`dump memory`** - Display memory layout
- **`stats`** - Show detailed statistics
- **`help`** - Display help
- **`exit`** - Exit simulator

## Development Status

### Phase 1: Foundation ✓ COMPLETE
- [x] Project structure
- [x] CMake build system
- [x] Common types and Result<T> template
- [x] PhysicalMemory class
- [x] MemoryBlock structure
- [x] Unit tests for PhysicalMemory (14 tests)

### Phase 2: Standard Allocation ✓ COMPLETE
- [x] IAllocator interface
- [x] StandardAllocator (First/Best/Worst Fit)
- [x] Block splitting and coalescing
- [x] Fragmentation metrics (internal/external)
- [x] Memory utilization tracking
- [x] Comprehensive unit tests (27 tests)
- [x] All three allocation strategies tested

### Phase 3: CLI and Basic Demo ✓ COMPLETE
- [x] Command parser with full command set
- [x] CLI interface with REPL loop
- [x] Memory manager orchestrator
- [x] ASCII visualization of memory layout
- [x] Interactive help system
- [x] Error handling and user feedback

### Phase 4: Buddy Allocator ✓ COMPLETE
- [x] Buddy allocation system with free lists
- [x] Power-of-two rounding logic
- [x] Recursive block splitting
- [x] Buddy address calculation using XOR
- [x] Recursive buddy coalescing
- [x] Memory size validation (must be power of 2)
- [x] Comprehensive unit tests (27 tests)
- [x] CLI integration with 'buddy' allocator type

### Phase 5: Cache Hierarchy ✓ COMPLETE
- [x] CacheLine structure with metadata
- [x] CacheLevel class with set-associative support
- [x] FIFO replacement policy
- [x] LRU replacement policy
- [x] LFU replacement policy
- [x] CacheHierarchy with L1/L2 coordination
- [x] Write-through cache policy
- [x] Address parsing (tag, set index, block offset)
- [x] Comprehensive unit tests (26 tests)
- [x] Integration tests for cache hierarchy (18 tests)

### Phase 6: Virtual Memory ✓ COMPLETE
- [x] PageTableEntry structure with metadata
- [x] VirtualMemory class with paging system
- [x] Address translation (virtual → physical)
- [x] Page table with valid/dirty/referenced bits
- [x] FIFO page replacement (queue-based)
- [x] LRU page replacement (timestamp-based)
- [x] Clock page replacement (second chance algorithm)
- [x] Page fault handling and eviction
- [x] Frame allocation tracking
- [x] Simulated disk operations (load/write)
- [x] Comprehensive unit tests (32 tests)

### Phase 7: System Integration ✓ COMPLETE
- [x] Full system integration tests (13 tests)
- [x] Allocator + Cache integration
- [x] Virtual Memory + Cache integration
- [x] Full pipeline: Allocator + Cache + Virtual Memory
- [x] Component interaction tests
- [x] Workload tests (sequential, random, temporal locality)
- [x] Stress testing under load
- [x] Performance comparison tests

### Phase 8: Testing and Documentation
- [ ] Comprehensive test coverage
- [ ] Design document
- [ ] User manual
- [ ] Performance analysis

## Testing

Run all tests:
```bash
cd build
ctest --output-on-failure
```

Run specific test:
```bash
./unit_tests --gtest_filter=PhysicalMemoryTest.*
```

## License

Academic project for ACM Open Projects.

## Authors

Implementation based on the Memory Management Simulator specification.
