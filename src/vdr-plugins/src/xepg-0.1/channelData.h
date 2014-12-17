

#ifndef __CHANNELDATA_H_
#define __CHANNELDATA_H_

#include <tinyxml/tinyxml.h>
#include <tinyxml/tinystr.h>
#include <iostream>
#include <time.h>
#include <string>
#include <vector>
#include <libsi/si.h>
#include <vdr/config.h>

#if VDRVERSNUM >= 10716
#define DOCUMENT    TINYXML_DOCUMENT
#define ELEMENT     TINYXML_ELEMENT
#define COMMENT     TINYXML_COMMENT
#define UNKNOWN     TINYXML_UNKNOWN
#define TEXT        TINYXML_TEXT
#define DECLARATION TINYXML_DECLARATION
#endif

using namespace std;

struct programmeData
{
  /* Title, start and end times and a description of the programme. */
  string title;
  string start_time;
  string end_time;
  string description;
  string short_text;
  string infourl;
};                              // end struct programmeData

class cChannelData
{
  friend int getChannelData (TiXmlNode *, cChannelData *); // extracts data from tinyXML data structure to cChannelData

private:

  char *EpgDataFileName;
  bool isParsed;

  string id;
  string source;
  string date;
    std::vector < programmeData > ProgrammeList;

public:

    cChannelData (const char *epgFileName);
    cChannelData ()
  {
    isParsed = false;
    EpgDataFileName = NULL;
  }
  bool setEpgFileName (const char *epgFileName);
  int loadEpgData (const char *channelName);
  bool parseEpgDataFile ();

};                              // end class cChannelData


/******Misc functions ****************/

#define POLY 0xA001             // CRC16
unsigned int crc16 (unsigned int crc, unsigned char const *p, int len);


time_t str2time_t (const char *parmTime, const char *parmDate = NULL, int jumpDays = 0);

/*
 *   parmTime is "1130" not "11:30"
 *   parmDate is "dd/mm/yyyy" for eg. "17/11/2007"
 *
 */

#endif
