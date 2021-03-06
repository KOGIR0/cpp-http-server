#include "Server.h"


const char* tcp::getaddrinfoException::what() const throw()
{
    return "getaddinfo failed";
}

const char* tcp::socketCreationException::what() const throw()
{
    return "error creating socket";
}

const char* tcp::bindSocketException::what() const throw()
{
    return "bind socket error";
}

const char* tcp::listenException::what() const throw()
{
    return "listen error";
}

tcp::Server::Server(std::string ip, std::string port)
{
    hints.ai_family = AF_INET; // AF_INET определяет, что будет
    // использоваться сеть для работы с сокетом
    hints.ai_socktype = SOCK_STREAM; // Задаем потоковый тип сокета
    hints.ai_protocol = IPPROTO_TCP; // Используем протокол TCP
    hints.ai_flags = AI_PASSIVE; // Сокет будет биндиться на адрес,
                                // чтобы принимать входящие соединения

    // Инициализируем структуру, хранящую адрес сокета - addr
    int result = getaddrinfo(ip.c_str(), port.c_str(), &hints, &addr);

    // Если инициализация структуры адреса завершилась с ошибкой,
    // выведем сообщением об этом и завершим выполнение программы
    if (result != 0) {
        throw getaddrinfoException();
    }

    // Создание сокета
    this->serverSocket = new tcp::Socket(::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol));
    // Если создание сокета завершилось с ошибкой, выводим сообщение,
    // освобождаем память, выделенную под структуру addr
    if (!this->serverSocket->ok()) {
        throw socketCreationException();
    }

    // Привязываем сокет к IP-адресу
    result = this->serverSocket->bind(addr);

    // Если привязать адрес к сокету не удалось, то выводим сообщение
    // об ошибке, освобождаем память, выделенную под структуру addr.
    // и закрываем открытый сокет.
    if (result == -1)
    {
        throw bindSocketException();
    }
}

tcp::Socket tcp::Server::accept()
{
    return tcp::Socket(this->serverSocket->accept());
}

void tcp::Server::listen()
{
    // Инициализируем слушающий сокет
    if(this->serverSocket->listen(SOMAXCONN) == -1)
    {
        throw tcp::listenException();
    }
}

tcp::Server::~Server()
{
    delete this->serverSocket;
    freeaddrinfo(addr);
}