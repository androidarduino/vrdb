#include "database.h"
#include <fstream>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <filesystem>

const std::string DATADIR = "data/"; // Define DATADIR for use in this file

MemTable::MemTable()
{
    // _storage_path = "./"; // Removed as it's no longer used
}

SSTable *MemTable::flush(const string &filename)
{
    std::cerr << "MemTable::flush called for filename: " << filename << std::endl;
    std::vector<KeyValuePair> sorted_data;
    for (auto const &[key, val] : data)
    {
        sorted_data.push_back({key, val});
    }
    // SSTables require sorted data
    std::sort(sorted_data.begin(), sorted_data.end());

    std::cerr << "Flushing " << sorted_data.size() << " key-value pairs to SSTable." << std::endl;

    SSTable *new_sst = new SSTable(DATADIR + filename, false); // Use DATADIR
    if (!new_sst->writeFromMemory(sorted_data))
    {
        std::cerr << "Error: Failed to write MemTable to SSTable file: " << filename << std::endl;
        delete new_sst;
        return nullptr; // Handle error
    }
    this->clear(); // Clear the MemTable after successful flush
    return new_sst;
}

bool MemTable::put(const string &key, const string &payload)
{
    data[key] = payload;
    return true;
}

string MemTable::get(const string &key) const
{
    auto it = data.find(key);
    if (it != data.end())
    {
        return it->second;
    }
    printf("Key not found: %s", key.c_str());
    return "";
}

void MemTable::clear()
{
    data.clear();
}

long long MemTable::get_size_bytes() const
{
    long long size_bytes = 0;
    for (const auto &pair : data)
    {
        size_bytes += pair.first.length();
        size_bytes += pair.second.length();
    }
    return size_bytes;
}

// The number of key-value pairs to store in each data block.
// A smaller number means a larger index but smaller reads from disk.
const size_t BLOCK_SIZE = 4;

SSTable::SSTable(const std::string &filePath, bool loadData) : filePath(filePath) // Use DATADIR
{
    if (loadData)
    {
        // Load all data into memory for merge operations
        std::ifstream infile(this->filePath, std::ios::binary);
        if (!infile.is_open())
        {
            std::cerr << "Error: Could not open file for reading during loadData: " << filePath << std::endl;
            return; // Or throw an exception
        }

        // First, read the footer to find where the index starts
        infile.seekg(-static_cast<std::streamoff>(sizeof(uint64_t)), std::ios::end);
        uint64_t index_start_offset = this->readUint64(infile);

        // Now, seek back to the beginning of the data and read all blocks
        infile.seekg(0, std::ios::beg);

        while (static_cast<uint64_t>(infile.tellg()) < index_start_offset)
        {
            uint64_t pairsInBlock = this->readUint64(infile);
            for (uint64_t i = 0; i < pairsInBlock; ++i)
            {
                string key = this->readString(infile);
                string value = this->readString(infile);
                this->data[key] = value;
            }
        }
        infile.close();
    }
}

// Writes a string with its length prefixed for easy deserialization.
void SSTable::writeString(std::ofstream &out, const std::string &str)
{
    uint64_t len = str.length();
    this->writeUint64(out, len);
    out.write(str.c_str(), len);
}

// Reads a string that was written with writeString.
std::string SSTable::readString(std::ifstream &in)
{
    uint64_t len = this->readUint64(in);
    std::vector<char> buffer(len);
    in.read(buffer.data(), len);
    return std::string(buffer.begin(), buffer.end());
}

// Writes a 64-bit integer in a portable binary format (little-endian).
void SSTable::writeUint64(std::ofstream &out, uint64_t value)
{
    out.write(reinterpret_cast<const char *>(&value), sizeof(value));
}

// Reads a 64-bit integer.
uint64_t SSTable::readUint64(std::ifstream &in)
{
    uint64_t value;
    in.read(reinterpret_cast<char *>(&value), sizeof(value));
    return value;
}

bool SSTable::writeFromMemory(const std::vector<KeyValuePair> &memtable)
{
    std::cerr << "SSTable::writeFromMemory called for file: " << filePath << " with " << memtable.size() << " entries." << std::endl;
    std::filesystem::path dir_path = std::filesystem::path(filePath).parent_path();
    std::cerr << "Attempting to create directories: " << dir_path << std::endl;
    std::error_code ec;
    // Only return false if create_directories fails AND reports an actual error
    if (!std::filesystem::create_directories(dir_path, ec) && ec)
    {
        std::cerr << "Error creating directories: " << dir_path << ", error: " << ec.message() << std::endl;
        return false;
    }

    std::cerr << "Attempting to open file for writing: " << filePath << std::endl;
    std::ofstream outFile(filePath, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open())
    {
        std::cerr << "Error: Could not open file for writing: " << filePath << " (errno: " << errno << ")" << std::endl;
        return false;
    }

    std::cerr << "File opened successfully for writing: " << filePath << std::endl;

    std::map<std::string, uint64_t> tempIndex;
    uint64_t currentOffset = 0;

    // --- 1. Write Data Blocks ---
    for (size_t i = 0; i < memtable.size(); i += BLOCK_SIZE)
    {
        // Record the start of the block and its first key for the index.
        tempIndex[memtable[i].key] = currentOffset;
        std::cerr << "Writing block starting with key: " << memtable[i].key << " at offset: " << currentOffset << std::endl;

        // Determine the end of the current block.
        size_t end = std::min(i + BLOCK_SIZE, memtable.size());

        // Write the number of pairs in this block.
        this->writeUint64(outFile, end - i);

        // Write the key-value pairs in this block.
        for (size_t j = i; j < end; ++j)
        {
            this->writeString(outFile, memtable[j].key);
            this->writeString(outFile, memtable[j].value);
            std::cerr << "  Wrote key: '" << memtable[j].key << "', value: '" << memtable[j].value << "'" << std::endl;
        }
        // Update offset for the next block
        currentOffset = outFile.tellp();
        std::cerr << "Block ended, next offset: " << currentOffset << std::endl;
    }

    // --- 2. Write Index Block ---
    uint64_t indexOffset = currentOffset;
    std::cerr << "Writing index block at offset: " << indexOffset << std::endl;

    // Write the total number of index entries.
    this->writeUint64(outFile, tempIndex.size());
    for (const auto &entry : tempIndex)
    {
        this->writeString(outFile, entry.first);  // The key
        this->writeUint64(outFile, entry.second); // The offset
        std::cerr << "  Wrote index entry: key='" << entry.first << "', offset=" << entry.second << std::endl;
    }

    // --- 3. Write Footer (Index Block Offset) ---
    // The footer is a fixed-size pointer at the very end of the file
    // that tells us where the index block begins.
    this->writeUint64(outFile, indexOffset);
    std::cerr << "Writing footer with index offset: " << indexOffset << std::endl;

    outFile.close();
    std::cerr << "SSTable::writeFromMemory finished, file closed: " << filePath << std::endl;
    return true;
}

bool SSTable::loadIndex()
{
    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile.is_open())
    {
        std::cerr << "Error: Could not open file for reading: " << filePath << std::endl;
        return false;
    }

    // --- 1. Read Footer to find Index Block ---
    inFile.seekg(-static_cast<std::streamoff>(sizeof(uint64_t)), std::ios::end);
    uint64_t indexOffset = this->readUint64(inFile);

    // --- 2. Seek to and Read the Index Block ---
    inFile.seekg(indexOffset);
    uint64_t indexSize = this->readUint64(inFile);

    sparseIndex.clear();
    for (uint64_t i = 0; i < indexSize; ++i)
    {
        std::string key = this->readString(inFile);
        uint64_t offset = this->readUint64(inFile);
        sparseIndex[key] = offset;
    }

    inFile.close();
    return true;
}

std::optional<std::string> SSTable::find(const std::string &key)
{
    // Load the index into memory if it hasn't been already.
    if (sparseIndex.empty())
    {
        if (!loadIndex())
        {
            return std::nullopt; // Failed to load index
        }
    }
    if (sparseIndex.empty())
    {
        return std::nullopt; // Index is empty, key cannot exist
    }

    // --- 1. Use the Sparse Index to Find the Right Data Block ---
    // Find the last key in the index that is less than or equal to the target key.
    auto it = sparseIndex.upper_bound(key);
    if (it == sparseIndex.begin())
    {
        // The key is smaller than the first key in the index, so it can't exist.
        return std::nullopt;
    }
    // The correct block is the one preceding the upper_bound.
    --it;

    uint64_t blockOffset = it->second;

    // --- 2. Read the Relevant Data Block from Disk ---
    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile.is_open())
    {
        return std::nullopt;
    }

    inFile.seekg(blockOffset);
    uint64_t pairsInBlock = this->readUint64(inFile);

    // --- 3. Scan the Block for the Key ---
    for (uint64_t i = 0; i < pairsInBlock; ++i)
    {
        std::string currentKey = this->readString(inFile);
        std::string currentValue = this->readString(inFile);
        if (currentKey == key)
        {
            return currentValue; // Key found!
        }
    }

    return std::nullopt; // Key not found in the block.
}

string SSTable::get(const string &key)
{
    // If the SSTable was loaded with all data (e.g., for merging), use the in-memory map.
    if (!data.empty())
    {
        auto it = data.find(key);
        if (it != data.end())
        {
            return it->second;
        }
        return "";
    }
    // Otherwise, perform a disk-based lookup.
    std::optional<std::string> result = find(key);
    if (result.has_value())
    {
        return result.value();
    }
    return "";
}

bool SSTable::put(const string &key, const string &value)
{
    // This `put` method operates on the in-memory `data` map, primarily for merge operations.
    // Changes are not persisted to disk by this method.
    data[key] = value;
    return true;
}

string SSTable::get_first_key() const
{
    // This method operates on the in-memory `data` map, primarily for merge operations.
    if (data.empty())
    {
        return "";
    }
    return data.begin()->first;
}

pair<string, string> SSTable::pop_first_item()
{
    // This method operates on the in-memory `data` map, primarily for merge operations.
    if (data.empty())
    {
        return {"", ""};
    }
    auto it = data.begin();
    pair<string, string> item = *it;
    data.erase(it);
    return item;
}

std::vector<KeyValuePair> SSTable::getAllKeyValues() const
{
    std::vector<KeyValuePair> all_kvs;
    for (const auto &pair : data)
    {
        all_kvs.push_back({pair.first, pair.second});
    }
    return all_kvs;
}

/**
 * @brief Returns the full file path of this SSTable.
 * @return The file path as a string.
 */
std::string SSTable::get_filename() const
{
    // Extract just the filename from the full path
    // This assumes filePath is in the format "data/filename.sst"
    size_t last_slash_pos = filePath.rfind('/');
    if (last_slash_pos != std::string::npos)
    {
        return filePath.substr(last_slash_pos + 1);
    }
    return filePath; // If no slash, filePath is just the filename
}

// These methods are not directly used with the current in-memory SSTable implementation
void SSTable::get_key_pos(const string &key, int &offset, int &length)
{
    // Not implemented for in-memory SSTable
}

string SSTable::get_value_by_pos(const int offset, const int length)
{
    // Not implemented for in-memory SSTable
    return "";
}