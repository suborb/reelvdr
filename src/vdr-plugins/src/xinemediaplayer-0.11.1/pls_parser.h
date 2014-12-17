#ifndef PLS_PARSER_H
#define PLS_PARSER_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

using namespace std;
////////////////////////////////////////////////////////////////////////////////
// class cTrackInfo
////////////////////////////////////////////////////////////////////////////////
class cTrackInfo // one entry in the pls file
{
    public:
        std::string URI; // path / URL
        int length; // -1 = indefinite
        std::string Title;

        cTrackInfo()
        {
            Clear();
        }

        void Clear()
        {
            length = 0;
            URI.clear();
            Title.clear();
        }
};

////////////////////////////////////////////////////////////////////////////////
// class cPLS
////////////////////////////////////////////////////////////////////////////////
class cPLS // .pls playlist
{
    private:
        std::vector<cTrackInfo> entries; 
        std::string fileName;
    public:
        void Display();
        void GetPlayLists(std::vector<string>& list, std::vector<std::string>& names);
        void SetFileName(std::string f) {fileName = f;}
        int Read();
        cPLS(){ }
        cPLS(std::string f){fileName = f;}
        // no write
        //Write(std::string filepath);
};


int pls_parser(std::string file, std::vector<string>& list, std::vector<std::string>& names) ;

#endif
