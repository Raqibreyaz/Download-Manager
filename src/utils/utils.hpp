#pragma once

#include <string>
#include <stdexcept>

struct ParsedUrl
{
    std::string host;
    std::string path = "/";
    std::string port = "80";
};

ParsedUrl parseUrl(const std::string &url);