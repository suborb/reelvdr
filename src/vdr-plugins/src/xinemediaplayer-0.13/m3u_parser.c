#include "m3u_parser.h"
#include <iostream>
#include <unistd.h>
#include <vdr/tools.h>

// removes leading and trailing white-spaces
int remove_leading_trailing_whitespaces(char** str) 
{
    if (!str || !*str) return -1; // error

    char *frontp, *rearp;
    frontp = *str;
    rearp = *str + strlen(*str) -1;  // char before '\0'

    //leading white-spaces
    while ( isspace(*frontp) && frontp != rearp+1 ) ++frontp;
    if (frontp == rearp+1) // empty string
        return 0;
    if (frontp == rearp ) // single char
        return 1;

    if ( *str != frontp)
    { 
        char  *new_str = (char*) malloc(sizeof(char)* (strlen(frontp)+1) );
        strncpy(new_str, frontp, strlen(frontp));

        frontp = new_str;
        rearp = frontp + strlen(frontp) -1;
        free(*str);
        *str = new_str;
    }


    //trailing white spaces
    while ( isspace(*rearp) && rearp != frontp ) --rearp;

    rearp[1]='\0';
        
    return strlen(*str);

    //if (frontp == rearp) // all spaces
    //       return 0; // empty string

}

////////////////////////////////////////////////////////////////////////////////
int cM3U_parser::Read()
{
    if (filename.size()<=0) return -1;

    FILE *fp = fopen(filename.c_str(), "r");
    printf("m3u parser: fopen ('%s')\n", filename.c_str());

    if (fp == NULL) return -1;

    int isPlaylist = 0;
    char * line = NULL;
    char * substr = NULL;
    //char *tmp_line;
    char title[128];
    //char currLine[256];
    size_t len = 0;
    ssize_t read;
    int duration=0;
    enum LineType{ eComment, eInfo, eUri};
    int prevLineType = eUri;

    while ((read = getline(&line, &len, fp)) != -1)
    {
        //printf("m3uparser got line> '%s'\n", line);
       if (remove_leading_trailing_whitespaces(&line)<= 0 )   continue; 
        
        if ( line == NULL || strlen(line) == 0)                 continue;
        
   
        if ( strstr(line, "#") )
        {
            if (strstr(line, "#EXTM3U")) {isPlaylist=1;continue;}
            substr = strstr(line,"#EXTINF:");
            if ( substr && strlen(substr+8)>2 ) // one number and one comma  ','
            {
                int dummy= sscanf(substr+8, "%d,%127[^\n]", &duration, title)  ;
                title[127] = 0;
                if (dummy != 2) { duration = 0; title[0]=0; break; }

                prevLineType = eInfo;
            }
        }
        else /*if (isPlaylist)*/ // does not matter if it is just a m3u playlistand not a EXTM3U playlist
        {
            // check for validity of this line
            // not a comment so, it is has the full file path or is a url
            if (strstr(line, "http") == NULL &&  access(line,R_OK) != 0)
            {
                esyslog("m3u parser omitting line '%s'", line);
                printf("m3u parser omitting line '%s'\n", line);

                continue;
            }

            if ( prevLineType == eUri ) 
            {
                //no title for this item
                vTitle.push_back("");
            }
            else
            {
                title[127] = 0;
                vTitle.push_back(title);
            //    printf("(%s:%i) %s (%d sec) ((%u *** %s))\n", __FILE__, __LINE__, title, duration ,vTitle.size(), vTitle[0].c_str());
                title[0]=0;
            }

            prevLineType = eUri;
            vList.push_back(line);
        }

    }
    fclose(fp);
    //return isPlaylist;
    return vList.size() > 0; // got some file/urls
}

void cM3U_parser::GetPlayLists(std::vector<std::string>& list1, std::vector<std::string>& title1 )
{
 list1 = vList;
 title1 = vTitle;
  //std::cout<<"sizes  ("<<  vList.size()<<"/"<< vTitle.size() <<")"<<std::endl;
  //printf("printf: sizes(%u/%u) %s/%s\n", vList.size(), vTitle.size(), title1[0].c_str(), vTitle[0].c_str() );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int m3u_parser(std::string file,std::vector<std::string>& list, std::vector<std::string>& title )
{
    cM3U_parser m3u_parser(file);
    printf("\nbefore m3u_parser()...\n");
    if ( !m3u_parser.Read()) 
    {
        list.clear();
        title.clear();
        return 0;
    }
    printf("after m3u_parser()...\n");
    m3u_parser.GetPlayLists(list,title);
    printf("got the playlist items (%d)\n\n", list.size());

    return (list.size() == title.size()) && list.size()!=0;
}

