 // File: tests/test_memory_manager.cpp
 #include "gtest/gtest.h"
 #include "bdi/runtime/MemoryManager.hpp" // Adjust path as needed
 using namespace bdi::runtime;
 TEST(MemoryManagerTest, Initialization) {
    ASSERT_NO_THROW(MemoryManager manager(1024));
    EXPECT_THROW(MemoryManager manager(0), std::invalid_argument); // Size 0 should fail
 }
 TEST(MemoryManagerTest, BasicAllocation) {
    MemoryManager manager(1024);
    auto region1_id_opt = manager.allocateRegion(100);
    ASSERT_TRUE(region1_id_opt.has_value());
    EXPECT_NE(region1_id_opt.value(), 0); // ID should not be 0
    auto region1_info = manager.getRegionInfo(region1_id_opt.value());
    ASSERT_TRUE(region1_info.has_value());
    EXPECT_EQ(region1_info.value().id, region1_id_opt.value());
    EXPECT_EQ(region1_info.value().size, 100);
    EXPECT_GE(region1_info.value().base_address, 0); // Should be at offset 0 for first alloc
    EXPECT_FALSE(region1_info.value().read_only);
    auto region2_id_opt = manager.allocateRegion(200, true); // Read-only
    ASSERT_TRUE(region2_id_opt.has_value());
    auto region2_info = manager.getRegionInfo(region2_id_opt.value());
    ASSERT_TRUE(region2_info.has_value());
    EXPECT_EQ(region2_info.value().size, 200);
    EXPECT_GE(region2_info.value().base_address, region1_info.value().base_address + region1_info.value().size);
    EXPECT_TRUE(region2_info.value().read_only);
 }
 TEST(MemoryManagerTest, OutOfMemory) {
    MemoryManager manager(100);
    auto region1_id_opt = manager.allocateRegion(80);
    ASSERT_TRUE(region1_id_opt.has_value());
    auto region2_id_opt = manager.allocateRegion(30); // Should fail
    EXPECT_FALSE(region2_id_opt.has_value());
 }
 TEST(MemoryManagerTest, ReadWriteValid) {
    MemoryManager manager(1024);
    auto region_id_opt = manager.allocateRegion(10);
    ASSERT_TRUE(region_id_opt.has_value());
    auto region_info = manager.getRegionInfo(region_id_opt.value());
    ASSERT_TRUE(region_info.has_value());
    uintptr_t base_addr = region_info.value().base_address;
    // Write data
    std::vector<std::byte> write_buffer = {std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE}, std::byte{0xEF}};
    ASSERT_TRUE(manager.writeMemory(base_addr + 2, write_buffer.data(), write_buffer.size()));
    // Read data back
    std::vector<std::byte> read_buffer(4);
    ASSERT_TRUE(manager.readMemory(base_addr + 2, read_buffer.data(), read_buffer.size()));
    EXPECT_EQ(read_buffer[0], std::byte{0xDE});
    EXPECT_EQ(read_buffer[1], std::byte{0xAD});
    EXPECT_EQ(read_buffer[2], std::byte{0xBE});
    EXPECT_EQ(read_buffer[3], std::byte{0xEF});
 }
 TEST(MemoryManagerTest, ReadWriteOutOfBounds) {
    MemoryManager manager(100);
    auto region_id_opt = manager.allocateRegion(10);
    ASSERT_TRUE(region_id_opt.has_value());
    auto region_info = manager.getRegionInfo(region_id_opt.value());
    ASSERT_TRUE(region_info.has_value());
 std::vector<std::byte> buffer = {std::byte{0x01}};
    // Read partially out of bounds
    EXPECT_FALSE(manager.readMemory(base_addr + 5, buffer.data(), 10)); // Tries to read 10 bytes from offset 5 in a 10 byte region
    // Read completely out of bounds (start)
    EXPECT_FALSE(manager.readMemory(base_addr + 10, buffer.data(), 1));
    // Read completely out of bounds (total size)
    EXPECT_FALSE(manager.readMemory(95, buffer.data(), 10));
     // Write partially out of bounds
    EXPECT_FALSE(manager.writeMemory(base_addr + 5, buffer.data(), 10));
    // Write completely out of bounds (start)
    EXPECT_FALSE(manager.writeMemory(base_addr + 10, buffer.data(), 1));
     // Write completely out of bounds (total size)
    EXPECT_FALSE(manager.writeMemory(95, buffer.data(), 10));
 }
 TEST(MemoryManagerTest, FreeRegionStub) {
     MemoryManager manager(1024);
    auto region_id_opt = manager.allocateRegion(100);
    ASSERT_TRUE(region_id_opt.has_value());
    RegionID id = region_id_opt.value();
    EXPECT_TRUE(manager.getRegionInfo(id).has_value());
    EXPECT_TRUE(manager.freeRegion(id)); // Currently only checks if ID exists
    EXPECT_FALSE(manager.getRegionInfo(id).has_value()); // Region info should be gone
    EXPECT_FALSE(manager.freeRegion(id)); // Freeing again should fail
 }
    uintptr_t base_addr = region_info.value().base_address;
