#include "serverLog.h"

const char* FileOpenException::what()
{
    return "Error opening file";
}

serverLog::serverLog(const std::string& filename)
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

void serverLog::open(const std::string& filename)
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

void serverLog::write(const std::string& s)
{
    server_log << s << std::endl;
}

serverLog::~serverLog()
{
    server_log.close();
}