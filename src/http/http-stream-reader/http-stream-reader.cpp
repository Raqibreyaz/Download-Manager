#include "http-stream-reader.hpp"

HttpStreamReader::HttpStreamReader(std::shared_ptr<ISocket> sock)
    : socket(sock), preBuffer("") {}

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

// inside HttpStreamReader class:

void HttpStreamReader::readChunkedContent(const std::function<void(const std::string &data)> &onData)
{
    std::string accumulatedData;

    while (true)
    {
        size_t chunkSize = getChunkSize();

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
            if (preBuffer.empty())
            {
                preBuffer += socket->receiveSome(FLUSH_THRESHOLD);
            }

            size_t toCopy = std::min(remaining, preBuffer.size());
            accumulatedData.append(preBuffer.substr(0, toCopy));
            preBuffer.erase(0, toCopy);
            remaining -= toCopy;

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

void HttpStreamReader::readSpecifiedChunkedContent(const size_t contentLength, const std::function<void(const std::string &data)> &callback)
{
    if (contentLength == 0)
    {
        readContent(contentLength, callback);
        return;
    }

    size_t remainingData = contentLength;
    std::string accumulatedData = preBuffer; // Start with any leftover data in preBuffer

    while (remainingData > 0)
    {
        size_t chunkSize = std::min(remainingData, (size_t)1024U); // Adjust chunk size based on the remaining data

        // Receive data from socket
        std::string receivedData = socket->receiveSome(chunkSize);
        accumulatedData += receivedData; // Add new data to accumulated data

        // If we have enough data, process it
        if (accumulatedData.size() >= chunkSize)
        {
            size_t toProcess = chunkSize;
            std::string dataToProcess = accumulatedData.substr(0, toProcess); // Take the required amount of data
            callback(dataToProcess);                                          // Call the callback with the processed data

            // Remove processed data from the accumulated data
            accumulatedData.erase(0, toProcess);

            remainingData -= toProcess; // Update remaining data length
        }
    }

    // Any leftover data goes into preBuffer for future reads
    preBuffer = accumulatedData;
}

size_t HttpStreamReader::getChunkSize()
{
    std::string line;
    while (true)
    {
        auto pos = preBuffer.find('\n');
        if (pos != std::string::npos)
        {
            line = preBuffer.substr(0, pos + 1);
            preBuffer.erase(0, pos + 1);

            // Remove \r\n safely
            if (!line.empty() && line.back() == '\n')
                line.pop_back();
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            break;
        }
        preBuffer += socket->receiveSome(FLUSH_THRESHOLD);
    }

    if (line.empty())
        throw std::runtime_error("Invalid or empty chunk size line");

    size_t chunkSize = std::stoul(line, nullptr, 16); // hexadecimal base
    return chunkSize;
}

void HttpStreamReader::ensureCRLF()
{
    while (preBuffer.size() < 2)
    {
        preBuffer += socket->receiveSome(FLUSH_THRESHOLD);
    }

    if (preBuffer.substr(0, 2) != "\r\n")
    {
        throw std::runtime_error("Expected CRLF after chunk data");
    }

    preBuffer.erase(0, 2);
}
