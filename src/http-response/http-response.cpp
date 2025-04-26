#include "http-response.hpp"

HttpResponse::HttpResponse() {}

HttpResponse::HttpResponse(const std::string &ver,
                           const int sc,
                           const std::string &sm,
                           const std::unordered_map<std::string, std::string> &hdr,
                           const std::string &bdy)
    : version(ver), statusCode(sc), statusMessage(sm), headers(hdr), body(bdy) {}

std::string HttpResponse::toString() const
{
    std::ostringstream oss;
    oss << this->version << " " << this->statusCode << " " << this->statusMessage << "\r\n";
    for (const auto &[key, value] : headers)
    {
        oss << key << ": " << value << "\r\n";
    }
    oss << "\r\n";
    oss << this->body;
    return oss.str();
}
HttpResponse HttpResponse::parse(const std::string &responseBuffer)
{
    HttpResponse res;
    std::istringstream stream(responseBuffer);
    std::string line;

    // Status line
    std::getline(stream, line);
    std::istringstream statusLine(line);
    statusLine >> res.version >> res.statusCode;
    std::getline(statusLine, res.statusMessage); // Rest of the line

    if (res.statusMessage.empty() && res.statusMessage.back() == '\r')
        res.statusMessage.pop_back();

    // Headers
    while (std::getline(stream, line) && line != "\r")
    {
        auto colon = line.find(':');
        if (colon != std::string::npos)
        {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);

            // triming leading spaces
            value.erase(0, value.find_first_not_of(" \t\r\n"));

            res.headers[key] = value;
        }
    }

    // Body (if any)
    std::ostringstream bodyStream;
    bodyStream << stream.rdbuf(); // Remaining part of stream
    res.body = bodyStream.str();

    return res;
}

HttpResponse::~HttpResponse(){}