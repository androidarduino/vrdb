#include "../src/database.h"
#include "../src/server.h" // For getCurrentUnixTimeString
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

// Function to list all key-value pairs in an SSTable
void list_sst_file(const std::string &filename)
{
    SSTable sst(filename, true); // Load all data into memory for listing
    std::vector<KeyValuePair> kvs = sst.getAllKeyValues();
    if (kvs.empty())
    {
        std::cout << "SSTable " << filename << " is empty or could not be loaded." << std::endl;
        return;
    }
    std::cout << "Contents of SSTable: " << filename << std::endl;
    for (const auto &kv : kvs)
    {
        std::cout << "  Key: " << kv.key << ", Value: " << kv.value << std::endl;
    }
}

// Function to get a value for a given key from an SSTable
void get_sst_value(const std::string &filename, const std::string &key)
{
    SSTable sst(filename, false); // No need to load all data for a single lookup
    std::optional<std::string> value = sst.find(key);
    if (value.has_value())
    {
        std::cout << "Value for key \"" << key << "\" in " << filename << ": " << value.value() << std::endl;
    }
    else
    {
        std::cout << "Key \"" << key << "\" not found in " << filename << std::endl;
    }
}

// Function to set (update or add) a key-value pair in an SSTable
// This creates a new SSTable as SSTables are immutable
void set_sst_value(const std::string &filename, const std::string &key, const std::string &value)
{
    SSTable original_sst(filename, true); // Load original data
    std::vector<KeyValuePair> kvs = original_sst.getAllKeyValues();

    // Convert to map for easy update
    std::map<std::string, std::string> temp_map;
    for (const auto &kv : kvs)
    {
        temp_map[kv.key] = kv.value;
    }

    // Update or add the new key-value
    temp_map[key] = value;

    // Convert back to sorted vector for writing a new SSTable
    std::vector<KeyValuePair> updated_kvs;
    for (const auto &pair : temp_map)
    {
        updated_kvs.push_back({pair.first, pair.second});
    }
    std::sort(updated_kvs.begin(), updated_kvs.end());

    // Create a new filename for the updated SSTable (e.g., original_updated_timestamp.sst)
    std::string updated_filename = fs::path(filename).stem().string() + "_updated_" + getCurrentUnixTimeString() + ".sst";

    SSTable updated_sst_obj(updated_filename, false); // Create new SSTable object for writing
    if (updated_sst_obj.writeFromMemory(updated_kvs))
    {
        std::cout << "Successfully updated/set key \"" << key << "\" in " << filename << ". New SSTable created: " << updated_filename << std::endl;
    }
    else
    {
        std::cerr << "Error: Failed to write updated SSTable for " << filename << std::endl;
    }
}

void print_help()
{
    std::cout << "Usage: sst_cli <command> [arguments]\n";
    std::cout << "Commands:\n";
    std::cout << "  list <filename>           List all key-value pairs in an SSTable file.\n";
    std::cout << "  get <filename> <key>      Get the value for a specific key from an SSTable file.\n";
    std::cout << "  set <filename> <key> <value> Set (update/add) a key-value pair in an SSTable file. Creates a new updated SSTable.\n";
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        print_help();
        return 1;
    }

    std::string command = argv[1];

    if (command == "list")
    {
        if (argc < 3)
        {
            std::cerr << "Error: Missing filename for list command.\n";
            print_help();
            return 1;
        }
        list_sst_file(argv[2]);
    }
    else if (command == "get")
    {
        if (argc < 4)
        {
            std::cerr << "Error: Missing filename or key for get command.\n";
            print_help();
            return 1;
        }
        get_sst_value(argv[2], argv[3]);
    }
    else if (command == "set")
    {
        if (argc < 5)
        {
            std::cerr << "Error: Missing filename, key, or value for set command.\n";
            print_help();
            return 1;
        }
        set_sst_value(argv[2], argv[3], argv[4]);
    }
    else
    {
        std::cerr << "Error: Unknown command \"" << command << "\".\n";
        print_help();
        return 1;
    }

    return 0;
}
