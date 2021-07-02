#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fstream>
#include <map>
#include <math.h>
#include <exception>

#include "ServerLog.h"
#include "Server.h"
#include "Socket.h"

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
    while(1)
    {
        std::stringstream response;      // сюда будет записываться ответ клиенту
        std::stringstream response_body; // тело ответа
        std::stringstream request;       // сюда будет записыватся сообщение от клиента
        // Принимаем входящие соединения,
        // сохраняем дескриптор клиента для отправки ему сообщений
        tcp::Socket client_socket = server->accept();
        std::ifstream file; // file to send to client
        
        if (!client_socket.ok())
        {
            log.write("!!! ERROR !!! accept failed");
            return 1;
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
            
            std::string s;
            s = parsedRequest["URL"];
            log.write("Request url " + s);

            if(s == "/end")break;

            // Если запрашивается видео mp4
            if(s.find(".mp4") != std::string::npos)
            {
                file.open(PATH_TO_PUBLIC + s);
                if(file)
                {
                    response_body << file.rdbuf();
                    std::cout << response_body.str().length() << std::endl;
                } else {
                    log.write("!!! ERROR !!! Unable to open video file");
                }

                // Формируем весь ответ вместе с заголовками
                // отправка файла по частям
                unsigned long read_chunk = std::pow(2, 10) * 500 ; // 500 Kb данных
                std::string read_str;
                int rangeStart = 0;
                
                if(parsedRequest.find("Range") != parsedRequest.end())
                {
                    rangeStart = std::stoi(parsedRequest["Range"]);
                }

                if(rangeStart + read_chunk <= response_body.str().length())
                {
                    read_str = response_body.str().substr(rangeStart, read_chunk);
                } else {
                    read_str = response_body.str().substr(rangeStart, response_body.str().length() - rangeStart);
                }

                response << "HTTP/1.1 206 Partial Content\r\n"
                << "Content-Range: bytes " << parsedRequest["Range"] << "-" << (rangeStart + read_str.length() - 1) << "/" << response_body.str().length() << "\r\n"
                << "Accept-Ranges: bytes\r\n"
                << "Content-Type: video/mp4\r\n"
                << "Content-Length: " << read_str.length()
                << "\r\n\r\n";

                log.write("Response header\n" + response.str() + '\n');

                response << read_str;
            }
            else if(s.find(".webm") != std::string::npos)
            {
                file.open(PATH_TO_PUBLIC + s);
                if(file)
                {
                    response_body << file.rdbuf();
                } else {
                    log.write("!!! ERROR !!! Unable to open video file");
                }

                // Формируем весь ответ вместе с заголовками
                // отправка файла по частям
                unsigned long read_chunk = std::pow(2, 10) * 500;
                std::string read_str;
                int rangeStart = 0;
                
                if(parsedRequest.find("Range") != parsedRequest.end())
                {
                    rangeStart = std::stoi(parsedRequest["Range"]);
                }

                if(rangeStart + read_chunk <= response_body.str().length())
                {
                    read_str = response_body.str().substr(rangeStart, read_chunk);
                } else {
                    read_str = response_body.str().substr(rangeStart, response_body.str().length() - rangeStart);
                }

                log.write("Bytes read: " + std::to_string(read_str.length()) + " In buf: " + std::to_string(read_str.length()));
                response << "HTTP/1.1 206 Partial Content\r\n"
                << "Content-Range: bytes " << parsedRequest["Range"] << "-" << (rangeStart + read_str.length() - 1) << "/" << response_body.str().length() << "\r\n"
                << "Accept-Ranges: bytes\r\n"
                << "Content-Type: video/webm\r\n"
                << "Content-Length: " << read_str.length()
                << "\r\n\r\n";

                log.write("Response header\n" + response.str() + '\n');

                response << read_str;
            }
            else if(urls.find(s) != urls.end())
            {
                file.open(".." + urls[s]);
                if(file)
                {
                    std::stringstream body;
                    body << file.rdbuf();
                    response_body << createHtmlPage(body.str());
                } else {
                    log.write("!!! ERROR !!! Unable to open file");
                }

                // Формируем весь ответ вместе с заголовками
                response << "HTTP/1.1 200 OK\r\n"
                    << "Version: HTTP/1.1\r\n"
                    << "Content-Type: text/html; charset=utf-8\r\n"
                    << "Content-Length: " << response_body.str().length()
                    << "\r\n\r\n"
                    << response_body.str();
            }
            else if (s.find(".css") != std::string::npos)
            {
                file.open(PATH_TO_PUBLIC + s);
                if(file)
                {
                    response_body << file.rdbuf();
                } else {
                    log.write("!!! ERROR !!!  Unable to open style file");
                }

                // Формируем весь ответ вместе с заголовками
                response << "HTTP/1.1 200 OK\r\n"
                    << "Version: HTTP/1.1\r\n"
                    << "Content-Type: text/css\r\n"
                    << "Content-Length: " << response_body.str().length()
                    << "\r\n\r\n"
                    << response_body.str();
            }
            else if(s.find(".png") != std::string::npos)
            {
                file.open(PATH_TO_PUBLIC + s);
                if(file)
                {
                    response_body << file.rdbuf();
                } else {
                    log.write("!!! ERROR !!! Unable to open icon file");
                }

                response << "HTTP/1.1 200 OK\r\n"
                    << "Version: HTTP/1.1\r\n"
                    << "Content-Type: image/png\r\n"
                    << "Content-Length: " << response_body.str().length()
                    << "\r\n\r\n"
                    << response_body.str();
            }
            else if(s.find(".js") != std::string::npos)
            {
                file.open(PATH_TO_PUBLIC + s);
                if(file)
                {
                    response_body << file.rdbuf();
                } else {
                    log.write("!!! ERROR !!! Unable to open style file");
                }

                // Формируем весь ответ вместе с заголовками
                response << "HTTP/1.1 200 OK\r\n"
                    << "Version: HTTP/1.1\r\n"
                    << "Content-Type: text/js\r\n"
                    << "Content-Length: " << response_body.str().length()
                    << "\r\n\r\n"
                    << response_body.str();
            } else {
                // url not Found
                file.open(PATH_TO_404_HTML);
                if(file)
                {
                    response_body << file.rdbuf();
                } else {
                    log.write("!!! ERROR !!! Unable to open style file");
                }

                response << "HTTP/1.1 404 Not Found\r\n"
                    << "Version: HTTP/1.1\r\n"
                    << "Content-Type: text/html\r\n"
                    << "Content-Length: " << response_body.str().length()
                    << "\r\n\r\n"
                    << response_body.str();
            }
            file.close();
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