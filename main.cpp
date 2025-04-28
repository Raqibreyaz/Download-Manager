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

    // extract the host, path and port from the url
    ParsedUrl url = parseUrl(actualUrl);

    // create a HttpRequest object for requesting to server
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

    // a socket for connecting, sending/receiving data to/from server
    std::shared_ptr<ISocket> sock;

    // for storing the response from server
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
        sock->sendAll(req.toString());

        // a reader helper to read different kinds of data
        HttpStreamReader reader(sock);

        // read headers first
        std::string headerString = reader.readHeaders();

        // set the received headers
        res.setHeaders(HttpResponse::parseHeaders(headerString));

        // extract key info from the headers
        std::string contentLengthString = res.getHeader("Content-Length");
        std::string contentType = res.getHeader("Content-Type");
        std::string contentDisposition = res.getHeader("Content-Disposition");
        size_t contentLength =
            contentLengthString.empty() ? 0 : std::stoi(contentLengthString);

        // find the filename with extension
        auto [filename, extension] = getFilenameAndExtension(contentDisposition, contentType, actualUrl);
        std::clog << "filename " << filename << extension << std::endl;

        int fileDownloadStatus = isFileDownloadedOrPartially("downloads/" + filename + extension, contentLength);

        std::clog << "file download status: " << fileDownloadStatus << std::endl;

        // when file is already downloaded then skip downloading
        if (fileDownloadStatus == 1)
        {
            std::clog << "file is already downloaded" << std::endl;
            sock->closeConnection();
            return 0;
        }

        // handle chunked data(will be saved to file)
        if (res.getHeader("Transfer-Encoding") == "chunked")
        {
            std::clog << "chunked transfer found" << std::endl;
            reader.readChunkedContent([filename, extension](const std::string &data)
                                      { saveToFile("downloads/" + filename + extension, data); });
        }

        // log text like content
        else if (contentType.starts_with("text/") || contentType.starts_with("application/json"))
        {
            std::clog << reader.readContent(contentLength) << std::endl;
        }

        // handle receiving large data(will be saved to file)
        else
        {
            reader.readSpecifiedChunkedContent(contentLength, [filename, extension](const std::string &data)
                                               { saveToFile("downloads/" + filename + extension, data); });
        }

        // finally close the connection to server
        sock->closeConnection();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
