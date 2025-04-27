#include "tcp-socket.hpp"

TcpSocket::TcpSocket(const std::string &h, const std::string &p) : sockfd(-1), host(h), port(p) {}

TcpSocket::~TcpSocket()
{
    this->closeConnection();
}

void TcpSocket::connectToServer()
{
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
        throw std::runtime_error("connection failed");

    std::clog << "connected to server" << std::endl;
}

void TcpSocket::sendAll(const std::string &data)
{
    size_t totalSent = 0;
    while (totalSent < data.size())
    {
        ssize_t sent = send(sockfd, data.c_str() + totalSent, data.size() - totalSent, 0);
        if (sent < 0)
            throw std::runtime_error("send failed");
        totalSent += sent;
    }
    std::clog << "Sent " << totalSent << " bytes data to server" << std::endl;
}

std::string TcpSocket::receiveAll()
{
    std::string result;
    char buffer[4096];
    ssize_t bytesRead;
    while ((bytesRead = recv(sockfd, buffer, sizeof(buffer), 0)) > 0)
    {
        result.append(buffer, bytesRead);
    }
    if (bytesRead < 0)
        throw std::runtime_error("recv failed");

    std::clog << bytesRead << " bytes data received from server" << std::endl;
    return result;
}

std::string TcpSocket::receiveSome(const int size)
{
    char buffer[1024];
    int totalLength = 0;
    std::string result;

    while (totalLength < size)
    {
        int bytesRead = recv(this->sockfd, buffer, sizeof(buffer) - 1, 0);

        if (bytesRead < 0)
            throw std::runtime_error("failed to recv data");
        totalLength += bytesRead;
        result.append(buffer);
    }
    return result;
}

void TcpSocket::closeConnection()
{
    if (sockfd != -1)
    {
        close(sockfd);
        sockfd = -1;
    }
}
