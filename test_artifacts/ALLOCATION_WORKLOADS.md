# Input Workloads - Memory Allocation Patterns

## Workload W1: Sequential Allocation
**Purpose**: Tests contiguous growth and block splitting

**Operations**:
```
malloc(100)
malloc(200)
malloc(300)
```

**Expected**:
- Memory splits correctly
- No fragmentation
- Used memory = 600 bytes

---

## Workload W2: Fragmentation Stress
**Purpose**: Create external fragmentation

**Operations**:
```
malloc(100)
malloc(200)
malloc(300)
free(200)
malloc(180)
```

**Expected**:
- First-fit and best-fit use 200-byte hole
- Worst-fit uses largest free block

---

## Workload W3: Coalescing Test
**Purpose**: Verify adjacent free blocks merge

**Operations**:
```
malloc(100)
malloc(200)
malloc(300)
free(200)
free(300)
```

**Expected**:
- Single free block of size 924 bytes

---

## Workload W4: Buddy System Power-of-2 Allocation
**Purpose**: Verify buddy allocator rounds up to power-of-2 sizes

**Operations**:
```
malloc(100)  // rounds to 128
malloc(250)  // rounds to 256
malloc(500)  // rounds to 512
```

**Expected**:
- All allocations rounded to nearest power-of-2
- Internal fragmentation: (128-100) + (256-250) + (512-500) = 46 bytes

---

## Workload W5: Buddy Coalescing
**Purpose**: Test recursive buddy coalescing

**Operations**:
```
malloc(128)  // Block A at 0x0000
malloc(128)  // Block B at 0x0080
malloc(128)  // Block C at 0x0100
malloc(128)  // Block D at 0x0180
free(B)      // Free 0x0080
free(A)      // Should coalesce A+B into 256-byte block
free(D)      // Free 0x0180
free(C)      // Should coalesce C+D, then merge with A+B into 512-byte block
```

**Expected**:
- Final state: One 512-byte free block at 0x0000
- Demonstrates recursive coalescing

---

## Workload W6: Mixed Strategy Comparison
**Purpose**: Compare First/Best/Worst Fit on same sequence

**Operations**:
```
malloc(100)
malloc(200)
malloc(150)
free(200)
malloc(50)
malloc(180)
```

**Expected**:
- First-fit: Places 50 and 180 in first available holes
- Best-fit: Places 50 in 200-byte hole, 180 may fail or use different hole
- Worst-fit: Places 50 in largest hole, different fragmentation pattern

---

## Workload W7: Allocation Failure
**Purpose**: Test behavior when memory exhausted

**Operations**:
```
malloc(8000)  // Assuming total memory < 8000
```

**Expected**:
- Returns nullptr
- No memory corruption
- Free list remains valid

---

## Workload W8: Repeated Allocation/Deallocation
**Purpose**: Stress test allocator stability

**Operations**:
```
for i in 1..100:
  p[i] = malloc(64)
for i in 1..100:
  free(p[i])
```

**Expected**:
- All allocations succeed
- All memory reclaimed
- No leaks or corruption
- Final free memory = initial free memory
