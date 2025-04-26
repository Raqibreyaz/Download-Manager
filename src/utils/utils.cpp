#include "utils.hpp"

// http://www.google.com/main
ParsedUrl parseUrl(const std::string &url)
{
    if (url.empty())
        throw std::runtime_error("invalid url provided");

    ParsedUrl result;
    std::string temp = url;

    if (temp.find("https") != std::string::npos)
    {
        result.port = "443";
    }

    size_t hostPos = temp.find("://");
    if (hostPos != std::string::npos)
        temp = temp.substr(hostPos + 3);

    size_t pathPos = temp.find('/');

    if (pathPos != std::string::npos)
    {
        result.path = temp.substr(pathPos);
        temp = temp.substr(0, pathPos);
    }

    size_t portPos = temp.find(':');

    if (portPos != std::string::npos)
    {
        result.port = temp.substr(portPos + 1);
        result.host = temp.substr(0, portPos);
    }
    else
        result.host = temp;

    return result;
}