#pragma once

#include "../isocket/isocket.hpp"
#include <string>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <iostream>
#include <sstream>

class TcpSocket : public ISocket
{
    int sockfd;
    std::string host, port;

public:
    TcpSocket(const std::string &h, const std::string &p);
    ~TcpSocket();

    void connectToServer() override;

    void sendAll(const std::string &data) override;

    std::string receiveAll() override;

    std::string receiveSome(const int size) override;

    void closeConnection() override;
};