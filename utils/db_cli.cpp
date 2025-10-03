#include "../src/request.h"
#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <algorithm>
#include <sstream>
#include <limits> // Required for std::numeric_limits

const int PORT = 5991;               // Default server port
const char *SERVER_IP = "127.0.0.1"; // Default server IP

// Function to send a request and receive a response
std::string send_request(const std::string &request_str)
{
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::cerr << "Error: Socket creation failed\n";
        return "ERROR Socket creation failed";
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0)
    {
        std::cerr << "Error: Invalid address/ Address not supported\n";
        close(sock);
        return "ERROR Invalid address/ Address not supported";
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        std::cerr << "Error: Connection Failed\n";
        close(sock);
        return "ERROR Connection Failed";
    }

    send(sock, request_str.c_str(), request_str.length(), 0);

    char buffer[1024] = {0};
    long valread = read(sock, buffer, 1024);
    std::string response_str(buffer, valread);

    close(sock);
    return response_str;
}

void display_help()
{
    std::cout << "\nAvailable commands:\n";
    std::cout << "  put <key> <value> - Stores a key-value pair.\n";
    std::cout << "  get <key>         - Retrieves the value for a given key.\n";
    std::cout << "  help              - Displays this help message.\n";
    std::cout << "  exit              - Exits the client.\n";
    std::cout << "\nExamples:\n";
    std::cout << "  put mykey myvalue\n";
    std::cout << "  get mykey\n";
    std::cout << "  exit\n";
}

int main()
{
    std::string line;
    std::cout << "Database CLI Client. Type 'help' for commands.\n";

    while (true)
    {
        std::cout << "> ";
        if (!std::getline(std::cin, line))
        {          // Check if getline fails (e.g., EOF or error)
            break; // Exit loop on failure
        }

        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "exit")
        {
            break;
        }
        else if (command == "help")
        {
            display_help();
        }
        else if (command == "put")
        {
            std::string key, value;
            iss >> key >> value;
            if (!key.empty() && !value.empty())
            {
                Request req(RequestType::PUT, key, value);
                std::string response_str = send_request(req.serialize());
                Response res = Response::deserialize(response_str);
                if (res.success)
                {
                    std::cout << "Server: " << res.message << std::endl;
                }
                else
                {
                    std::cerr << "Error: " << res.message << std::endl;
                }
            }
            else
            {
                std::cerr << "Usage: put <key> <value>\n";
            }
        }
        else if (command == "get")
        {
            std::string key;
            iss >> key;
            if (!key.empty())
            {
                Request req(RequestType::GET, key);
                std::string response_str = send_request(req.serialize());
                Response res = Response::deserialize(response_str);
                if (res.success && res.message == "VALUE")
                {
                    std::cout << "Value: " << res.value << std::endl;
                }
                else if (res.success)
                {
                    std::cout << "Server: " << res.message << std::endl; // Should not happen for GET if key exists
                }
                else
                {
                    std::cerr << "Error: " << res.message << std::endl;
                }
            }
            else
            {
                std::cerr << "Usage: get <key>\n";
            }
        }
        else if (!command.empty())
        {
            std::cerr << "Unknown command: " << command << ". Type 'help' for commands.\n";
        }
    }
    return 0;
}
