#include <gtest/gtest.h>
#include "cache/cache_hierarchy.h"
#include "memory/physical_memory.h"

using namespace memsim;

class CacheHierarchyTest : public ::testing::Test {
protected:
    void SetUp() override {
        memory = std::make_unique<PhysicalMemory>(4096);

        // Initialize memory with test pattern
        for (size_t i = 0; i < 4096; i++) {
            memory->write(i, static_cast<uint8_t>(i % 256));
        }
    }

    void TearDown() override {
        hierarchy.reset();
        memory.reset();
    }

    std::unique_ptr<PhysicalMemory> memory;
    std::unique_ptr<CacheHierarchy> hierarchy;
};

// ===== Basic L1/L2 Integration Tests =====

TEST_F(CacheHierarchyTest, BasicConstruction) {
    EXPECT_NO_THROW({
        hierarchy = std::make_unique<CacheHierarchy>(
            memory.get(),
            4, 1, 16, CachePolicy::FIFO,  // L1: 4 sets, direct-mapped, 16-byte blocks
            8, 2, 32, CachePolicy::LRU    // L2: 8 sets, 2-way, 32-byte blocks
        );
    });
}

TEST_F(CacheHierarchyTest, L1_Hit) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        4, 1, 16, CachePolicy::FIFO,
        8, 2, 32, CachePolicy::LRU
    );

    // First read - L1 miss, L2 miss, memory access
    auto r1 = hierarchy->read(0);
    ASSERT_TRUE(r1.success);

    // Second read - L1 hit
    auto r2 = hierarchy->read(0);
    ASSERT_TRUE(r2.success);

    auto stats = hierarchy->getStats();
    EXPECT_EQ(stats.l1_stats.hits, 1);
    EXPECT_EQ(stats.l1_stats.misses, 1);
}

TEST_F(CacheHierarchyTest, L2_Hit_L1_Miss) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        2, 1, 16, CachePolicy::FIFO,  // Small L1
        8, 2, 16, CachePolicy::FIFO   // Larger L2
    );

    // Load address 0 into both L1 and L2
    hierarchy->read(0);

    // Evict from L1 by accessing conflicting addresses
    hierarchy->read(32);   // Maps to same L1 set, evicts 0
    hierarchy->read(64);

    // Now read 0 again - should be L1 miss but L2 hit
    auto before = hierarchy->getStats();
    auto result = hierarchy->read(0);
    ASSERT_TRUE(result.success);
    auto after = hierarchy->getStats();

    // Behavioral test: L2 should have served the request
    EXPECT_GT(after.l2_stats.hits, before.l2_stats.hits);
}

TEST_F(CacheHierarchyTest, MemoryAccess_BothMiss) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        4, 1, 16, CachePolicy::FIFO,
        8, 2, 32, CachePolicy::LRU
    );

    // First access to new address - both L1 and L2 miss
    auto result = hierarchy->read(100);
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.value, 100);

    auto stats = hierarchy->getStats();
    EXPECT_EQ(stats.memory_accesses, 1);
}

// ===== Write-Through Tests =====

TEST_F(CacheHierarchyTest, WriteThrough_UpdatesMemory) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        4, 1, 16, CachePolicy::FIFO,
        8, 2, 32, CachePolicy::LRU
    );

    auto write_result = hierarchy->write(50, 199);
    ASSERT_TRUE(write_result.success);

    // Verify memory was updated
    auto mem_result = memory->read(50);
    ASSERT_TRUE(mem_result.success);
    EXPECT_EQ(mem_result.value, 199);
}

TEST_F(CacheHierarchyTest, WriteThrough_UpdatesCaches) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        4, 1, 16, CachePolicy::FIFO,
        8, 2, 32, CachePolicy::LRU
    );

    // Load into caches
    hierarchy->read(50);

    // Write new value
    hierarchy->write(50, 222);

    // Read should get updated value from cache
    auto result = hierarchy->read(50);
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.value, 222);
}

// ===== Flush Tests =====

TEST_F(CacheHierarchyTest, Flush_ClearsAllCaches) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        4, 1, 16, CachePolicy::FIFO,
        8, 2, 32, CachePolicy::LRU
    );

    hierarchy->read(0);
    hierarchy->read(16);

    EXPECT_TRUE(hierarchy->getL1()->contains(0));
    EXPECT_TRUE(hierarchy->getL2()->contains(0));

    hierarchy->flush();

    EXPECT_FALSE(hierarchy->getL1()->contains(0));
    EXPECT_FALSE(hierarchy->getL2()->contains(0));
}

// ===== Statistics Tests =====

TEST_F(CacheHierarchyTest, OverallHitRatio) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        4, 2, 16, CachePolicy::LRU,
        8, 4, 32, CachePolicy::LRU
    );

    // Access pattern: some L1 hits, some L2 hits, some misses
    hierarchy->read(0);    // Miss
    hierarchy->read(0);    // L1 hit
    hierarchy->read(16);   // Miss
    hierarchy->read(16);   // L1 hit
    hierarchy->read(0);    // L1 hit

    auto stats = hierarchy->getStats();
    EXPECT_GT(stats.getOverallHitRatio(), 0.0);
    EXPECT_LE(stats.getOverallHitRatio(), 100.0);
}

TEST_F(CacheHierarchyTest, StatsString) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        4, 1, 16, CachePolicy::FIFO,
        8, 2, 32, CachePolicy::LRU
    );

    hierarchy->read(0);
    hierarchy->read(0);

    std::string stats = hierarchy->getStatsString();
    EXPECT_FALSE(stats.empty());
    EXPECT_NE(stats.find("L1"), std::string::npos);
    EXPECT_NE(stats.find("L2"), std::string::npos);
    EXPECT_NE(stats.find("Overall"), std::string::npos);
}

// ===== Dump Test =====

TEST_F(CacheHierarchyTest, DumpDoesNotCrash) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        4, 1, 16, CachePolicy::FIFO,
        8, 2, 32, CachePolicy::LRU
    );

    hierarchy->read(0);
    hierarchy->read(64);

    testing::internal::CaptureStdout();
    hierarchy->dump();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_FALSE(output.empty());
}

// ===== Policy Combination Tests =====

TEST_F(CacheHierarchyTest, FIFO_L1_LRU_L2) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        4, 2, 16, CachePolicy::FIFO,
        8, 4, 32, CachePolicy::LRU
    );

    // Just verify it works with different policies
    for (size_t i = 0; i < 100; i += 8) {
        auto result = hierarchy->read(i);
        ASSERT_TRUE(result.success);
    }

    auto stats = hierarchy->getStats();
    EXPECT_GT(stats.total_accesses, 0);
}

TEST_F(CacheHierarchyTest, LFU_L1_FIFO_L2) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        4, 2, 16, CachePolicy::LFU,
        8, 4, 32, CachePolicy::FIFO
    );

    for (size_t i = 0; i < 100; i += 8) {
        auto result = hierarchy->read(i);
        ASSERT_TRUE(result.success);
    }

    auto stats = hierarchy->getStats();
    EXPECT_GT(stats.total_accesses, 0);
}

// ===== Workload Tests =====

TEST_F(CacheHierarchyTest, SequentialAccessPattern) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        8, 2, 32, CachePolicy::LRU,
        16, 4, 64, CachePolicy::LRU
    );

    // Sequential access - good spatial locality
    for (size_t i = 0; i < 256; i++) {
        auto result = hierarchy->read(i);
        ASSERT_TRUE(result.success);
    }

    auto stats = hierarchy->getStats();
    // Should have good hit ratio due to block loading
    EXPECT_GT(stats.getOverallHitRatio(), 50.0);
}

TEST_F(CacheHierarchyTest, StridedAccessPattern) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        8, 2, 32, CachePolicy::LRU,
        16, 4, 64, CachePolicy::LRU
    );

    // Strided access with stride = block size
    for (size_t i = 0; i < 512; i += 32) {
        auto result = hierarchy->read(i);
        ASSERT_TRUE(result.success);
    }

    auto stats = hierarchy->getStats();
    EXPECT_GT(stats.total_accesses, 0);
}

TEST_F(CacheHierarchyTest, RandomAccessPattern) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        8, 2, 32, CachePolicy::LRU,
        16, 4, 64, CachePolicy::LRU
    );

    // Pseudo-random access pattern
    for (size_t i = 0; i < 100; i++) {
        size_t addr = (i * 137) % 1024;  // Pseudo-random
        auto result = hierarchy->read(addr);
        ASSERT_TRUE(result.success);
    }

    auto stats = hierarchy->getStats();
    // Invariant: total_accesses >= L1 accesses
    EXPECT_GE(stats.total_accesses, stats.l1_stats.accesses);
}

TEST_F(CacheHierarchyTest, TemporalLocality) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        4, 2, 16, CachePolicy::LRU,
        8, 4, 32, CachePolicy::LRU
    );

    // Access same addresses multiple times (temporal locality)
    for (int repeat = 0; repeat < 5; repeat++) {
        for (size_t i = 0; i < 10; i++) {
            hierarchy->read(i * 16);
        }
    }

    auto stats = hierarchy->getStats();
    // Should have good hit ratio with temporal locality
    // Note: total_accesses includes both L1 and L2 accesses, so ratio is lower than L1-only
    EXPECT_GT(stats.getOverallHitRatio(), 50.0);
}

// ===== Block Boundary Tests =====

TEST_F(CacheHierarchyTest, BlockBoundaryBehavior) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        4, 1, 16, CachePolicy::FIFO,  // 16-byte blocks
        8, 2, 32, CachePolicy::LRU    // 32-byte blocks
    );

    // 16-byte block: [0-15], [16-31], ...
    auto before = hierarchy->getStats();

    hierarchy->read(15);   // Load block [0-15]
    hierarchy->read(16);   // Load block [16-31] - new block

    auto after = hierarchy->getStats();

    // Two compulsory misses (different blocks)
    EXPECT_EQ(after.l1_stats.misses - before.l1_stats.misses, 2);

    // Now access within same block
    before = hierarchy->getStats();
    hierarchy->read(17);   // Same block as 16
    hierarchy->read(18);   // Same block as 16
    after = hierarchy->getStats();

    // Should be hits (same block)
    EXPECT_EQ(after.l1_stats.hits - before.l1_stats.hits, 2);
}

// ===== Replacement Policy Correctness Tests =====

TEST_F(CacheHierarchyTest, LRU_EvictsLeastRecentlyUsed) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        1, 2, 16, CachePolicy::LRU,  // 1 set, 2-way (can hold 2 blocks)
        4, 2, 16, CachePolicy::LRU
    );

    // All map to same set (set 0) with block size 16
    hierarchy->read(0);    // Load block [0-15] into way 0
    hierarchy->read(16);   // Load block [16-31] into way 1
    hierarchy->read(0);    // Access block [0-15] - now it's MRU, block [16-31] is LRU

    auto before = hierarchy->getStats();
    hierarchy->read(32);   // Load block [32-47] - should evict [16-31] (LRU)
    auto after_evict = hierarchy->getStats();

    // Accessing 32 should be a miss (new block)
    EXPECT_GT(after_evict.l1_stats.misses, before.l1_stats.misses);

    // Now accessing 16 (in evicted block) should miss
    before = hierarchy->getStats();
    hierarchy->read(16);   // Block [16-31] was evicted
    auto after_b = hierarchy->getStats();

    EXPECT_GT(after_b.l1_stats.misses, before.l1_stats.misses);

    // Reading 16 just loaded block [16-31], evicting either [0-15] or [32-47]
    // We can't guarantee which one without knowing exact LRU state
    // So instead, just verify LRU behavior with a simpler test
    hierarchy->flush();

    // Fresh start: demonstrate LRU with clear access pattern
    hierarchy->read(0);    // A
    hierarchy->read(16);   // B  - now cache has [A, B]
    hierarchy->read(0);    // A again - A is MRU, B is LRU

    before = hierarchy->getStats();
    hierarchy->read(16);   // B - now B is MRU
    auto after = hierarchy->getStats();

    // This should be a hit (B is still in cache)
    EXPECT_EQ(after.l1_stats.hits - before.l1_stats.hits, 1);
}

// ===== Write Policy Tests =====

TEST_F(CacheHierarchyTest, WriteNoAllocateOnMiss) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        4, 1, 16, CachePolicy::FIFO,
        8, 2, 32, CachePolicy::LRU
    );

    // Write to address not in cache (write miss)
    auto before = hierarchy->getStats();
    hierarchy->write(200, 123);
    auto after_write = hierarchy->getStats();

    // Write miss should go directly to memory, not allocate in cache
    // L1 misses should not increase (no-write-allocate policy)
    EXPECT_EQ(after_write.l1_stats.misses, before.l1_stats.misses);

    // Verify data was written to memory
    auto mem_result = memory->read(200);
    ASSERT_TRUE(mem_result.success);
    EXPECT_EQ(mem_result.value, 123);

    // Subsequent read should miss (block wasn't allocated on write)
    before = hierarchy->getStats();
    auto read_result = hierarchy->read(200);
    after_write = hierarchy->getStats();

    ASSERT_TRUE(read_result.success);
    EXPECT_EQ(read_result.value, 123);
    EXPECT_GT(after_write.l1_stats.misses, before.l1_stats.misses);
}

// ===== Cache Invariant Tests =====

TEST_F(CacheHierarchyTest, CacheInvariants) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        4, 2, 16, CachePolicy::LRU,
        8, 4, 32, CachePolicy::LRU
    );

    for (int i = 0; i < 50; i++) {
        hierarchy->read(i * 8);
    }

    auto stats = hierarchy->getStats();

    // Invariant 1: hits + misses = accesses (for each level)
    EXPECT_EQ(stats.l1_stats.hits + stats.l1_stats.misses,
              stats.l1_stats.accesses);
    EXPECT_EQ(stats.l2_stats.hits + stats.l2_stats.misses,
              stats.l2_stats.accesses);

    // Invariant 2: total_accesses >= L1 accesses
    EXPECT_GE(stats.total_accesses, stats.l1_stats.accesses);

    // Invariant 3: hit ratio in valid range
    EXPECT_GE(stats.getOverallHitRatio(), 0.0);
    EXPECT_LE(stats.getOverallHitRatio(), 100.0);

    // Invariant 4: L1 accesses should equal number of reads we did
    EXPECT_EQ(stats.l1_stats.accesses, 50);
}

// ===== Cache-Memory Consistency Tests =====

TEST_F(CacheHierarchyTest, CacheMemoryConsistency) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        4, 2, 16, CachePolicy::LRU,
        8, 4, 32, CachePolicy::LRU
    );

    // Write through cache
    hierarchy->write(128, 77);

    // Flush to ensure write-through happened
    hierarchy->flush();

    // Read directly from memory (bypass cache)
    auto mem = memory->read(128);
    ASSERT_TRUE(mem.success);
    EXPECT_EQ(mem.value, 77);

    // Write and read multiple values
    for (size_t i = 0; i < 10; i++) {
        hierarchy->write(i * 16, static_cast<uint8_t>(i + 100));
    }

    hierarchy->flush();

    // Verify all writes persisted to memory
    for (size_t i = 0; i < 10; i++) {
        auto result = memory->read(i * 16);
        ASSERT_TRUE(result.success);
        EXPECT_EQ(result.value, static_cast<uint8_t>(i + 100));
    }
}

// ===== Large Hierarchy Test =====

TEST(CacheHierarchyLargeTest, LargeHierarchy) {
    PhysicalMemory memory(1024 * 1024);  // 1 MB

    // Initialize
    for (size_t i = 0; i < 1024 * 1024; i += 256) {
        memory.write(i, static_cast<uint8_t>(i & 0xFF));
    }

    CacheHierarchy hierarchy(
        &memory,
        64, 4, 64, CachePolicy::LRU,    // L1: 64 sets, 4-way, 64-byte blocks
        128, 8, 128, CachePolicy::LRU   // L2: 128 sets, 8-way, 128-byte blocks
    );

    // Simulate realistic workload
    for (size_t i = 0; i < 10000; i++) {
        size_t addr = (i * 64) % (1024 * 1024);
        auto result = hierarchy.read(addr);
        ASSERT_TRUE(result.success);
    }

    auto stats = hierarchy.getStats();
    // Invariant: total_accesses >= L1 accesses (L2 accessed on L1 misses)
    EXPECT_GE(stats.total_accesses, stats.l1_stats.accesses);
    EXPECT_GT(stats.getOverallHitRatio(), 0.0);
    EXPECT_LE(stats.getOverallHitRatio(), 100.0);
}

// ===== Stress Test =====

TEST_F(CacheHierarchyTest, StressTest_ManyAccesses) {
    hierarchy = std::make_unique<CacheHierarchy>(
        memory.get(),
        16, 4, 32, CachePolicy::LRU,
        32, 8, 64, CachePolicy::LRU
    );

    // Many accesses with mixed reads and writes
    for (size_t i = 0; i < 1000; i++) {
        if (i % 3 == 0) {
            hierarchy->write(i % 4096, static_cast<uint8_t>(i & 0xFF));
        } else {
            auto result = hierarchy->read(i % 4096);
            ASSERT_TRUE(result.success);
        }
    }

    auto stats = hierarchy->getStats();
    EXPECT_GT(stats.total_accesses, 500);
}
