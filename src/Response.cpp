#include "Response.h"

Response::Response(Request req)
{
    this->response = this->createResponse(req);
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
    response << "HTTP/1.1 200 OK\r\n"
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
    
    response << "HTTP/1.1 206 Partial Content\r\n"
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
    response << "HTTP/1.1 200 OK\r\n"
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

    response << "HTTP/1.1 404 Not Found\r\n"
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

std::string Response::createHtmlPage(std::string body)
{
    // page header
    in::File header(PATH_TO_HEADER);
    // *.ccs and *.js common for every page
    in::File resources(PATH_TO_RESOURCES_HTML);
    // page footer
    in::File footer(PATH_TO_FOOTER_HTML);
    // page start
    std::string result = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Piki</title>";

    if(header.ok() && resources.ok())
    {
        // add common resources(*.css, *.js) to page head
        result += resources.read(0, resources.size());

        result += "</head><body>";  // close head and open body tags
        
        // add header to the page
        result += header.read(0, header.size());

        // add body to the page
        result += "<div id=\"main-content\">" + body + "</div>";

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