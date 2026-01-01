# Test Artifacts

This directory contains all test artifacts for the Memory Management Simulator project, organized for submission and evaluation.

## Contents

### 1. [allocation_log.txt](allocation_log.txt)
Actual memory allocation pattern log from test execution.

Demonstrates:
- First Fit allocation strategy
- Sequential allocations
- Deallocation and memory reuse
- Fragmentation metrics
- Utilization statistics

Format:
- Operation performed (allocate/deallocate)
- Block addresses and sizes
- Memory utilization percentages
- Fragmentation statistics

### 2. [cache_access_log.txt](cache_access_log.txt)
Actual cache access log from cache tests.

Demonstrates:
- Sequential access pattern showing spatial locality
- L1 and L2 cache behavior
- Cache hits and misses per access
- Block loading on misses
- Final hit ratio statistics (97.5% L1 hit ratio)

Format:
- Time, Address, L1 Status, L2 Status, Memory access
- Action taken (load block, served from cache)
- Final statistics with hit ratios

### 3. [vm_access_log.txt](vm_access_log.txt)
Actual virtual memory access log from VM tests.

Demonstrates:
- Page fault demonstration
- Demand paging (load on first access)
- LRU page replacement when frames exhausted
- 100% fault rate scenario (thrashing)

Format:
- Time, Virtual address, Page number, Frame number
- Status (PAGE FAULT / HIT)
- Action (load page, evict LRU)
- Final statistics

### 4. [integrated_system_log.txt](integrated_system_log.txt)
Combined system log showing all subsystems working together.

### 5. [ALLOCATION_WORKLOADS.md](ALLOCATION_WORKLOADS.md)
Documentation of 8 predefined test workloads.

Contains workloads testing:
- Sequential allocation, fragmentation stress, coalescing
- Buddy allocator specifics, strategy comparison
- Allocation failure, repeated operations

### 6. [EXPECTED_OUTPUTS.md](EXPECTED_OUTPUTS.md)
Correctness criteria and expected outputs for all components.

Defines pass/fail criteria for:
- Memory allocator invariants and tests
- Buddy allocator invariants and tests
- Cache correctness and performance
- Virtual memory behavior
- Integrated system testing

Includes quantitative success metrics for each subsystem.

### 7. [TEST_SCRIPTS.md](TEST_SCRIPTS.md)
Documentation of all automated test programs.

Describes:
- `test_allocator_algorithms.cpp` - Standard allocator tests
- `test_buddy_algorithms.cpp` - Buddy allocator tests
- `test_cache_comprehensive.cpp` - Cache hierarchy tests
- `test_vm_comprehensive.cpp` - Virtual memory tests

Each section includes:
- Purpose and scope
- What is tested
- How to compile and run
- Expected output
- Test coverage summary

---

## Quick Reference

### Running All Tests
```bash
cd /home/sneax/ACM_OpenProjects/memory-simulator

# Allocator
g++ tests/test_allocator_algorithms.cpp src/physical_memory.cpp src/allocator.cpp -Iinclude -o test_allocator
./test_allocator

# Buddy Allocator
g++ tests/test_buddy_algorithms.cpp src/physical_memory.cpp src/buddy_allocator.cpp -Iinclude -o test_buddy
./test_buddy

# Cache
g++ tests/test_cache_comprehensive.cpp src/physical_memory.cpp src/cache.cpp -Iinclude -o test_cache
./test_cache

# Virtual Memory
g++ tests/test_vm_comprehensive.cpp src/physical_memory.cpp src/virtual_memory.cpp -Iinclude -o test_vm
./test_vm
```

### Test Results Summary
All test programs are fully automated and self-validating:
- Exit code 0 = All tests passed
- Exit code non-zero = Test failure

---

## Organization

This directory is organized to satisfy project submission requirements:

**Actual Execution Logs**:
- [allocation_log.txt](allocation_log.txt) - Real memory allocation pattern from tests
- [cache_access_log.txt](cache_access_log.txt) - Real cache access trace (97.5% hit ratio)
- [vm_access_log.txt](vm_access_log.txt) - Real VM access trace (thrashing scenario)
- [integrated_system_log.txt](integrated_system_log.txt) - Combined system execution

**Test Documentation**:
- [ALLOCATION_WORKLOADS.md](ALLOCATION_WORKLOADS.md) - Predefined test workload specifications
- [EXPECTED_OUTPUTS.md](EXPECTED_OUTPUTS.md) - Correctness criteria and invariants
- [TEST_SCRIPTS.md](TEST_SCRIPTS.md) - Test execution guide and coverage summary

---

## Relationship to Design Document

These artifacts complement the main [DESIGN_DOCUMENT.md](../DESIGN_DOCUMENT.md):

- **Design Document** → Describes WHAT the system does
- **Test Artifacts** → Demonstrates that it WORKS correctly

Together they provide:
1. Theoretical foundation (design)
2. Empirical validation (test artifacts)
3. Reproducible verification (test scripts)

---

## Artifact Summary

| File | Purpose | Type |
|------|---------|------|
| allocation_log.txt | Real allocation execution trace | Actual log |
| cache_access_log.txt | Real cache access trace (97.5% hit) | Actual log |
| vm_access_log.txt | Real VM access trace (100% fault) | Actual log |
| integrated_system_log.txt | Combined system execution | Actual log |
| ALLOCATION_WORKLOADS.md | Test workload specifications | Documentation |
| EXPECTED_OUTPUTS.md | Correctness criteria | Documentation |
| TEST_SCRIPTS.md | Test execution guide | Documentation |

---

## Notes for Evaluators

**Real execution logs included**:
- [allocation_log.txt](allocation_log.txt), [cache_access_log.txt](cache_access_log.txt), [vm_access_log.txt](vm_access_log.txt) are actual outputs from test runs
- Not example data - these are real traces from the implemented system
- Demonstrates actual behavior and performance (97.5% L1 cache hit ratio, etc.)

**Tests are automated**:
- No manual intervention required
- Self-checking with assertions
- Clear pass/fail output

**Artifacts are self-contained**:
- Each log file is human-readable
- Documentation explains what to look for
- Can be evaluated independently

---

## Project Structure

```
memory-simulator/
├── DESIGN_DOCUMENT.md          # System design specification
├── test_artifacts/             # This directory
│   ├── README.md               # This file
│   ├── allocation_log.txt      # Real allocation execution log
│   ├── cache_access_log.txt    # Real cache access log
│   ├── vm_access_log.txt       # Real VM access log
│   ├── integrated_system_log.txt # Combined system log
│   ├── ALLOCATION_WORKLOADS.md # Test workload specifications
│   ├── EXPECTED_OUTPUTS.md     # Correctness criteria
│   └── TEST_SCRIPTS.md         # Test execution guide
├── tests/                      # Test implementations
│   ├── test_allocator_algorithms.cpp
│   ├── test_buddy_algorithms.cpp
│   ├── test_cache_comprehensive.cpp
│   └── test_vm_comprehensive.cpp
└── src/                        # Implementation
    ├── physical_memory.cpp
    ├── allocator.cpp
    ├── buddy_allocator.cpp
    ├── cache.cpp
    └── virtual_memory.cpp
```

---

## Verification Checklist

- [x] Input workloads documented
- [x] Actual cache access logs from test execution
- [x] Actual virtual memory logs from test execution
- [x] Actual allocation logs from test execution
- [x] Expected outputs and correctness criteria defined
- [x] Test scripts documented
- [x] All tests automated
- [x] Real execution traces included (not examples)
- [x] Relationship to design document clear

All test artifacts complete and ready for evaluation.
