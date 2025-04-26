#pragma once

#include <string>
#include <unordered_map>
#include <sstream>
#include <iostream>

class HttpResponse
{
    std::string version;
    int statusCode;
    std::string statusMessage;
    std::unordered_map<std::string, std::string> headers;
    std::string body;

public:
    HttpResponse();
    HttpResponse(
        const std::string &ver,
        const int sc,
        const std::string &sm,
        const std::unordered_map<std::string, std::string> &headers,
        const std::string &body);
    ~HttpResponse();
    std::string getHeader(const std::string &key);
    std::string getContent();
    std::string toString() const;
    static HttpResponse parse(const std::string &responseBuffer);
};
