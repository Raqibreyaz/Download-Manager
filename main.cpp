#include "src/http-request/http-request.hpp"
#include "src/http-response/http-response.hpp"
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
         {
             "User-Agent",
             "CustomDownloader/1.0"
             //   "Mozilla/5.0 "
             //   "(Windows NT 10.0; Win64; x64) "
             //   "AppleWebKit/537.36 "
             //   "(KHTML, like Gecko) "
             //   "Chrome/91.0.4472.124 "
             //   "Safari/537.36"
         },
         {"Connection", "close"}});

    std::unique_ptr<ISocket> sock;

    std::cout << "host: " << url.host << std::endl
              << "path: " << url.path << std::endl
              << "port: " << url.port << std::endl;

    try
    {
        if (actualUrl.find("https") != std::string::npos)
            sock = std::make_unique<SslSocket>(url.host, url.port);
        else
            sock = std::make_unique<TcpSocket>(url.host, url.port);

        sock->connectToServer();

        const std::string requestBuffer = req.toString();

        std::clog << "request: " << requestBuffer << std::endl;

        sock->sendAll(req.toString());

        std::cout << sock->receiveAll();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
