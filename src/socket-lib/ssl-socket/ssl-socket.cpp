#include "ssl-socket.hpp"

SslSocket::SslSocket(const std::string &host, const std::string &port)
    : host(host), port(port), ctx(nullptr), ssl(nullptr), sockfd(-1) {}

SslSocket::~SslSocket()
{
    this->closeConnection();
}

void SslSocket::connectToServer()
{
    OPENSSL_init_ssl(0, nullptr);

    ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx)
    {
        ERR_print_errors_fp(stderr);
        throw std::runtime_error("failed to create ssl context!");
    }

    // currently dont verify certificate
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);

    struct addrinfo hints{}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &res) != 0)
        throw std::runtime_error("getaddrinfo failed");

    const struct addrinfo *temp = res;
    while (temp != nullptr)
    {
        int sockfd_temp = socket(temp->ai_family, temp->ai_socktype, temp->ai_protocol);
        if (sockfd_temp < 0)
        {
            temp = temp->ai_next;
            continue;
        }

        if (::connect(sockfd_temp, temp->ai_addr, temp->ai_addrlen) == 0)
        {
            sockfd = sockfd_temp; // Success => save
            break;
        }

        close(sockfd_temp);
        temp = temp->ai_next;
    }

    freeaddrinfo(res);

    if (temp == nullptr)
        throw std::runtime_error("TCP handshake failed");

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);

    if (!SSL_set_tlsext_host_name(ssl, host.c_str()))
    {
        ERR_print_errors_fp(stderr);
        throw std::runtime_error("Failed to set TLS hostname (SNI)");
    }

    if (SSL_connect(ssl) <= 0)
    {
        ERR_print_errors_fp(stderr);
        throw std::runtime_error("TLS handshake failed");
    }
    std::clog << "securely connected to server" << std::endl;
}

void SslSocket::sendAll(const std::string &data)
{
    size_t totalSent = 0;
    while (totalSent < data.size())
    {
        int sent = SSL_write(ssl, data.c_str() + totalSent, data.size() - totalSent);
        if (sent <= 0)
        {
            int err = SSL_get_error(ssl, sent);
            throw std::runtime_error("SSL_write failed with error: " + std::to_string(err));
        }
        totalSent += sent;
    }
    std::clog << "securely sent " << totalSent << " bytes data" << std::endl;
}

std::string SslSocket::receiveAll()
{
    std::string result;
    char buffer[4096];
    int bytesRead;

    while (true)
    {
        bytesRead = SSL_read(ssl, buffer, sizeof(buffer));
        if (bytesRead > 0)
        {
            result.append(buffer, bytesRead);
        }
        else
        {
            int err = SSL_get_error(ssl, bytesRead);
            if (err == SSL_ERROR_ZERO_RETURN)
                // Proper shutdown, no more data
                break;
            else if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
                // Non-fatal, just retry
                continue;
            else
                throw std::runtime_error("SSL_read failed with error: " + std::to_string(err));
        }
    }
    return result;
}

std::string SslSocket::receiveSome(const int size)
{
    std::string result;
    char buffer[1024];
    int bytesRead;
    int totalBytesRead = 0;

    // read only the needed amount of data
    while (totalBytesRead < size)
    {
        bytesRead = SSL_read(ssl, buffer, sizeof(buffer));
        if (bytesRead > 0)
        {
            result.append(buffer, bytesRead);
            totalBytesRead += bytesRead;
        }
        else
        {
            int err = SSL_get_error(ssl, bytesRead);
            if (err == SSL_ERROR_ZERO_RETURN)
                // Proper shutdown, no more data
                break;
            else if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
                // Non-fatal, just retry
                continue;
            else
                throw std::runtime_error("SSL_read failed with error: " + std::to_string(err));
        }
    }
    return result;
}

void SslSocket::closeConnection()
{
    if (ssl)
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        OPENSSL_cleanup();
        ssl = nullptr;
        std::clog << "ssl freed" << std::endl;
    }

    if (ctx)
    {
        SSL_CTX_free(ctx);
        ctx = nullptr;
        std::clog << "ctx freed" << std::endl;
    }

    if (sockfd >= 0)
    {
        close(sockfd);
        sockfd = -1;
        std::clog << "socket freed" << std::endl;
    }
}