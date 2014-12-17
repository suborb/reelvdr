/****************************************************************************
 * DESCRIPTION:
 *            
 *
 * $Id: autotimer.h,v 1.9 2006/08/15 20:35:22 ralf Exp $
 *
 * Contact:    ranga@teddycats.de
 *
 * Copyright (C) 2005 by Ralf Dotzert
 ****************************************************************************/
 
#ifndef AUTOTIMER_H
#define AUTOTIMER_H
#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <vector>
#include <mysql/mysql.h>
#include <vdr/tools.h>
#include "libmysqlwrapped.h"


#define TRUESTR          "y"
#define FALSESTR         "n"
#define FIELDTITLE       "title"
#define FIELDSUBTITLE    "subtitle"
#define FIELDDESCRIPTION "description"


class cXxvAutoTimer : public cListObject
{
private:
       int    _id;
       int    _active;
       int    _done;
       char   _search[101];
       char   _searchFields[101];
       int    _searchTitle;
       int    _searchSubTitle;
       int    _searchDescription;

       int    _nrChannelsToUse;
       std::vector <int> _channels;
       std::string _channelNames;
       std::string _channelStr;


       int    _start;
       int    _stop;
       int    _minlength;
       int    _priority;
       int    _lifetime;
       char   _dir[101];
       int    _useChannel;
       int    _useStart;
       int    _useStop;

       // Parameters used in XXV > 0.20
       int    _vps;
       int    _useStandardPrevMinutes;
       int    _prevMinutes;
       int    _useStandardAfterMinutes;
       int    _afterMinutes;

       int    _useWeekDays;
       int    _weekMon;
       int    _weekTue;
       int    _weekWed;
       int    _weekThu;
       int    _weekFri;
       int    _weekSat;
       int    _weekSun;
       char   _weekDays[29];
       std::string _sql;

       const char *boolToString(int boolVal)     {if(boolVal == 1)  return(TRUESTR); else return(FALSESTR);}
       const char *fieldsToString();
       //left num string with lenth padding zero
       std::string prefixZero(int val)       {  static std::string ret;
                                                 std::stringstream s;
                                               s << val;
                                               ret = s.str();
                                               if( ret.length()<4)
                                                 ret.insert(0,4-ret.length(),'0');

                                               return(ret);
                                             }

public:
    cXxvAutoTimer();
    ~cXxvAutoTimer();
    void Reset();
    bool operator< (cXxvAutoTimer &param);
    bool operator> (cXxvAutoTimer &param);
    void operator= (cXxvAutoTimer &param);
    void SetId(const int& theValue)       {  _id = theValue;}
    int GetId() const                     {   return _id;}

    void SetActive(const char * theValue) {  if(theValue != NULL && strcmp(theValue, FALSESTR)==0)
                                                _active = 0;
                                             else
                                                _active = 1;
                                          }
    void SetActive(const int& theValue)   {  _active = theValue; }
    int GetActive() const                 {   return _active;}
    int *GetActiveRef()                   {  return(&_active);}

    void SetDone(const int& theValue)     {  _done = theValue;}
    void SetDone(const char *theValue)    {
                                            if(theValue != NULL && strcmp(theValue, FALSESTR)==0)
                                                _done = 0;
                                            else
                                                _done = 1;
                                          }
    int GetDone() const                   { return _done; }
    int *GetDoneRef()                     { return &_done; }
    void SetSearch(const char* theValue)  { strncpy(_search,theValue, sizeof(_search)); }
    char *GetSearch()                     { return _search; }

    int  GetSearchLen()                   {return(sizeof(_search));}
    void SetInfields(const char* theValue);

    int  *GetSearchTitleRef()             { return(&_searchTitle);}
    int  *GetSearchSubTitleRef()          { return(&_searchSubTitle);}
    int  *GetSearchDescriptionRef()       { return(&_searchDescription);}

    void SetChannels(const char* theValue);
    int   GetNrOfChannels()               { return(_channels.size());}
    int   GetChannelNumber(int index)     { return _channels[index];}
    int  *GetChannelNumberRef(int index)  { return &(_channels[index]);}

    void  AdjustNrOfChannels()            { return(_channels.resize(_nrChannelsToUse, 1));}
    int GetNrChannlesToUse()              { return _nrChannelsToUse;}
    int *GetNrChannlesToUseRef()           { return &_nrChannelsToUse;}

    const char *GetChannelNames();
    const char  *GetChannelStr();

    int   GetUseChannel()                    { return _useChannel;}
    void  SetUseChannel(const int& theValue) { _useChannel = theValue;}
    int  *GetUseChannelRef()                 { return &_useChannel;}

    void SetStart(const int& theValue);
    const char* GetStart()                         { return prefixZero(_start).c_str(); }
    int *GetStartRef()                     { return &_start; }

    int  GetUseStart()                     { return _useStart; }
    int *GetUseStartRef()                  { return &_useStart; }

    void SetStop(const int& theValue);
    const char*  GetStop()                 { return prefixZero(_stop).c_str();  }
    int *GetStopRef()                      { return &_stop;  }
    int  GetUseStop()                      { return _useStop; }
    int *GetUseStopRef()                   { return &_useStop; }

    void SetMinlength(const int& theValue){ _minlength = theValue;}
    int  GetMinlength() const             { return _minlength;}
    int *GetMinlengthRef()                { return &_minlength;}


    void SetPriority(const int& theValue) { _priority = theValue; }
    int  GetPriority() const              { return _priority;  }
    int *GetPriorityRef()                 { return &_priority;  }

    void SetLifetime(const int& theValue) { _lifetime = theValue;}
    int GetLifetime() const               { return _lifetime;  }
    int *GetLifetimeRef()                 { return &_lifetime;  }


    void SetDir(const char* theValue)      { strncpy(_dir, theValue, sizeof(_dir)); }
    char *GetDir()                         { return _dir;  }
    int  GetDirLen()                       { return(sizeof(_dir));}

    void SetVPS(const char *theValue)    {
      if(theValue != NULL && strcmp(theValue, FALSESTR)==0)
        _vps = 0;
      else
        _vps = 1;
    }
    int GetVPS() const                 {   return _vps;}
    int *GetVPSRef()                   {  return(&_vps);}

    void SetPrevMinutes(const int& theValue){ _useStandardPrevMinutes=false; _prevMinutes = theValue;}
    int  GetPrevMinutes() const             { return _prevMinutes;}
    int *GetPrevMinutesRef()                { return &_prevMinutes;}
    bool GetUseStandardPrevMinutes()         {return _useStandardPrevMinutes;}
    int  *GetUseStandardPrevMinutesRef()     {return &_useStandardPrevMinutes;}
    
    void SetAfterMinutes(const int& theValue){ _useStandardAfterMinutes=false; _afterMinutes = theValue;}
    int  GetAfterMinutes() const             { return _afterMinutes;}
    int *GetAfterMinutesRef()                { return &_afterMinutes;}
    bool GetUseStandardAfterMinutes()         {return _useStandardAfterMinutes;}
    int  *GetUseStandardAfterMinutesRef()     {return &_useStandardAfterMinutes;}


    int  GetUseWeekDays() const             { return _useWeekDays;}
    int *GetUseWeekDaysRef()                { return &_useWeekDays;}
    void SetWeekDays(const char* theValue);
    const char  *GetWeekDays();
    int *GetWeekMonRef()                { return &_weekMon;}
    int *GetWeekTueRef()                { return &_weekTue;}
    int *GetWeekWedRef()                { return &_weekWed;}
    int *GetWeekThuRef()                { return &_weekThu;}
    int *GetWeekFriRef()                { return &_weekFri;}
    int *GetWeekSatRef()                { return &_weekSat;}
    int *GetWeekSunRef()                { return &_weekSun;}
    
    
    const char *ToString();
    const char *CreateUpdateSqlString(Database *db);
    const char *CreateInsertSqlString(Database *db);
    const char *CreateDeleteSqlString(Database *db);

};

class cXxvAutoTimers : public cList<cXxvAutoTimer>
{
  
  public:
    enum Order { ASC, DESC};
    void Sort( enum Order order);
};

#endif
