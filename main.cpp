#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fstream>
#include <chrono>

int main()
{
    std::string port = "8080";    // номер порта нашего HTTP сервера
    struct addrinfo* addr = NULL; // структура, которая будет хранить адрес слушающего сокета
    struct addrinfo hints;
    int listen_socket_fd; // дескриптор сокета сервера
    int already_read = 0;

    // создаем лог фаил
    std::ofstream server_log("server.log");

    if(server_log)
    {
        auto date = std::chrono::system_clock::now();
        std::time_t date_time = std::chrono::system_clock::to_time_t(date);
        std::string todays_date(std::ctime(&date_time));
        server_log << "<<< SERVER WORK START >>>.\n Date: " << todays_date << std::endl;
    } else {
        std::cerr << "Unable to open log file" << std::endl;
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
        server_log << "!!! ERROR !!! " << "getaddrinfo failed." << "\n";
        return 1;
    }

    // Создание сокета
    listen_socket_fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    // Если создание сокета завершилось с ошибкой, выводим сообщение,
    // освобождаем память, выделенную под структуру addr
    if (listen_socket_fd == -1) {
        server_log << "!!! ERROR !!! " << "Error creating socket" << std::endl;
        freeaddrinfo(addr);
        return 1;
    }

    // Привязываем сокет к IP-адресу
    result = bind(listen_socket_fd, addr->ai_addr, (int)addr->ai_addrlen);

    // Если привязать адрес к сокету не удалось, то выводим сообщение
    // об ошибке, освобождаем память, выделенную под структуру addr.
    // и закрываем открытый сокет.
    if (result == -1) {
        server_log << "!!! ERROR !!! " << "bind error" << std::endl;
        freeaddrinfo(addr);
        close(listen_socket_fd);
        return 1;
    }

    // Инициализируем слушающий сокет
    if (listen(listen_socket_fd, SOMAXCONN) == -1) {
        server_log << "!!! ERROR !!! " << "listen failed with error" << std::endl;
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
            server_log << "!!! ERROR !!! " << "accept failed" << std::endl;
            return 1;
        }

        const int max_client_buffer_size = 1024;
        char buf[max_client_buffer_size];

        result = recv(client_socket_fd, buf, max_client_buffer_size, 0);


        if (result == -1) {
            // ошибка получения данных
            server_log << "!!! ERROR !!! " << "recv failed" << std::endl;
            close(client_socket_fd);
        } else if (result == 0) {
            // соединение закрыто клиентом
            server_log << "!!! ERROR !!! " << "connection closed..." << std::endl;
        } else if (result > 0) {
            server_log << "//// REQUEST START \\\\\\\\" << std::endl;
            // Мы знаем фактический размер полученных данных, поэтому ставим метку конца строки
            // В буфере запроса.
            buf[result] = '\0';
            server_log << "Data recieved" << std::endl;
            server_log << buf << std::endl;

            // Для удобства работы запишем полученные данные
            // в stringstrem request
            request << buf;
            // Данные приходят в виде HTTP запроса
            // Первая строка имеет вид: вид запроса(GET, POST ...) url
            std::string s;
            request >> s >> s;
            server_log << "Request url " << s << std::endl;
            if(s == "/end")
            {
                break;
            }
            // Если запрашивается видео
            if(s.find(".mp4") != std::string::npos)
            {
                std::ifstream video("../public" + s);
                server_log << "Sending video: " << s << std::endl;
                if(video)
                {
                    response_body << video.rdbuf();
                    video.close();
                } else {
                    server_log << "!!! ERROR !!! " << "Unable to open video file" << std::endl;
                }

                // Формируем весь ответ вместе с заголовками
                response << "HTTP/1.1 200 OK\r\n"
                << "Version: HTTP/1.1\r\n"
                << "Content-Type: video/mp4"
                << "Content-Length: " << response_body.str().length()
                << "\r\n\r\n"
                << response_body.str();
                /*char buf[1024];
                response_body.seekg(already_read);
                int r = response_body.readsome(buf, 1024);
                std::cout << "Read from response body: " << r << " Alredy read: " << already_read << std::endl;
                already_read += 1024;
                response << "HTTP/1.1 206 Partial Content\r\n"
                << "Content-Range: bytes " << (already_read - 1024) << "-" << (already_read - 1) << "/" << response_body.str().length() << "\r\n"
                << "Accept-Ranges: bytes\r\n"
                << "Content-Type: video/mp4\r\n"
                << "Content-Length: " << sizeof(buf)
                << "\r\n\r\n"
                << response_body.str();*/

                // Отправляем ответ клиенту с помощью функции send
                result = send(client_socket_fd, response.str().c_str(), response.str().length(), 0);
            } else if(s.find(".html") != std::string::npos)
            {
                // Данные успешно получены
                // формируем тело ответа из html файла
                std::ifstream file("../public/html" + s);
                if(file)
                {
                    server_log << "Sending html file: index.html" << std::endl;
                    response_body << file.rdbuf();
                    file.close();
                } else {
                    server_log << "!!! ERROR !!! " << "Unable to open file" << std::endl;
                }

                // Формируем весь ответ вместе с заголовками
                response << "HTTP/1.1 200 OK\r\n"
                    << "Version: HTTP/1.1\r\n"
                    << "Content-Type: text/html; charset=utf-8\r\n"
                    << "Content-Length: " << response_body.str().length()
                    << "\r\n\r\n"
                    << response_body.str();

                // Отправляем ответ клиенту с помощью функции send
                result = send(client_socket_fd, response.str().c_str(), response.str().length(), 0);
            }
            else if (s.find(".css") != std::string::npos)
            {
                // Данные успешно получены
                // формируем тело ответа из html файла
                std::ifstream file("../public" + s);
                if(file)
                {
                    server_log << "Sending html file: index.css" << std::endl;
                    response_body << file.rdbuf();
                    file.close();
                } else {
                    server_log << "!!! ERROR !!! " << "Unable to open style file" << std::endl;
                }

                // Формируем весь ответ вместе с заголовками
                response << "HTTP/1.1 200 OK\r\n"
                    << "Version: HTTP/1.1\r\n"
                    << "Content-Type: text/css\r\n"
                    << "Content-Length: " << response_body.str().length()
                    << "\r\n\r\n"
                    << response_body.str();

                // Отправляем ответ клиенту с помощью функции send
                result = send(client_socket_fd, response.str().c_str(), response.str().length(), 0);
            }

            server_log << "\\\\\\\\ REQUEST END ////" << std::endl;

            if (result == -1) {
                // произошла ошибка при отправле данных
                server_log << "!!! ERROR !!! " << "send failed " << std::endl;
            }
            // Закрываем соединение к клиентом
            close(client_socket_fd);
        }
    }

    // Убираем за собой
    server_log << "<<< SERVER WORK END >>>" << std::endl;
    close(listen_socket_fd);
    freeaddrinfo(addr);
    server_log.close();
    return 0;
}