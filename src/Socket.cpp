#include "Socket.h"

tcp::Socket::Socket(int fd)
{
    this->fd = fd;
}

tcp::Socket::~Socket()
{
    close(this->fd);
}

bool tcp::Socket::ok()
{
    return this->fd != -1;
}

int tcp::Socket::recv(void* buf, size_t n)
{
    return ::recv(this->fd, buf, n, 0);
}

int tcp::Socket::send(const std::string& s)
{
    return ::send(this->fd, s.c_str(), s.length(), 0);
}

int tcp::Socket::bind(const addrinfo* addr)
{
    return ::bind(this->fd, addr->ai_addr, (int)addr->ai_addrlen);
}

int tcp::Socket::accept()
{
    return ::accept(this->fd, NULL, NULL);
}

int tcp::Socket::listen(int n)
{
    return ::listen(this->fd, n);
}