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
- **Virtual Memory**: Paging with FIFO and LRU page replacement policies
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

### 📘 Available Commands

#### 🧠 Memory Management
- **`init memory <size>`** – Initialize physical memory with the specified size (in bytes)  
  _Example:_ `init memory 1024`

---

#### 🧩 Allocator Configuration
- **`set allocator <type>`** – Set the memory allocation strategy  
  _Types:_ `first_fit`, `best_fit`, `worst_fit`, `buddy`  
  _Example:_ `set allocator first_fit`  
  _Note:_ Buddy allocator rounds allocations to powers of two and coalesces free buddies automatically

---

#### 📦 Memory Operations
- **`malloc <size>`** – Allocate a memory block of the given size  
  _Example:_ `malloc 100`

- **`free <block_id>`** – Deallocate a memory block by block ID  
  _Example:_ `free 1`

- **`free_addr <physical_address>`** – Deallocate a memory block by physical address  
  _Example:_ `free_addr 0`

---

#### 🧮 Cache Hierarchy
- **`init cache <l1_s> <l1_a> <l1_b> <l1_p> <l2_s> <l2_a> <l2_b> <l2_p>`** – Initialize L1/L2 cache hierarchy  
  _Example:_ `init cache 4 2 16 lru 8 4 32 lru`

- **`cache read <address>`** – Read from cache using physical address  
  _Example:_ `cache read 1024`

- **`cache write <address> <value>`** – Write to cache (write-through)  
  _Example:_ `cache write 1024 42`

- **`cache stats`** – Show cache hit/miss statistics  
- **`cache dump`** – Display cache contents  
- **`cache flush`** – Invalidate all cache lines

---

#### 🧾 Virtual Memory
- **`init vm <vp> <pf> <ps> <policy>`** – Initialize virtual memory system  
  _Example:_ `init vm 16 4 256 lru`

- **`vm read <virtual_address>`** – Read from virtual address  
  _Example:_ `vm read 1024`

- **`vm write <virtual_address> <value>`** – Write to virtual address  
  _Example:_ `vm write 1024 42`

- **`vm translate <virtual_address>`** – Translate virtual to physical address  
  _Example:_ `vm translate 1024`

- **`vm stats`** – Show virtual memory statistics  
- **`vm dump`** – Display page table

---

#### 📊 Visualization & Statistics
- **`dump memory`** – Display memory layout  
- **`stats`** – Show allocator statistics (strategy, fragmentation, utilization)

---

#### ⚙️ General
- **`help`** – Display help  
- **`exit`** – Exit the simulator



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
All 154 tests passing.


## Important Notes

### Subsystem Independence
**The allocator and virtual memory subsystems are independent and do not share state.** They operate on the same physical memory but in separate modes:
- When using the allocator (`malloc`/`free`), memory is managed through allocation strategies
- When using virtual memory (`vm read`/`vm write`), memory is accessed through page tables
- These two modes cannot be used simultaneously - choose one approach per simulation session

### Allocator Behavior
- Switching allocators (e.g., `set allocator buddy`) **invalidates all previous allocations**
- Block IDs are reset when changing allocation strategies
- Always reinitialize or clear allocations when switching allocators

## Performance Characteristics

### Time Complexity
- **Standard Allocator**: O(n) allocation/deallocation
- **Buddy Allocator**: O(log n) allocation/deallocation
- **Cache Lookup**: O(associativity) per cache level
- **Virtual Memory Translation**: O(1) page table lookup
- **Page Replacement**: O(1) FIFO, O(n) LRU

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