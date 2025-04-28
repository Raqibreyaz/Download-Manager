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
    if (contentLength == 0)
    {
        readContent(contentLength, callback);
        return;
    }

    const size_t DEFAULT_CHUNK_SIZE = 8192;
    const int MAX_EMPTY_RETRIES = 5; // max retries if no data received
    const int RETRY_DELAY_MS = 100;  // wait 100 ms between retries

    std::string accumulatedData = preBuffer;
    size_t remainingData = contentLength - accumulatedData.size();

    while (remainingData > 0)
    {
        size_t chunkSize = std::min(remainingData, DEFAULT_CHUNK_SIZE);

        int emptyRetries = 0;
        while (accumulatedData.size() < chunkSize)
        {
            std::string receivedData = socket->receiveSome(chunkSize - accumulatedData.size());
            if (receivedData.empty())
            {
                if (++emptyRetries > MAX_EMPTY_RETRIES)
                {
                    std::cerr << "\nConnection lost or server closed prematurely.\n";
                    break; // break inner loop after retries
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS)); // small wait before retry
                continue;
            }
            emptyRetries = 0; // reset retry counter if some data received
            accumulatedData += receivedData;
        }

        if (accumulatedData.empty())
        {
            break; // no more data available after retries
        }

        size_t processSize = std::min(chunkSize, accumulatedData.size());
        std::string dataToProcess = accumulatedData.substr(0, processSize);
        callback(dataToProcess);
        accumulatedData.erase(0, processSize);
        remainingData -= processSize;

        double downloadStatus = (((contentLength - remainingData) * 1.0) / contentLength) * 100;
        downloadStatus = std::round(downloadStatus * 10.0) / 10.0;
        std::clog << "\r" << downloadStatus << "% downloaded" << std::flush;
    }

    // After loop finishes, check if any remaining data is there
    if (!accumulatedData.empty())
    {
        // if there's remaining data (even when remainingData = 0), process it
        callback(accumulatedData);
        accumulatedData.clear(); // Clear it after processing
    }

    std::clog << "\r100% downloaded" << std::flush
              << std::endl
              << "Download Finished";

    preBuffer = accumulatedData; // Final leftover data will go into preBuffer
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
