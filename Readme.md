# **Download Manager**

A simple yet powerful download manager built to handle HTTP/HTTPS requests, process chunked transfer encoding, and efficiently save downloaded files. This tool supports downloading files via both HTTP and HTTPS, with chunked data handling for large files.

---

## **Features**

- **HTTP/HTTPS Support:** Supports both HTTP and HTTPS protocols for downloading files.
- **Chunked Transfer Encoding:** Handles chunked responses and downloads data in manageable chunks, ensuring large files can be downloaded without memory overflow.
- **File Saving:** Efficiently saves data to a file, appending it if necessary to prevent overwriting the existing content.
- **Flexible Content Handling:** Handles different content types (binary, text, JSON) and saves them accordingly.

---

## **Installation**

### Prerequisites

Ensure that you have the following installed:

- **C++ Compiler** (e.g., `g++`)
- **CMake** (for building the project)
- **OpenSSL** (for handling SSL/TLS connections if using HTTPS)

### Steps

1. Clone the repository:

   ```bash
   git clone <your-repository-url>
   ```

2. Navigate to the project directory:

   ```bash
   cd <project-directory>
   ```

3. Install dependencies (if any) and set up OpenSSL for HTTPS support.

4. Create a build directory:

   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

5. Run the program:

   ```bash
   ./download-manager <URL>
   ```

---

## **Usage**

### Minimal Working Setup

To use this tool, you need to pass a URL as a command-line argument. The program will:

1. **Send an HTTP GET Request** to the provided URL.
2. **Receive HTTP Response**: It processes headers, content length, and handles chunked transfer encoding.
3. **Save the File**: Depending on the response, the content is saved to disk, handling both regular file downloads and chunked content.

Here's the basic usage:

```cpp
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
```

---

## **Explanation of the Code**

1. **URL Parsing:** The program takes the URL input, parses it to extract the host and path, and prepares the HTTP request.
2. **Socket Initialization:** Depending on the URL protocol (HTTP/HTTPS), the appropriate socket (`TcpSocket` or `SslSocket`) is created.
3. **Sending the Request:** A `GET` request is sent to the server for the specified URL.
4. **Reading Headers:** The headers are read first to extract details like content length, content type, and content disposition.
5. **Handling Chunked Data:** If the server uses chunked transfer encoding, the `HttpStreamReader` reads the data chunk by chunk and appends it to the file.
6. **File Saving:** The received data is saved to a file in the `downloads/` folder.

---

## **How the Project Works**

- **HTTP/HTTPS Requests:** Handles requests for both HTTP and HTTPS protocols.
- **Chunked Transfer Encoding:** Processes chunked data, ensuring data is handled incrementally without overwhelming memory.
- **Content-Type Handling:** Supports different content types such as text and binary. Text responses are logged, while binary content is saved to files.
- **File Saving:** Automatically saves downloaded data to a file, ensuring no data is lost, even when chunks arrive at different times.

---

## **Contributing**

Feel free to fork this repository and submit pull requests. If you find bugs or have suggestions for improvements, create an issue to discuss them.

---