#pragma once

#include "../../socket-lib/isocket/isocket.hpp"
#include <memory>
#include <functional>

#define FLUSH_THRESHOLD 8192 // 8KB ka threshold flush karne ke liye

class HttpStreamReader
{
    std::shared_ptr<ISocket> socket; // TCP/SSl socket
    std::string preBuffer;

public:
    HttpStreamReader(std::shared_ptr<ISocket> sock);
    std::string readHeaders();                                                                                                   // reads only the headers from buffer
    std::string readContent(const size_t contentLength, const std::function<void(const std::string &data)> &callback = nullptr); // reads the body from the buffer
    void readChunkedContent(const std::function<void(const std::string &)> &callback);                                           // reads the chunked data via buffer
private:
    size_t getChunkSize();
    void ensureCRLF();
};