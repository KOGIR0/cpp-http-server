#include "File.h"

in::File::File(const std::string& s)
{
    this->file.open(s);
    this->file.seekg(0, this->file.end);
    this->fileLength = this->file.tellg();
    this->file.seekg(0, this->file.beg);
}

bool in::File::ok()
{
    return this->file.is_open();
}

void in::File::open(const std::string& s)
{
    this->file.open(s);
    this->file.seekg(0, this->file.end);
    this->fileLength = this->file.tellg();
    this->file.seekg(0, this->file.beg);
}

unsigned int in::File::size()
{
    return this->fileLength;
}

in::File::~File()
{
    file.close();
}

std::string in::File::read(const int& start, const int& n)
{
    std::string result;
    char ch;

    this->file.seekg(start, this->file.beg);
    while( this->file.get(ch) && ch == '\n');
    result += ch;
    while (result.length() < n && this->file.get(ch)) {
        result += ch;
    }
    this->file.seekg(0, this->file.beg);

    return result;
}
