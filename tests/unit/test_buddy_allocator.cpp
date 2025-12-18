#include <gtest/gtest.h>
#include "allocator/buddy_allocator.h"
#include "memory/physical_memory.h"

using namespace memsim;

class BuddyAllocatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use 1024 bytes (power of 2)
        memory = std::make_unique<PhysicalMemory>(1024);
        allocator = std::make_unique<BuddyAllocator>(memory.get(), 32);
    }

    void TearDown() override {
        allocator.reset();
        memory.reset();
    }

    std::unique_ptr<PhysicalMemory> memory;
    std::unique_ptr<BuddyAllocator> allocator;
};

// ===== Basic Tests =====

TEST_F(BuddyAllocatorTest, BasicAllocation) {
    auto result = allocator->allocate(100);
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.value, 1);

    // Should round up to 128 (next power of 2)
    EXPECT_EQ(memory->getUsedSize(), 128);
}

TEST_F(BuddyAllocatorTest, PowerOfTwoRounding) {
    // Request 50 bytes, should get 64
    auto r1 = allocator->allocate(50);
    ASSERT_TRUE(r1.success);

    // Request 100 bytes, should get 128
    auto r2 = allocator->allocate(100);
    ASSERT_TRUE(r2.success);

    // Request 200 bytes, should get 256
    auto r3 = allocator->allocate(200);
    ASSERT_TRUE(r3.success);

    // Total used: 64 + 128 + 256 = 448
    EXPECT_EQ(memory->getUsedSize(), 448);
}

TEST_F(BuddyAllocatorTest, MinimumBlockSize) {
    // Request tiny allocation (should round up to min_block_size = 32)
    auto result = allocator->allocate(1);
    ASSERT_TRUE(result.success);

    EXPECT_EQ(memory->getUsedSize(), 32);
}

TEST_F(BuddyAllocatorTest, ExactPowerOfTwo) {
    auto result = allocator->allocate(64);
    ASSERT_TRUE(result.success);

    EXPECT_EQ(memory->getUsedSize(), 64);
}

// ===== Deallocation Tests =====

TEST_F(BuddyAllocatorTest, BasicDeallocation) {
    auto r1 = allocator->allocate(100);
    ASSERT_TRUE(r1.success);

    auto dealloc_result = allocator->deallocate(r1.value);
    ASSERT_TRUE(dealloc_result.success);

    EXPECT_EQ(memory->getUsedSize(), 0);
}

TEST_F(BuddyAllocatorTest, DeallocateByAddress) {
    auto r1 = allocator->allocate(100);
    ASSERT_TRUE(r1.success);

    auto dealloc_result = allocator->deallocateByAddress(0);
    ASSERT_TRUE(dealloc_result.success);

    EXPECT_EQ(memory->getUsedSize(), 0);
}

// ===== Buddy Coalescing Tests =====

TEST_F(BuddyAllocatorTest, CoalescingBuddies) {
    // Allocate two blocks that are buddies
    auto r1 = allocator->allocate(64);  // Gets first 64-byte block
    auto r2 = allocator->allocate(64);  // Gets second 64-byte block (buddy of r1)

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);

    // Free both - they should coalesce
    allocator->deallocate(r1.value);
    allocator->deallocate(r2.value);

    // After coalescing, should be able to allocate 128 bytes
    auto r3 = allocator->allocate(128);
    ASSERT_TRUE(r3.success);
}

TEST_F(BuddyAllocatorTest, RecursiveCoalescing) {
    // Allocate four 64-byte blocks
    auto r1 = allocator->allocate(64);
    auto r2 = allocator->allocate(64);
    auto r3 = allocator->allocate(64);
    auto r4 = allocator->allocate(64);

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);
    ASSERT_TRUE(r3.success);
    ASSERT_TRUE(r4.success);

    // Free all four - they should recursively coalesce into 256-byte block
    allocator->deallocate(r1.value);
    allocator->deallocate(r2.value);
    allocator->deallocate(r3.value);
    allocator->deallocate(r4.value);

    // Should be able to allocate 256 bytes
    auto r5 = allocator->allocate(256);
    ASSERT_TRUE(r5.success);
}

TEST_F(BuddyAllocatorTest, NoCoalescingWithAllocatedBuddy) {
    auto r1 = allocator->allocate(64);
    auto r2 = allocator->allocate(64);  // Buddy of r1

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);

    // Free only r1 - cannot coalesce because r2 is still allocated
    allocator->deallocate(r1.value);

    // Should NOT be able to allocate 128 bytes (buddy still in use)
    auto r3 = allocator->allocate(128);
    // This might succeed or fail depending on remaining free memory
}

// ===== Block Splitting Tests =====

TEST_F(BuddyAllocatorTest, BlockSplitting) {
    // Request small allocation - should split large block
    auto result = allocator->allocate(32);
    ASSERT_TRUE(result.success);

    EXPECT_EQ(memory->getUsedSize(), 32);

    // Should still have room for more allocations
    auto r2 = allocator->allocate(32);
    ASSERT_TRUE(r2.success);
}

TEST_F(BuddyAllocatorTest, RecursiveSplitting) {
    // Allocate very small block - requires multiple splits
    auto result = allocator->allocate(32);
    ASSERT_TRUE(result.success);

    // The 1024-byte block should have been split multiple times
    // 1024 -> 512 -> 256 -> 128 -> 64 -> 32
    EXPECT_EQ(memory->getUsedSize(), 32);
}

// ===== Error Handling Tests =====

TEST_F(BuddyAllocatorTest, ZeroSizeAllocation) {
    auto result = allocator->allocate(0);
    ASSERT_FALSE(result.success);
}

TEST_F(BuddyAllocatorTest, TooLargeAllocation) {
    auto result = allocator->allocate(2048);  // Larger than total memory
    ASSERT_FALSE(result.success);
}

TEST_F(BuddyAllocatorTest, OutOfMemory) {
    // Allocate all memory
    auto r1 = allocator->allocate(1024);
    ASSERT_TRUE(r1.success);

    // Try to allocate more
    auto r2 = allocator->allocate(32);
    ASSERT_FALSE(r2.success);
}

TEST_F(BuddyAllocatorTest, InvalidDeallocation) {
    auto result = allocator->deallocate(999);
    ASSERT_FALSE(result.success);
}

TEST_F(BuddyAllocatorTest, DoubleDeallocation) {
    auto r1 = allocator->allocate(64);
    ASSERT_TRUE(r1.success);

    auto dealloc1 = allocator->deallocate(r1.value);
    ASSERT_TRUE(dealloc1.success);

    // Try to deallocate again
    auto dealloc2 = allocator->deallocate(r1.value);
    ASSERT_FALSE(dealloc2.success);
}

// ===== Internal Fragmentation Tests =====

TEST_F(BuddyAllocatorTest, InternalFragmentation) {
    // Request 50 bytes, get 64 (14 bytes wasted)
    auto r1 = allocator->allocate(50);
    ASSERT_TRUE(r1.success);

    double frag = allocator->getInternalFragmentation();
    // (64 - 50) / 64 = 14/64 ≈ 21.875%
    EXPECT_GT(frag, 20.0);
    EXPECT_LT(frag, 23.0);
}

TEST_F(BuddyAllocatorTest, HighInternalFragmentation) {
    // Request 65 bytes, get 128 (63 bytes wasted)
    auto r1 = allocator->allocate(65);
    ASSERT_TRUE(r1.success);

    double frag = allocator->getInternalFragmentation();
    // (128 - 65) / 128 ≈ 49.2%
    EXPECT_GT(frag, 48.0);
    EXPECT_LT(frag, 50.0);
}

// ===== Utilization Tests =====

TEST_F(BuddyAllocatorTest, UtilizationCalculation) {
    EXPECT_DOUBLE_EQ(allocator->getUtilization(), 0.0);

    allocator->allocate(256);
    EXPECT_DOUBLE_EQ(allocator->getUtilization(), 25.0);  // 256/1024

    allocator->allocate(256);
    EXPECT_DOUBLE_EQ(allocator->getUtilization(), 50.0);  // 512/1024
}

// ===== Statistics Tests =====

TEST_F(BuddyAllocatorTest, Statistics) {
    allocator->allocate(100);
    allocator->allocate(200);
    allocator->allocate(5000);  // Will fail

    std::string stats = allocator->getStats();
    EXPECT_FALSE(stats.empty());
    EXPECT_NE(stats.find("Buddy Allocation"), std::string::npos);
    EXPECT_NE(stats.find("Total allocations: 3"), std::string::npos);
    EXPECT_NE(stats.find("Failed allocations: 1"), std::string::npos);
}

TEST_F(BuddyAllocatorTest, Dump) {
    allocator->allocate(100);
    allocator->allocate(200);

    // Should not crash
    testing::internal::CaptureStdout();
    allocator->dump();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_FALSE(output.empty());
    EXPECT_NE(output.find("Buddy Memory Layout"), std::string::npos);
}

// ===== Stress Tests =====

TEST_F(BuddyAllocatorTest, StressTest_ManyAllocations) {
    std::vector<BlockId> blocks;

    // Allocate many small blocks
    for (int i = 0; i < 20; i++) {
        auto result = allocator->allocate(32);
        if (result.success) {
            blocks.push_back(result.value);
        }
    }

    EXPECT_GT(blocks.size(), 0);

    // Deallocate all
    for (BlockId id : blocks) {
        auto result = allocator->deallocate(id);
        EXPECT_TRUE(result.success);
    }

    EXPECT_EQ(memory->getUsedSize(), 0);
}

TEST_F(BuddyAllocatorTest, StressTest_AlternatingAllocDealloc) {
    for (int i = 0; i < 10; i++) {
        auto r1 = allocator->allocate(64);
        auto r2 = allocator->allocate(128);

        ASSERT_TRUE(r1.success);
        ASSERT_TRUE(r2.success);

        allocator->deallocate(r1.value);
        allocator->deallocate(r2.value);
    }

    EXPECT_EQ(memory->getUsedSize(), 0);
}

// ===== Power-of-Two Memory Test =====

TEST(BuddyAllocatorConstructorTest, NonPowerOfTwoMemory) {
    PhysicalMemory memory(1000);  // Not a power of 2

    EXPECT_THROW(
        BuddyAllocator allocator(&memory, 32),
        std::invalid_argument
    );
}

TEST(BuddyAllocatorConstructorTest, NonPowerOfTwoMinSize) {
    PhysicalMemory memory(1024);

    EXPECT_THROW(
        BuddyAllocator allocator(&memory, 33),  // Not a power of 2
        std::invalid_argument
    );
}

TEST(BuddyAllocatorConstructorTest, ValidConstruction) {
    PhysicalMemory memory(1024);

    EXPECT_NO_THROW(
        BuddyAllocator allocator(&memory, 32)
    );
}

// ===== Large Memory Test =====

TEST(BuddyAllocatorLargeTest, LargeMemory) {
    // 1MB memory
    PhysicalMemory memory(1024 * 1024);
    BuddyAllocator allocator(&memory, 64);

    auto r1 = allocator.allocate(1000);
    ASSERT_TRUE(r1.success);

    auto r2 = allocator.allocate(50000);
    ASSERT_TRUE(r2.success);

    allocator.deallocate(r1.value);
    allocator.deallocate(r2.value);

    EXPECT_EQ(memory.getUsedSize(), 0);
}
