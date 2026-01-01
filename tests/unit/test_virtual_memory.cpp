#include <gtest/gtest.h>
#include "virtual_memory/virtual_memory.h"
#include "memory/physical_memory.h"

using namespace memsim;

class VirtualMemoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 4KB physical memory, 256-byte pages = 16 physical frames
        memory = std::make_unique<PhysicalMemory>(4096);
    }

    void TearDown() override {
        vm.reset();
        memory.reset();
    }

    std::unique_ptr<PhysicalMemory> memory;
    std::unique_ptr<VirtualMemory> vm;
};

// ===== Constructor Tests =====

TEST_F(VirtualMemoryTest, ValidConstruction) {
    EXPECT_NO_THROW({
        vm = std::make_unique<VirtualMemory>(
            memory.get(),
            32,    // 32 virtual pages
            16,    // 16 physical frames
            256,   // 256-byte pages
            PageReplacementPolicy::FIFO
        );
    });
}

TEST_F(VirtualMemoryTest, InvalidPageSize_NotPowerOfTwo) {
    EXPECT_THROW({
        vm = std::make_unique<VirtualMemory>(
            memory.get(), 32, 16, 255, PageReplacementPolicy::FIFO
        );
    }, std::invalid_argument);
}

TEST_F(VirtualMemoryTest, InvalidNumVirtualPages_Zero) {
    EXPECT_THROW({
        vm = std::make_unique<VirtualMemory>(
            memory.get(), 0, 16, 256, PageReplacementPolicy::FIFO
        );
    }, std::invalid_argument);
}

TEST_F(VirtualMemoryTest, InvalidNumPhysicalFrames_Zero) {
    EXPECT_THROW({
        vm = std::make_unique<VirtualMemory>(
            memory.get(), 32, 0, 256, PageReplacementPolicy::FIFO
        );
    }, std::invalid_argument);
}

TEST_F(VirtualMemoryTest, InvalidFrames_ExceedVirtualPages) {
    EXPECT_THROW({
        vm = std::make_unique<VirtualMemory>(
            memory.get(), 16, 32, 256, PageReplacementPolicy::FIFO
        );
    }, std::invalid_argument);
}

// ===== Address Translation Tests =====

TEST_F(VirtualMemoryTest, BasicTranslation_ColdMiss) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 32, 16, 256, PageReplacementPolicy::FIFO
    );

    // First access - page fault
    auto result = vm->translate(0);
    ASSERT_TRUE(result.success);

    auto stats = vm->getStats();
    EXPECT_EQ(stats.page_faults, 1);
    EXPECT_EQ(stats.page_hits, 0);
}

TEST_F(VirtualMemoryTest, BasicTranslation_PageHit) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 32, 16, 256, PageReplacementPolicy::FIFO
    );

    // First access - page fault
    vm->translate(0);

    // Second access - page hit
    auto result = vm->translate(0);
    ASSERT_TRUE(result.success);

    auto stats = vm->getStats();
    EXPECT_EQ(stats.page_faults, 1);
    EXPECT_EQ(stats.page_hits, 1);
}

TEST_F(VirtualMemoryTest, AddressParsing_SamePage) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 32, 16, 256, PageReplacementPolicy::FIFO
    );

    // Addresses 0 and 100 are in same page (page 0)
    vm->translate(0);
    auto result = vm->translate(100);

    auto stats = vm->getStats();
    EXPECT_EQ(stats.page_faults, 1);  // Only one fault
    EXPECT_EQ(stats.page_hits, 1);    // Second is a hit
}

TEST_F(VirtualMemoryTest, AddressParsing_DifferentPages) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 32, 16, 256, PageReplacementPolicy::FIFO
    );

    // Address 0 -> page 0, address 256 -> page 1
    vm->translate(0);
    auto result = vm->translate(256);

    auto stats = vm->getStats();
    EXPECT_EQ(stats.page_faults, 2);  // Two different pages
}

// ===== Read/Write Tests =====

TEST_F(VirtualMemoryTest, BasicRead) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 32, 16, 256, PageReplacementPolicy::FIFO
    );

    auto result = vm->read(0);
    ASSERT_TRUE(result.success);
    // loadPageFromDisk initializes with pattern based on address
    EXPECT_EQ(result.value, 0);
}

TEST_F(VirtualMemoryTest, BasicWrite) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 32, 16, 256, PageReplacementPolicy::FIFO
    );

    auto write_result = vm->write(10, 99);
    ASSERT_TRUE(write_result.success);

    auto read_result = vm->read(10);
    ASSERT_TRUE(read_result.success);
    EXPECT_EQ(read_result.value, 99);
}

TEST_F(VirtualMemoryTest, WriteThenRead_MultiplePage) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 32, 16, 256, PageReplacementPolicy::FIFO
    );

    // Write to different pages
    vm->write(0, 10);
    vm->write(256, 20);
    vm->write(512, 30);

    // Read back
    auto r1 = vm->read(0);
    auto r2 = vm->read(256);
    auto r3 = vm->read(512);

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);
    ASSERT_TRUE(r3.success);

    EXPECT_EQ(r1.value, 10);
    EXPECT_EQ(r2.value, 20);
    EXPECT_EQ(r3.value, 30);
}

// ===== FIFO Page Replacement Tests =====

TEST_F(VirtualMemoryTest, FIFO_PageReplacement) {
    // 3 physical frames, 10 virtual pages
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 10, 3, 256, PageReplacementPolicy::FIFO
    );

    // Load 3 pages (fill all frames)
    vm->read(0);      // Page 0
    vm->read(256);    // Page 1
    vm->read(512);    // Page 2

    // Load 4th page - should evict page 0 (FIFO)
    vm->read(768);    // Page 3

    auto stats = vm->getStats();
    EXPECT_EQ(stats.page_faults, 4);
}

TEST_F(VirtualMemoryTest, FIFO_OrderPreservation) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 10, 3, 256, PageReplacementPolicy::FIFO
    );

    // Load pages 0, 1, 2
    vm->read(0);
    vm->read(256);
    vm->read(512);

    // Access page 0 again (doesn't change FIFO order)
    vm->read(0);

    // Load page 3 - should still evict page 0
    vm->read(768);

    // Try to access page 0 - should be page fault
    vm->read(0);

    auto stats = vm->getStats();
    EXPECT_EQ(stats.page_faults, 5);  // 0,1,2,3, and 0 again
}

// ===== LRU Page Replacement Tests =====

TEST_F(VirtualMemoryTest, LRU_PageReplacement) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 10, 3, 256, PageReplacementPolicy::LRU
    );

    // Load 3 pages
    vm->read(0);      // Page 0
    vm->read(256);    // Page 1
    vm->read(512);    // Page 2

    // Access page 0 again (updates LRU)
    vm->read(0);

    // Load page 3 - should evict page 1 (least recently used)
    vm->read(768);

    // Access page 1 - should be page fault
    vm->read(256);

    auto stats = vm->getStats();
    EXPECT_EQ(stats.page_faults, 5);  // 0, 1, 2, 3, and 1 again
}

TEST_F(VirtualMemoryTest, LRU_UpdateOnAccess) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 10, 3, 256, PageReplacementPolicy::LRU
    );

    vm->read(0);
    vm->read(256);
    vm->read(512);

    // Access page 0 multiple times
    vm->read(0);
    vm->read(0);
    vm->read(0);

    // Load page 3 - should evict page 1 (not page 0)
    vm->read(768);

    // Page 0 should still be in memory
    vm->read(0);

    auto stats = vm->getStats();
    // Faults: 0, 1, 2, 3 = 4 total
    // Hits: 0 (3 times), 0 (after page 3) = 4 hits
    EXPECT_EQ(stats.page_faults, 4);
    EXPECT_EQ(stats.page_hits, 4);
}

// ===== Flush Tests =====

TEST_F(VirtualMemoryTest, Flush) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 32, 16, 256, PageReplacementPolicy::FIFO
    );

    vm->read(0);
    vm->read(256);

    vm->flush();

    // After flush, accessing same pages should cause faults
    vm->read(0);
    vm->read(256);

    auto stats = vm->getStats();
    // 2 before flush + 2 after = 4 faults total
    EXPECT_EQ(stats.page_faults, 4);
}

// ===== Statistics Tests =====

TEST_F(VirtualMemoryTest, PageFaultRate) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 32, 16, 256, PageReplacementPolicy::FIFO
    );

    // 2 faults
    vm->read(0);
    vm->read(256);

    // 3 hits (same pages)
    vm->read(0);
    vm->read(256);
    vm->read(0);

    auto stats = vm->getStats();
    EXPECT_EQ(stats.total_accesses, 5);
    EXPECT_DOUBLE_EQ(stats.getPageFaultRate(), 40.0);  // 2/5 = 40%
}

TEST_F(VirtualMemoryTest, PageHitRate) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 32, 16, 256, PageReplacementPolicy::FIFO
    );

    vm->read(0);     // Fault
    vm->read(0);     // Hit
    vm->read(0);     // Hit

    auto stats = vm->getStats();
    EXPECT_NEAR(stats.getPageHitRate(), 66.67, 0.01);  // 2/3 ≈ 66.67%
}

TEST_F(VirtualMemoryTest, StatsString) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 32, 16, 256, PageReplacementPolicy::FIFO
    );

    vm->read(0);
    vm->read(0);

    std::string stats = vm->getStatsString();
    EXPECT_FALSE(stats.empty());
    EXPECT_NE(stats.find("Page Faults:"), std::string::npos);
    EXPECT_NE(stats.find("Page Hits:"), std::string::npos);
}

// ===== Dump Test =====

TEST_F(VirtualMemoryTest, DumpDoesNotCrash) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 32, 16, 256, PageReplacementPolicy::LRU
    );

    vm->read(0);
    vm->read(256);

    testing::internal::CaptureStdout();
    vm->dump();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_FALSE(output.empty());
    EXPECT_NE(output.find("Page Table"), std::string::npos);
}

// ===== Stress Tests =====

TEST_F(VirtualMemoryTest, StressTest_ManyPages) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 16, 8, 256, PageReplacementPolicy::LRU
    );

    // Access all 16 virtual pages (more than physical frames)
    for (size_t i = 0; i < 16; i++) {
        vm->read(i * 256);
    }

    auto stats = vm->getStats();
    EXPECT_EQ(stats.page_faults, 16);
}

TEST_F(VirtualMemoryTest, StressTest_RepeatedAccesses) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 32, 16, 256, PageReplacementPolicy::FIFO
    );

    // Repeated access pattern
    for (int i = 0; i < 100; i++) {
        vm->read((i % 10) * 256);  // Cycle through 10 pages
    }

    auto stats = vm->getStats();
    EXPECT_EQ(stats.total_accesses, 100);
    // First 10 are faults, rest are hits
    EXPECT_EQ(stats.page_faults, 10);
    EXPECT_EQ(stats.page_hits, 90);
}

// ===== Dirty Page Tests =====

TEST_F(VirtualMemoryTest, DirtyBit_SetOnWrite) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 32, 16, 256, PageReplacementPolicy::FIFO
    );

    // Write sets dirty bit
    vm->write(0, 42);

    // Subsequent read should work
    auto result = vm->read(0);
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.value, 42);
}

// ===== Workload Pattern Tests =====

TEST_F(VirtualMemoryTest, SequentialAccess) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 16, 8, 256, PageReplacementPolicy::LRU
    );

    // Sequential access within pages
    for (size_t i = 0; i < 100; i++) {
        vm->read(i);
    }

    auto stats = vm->getStats();
    // 100 addresses in 256-byte pages = 1 page
    EXPECT_EQ(stats.page_faults, 1);
    EXPECT_EQ(stats.page_hits, 99);
}

TEST_F(VirtualMemoryTest, RandomAccess) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 16, 8, 256, PageReplacementPolicy::LRU
    );

    // Pseudo-random access pattern
    for (size_t i = 0; i < 50; i++) {
        size_t addr = (i * 137) % (16 * 256);
        vm->read(addr);
    }

    auto stats = vm->getStats();
    EXPECT_EQ(stats.total_accesses, 50);
}

TEST_F(VirtualMemoryTest, LocalityOfReference) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 32, 8, 256, PageReplacementPolicy::LRU
    );

    // Good locality - repeatedly access same few pages
    for (int i = 0; i < 100; i++) {
        vm->read((i % 3) * 256);  // Only 3 pages
    }

    auto stats = vm->getStats();
    // 3 faults for 3 pages, rest are hits
    EXPECT_EQ(stats.page_faults, 3);
    EXPECT_EQ(stats.page_hits, 97);
}

// ===== Large Virtual Memory Test =====

TEST(VirtualMemoryLargeTest, LargeAddressSpace) {
    PhysicalMemory memory(64 * 1024);  // 64 KB physical

    // 256 virtual pages, 64 physical frames, 1KB pages
    VirtualMemory vm(
        &memory, 256, 64, 1024, PageReplacementPolicy::LRU
    );

    // Access various pages
    for (size_t i = 0; i < 100; i++) {
        auto result = vm.read(i * 1024);
        ASSERT_TRUE(result.success);
    }

    auto stats = vm.getStats();
    EXPECT_GT(stats.total_accesses, 0);
}

// ===== Edge Cases =====

TEST_F(VirtualMemoryTest, InvalidVirtualAddress) {
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 8, 4, 256, PageReplacementPolicy::FIFO
    );

    // Address beyond virtual address space (8 pages * 256 bytes = 2048)
    auto result = vm->translate(3000);
    ASSERT_FALSE(result.success);
}

TEST_F(VirtualMemoryTest, AllFramesOccupied) {
    // 4 frames, access more than 4 pages
    vm = std::make_unique<VirtualMemory>(
        memory.get(), 10, 4, 256, PageReplacementPolicy::FIFO
    );

    // Fill all frames
    for (size_t i = 0; i < 4; i++) {
        vm->read(i * 256);
    }

    // This should trigger replacement
    auto result = vm->read(4 * 256);
    ASSERT_TRUE(result.success);

    auto stats = vm->getStats();
    EXPECT_EQ(stats.page_faults, 5);
}
