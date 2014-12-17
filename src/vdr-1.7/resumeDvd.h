#ifdef REELVDR
#ifndef _RESUMEDVD_H
#define _RESUMEDVD_H

#include <map>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <time.h>

class cResumeInfo
{
	public:
		
		time_t lastSeen;
		std::string titleChapter;

		static const time_t ExpireTime;// = 604800; //7*24*3600; // 1 week in seconds

		cResumeInfo();
		cResumeInfo(std::string line);
		void Init();
		std::string ToString();
		bool FromString(std::string);
		bool Expired();
		
};

class cResumeDvd
{
private:
    std::string fileName_;
    std::string currDvdId_;
    std::map<std::string, cResumeInfo> DvdList_;

public:
    cResumeDvd (std::string);
    ~cResumeDvd ();
    // loose all unsave information
    bool Read ();

    //(over)write list to a file
    bool Save ();

    //makes no entry in the list, just sets currDvdId_
    bool SetCurrentDvd (std::string id);

    // sets title chapter of current DVD
    bool SetTitleChapter (std::string tc);

    // get title and chapter of current DVD
    std::string TitleChapter ();
};

extern cResumeDvd ResumeDvd;

std::string DvdId ();

#endif
#endif /*REELVDR*/

