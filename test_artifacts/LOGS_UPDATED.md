# Test Artifacts - Updated Logs

All execution logs have been regenerated using the latest implementation with full CLI support for virtual memory and cache commands.

---

## Updated Files

### 1. [allocation_log.txt](allocation_log.txt)
**Generated from**: First Fit allocator with allocation, deallocation, and reallocation

**Demonstrates**:
- ✅ Initial allocations (1024, 2048, 512 bytes)
- ✅ Memory layout visualization with addresses
- ✅ Deallocation creating free blocks (freed block 2)
- ✅ Reallocation into freed space (1500 bytes allocated into freed 2048-byte block)
- ✅ Block splitting (2048-byte block split into 1500 used + 548 free)
- ✅ External fragmentation tracking (4.11%)
- ✅ Utilization metrics (21.88% → 18.53%)

**Key Statistics**:
- Total allocations: 4
- Deallocations: 1
- Success rate: 100%
- External fragmentation: 4.11%

---

### 2. [cache_access_log.txt](cache_access_log.txt)
**Generated from**: L1/L2 cache hierarchy with LRU replacement

**Demonstrates**:
- ✅ L1 cache: 8 sets, 2-way, 64-byte blocks, LRU
- ✅ L2 cache: 16 sets, 4-way, 64-byte blocks, LRU
- ✅ Cache writes (write-through to memory)
- ✅ Cache reads with hits and misses
- ✅ L1 hit ratio: 25.00%
- ✅ L2 hit ratio: 0.00%
- ✅ Overall hit ratio: 14.29%
- ✅ Detailed statistics for both cache levels

**Key Statistics**:
- Total accesses: 21
- L1 hits: 3
- L2 hits: 0
- Memory accesses: 9
- Overall hit ratio: 14.29%

---

### 3. [vm_access_log.txt](vm_access_log.txt)
**Generated from**: Virtual memory with LRU page replacement

**Demonstrates**:
- ✅ Virtual memory initialization (32 virtual pages, 8 physical frames, 512 bytes/page)
- ✅ LRU page replacement policy
- ✅ Virtual address writes causing page faults
- ✅ Virtual address reads
- ✅ Page fault rate: 100.00% (demand paging scenario)
- ✅ Page table dump showing valid pages and frame assignments
- ✅ Dirty bit tracking on writes

**Key Statistics**:
- Total accesses: 13
- Page faults: 13
- Page hits: 0
- Page fault rate: 100% (thrashing scenario)
- Pages in memory: 8

**Page Table Details**:
- Shows valid bit, frame number, dirty bit, reference bit, last access time
- Demonstrates LRU tracking with LastAccess timestamps

---

### 4. [integrated_system_log.txt](integrated_system_log.txt)
**Generated from**: Full system with Buddy allocator + Cache + Virtual Memory

**Demonstrates**:
- ✅ Buddy allocator (power-of-2 allocation)
- ✅ L1/L2 cache hierarchy (4-way/8-way associative)
- ✅ Virtual memory with LRU (64 virtual pages, 16 frames, 128 bytes/page)
- ✅ All three subsystems working together
- ✅ Statistics from all components
- ✅ Buddy memory layout with free lists

**Key Statistics**:
- **Allocator**: 2 allocations, 9.38% utilization, 44.83% external fragmentation
- **Cache**: L1 hit ratio 0%, L2 hit ratio 0%, Overall hit ratio 0%
- **VM**: 3 page faults, 2 page hits, 60% page fault rate, 40% hit rate

**Shows Integration**:
- Physical memory allocations
- Cache operations on physical addresses
- Virtual memory operations with address translation
- All subsystems producing coherent statistics

---

## Implementation Details

- ✅ Generated using CLI interface
- ✅ Uses `init memory`, `init cache`, `init vm` commands
- ✅ Shows complete CLI-based workflow
- ✅ Real execution traces from actual simulator runs

---

## Verification

All logs verified to show:

**Allocation Log**:
- [x] Allocation behavior with block IDs and addresses
- [x] Deallocation creating free blocks
- [x] Reallocation reusing freed space
- [x] Block splitting on partial allocation
- [x] External fragmentation metrics
- [x] Utilization tracking

**Cache Log**:
- [x] L1 and L2 cache hierarchy
- [x] Cache hits and misses
- [x] Hit ratio and miss ratio statistics
- [x] LRU replacement policy
- [x] Write-through behavior

**VM Log**:
- [x] Page faults on first access
- [x] Page replacement (LRU policy)
- [x] Page fault rate statistics
- [x] Page table dump with frame mappings
- [x] Dirty bit tracking

**Integrated Log**:
- [x] All three subsystems initialized
- [x] Buddy allocator with power-of-2 sizing
- [x] Cache operations
- [x] Virtual memory operations
- [x] Statistics from all components

---

## Usage

These logs serve as:

1. **Proof of Functionality**: Demonstrates that all components work correctly
2. **Test Artifacts**: Real execution traces for project evaluation
3. **Documentation**: Shows actual behavior and output format
4. **Reference**: Examples of expected outputs for correctness verification

---

## Related Files

- [ALLOCATION_WORKLOADS.md](ALLOCATION_WORKLOADS.md) - Predefined test workloads
- [EXPECTED_OUTPUTS.md](EXPECTED_OUTPUTS.md) - Correctness criteria
- [TEST_SCRIPTS.md](TEST_SCRIPTS.md) - Automated test documentation
- [README.md](README.md) - Test artifacts overview

---
