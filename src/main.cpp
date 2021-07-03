#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <math.h>
#include <exception>

#include "ServerLog.h"
#include "Server.h"
#include "Socket.h"
#include "File.h"

#define PATH_TO_PUBLIC         "../public"
#define PATH_TO_404_HTML       std::string(PATH_TO_PUBLIC) + "/html/constants/404.html"
#define PATH_TO_HEADER         std::string(PATH_TO_PUBLIC) + "/html/constants/header.html"
#define PATH_TO_RESOURCES_HTML std::string(PATH_TO_PUBLIC) + "/html/constants/resources.html"
#define PATH_TO_CONFIG         "../.config"

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
        }
        result[url] = pathToFile;
    }
    
    return result;
}

int main()
{
    std::string port = getPort(); // номер порта нашего HTTP сервера
    std::string ip = "127.0.0.1"; // ip адресс сервера
    ServerLog log;
    tcp::Server* server;
    // url for different pages(map<url, path_to_file>)
    std::map<std::string, std::string> urls = loadURLs();

    // создаем лог фаил
    try
    {
        log.open("server.log");
    }
    catch(FileOpenException& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    
    try
    {
        server = new tcp::Server(ip, port);
    } catch (tcp::bindSocketException& e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }

    int result = server->listen();
    if(result == -1)
    {
        log.write("!!! ERROR !!! listen");
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
        in::File file; // file which contains data to send to client
        
        if (!client_socket.ok())
        {
            log.write("!!! ERROR !!! accept failed");
            continue;
        }

        const int max_client_buffer_size = 1024;
        char buf[max_client_buffer_size];

        int result = client_socket.recv(buf, max_client_buffer_size);

        if (result == -1)
        {
            // ошибка получения данных
            log.write("!!! ERROR !!! recv failed");
        }
        else if (result == 0)
        {
            // соединение закрыто клиентом
            log.write("!!! ERROR !!! connection closed...");
        }
        else if (result > 0)
        {
            log.write("//// REQUEST START \\\\\\\\");
            // Мы знаем фактический размер полученных данных, поэтому ставим метку конца строки
            // В буфере запроса.
            buf[result] = '\0';
            log.write("Data recieved");
            log.write(buf);

            // Для удобства работы запишем полученные данные
            // в stringstrem request
            request << buf;
            auto parsedRequest = parseRequest(request.str());
            
            std::string Url_path;
            Url_path = parsedRequest["URL"];
            log.write("Request url " + Url_path);
            
            // Если запрашивается видео mp4
            if(Url_path.find(".mp4") != std::string::npos)
            {
                file.open(PATH_TO_PUBLIC + Url_path);

                if(!file.ok())
                {
                    log.write("!!! ERROR !!! Unable to open video file");
                    continue;
                }

                // Формируем весь ответ вместе с заголовками
                // отправка файла по частям
                unsigned long read_chunk = std::pow(2, 10) * 500 ; // 500 Kb данных
                int rangeStart = 0;
                
                if(parsedRequest.find("Range") != parsedRequest.end())
                {
                    rangeStart = std::stoi(parsedRequest["Range"]);
                }

                std::string read_str = file.read(rangeStart, read_chunk);
                
                response << "HTTP/1.1 206 Partial Content\r\n"
                    << "Content-Range: bytes " << rangeStart << "-"
                    << (rangeStart + read_str.length()) << "/" << file.size() << "\r\n"
                    << "Accept-Ranges: bytes\r\n"
                    << "Content-Type: video/mp4\r\n"
                    << "Content-Length: " << read_str.length()
                    << "\r\n\r\n";

                log.write("Response header\n" + response.str() + '\n');

                response << read_str;
            }
            else if(Url_path.find(".webm") != std::string::npos)
            {
                file.open(PATH_TO_PUBLIC + Url_path);
                if(!file.ok())
                {
                    log.write("!!! ERROR !!! Unable to open video file");
                }

                // Формируем весь ответ вместе с заголовками
                // отправка файла по частям
                unsigned long read_chunk = std::pow(2, 10) * 500;
                int rangeStart = 0;
                
                if(parsedRequest.find("Range") != parsedRequest.end())
                {
                    rangeStart = std::stoi(parsedRequest["Range"]);
                }

                std::string read_str = file.read(rangeStart, read_chunk);

                log.write("Bytes read: " + std::to_string(read_str.length()) + " In buf: " + std::to_string(read_str.length()));
                response << "HTTP/1.1 206 Partial Content\r\n"
                    << "Content-Range: bytes " << rangeStart << "-"
                    << (rangeStart + read_str.length()) << "/" << file.size() << "\r\n"
                    << "Accept-Ranges: bytes\r\n"
                    << "Content-Type: video/webm\r\n"
                    << "Content-Length: " << read_str.length()
                    << "\r\n\r\n";

                log.write("Response header\n" + response.str() + '\n');

                response << read_str;
            }
            else if(urls.find(Url_path) != urls.end()) // if url return html page
            {
                file.open(".." + urls[Url_path]);
                std::string html; // string to store html page
                if(file.ok())
                {
                    html = createHtmlPage(file.read(0, file.size()));
                } else {
                    log.write("!!! ERROR !!! Unable to open file");
                }

                // Формируем весь ответ вместе с заголовками
                response << "HTTP/1.1 200 OK\r\n"
                    << "Version: HTTP/1.1\r\n"
                    << "Content-Type: text/html; charset=utf-8\r\n"
                    << "Content-Length: " << html.length()
                    << "\r\n\r\n"
                    << html;
            }
            else if (Url_path.find(".css") != std::string::npos)
            {
                file.open(PATH_TO_PUBLIC + Url_path);
                std::string css;
                if(file.ok())
                {
                    css = file.read(0, file.size());
                } else {
                    log.write("!!! ERROR !!!  Unable to open style file");
                }

                // Формируем весь ответ вместе с заголовками
                response << "HTTP/1.1 200 OK\r\n"
                    << "Version: HTTP/1.1\r\n"
                    << "Content-Type: text/css\r\n"
                    << "Content-Length: " << css.length()
                    << "\r\n\r\n"
                    << css;
            }
            else if(Url_path.find(".png") != std::string::npos)
            {
                file.open(PATH_TO_PUBLIC + Url_path);
                std::string png;
                if(file.ok())
                {
                    png = file.read(0, file.size());
                } else {
                    log.write("!!! ERROR !!! Unable to open icon file");
                }

                response << "HTTP/1.1 200 OK\r\n"
                    << "Version: HTTP/1.1\r\n"
                    << "Content-Type: image/png\r\n"
                    << "Content-Length: " << png.length()
                    << "\r\n\r\n"
                    << png;
            }
            else if(Url_path.find(".js") != std::string::npos)
            {
                file.open(PATH_TO_PUBLIC + Url_path);
                std::string js;
                if(file.ok())
                {
                    js = file.read(0, file.size());
                } else {
                    log.write("!!! ERROR !!! Unable to open style file");
                }

                // Формируем весь ответ вместе с заголовками
                response << "HTTP/1.1 200 OK\r\n"
                    << "Version: HTTP/1.1\r\n"
                    << "Content-Type: text/js\r\n"
                    << "Content-Length: " << js.length()
                    << "\r\n\r\n"
                    << js;
            } else {
                // url not Found
                file.open(PATH_TO_404_HTML);
                std::string notFound;
                if(file.ok())
                {
                    notFound = file.read(0, file.size());
                } else {
                    log.write("!!! ERROR !!! Unable to open style file");
                }

                response << "HTTP/1.1 404 Not Found\r\n"
                    << "Version: HTTP/1.1\r\n"
                    << "Content-Type: text/html\r\n"
                    << "Content-Length: " << notFound.length()
                    << "\r\n\r\n"
                    << notFound;
            }
            // Отправляем ответ клиенту с помощью функции send
            result = client_socket.send(response.str());

            // произошла ошибка при отправке данных
            if (result == -1)
            {
                log.write("!!! ERROR !!! send failed");
            }
            log.write("\\\\\\\\ REQUEST END ////");
        }
    }

    // clean up
    delete server;
    log.write("<<< SERVER WORK END >>>");
    return 0;
}