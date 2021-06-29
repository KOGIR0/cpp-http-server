#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fstream>
#include <chrono>
#include <map>
#include <math.h>
#include <exception>

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

class FileOpenException: public std::exception
{
public:
    const char* what()
    {
        return "Error opening file";
    }
};

class serverLog
{
public:
    serverLog(){};
    serverLog(const std::string& filename)
    {
        server_log.open(filename);
        if(server_log)
        {
            auto date = std::chrono::system_clock::now();
            std::time_t date_time = std::chrono::system_clock::to_time_t(date);
            std::string todays_date(std::ctime(&date_time));
            server_log << "<<< SERVER WORK START >>>.\n Date: " << todays_date << std::endl;
        } else {
            throw new FileOpenException();
        }
    }

    void open(const std::string& filename)
    {
        server_log.open(filename);
        if(server_log)
        {
            auto date = std::chrono::system_clock::now();
            std::time_t date_time = std::chrono::system_clock::to_time_t(date);
            std::string todays_date(std::ctime(&date_time));
            server_log << "<<< SERVER WORK START >>>.\n Date: " << todays_date << std::endl;
        } else {
            throw new FileOpenException();
        }  
    }

    void write(const std::string& s)
    {
        server_log << s << std::endl;
    }

    ~serverLog()
    {
        server_log.close();
    }

private:
    std::ofstream server_log;
};

int main()
{
    std::string port = "8000";    // номер порта нашего HTTP сервера
    struct addrinfo* addr = NULL; // структура, которая будет хранить адрес слушающего сокета
    struct addrinfo hints;
    int listen_socket_fd; // дескриптор сокета сервера
    int already_read = 0;
    serverLog log;

    // создаем лог фаил
    try
    {
        log.open("server.log");
    }
    catch(FileOpenException& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    hints.ai_family = AF_INET; // AF_INET определяет, что будет
    // использоваться сеть для работы с сокетом
    hints.ai_socktype = SOCK_STREAM; // Задаем потоковый тип сокета
    hints.ai_protocol = IPPROTO_TCP; // Используем протокол TCP
    hints.ai_flags = AI_PASSIVE; // Сокет будет биндиться на адрес,
                                // чтобы принимать входящие соединения

    // Инициализируем структуру, хранящую адрес сокета - addr
    int result = getaddrinfo("127.0.0.1", port.c_str(), &hints, &addr);

    // Если инициализация структуры адреса завершилась с ошибкой,
    // выведем сообщением об этом и завершим выполнение программы
    if (result != 0) {
        log.write("!!! ERROR !!! getaddrinfo failed.");
        return 1;
    }

    // Создание сокета
    listen_socket_fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    // Если создание сокета завершилось с ошибкой, выводим сообщение,
    // освобождаем память, выделенную под структуру addr
    if (listen_socket_fd == -1) {
        log.write("!!! ERROR !!! Error creating socket");
        freeaddrinfo(addr);
        return 1;
    }

    // Привязываем сокет к IP-адресу
    result = bind(listen_socket_fd, addr->ai_addr, (int)addr->ai_addrlen);

    // Если привязать адрес к сокету не удалось, то выводим сообщение
    // об ошибке, освобождаем память, выделенную под структуру addr.
    // и закрываем открытый сокет.
    if (result == -1) {
        log.write("!!! ERROR !!! bind error");
        freeaddrinfo(addr);
        close(listen_socket_fd);
        return 1;
    }

    // Инициализируем слушающий сокет
    if (listen(listen_socket_fd, SOMAXCONN) == -1) {
        log.write("!!! ERROR !!! listen failed with error");
        close(listen_socket_fd);
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
        int client_socket_fd = accept(listen_socket_fd, NULL, NULL);
        
        if (client_socket_fd == -1) {
            log.write("!!! ERROR !!! accept failed");
            return 1;
        }

        const int max_client_buffer_size = 1024;
        char buf[max_client_buffer_size];

        result = recv(client_socket_fd, buf, max_client_buffer_size, 0);

        if (result == -1) {
            // ошибка получения данных
            log.write("!!! ERROR !!! recv failed");
            close(client_socket_fd);
        } else if (result == 0) {
            // соединение закрыто клиентом
            log.write("!!! ERROR !!! connection closed...");
        } else if (result > 0) {
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
            std::cout << parsedRequest["URL"] << std::endl;
            s = parsedRequest["URL"];
            log.write("Request url " + s);
            if(s == "/end")
            {
                break;
            }
            // Если запрашивается видео mp4
            if(s.find(".mp4") != std::string::npos)
            {
                std::ifstream video("../public" + s);
                log.write("Sending video: " + s);
                if(video)
                {
                    response_body << video.rdbuf();
                    video.close();
                } else {
                    log.write("!!! ERROR !!! Unable to open video file");
                }

                // Формируем весь ответ вместе с заголовками
                // отправка файла по частям
                unsigned long read_chunk = std::pow(2, 20) * 10; // 10Mb данных
                std::string read_str;
                if(std::stoi(parsedRequest["Range"]) + read_chunk <= response_body.str().length())
                {
                    read_str = response_body.str().substr(std::stoi(parsedRequest["Range"]), read_chunk);
                } else {
                    read_str = response_body.str().substr(std::stoi(parsedRequest["Range"]), response_body.str().length() - std::stoi(parsedRequest["Range"]));
                }

                log.write("Bytes read: " + std::to_string(read_str.length()) + " Buf size: " + std::to_string(read_str.length()));
                response << "HTTP/1.1 206 Partial Content\r\n"
                << "Content-Range: bytes " << parsedRequest["Range"] << "-" << (std::stoi(parsedRequest["Range"]) + read_str.length() - 1) << "/" << response_body.str().length() << "\r\n"
                << "Accept-Ranges: bytes\r\n"
                << "Content-Type: video/mp4\r\n"
                << "Content-Length: " << read_str.length()
                << "\r\n\r\n";

                log.write("Response header\n" + response.str() + '\n');

                response << read_str;
            }
            else if(s.find(".webm") != std::string::npos)
            {
                std::ifstream video("../public" + s);
                log.write("Sending video: " + s);
                if(video)
                {
                    response_body << video.rdbuf();
                    video.close();
                } else {
                    log.write("!!! ERROR !!! Unable to open video file");
                }

                // Формируем весь ответ вместе с заголовками
                // отправка файла по частям
                unsigned long read_chunk = std::pow(2, 20);
                std::string read_str;
                if(std::stoi(parsedRequest["Range"]) + read_chunk <= response_body.str().length())
                {
                    read_str = response_body.str().substr(std::stoi(parsedRequest["Range"]), read_chunk);
                } else {
                    read_str = response_body.str().substr(std::stoi(parsedRequest["Range"]), response_body.str().length() - std::stoi(parsedRequest["Range"]));
                }

                log.write("Bytes read: " + std::to_string(read_str.length()) + " In buf: " + std::to_string(read_str.length()));
                response << "HTTP/1.1 206 Partial Content\r\n"
                << "Content-Range: bytes " << parsedRequest["Range"] << "-" << (std::stoi(parsedRequest["Range"]) + read_str.length() - 1) << "/" << response_body.str().length() << "\r\n"
                << "Accept-Ranges: bytes\r\n"
                << "Content-Type: video/webm\r\n"
                << "Content-Length: " << read_str.length()
                << "\r\n\r\n";

                log.write("Response header\n" + response.str() + '\n');

                response << read_str;
            }
            else if(s.find(".html") != std::string::npos)
            {
                // Данные успешно получены
                // формируем тело ответа из html файла
                std::ifstream file("../public/html" + s);
                if(file)
                {
                    log.write("Sending html file: " + s);
                    response_body << file.rdbuf();
                    file.close();
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
                // Данные успешно получены
                // формируем тело ответа из html файла
                std::ifstream file("../public" + s);
                if(file)
                {
                    log.write("Sending html file: " + s);
                    response_body << file.rdbuf();
                    file.close();
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
            else if(s.find(".js") != std::string::npos)
            {
                // Данные успешно получены
                // формируем тело ответа из html файла
                std::ifstream file("../public" + s);
                if(file)
                {
                    log.write("Sending html file: " + s);
                    response_body << file.rdbuf();
                    file.close();
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
            }
            // Отправляем ответ клиенту с помощью функции send
            result = send(client_socket_fd, response.str().c_str(), response.str().length(), 0);

            // произошла ошибка при отправле данных
            if (result == -1) {
                log.write("!!! ERROR !!! send failed");
            }
            log.write("\\\\\\\\ REQUEST END ////");
            // Закрываем соединение с клиентом
            close(client_socket_fd);
        }
    }

    // Убираем за собой
    log.write("<<< SERVER WORK END >>>");
    close(listen_socket_fd);
    freeaddrinfo(addr);
    return 0;
}