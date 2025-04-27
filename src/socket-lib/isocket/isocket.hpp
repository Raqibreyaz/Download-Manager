#pragma once

#include <string>

class ISocket
{
private:
    /* data */
public:
    virtual void connectToServer() = 0;
    virtual void sendAll(const std::string &data) = 0;
    virtual std::string receiveAll() = 0;
    virtual std::string receiveSome(const int size) = 0;
    virtual void closeConnection() = 0;
    virtual ~ISocket();
};
