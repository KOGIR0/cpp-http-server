#include "html.h"

html::Page::Page(const std::string& path2header,
                const std::string& path2footer,
                const std::string& path2body,
                const std::string& path2resources)
{
    // page header
    std::ifstream header(path2header);
    // page body
    std::ifstream body(path2body);
    // page footer
    std::ifstream footer(path2footer);
    // page start
    std::stringstream result;
    result << "<!DOCTYPE html><html>";
    
    // get topic-name from body
    std::stringstream ss_body;
    if(body)
    {
        ss_body << body.rdbuf();
    }
    std::string topicName = getTopicName(ss_body.str());
    // end get topic from body

    result << createHead(path2resources, topicName);

    result << "<body>";  // open body tag
    
    // add header to the page
    if(header)
    {
        result << header.rdbuf();
    }

    // add body to the page
    result << createBody(ss_body.str());
    // add footer to the page
    if(footer)
    {
        result << footer.rdbuf();
    }

    result << "</body></html>"; // close body and html tags
    this->page = result.str();
}

std::string html::Page::getTopicName(const std::string& body)
{
    std::string bodyStr = body;
    std::string h1OpenTag = "<h1>";
    int h1Start = bodyStr.find(h1OpenTag);
    int h1End = bodyStr.find("</h1>");
    int topicNameStart = (h1Start == std::string::npos) ? 0 : (h1Start + h1OpenTag.size());
    int topicNameSize = (h1End == std::string::npos || !topicNameStart) ? 0 : (h1End - topicNameStart);
    return bodyStr.substr(topicNameStart, topicNameSize);
}

std::string html::Page::createBody(const std::string& body)
{
    std::stringstream bodySS;
    bodySS << "<div id=\"page-content\">"
            << "<div id=\'left-side\'>" << "</div>"
            << "<div id=\"main-content\">" << body << "</div>"
            << "<div id=\'right-side\'>" << "</div>"
            << "</div>";
    return bodySS.str();
}

std::string html::Page::createHead(const std::string& path2resources, const std::string& topicName)
{
    // *.ccs and *.js common for every page
    std::ifstream resources(path2resources);
    std::stringstream html;

    html << "<head><meta charset=\"utf-8\"><title>"
            << (topicName.size() ? (topicName) : topicName) + "</title>";
    if(resources)
    {
        html << resources.rdbuf();
    }
    html << "</head>";

    return html.str();
}

std::string html::Page::getPage()
{
    return this->page;
}