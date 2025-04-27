#include "http-request.hpp"

HttpRequest::HttpRequest() {}

HttpRequest::HttpRequest(const std::string &m,
                         const std::string &p,
                         const std::string &v,
                         const std::unordered_map<std::string, std::string> &h)
    : method(m),
      path(p), version(v), headers(h)
{
}

std::string HttpRequest::toString() const
{
    std::ostringstream oss;
    oss << method << " " << path << " " << version << "\r\n";
    for (const auto &[key, value] : headers)
    {
        oss << key << ": " << value << "\r\n";
    }
    oss << "\r\n";
    return oss.str();
}
HttpRequest HttpRequest::parse(const std::string &requestBuffer)
{
    HttpRequest req;
    std::istringstream stream(requestBuffer);
    std::string line;

    // First line → Method Path Version
    std::getline(stream, line);
    std::istringstream firstLine(line);
    firstLine >> req.method >> req.path >> req.version;

    // Headers
    while (std::getline(stream, line) && line != "\r")
    {
        auto colon = line.find(':');
        if (colon != std::string::npos)
        {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            // remove leading space or tab
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            req.headers[key] = value;
        }
    }

    return req;
}

HttpRequest::~HttpRequest()
{
    // std::cout << "requets object being freed" << std::endl;
}