#include "server.h"
#include <chrono>
#include <filesystem>
#include <iostream>  // For std::cerr and std::cout
#include <algorithm> // For std::sort
#include <cstring>   // For memset

using namespace std;
namespace fs = std::filesystem; // For filesystem operations

/**
 * @brief Constructs a new Server object with a specified address and port.
 * @param address The network address the server will listen on.
 * @param port The port number the server will listen on.
 */
Server::Server(const string &address, int port) : _server_fd(-1),
                                                  _new_socket(-1),
                                                  _addrlen(sizeof(_address)),
                                                  _server_address(address),
                                                  _port(port),
                                                  storage(nullptr)
{
    memset(&_address, 0, _addrlen);

    // Create socket file descriptor
    if ((_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to reuse address and port
    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = inet_addr(address.c_str());
    _address.sin_port = htons(port);

    // Bind the socket to the specified IP and port
    if (::bind(_server_fd, (struct sockaddr *)&_address, _addrlen) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    this->storage = new Storage(this); // Initialize Storage with a pointer to this Server instance
    // RequestHandler is no longer used, removed initialization
}

/**
 * @brief Destructor for the Server class, cleans up allocated resources.
 */
Server::~Server()
{
    delete this->storage;
    // RequestHandler is no longer used, removed deletion
    shutdown(); // Ensure socket is closed on destruction
}

/**
 * @brief Starts the server, making it ready to accept client connections.
 */
void Server::start()
{
    // Listen for incoming connections
    if (listen(_server_fd, 3) < 0) // Max 3 pending connections
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    cout << "Server listening on " << _server_address << ":" << _port << endl;

    while (true)
    {
        cout << "\nWaiting for a connection..." << endl;
        if ((_new_socket = accept(_server_fd, (struct sockaddr *)&_address, (socklen_t *)&_addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        char buffer[1024] = {0};
        long valread = read(_new_socket, buffer, 1024);
        if (valread < 0)
        {
            perror("read");
            close(_new_socket);
            continue;
        }
        string request_str(buffer, valread);
        cout << "Received request: " << request_str << endl;

        Request req = Request::deserialize(request_str);
        Response res;

        switch (req.type)
        {
        case RequestType::GET:
        {
            string value = get(req.key);
            if (!value.empty())
            {
                res = Response(true, "VALUE", value);
            }
            else
            {
                res = Response(false, "Key not found: " + req.key);
            }
            break;
        }
        case RequestType::PUT:
        {
            bool success = put(req.key, req.value);
            if (success)
            {
                res = Response(true, "OK");
            }
            else
            {
                res = Response(false, "Failed to put key: " + req.key);
            }
            break;
        }
        default:
            res = Response(false, "Unknown request type");
            break;
        }

        string response_str = res.serialize();
        send(_new_socket, response_str.c_str(), response_str.length(), 0);
        cout << "Sent response: " << response_str << endl;
        close(_new_socket);
    }
}

/**
 * @brief Shuts down the server, gracefully closing connections and releasing resources.
 */
void Server::shutdown()
{
    cout << "Server shutting down..." << endl;
    if (_server_fd != -1)
    {
        close(_server_fd);
        _server_fd = -1;
    }
}

/**
 * @brief Stores a key-value pair in the database.
 * @param key The key to store.
 * @param payload The value associated with the key.
 * @return True if the put operation was successful.
 */
bool Server::put(const string &key, const string &payload)
{
    cout << "putting key " << key << " to database with payload " << payload << endl;
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
    cout << "getting key " << key << " from database" << endl;
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
