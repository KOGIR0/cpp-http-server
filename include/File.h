#ifndef IN_FILE
#define IN_FILE

#include <string>
#include <fstream>

namespace in
{
    class File
    {
    public:
        File(){}
        File(const std::string& s);
        bool ok();
        void open(const std::string& s);
        unsigned int size();
        ~File();

        // function skips offset in the begining
        // read n chars from start
        std::string read(const int& start, const int& n);

    private:
        std::ifstream file;
        unsigned int fileLength;
    };
}

#endif