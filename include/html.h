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
        std::string createHead(const std::string& path2resources, const std::string& topicName);
        std::string createBody(const std::string& body);
        // function gets topic name from h1 tag ( <h1>Title</h1> )
        std::string getTopicName(const std::string& body);
    public:
        Page(const std::string& path2header,
            const std::string& path2footer, 
            const std::string& path2body,
            const std::string& path2resources);
        std::string getPage();
    };
}

#endif