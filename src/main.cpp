#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <math.h>
#include <exception>
#include <sys/stat.h>

#include "ServerLog.h"
#include "Server.h"
#include "Socket.h"
#include "File.h"

#define PATH_TO_PUBLIC         "../public"
#define PATH_TO_404_HTML       std::string(PATH_TO_PUBLIC) + "/html/constants/404.html"
#define PATH_TO_HEADER         std::string(PATH_TO_PUBLIC) + "/html/constants/header.html"
#define PATH_TO_RESOURCES_HTML std::string(PATH_TO_PUBLIC) + "/html/constants/resources.html"
#define PATH_TO_CONFIG         "../.config"

// serverLog file to strore server info
ServerLog serverLog;

std::map<std::string, std::string> parseRequest(std::string req)
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
    std::ifstream header(PATH_TO_HEADER);
    // *.ccs and *.js common for every page
    std::ifstream resources(PATH_TO_RESOURCES_HTML);
    // page start
    std::string result = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Piki</title>";

    if(header && resources)
    {
        std::stringstream ss;
        // add common resources(*.css, *.js) to page head
        ss << resources.rdbuf();
        result += ss.str();

        result += "</head><body>";  // close head and open body tags
        
        // add header to the page
        ss << header.rdbuf();
        result += ss.str();

        result += body + "</body></html>"; // close body and html tags
    }

    header.close();
    resources.close();

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

    for(auto p : result)
    {
        std::cout << p.first << " " << p.second << std::endl;
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
std::stringstream createResponse(std::map<std::string, std::string> request)
{
    in::File file; // file which contains data to send to client
    std::stringstream response;
    std::string Url_path = request["URL"];
    std::string pathToFile = PATH_TO_PUBLIC + Url_path;

    serverLog.write("Request url " + Url_path);
    if(fileExists(pathToFile))
    {
        serverLog.write("File format " + fileFormat(pathToFile));
    }

    std::string format = fileFormat(pathToFile);
    // send video file
    if(fileExists(pathToFile) && (format == "video/mp4" || format == "video/webm"))
    {
        file.open(pathToFile);

        if(!file.ok())
        {
            serverLog.write("!!! ERROR !!! Unable to open video file");
            return response;
        }

        // Формируем весь ответ вместе с заголовками
        // отправка файла по частям
        unsigned long read_chunk = std::pow(2, 10) * 500; // 500 Kb данных
        int rangeStart = 0;
        
        if(request.find("Range") != request.end())
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

        serverLog.write("Response header\n" + response.str() + '\n');

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
        } else {
            serverLog.write("!!! ERROR !!!  Unable to open file" + pathToFile);
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

    if(urls.find(Url_path) != urls.end()) // if url return html page
    {
        file.open(".." + urls[Url_path]);
        std::string html; // string to store html page
        if(file.ok())
        {
            html = createHtmlPage(file.read(0, file.size()));
        } else {
            serverLog.write("!!! ERROR !!! Unable to open file");
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

    // request to unknown url
    // create page not found response
    file.open(PATH_TO_404_HTML);
    std::string notFound;
    if(file.ok())
    {
        notFound = file.read(0, file.size());
    } else {
        serverLog.write("!!! ERROR !!! Unable to open style file");
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
    tcp::Server* server;

    // создаем лог фаил
    try
    {
        serverLog.open("server.log");
    }
    catch(FileOpenException& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    
    try
    {
        server = new tcp::Server(ip, port);
        server->listen();
    } catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Listening on port " + port << std::endl;

    // для того чтобы программа не завершалась после первого подключения
    // добавляем бесконечный цикл
    while(true)
    {
        std::stringstream response;      // сюда будет записываться ответ клиенту
        std::stringstream request;       // сюда будет записыватся сообщение от клиента
        // Принимаем входящие соединения,
        // сохраняем дескриптор клиента для отправки ему сообщений
        tcp::Socket client_socket = server->accept();
        
        if (!client_socket.ok())
        {
            serverLog.write("!!! ERROR !!! accept failed");
            continue;
        }

        const int max_client_buffer_size = 1024;
        char buf[max_client_buffer_size];

        int result = client_socket.recv(buf, max_client_buffer_size);

        if (result == -1)
        {
            // ошибка получения данных
            serverLog.write("!!! ERROR !!! recv failed");
        }
        else if (result == 0)
        {
            // соединение закрыто клиентом
            serverLog.write("!!! ERROR !!! connection closed...");
        }
        else if (result > 0)
        {
            serverLog.write("//// REQUEST START \\\\\\\\");
            // Мы знаем фактический размер полученных данных, поэтому ставим метку конца строки
            // В буфере запроса.
            buf[result] = '\0';
            serverLog.write("Data recieved");
            serverLog.write(buf);

            // Для удобства работы запишем полученные данные
            // в stringstream request
            request << buf;
            auto parsedRequest = parseRequest(request.str());

            // creating response
            response = createResponse(parsedRequest);
            
            // Отправляем ответ клиенту с помощью функции send
            result = client_socket.send(response.str());

            // произошла ошибка при отправке данных
            if (result == -1)
            {
                serverLog.write("!!! ERROR !!! send failed");
            }
            serverLog.write("\\\\\\\\ REQUEST END ////");
        }
    }

    // clean up
    delete server;
    serverLog.write("<<< SERVER WORK END >>>");
    return 0;
}