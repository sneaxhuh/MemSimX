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

## Project Statistics

- **Total Lines of Code**: ~6,650 lines
- **Source Files**: 10 implementation files
- **Header Files**: 53 header files
- **Test Files**: 7 test files
- **Total Tests**: 157 (all passing)
  - Unit Tests: 126
  - Integration Tests: 31
- **Test Coverage**: All components tested
- **Build System**: CMake with C++17
- **Testing Framework**: Google Test

## Testing

### Run All Tests
```bash
cd build
ctest --output-on-failure
```

### Run Specific Test Suite
```bash
./unit_tests --gtest_filter=PhysicalMemoryTest.*
./unit_tests --gtest_filter=BuddyAllocatorTest.*
./unit_tests --gtest_filter=CacheLevelTest.*
./unit_tests --gtest_filter=VirtualMemoryTest.*
./integration_tests --gtest_filter=FullSystemTest.*
```

### Test Coverage
All 157 tests passing.

## Documentation

- **README.md**: Project overview and quick start
- **DESIGN_DOCUMENT.md**: System architecture and algorithms
- **Inline Documentation**: Comments in headers and source files

## Performance Characteristics

### Time Complexity
- **Standard Allocator**: O(n) allocation/deallocation
- **Buddy Allocator**: O(log n) allocation/deallocation
- **Cache Lookup**: O(associativity) per cache level
- **Virtual Memory Translation**: O(1) page table lookup
- **Page Replacement**: O(1) FIFO, O(n) LRU, O(n) Clock

### Space Complexity
- **Physical Memory**: O(memory_size)
- **Allocator Metadata**: O(number_of_blocks)
- **Cache Storage**: O(sets × associativity × block_size)
- **Page Table**: O(virtual_pages)

## Usage Examples

### Initialize and Allocate Memory
```
init memory 1024
set allocator first_fit
malloc 100
malloc 200
stats
dump memory
```

### Test Buddy Allocation
```
init memory 1024
set allocator buddy
malloc 50
malloc 100
malloc 200
stats
```

### View Statistics
```
stats
```

### Free Memory
```
free 1
free_addr 0x00000064
```