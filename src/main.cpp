#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <math.h>
#include <exception>
#include <sys/stat.h>
#include <memory>

#include "ServerLog.h"
#include "Server.h"
#include "Socket.h"
#include "File.h"
#include "Request.h"

#define PATH_TO_PUBLIC         "../public"
#define PATH_TO_404_HTML       std::string(PATH_TO_PUBLIC) + "/html/constants/404.html"
#define PATH_TO_HEADER         std::string(PATH_TO_PUBLIC) + "/html/constants/header.html"
#define PATH_TO_RESOURCES_HTML std::string(PATH_TO_PUBLIC) + "/html/constants/resources.html"
#define PATH_TO_FOOTER_HTML    std::string(PATH_TO_PUBLIC) + "/html/constants/footer.html"
#define PATH_TO_CONFIG         "../.config"

std::string getPort()
{
    std::ifstream config(PATH_TO_CONFIG);
    std::string port = "3000";
    if(!config)
    {
        return port;
    }

    while(!config.eof())
    {
        int bufSize = 1024;
        char buf[bufSize];
        std::stringstream line;
        config.getline(buf, bufSize);
        line << buf;

        std::string s;
        line >> s;
        if(s == "PORT")
        {
            line >> s;
            return s;
        }
    }
    return port;
}

std::string createHtmlPage(std::string body)
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

std::map<std::string, std::string> loadURLs()
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

// check if file exists
// path - path to file
inline bool fileExists (const std::string& path)
{
  struct stat buffer;   
  return (stat (path.c_str(), &buffer) == 0);
}

// find file format
// path - path to file
inline std::string fileFormat(const std::string& path)
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

// creates http response
// response - response storage
std::stringstream createResponse(Request request)
{
    in::File file; // file which contains data to send to client
    std::stringstream response;
    std::string Url_path = request["URL"];
    std::string pathToFile = PATH_TO_PUBLIC + Url_path;

    std::string format = fileFormat(pathToFile);
    // send video file
    if(fileExists(pathToFile) && (format == "video/mp4" || format == "video/webm"))
    {
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

    if(fileExists(pathToFile) && format != "unknown")
    {
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

    // url for different pages(map<url, path_to_file>)
    std::map<std::string, std::string> urls = loadURLs();

    if(urls.find(Url_path) != urls.end() && fileExists(".." + urls[Url_path])) // if url return html page
    {
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

    std::cout << "Sending page not found" << std::endl << std::endl;

    // request to unknown url
    // create page not found response
    file.open(PATH_TO_404_HTML);
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

int main()
{
    std::string port = getPort(); // номер порта нашего HTTP сервера
    std::string ip = "127.0.0.1"; // ip адресс сервера
    std::unique_ptr<tcp::Server> server;
    // redirect stdout and stderr to files to write logging info
    freopen( "serverLog.txt", "w", stdout );
    freopen( "serverErrors.txt", "w", stderr );
    
    try
    {
        // create server and start listening for connections
        server = std::make_unique<tcp::Server>(ip, port);
        server->listen();
    } catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Listening on port " + port << std::endl;

    // для того чтобы программа не завершалась после первого подключения
    // добавляем бесконечный цикл
    while(true)
    {
        std::stringstream response;      // сюда будет записываться ответ клиенту
        // Принимаем входящие соединения,
        // сохраняем дескриптор клиента для отправки ему сообщений
        tcp::Socket client_socket = server->accept();
        
        if (!client_socket.ok())
        {
            std::cerr << "!!! ERROR !!! accept failed" << std::endl;
            continue;
        }

        const int max_client_buffer_size = 1024;
        char buf[max_client_buffer_size];

        int result = client_socket.recv(buf, max_client_buffer_size);

        if (result == -1)
        {
            // ошибка получения данных
            std::cerr << "!!! ERROR !!! recv failed" << std::endl;
        }
        else if (result == 0)
        {
            // соединение закрыто клиентом
            std::cerr << "!!! ERROR !!! connection closed..." << std::endl;
        }
        else if (result > 0)
        {
            std::cout << "//// REQUEST START \\\\\\\\" << std::endl;
            // Мы знаем фактический размер полученных данных, поэтому ставим метку конца строки
            // В буфере запроса.
            buf[result] = '\0';
            std::cout << "Data recieved" << std::endl;
            std::cout << buf << std::endl;

            // Для удобства работы запишем полученные данные
            // в stringstream request
            Request request(buf);

            // creating response
            response = createResponse(request);
            
            // Отправляем ответ клиенту с помощью функции send
            result = client_socket.send(response.str());

            // произошла ошибка при отправке данных
            if (result == -1)
            {
                std::cerr << "!!! ERROR !!! send failed" << std::endl;
            }
            std::cout << "\\\\\\\\ REQUEST END ////" << std::endl << std::endl;
        }
    }

    // clean up
    std::cout << "<<< SERVER WORK END >>>" << std::endl;
    return 0;
}