#include "html.h"

html::Page::Page(const std::string& path2header,
                const std::string& path2footer,
                const std::string& path2body,
                const std::string& path2resources)
{
    // page header
    std::fstream header(path2header);
    // page body
    std::fstream body(path2body);
    // page footer
    std::fstream footer(path2footer);
    // page start
    std::stringstream result;
    result << "<!DOCTYPE html><html>";
    
    result << createHead(path2resources);

    result << "<body>";  // open body tag
    
    // add header to the page
    if(header)
    {
        result << header.rdbuf();
    }

    // add left side to the page
    result << "<div id=\"page-content\"><div id=\'left-side\'>";

    // add body to the page
    if(body)
    {
        result << "</div><div id=\"main-content\">" << 
                body.rdbuf() << "</div><div id=\'right-side\'></div></div>";
    }
    // add footer to the page
    if(footer)
    {
        result << footer.rdbuf();
    }

    result << "</body></html>"; // close body and html tags
    this->page = result.str();
}

std::string html::Page::createHead(const std::string& path2resources)
{
    // *.ccs and *.js common for every page
    std::fstream resources(path2resources);
    std::stringstream html;

    html << "<head><meta charset=\"utf-8\"><title>Piki</title>";
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