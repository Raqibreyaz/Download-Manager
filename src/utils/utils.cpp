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

            // ✨ Clean filename from \r and \n
            filename.erase(std::remove(filename.begin(), filename.end(), '\r'), filename.end());
            filename.erase(std::remove(filename.begin(), filename.end(), '\n'), filename.end());

            return {filename, ""}; // Extension already included
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
    std::clog << "[DEBUG] Trying to open file: '" << filename << "'" << std::endl;

    std::ofstream file(filename, std::ios::binary);
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

int hexaDecimalToDecimal(const std::string &num)
{
    int number = std::stoi(num, nullptr, 16);
    std::clog << "converted number: " << number << " from: " << num << std::endl;
    return number;
}

std::string readChunksAndSaveToFile(const std::string &buffer, const std::string &filename)
{
    // 4\r\n
    // abcd\r\n

    // work on the copy of it
    std::string temp = buffer;

    // tracks of is the value a number or string data
    bool isChunkSize = true;

    // getting total size by summation of all chunk sizes
    int totalLength = 0;

    // appending all chunked data in this string
    std::string data = "";

    // will have the preserved line
    // which is !completely received
    std::string returnValue = "";

    size_t startPos = 0;
    while (startPos < temp.size())
    {
        size_t stopPos = temp.find("\r\n", startPos);

        // when we cant find the end of line then this line should be preserved
        // so return this line
        if (stopPos == std::string::npos)
        {
            returnValue = temp.substr(startPos);
            break;
        }

        std::string value = temp.substr(startPos, stopPos - startPos);

        // if it is a size then convert to number
        if (isChunkSize)
        {
            int size = hexaDecimalToDecimal(value);

            if (size == 0)
                break;

            std::clog << value << " size" << std::endl;
            totalLength += size;
        }
        // if it is a data then append to string
        else
        {
            data.append(value);
            std::clog << value << " data" << std::endl;
        }

        // update boolean
        isChunkSize = !isChunkSize;

        // move start to next value
        startPos = stopPos + 2;
    }

    if (totalLength != data.size())
        std::cerr << "Inconsistent chunked size and data received" << std::endl;

    // saving the received data to the file
    saveToFile(filename, data);

    // return the preserved line
    return returnValue;
}
