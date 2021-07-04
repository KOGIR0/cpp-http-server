#ifndef TCP_SOCKET
#define TCP_SOCKET

#include <exception>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "Socket.h"

namespace tcp
{
    class getaddrinfoException : public std::exception
    {
    public:
        const char* what();
    };

    class listenException : public std::exception
    {
    public:
        const char* what();
    };

    class socketCreationException : public std::exception
    {
    public:
        const char* what();
    };

    class bindSocketException : public std::exception
    {
    public:
        const char* what();
    };

    class Server
    {
    public:
        Server(std::string ip, std::string port);
        tcp::Socket accept();
        int listen();
        ~Server();

    private:
        struct addrinfo* addr = NULL;
        struct addrinfo hints;
        tcp::Socket* serverSocket;
    };
}

#endif