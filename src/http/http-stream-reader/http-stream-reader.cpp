#include "http-stream-reader.hpp"

// create a HttpStreamReader with provided socket
HttpStreamReader::HttpStreamReader(std::shared_ptr<ISocket> sock)
    : socket(sock), preBuffer("") {}

// read only the headers via socket
std::string HttpStreamReader::readHeaders()
{
    int sizeForHeaders = 4096;

    std::string data = socket->receiveSome(sizeForHeaders);

    // status line end dhundho (properly find "\r\n")
    size_t statusLineEnding = data.find("\r\n");
    if (statusLineEnding == std::string::npos)
    {
        this->preBuffer += data;
        return "";
    }
    statusLineEnding += 2; // move after \r\n

    // headers end dhundho (properly find "\r\n\r\n")
    size_t headersEnding = data.find("\r\n\r\n", statusLineEnding);
    if (headersEnding == std::string::npos)
    {
        this->preBuffer += data;
        return "";
    }

    // Extract headers
    std::string headers = data.substr(statusLineEnding, headersEnding - statusLineEnding);

    // Save body data (after \r\n\r\n)
    this->preBuffer += data.substr(headersEnding + 4);

    return headers;
}

// read only the body content via socket
std::string HttpStreamReader::readContent(const size_t contentLength = 0, const std::function<void(const std::string &data)> &callback)
{
    std::string receivedData =
        contentLength == 0 ? socket->receiveAll() : socket->receiveSome(contentLength);

    // add the pre buffer in the main data
    std::string data = preBuffer + receivedData;

    if (callback)
    {
        callback(data);
    }
    return data;
}

// read the chunked body content and provide it to the callback
void HttpStreamReader::readChunkedContent(const std::function<void(const std::string &data)> &onData)
{
    std::string accumulatedData;

    while (true)
    {
        // get the size of the chunk
        size_t chunkSize = getChunkSize();

        // when all data is received then exit
        if (chunkSize == 0)
        {
            // Sab kuch receive ho gaya
            if (!accumulatedData.empty())
            {
                onData(accumulatedData);
            }
            break;
        }

        size_t remaining = chunkSize;

        while (remaining > 0)
        {
            // when there is no data available then receive some data
            if (preBuffer.empty())
            {
                preBuffer += socket->receiveSome(FLUSH_THRESHOLD);
            }

            // take only the amount of data we can/should take
            size_t toCopy = std::min(remaining, preBuffer.size());

            // append the extracted specified amount of data
            accumulatedData.append(preBuffer.substr(0, toCopy));

            // erase the extracted data from storage and decrease the remaining data required
            preBuffer.erase(0, toCopy);
            remaining -= toCopy;

            // if enough data available then bulk provide to the callback and clear the stored data
            if (accumulatedData.size() >= FLUSH_THRESHOLD)
            {
                onData(accumulatedData);
                accumulatedData.clear();
            }
        }

        // Chunk ke data ke baad \r\n aata hai, usko discard karna padega
        ensureCRLF();
    }

    // Last me agar kuch bach gaya accumulatedData me
    if (!accumulatedData.empty())
    {
        onData(accumulatedData);
    }
}

// read the given Content-Length size data and provide it to the callback
void HttpStreamReader::readSpecifiedChunkedContent(const size_t contentLength, const std::function<void(const std::string &data)> &callback)
{
    std::string data = preBuffer;

    size_t remainingData = contentLength - data.size();

    size_t noOfChunksCompleted = 0;
    std::string receivedData = "";

    // receiving data till we get empty data
    while (!(receivedData = socket->receiveSome(FLUSH_THRESHOLD)).empty())
    {
        // incrementing when a chunk fetched
        noOfChunksCompleted++;

        // decrease what amount of data we fetched
        remainingData -= receivedData.size();

        // adding to fetched data to our stored data
        data = data + receivedData;

        // showing the download status
        double downloadStatus = ((contentLength - remainingData) * 1.0 / contentLength) * 100;
        downloadStatus = round(downloadStatus * 10.0) / 10.0;
        std::clog << "\rdownloading " << downloadStatus << "%" << std::flush;

        // for every 5 fetched chunks we are writing to the file
        if (noOfChunksCompleted % 5 == 0)
        {
            callback(data);

            // clear the data as written to the file
            data.clear();
        }
    }

    std::clog << "\nno of chunks we received: " << noOfChunksCompleted << std::endl;

    // if we doesnt got specified amount of data
    if (remainingData != 0)
        std::clog << "Data doesnt received as much as specified" << std::endl;

    // if there is  some data left then write to the file
    if (!data.empty())
    {
        callback(data);
        data.clear();
    }

    // adding empty data to prebuffer
    preBuffer = data;
}

// extract the chunk size from the chunk
size_t HttpStreamReader::getChunkSize()
{
    // take the first line from the prebuffered data chunks
    std::string line;
    while (true)
    {
        // find the line ending
        auto pos = preBuffer.find('\n');

        // if there is a line ending then take the line and remove the line ending
        if (pos != std::string::npos)
        {
            line = preBuffer.substr(0, pos + 1);
            preBuffer.erase(0, pos + 1);

            // Remove \r\n safely
            while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
                line.pop_back();

            break;
        }
        preBuffer += socket->receiveSome(FLUSH_THRESHOLD);
    }

    // if there is no line
    if (line.empty())
        throw std::runtime_error("Invalid or empty chunk size line");

    // convert the stringified hexadecimal number to decimal number
    return std::stoul(line, nullptr, 16);
}

// will remove the CRLF("\r\n") from the chunk
void HttpStreamReader::ensureCRLF()
{
    // when chunk data ending not available then receive some data for it
    while (preBuffer.size() < 2)
    {
        preBuffer += socket->receiveSome(FLUSH_THRESHOLD);
    }

    // remove the chunked data ending
    if (preBuffer.substr(0, 2) != "\r\n")
    {
        throw std::runtime_error("Expected CRLF after chunk data");
    }
    preBuffer.erase(0, 2);
}
