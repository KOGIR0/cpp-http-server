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
    if(this->parsedRequest.find(s) != this->parsedRequest.end())
    {
        return this->parsedRequest[s];
    }
    return "";
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

    while(ss >> next)
    {
        if(next == "Content-Length:")
        {
            ss >> next;
            parsedReq["Content-Length"] = next;
            break;
        }
    }

    std::string data = ss.str();
    if(parsedReq.find("Content-Length") != parsedReq.end())
    {
        parsedReq["Data"] = data.substr(data.size() - std::stoi(parsedReq["Content-Length"]), std::stoi(parsedReq["Content-Length"]));
    }

    return parsedReq;
}