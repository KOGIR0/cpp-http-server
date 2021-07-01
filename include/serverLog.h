#ifndef SERVER_LOG
#define SERVER_LOG

#include <chrono>
#include <string>
#include <fstream>
#include <exception>

class FileOpenException : public std::exception
{
public:
    const char* what();
};

class serverLog
{
public:
    serverLog(){};
    serverLog(const std::string& filename);
    void open(const std::string& filename);
    void write(const std::string& s);
    ~serverLog();

private:
    std::ofstream server_log;
};

#endif