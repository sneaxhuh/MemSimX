#include <gtest/gtest.h>
#include "memory/physical_memory.h"
#include <cstring>

using namespace memsim;

class PhysicalMemoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a 1KB memory for testing
        memory = std::make_unique<PhysicalMemory>(1024);
    }

    void TearDown() override {
        memory.reset();
    }

    std::unique_ptr<PhysicalMemory> memory;
};

// Test basic construction
TEST_F(PhysicalMemoryTest, Construction) {
    EXPECT_EQ(memory->getTotalSize(), 1024);
    EXPECT_EQ(memory->getUsedSize(), 0);
    EXPECT_EQ(memory->getFreeSize(), 1024);
}

// Test write and read operations
TEST_F(PhysicalMemoryTest, WriteAndRead) {
    uint32_t test_value = 0xDEADBEEF;

    // Write value to address 0
    EXPECT_TRUE(memory->write(0, &test_value, sizeof(test_value)));

    // Read it back
    uint32_t read_value = 0;
    EXPECT_TRUE(memory->read(0, &read_value, sizeof(read_value)));

    EXPECT_EQ(read_value, test_value);
}

// Test write and read at different offsets
TEST_F(PhysicalMemoryTest, WriteAndReadAtOffset) {
    uint64_t test_value1 = 0x123456789ABCDEF0;
    uint64_t test_value2 = 0xFEDCBA9876543210;

    // Write at offset 100
    EXPECT_TRUE(memory->write(100, &test_value1, sizeof(test_value1)));

    // Write at offset 500
    EXPECT_TRUE(memory->write(500, &test_value2, sizeof(test_value2)));

    // Read them back
    uint64_t read_value1 = 0, read_value2 = 0;
    EXPECT_TRUE(memory->read(100, &read_value1, sizeof(read_value1)));
    EXPECT_TRUE(memory->read(500, &read_value2, sizeof(read_value2)));

    EXPECT_EQ(read_value1, test_value1);
    EXPECT_EQ(read_value2, test_value2);
}

// Test writing a string
TEST_F(PhysicalMemoryTest, WriteAndReadString) {
    const char* test_str = "Hello, Memory Simulator!";
    size_t str_len = strlen(test_str) + 1; // Include null terminator

    EXPECT_TRUE(memory->write(0, test_str, str_len));

    char buffer[50] = {0};
    EXPECT_TRUE(memory->read(0, buffer, str_len));

    EXPECT_STREQ(buffer, test_str);
}

// Test writing an array
TEST_F(PhysicalMemoryTest, WriteAndReadArray) {
    int test_array[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    size_t array_size = sizeof(test_array);

    EXPECT_TRUE(memory->write(0, test_array, array_size));

    int read_array[10] = {0};
    EXPECT_TRUE(memory->read(0, read_array, array_size));

    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(read_array[i], test_array[i]);
    }
}

// Test out of bounds write
TEST_F(PhysicalMemoryTest, OutOfBoundsWrite) {
    uint32_t test_value = 0x12345678;

    // Try to write beyond memory bounds
    EXPECT_FALSE(memory->write(1024, &test_value, sizeof(test_value)));
    EXPECT_FALSE(memory->write(1021, &test_value, sizeof(test_value))); // Partially out of bounds
    EXPECT_FALSE(memory->write(2000, &test_value, sizeof(test_value))); // Way out of bounds
}

// Test out of bounds read
TEST_F(PhysicalMemoryTest, OutOfBoundsRead) {
    uint32_t buffer = 0;

    // Try to read beyond memory bounds
    EXPECT_FALSE(memory->read(1024, &buffer, sizeof(buffer)));
    EXPECT_FALSE(memory->read(1021, &buffer, sizeof(buffer))); // Partially out of bounds
    EXPECT_FALSE(memory->read(2000, &buffer, sizeof(buffer))); // Way out of bounds
}

// Test edge case: read/write at exact boundary
TEST_F(PhysicalMemoryTest, BoundaryAccess) {
    uint32_t test_value = 0xABCDEF01;

    // Write at the last valid position (1024 - 4 = 1020)
    EXPECT_TRUE(memory->write(1020, &test_value, sizeof(test_value)));

    uint32_t read_value = 0;
    EXPECT_TRUE(memory->read(1020, &read_value, sizeof(read_value)));

    EXPECT_EQ(read_value, test_value);
}

// Test zero-size operations
TEST_F(PhysicalMemoryTest, ZeroSizeOperations) {
    // Zero-size operations should succeed (no-op)
    EXPECT_TRUE(memory->write(0, nullptr, 0));
    EXPECT_TRUE(memory->read(0, nullptr, 0));
    EXPECT_TRUE(memory->write(1000, nullptr, 0));
}

// Test clear operation
TEST_F(PhysicalMemoryTest, Clear) {
    // Write some data
    uint32_t test_value = 0xDEADBEEF;
    memory->write(0, &test_value, sizeof(test_value));
    memory->write(500, &test_value, sizeof(test_value));

    // Update used size
    memory->updateUsedSize(100);
    EXPECT_EQ(memory->getUsedSize(), 100);

    // Clear memory
    memory->clear();

    // Check that memory is zeroed
    uint32_t read_value = 0xFFFFFFFF;
    memory->read(0, &read_value, sizeof(read_value));
    EXPECT_EQ(read_value, 0);

    memory->read(500, &read_value, sizeof(read_value));
    EXPECT_EQ(read_value, 0);

    // Check that used size is reset
    EXPECT_EQ(memory->getUsedSize(), 0);
}

// Test valid range checking
TEST_F(PhysicalMemoryTest, IsValidRange) {
    EXPECT_TRUE(memory->isValidRange(0, 100));
    EXPECT_TRUE(memory->isValidRange(0, 1024));
    EXPECT_TRUE(memory->isValidRange(500, 524));
    EXPECT_TRUE(memory->isValidRange(1023, 1));

    EXPECT_FALSE(memory->isValidRange(1024, 1));
    EXPECT_FALSE(memory->isValidRange(500, 525));
    EXPECT_FALSE(memory->isValidRange(1020, 5));
}

// Test used size tracking
TEST_F(PhysicalMemoryTest, UsedSizeTracking) {
    EXPECT_EQ(memory->getUsedSize(), 0);
    EXPECT_EQ(memory->getFreeSize(), 1024);

    memory->updateUsedSize(256);
    EXPECT_EQ(memory->getUsedSize(), 256);
    EXPECT_EQ(memory->getFreeSize(), 768);

    memory->updateUsedSize(512);
    EXPECT_EQ(memory->getUsedSize(), 512);
    EXPECT_EQ(memory->getFreeSize(), 512);

    memory->updateUsedSize(0);
    EXPECT_EQ(memory->getUsedSize(), 0);
    EXPECT_EQ(memory->getFreeSize(), 1024);
}

// Test large memory allocation
// Test small memory allocation
TEST(PhysicalMemorySmallTest, SmallMemory) {
    // Create very small memory (64 bytes)
    PhysicalMemory small_memory(64);

    EXPECT_EQ(small_memory.getTotalSize(), 64);

    uint32_t test_value = 0xABCDEF01;
    EXPECT_TRUE(small_memory.write(0, &test_value, sizeof(test_value)));
    EXPECT_TRUE(small_memory.write(60, &test_value, sizeof(test_value)));

    // Should fail - out of bounds
    EXPECT_FALSE(small_memory.write(61, &test_value, sizeof(test_value)));
    EXPECT_FALSE(small_memory.write(64, &test_value, sizeof(test_value)));
}
