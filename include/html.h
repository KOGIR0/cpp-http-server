// class for html page creation
#ifndef HTML
#define HTML
#include <string>
#include <fstream>
#include <sstream>

namespace html
{
    class Page
    {
        std::string page;
        std::string createHead(const std::string& path2resources);
    public:
        Page(const std::string& path2header,
            const std::string& path2footer, 
            const std::string& path2body,
            const std::string& path2resources);
        std::string getPage();
    };
}

#endif