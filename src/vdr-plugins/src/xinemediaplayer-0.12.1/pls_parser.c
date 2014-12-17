#include "pls_parser.h"
#include <cstring>

////////////////////////////////////////////////////////////////////////////////
int cPLS::Read()
{
    fstream fp(fileName.c_str(), ios::in);
    
    // file could not be opened for reading
    if ( !fp.is_open() ) return -1; 

    char buffer[512];
    bool isPlaylist=false;
    std::string line;
    cTrackInfo track;
    size_t idx;
    //int num = 0;
    while(! fp.eof() )
    {
        fp.getline(buffer, 511, '\n');
        line = buffer;
        //    std::cout<<++num<<"\t\t"<<line<<endl;

        if (strcasestr(buffer, "[playlist]")) { isPlaylist = true ; continue; }
        else if (strcasestr(buffer, "numberofentries")) continue;
       else if (strcasestr(buffer, "Version")) continue;

        // TitleX
       else if (strcasestr(buffer, "Title")) continue;
        /* {
           idx = line.find("=");
           if (idx == std::string::npos) continue;
           track.Title = line.substr(idx+1);
           }*/
        // LengthX
       else if (strcasestr(buffer, "Length")) continue;
        /*{
          idx = line.find("=");
          if (idx == std::string::npos) continue;
          track.length = atoi( line.substr(idx+1).c_str() );
          } */

        // FileX
      else  if (strcasestr(buffer, "File")) 
        {
            idx = line.find("="); 
            if (idx == std::string::npos) continue;
            track.URI = line.substr(idx+1);
        }
        else
            if ( line.find("=") != std::string::npos)
                track.URI = line; // add it as it is

        if ( isPlaylist && track.URI.size() > 0 )
        {
            entries.push_back(track);
            //printf("got track\n");
            track.Clear();
        }
    }
    fp.close();


    return isPlaylist; // 
}

////////////////////////////////////////////////////////////////////////////////
void cPLS::Display()
{
    std::vector <cTrackInfo>::iterator it = entries.begin();
    int num = 0;
    while(it != entries.end() )
    {
        std::cout<<++num<<"\t:"<<it->Title<<":\t"<<it->URI<<std::endl;
        ++it;
    }
}

////////////////////////////////////////////////////////////////////////////////
void cPLS::GetPlayLists(std::vector<string>& list, std::vector<std::string>& names)
{
    if (entries.size()<=0) return;
    list.clear();
    names.clear();
    std::vector <cTrackInfo>::iterator it = entries.begin();
    while(it != entries.end() )
    {
        list.push_back(it->URI);
        names.push_back(it->Title);
        ++it;
    }
    //Display();
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


int pls_parser(std::string file, std::vector<string>& list, std::vector<std::string>& names)
{
    cPLS playlist( file); // file name
    printf("before Reading pls files..");
    if ( playlist.Read() <= 0 ) // no playlists were found / file could not be opened
    {
        list.clear();
        names.clear();
        return 0;
    }

    printf("after Reading pls files\n");

    playlist.GetPlayLists(list, names); // get the playlist and their names 
    return list.size();
}
