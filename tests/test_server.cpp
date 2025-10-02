#include "../src/server.h"
#include "../src/database.h"
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>    // For getCurrentUnixTimeString
#include <algorithm> // For std::sort

// Simple assertion macros
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

TEST(Server_put_get)
{
    Server server("127.0.0.1", 8080); // Dummy server instance
    server.put("server_key1", "server_value1");
    ASSERT_EQ(std::string("server_value1"), server.get("server_key1"), "Server::put/get failed for server_key1");
    server.put("server_key2", "server_value2");
    ASSERT_EQ(std::string("server_value2"), server.get("server_key2"), "Server::put/get failed for server_key2");
    ASSERT_EQ(std::string(""), server.get("nonexistent_server_key"), "Server::get for nonexistent key should return empty string");
}
END_TEST

TEST(Storage_check_for_compaction)
{
    // cleanup_test_files(); // Commented out to preserve SST files
    Server server("127.0.0.1", 8081); // Dummy server instance
    // Storage storage(&server); // This is now created by Server constructor

    server.storage->main_mdb->max_size = 2;
    server.put("k1", "v1");
    server.put("k2", "v2");

    // Compaction should have been triggered by the second put
    // We check its side effects directly
    ASSERT_TRUE(server.storage->main_mdb->is_empty(), "main_mdb should be empty after compaction");
    ASSERT_TRUE(server.storage->second_mdb->readonly, "second_mdb should be readonly after compaction");
    ASSERT_TRUE(server.storage->tables_to_merge.size() == 1, "tables_to_merge should contain one flushed SSTable");
    ASSERT_TRUE(fs::exists(server.storage->tables_to_merge[0]), "Flushed SSTable file should exist");
    // cleanup_test_files(); // Commented out to preserve SST files
}
END_TEST

TEST(Storage_merge)
{
    // cleanup_test_files(); // Commented out to preserve SST files
    Server server("127.0.0.1", 8082); // Dummy server instance

    // Create two SSTables to merge
    MemTable mt1;
    mt1.put("apple", "A");
    mt1.put("banana", "B");
    SSTable *sst1 = mt1.flush("test_merge_1.sst");

    MemTable mt2;
    mt2.put("cherry", "C");
    mt2.put("date", "D");
    SSTable *sst2 = mt2.flush("test_merge_2.sst");

    ASSERT_TRUE(fs::exists("test_merge_1.sst"), "test_merge_1.sst should exist");
    ASSERT_TRUE(fs::exists("test_merge_2.sst"), "test_merge_2.sst should exist");

    server.storage->tables_to_merge.push_back("test_merge_1.sst");
    server.storage->tables_to_merge.push_back("test_merge_2.sst");

    server.storage->merge();

    ASSERT_TRUE(server.storage->tables_to_merge.size() == 1, "After merge, tables_to_merge should have one entry");
    std::string merged_sst_name = server.storage->tables_to_merge[0];
    ASSERT_TRUE(fs::exists(merged_sst_name), "Merged SSTable file should exist");
    ASSERT_TRUE(!fs::exists("test_merge_1.sst"), "Original test_merge_1.sst should be removed");
    ASSERT_TRUE(!fs::exists("test_merge_2.sst"), "Original test_merge_2.sst should be removed");

    SSTable loaded_merged_sst(merged_sst_name, true);
    ASSERT_EQ(std::string("A"), loaded_merged_sst.get("apple"), "Merged SSTable content for apple incorrect");
    ASSERT_EQ(std::string("B"), loaded_merged_sst.get("banana"), "Merged SSTable content for banana incorrect");
    ASSERT_EQ(std::string("C"), loaded_merged_sst.get("cherry"), "Merged SSTable content for cherry incorrect");
    ASSERT_EQ(std::string("D"), loaded_merged_sst.get("date"), "Merged SSTable content for date incorrect");
    ASSERT_EQ(std::string(""), loaded_merged_sst.get("nonexistent"), "Merged SSTable content for nonexistent key incorrect");

    delete sst1;
    delete sst2;
    // cleanup_test_files(); // Commented out to preserve SST files
}
END_TEST

int main()
{
    std::cout << "Running all server tests..." << std::endl;
    RUN_TEST(Server_put_get);
    RUN_TEST(Storage_check_for_compaction);
    RUN_TEST(Storage_merge);
    std::cout << "All server tests passed!" << std::endl;
    return 0;
}
