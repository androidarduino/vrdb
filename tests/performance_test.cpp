#include "../src/server.h"
#include "../src/database.h"
#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <filesystem>
#include <random>

namespace fs = std::filesystem;

// Helper to clean up test files
void cleanup_performance_files()
{
    for (const auto &entry : fs::directory_iterator(fs::current_path()))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".sst")
        {
            fs::remove(entry.path());
        }
    }
}

// Function to generate random string keys and values
std::string generate_random_string(size_t length)
{
    const std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, characters.size() - 1);

    std::string random_string;
    for (size_t i = 0; i < length; ++i)
    {
        random_string += characters[distribution(generator)];
    }
    return random_string;
}

int main()
{
    std::cout << "Starting performance test..." << std::endl;
    // cleanup_performance_files(); // Clean up from previous runs

    Server server("127.0.0.1", 8080); // Dummy server

    // Configure MemTable and Storage for performance test
    server.storage->main_mdb->max_size = 1000; // Small max_size to trigger frequent flushes
    const int num_items_to_insert = 10000;     // Insert many items
    const size_t key_length = 16;
    const size_t value_length = 64;

    auto total_start_time = chrono::high_resolution_clock::now();

    // Insert items and trigger flushes
    for (int i = 0; i < num_items_to_insert; ++i)
    {
        std::string key = generate_random_string(key_length);
        std::string value = generate_random_string(value_length);
        server.put(key, value);
        // check_for_compaction is now triggered inside Server::put
    }

    // Trigger merge if there are tables to merge
    if (!server.storage->tables_to_merge.empty())
    {
        std::cout << "Triggering merge operation..." << std::endl;
        server.storage->merge();
    }

    auto total_end_time = chrono::high_resolution_clock::now();
    std::chrono::duration<double> total_execution_time_seconds = total_end_time - total_start_time;

    // Report performance metrics
    std::cout << "\nPerformance Metrics:" << std::endl;
    std::cout << "----------------------" << std::endl;
    std::cout << "Total items inserted: " << num_items_to_insert << std::endl;
    std::cout << "Total execution time: " << total_execution_time_seconds.count() << " seconds" << std::endl;
    std::cout << "Operations per second: " << (double)num_items_to_insert / total_execution_time_seconds.count() << std::endl;
    std::cout << "Time to flush data into disk: " << server.storage->get_flush_time_ns() / 1e9 << " seconds" << std::endl;
    std::cout << "Time to compact SST files:    " << server.storage->get_merge_time_ns() / 1e9 << " seconds" << std::endl;
    std::cout << "Data amount flushed:          " << server.storage->get_flush_bytes_operated() << " bytes" << std::endl;
    std::cout << "Data amount compacted:        " << server.storage->get_merge_bytes_operated() << " bytes" << std::endl;

    // cleanup_performance_files(); // Commented out to keep SST files for CLI tool testing
    std::cout << "Performance test completed." << std::endl;
    return 0;
}
