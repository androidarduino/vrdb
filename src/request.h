#ifndef REQUEST_H
#define REQUEST_H

#include <string>
#include <vector>
#include <map>

// Enum for request types
enum class RequestType
{
    GET,
    PUT,
    UNKNOWN
};

// Base Request structure
struct Request
{
    RequestType type;
    std::string key;
    std::string value;

    Request() : type(RequestType::UNKNOWN) {}
    Request(RequestType t, const std::string &k = "", const std::string &v = "")
        : type(t), key(k), value(v) {}

    // Serialize request to string (e.g., "GET key", "PUT key value")
    std::string serialize() const
    {
        if (type == RequestType::GET)
        {
            return "GET " + key;
        }
        else if (type == RequestType::PUT)
        {
            return "PUT " + key + " " + value;
        }
        return "UNKNOWN";
    }

    // Deserialize request from string
    static Request deserialize(const std::string &data)
    {
        if (data.rfind("GET ", 0) == 0)
        { // Starts with "GET "
            return Request(RequestType::GET, data.substr(4));
        }
        else if (data.rfind("PUT ", 0) == 0)
        {                                           // Starts with "PUT "
            size_t first_space = data.find(' ', 4); // Find space after "PUT "
            if (first_space != std::string::npos)
            {
                std::string key_str = data.substr(4, first_space - 4);
                std::string value_str = data.substr(first_space + 1);
                return Request(RequestType::PUT, key_str, value_str);
            }
        }
        return Request(); // UNKNOWN request
    }
};

// Base Response structure
struct Response
{
    bool success;
    std::string message;
    std::string value;

    Response(bool s = false, const std::string &msg = "", const std::string &val = "")
        : success(s), message(msg), value(val) {}

    // Serialize response to string (e.g., "OK", "VALUE value", "ERROR message")
    std::string serialize() const
    {
        if (success)
        {
            if (message == "VALUE")
            {
                return "VALUE " + value;
            }
            else
            {
                return "OK";
            }
        }
        else
        {
            return "ERROR " + message;
        }
    }

    // Deserialize response from string
    static Response deserialize(const std::string &data)
    {
        if (data.rfind("OK", 0) == 0)
        {
            return Response(true, "OK");
        }
        else if (data.rfind("VALUE ", 0) == 0)
        {
            return Response(true, "VALUE", data.substr(6));
        }
        else if (data.rfind("ERROR ", 0) == 0)
        {
            return Response(false, data.substr(6));
        }
        return Response(false, "UNKNOWN_RESPONSE");
    }
};

#endif // REQUEST_H
