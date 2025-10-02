#include "server.h"
#include <chrono>
#include <filesystem>
#include <iostream>  // For std::cerr
#include <algorithm> // For std::sort
using namespace std;
namespace fs = std::filesystem; // For filesystem operations

/**
 * @brief Constructs a new Server object with a specified address and port.
 * @param address The network address the server will listen on.
 * @param port The port number the server will listen on.
 */
Server::Server(const string &address, int port)
{
    // todo: create listening service
    _port = port;
    this->storage = new Storage(this); // Initialize Storage with a pointer to this Server instance
    this->rh = new RequestHandler();   // Assuming RequestHandler is implemented elsewhere
}

/**
 * @brief Destructor for the Server class, cleans up allocated resources.
 */
Server::~Server()
{
    delete this->storage;
    delete this->rh;
}

/**
 * @brief Starts the server, making it ready to accept client connections.
 */
void Server::start()
{
    printf("Server starting\n");
}

/**
 * @brief Shuts down the server, gracefully closing connections and releasing resources.
 */
void Server::shutdown()
{
    printf("Server shutting down\n");
}

/**
 * @brief Stores a key-value pair in the database.
 * @param key The key to store.
 * @param payload The value associated with the key.
 * @return True if the put operation was successful.
 */
bool Server::put(const string &key, const string &payload)
{
    printf("putting key %s to database with payload %s\n", key.c_str(), payload.c_str());
    this->storage->main_mdb->put(key, payload);
    this->storage->check_for_compaction(); // Trigger compaction check after each put
    return true;
}

/**
 * @brief Retrieves the value associated with a given key from the database.
 * @param key The key to retrieve.
 * @return The value associated with the key, or an empty string if the key is not found.
 */
string Server::get(const string &key)
{
    printf("getting key %s from database\n", key.c_str());
    // First, check main_mdb, then second_mdb, then SSTables on disk
    string value = this->storage->main_mdb->get(key);
    if (!value.empty())
    {
        return value;
    }
    value = this->storage->second_mdb->get(key);
    if (!value.empty())
    {
        return value;
    }
    // If not found in MemTables, check the most recent SSTable (this->sst)
    if (this->storage->sst)
    {
        auto result = this->storage->sst->find(key);
        if (result.has_value())
        {
            return result.value();
        }
    }
    // Iterate through tables_to_merge (older SSTables) if not found in current sst
    for (const string &filename : this->storage->tables_to_merge)
    {
        SSTable temp_sst(filename, true);
        auto result = temp_sst.find(key);
        if (result.has_value())
        {
            return result.value();
        }
    }
    return ""; // Key not found
}

/**
 * @brief Generates a unique Unix timestamp string.
 * @return A string representation of the current Unix timestamp.
 */
string getCurrentUnixTimeString()
{
    auto now = chrono::system_clock::now();
    auto time_since_epoch = now.time_since_epoch();
    auto seconds = chrono::duration_cast<std::chrono::seconds>(time_since_epoch).count();
    return to_string(seconds);
}

/**
 * @brief Constructs a new Storage object.
 * @param server A pointer to the Server instance associated with this storage.
 */
Storage::Storage(Server *server) : main_mdb(new MemTable()), second_mdb(new MemTable()), sst(nullptr), merging(false) {}

/**
 * @brief Checks if compaction (flushing MemTable to SSTable or merging SSTables) is needed.
 * If the main MemTable is oversized and no merge is in progress, it triggers a flush.
 * @return True if a compaction was initiated, false otherwise.
 */
bool Storage::check_for_compaction()
{
    if (this->main_mdb->oversize() && !this->merging)
    {
        auto start_time = chrono::high_resolution_clock::now();
        long long bytes_flushed = this->main_mdb->get_size_bytes();

        // swap the two in memory tables
        this->main_mdb->readonly = true;
        this->second_mdb->readonly = false;
        this->second_mdb->clear();
        auto tmp = this->main_mdb;
        this->main_mdb = this->second_mdb;
        this->second_mdb = tmp;
        // persist the main table to disk
        string flushed_filename = getCurrentUnixTimeString() + ".sst";
        SSTable *flushed_sst = this->second_mdb->flush(flushed_filename);
        if (flushed_sst)
        {
            this->tables_to_merge.push_back(flushed_filename);
            delete flushed_sst; // Flush returns a new SSTable, which is not owned by Storage
        }
        else
        {
            std::cerr << "Error: Failed to flush MemTable to disk." << std::endl;
            // Consider reverting swap or other error handling
        }

        auto end_time = chrono::high_resolution_clock::now();
        this->flush_time_ns += chrono::duration_cast<chrono::nanoseconds>(end_time - start_time).count();
        this->flush_bytes_operated += bytes_flushed;

        return true;
    }
    return false;
}

/**
 * @brief Merges the SSTables listed in tables_to_merge into a new, consolidated SSTable.
 * This process reads data from multiple SSTables, merges them, and writes to a new file.
 * Old SSTables are then deleted, and the new merged SSTable is added to the merge list for future consideration.
 */
void Storage::merge()
{
    if (this->tables_to_merge.empty())
    {
        return;
    }
    this->merging = true;

    auto start_time = chrono::high_resolution_clock::now();
    long long bytes_merged = 0;

    vector<SSTable *> to_merge;
    vector<string> to_merge_names;

    for (const string &filename : this->tables_to_merge)
    {
        // Load SSTable data into memory for merging
        SSTable *sst_to_load = new SSTable(filename, true);
        to_merge.push_back(sst_to_load);
        to_merge_names.push_back(filename);

        // Calculate bytes operated from input SSTables
        for (const auto &pair : sst_to_load->getAllKeyValues())
        {
            bytes_merged += pair.key.length();
            bytes_merged += pair.value.length();
        }
    }
    this->tables_to_merge.clear();

    // The new_main_sst_name will be the name of the new SSTable after merging and renaming
    string new_main_sst_name = getCurrentUnixTimeString() + ".sst";
    auto target_table = new SSTable(new_main_sst_name, false); // target_table will write to disk

    // merge the files into a new sst file
    SSTable *candidate;
    string min_key;

    std::vector<KeyValuePair> merged_data;

    while (true)
    {
        candidate = nullptr;
        min_key = ""; // Reset min_key for each iteration

        for (SSTable *sst_ptr : to_merge)
        {
            // Only consider SSTables that still have data
            if (!sst_ptr->get_first_key().empty())
            {
                if (candidate == nullptr || sst_ptr->get_first_key() < min_key)
                {
                    candidate = sst_ptr;
                    min_key = sst_ptr->get_first_key();
                }
            }
        }
        if (candidate == nullptr || min_key.empty())
        {
            break; // All SSTables are empty
        }
        auto record = candidate->pop_first_item();
        merged_data.push_back({record.first, record.second});
    }

    // Calculate bytes operated for output SSTable
    for (const auto &pair : merged_data)
    {
        bytes_merged += pair.key.length();
        bytes_merged += pair.value.length();
    }

    // Write the merged data to the target SSTable
    std::sort(merged_data.begin(), merged_data.end()); // Ensure data is sorted before writing
    if (!target_table->writeFromMemory(merged_data))
    {
        std::cerr << "Error: Failed to write merged SSTable to disk." << std::endl;
    }

    this->merging = false;

    // Delete all to_merge files and SSTable objects
    for (string filename : to_merge_names)
    {
        fs::remove(filename);
    }
    for (SSTable *sst_ptr : to_merge)
    {
        delete sst_ptr;
    }

    // Assign the new merged SSTable to the 'sst' member of Storage
    if (this->sst)
    {
        delete this->sst;
    }
    this->sst = target_table; // Storage now owns this SSTable

    // Add the newly created main.sst to the next merge list
    this->tables_to_merge.push_back(new_main_sst_name);

    auto end_time = chrono::high_resolution_clock::now();
    this->merge_time_ns += chrono::duration_cast<chrono::nanoseconds>(end_time - start_time).count();
    this->merge_bytes_operated += bytes_merged;
}
