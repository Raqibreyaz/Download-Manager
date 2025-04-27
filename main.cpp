#include "src/http/http-request/http-request.hpp"
#include "src/http/http-response/http-response.hpp"
#include "src/http/http-stream-reader/http-stream-reader.hpp"
#include "src/socket-lib/tcp-socket/tcp-socket.hpp"
#include "src/socket-lib/ssl-socket/ssl-socket.hpp"
#include "src/utils/utils.hpp"
#include <iostream>
#include <string>
#include <memory>
    
int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        std::cerr << "url required!!";
        exit(EXIT_FAILURE);
    }

    std::string actualUrl(argv[1]);

    ParsedUrl url = parseUrl(actualUrl);

    HttpRequest req(
        "GET", url.path, "HTTP/1.1",
        {{"Accept", "*/*"},
         {"Host", url.host},
         {"User-Agent",
          "Mozilla/5.0 "
          "(Windows NT 10.0; Win64; x64) "
          "AppleWebKit/537.36 "
          "(KHTML, like Gecko) "
          "Chrome/91.0.4472.124 "
          "Safari/537.36"},
         {"Connection", "close"}});

    std::shared_ptr<ISocket> sock;
    HttpResponse res;

    try
    {
        // create a socket for that host and port
        if (actualUrl.find("https") != std::string::npos)
            sock = std::make_shared<SslSocket>(url.host, url.port);
        else
            sock = std::make_shared<TcpSocket>(url.host, url.port);

        // connect to the server
        sock->connectToServer();

        // send request to server
        const std::string requestBuffer = req.toString();
        sock->sendAll(requestBuffer);

        // a reader helper to read different kinds of data
        HttpStreamReader reader(sock);

        // read headers first
        std::string headerString = reader.readHeaders();

        std::clog << "received headers " << headerString << std::endl;

        res.setHeaders(HttpResponse::parseHeaders(headerString));

        std::string contentLengthString = res.getHeader("Content-Length");
        std::string contentType = res.getHeader("Content-Type");
        std::string contentDisposition = res.getHeader("Content-Disposition");
        size_t contentLength =
            contentLengthString.empty() ? 0 : std::stoi(contentLengthString);

        auto [filename, extension] = getFilenameAndExtension(contentDisposition, contentType);

        std::clog << "filename " << filename << extension << std::endl;

        // chunked content will be saved in a file
        if (res.getHeader("Transfer-Encoding") == "chunked")
        {
            std::clog << "chunked transfer found" << std::endl;
            reader.readChunkedContent([filename, extension](const std::string &data)
                                      { saveToFile("downloads/" + filename + extension, data); });
        }

        // logable content will be logged
        else if (contentType.starts_with("text/") || contentType.starts_with("application/json"))
        {
            std::clog << reader.readContent(contentLength) << std::endl;
        }

        // read the content and save it to that corresponding file
        else
        {
            std::clog << "reading whole data directly" << std::endl;
            reader.readSpecifiedChunkedContent(contentLength, [filename, extension](const std::string &data)
                                               { saveToFile("downloads/" + filename + extension, data); });
        }

        sock->closeConnection();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
