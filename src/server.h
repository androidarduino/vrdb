#ifndef SERVER_H
#define SERVER_H

#include "request.h"
#include "database.h"
#include <stdio.h>
#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
#include <sys/socket.h> // For socket, bind, listen, accept
#include <netinet/in.h> // For sockaddr_in
#include <unistd.h>     // For close
#include <arpa/inet.h>  // For inet_ntoa
#include <signal.h>     // Required for signal handling

using namespace std;

class Server; // Forward declaration for Storage class

/**
 * @brief The Storage class manages in-memory tables (MemTables) and on-disk tables (SSTables),
 * handling compaction and merging processes.
 */
class Storage
{
    /**
     * @brief The maintainer checks every check_interval for two things:
     * 1. whether the memory table size is larger than threshold, if so, trigger a flush process
     * 2. whether it is a good time to merge on disk tables, if so, start a merging process
     */
public:
    /**
     * @brief Constructs a new Storage object.
     * @param server A pointer to the Server instance associated with this storage.
     */
    Storage(Server *server);

    /**
     * @brief Checks if compaction (flushing MemTable to SSTable or merging SSTables) is needed.
     * @return True if a compaction was initiated, false otherwise.
     */
    bool check_for_compaction();

    /**
     * @brief The interval (in some unit, e.g., seconds) at which the storage maintainer checks for compaction.
     */
    float check_interval = 0.1f;

    MemTable *main_mdb;
    MemTable *second_mdb;
    SSTable *sst;

public: // Changed for testing purposes
    vector<string> tables_to_merge;
    /**
     * @brief Merges the SSTables listed in tables_to_merge into a new, consolidated SSTable.
     */
    void merge();

    // Performance metrics
    long long flush_time_ns = 0;
    long long merge_time_ns = 0;
    long long flush_bytes_operated = 0;
    long long merge_bytes_operated = 0;

    // Public accessors for metrics
    long long get_flush_time_ns() const { return flush_time_ns; }
    long long get_merge_time_ns() const { return merge_time_ns; }
    long long get_flush_bytes_operated() const { return flush_bytes_operated; }
    long long get_merge_bytes_operated() const { return merge_bytes_operated; }

    /**
     * @brief Flushes all in-memory MemTables (main and second) to disk as SSTables.
     * This method is typically called during server shutdown to ensure data persistence.
     */
    void flush_all_memtables_to_disk();

private:
    /**
     * @brief Flag indicating if a merge operation is currently in progress.
     * Prevents multiple concurrent merge operations.
     */
    bool merging;
};

/**
 * @brief The Server class represents the main server application,
 * handling client requests (put, get) and managing the underlying storage.
 */
class Server
{
public:
    /**
     * @brief Default constructor for the Server class.
     */
    Server() : storage(nullptr) {};

    /**
     * @brief Constructs a new Server object with a specified address and port.
     * @param address The network address the server will listen on.
     * @param port The port number the server will listen on.
     */
    Server(const string &, int);

    /**
     * @brief Starts the server, making it ready to accept client connections.
     */
    void start();

    /**
     * @brief Stores a key-value pair in the database.
     * @param key The key to store.
     * @param payload The value associated with the key.
     * @return True if the put operation was successful.
     */
    bool put(const string &, const string &);

    /**
     * @brief Retrieves the value associated with a given key from the database.
     * @param key The key to retrieve.
     * @return The value associated with the key, or an empty string if the key is not found.
     */
    string get(const string &);

    /**
     * @brief Shuts down the server, gracefully closing connections and releasing resources.
     */
    void shutdown();

    /**
     * @brief Destructor for the Server class, cleans up allocated resources.
     */
    ~Server();

    // Changed for testing purposes
private:
    static volatile sig_atomic_t _running; // Flag to control the server's main loop
    static Server *_instance;              // Static pointer to the Server instance

    // Static signal handler to allow C-style signal function to access Server members
    static void handle_signal(int signal);

    int _server_fd;              // Server socket file descriptor
    int _new_socket;             // New socket for accepted client connections
    struct sockaddr_in _address; // Server address structure
    string _server_address;      // Server address structure
    int _addrlen;
    int _port;

public: // Changed for testing purposes
    Storage *storage;

    // RequestHandler *rh; // Removed RequestHandler as it's no longer used
};

string getCurrentUnixTimeString();

#endif // SERVER_H
