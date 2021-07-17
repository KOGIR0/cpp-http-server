#include "Response.h"

#include <regex>

Response::Response(Request req)
{
    std::string requestType = req["Type"];
    if(requestType == "GET")
    {
        this->response = this->createResponse(req);
    } else if (requestType == "POST")
    {
        // create response to post request
        std::cout << "Data: " << req["Data"] << std::endl;
        this->response = this->processData(req["Data"]);
    }
}

std::stringstream Response::processData(std::string data)
{
    std::stringstream response;
    response << "HTTP/1.1 200 " << getHTTPStatusCodeStr(200) << "\r\n"
        << "Version: HTTP/1.1\r\n"
        << "Content-Type: " << "text/json" <<"\r\n"
        << "Content-Length: " << data.length()
        << "\r\n\r\n"
        << data;

    return response;
}

std::stringstream Response::getResponse()
{
    return std::move(response);
}

std::stringstream Response::createResponse(Request request)
{
    std::string Url_path = request["URL"];
    std::string pathToFile = PATH_TO_PUBLIC + Url_path;
    std::string format = fileFormat(pathToFile);

    // send video file
    if(fileExists(pathToFile) && (format == "video/mp4" || format == "video/webm"))
    {
        return createMediaResponse(request);
    }

    if(fileExists(pathToFile) && format != "unknown")
    {
        return createFileResponse(request);
    }

    // url for different pages(map<url, path_to_file>)
    std::map<std::string, std::string> urls = loadURLs();

    // if url return html page
    if(urls.find(Url_path) != urls.end() && fileExists(".." + urls[Url_path]))
    {
        return createHTMLPageResponse(request);
    }

    std::cout << "Sending page not found" << std::endl << std::endl;

    return createNotFoundResponse(request);
}

std::stringstream Response::createFileResponse(Request request)
{
    in::File file;
    std::string Url_path = request["URL"];
    std::string pathToFile = PATH_TO_PUBLIC + Url_path;
    std::stringstream response;
    std::string format = fileFormat(pathToFile);

    file.open(pathToFile);
    std::string content;
    if(file.ok())
    {
        content = file.read(0, file.size());
    }

    // Формируем весь ответ вместе с заголовками
    response << "HTTP/1.1 200 " << getHTTPStatusCodeStr(200) << "\r\n"
        << "Version: HTTP/1.1\r\n"
        << "Content-Type: " << format <<"\r\n"
        << "Content-Length: " << content.length()
        << "\r\n\r\n"
        << content;
    
    return response;
}

std::stringstream Response::createMediaResponse(Request request)
{
    in::File file;
    std::string Url_path = request["URL"];
    std::string pathToFile = PATH_TO_PUBLIC + Url_path;
    std::stringstream response;
    std::string format = fileFormat(pathToFile);

    file.open(pathToFile);

    if(!file.ok())
    {
        return response;
    }

    // Формируем весь ответ вместе с заголовками
    // отправка файла по частям
    unsigned long read_chunk = std::pow(2, 10) * 500; // 500 Kb данных
    int rangeStart = 0;
    
    if(request.find("Range"))
    {
        rangeStart = std::stoi(request["Range"]);
    }

    std::string read_str = file.read(rangeStart, read_chunk);
    
    response << "HTTP/1.1 206 " << getHTTPStatusCodeStr(206) << "\r\n"
        << "Content-Range: bytes " << rangeStart << "-"
        << (rangeStart + read_str.length()) << "/" << file.size() << "\r\n"
        << "Accept-Ranges: bytes\r\n"
        << "Content-Type: " << format << "\r\n"
        << "Content-Length: " << read_str.length()
        << "\r\n\r\n";

    response << read_str;
    return response;
}

std::stringstream Response::createHTMLPageResponse(Request request)
{
    in::File file;
    std::stringstream response;
    std::string Url_path = request["URL"];
    // url for different pages(map<url, path_to_file>)
    std::map<std::string, std::string> urls = loadURLs();

    file.open(".." + urls[Url_path]);
    std::string html; // string to store html page
    if(file.ok())
    {
        html = createHtmlPage(file.read(0, file.size()));
    }

    // Формируем весь ответ вместе с заголовками
    response << "HTTP/1.1 200 " << getHTTPStatusCodeStr(200) << "\r\n"
        << "Version: HTTP/1.1\r\n"
        << "Content-Type: text/html; charset=utf-8\r\n"
        << "Content-Length: " << html.length()
        << "\r\n\r\n"
        << html;

    return response;
}

std::stringstream Response::createNotFoundResponse(Request request)
{
    in::File file; // file which contains data to send to client
    // request to unknown url
    // create page not found response
    file.open(PATH_TO_404_HTML);
    std::stringstream response;
    std::string notFound;
    if(file.ok())
    {
        notFound = file.read(0, file.size());
    }

    response << "HTTP/1.1 404 " << getHTTPStatusCodeStr(404) << "\r\n"
        << "Version: HTTP/1.1\r\n"
        << "Content-Type: text/html\r\n"
        << "Content-Length: " << notFound.length()
        << "\r\n\r\n"
        << notFound;

    return response;
}

// find file format
// path - path to file
inline std::string Response::fileFormat(const std::string& path)
{
    std::map<std::string, std::string> formats{
        {".mp4", "video/mp4"},
        {".webm", "video/webm"},
        {".css", "text/css"},
        {".js", "text/js"},
        {".png", "image/png"}
    };

    for(auto p : formats)
    {
        if(path.find(p.first) != std::string::npos)
            return p.second;
    }

    return "unknown";
}

// check if file exists
// path - path to file
inline bool Response::fileExists (const std::string& path)
{
    struct stat buffer;   
    return (stat (path.c_str(), &buffer) == 0);
}

std::string Response::addHeadToHTML(std::string html)
{
    // *.ccs and *.js common for every page
    in::File resources(PATH_TO_RESOURCES_HTML);

    html += "<head><meta charset=\"utf-8\"><title>Piki</title>";
    if(resources.ok())
        html += resources.read(0, resources.size());
    html += "</head>";

    return html;
}

std::string Response::createHtmlPage(std::string body)
{
    // page header
    in::File header(PATH_TO_HEADER);
    // page footer
    in::File footer(PATH_TO_FOOTER_HTML);
    // page start
    std::string result = "<!DOCTYPE html><html>";

    if(header.ok())
    {
        result = addHeadToHTML(result);

        result += "<body>";  // close head and open body tags
        
        // add header to the page
        result += header.read(0, header.size());

        // add left side to the page
        result += "<div id=\"page-content\"><div id=\'left-side\'>";

        // add body to the page
        result += "</div><div id=\"main-content\">" + body + "</div><div id=\'right-side\'></div></div>";

        // add footer to the page
        result += footer.read(0, header.size());

        result += "</body></html>"; // close body and html tags
    }

    return result;
}

std::map<std::string, std::string> Response::loadURLs()
{
    std::map<std::string, std::string> result;
    std::ifstream config(PATH_TO_CONFIG);
    if(!config)
    {
        return result;
    }
    
    int bufSize = 1024;
    char buf[bufSize];
    std::string key, url, pathToFile;
    while(!config.eof())
    {
        std::stringstream line;
        config.getline(buf, bufSize);
        line << buf;
        line >> key;
        
        if(key == "URL")
        {
            line >> url >> pathToFile;
            result[url] = pathToFile;
        }
    }
    
    return result;
}

const std::string Response::getHTTPStatusCodeStr(int statusCode) {
  switch (statusCode) {
    case 100: return "Continue";
    case 101: return "Switching Protocols";
    case 102: return "Processing";
    case 200: return "OK";
    case 201: return "Created";
    case 202: return "Accepted";
    case 203: return "Non-authoritative Information";
    case 204: return "No Content";
    case 205: return "Reset Content";
    case 206: return "Partial Content";
    case 207: return "Multi-Status";
    case 208: return "Already Reported";
    case 226: return "IM Used";
    case 300: return "Multiple Choices";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 303: return "See Other";
    case 304: return "Not Modified";
    case 305: return "Use Proxy";
    case 307: return "Temporary Redirect";
    case 308: return "Permanent Redirect";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 402: return "Payment Required";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 406: return "Not Acceptable";
    case 407: return "Proxy Authentication Required";
    case 408: return "Request Timeout";
    case 409: return "Conflict";
    case 410: return "Gone";
    case 411: return "Length Required";
    case 412: return "Precondition Failed";
    case 413: return "Payload Too Large";
    case 414: return "Request-URI Too Long";
    case 415: return "Unsupported Media Type";
    case 416: return "Requested Range Not Satisfiable";
    case 417: return "Expectation Failed";
    case 418: return "I'm a teapot";
    case 421: return "Misdirected Request";
    case 422: return "Unprocessable Entity";
    case 423: return "Locked";
    case 424: return "Failed Dependency";
    case 426: return "Upgrade Required";
    case 428: return "Precondition Required";
    case 429: return "Too Many Requests";
    case 431: return "Request Header Fields Too Large";
    case 444: return "Connection Closed Without Response";
    case 451: return "Unavailable For Legal Reasons";
    case 499: return "Client Closed Request";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    case 504: return "Gateway Timeout";
    case 505: return "HTTP Version Not Supported";
    case 506: return "Variant Also Negotiates";
    case 507: return "Insufficient Storage";
    case 508: return "Loop Detected";
    case 510: return "Not Extended";
    case 511: return "Network Authentication Required";
    case 599: return "Network Connect Timeout Error";
    default: return "OK";
  }
}