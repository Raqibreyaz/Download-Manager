#pragma once

#include <string>
#include <cstring>
#include <cerrno>
#include <filesystem>
#include <stdexcept>
#include <regex>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <unistd.h>

struct ParsedUrl
{
    std::string host;
    std::string path = "/";
    std::string port = "80";
};

ParsedUrl parseUrl(const std::string &url);

std::pair<std::string, std::string> getFilenameAndExtension(
    const std::string &contentDisposition,
    const std::string &contentType,
    const std::string &url);

std::vector<std::string> split(const std::string &str, const char &delim);

void saveToFile(const std::string &filename, const std::string &data);

long long getFileSizeIfPresent(const std::string& filename);

int hexaDecimalToDecimal(const std::string &num);
