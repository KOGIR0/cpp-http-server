#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <exception>
#include <memory>

#include "Server.h"
#include "Socket.h"
#include "File.h"
#include "Request.h"
#include "Response.h"

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
            Response res(request);
            response = res.getResponse();
            
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