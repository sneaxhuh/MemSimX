# Expected Outputs and Correctness Criteria

## Memory Allocator Correctness Criteria

### Invariants

**Memory Accounting**:
- Used memory + Free memory = Total memory
- At all times: sum of all block sizes = Total memory

**Block Integrity**:
- No overlapping blocks
- Block addresses within valid range [0, MEMORY_SIZE)
- Block size > 0

**Free List Integrity**:
- All blocks in free list marked as free
- All allocated blocks NOT in free list
- Free list properly linked (no cycles, no broken links)

**Coalescing**:
- No two adjacent free blocks
- After free(), adjacent blocks merge immediately

**Allocation Strategy**:
- First-fit: Returns first block with size ≥ requested
- Best-fit: Returns smallest block with size ≥ requested
- Worst-fit: Returns largest available block

### Test Expectations

**Test: Sequential Allocation**
```
Input:  malloc(100), malloc(200), malloc(300)
Expected:
  - All allocations succeed
  - Used memory = 100 + 200 + 300 + metadata overhead
  - 3 blocks allocated
  - No fragmentation
```

**Test: Coalescing**
```
Input:  malloc(100), malloc(200), malloc(300), free(200), free(300)
Expected:
  - Blocks at offset 100 and 300 merge
  - Free list contains merged block of size 500
  - No adjacent free blocks remain
```

**Test: Fragmentation**
```
Input:  malloc(100), malloc(200), malloc(100), free(200)
Expected:
  - 200-byte hole created
  - External fragmentation = 200 bytes
  - Subsequent malloc(250) fails (no block large enough)
  - Subsequent malloc(180) succeeds in 200-byte hole (best-fit/first-fit)
```

**Test: Double Free Detection**
```
Input:  p = malloc(100), free(p), free(p)
Expected:
  - Second free() rejected
  - No corruption
  - Error message or assertion failure
```

---

## Buddy Allocator Correctness Criteria

### Invariants

**Power-of-2 Sizing**:
- All block sizes are powers of 2
- Requested size rounded up to next power of 2

**Buddy Calculation**:
- Buddy address = current_address XOR size
- Buddies have same size
- Buddies are adjacent in memory

**Free List Organization**:
- Each free list contains blocks of one size only
- Free list index i contains blocks of size 2^i

**Coalescing**:
- When both buddies free, they merge recursively
- Merged block placed in next size free list

### Test Expectations

**Test: Power-of-2 Rounding**
```
Input:  malloc(100)
Expected:
  - Allocated size = 128 (next power of 2)
  - Internal fragmentation = 28 bytes
  - Block placed in size-128 free list
```

**Test: Buddy Coalescing**
```
Input:
  a = malloc(128)  // Block at 0x0000
  b = malloc(128)  // Block at 0x0080
  free(a)
  free(b)
Expected:
  - Blocks 0x0000 and 0x0080 are buddies (0x0000 XOR 128 = 0x0080)
  - They merge into 256-byte block at 0x0000
  - 256-byte block added to free list[8]
```

**Test: Splitting**
```
Input:  malloc(64) when only 256-byte block available
Expected:
  - 256-byte block splits into two 128-byte buddies
  - One 128-byte block splits into two 64-byte buddies
  - One 64-byte block allocated
  - Free lists: 64(1), 128(1)
```

**Test: XOR Buddy Verification**
```
Given:  Block at address A, size S
Expected:
  - Buddy address = A XOR S
  - (A XOR S) XOR S = A (identity)
  - Both blocks aligned to size S
```

---

## Cache Correctness Criteria

### Invariants

**Hit/Miss Accounting**:
- Total accesses = Hits + Misses
- Hit ratio = Hits / Total accesses ∈ [0, 1]

**Set Mapping**:
- Set index = (address / block_size) % num_sets
- All addresses in same set map to same set index

**LRU Replacement**:
- On miss, least recently used block in set evicted
- On hit, accessed block becomes most recently used

**Cache Hierarchy**:
- L1 miss → check L2
- L2 miss → fetch from memory
- Data in L1 ⊆ data in L2 (inclusive policy, if implemented)

**Write Policy**:
- Write-through: Writes update cache AND memory
- No write-allocate: Write misses bypass cache

### Test Expectations

**Test: Spatial Locality**
```
Input:  READ 0x1000, READ 0x1001, READ 0x1002
Expected:
  - First access: L1 MISS, loads block [0x1000-0x103F]
  - Subsequent accesses: L1 HIT
  - Hit ratio = 66.7%
```

**Test: LRU Eviction**
```
Input:
  READ 0x0000 (set 0, way 0)
  READ 0x0200 (set 0, way 1)
  READ 0x0000 (set 0, way 0) - access
  READ 0x0400 (set 0, full)
Expected:
  - 0x0400 evicts 0x0200 (LRU)
  - 0x0000 remains (recently accessed)
```

**Test: Write-Through**
```
Input:  WRITE 0x2000 (cache hit)
Expected:
  - Cache block updated
  - Memory location 0x2000 updated
  - Both cache and memory consistent
```

**Test: No Write-Allocate**
```
Input:  WRITE 0x3000 (cache miss)
Expected:
  - Write goes to memory
  - Block NOT loaded into cache
  - Subsequent READ 0x3000 misses
```

---

## Virtual Memory Correctness Criteria

### Invariants

**Address Translation**:
- VA = Page_number × Page_size + Offset
- PA = Frame_number × Page_size + Offset
- Offset preserved in translation

**Page Table**:
- One entry per virtual page
- Entry contains: valid bit, frame number, timestamp/reference bit

**Demand Paging**:
- Pages loaded only on first access
- Page fault on first access to each page

**LRU Replacement**:
- On page fault with no free frames, evict LRU page
- LRU tracked via timestamp or reference bit

**Working Set**:
- Active pages fit in physical memory → low fault rate
- Working set > physical frames → thrashing

### Test Expectations

**Test: Demand Paging**
```
Input:  Access VA 0x0000, VA 0x0200
Expected:
  - First access to each page causes fault
  - Page loaded into free frame
  - Subsequent access to same page hits
  - Fault rate = 50%
```

**Test: LRU Eviction**
```
Input:
  Access pages 0,1,2,3,4,5,6,7 (fill all 8 frames)
  Access page 8
Expected:
  - Page 0 evicted (LRU)
  - Page 8 loaded into frame previously holding page 0
  - Accessing page 0 again causes fault
```

**Test: Address Translation**
```
Input:  VA 0x0234
Expected:
  - Page number = 0x0234 / 512 = 1
  - Offset = 0x0234 % 512 = 0x34
  - If page 1 → frame 3, then PA = 3 × 512 + 0x34 = 0x0634
```

**Test: Page Fault Rate**
```
Input:  Uniform random access to 16 pages with 8 frames
Expected:
  - Initial faults: 8 (loading first 8 pages)
  - Steady-state fault rate depends on locality
  - Worst case (no locality): ~50% fault rate
  - Best case (perfect locality): 0% fault rate after warm-up
```

**Test: Invalid Page Access**
```
Input:  VA with page number ≥ 32
Expected:
  - Segmentation fault / error
  - No memory corruption
```

**Test: Thrashing Detection**
```
Input:  Working set = 10 pages, physical frames = 8
Expected:
  - Fault rate > 80%
  - Continuous eviction and reload
  - System spends more time paging than executing
```

---

## Integrated System Correctness Criteria

### End-to-End Invariants

**Memory Safety**:
- No buffer overflows
- No use-after-free
- No double-free
- No memory leaks

**Consistency**:
- Cache coherent with memory (write-through ensures this)
- Page table consistent with physical frames
- Virtual and physical addresses correctly mapped

**Performance Expectations**:
- Cache hit ratio > 50% for programs with locality
- Page fault rate < 10% for working set ≤ physical frames
- Allocator fragmentation < 20% for typical workloads

### Test Expectations

**Test: Memory Allocator + Cache**
```
Input:
  p = malloc(1024)
  WRITE p[0]
  READ p[0]
Expected:
  - Allocation succeeds
  - Write updates cache and memory
  - Read hits in cache
```

**Test: Virtual Memory + Cache**
```
Input:
  Access VA 0x0000 (triggers page fault)
  Access VA 0x0000 (page now resident)
Expected:
  - First access: page fault + cache miss
  - Second access: page hit + cache hit (if cached)
```

**Test: Full Integration**
```
Input:
  p = malloc(512)           // Allocator
  WRITE p[0] = 42          // Cache + Memory
  Context switch           // VM may evict page
  READ p[0]                // May cause page fault
Expected:
  - Value 42 preserved (written to memory)
  - System state consistent across subsystems
```

---

## Quantitative Success Criteria

### Memory Allocator
- **Pass**: All allocations succeed when sufficient memory available
- **Pass**: Free memory correctly reclaimed
- **Pass**: No overlapping blocks detected
- **Pass**: Coalescing reduces free list size

### Buddy Allocator
- **Pass**: All block sizes are powers of 2
- **Pass**: Buddy calculation correct (A XOR S)
- **Pass**: Coalescing reduces internal fragmentation
- **Pass**: Free lists properly organized by size

### Cache
- **Pass**: Hit ratio ∈ [0%, 100%]
- **Pass**: Set mapping formula correctly applied
- **Pass**: LRU evicts correct block
- **Pass**: Write-through maintains cache-memory consistency

### Virtual Memory
- **Pass**: Page faults + page hits = total accesses
- **Pass**: Address translation correct for all VAs
- **Pass**: LRU evicts least recently used page
- **Pass**: Invalid page access rejected

### Overall System
- **Pass**: All subsystem tests pass
- **Pass**: No memory corruption detected
- **Pass**: No crashes or undefined behavior
- **Pass**: Performance within expected bounds
