#include "Request.h"

Request::Request(const char* requestBuf)
{
    std::stringstream request(requestBuf);

    this->parsedRequest = this->parseRequest(request.str());
}

bool Request::find(const std::string& s)
{
    return this->parsedRequest.find(s) != this->parsedRequest.end();
}

std::string Request::operator[](const std::string& s)
{
    return this->parsedRequest[s];
}

std::map<std::string, std::string> Request::parseRequest(std::string req)
{
    std::stringstream ss(req);
    std::string next;
    
    // get type of request(GET, POST ...)
    ss >> next;
    std::map<std::string, std::string> parsedReq;
    parsedReq["Type"] = next;
    
    // get url
    ss >> next;
    parsedReq["URL"] = next;

    // get range
    if(req.find("Range: ") != std::string::npos)
    {
        ss.seekg(req.find("Range: "));
        ss >> next >> next;
        parsedReq["Range"] = next.substr(next.find('=') + 1, next.find('-') - 6);
    }

    return parsedReq;
}