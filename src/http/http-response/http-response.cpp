#include "http-response.hpp"

// create HttpResponse object from default values
HttpResponse::HttpResponse()
    : version("HTTP/1.1"), statusCode(200), statusMessage("OK"),
      headers(std::unordered_map<std::string, std::string>()), body("") {}

// create HttpResponse object from provided values
HttpResponse::HttpResponse(
    const std::string &ver, const int sc, const std::string &sm,
    const std::unordered_map<std::string, std::string> &hdr,
    const std::string &bdy)
    : version(ver), statusCode(sc), statusMessage(sm), headers(hdr), body(bdy) {
}

// get the header value if found, fallback to empty string
std::string HttpResponse::getHeader(const std::string &key) {
  auto it = this->headers.find(key);

  return it == this->headers.end() ? "" : it->second;
}

// get the content of the response
std::string HttpResponse::getContent() { return this->body; }

// stringify the HttpResponse object
std::string HttpResponse::toString() const {
  std::ostringstream oss;
  oss << this->version << " " << this->statusCode << " " << this->statusMessage
      << "\r\n";
  for (const auto &[key, value] : headers) {
    oss << key << ": " << value << "\r\n";
  }
  oss << "\r\n";
  oss << this->body;
  return oss.str();
}

// set the Body content of the HttpRespons object
void HttpResponse::setContent(const std::string &body) { this->body = body; }

// set the HttpResponse object header
void HttpResponse::setHeader(
    const std::pair<std::string, std::string> &header) {
  this->headers[header.first] = header.second;
}

// set the headers of the HttpResponse object
void HttpResponse::setHeaders(
    const std::unordered_map<std::string, std::string> &headers) {
  this->headers = headers;
}

// parse the response buffer into object
HttpResponse HttpResponse::parse(const std::string &responseBuffer) {
  HttpResponse res;
  std::istringstream stream(responseBuffer);
  std::string line;

  // Status line
  std::getline(stream, line);
  std::istringstream statusLine(line);
  statusLine >> res.version >> res.statusCode;
  std::getline(statusLine, res.statusMessage); // Rest of the line

  if (!res.statusMessage.empty() && res.statusMessage.back() == '\r')
    res.statusMessage.pop_back();

  // Headers
  while (std::getline(stream, line) && line != "\r") {
    auto colon = line.find(':');
    if (colon != std::string::npos) {
      std::string key = line.substr(0, colon);
      std::string value = line.substr(colon + 1);

      // triming leading spaces
      key.erase(0, key.find_first_not_of(" \t\r\n"));
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

// parse the status line to the object
void HttpResponse::parseStatusLine(const std::string &statusBuffer) {
  if (statusBuffer.starts_with("HTTP/")) {
    std::istringstream statusLine(statusBuffer);
    statusLine >> version >> statusCode;
    std::getline(statusLine, statusMessage); // Rest of the line

    if (!statusMessage.empty() && statusMessage.back() == '\r')
      statusMessage.pop_back();
  }
}

// get a map of headers
std::unordered_map<std::string, std::string>
HttpResponse::parseHeaders(const std::string &headerBuffer) {

  std::unordered_map<std::string, std::string> headers;

  // when no header is present then return empty map
  if (headerBuffer.empty())
    return headers;

  std::istringstream stream(headerBuffer);
  std::string line;

  //   go to every header line and add key & value to map
  while (std::getline(stream, line) && line != "\r") {
    size_t colonPos = line.find(':');

    // if there is header 
    if (colonPos != std::string::npos) {

      // extract key and value
      std::string key = line.substr(0, colonPos);
      std::string value = line.substr(colonPos + 1);

      // triming leading spaces
      key.erase(0, key.find_first_not_of(" \t\r\n"));
      value.erase(0, value.find_first_not_of(" \t\r\n"));

      headers[key] = value;
    }
  }
  return headers;
}

// get the content of the body
std::string HttpResponse::parseBody(const std::string &bodyBuffer) {
  // find the headers ending for body start
  size_t bodyStart = bodyBuffer.find("\r\n\r\n");

  // if there is no headers ending then whole is the body
  if (bodyStart == std::string::npos)
    bodyStart = 0;

  // skip the headers ending
  else
    bodyStart += 4;

  return bodyBuffer.substr(bodyStart);
}

HttpResponse::~HttpResponse() {}