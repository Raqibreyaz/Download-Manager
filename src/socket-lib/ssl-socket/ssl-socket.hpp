#pragma once

#include "../isocket/isocket.hpp"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <stdexcept>
#include <iostream>

class SslSocket : public ISocket
{
    std::string host;
    std::string port;
    SSL_CTX *ctx;
    SSL *ssl;
    int sockfd;

public:
    SslSocket(const std::string &host, const std::string &port);
    ~SslSocket();

    void connectToServer() override;
    std::string receiveAll() override;
    std::string receiveSome(const int size) override;
    void sendAll(const std::string &data) override;
    void closeConnection() override;
};