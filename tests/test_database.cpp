#include "../src/database.h"
#include "../src/server.h" // For getCurrentUnixTimeString
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <map>
#include <algorithm> // For std::sort

// Simple assertion macro
#define ASSERT_EQ(expected, actual, message)                                          \
    if ((expected) != (actual))                                                       \
    {                                                                                 \
        std::cerr << "Assertion failed: " << message << std::endl;                    \
        std::cerr << "Expected: " << expected << ", Actual: " << actual << std::endl; \
        exit(1);                                                                      \
    }

#define ASSERT_TRUE(condition, message)                            \
    if (!(condition))                                              \
    {                                                              \
        std::cerr << "Assertion failed: " << message << std::endl; \
        exit(1);                                                   \
    }

#define TEST(name)     \
    void test_##name() \
    {                  \
        std::cout << "Running test: " << #name << std::endl;

#define END_TEST \
    }

#define RUN_TEST(name) \
    test_##name();

namespace fs = std::filesystem;

// Helper to clean up test files
void cleanup_test_files()
{
    for (const auto &entry : fs::directory_iterator(fs::current_path()))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".sst")
        {
            fs::remove(entry.path());
        }
    }
}

TEST(MemTable_put_get)
{
    MemTable mt;
    mt.put("key1", "value1");
    ASSERT_EQ(std::string("value1"), mt.get("key1"), "MemTable::put/get failed for key1");
    mt.put("key2", "value2");
    ASSERT_EQ(std::string("value2"), mt.get("key2"), "MemTable::put/get failed for key2");
    ASSERT_EQ(std::string(""), mt.get("nonexistent"), "MemTable::get for nonexistent key should return empty string");
}
END_TEST

TEST(MemTable_oversize_clear)
{
    MemTable mt;
    mt.max_size = 1; // Set a small max size for testing
    mt.put("k1", "v1");
    ASSERT_TRUE(mt.oversize(), "MemTable should be oversize after 1 item");

    mt.put("k2", "v2"); // This should not change oversize state if already oversize
    ASSERT_TRUE(mt.oversize(), "MemTable should remain oversize after adding more items");

    mt.clear();
    ASSERT_TRUE(mt.is_empty(), "MemTable should be empty after clear");
    ASSERT_TRUE(!mt.oversize(), "MemTable should not be oversize after clear");
}
END_TEST

TEST(MemTable_flush)
{
    // cleanup_test_files(); // Commented out to preserve SST files
    MemTable mt;
    mt.put("flushkey1", "flushvalue1");
    mt.put("flushkey2", "flushvalue2");
    SSTable *sst = mt.flush("test_flush.sst");
    ASSERT_TRUE(sst != nullptr, "MemTable::flush should return a valid SSTable pointer");
    ASSERT_TRUE(fs::exists("test_flush.sst"), "Flushed SSTable file should exist");
    ASSERT_TRUE(mt.is_empty(), "MemTable should be empty after flush");

    // Verify content of flushed SSTable
    SSTable loaded_sst("test_flush.sst", true);
    ASSERT_EQ(std::string("flushvalue1"), loaded_sst.get("flushkey1"), "Flushed SSTable content for flushkey1 incorrect");
    ASSERT_EQ(std::string("flushvalue2"), loaded_sst.get("flushkey2"), "Flushed SSTable content for flushkey2 incorrect");
    ASSERT_EQ(std::string(""), loaded_sst.get("nonexistent_flush_key"), "Flushed SSTable content for nonexistent key incorrect");

    delete sst;
    // cleanup_test_files(); // Commented out to preserve SST files
}
END_TEST

TEST(SSTable_writeFromMemory_find)
{
    // cleanup_test_files(); // Commented out to preserve SST files
    std::vector<KeyValuePair> data_to_write;
    data_to_write.push_back({"keyA", "valueA"});
    data_to_write.push_back({"keyB", "valueB"});
    data_to_write.push_back({"keyC", "valueC"});

    SSTable sst("test.sst", false); // false for loadData, as we are writing
    ASSERT_TRUE(sst.writeFromMemory(data_to_write), "SSTable::writeFromMemory failed");
    ASSERT_TRUE(fs::exists("test.sst"), "SSTable file should exist after writeFromMemory");

    // Now test find
    std::optional<std::string> valA = sst.find("keyA");
    ASSERT_TRUE(valA.has_value(), "SSTable::find failed to find keyA");
    ASSERT_EQ(std::string("valueA"), valA.value(), "SSTable::find returned incorrect value for keyA");

    std::optional<std::string> valC = sst.find("keyC");
    ASSERT_TRUE(valC.has_value(), "SSTable::find failed to find keyC");
    ASSERT_EQ(std::string("valueC"), valC.value(), "SSTable::find returned incorrect value for keyC");

    std::optional<std::string> valD = sst.find("keyD");
    ASSERT_TRUE(!valD.has_value(), "SSTable::find should not find nonexistent keyD");
    // cleanup_test_files(); // Commented out to preserve SST files
}
END_TEST

TEST(SSTable_get_disk_vs_memory)
{
    // cleanup_test_files(); // Commented out to preserve SST files
    std::vector<KeyValuePair> data_to_write;
    data_to_write.push_back({"disk_key1", "disk_value1"});
    data_to_write.push_back({"disk_key2", "disk_value2"});

    SSTable sst_disk_write("test_get_disk.sst", false);
    ASSERT_TRUE(sst_disk_write.writeFromMemory(data_to_write), "SSTable::writeFromMemory failed for disk_test");

    // Test get for disk-based SSTable (should use find)
    SSTable sst_disk_read("test_get_disk.sst", false); // loadData=false, will use find()
    ASSERT_EQ(std::string("disk_value1"), sst_disk_read.get("disk_key1"), "SSTable::get disk-read failed for disk_key1");
    ASSERT_EQ(std::string("disk_value2"), sst_disk_read.get("disk_key2"), "SSTable::get disk-read failed for disk_key2");
    ASSERT_EQ(std::string(""), sst_disk_read.get("nonexistent_disk_key"), "SSTable::get disk-read for nonexistent key should be empty");

    // Test get for in-memory SSTable (should use data map)
    SSTable sst_memory("dummy_file.sst", false); // No actual file needed, just a path for the object
    sst_memory.put("mem_key1", "mem_value1");
    sst_memory.put("mem_key2", "mem_value2");
    ASSERT_EQ(std::string("mem_value1"), sst_memory.get("mem_key1"), "SSTable::get memory-read failed for mem_key1");
    ASSERT_EQ(std::string("mem_value2"), sst_memory.get("mem_key2"), "SSTable::get memory-read failed for mem_key2");
    ASSERT_EQ(std::string(""), sst_memory.get("nonexistent_mem_key"), "SSTable::get memory-read for nonexistent key should be empty");
    // cleanup_test_files(); // Commented out to preserve SST files
}
END_TEST

TEST(SSTable_get_first_key_pop_first_item)
{
    // cleanup_test_files(); // Commented out to preserve SST files
    SSTable sst_merge_tmp("test_sstable_merge_operations.sst", false);
    sst_merge_tmp.put("apple", "A");
    sst_merge_tmp.put("banana", "B");
    sst_merge_tmp.put("cherry", "C");

    ASSERT_EQ(std::string("apple"), sst_merge_tmp.get_first_key(), "get_first_key should return 'apple'");
    KeyValuePair item1 = sst_merge_tmp.pop_first_item();
    ASSERT_EQ(std::string("apple"), item1.key, "pop_first_item should return 'apple' key");
    ASSERT_EQ(std::string("A"), item1.value, "pop_first_item should return 'A' value");

    ASSERT_EQ(std::string("banana"), sst_merge_tmp.get_first_key(), "get_first_key should return 'banana'");
    KeyValuePair item2 = sst_merge_tmp.pop_first_item();
    ASSERT_EQ(std::string("banana"), item2.key, "pop_first_item should return 'banana' key");
    ASSERT_EQ(std::string("B"), item2.value, "pop_first_item should return 'B' value");

    ASSERT_EQ(std::string("cherry"), sst_merge_tmp.get_first_key(), "get_first_key should return 'cherry'");
    sst_merge_tmp.pop_first_item();
    ASSERT_TRUE(sst_merge_tmp.get_first_key().empty(), "get_first_key should be empty after popping all items");
    // cleanup_test_files(); // Commented out to preserve SST files
}
END_TEST

int main()
{
    std::cout << "Running all database tests..." << std::endl;
    RUN_TEST(MemTable_put_get);
    RUN_TEST(MemTable_oversize_clear);
    RUN_TEST(MemTable_flush);
    RUN_TEST(SSTable_writeFromMemory_find);
    RUN_TEST(SSTable_get_disk_vs_memory);
    RUN_TEST(SSTable_get_first_key_pop_first_item);
    std::cout << "All database tests passed!" << std::endl;
    return 0;
}
