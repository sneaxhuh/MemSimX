#include <gtest/gtest.h>
#include "memory/physical_memory.h"
#include "allocator/standard_allocator.h"
#include "allocator/buddy_allocator.h"
#include "cache/cache_hierarchy.h"
#include "virtual_memory/virtual_memory.h"

using namespace memsim;

// ===== Full System Integration Test =====

class FullSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 16 KB physical memory
        memory = std::make_unique<PhysicalMemory>(16 * 1024);
    }

    void TearDown() override {
        vm.reset();
        cache.reset();
        allocator.reset();
        memory.reset();
    }

    std::unique_ptr<PhysicalMemory> memory;
    std::unique_ptr<IAllocator> allocator;
    std::unique_ptr<CacheHierarchy> cache;
    std::unique_ptr<VirtualMemory> vm;
};

// ===== Allocator + Cache Integration =====

TEST_F(FullSystemTest, AllocatorWithCache) {
    // Create allocator
    allocator = std::make_unique<StandardAllocator>(
        memory.get(), AllocatorType::FIRST_FIT
    );

    // Create cache hierarchy
    cache = std::make_unique<CacheHierarchy>(
        memory.get(),
        8, 2, 32, CachePolicy::LRU,   // L1
        16, 4, 64, CachePolicy::LRU   // L2
    );

    // Allocate some blocks
    auto b1 = allocator->allocate(128);
    auto b2 = allocator->allocate(256);
    ASSERT_TRUE(b1.success);
    ASSERT_TRUE(b2.success);

    // Access allocated memory through cache
    // This should work because cache accesses physical memory
    auto read_result = cache->read(0);
    ASSERT_TRUE(read_result.success);

    // Write through cache
    auto write_result = cache->write(100, 42);
    ASSERT_TRUE(write_result.success);

    // Verify write
    auto verify = cache->read(100);
    ASSERT_TRUE(verify.success);
    EXPECT_EQ(verify.value, 42);
}

TEST_F(FullSystemTest, BuddyAllocatorWithCache) {
    // Buddy allocator requires power-of-2 memory
    memory = std::make_unique<PhysicalMemory>(8192);

    allocator = std::make_unique<BuddyAllocator>(memory.get(), 32);
    cache = std::make_unique<CacheHierarchy>(
        memory.get(),
        4, 2, 32, CachePolicy::FIFO,
        8, 4, 64, CachePolicy::FIFO
    );

    // Allocate with buddy system
    auto b1 = allocator->allocate(50);   // Rounded to 64
    auto b2 = allocator->allocate(100);  // Rounded to 128
    ASSERT_TRUE(b1.success);
    ASSERT_TRUE(b2.success);

    // Access through cache
    cache->write(64, 99);
    auto result = cache->read(64);
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.value, 99);
}

// ===== Virtual Memory + Cache Integration =====

TEST_F(FullSystemTest, VirtualMemoryWithCache) {
    // Create virtual memory system
    vm = std::make_unique<VirtualMemory>(
        memory.get(),
        64,    // 64 virtual pages
        32,    // 32 physical frames
        512,   // 512-byte pages
        PageReplacementPolicy::LRU
    );

    // Create cache
    cache = std::make_unique<CacheHierarchy>(
        memory.get(),
        16, 2, 32, CachePolicy::LRU,
        32, 4, 64, CachePolicy::LRU
    );

    // Write through virtual memory
    auto write_vm = vm->write(0, 77);
    ASSERT_TRUE(write_vm.success);

    // Translate virtual address to physical
    auto translate = vm->translate(0);
    ASSERT_TRUE(translate.success);

    // Access same physical location through cache
    auto read_cache = cache->read(translate.value);
    ASSERT_TRUE(read_cache.success);
    EXPECT_EQ(read_cache.value, 77);
}

TEST_F(FullSystemTest, VirtualMemoryPageFaults) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(),
        32,   // 32 virtual pages
        8,    // Only 8 physical frames (will cause page faults)
        512,
        PageReplacementPolicy::FIFO
    );

    // Access more pages than physical frames
    for (size_t i = 0; i < 16; i++) {
        auto result = vm->write(i * 512, static_cast<uint8_t>(i));
        ASSERT_TRUE(result.success);
    }

    auto stats = vm->getStats();
    EXPECT_EQ(stats.page_faults, 16);  // First access to each page

    // Access first 8 pages again - should cause more faults
    for (size_t i = 0; i < 8; i++) {
        vm->read(i * 512);
    }

    auto stats2 = vm->getStats();
    EXPECT_GT(stats2.page_faults, 16);  // Some were evicted
}

// ===== Full Pipeline: Allocator + Cache + Virtual Memory =====

TEST_F(FullSystemTest, FullPipeline_StandardAllocator) {
    // Setup full pipeline
    allocator = std::make_unique<StandardAllocator>(
        memory.get(), AllocatorType::BEST_FIT
    );

    cache = std::make_unique<CacheHierarchy>(
        memory.get(),
        8, 2, 32, CachePolicy::LRU,
        16, 4, 64, CachePolicy::LRU
    );

    vm = std::make_unique<VirtualMemory>(
        memory.get(),
        32, 16, 512, PageReplacementPolicy::LRU
    );

    // Allocate memory blocks
    auto b1 = allocator->allocate(256);
    auto b2 = allocator->allocate(512);
    ASSERT_TRUE(b1.success);
    ASSERT_TRUE(b2.success);

    // Write through virtual memory
    vm->write(0, 100);
    vm->write(512, 200);

    // Read back through cache
    auto translate1 = vm->translate(0);
    auto translate2 = vm->translate(512);
    ASSERT_TRUE(translate1.success);
    ASSERT_TRUE(translate2.success);

    auto cache_read1 = cache->read(translate1.value);
    auto cache_read2 = cache->read(translate2.value);
    ASSERT_TRUE(cache_read1.success);
    ASSERT_TRUE(cache_read2.success);
    EXPECT_EQ(cache_read1.value, 100);
    EXPECT_EQ(cache_read2.value, 200);
}

TEST_F(FullSystemTest, FullPipeline_BuddyAllocator) {
    memory = std::make_unique<PhysicalMemory>(8192);

    allocator = std::make_unique<BuddyAllocator>(memory.get(), 32);

    cache = std::make_unique<CacheHierarchy>(
        memory.get(),
        4, 2, 32, CachePolicy::FIFO,
        8, 4, 32, CachePolicy::FIFO
    );

    vm = std::make_unique<VirtualMemory>(
        memory.get(),
        32, 16, 256, PageReplacementPolicy::CLOCK
    );

    // Allocate with buddy system
    auto b1 = allocator->allocate(60);   // -> 64
    auto b2 = allocator->allocate(120);  // -> 128
    ASSERT_TRUE(b1.success);
    ASSERT_TRUE(b2.success);

    // Access through virtual memory and cache
    for (size_t i = 0; i < 10; i++) {
        auto virt_addr = i * 256;
        vm->write(virt_addr, static_cast<uint8_t>(i * 10));
    }

    // Verify reads
    for (size_t i = 0; i < 10; i++) {
        auto virt_addr = i * 256;
        auto result = vm->read(virt_addr);
        ASSERT_TRUE(result.success);
        EXPECT_EQ(result.value, i * 10);
    }
}

// ===== Stress Test: Full System Under Load =====

TEST_F(FullSystemTest, StressTest_FullSystem) {
    allocator = std::make_unique<StandardAllocator>(
        memory.get(), AllocatorType::FIRST_FIT
    );

    cache = std::make_unique<CacheHierarchy>(
        memory.get(),
        16, 4, 64, CachePolicy::LRU,
        32, 8, 128, CachePolicy::LRU
    );

    vm = std::make_unique<VirtualMemory>(
        memory.get(),
        64, 32, 512, PageReplacementPolicy::LRU
    );

    // Allocate multiple blocks
    std::vector<BlockId> blocks;
    for (size_t i = 0; i < 10; i++) {
        auto result = allocator->allocate(128 + i * 16);
        if (result.success) {
            blocks.push_back(result.value);
        }
    }

    // Heavy virtual memory access with page faults
    for (size_t i = 0; i < 50; i++) {
        vm->write(i * 512, static_cast<uint8_t>(i % 256));
    }

    // Cache access pattern
    for (size_t i = 0; i < 100; i++) {
        cache->read(i * 10);
    }

    // Verify statistics
    auto vm_stats = vm->getStats();
    auto cache_stats = cache->getStats();

    EXPECT_GT(vm_stats.total_accesses, 0);
    EXPECT_GT(cache_stats.total_accesses, 0);

    // Deallocate some blocks
    for (size_t i = 0; i < blocks.size() / 2; i++) {
        allocator->deallocate(blocks[i]);
    }
}

// ===== Workload: Sequential Access Pattern =====

TEST_F(FullSystemTest, Workload_SequentialAccess) {
    cache = std::make_unique<CacheHierarchy>(
        memory.get(),
        8, 2, 64, CachePolicy::LRU,
        16, 4, 128, CachePolicy::LRU
    );

    vm = std::make_unique<VirtualMemory>(
        memory.get(),
        32, 16, 512, PageReplacementPolicy::LRU
    );

    // Sequential write pattern
    for (size_t i = 0; i < 200; i++) {
        vm->write(i, static_cast<uint8_t>(i % 256));
    }

    // Sequential read pattern
    for (size_t i = 0; i < 200; i++) {
        auto result = vm->read(i);
        ASSERT_TRUE(result.success);
        EXPECT_EQ(result.value, i % 256);
    }

    auto stats = vm->getStats();
    // Good locality should result in high hit ratio after first pass
    EXPECT_GT(stats.page_hits, stats.page_faults);
}

// ===== Workload: Random Access Pattern =====

TEST_F(FullSystemTest, Workload_RandomAccess) {
    cache = std::make_unique<CacheHierarchy>(
        memory.get(),
        16, 4, 64, CachePolicy::LFU,
        32, 8, 128, CachePolicy::LFU
    );

    vm = std::make_unique<VirtualMemory>(
        memory.get(),
        64, 16, 256, PageReplacementPolicy::CLOCK
    );

    // Pseudo-random access
    for (size_t i = 0; i < 100; i++) {
        size_t virt_addr = (i * 137) % (64 * 256);
        vm->write(virt_addr, static_cast<uint8_t>(i));
    }

    auto stats = vm->getStats();
    EXPECT_GT(stats.page_faults, 0);
}

// ===== Workload: Mixed Read/Write with Temporal Locality =====

TEST_F(FullSystemTest, Workload_TemporalLocality) {
    cache = std::make_unique<CacheHierarchy>(
        memory.get(),
        8, 2, 64, CachePolicy::LRU,
        16, 4, 128, CachePolicy::LRU
    );

    vm = std::make_unique<VirtualMemory>(
        memory.get(),
        32, 16, 512, PageReplacementPolicy::LRU
    );

    // Repeatedly access same pages (good temporal locality)
    for (int round = 0; round < 10; round++) {
        for (size_t i = 0; i < 5; i++) {
            vm->write(i * 512, static_cast<uint8_t>(round));
        }
    }

    auto stats = vm->getStats();
    // Should have high hit ratio due to temporal locality
    double hit_ratio = stats.getPageHitRate();
    EXPECT_GT(hit_ratio, 50.0);  // At least 50% hits
}

// ===== Component Interaction Tests =====

TEST_F(FullSystemTest, AllocatorFragmentation_ImpactsCache) {
    allocator = std::make_unique<StandardAllocator>(
        memory.get(), AllocatorType::WORST_FIT
    );

    cache = std::make_unique<CacheHierarchy>(
        memory.get(),
        8, 2, 32, CachePolicy::FIFO,
        16, 4, 64, CachePolicy::FIFO
    );

    // Create fragmentation with allocations
    auto b1 = allocator->allocate(100);
    auto b2 = allocator->allocate(200);
    auto b3 = allocator->allocate(150);

    ASSERT_TRUE(b1.success);
    ASSERT_TRUE(b2.success);
    ASSERT_TRUE(b3.success);

    // Free middle block
    allocator->deallocate(b2.value);

    // Access through cache
    for (size_t i = 0; i < 450; i += 10) {
        cache->read(i);
    }

    auto cache_stats = cache->getStats();
    EXPECT_GT(cache_stats.total_accesses, 0);
}

TEST_F(FullSystemTest, VirtualMemory_PageFaults_AffectCache) {
    cache = std::make_unique<CacheHierarchy>(
        memory.get(),
        4, 2, 32, CachePolicy::LRU,
        8, 4, 64, CachePolicy::LRU
    );

    // Small physical memory to force page faults
    vm = std::make_unique<VirtualMemory>(
        memory.get(),
        32, 4, 512, PageReplacementPolicy::FIFO
    );

    // Access many pages (more than frames)
    for (size_t i = 0; i < 20; i++) {
        vm->write(i * 512, static_cast<uint8_t>(i));
    }

    auto vm_stats = vm->getStats();
    // Should have many page faults
    EXPECT_GT(vm_stats.page_faults, 10);
}

// ===== Performance Comparison Test =====

TEST_F(FullSystemTest, Performance_AllocationStrategies) {
    // Test First Fit
    auto ff_alloc = std::make_unique<StandardAllocator>(
        memory.get(), AllocatorType::FIRST_FIT
    );

    for (int i = 0; i < 20; i++) {
        ff_alloc->allocate(100 + i * 10);
    }

    double ff_util = ff_alloc->getUtilization();

    // Reset memory
    memory->clear();
    memory->updateUsedSize(0);

    // Test Best Fit
    auto bf_alloc = std::make_unique<StandardAllocator>(
        memory.get(), AllocatorType::BEST_FIT
    );

    for (int i = 0; i < 20; i++) {
        bf_alloc->allocate(100 + i * 10);
    }

    double bf_util = bf_alloc->getUtilization();

    // Both should successfully allocate
    EXPECT_GT(ff_util, 0.0);
    EXPECT_GT(bf_util, 0.0);
}
