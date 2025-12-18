#include <gtest/gtest.h>
#include "cache/cache_level.h"
#include "memory/physical_memory.h"

using namespace memsim;

// ===== Test Fixture for Direct-Mapped Cache =====

class CacheLevelDirectMappedTest : public ::testing::Test {
protected:
    void SetUp() override {
        memory = std::make_unique<PhysicalMemory>(1024);

        // Write some test data to memory
        for (size_t i = 0; i < 1024; i++) {
            memory->write(i, static_cast<uint8_t>(i % 256));
        }
    }

    void TearDown() override {
        cache.reset();
        memory.reset();
    }

    std::unique_ptr<PhysicalMemory> memory;
    std::unique_ptr<CacheLevel> cache;
};

// ===== Test Fixture for Set-Associative Cache =====

class CacheLevelSetAssociativeTest : public ::testing::Test {
protected:
    void SetUp() override {
        memory = std::make_unique<PhysicalMemory>(1024);

        // Write test data
        for (size_t i = 0; i < 1024; i++) {
            memory->write(i, static_cast<uint8_t>(i % 256));
        }
    }

    void TearDown() override {
        cache.reset();
        memory.reset();
    }

    std::unique_ptr<PhysicalMemory> memory;
    std::unique_ptr<CacheLevel> cache;
};

// ===== Constructor Tests =====

TEST_F(CacheLevelDirectMappedTest, ValidConstruction) {
    EXPECT_NO_THROW({
        cache = std::make_unique<CacheLevel>(
            1,      // L1
            4,      // 4 sets
            1,      // Direct-mapped (1-way)
            16,     // 16-byte blocks
            CachePolicy::FIFO,
            memory.get()
        );
    });
}

TEST_F(CacheLevelDirectMappedTest, InvalidNumSets_NotPowerOfTwo) {
    EXPECT_THROW({
        cache = std::make_unique<CacheLevel>(
            1, 5, 1, 16, CachePolicy::FIFO, memory.get()
        );
    }, std::invalid_argument);
}

TEST_F(CacheLevelDirectMappedTest, InvalidBlockSize_NotPowerOfTwo) {
    EXPECT_THROW({
        cache = std::make_unique<CacheLevel>(
            1, 4, 1, 15, CachePolicy::FIFO, memory.get()
        );
    }, std::invalid_argument);
}

TEST_F(CacheLevelDirectMappedTest, InvalidAssociativity_Zero) {
    EXPECT_THROW({
        cache = std::make_unique<CacheLevel>(
            1, 4, 0, 16, CachePolicy::FIFO, memory.get()
        );
    }, std::invalid_argument);
}

// ===== Basic Read Tests =====

TEST_F(CacheLevelDirectMappedTest, BasicRead_ColdMiss) {
    cache = std::make_unique<CacheLevel>(
        1, 4, 1, 16, CachePolicy::FIFO, memory.get()
    );

    auto result = cache->read(0);
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.value, 0);

    // Should be a miss
    auto stats = cache->getStats();
    EXPECT_EQ(stats.hits, 0);
    EXPECT_EQ(stats.misses, 1);
}

TEST_F(CacheLevelDirectMappedTest, BasicRead_CacheHit) {
    cache = std::make_unique<CacheLevel>(
        1, 4, 1, 16, CachePolicy::FIFO, memory.get()
    );

    // First read (miss)
    cache->read(0);

    // Second read (hit - same block)
    auto result = cache->read(0);
    ASSERT_TRUE(result.success);

    auto stats = cache->getStats();
    EXPECT_EQ(stats.hits, 1);
    EXPECT_EQ(stats.misses, 1);
}

TEST_F(CacheLevelDirectMappedTest, BlockLoading) {
    cache = std::make_unique<CacheLevel>(
        1, 4, 1, 16, CachePolicy::FIFO, memory.get()
    );

    // Read address 0 - loads block [0, 15]
    cache->read(0);

    // Read address 8 - should hit (same block)
    auto result = cache->read(8);
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.value, 8);

    auto stats = cache->getStats();
    EXPECT_EQ(stats.hits, 1);
    EXPECT_EQ(stats.misses, 1);
}

// ===== Write Tests =====

TEST_F(CacheLevelDirectMappedTest, BasicWrite) {
    cache = std::make_unique<CacheLevel>(
        1, 4, 1, 16, CachePolicy::FIFO, memory.get()
    );

    auto write_result = cache->write(0, 99);
    ASSERT_TRUE(write_result.success);

    // Verify write-through to memory
    auto mem_result = memory->read(0);
    ASSERT_TRUE(mem_result.success);
    EXPECT_EQ(mem_result.value, 99);

    // Verify cache was updated
    auto cache_result = cache->read(0);
    ASSERT_TRUE(cache_result.success);
    EXPECT_EQ(cache_result.value, 99);
}

TEST_F(CacheLevelDirectMappedTest, WriteThenRead) {
    cache = std::make_unique<CacheLevel>(
        1, 4, 1, 16, CachePolicy::FIFO, memory.get()
    );

    cache->write(10, 123);
    auto result = cache->read(10);

    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.value, 123);
}

// ===== FIFO Replacement Policy Tests =====

TEST_F(CacheLevelSetAssociativeTest, FIFO_Replacement) {
    // 2-way set-associative, 4 sets, 16-byte blocks
    cache = std::make_unique<CacheLevel>(
        1, 4, 2, 16, CachePolicy::FIFO, memory.get()
    );

    // Access addresses that map to same set
    // With 4 sets and 16-byte blocks: set_index = (addr >> 4) & 3
    // Addresses 0, 64, 128 all map to set 0

    cache->read(0);    // Miss - load into way 0
    cache->read(64);   // Miss - load into way 1
    cache->read(128);  // Miss - evict way 0 (FIFO)

    // Now address 0 should be evicted
    EXPECT_FALSE(cache->contains(0));
    EXPECT_TRUE(cache->contains(64));
    EXPECT_TRUE(cache->contains(128));
}

TEST_F(CacheLevelSetAssociativeTest, FIFO_OrderPreservation) {
    cache = std::make_unique<CacheLevel>(
        1, 4, 2, 16, CachePolicy::FIFO, memory.get()
    );

    cache->read(0);
    cache->read(64);
    cache->read(0);    // Hit - doesn't change FIFO order
    cache->read(128);  // Should still evict original first entry (0)

    EXPECT_FALSE(cache->contains(0));
}

// ===== LRU Replacement Policy Tests =====

TEST_F(CacheLevelSetAssociativeTest, LRU_Replacement) {
    cache = std::make_unique<CacheLevel>(
        1, 4, 2, 16, CachePolicy::LRU, memory.get()
    );

    cache->read(0);    // Load into way 0
    cache->read(64);   // Load into way 1
    cache->read(0);    // Hit - update LRU timestamp for way 0
    cache->read(128);  // Evict way 1 (64 is least recently used)

    EXPECT_TRUE(cache->contains(0));
    EXPECT_FALSE(cache->contains(64));
    EXPECT_TRUE(cache->contains(128));
}

TEST_F(CacheLevelSetAssociativeTest, LRU_UpdateOnAccess) {
    cache = std::make_unique<CacheLevel>(
        1, 4, 2, 16, CachePolicy::LRU, memory.get()
    );

    cache->read(0);
    cache->read(64);

    // Access 0 multiple times - it should become most recently used
    cache->read(0);
    cache->read(0);

    cache->read(128);  // Should evict 64

    EXPECT_TRUE(cache->contains(0));
    EXPECT_FALSE(cache->contains(64));
}

// ===== LFU Replacement Policy Tests =====

TEST_F(CacheLevelSetAssociativeTest, LFU_Replacement) {
    cache = std::make_unique<CacheLevel>(
        1, 4, 2, 16, CachePolicy::LFU, memory.get()
    );

    cache->read(0);    // Count = 0 (new load)
    cache->read(64);   // Count = 0 (new load)
    cache->read(0);    // Count = 1
    cache->read(0);    // Count = 2
    cache->read(128);  // Evict 64 (least frequently used)

    EXPECT_TRUE(cache->contains(0));
    EXPECT_FALSE(cache->contains(64));
    EXPECT_TRUE(cache->contains(128));
}

TEST_F(CacheLevelSetAssociativeTest, LFU_AccessCounting) {
    cache = std::make_unique<CacheLevel>(
        1, 4, 2, 16, CachePolicy::LFU, memory.get()
    );

    cache->read(0);
    cache->read(64);

    // Access 64 more times
    for (int i = 0; i < 10; i++) {
        cache->read(64);
    }

    cache->read(128);  // Should evict 0 (less frequently used)

    EXPECT_FALSE(cache->contains(0));
    EXPECT_TRUE(cache->contains(64));
}

// ===== Flush Tests =====

TEST_F(CacheLevelDirectMappedTest, Flush) {
    cache = std::make_unique<CacheLevel>(
        1, 4, 1, 16, CachePolicy::FIFO, memory.get()
    );

    cache->read(0);
    cache->read(16);

    EXPECT_TRUE(cache->contains(0));
    EXPECT_TRUE(cache->contains(16));

    cache->flush();

    EXPECT_FALSE(cache->contains(0));
    EXPECT_FALSE(cache->contains(16));
}

// ===== Statistics Tests =====

TEST_F(CacheLevelDirectMappedTest, HitRatioCalculation) {
    cache = std::make_unique<CacheLevel>(
        1, 4, 1, 16, CachePolicy::FIFO, memory.get()
    );

    // 1 miss
    cache->read(0);

    // 4 hits (same block)
    cache->read(1);
    cache->read(2);
    cache->read(3);
    cache->read(4);

    auto stats = cache->getStats();
    EXPECT_EQ(stats.hits, 4);
    EXPECT_EQ(stats.misses, 1);
    EXPECT_EQ(stats.accesses, 5);
    EXPECT_DOUBLE_EQ(stats.getHitRatio(), 80.0);
}

TEST_F(CacheLevelDirectMappedTest, MissRatioCalculation) {
    cache = std::make_unique<CacheLevel>(
        1, 4, 1, 16, CachePolicy::FIFO, memory.get()
    );

    cache->read(0);
    cache->read(1);

    auto stats = cache->getStats();
    EXPECT_DOUBLE_EQ(stats.getMissRatio(), 50.0);
}

// ===== Contains Tests =====

TEST_F(CacheLevelDirectMappedTest, ContainsAfterRead) {
    cache = std::make_unique<CacheLevel>(
        1, 4, 1, 16, CachePolicy::FIFO, memory.get()
    );

    EXPECT_FALSE(cache->contains(0));
    cache->read(0);
    EXPECT_TRUE(cache->contains(0));
}

TEST_F(CacheLevelDirectMappedTest, ContainsBlockRange) {
    cache = std::make_unique<CacheLevel>(
        1, 4, 1, 16, CachePolicy::FIFO, memory.get()
    );

    // Read address 0 - loads block [0, 15]
    cache->read(0);

    // All addresses in block should be contained
    for (size_t i = 0; i < 16; i++) {
        EXPECT_TRUE(cache->contains(i));
    }

    // Address 16 is in different block
    EXPECT_FALSE(cache->contains(16));
}

// ===== Dump and Stats String Tests =====

TEST_F(CacheLevelDirectMappedTest, DumpDoesNotCrash) {
    cache = std::make_unique<CacheLevel>(
        1, 4, 1, 16, CachePolicy::FIFO, memory.get()
    );

    cache->read(0);
    cache->read(64);

    testing::internal::CaptureStdout();
    cache->dump();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_FALSE(output.empty());
    EXPECT_NE(output.find("L1 Cache"), std::string::npos);
}

TEST_F(CacheLevelDirectMappedTest, StatsString) {
    cache = std::make_unique<CacheLevel>(
        1, 4, 1, 16, CachePolicy::FIFO, memory.get()
    );

    cache->read(0);
    cache->read(0);

    std::string stats = cache->getStatsString();
    EXPECT_FALSE(stats.empty());
    EXPECT_NE(stats.find("Hits:"), std::string::npos);
    EXPECT_NE(stats.find("Misses:"), std::string::npos);
}

// ===== Large Cache Tests =====

TEST(CacheLevelLargeTest, LargeDirectMappedCache) {
    PhysicalMemory memory(64 * 1024);  // 64 KB

    // Initialize memory
    for (size_t i = 0; i < 64 * 1024; i++) {
        memory.write(i, static_cast<uint8_t>(i & 0xFF));
    }

    CacheLevel cache(1, 256, 1, 64, CachePolicy::LRU, &memory);

    // Access many different blocks
    for (size_t i = 0; i < 1000; i += 64) {
        auto result = cache.read(i);
        ASSERT_TRUE(result.success);
    }

    auto stats = cache.getStats();
    EXPECT_GT(stats.accesses, 0);
}

TEST(CacheLevelLargeTest, HighlyAssociativeCache) {
    PhysicalMemory memory(1024);

    for (size_t i = 0; i < 1024; i++) {
        memory.write(i, static_cast<uint8_t>(i % 256));
    }

    // 4 sets, 8-way set-associative
    CacheLevel cache(1, 4, 8, 16, CachePolicy::LRU, &memory);

    // Access many blocks mapping to same set
    for (size_t i = 0; i < 64; i += 16) {
        cache.read(i);
    }

    auto stats = cache.getStats();
    EXPECT_GT(stats.accesses, 0);
}

// ===== Address Parsing Tests =====

TEST_F(CacheLevelDirectMappedTest, AddressParsing_DifferentSets) {
    // 4 sets, 16-byte blocks
    // Offset: 4 bits [0-3]
    // Index: 2 bits [4-5]
    // Tag: remaining bits
    cache = std::make_unique<CacheLevel>(
        1, 4, 1, 16, CachePolicy::FIFO, memory.get()
    );

    // Address 0 -> set 0
    cache->read(0);
    EXPECT_TRUE(cache->contains(0));

    // Address 16 -> set 1 (different set, no conflict)
    cache->read(16);
    EXPECT_TRUE(cache->contains(0));
    EXPECT_TRUE(cache->contains(16));
}

TEST_F(CacheLevelDirectMappedTest, AddressParsing_SameSet) {
    cache = std::make_unique<CacheLevel>(
        1, 4, 1, 16, CachePolicy::FIFO, memory.get()
    );

    // Addresses 0 and 64 map to same set (direct-mapped)
    cache->read(0);
    cache->read(64);  // Evicts 0

    EXPECT_FALSE(cache->contains(0));
    EXPECT_TRUE(cache->contains(64));
}
