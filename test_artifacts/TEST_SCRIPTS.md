# Test Scripts Documentation

## Overview

This document describes all test programs included in the memory simulator project, what they test, and how to run them.

The project includes two test suites:
1. **Google Test Suite** (`tests/unit/`, `tests/integration/`) - Main comprehensive automated tests
2. **Standalone Tests** (`testing/`) - Additional demonstration tests
3. **Log Generator** (`scripts/`) - Generates actual execution logs

---

## Google Test Suite (Main Test Suite)

### 1. tests/unit/test_standard_allocator.cpp

**Purpose**: Comprehensive tests for standard memory allocation strategies.

**What It Tests**:
- First Fit allocation strategy
- Best Fit allocation strategy
- Worst Fit allocation strategy
- Block splitting when allocation smaller than free block
- Block coalescing when adjacent blocks freed
- External fragmentation handling
- Memory accounting (used + free = total)
- Free list integrity

**Key Test Cases**:
- Sequential allocation without fragmentation
- Fragmentation stress test
- Coalescing adjacent free blocks
- Strategy comparison on identical workloads
- Edge cases (allocation failure, full memory, empty memory)

**Run Instructions**:
```bash
cd /home/sneax/ACM_OpenProjects/memory-simulator/build
ctest --test-dir . -R StandardAllocator -V
```

**Expected Output**:
```
[==========] Running tests from StandardAllocatorTest
[  PASSED  ] All tests passed
```

---

### 2. tests/unit/test_buddy_allocator.cpp

**Purpose**: Tests buddy allocator implementation with power-of-2 sizing and XOR-based buddy calculation.

**What It Tests**:
- Power-of-2 size rounding
- XOR buddy address calculation
- Free list organization by size
- Block splitting (recursive)
- Block coalescing (recursive)
- Internal fragmentation tracking
- Buddy pairing validation

**Key Test Cases**:
- Allocation rounds to power-of-2
- Buddy calculation using XOR formula
- Splitting large blocks into smaller buddies
- Coalescing buddies recursively up the tree
- Free list integrity across multiple operations

**Run Instructions**:
```bash
cd /home/sneax/ACM_OpenProjects/memory-simulator/build
ctest --test-dir . -R BuddyAllocator -V
```

**Expected Output**:
```
[==========] Running tests from BuddyAllocatorTest
[  PASSED  ] All tests passed
```

---

### 3. tests/unit/test_cache_level.cpp

**Purpose**: Tests cache level implementation with set-associative mapping and LRU replacement.

**What It Tests**:
- L1 and L2 cache structure
- Set-associative mapping
- LRU replacement policy
- Spatial locality (sequential accesses within block)
- Temporal locality (repeated access to same address)
- Write-through policy
- No write-allocate policy
- Cache hit/miss counting
- Set index calculation

**Key Test Cases**:
- Spatial locality test (consecutive addresses)
- Temporal locality test (repeated accesses)
- LRU eviction test (fill set beyond capacity)
- Write-through test (verify memory updated)
- No write-allocate test (write miss behavior)
- Cache hierarchy test (L1 miss, L2 hit)
- Thrashing test (working set exceeds cache)

**Run Instructions**:
```bash
cd /home/sneax/ACM_OpenProjects/memory-simulator/build
ctest --test-dir . -R CacheLevel -V
```

**Expected Output**:
```
[==========] Running tests from CacheLevelTest
[  PASSED  ] All tests passed
```

---

### 4. tests/unit/test_virtual_memory.cpp

**Purpose**: Tests virtual memory management with demand paging and page replacement.

**What It Tests**:
- Single-level page table
- Virtual to physical address translation
- Demand paging (load on first access)
- LRU page replacement
- Page fault handling
- Page hit detection
- Working set management
- Thrashing detection
- Invalid page access handling

**Key Test Cases**:
- Demand paging (first access causes fault)
- Address translation (VA → PA conversion)
- LRU eviction (evict oldest page when frames full)
- Page fault rate calculation
- Working set vs. physical frame comparison
- Thrashing scenario (working set > frames)
- Invalid virtual address rejection

**Run Instructions**:
```bash
cd /home/sneax/ACM_OpenProjects/memory-simulator/build
ctest --test-dir . -R VirtualMemory -V
```

**Expected Output**:
```
[==========] Running tests from VirtualMemoryTest
[  PASSED  ] All tests passed
```

---

### 5. tests/integration/test_cache_integration.cpp

**Purpose**: Integration tests for cache hierarchy with L1 and L2 interaction.

**Run Instructions**:
```bash
cd /home/sneax/ACM_OpenProjects/memory-simulator/build
ctest --test-dir . -R CacheIntegration -V
```

---

### 6. tests/integration/test_full_system.cpp

**Purpose**: End-to-end integration tests for the complete memory system.

**Run Instructions**:
```bash
cd /home/sneax/ACM_OpenProjects/memory-simulator/build
ctest --test-dir . -R FullSystem -V
```

---

## Running All Google Tests

```bash
cd /home/sneax/ACM_OpenProjects/memory-simulator/build
ctest --test-dir . -V
```

Or run specific test suites:
```bash
# Unit tests only
ctest --test-dir . -R "unit" -V

# Integration tests only
ctest --test-dir . -R "integration" -V
```

---

## Log Generator Script

### scripts/generate_test_logs.cpp

**Purpose**: Generates actual execution logs from running the simulator.

**What It Generates**:
- `logs/cache_access_log.txt` - Real cache access trace
- `logs/vm_access_log.txt` - Real VM access trace
- `logs/allocation_log.txt` - Real allocation pattern log
- `logs/integrated_system_log.txt` - Combined system log

**Run Instructions**:
```bash
cd /home/sneax/ACM_OpenProjects/memory-simulator/build
./generate_test_logs
```

**Expected Output**:
```
=== Real Test Log Generator ===
✓ Cache access log written to logs/cache_access_log.txt
✓ VM access log written to logs/vm_access_log.txt
✓ Integrated system log written to logs/integrated_system_log.txt
✓ Allocation log written to logs/allocation_log.txt
✅ All logs generated successfully!
```

These logs are copied to `test_artifacts/` for submission

---

## Test Coverage Summary

All subsystems have comprehensive test coverage using Google Test framework.

### Unit Tests Coverage
| Test File | Component Tested | Tests Count |
|-----------|-----------------|-------------|
| test_physical_memory.cpp | Physical memory operations | 10+ |
| test_standard_allocator.cpp | First/Best/Worst Fit allocators | 30+ |
| test_buddy_allocator.cpp | Buddy allocation system | 15+ |
| test_cache_level.cpp | Cache hierarchy (L1/L2) | 25+ |
| test_virtual_memory.cpp | VM and page replacement | 20+ |

### Integration Tests Coverage
| Test File | Integration Tested | Tests Count |
|-----------|-------------------|-------------|
| test_cache_integration.cpp | L1+L2 cache interaction | 10+ |
| test_full_system.cpp | Complete memory system | 15+ |

### Feature Coverage
| Feature | Unit Tests | Integration Tests | Log Generator |
|---------|-----------|-------------------|---------------|
| Memory Allocators | ✓ | ✓ | ✓ |
| Buddy System | ✓ | ✓ | - |
| Cache Hierarchy | ✓ | ✓ | ✓ |
| Virtual Memory | ✓ | ✓ | ✓ |
| Full System | - | ✓ | ✓ |

---

## Build and Test

### Building the Project

```bash
cd /home/sneax/ACM_OpenProjects/memory-simulator
mkdir -p build
cd build
cmake ..
make
```

### Running All Tests

```bash
# Run all tests with CTest
cd build
ctest --test-dir .

# Run with verbose output
ctest --test-dir . -V

# Run specific test
ctest --test-dir . -R StandardAllocator
```

### Test Framework

All tests use Google Test (gtest) framework:
- Automatic test discovery
- Detailed failure messages
- Test fixtures for setup/teardown
- Parameterized tests where applicable

### Test Output

**Success**:
```
[==========] Running 45 tests from 5 test suites.
[----------] Global test environment set-up.
...
[  PASSED  ] 45 tests.
```

**Failure**:
```
[  FAILED  ] StandardAllocatorTest.FirstFit_Coalescing
Expected equality of these values:
  free_blocks
    Which is: 2
  1
```

---

## Troubleshooting

**Build Errors**:
```bash
# Clean and rebuild
cd build
rm -rf *
cmake ..
make
```

**Test Not Found**:
```bash
# List all available tests
ctest --test-dir . -N
```

**Google Test Not Found**:
- Ensure gtest is installed: `sudo apt-get install libgtest-dev`
- Or CMake will fetch it automatically if configured

**Segmentation Fault in Tests**:
```bash
# Run with debugger
gdb ./build/unit_tests
run
bt  # backtrace when it crashes
```

---

## Adding New Tests

To add new test cases using Google Test:

1. Open the appropriate test file in `tests/unit/` or `tests/integration/`
2. Add new TEST_F within the test fixture

Example:
```cpp
TEST_F(StandardAllocatorTest, NewFeature_Description) {
    createAllocator(AllocatorType::FIRST_FIT);

    // Test operation
    auto result = allocator->allocate(100);

    // Verify expectations
    ASSERT_TRUE(result.success);
    EXPECT_EQ(allocator->getUtilization(), expected_value);
}
```

3. Rebuild and run:
```bash
cd build
make
ctest --test-dir . -R NewFeature -V
```

---

## Summary

The project includes comprehensive testing at multiple levels:

**Google Test Suite** (`tests/`):
- 100+ unit tests covering all components
- Integration tests for full system behavior
- Automated with CTest
- Industry-standard testing framework

**Log Generator** (`scripts/generate_test_logs.cpp`):
- Generates real execution traces
- Produces actual logs for documentation
- Demonstrates system behavior with real data

**Test Artifacts** (`test_artifacts/`):
- Real logs from actual execution (not examples)
- Comprehensive test workload documentation
- Expected outputs and correctness criteria

To verify the entire system:
```bash
cd /home/sneax/ACM_OpenProjects/memory-simulator/build
ctest --test-dir . -V
```

All tests passing confirms correctness of all subsystems.
