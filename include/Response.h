#ifndef RESPONSE
#define RESPONSE

#include <sstream>
#include <sys/stat.h>
#include <math.h>
#include <iostream>

#include "Request.h"
#include "File.h"

#define PATH_TO_CONFIG         "../.config"
#define PATH_TO_PUBLIC         "../public"
#define PATH_TO_404_HTML       std::string(PATH_TO_PUBLIC) + "/html/constants/404.html"
#define PATH_TO_HEADER         std::string(PATH_TO_PUBLIC) + "/html/constants/header.html"
#define PATH_TO_RESOURCES_HTML std::string(PATH_TO_PUBLIC) + "/html/constants/resources.html"
#define PATH_TO_FOOTER_HTML    std::string(PATH_TO_PUBLIC) + "/html/constants/footer.html"

class Response
{
public:
    Response(Request req);
    std::stringstream getResponse();

private:
    std::stringstream response;
    // map from type to function
    /*std::map<std::string, std::stringstream (Response::*)(Request)> functions = {
        {"video/mp4", &Response::createMediaResponse},
        {"video/webm", &Response::createMediaResponse}
    };*/

    std::stringstream processData(std::string data);

    std::stringstream createResponse(Request request);
    std::stringstream createFileResponse(Request request);
    std::stringstream createMediaResponse(Request request);
    std::stringstream createHTMLPageResponse(Request request);
    std::stringstream createNotFoundResponse(Request request);
    inline std::string fileFormat(const std::string& path);
    inline bool fileExists (const std::string& path);
    std::string createHtmlPage(std::string body);
    std::string addHeadToHTML(std::string html);
    std::map<std::string, std::string> loadURLs();
    const std::string getHTTPStatusCodeStr(int statusCode);
};

#endif