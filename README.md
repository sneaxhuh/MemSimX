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

(Coming in Phase 3 - CLI Implementation)

Example commands:
```
> init memory 1024
> set allocator first_fit
> malloc 100
Allocated block id=1 at address=0x0000
> free 1
> dump memory
> stats
```

## Development Status

### Phase 1: Foundation ✓ COMPLETE
- [x] Project structure
- [x] CMake build system
- [x] Common types and Result<T> template
- [x] PhysicalMemory class
- [x] MemoryBlock structure
- [x] Unit tests for PhysicalMemory

### Phase 2: Standard Allocation (In Progress)
- [ ] IAllocator interface
- [ ] StandardAllocator (First/Best/Worst Fit)
- [ ] Block splitting and coalescing
- [ ] Fragmentation metrics
- [ ] Unit tests

### Phase 3: CLI and Basic Demo
- [ ] Command parser
- [ ] CLI interface
- [ ] Memory manager
- [ ] ASCII visualization

### Phase 4: Buddy Allocator
- [ ] Buddy allocation system
- [ ] Power-of-two rounding
- [ ] Buddy coalescing
- [ ] Unit tests

### Phase 5: Cache Hierarchy
- [ ] Cache line and level implementation
- [ ] FIFO/LRU/LFU policies
- [ ] Multilevel cache coordination
- [ ] Unit tests

### Phase 6: Virtual Memory
- [ ] Page table implementation
- [ ] Address translation
- [ ] Page replacement policies
- [ ] Unit tests

### Phase 7: System Integration
- [ ] Integrate all subsystems
- [ ] Full pipeline testing
- [ ] Workload tests

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
