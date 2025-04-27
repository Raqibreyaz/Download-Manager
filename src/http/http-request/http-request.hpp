#pragma once 

#include <string>
#include <unordered_map>
#include <sstream>

class HttpRequest
{
    std::string method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;

public:
    HttpRequest();
    HttpRequest(const std::string &method,
                const std::string &path,
                const std::string &version,
                const std::unordered_map<std::string, std::string> &headers);
    ~HttpRequest();
    std::string toString() const;
    static HttpRequest parse(const std::string &requestBuffer);
};
