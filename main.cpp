#include "src/http/http-request/http-request.hpp"
#include "src/http/http-response/http-response.hpp"
#include "src/http/http-stream-reader/http-stream-reader.hpp"
#include "src/socket-lib/ssl-socket/ssl-socket.hpp"
#include "src/socket-lib/tcp-socket/tcp-socket.hpp"
#include "src/utils/utils.hpp"
#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

int main(int argc, char const *argv[]) {
  if (argc < 2) {
    std::cerr << "url required!!";
    exit(EXIT_FAILURE);
  }

  std::string actualUrl(argv[1]);

  // extract the host, path and port from the url
  ParsedUrl url = parseUrl(actualUrl);

  // create a HttpRequest object for requesting to server
  HttpRequest req("GET", url.path, "HTTP/1.1",
                  {{"Accept", "*/*"},
                   {"Host", url.host},
                   {"User-Agent", "Mozilla/5.0 "
                                  "(Windows NT 10.0; Win64; x64) "
                                  "AppleWebKit/537.36 "
                                  "(KHTML, like Gecko) "
                                  "Chrome/91.0.4472.124 "
                                  "Safari/537.36"},
                   {"Connection", "close"}});

  // a socket for connecting, sending/receiving data to/from server
  std::shared_ptr<ISocket> sock;

  // for storing the response from server
  HttpResponse res;

  try {
    // create a socket for that host and port
    if (actualUrl.find("https") != std::string::npos)
      sock = std::make_shared<SslSocket>(url.host, url.port);
    else
      sock = std::make_shared<TcpSocket>(url.host, url.port);

    // connect to the server
    sock->connectToServer();

    // send request to server
    sock->sendAll(req.toString());

    // a reader helper to read different kinds of data
    HttpStreamReader reader(sock);

    const std::string statusLineString = reader.readStatusLine();
    const std::string headerString = reader.readHeaders();

    // std::clog << statusLineString << "\n\n" << headerString << std::endl;

    // set status, headers
    res.parseStatusLine(statusLineString);
    res.setHeaders(HttpResponse::parseHeaders(headerString));

    const std::string contentLengthString = res.getHeader("Content-Length");
    const size_t contentLength =
        std::stoi(contentLengthString.empty() ? "0" : contentLengthString);

    std::clog << statusLineString << std::endl << headerString << std::endl;

    auto [filename, extension] =
        getFilenameAndExtension(res.getHeader("Content-Disposition"),
                                res.getHeader("Content-Type"), actualUrl);

    const std::function<void(const std::string &data)> callback =
        [filename, extension](const std::string &data) {
          saveToFile("downloads/" + filename + extension, data);
        };

    const std::string contentString =
        reader.readContent(callback, contentLength);

    // finally close the connection to server
    sock->closeConnection();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }

  return 0;
}
