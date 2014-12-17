#ifndef M3U_PARSER_H
#define M3U_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

// removes leading and trailing white-spaces
int remove_leading_trailing_whitespaces(char** str) ;

////////////////////////////////////////////////////////////////////////////////
// class cM3U_parser
////////////////////////////////////////////////////////////////////////////////
class cM3U_parser
{
    private:
        std::string filename;
    public:
        std::vector<std::string> vTitle, vList;
        int Read();
        cM3U_parser(std::string f) {filename = f;}
        void GetPlayLists(std::vector<std::string>& list1, std::vector<std::string>& title1 ) ;
};

////////////////////////////////////////////////////////////////////////////////

int m3u_parser(std::string file,std::vector<std::string>& list, std::vector<std::string>& title );

#endif
