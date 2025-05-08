#pragma once

#include "../../socket-lib/isocket/isocket.hpp"
#include <functional>
#include <iostream>
#include <math.h>
#include <memory>

#define FLUSH_THRESHOLD 8192 // 8KB ka threshold flush karne ke liye

class HttpStreamReader {
  std::shared_ptr<ISocket> socket; // TCP/SSl socket
  std::string preHeader;
  std::string preContent;

public:
  HttpStreamReader(std::shared_ptr<ISocket> sock);

  //   read only the status line from buffer
  std::string readStatusLine();

  // reads only the headers from buffer
  std::string readHeaders();

  // reads the body from the buffer
  std::string
  readContent(const std::function<void(const std::string &data)> &callback,
              const size_t contentLength = 0);

  // reads the chunked data via buffer
  void
  readChunkedContent(const std::function<void(const std::string &)> &callback);

  void readSpecifiedChunkedContent(
      const size_t contentLength,
      const std::function<void(const std::string &)> &callback);

private:
  size_t getChunkSize();
  void ensureCRLF();
};