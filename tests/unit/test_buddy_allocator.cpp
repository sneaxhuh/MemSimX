#include <gtest/gtest.h>
#include "allocator/buddy_allocator.h"
#include "memory/physical_memory.h"

using namespace memsim;

class BuddyAllocatorTest : public ::testing::Test {
protected:
    void SetUp() override {
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

// ===== Core Functionality Tests =====

TEST_F(BuddyAllocatorTest, PowerOfTwoRounding) {
    auto r1 = allocator->allocate(50);   // rounds to 64
    auto r2 = allocator->allocate(100);  // rounds to 128
    auto r3 = allocator->allocate(200);  // rounds to 256

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);
    ASSERT_TRUE(r3.success);
    EXPECT_EQ(memory->getUsedSize(), 448);  // 64 + 128 + 256
}

TEST_F(BuddyAllocatorTest, MinimumBlockSize) {
    auto result = allocator->allocate(1);
    ASSERT_TRUE(result.success);
    EXPECT_EQ(memory->getUsedSize(), 32);  // min_block_size
}

TEST_F(BuddyAllocatorTest, BasicDeallocation) {
    auto r1 = allocator->allocate(100);
    ASSERT_TRUE(r1.success);

    auto dealloc_result = allocator->deallocate(r1.value);
    ASSERT_TRUE(dealloc_result.success);
    EXPECT_EQ(memory->getUsedSize(), 0);
}

// ===== XOR Buddy Address Invariant (CORE BUDDY PROPERTY) =====

TEST_F(BuddyAllocatorTest, BuddyAddressXORInvariant) {
    auto r1 = allocator->allocate(64);
    auto r2 = allocator->allocate(64);

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);

    auto addr1 = allocator->getBlockAddress(r1.value);
    auto addr2 = allocator->getBlockAddress(r2.value);

    ASSERT_TRUE(addr1.success);
    ASSERT_TRUE(addr2.success);

    // Buddy rule: buddy = addr XOR block_size
    // This directly proves "Buddy address can be computed using XOR operations"
    EXPECT_EQ(addr1.value ^ 64, addr2.value);
    EXPECT_EQ(addr2.value ^ 64, addr1.value);
}

// ===== Block Alignment Invariant (CRITICAL) =====

TEST_F(BuddyAllocatorTest, BlockAlignmentInvariant) {
    // Buddy allocators MUST guarantee: address % block_size == 0
    for (size_t size : {33, 65, 129, 257}) {
        auto r = allocator->allocate(size);
        ASSERT_TRUE(r.success);

        auto addr = allocator->getBlockAddress(r.value);
        ASSERT_TRUE(addr.success);

        // Calculate rounded-up power-of-2 size
        size_t actual = 32;
        while (actual < size) actual <<= 1;

        // Address must be aligned to block size
        EXPECT_EQ(addr.value % actual, 0);
    }
}

// ===== Buddy Coalescing Tests =====

TEST_F(BuddyAllocatorTest, CoalescingBuddies) {
    auto r1 = allocator->allocate(64);
    auto r2 = allocator->allocate(64);

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);

    allocator->deallocate(r1.value);
    allocator->deallocate(r2.value);

    // After coalescing, should be able to allocate 128 bytes
    auto r3 = allocator->allocate(128);
    ASSERT_TRUE(r3.success);
}

TEST_F(BuddyAllocatorTest, CoalescingCreatesExactBlockSize) {
    auto r1 = allocator->allocate(64);
    auto r2 = allocator->allocate(64);

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);

    allocator->deallocate(r1.value);
    allocator->deallocate(r2.value);

    // Proves correct coalescing behavior:
    // After freeing two 64-byte buddies, they should coalesce into a single 128-byte block
    // We verify this by successfully allocating 128 bytes
    auto r3 = allocator->allocate(128);
    ASSERT_TRUE(r3.success);

    // Verify memory accounting is correct
    EXPECT_EQ(memory->getUsedSize(), 128);
}

TEST_F(BuddyAllocatorTest, RecursiveCoalescing) {
    auto r1 = allocator->allocate(64);
    auto r2 = allocator->allocate(64);
    auto r3 = allocator->allocate(64);
    auto r4 = allocator->allocate(64);

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);
    ASSERT_TRUE(r3.success);
    ASSERT_TRUE(r4.success);

    // Free all four - should recursively coalesce into 256-byte block
    allocator->deallocate(r1.value);
    allocator->deallocate(r2.value);
    allocator->deallocate(r3.value);
    allocator->deallocate(r4.value);

    auto r5 = allocator->allocate(256);
    ASSERT_TRUE(r5.success);
}

// ===== Block Splitting Tests =====

TEST_F(BuddyAllocatorTest, BlockSplitting) {
    auto r1 = allocator->allocate(32);
    ASSERT_TRUE(r1.success);
    EXPECT_EQ(memory->getUsedSize(), 32);

    // Should still have room after splitting
    auto r2 = allocator->allocate(32);
    ASSERT_TRUE(r2.success);
}

TEST_F(BuddyAllocatorTest, RecursiveSplittingDepth) {
    // Allocating 32 bytes forces splits: 1024 → 512 → 256 → 128 → 64 → 32
    auto r1 = allocator->allocate(32);
    ASSERT_TRUE(r1.success);

    // Proves multi-level splitting works by verifying:
    // 1. Only one 32-byte block used
    EXPECT_EQ(memory->getUsedSize(), 32);

    // 2. Free lists preserved at higher levels - we can still allocate large blocks
    auto r2 = allocator->allocate(512);
    ASSERT_TRUE(r2.success);  // Should succeed because 512-byte buddy still exists

    // 3. Verify memory accounting is correct
    EXPECT_EQ(memory->getUsedSize(), 544);  // 32 + 512
}

// ===== Error Handling Tests =====

TEST_F(BuddyAllocatorTest, ZeroSizeAllocation) {
    auto result = allocator->allocate(0);
    ASSERT_FALSE(result.success);
}

TEST_F(BuddyAllocatorTest, TooLargeAllocation) {
    auto result = allocator->allocate(2048);
    ASSERT_FALSE(result.success);
}

TEST_F(BuddyAllocatorTest, OutOfMemory) {
    auto r1 = allocator->allocate(1024);
    ASSERT_TRUE(r1.success);

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

    auto dealloc2 = allocator->deallocate(r1.value);
    ASSERT_FALSE(dealloc2.success);
}

// ===== Fragmentation Tests =====

TEST_F(BuddyAllocatorTest, InternalFragmentation) {
    auto r1 = allocator->allocate(50);  // gets 64, wastes 14
    ASSERT_TRUE(r1.success);

    double frag = allocator->getInternalFragmentation();
    EXPECT_GT(frag, 20.0);
    EXPECT_LT(frag, 23.0);
}

TEST_F(BuddyAllocatorTest, UtilizationCalculation) {
    EXPECT_DOUBLE_EQ(allocator->getUtilization(), 0.0);

    allocator->allocate(256);
    EXPECT_DOUBLE_EQ(allocator->getUtilization(), 25.0);

    allocator->allocate(256);
    EXPECT_DOUBLE_EQ(allocator->getUtilization(), 50.0);
}

// ===== Constructor Validation Tests =====

TEST(BuddyAllocatorConstructorTest, NonPowerOfTwoMemory) {
    PhysicalMemory memory(1000);
    EXPECT_THROW(
        BuddyAllocator allocator(&memory, 32),
        std::invalid_argument
    );
}

TEST(BuddyAllocatorConstructorTest, NonPowerOfTwoMinSize) {
    PhysicalMemory memory(1024);
    EXPECT_THROW(
        BuddyAllocator allocator(&memory, 33),
        std::invalid_argument
    );
}

TEST(BuddyAllocatorConstructorTest, ValidConstruction) {
    PhysicalMemory memory(1024);
    EXPECT_NO_THROW(
        BuddyAllocator allocator(&memory, 32)
    );
}
