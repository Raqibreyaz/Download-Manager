#include "utils.hpp"

// extracts the host, path and port from the provided url
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

// extract filename and extension from the content type and content disposition
std::pair<std::string, std::string> getFilenameAndExtension(
    const std::string &contentDisposition,
    const std::string &contentType,
    const std::string &url)
{
    // Try extracting filename from Content-Disposition
    if (!contentDisposition.empty())
    {
        std::regex filenameRegex("filename\\s*=\\s*\"([^\"]+)\"");
        std::smatch match;

        // if filename found then return that
        if (std::regex_search(contentDisposition, match, filenameRegex))
        {
            std::string filename = match[1];

            // ✨ Clean filename from \r and \n
            filename.erase(std::remove(filename.begin(), filename.end(), '\r'), filename.end());
            filename.erase(std::remove(filename.begin(), filename.end(), '\n'), filename.end());

            return {filename, ""}; // Extension already included
        }
    }

    // check if filename is in url
    if (!url.empty())
    {
        size_t lastSlash = url.find_last_of('/');
        if (lastSlash != std::string::npos)
        {
            std::string afterLastSlash = url.substr(lastSlash + 1);

            size_t invalidChar = afterLastSlash.find_first_of("&;#?% ");
            if (invalidChar != std::string::npos)
                afterLastSlash = afterLastSlash.substr(0, invalidChar);

            return {afterLastSlash, ""};
        }
    }

    size_t semicolon = contentType.find(';');
    if (semicolon == std::string::npos)
        semicolon = contentType.size();

    auto parts = split(contentType.substr(0, semicolon), '/');

    std::string name = parts.size() > 0 ? parts[0] : "download";
    std::string ext = parts.size() > 1 ? ("." + parts[1]) : ".bin";

    // ✨ Clean name and ext (rarely needed but safe)
    name.erase(std::remove(name.begin(), name.end(), '\r'), name.end());
    name.erase(std::remove(name.begin(), name.end(), '\n'), name.end());
    ext.erase(std::remove(ext.begin(), ext.end(), '\r'), ext.end());
    ext.erase(std::remove(ext.begin(), ext.end(), '\n'), ext.end());

    return {name, ext};
}

// splits a string into an array using a delimeter
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

// 1 for downloaded, 2 for partially, 0 for file not exist
int isFileDownloadedOrPartially(const std::string &filename, size_t expectedSize)
{
    std::ifstream file(filename, std::ios::binary);

    if (file)
    {
        file.seekg(0, std::ios::end);

        std::streampos pos = file.tellg();
        if (pos == -1)
            return 0; // Error reading file size, treat as not existing

        size_t fileSize = static_cast<size_t>(pos);

        std::cout << "downloaded size: " << fileSize << std::endl
                  << "expected size: " << expectedSize << std::endl;

        if (fileSize == expectedSize)
            return 1; // Fully downloaded
        else
            return 2; // Partially downloaded
    }
    return 0; // File doesn't exist
}

// saves the provided data to the given file
void saveToFile(const std::string &filename, const std::string &data)
{
    // std::clog << "[DEBUG] Trying to open file: '" << filename << "'" << std::endl;

    std::ofstream file(filename, std::ios::binary | std::ios::app);
    if (!file)
    {
        int err = errno; // system error code
        std::clog << "[ERROR] Failed to open file. errno = " << err
                  << " (" << std::strerror(err) << ")" << std::endl;

        auto dir = std::filesystem::path(filename).parent_path();
        std::clog << "[DEBUG] Parent directory: '" << dir.string() << "'" << std::endl;
        std::clog << "[DEBUG] Directory exists: " << (std::filesystem::exists(dir) ? "YES" : "NO") << std::endl;

        throw std::runtime_error("failed to create/open the file");
    }

    file.write(data.c_str(), data.size());
}

// converts hexdecimal string to decimal number
int hexaDecimalToDecimal(const std::string &num)
{
    int number = std::stoi(num, nullptr, 16);
    std::clog << "converted number: " << number << " from: " << num << std::endl;
    return number;
}
