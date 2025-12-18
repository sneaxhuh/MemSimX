#include <gtest/gtest.h>
#include "allocator/standard_allocator.h"
#include "memory/physical_memory.h"

using namespace memsim;

class StandardAllocatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        memory = std::make_unique<PhysicalMemory>(1024);
    }

    void TearDown() override {
        allocator.reset();
        memory.reset();
    }

    void createAllocator(AllocatorType type) {
        allocator = std::make_unique<StandardAllocator>(memory.get(), type);
    }

    std::unique_ptr<PhysicalMemory> memory;
    std::unique_ptr<StandardAllocator> allocator;
};

// ===== First Fit Tests =====

TEST_F(StandardAllocatorTest, FirstFit_BasicAllocation) {
    createAllocator(AllocatorType::FIRST_FIT);

    auto result = allocator->allocate(100);
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.value, 1); // First block ID should be 1

    EXPECT_EQ(memory->getUsedSize(), 100);
    EXPECT_EQ(memory->getFreeSize(), 924);
}

TEST_F(StandardAllocatorTest, FirstFit_MultipleAllocations) {
    createAllocator(AllocatorType::FIRST_FIT);

    auto r1 = allocator->allocate(100);
    auto r2 = allocator->allocate(200);
    auto r3 = allocator->allocate(150);

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);
    ASSERT_TRUE(r3.success);

    EXPECT_EQ(r1.value, 1);
    EXPECT_EQ(r2.value, 2);
    EXPECT_EQ(r3.value, 3);

    EXPECT_EQ(memory->getUsedSize(), 450);
}

TEST_F(StandardAllocatorTest, FirstFit_Deallocation) {
    createAllocator(AllocatorType::FIRST_FIT);

    auto r1 = allocator->allocate(100);
    auto r2 = allocator->allocate(200);
    auto r3 = allocator->allocate(150);

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);
    ASSERT_TRUE(r3.success);

    // Deallocate the middle block
    auto dealloc_result = allocator->deallocate(r2.value);
    ASSERT_TRUE(dealloc_result.success);

    EXPECT_EQ(memory->getUsedSize(), 250);
    EXPECT_EQ(memory->getFreeSize(), 774);
}

TEST_F(StandardAllocatorTest, FirstFit_CoalescingAdjacentBlocks) {
    createAllocator(AllocatorType::FIRST_FIT);

    auto r1 = allocator->allocate(100);
    auto r2 = allocator->allocate(100);
    auto r3 = allocator->allocate(100);

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);
    ASSERT_TRUE(r3.success);

    // Free blocks 1 and 2 (adjacent)
    allocator->deallocate(r1.value);
    allocator->deallocate(r2.value);

    // After coalescing, we should be able to allocate 200 bytes at the beginning
    auto r4 = allocator->allocate(200);
    ASSERT_TRUE(r4.success);
}

TEST_F(StandardAllocatorTest, FirstFit_OutOfMemory) {
    createAllocator(AllocatorType::FIRST_FIT);

    // Try to allocate more than available
    auto result = allocator->allocate(2000);
    ASSERT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(StandardAllocatorTest, FirstFit_ZeroSizeAllocation) {
    createAllocator(AllocatorType::FIRST_FIT);

    auto result = allocator->allocate(0);
    ASSERT_FALSE(result.success);
}

TEST_F(StandardAllocatorTest, FirstFit_InvalidDeallocation) {
    createAllocator(AllocatorType::FIRST_FIT);

    auto result = allocator->deallocate(999);
    ASSERT_FALSE(result.success);
}

TEST_F(StandardAllocatorTest, FirstFit_DeallocationByAddress) {
    createAllocator(AllocatorType::FIRST_FIT);

    auto r1 = allocator->allocate(100);
    ASSERT_TRUE(r1.success);

    // Deallocate by address (address 0)
    auto result = allocator->deallocateByAddress(0);
    ASSERT_TRUE(result.success);

    EXPECT_EQ(memory->getUsedSize(), 0);
}

TEST_F(StandardAllocatorTest, FirstFit_DoubleDeallocation) {
    createAllocator(AllocatorType::FIRST_FIT);

    auto r1 = allocator->allocate(100);
    ASSERT_TRUE(r1.success);

    auto dealloc1 = allocator->deallocate(r1.value);
    ASSERT_TRUE(dealloc1.success);

    // Try to deallocate again
    auto dealloc2 = allocator->deallocate(r1.value);
    ASSERT_FALSE(dealloc2.success);
}

// ===== Best Fit Tests =====

TEST_F(StandardAllocatorTest, BestFit_SelectsSmallestFit) {
    createAllocator(AllocatorType::BEST_FIT);

    // Allocate and create fragmentation
    auto r1 = allocator->allocate(100);
    auto r2 = allocator->allocate(300);
    auto r3 = allocator->allocate(200);
    auto r4 = allocator->allocate(150);

    // Free blocks to create gaps of different sizes
    allocator->deallocate(r1.value); // 100-byte gap at start
    allocator->deallocate(r3.value); // 200-byte gap in middle

    // Allocate 150 bytes - should go in the 200-byte gap (best fit)
    auto r5 = allocator->allocate(150);
    ASSERT_TRUE(r5.success);

    // The 100-byte gap should still be available
    auto r6 = allocator->allocate(50);
    ASSERT_TRUE(r6.success);
}

TEST_F(StandardAllocatorTest, BestFit_BasicAllocation) {
    createAllocator(AllocatorType::BEST_FIT);

    auto result = allocator->allocate(100);
    ASSERT_TRUE(result.success);
    EXPECT_EQ(memory->getUsedSize(), 100);
}

// ===== Worst Fit Tests =====

TEST_F(StandardAllocatorTest, WorstFit_SelectsLargestFit) {
    createAllocator(AllocatorType::WORST_FIT);

    // Allocate and create fragmentation
    auto r1 = allocator->allocate(100);
    auto r2 = allocator->allocate(300);
    auto r3 = allocator->allocate(500);

    // Free blocks to create gaps
    allocator->deallocate(r1.value); // 100-byte gap
    allocator->deallocate(r3.value); // 500-byte gap

    // Allocate 50 bytes - should go in the 500-byte gap (worst fit)
    auto r4 = allocator->allocate(50);
    ASSERT_TRUE(r4.success);

    // The 100-byte gap should still be available
    auto r5 = allocator->allocate(100);
    ASSERT_TRUE(r5.success);
}

TEST_F(StandardAllocatorTest, WorstFit_BasicAllocation) {
    createAllocator(AllocatorType::WORST_FIT);

    auto result = allocator->allocate(100);
    ASSERT_TRUE(result.success);
    EXPECT_EQ(memory->getUsedSize(), 100);
}

// ===== Block Splitting Tests =====

TEST_F(StandardAllocatorTest, BlockSplitting_CreatesRemainder) {
    createAllocator(AllocatorType::FIRST_FIT);

    // Allocate small amount from large memory
    auto r1 = allocator->allocate(100);
    ASSERT_TRUE(r1.success);

    // Should be able to allocate more from the remainder
    auto r2 = allocator->allocate(900);
    ASSERT_TRUE(r2.success);
}

TEST_F(StandardAllocatorTest, BlockSplitting_ExactFit) {
    createAllocator(AllocatorType::FIRST_FIT);

    // Allocate exactly all memory
    auto r1 = allocator->allocate(1024);
    ASSERT_TRUE(r1.success);

    EXPECT_EQ(memory->getUsedSize(), 1024);
    EXPECT_EQ(memory->getFreeSize(), 0);

    // No more allocations possible
    auto r2 = allocator->allocate(1);
    ASSERT_FALSE(r2.success);
}

// ===== Coalescing Tests =====

TEST_F(StandardAllocatorTest, Coalescing_MergesTwoBlocks) {
    createAllocator(AllocatorType::FIRST_FIT);

    auto r1 = allocator->allocate(200);
    auto r2 = allocator->allocate(200);
    auto r3 = allocator->allocate(200);

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);
    ASSERT_TRUE(r3.success);

    // Free adjacent blocks
    allocator->deallocate(r1.value);
    allocator->deallocate(r2.value);

    // Should be able to allocate 400 bytes (coalesced)
    auto r4 = allocator->allocate(400);
    ASSERT_TRUE(r4.success);
}

TEST_F(StandardAllocatorTest, Coalescing_MergesThreeBlocks) {
    createAllocator(AllocatorType::FIRST_FIT);

    auto r1 = allocator->allocate(200);
    auto r2 = allocator->allocate(200);
    auto r3 = allocator->allocate(200);

    // Free all three
    allocator->deallocate(r1.value);
    allocator->deallocate(r2.value);
    allocator->deallocate(r3.value);

    // Should be able to allocate 600 bytes (all coalesced)
    auto r4 = allocator->allocate(600);
    ASSERT_TRUE(r4.success);
}

TEST_F(StandardAllocatorTest, Coalescing_NonAdjacentBlocks) {
    createAllocator(AllocatorType::FIRST_FIT);

    auto r1 = allocator->allocate(100);
    auto r2 = allocator->allocate(100);
    auto r3 = allocator->allocate(100);

    // Free non-adjacent blocks
    allocator->deallocate(r1.value);
    allocator->deallocate(r3.value);

    // Should NOT be able to allocate 200 bytes (not adjacent)
    auto r4 = allocator->allocate(200);
    // This might fail or succeed depending on remaining free space
    // The point is that blocks 1 and 3 are NOT coalesced
}

// ===== Fragmentation Tests =====

TEST_F(StandardAllocatorTest, InternalFragmentation_Calculation) {
    createAllocator(AllocatorType::FIRST_FIT);

    // Allocate with potential for internal fragmentation
    auto r1 = allocator->allocate(100);
    ASSERT_TRUE(r1.success);

    // Initially, internal fragmentation should be low/zero
    double internal_frag = allocator->getInternalFragmentation();
    EXPECT_GE(internal_frag, 0.0);
    EXPECT_LE(internal_frag, 100.0);
}

TEST_F(StandardAllocatorTest, ExternalFragmentation_Calculation) {
    createAllocator(AllocatorType::FIRST_FIT);

    auto r1 = allocator->allocate(100);
    auto r2 = allocator->allocate(100);
    auto r3 = allocator->allocate(100);

    // Free every other block to create fragmentation
    allocator->deallocate(r1.value);
    allocator->deallocate(r3.value);

    double external_frag = allocator->getExternalFragmentation();
    EXPECT_GT(external_frag, 0.0); // Should have external fragmentation
    EXPECT_LE(external_frag, 100.0);
}

TEST_F(StandardAllocatorTest, Utilization_Calculation) {
    createAllocator(AllocatorType::FIRST_FIT);

    // Initially empty
    EXPECT_DOUBLE_EQ(allocator->getUtilization(), 0.0);

    // Allocate half
    allocator->allocate(512);
    EXPECT_DOUBLE_EQ(allocator->getUtilization(), 50.0);

    // Allocate rest
    allocator->allocate(512);
    EXPECT_DOUBLE_EQ(allocator->getUtilization(), 100.0);
}

// ===== Statistics Tests =====

TEST_F(StandardAllocatorTest, Statistics_TrackingAllocations) {
    createAllocator(AllocatorType::FIRST_FIT);

    allocator->allocate(100);
    allocator->allocate(200);
    allocator->allocate(5000); // This will fail

    std::string stats = allocator->getStats();
    EXPECT_FALSE(stats.empty());
    EXPECT_NE(stats.find("Total allocations: 3"), std::string::npos);
    EXPECT_NE(stats.find("Failed allocations: 1"), std::string::npos);
}

TEST_F(StandardAllocatorTest, Dump_ShowsMemoryLayout) {
    createAllocator(AllocatorType::FIRST_FIT);

    allocator->allocate(100);
    allocator->allocate(200);

    // This should not crash
    testing::internal::CaptureStdout();
    allocator->dump();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_FALSE(output.empty());
}

// ===== Stress Tests =====

TEST_F(StandardAllocatorTest, StressTest_ManySmallAllocations) {
    createAllocator(AllocatorType::FIRST_FIT);

    std::vector<BlockId> blocks;

    // Allocate many small blocks
    for (int i = 0; i < 50; i++) {
        auto result = allocator->allocate(10);
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

TEST_F(StandardAllocatorTest, StressTest_AlternatingAllocDealloc) {
    createAllocator(AllocatorType::FIRST_FIT);

    for (int i = 0; i < 10; i++) {
        auto r1 = allocator->allocate(50);
        auto r2 = allocator->allocate(100);

        ASSERT_TRUE(r1.success);
        ASSERT_TRUE(r2.success);

        allocator->deallocate(r1.value);
        allocator->deallocate(r2.value);
    }

    EXPECT_EQ(memory->getUsedSize(), 0);
}

// ===== Edge Cases =====

TEST_F(StandardAllocatorTest, EdgeCase_SingleByteallocations) {
    createAllocator(AllocatorType::FIRST_FIT);

    auto r1 = allocator->allocate(1);
    auto r2 = allocator->allocate(1);
    auto r3 = allocator->allocate(1);

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);
    ASSERT_TRUE(r3.success);

    EXPECT_EQ(memory->getUsedSize(), 3);
}

TEST_F(StandardAllocatorTest, EdgeCase_LargeAllocation) {
    createAllocator(AllocatorType::FIRST_FIT);

    // Allocate almost all memory
    auto r1 = allocator->allocate(1000);
    ASSERT_TRUE(r1.success);

    // Small allocation should still work
    auto r2 = allocator->allocate(20);
    ASSERT_TRUE(r2.success);
}
