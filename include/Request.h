#ifndef REQUEST
#define REQUEST

#include <string>
#include <map>
#include <sstream>

// creates request from buffer recived from user
class Request
{
public:
    Request(){}
    Request(const char* requestBuf);
    bool find(const std::string& s);
    std::string operator[](const std::string& s);

private:
    std::map<std::string, std::string> parsedRequest;
    std::map<std::string, std::string> parseRequest(std::string req);
};

#endif