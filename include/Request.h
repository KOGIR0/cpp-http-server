#ifndef REQUEST
#define REQUEST

#include <string>
#include <map>
#include <sstream>
#include <iostream>

// creates request from buffer recived from user
class Request
{
public:
    Request(){}
    Request(const char* requestBuf);
    bool find(const std::string& s);
    std::string operator[](const std::string& s);

private:
    std::string data;
    std::map<std::string, std::string> parsedRequest;
    std::map<std::string, std::string> parseRequest(std::string req);
};

#endif