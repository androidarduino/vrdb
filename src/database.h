#ifndef DATABASE_H
#define DATABASE_H

#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <optional>
#include <cstdint> // For uint64_t
using namespace std;

class SSTable; // Forward declaration

/**
 * @brief The MemTable class represents an in-memory key-value store.
 * It is responsible for temporarily storing data before flushing to disk as an SSTable.
 */
class MemTable
{
public:
    /**
     * @brief Flag indicating if the MemTable is read-only.
     * When true, no new writes are allowed, and it's awaiting flush.
     */
    bool readonly = false;
    /**
     * @brief The maximum number of key-value pairs this MemTable can hold before triggering a flush.
     */
    long max_size = 1000000;

    /**
     * @brief Constructs a new MemTable object.
     */
    MemTable();

    /**
     * @brief Retrieves the value associated with a given key.
     * @param key The key to look up.
     * @return The value associated with the key, or an empty string if the key is not found.
     */
    string get(const string &) const;

    /**
     * @brief Inserts or updates a key-value pair in the MemTable.
     * @param key The key to insert or update.
     * @param payload The value to associate with the key.
     * @return True if the operation was successful.
     */
    bool put(const string &, const string &);

    /**
     * @brief Flushes the current contents of the MemTable to a new SSTable file on disk.
     * @param filename The name of the file to create for the SSTable.
     * @return A pointer to the newly created SSTable object.
     */
    SSTable *flush(const string &filename);

    /**
     * @brief Checks if the MemTable has exceeded its maximum size.
     * @return True if the number of entries is greater than or equal to max_size, false otherwise.
     */
    bool oversize() { return data.size() >= static_cast<std::map<string, string>::size_type>(max_size); }

    /**
     * @brief Clears all key-value pairs from the MemTable.
     */
    void clear();

    /**
     * @brief Checks if the MemTable is empty.
     * @return True if the MemTable contains no key-value pairs, false otherwise.
     */
    bool is_empty() const { return data.empty(); }

    /**
     * @brief Calculates the approximate size in bytes of the data currently in the MemTable.
     * This is based on the sum of key and value string lengths.
     * @return The approximate size of the MemTable data in bytes.
     */
    long long get_size_bytes() const;

private:
    map<string, string> data;
    string _storage_path;
};

/**
 * @brief Represents a single key-value pair.
 */
struct KeyValuePair
{
    string key;
    string value;

    // Constructor for implicit conversion from std::pair
    KeyValuePair(const std::pair<std::string, std::string> &p) : key(p.first), value(p.second) {}

    // Default constructor
    KeyValuePair() : key(""), value("") {}

    // Existing constructor
    KeyValuePair(const std::string &k, const std::string &v) : key(k), value(v) {}

    /**
     * @brief Comparison operator for sorting KeyValuePair objects based on their keys.
     * @param other The other KeyValuePair to compare against.
     * @return True if this key is less than the other key, false otherwise.
     */
    bool operator<(const KeyValuePair &other) const
    {
        return key < other.key;
    }
};

/**
 * @brief A simplified implementation of a Sorted String Table (SSTable).
 * SSTables are immutable files on disk that store sorted key-value pairs.
 */
class SSTable
{
public:
    /**
     * @brief Constructs a new SSTable object.
     * If loadData is true, the constructor loads the entire SSTable data from the specified file into memory.
     * Otherwise, it only stores the filePath and relies on the find method for disk lookups.
     * @param filePath The path to the file where the SSTable is/will be stored.
     * @param loadData If true, loads all key-value pairs into the in-memory data map.
     */
    explicit SSTable(const string &filePath, bool loadData = false);

    /**
     * @brief Retrieves the value associated with a given key from the SSTable.
     * If the SSTable was loaded with all data into memory (loadData = true), it searches the in-memory map.
     * Otherwise, it performs a disk-based lookup using the sparse index.
     * @param key The key to look up.
     * @return The value associated with the key, or an empty string if the key is not found.
     */
    string get(const string &key);

    /**
     * @brief Inserts or updates a key-value pair in the in-memory representation of the SSTable.
     * Note: This method does not persist changes to disk immediately. It's primarily for merging operations.
     * @param key The key to insert or update.
     * @param value The value to associate with the key.
     * @return True if the operation was successful.
     */
    bool put(const string &key, const string &value);

    /**
     * @brief Retrieves the smallest key currently present in the in-memory SSTable data.
     * @return The smallest key, or an empty string if the SSTable is empty.
     */
    string get_first_key() const;

    /**
     * @brief Removes and returns the key-value pair with the smallest key from the in-memory SSTable data.
     * @return A pair containing the smallest key and its value, or {"", ""} if the SSTable is empty.
     */
    pair<string, string> pop_first_item();

    /**
     * @brief Returns a vector of all key-value pairs currently in the in-memory data map.
     * This is primarily for the CLI tool to view contents or for preparing data for writing.
     * @return A vector of KeyValuePair objects.
     */
    std::vector<KeyValuePair> getAllKeyValues() const;

    /**
     * @brief Writes a vector of sorted key-value pairs (representing a Memtable) to the file.
     * This method is used when flushing a MemTable to disk.
     * @param memtable A vector of KeyValuePair objects to write to the file.
     * @return True on success, false on failure.
     */
    bool writeFromMemory(const std::vector<KeyValuePair> &memtable);

    /**
     * @brief Finds a value for a given key by searching the SSTable file.
     * @param key The key to search for.
     * @return An optional<string> which is empty if the key is not found, otherwise contains the value.
     */
    std::optional<std::string> find(const std::string &key);

    /**
     * @brief Returns the full file path of this SSTable.
     * @return The file path as a string.
     */
    std::string get_filename() const; // New method declaration

private:
    std::string filePath;
    map<string, string> data;
    // The in-memory representation of the SSTable's index.
    // Maps the first key of a data block to the offset of that block in the file.
    std::map<std::string, uint64_t> sparseIndex;

    /**
     * @brief Loads the sparse index from the SSTable file into memory.
     * This is called automatically by find() if the index isn't already loaded.
     * @return True if the index was loaded successfully, false otherwise.
     */
    bool loadIndex();

    // Helper functions for serializing/deserializing data to/from the file.
    /**
     * @brief Writes a string to an output filestream.
     * @param out The output filestream.
     * @param str The string to write.
     */
    void writeString(std::ofstream &out, const std::string &str);

    /**
     * @brief Reads a string from an input filestream.
     * @param in The input filestream.
     * @return The string read from the stream.
     */
    std::string readString(std::ifstream &in);

    /**
     * @brief Writes a uint64_t value to an output filestream.
     * @param out The output filestream.
     * @param value The uint64_t value to write.
     */
    void writeUint64(std::ofstream &out, uint64_t value);

    /**
     * @brief Reads a uint64_t value from an input filestream.
     * @param in The input filestream.
     * @return The uint64_t value read from the stream.
     */
    uint64_t readUint64(std::ifstream &in);

    /**
     * @brief Placeholder method to get the key position within the file. Not fully implemented for in-memory SSTable.
     * @param key The key to find.
     * @param offset Output parameter for the offset of the key.
     * @param length Output parameter for the length of the key's value.
     */
    void get_key_pos(const string &key, int &offset, int &length);

    /**
     * @brief Placeholder method to get the value by position within the file. Not fully implemented for in-memory SSTable.
     * @param offset The offset of the value.
     * @param length The length of the value.
     * @return The value string.
     */
    string get_value_by_pos(const int offset, const int length);
};

#endif // DATABASE_H