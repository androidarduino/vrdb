#include "server.h"
#include <iostream>
#include <string>

int main()
{
    // Create a Server instance listening on port 5991
    Server server("127.0.0.1", 5991);
    std::cout << "Server listening on port 5991..." << std::endl;

    // Example: Handle a few requests (in a real scenario, this would be in a loop)
    std::cout << "\nPutting key1=value1..." << std::endl;
    server.put("key1", "value1");

    std::cout << "Getting key1: " << server.get("key1") << std::endl; // Should be value1

    std::cout << "Putting key2=value2..." << std::endl;
    server.put("key2", "value2");

    std::cout << "Getting key2: " << server.get("key2") << std::endl; // Should be value2

    std::cout << "Getting nonexistent_key: " << server.get("nonexistent_key") << std::endl; // Should be empty

    // In a real server, you would have a continuous loop to accept connections
    // and process requests. For this example, we'll just demonstrate a few.
    return 0;
}
