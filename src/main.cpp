#include "server.h"
#include <iostream>
#include <string>

int main()
{
    // Create a Server instance listening on port 5991
    Server server("127.0.0.1", 5991);
    std::cout << "Server instance created." << std::endl;

    // Start the server to listen for incoming connections
    server.start();

    return 0;
}
