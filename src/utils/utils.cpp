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

std::pair<std::string, std::string> getFilenameAndExtension(
    const std::string &contentDisposition,
    const std::string &contentType)
{
    // Try extracting filename from Content-Disposition
    if (!contentDisposition.empty())
    {
        std::regex filenameRegex("filename\\s*=\\s*\"([^\"]+)\"");
        std::smatch match;
        if (std::regex_search(contentDisposition, match, filenameRegex))
        {
            std::string filename = match[1];
            return {filename, ""}; // Extension already included in filename
        }
    }

    size_t semicolon = contentType.find(';');

    if (semicolon == std::string::npos)
        semicolon = contentType.size();

    auto parts = split(contentType.substr(0, semicolon), '/');

    // Default name (eg. image/png-> name: image, ext: png)
    return {parts.size() > 0 ? parts[0] : "download", parts.size() > 1 ? ("." + parts[1]) : ".bin"};
}

std::vector<std::string> split(const std::string &str, const char &delim)
{
    if (str.empty())
        return std::vector<std::string>();

    std::stringstream ss(str);
    std::string token;

    std::vector<std::string> splittedStr;

    while (std::getline(ss, token, delim))
        splittedStr.push_back(token);

    return splittedStr;
}

void saveToFile(const std::string &filename, const std::string &data)
{
    // open file in write mode
    std::ofstream file(filename, std::ios::binary);

    if (!file)
        return throw std::runtime_error("failed to create/open the file");

    file.write(data.c_str(), data.size());
}