#ifndef SOCKET
#define SOCKET

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string>

namespace tcp
{
    class Socket
    {
    public:
        Socket(int fd);
        ~Socket();
        // read n bytes
        // returns the number read or error
        int recv(void* buf, size_t n);
        int send(const std::string& s);
        int bind(const addrinfo* addr);
        int accept();
        int listen(int n);
        bool ok();

    private:
        int fd;
    };
}

#endif