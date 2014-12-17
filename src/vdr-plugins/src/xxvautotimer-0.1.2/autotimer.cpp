/****************************************************************************
 * DESCRIPTION:
 *            
 *
 * $Id: autotimer.cpp,v 1.11 2006/08/15 20:35:22 ralf Exp $
 *
 * Contact:    ranga@vdrtools.de
 *
 * Copyright (C) 2005 by Ralf Dotzert
 ****************************************************************************/



#include <vdr/channels.h>
#include "setup.h"
#include "autotimer.h"
#include "i18n.h"


#ifdef HAVE_ICONPATCH
  #include <vdr/iconpatch.h>
#else
  #define ICON_UHR '*'
#endif

#define FIELD_TITLE       "title"
#define FIELD_SUBTITLE    "subtitle"
#define FIELD_DESCRIPTION "description"

#define WEEK_MON   "Mon"
#define WEEK_TUE   "Tue"
#define WEEK_WED   "Wed"
#define WEEK_THU   "Thu"
#define WEEK_FRI   "Fri"
#define WEEK_SAT   "Sat"
#define WEEK_SUN   "Sun"

//#define STRADD(str, args...)   snprintf(str +strlen(str), sizeof(str)-strlen(str), ##args )



cXxvAutoTimer::cXxvAutoTimer()
{
   Reset();
}

void cXxvAutoTimer::Reset( )
{
   _active    = true;
   _done      = autotimerSetup.defaultDone;
   _search[0] = '\0';
   _searchFields[0] = '\0';
   _searchTitle = true;
   _searchSubTitle = true;
   _searchDescription = false;

   _useChannel = false;
   _channels.clear();
   _channelStr.clear();
   _channelNames.clear();
   _nrChannelsToUse = 1;
   _useStart     = false;
   _start        = 0;
   _stop         = 0;
   _useStop      = false;
   _minlength    = 0;
   _priority  = autotimerSetup.defaultLifetime;
   _lifetime  = autotimerSetup.defaultPriority;
   _dir[0]    = '\0';

   _vps          = false;
   _useStandardPrevMinutes = true;
   _prevMinutes  = autotimerSetup.defaultPrevMinutes;
   _useStandardAfterMinutes = true;
   _afterMinutes = autotimerSetup.defaultAfterMinutes;

   _useWeekDays  = false;
   _weekMon      = false;
   _weekTue      = false;
   _weekWed      = false;
   _weekThu      = false;
   _weekFri      = false;
   _weekSat      = false;
   _weekSun      = false;
   _weekDays[0]  = '\0';;
   
}


cXxvAutoTimer::~cXxvAutoTimer()
{

}

void cXxvAutoTimer::cXxvAutoTimer::operator =( cXxvAutoTimer &param )
{
  if( &param != this)
  {
       _id = param._id;
       _active = param._active;
       _done   = param._done;
       strcpy(_search, param._search);
       strcpy(_searchFields, param._searchFields);
       _searchTitle       = param._searchTitle;
       _searchSubTitle    = param._searchSubTitle;
       _searchDescription = param._searchDescription;
       _channelNames      = param._channelNames;
       _channelStr        = param._channelStr;
       _nrChannelsToUse   = param._nrChannelsToUse;
       _channels  = param._channels;
       _start     = param._start;
       _stop      = param._stop;
       _minlength = param._minlength;
       _priority  = param._priority;
       _lifetime  = param._lifetime;
       strcpy(_dir, param._dir);
       _useChannel = param._useChannel;
       _useStart   = param._useStart;
       _useStop    = param._useStop;

       _vps             = param._vps;
       _useStandardPrevMinutes  = param._useStandardPrevMinutes;
       _prevMinutes     = param._prevMinutes;
       _useStandardAfterMinutes = param._useStandardAfterMinutes;
       _afterMinutes    = param._afterMinutes;

       _useWeekDays  = param._useWeekDays;
       _weekMon      = param._weekMon;
       _weekTue      = param._weekTue;
       _weekWed      = param._weekWed;
       _weekThu      = param._weekThu;
       _weekFri      = param._weekFri;
       _weekSat      = param._weekSat;
       _weekSun      = param._weekSun;
       strcpy(_weekDays, param._weekDays);
  }
}

/**
 * Overload operator < to compare Plugins
 * @param param the Plugin to compare
 * @return true if search-string of left autotimer is less the right autotimer
 */
                         bool cXxvAutoTimer::operator <( cXxvAutoTimer &param )
{
  if(strcasecmp(_search, param._search)<0)
    return(true);
  else
    return(false);
}

/**
 * Overload operator > to compare Plugins

 * @param param the Plugin to compare
 * @return true if search-string of left autotimer is greater the right autotimer
 */
bool cXxvAutoTimer::operator >( cXxvAutoTimer &param )
{
  if(strcasecmp(_search, param._search)>0)
    return(true);
  else
    return(false);
}


void cXxvAutoTimer::SetInfields( const char * theValue )
{
  strstr(theValue, FIELD_TITLE)       != NULL ? _searchTitle = true : _searchTitle = false;
  strstr(theValue, FIELD_SUBTITLE)    != NULL ? _searchSubTitle = true : _searchSubTitle = false;
  strstr(theValue, FIELD_DESCRIPTION) != NULL ? _searchDescription = true : _searchDescription = false;
}


void cXxvAutoTimer::SetChannels(const char* theValue)
{
  _useChannel = false;
  char *channel = NULL;
  
  if( strlen(theValue)>0)
  {
    channel = strtok((char*)theValue, ",");
    do
    {
      tChannelID cid = tChannelID::FromString(channel);
      if( cid.Valid())
      {
        cChannel *chan = Channels.GetByChannelID(cid);
        if( chan != NULL)
        {
            _channels.push_back(chan->Number());
        }
      }
    }while( (channel=strtok(NULL, ",")));
    _useChannel = true;
    _nrChannelsToUse = _channels.size();
  }
}


void cXxvAutoTimer::SetStart(const int& theValue)
{
  _start = theValue;
   if( theValue != 0)
     _useStart = true;
   else
    _useStart = false;
}


void cXxvAutoTimer::SetStop(const int& theValue)
{
  _stop = theValue;

   if( theValue != 0)
     _useStop = true;
   else
    _useStop = false;
}

const char * cXxvAutoTimer::ToString( )
{
  char *buffer = NULL;
  char *tmp[]  = { NULL, NULL, NULL, NULL };
  int pos = 0;
  asprintf(&tmp[pos++], "\t%s%s%s",
                          _searchTitle ? tr("Display1$T") : "-",
                          _searchSubTitle ? tr("Display2$S") : "-",
                          _searchDescription ? tr("Display3$D") : "-");

    if (_useChannel)
    {
      asprintf(&tmp[pos++], "\t%.7s", GetChannelNames());
    }
    else
      asprintf(&tmp[pos++], "\t--");


    if (_useStart)
      asprintf(&tmp[pos++], "\t%02d:%02d", _start / 100, _start % 100);
    else
      asprintf(&tmp[pos++], "\t--:--");



    if (_useStop)
      asprintf(&tmp[pos++], "\t%02d:%02d", _stop / 100, _stop % 100);
    else
      asprintf(&tmp[pos++], "\t--:--");


  asprintf(&buffer, "%c%s%s%s%s\t%s", _active ? ICON_UHR : ' ', tmp[0], tmp[1], tmp[2], tmp[3], _search);

  //release alloctaed buffers
  free(tmp[0]);
  free(tmp[1]);
  free(tmp[2]);
  free(tmp[3]);

  return(buffer);

}


const char * cXxvAutoTimer::GetChannelNames( )
{
  _channelNames.clear();

  for(unsigned int i=0; i<_channels.size(); i++)
  {
    cChannel *chan = Channels.GetByNumber( _channels[i] );
    if( chan != NULL)
    {
      if(i >0) _channelNames += ", ";
      _channelNames += chan->Name();
    }
  }
  return(_channelNames.c_str());
}


const char * cXxvAutoTimer::GetChannelStr( )
{
  _channelStr.clear();

  if( _useChannel == 1 )
  {
    for(unsigned int i=0; i<_channels.size(); i++)
    {
      cChannel *chan = Channels.GetByNumber(_channels[i]);
      if(chan != NULL)
      {
        char *tmp=NULL;
        asprintf(&tmp, "%s-%d-%d-%d", (const char*)cSource::ToString(chan->Source()),
                                            chan->Nid(), chan->Tid(), chan->Sid());
        if( i>0) _channelStr += ",";
        _channelStr += tmp;
        free(tmp);
      }
    }
  }
  return(_channelStr.c_str());
}


const char * cXxvAutoTimer::fieldsToString( )
{
  bool fieldSet=false;
  _searchFields[0] = '\0';
  if( _searchTitle == 1 || _searchSubTitle == 1 || _searchDescription == 1)
  {
     if( _searchTitle == 1)
     { snprintf(_searchFields +strlen(_searchFields), sizeof(_searchFields), FIELDTITLE);
       fieldSet=true;
     }
     if(_searchSubTitle  == 1)
     { if(fieldSet) snprintf(_searchFields +strlen(_searchFields), sizeof(_searchFields), ",");
       snprintf(_searchFields +strlen(_searchFields), sizeof(_searchFields), FIELDSUBTITLE);
       fieldSet=true;
     }
     if( _searchDescription == 1)
     { if(fieldSet) snprintf(_searchFields +strlen(_searchFields), sizeof(_searchFields), ",");
       snprintf(_searchFields +strlen(_searchFields), sizeof(_searchFields), FIELDDESCRIPTION);
       fieldSet=true;
     }
  }
  return(_searchFields);
}




const char * cXxvAutoTimer::CreateUpdateSqlString(Database *db )
{
  char escSearch[2*sizeof(_search)]= "";
  char escDir[2*sizeof(_dir)]      = "";
  Database::OPENDB  *oDb = db->grabdb();

  mysql_real_escape_string(&(oDb->mysql), escSearch, _search, strlen(_search));
  mysql_real_escape_string(&(oDb->mysql), escDir,    _dir, strlen(_dir));
  db->freedb(oDb);
  std::stringstream s;

  s << "UPDATE AUTOTIMER SET Id=" << _id
      << ", Activ='"    << boolToString(_active) << "'"
      << ", Done='"     << boolToString(_done)   << "'"
      << ", Search='"   << escSearch             << "'"
      << ", Infields='" << fieldsToString()      << "'"
      << ", Channels='" << GetChannelStr()       << "'"
      << ", Minlength=" << GetMinlength()
      << ", Priority="  << GetPriority()
      << ", Lifetime="  << GetLifetime()
      << ", Dir='"      << escDir << "'";
 
  _useStart ? s <<  ", Start='" << GetStart() << "'":
              s <<  ", Start=NULL";

  _useStop ? s <<  ", Stop='" << GetStop() << "'":
             s <<  ", Stop=NULL";


    if(!autotimerSetup.xxv020)
    {
      s << ", VPS='"      << boolToString(_vps) << "'" 
        << ", weekdays='" << GetWeekDays()      << "'";


    _useStandardPrevMinutes ?  s <<  ", prevminutes=NULL" :
                               s <<  ", prevminutes='" << GetPrevMinutes() << "'";

    _useStandardAfterMinutes ?  s <<  ", afterminutes=NULL" :
                                s <<  ", afterminutes='" << GetAfterMinutes() << "'";
    }
    s <<  " WHERE Id=" << _id;

    //fprintf(stderr, "UPDATE SQL: %s\n",s.str().c_str());fflush(stderr);
    _sql = s.str();
  return(_sql.c_str());
}

const char * cXxvAutoTimer::CreateInsertSqlString( Database * db )
{
  char escSearch[2*sizeof(_search)]= "";
  char escDir[2*sizeof(_dir)]      = "";
  Database::OPENDB  *oDb = db->grabdb();

  mysql_real_escape_string(&(oDb->mysql), escSearch, _search, strlen(_search));
  mysql_real_escape_string(&(oDb->mysql), escDir,    _dir, strlen(_dir));

  std::stringstream s;

  s <<  "INSERT AUTOTIMER  (Activ, Done,Search,Infields,Channels,  Minlength, Priority, Lifetime, Dir, Start, Stop";

  if(!autotimerSetup.xxv020)
    s << ",VPS, prevminutes, afterminutes, Weekdays";

  
  s << ") VALUES( "
    << "'" << boolToString(_active) << "',"
    << "'" << boolToString(_done)   << "',"
    << "'" << escSearch             << "',"
    << "'" << fieldsToString()      << "',"
    << "'" << GetChannelStr()       << "',"
    << GetMinlength()               << ","
    << GetPriority()                << ","
    << GetLifetime()                << ","
    << "'" << escDir                << "'";

  _useStart ? s << ",'" << GetStart() << "'" :
              s << ",NULL";

  _useStop ? s << ",'" << GetStop() << "'" :
             s << ",NULL";

  if(!autotimerSetup.xxv020)
  {
    s <<  ", '" << boolToString(_vps) << "'";

    _useStandardPrevMinutes  ?  s << ", NULL" : s << ",'" << _prevMinutes  << "'";
    _useStandardAfterMinutes ?  s << ", NULL" : s << ",'" << _afterMinutes << "'";
    s << ", '" << GetWeekDays() << "'";
  }

  
  s << ")";

  _sql = s.str();
  return(_sql.c_str());
}

const char * cXxvAutoTimer::CreateDeleteSqlString( Database * db )
{

  std::stringstream s;
  s <<  "DELETE FROM AUTOTIMER  Where Id=" <<  _id;
  _sql = s.str();
  return(_sql.c_str());
}




void cXxvAutoTimer::SetWeekDays( const char* theValue )
{
  if( theValue != NULL && strlen(theValue)>0)
  {
  _useWeekDays = true;

  strstr(theValue, WEEK_MON)     != NULL ? _weekMon = true : _weekMon = false;
  strstr(theValue, WEEK_TUE)     != NULL ? _weekTue = true : _weekTue = false;
  strstr(theValue, WEEK_WED)     != NULL ? _weekWed = true : _weekWed = false;
  strstr(theValue, WEEK_THU)     != NULL ? _weekThu = true : _weekThu = false;
  strstr(theValue, WEEK_FRI)     != NULL ? _weekFri = true : _weekFri = false;
  strstr(theValue, WEEK_SAT)     != NULL ? _weekSat = true : _weekSat = false;
  strstr(theValue, WEEK_SUN)     != NULL ? _weekSun = true : _weekSun = false;
  }
  else
    _useWeekDays = false;
}




const char * cXxvAutoTimer::GetWeekDays( )
{
  bool fieldSet=false;
  _weekDays[0] = '\0';

  if( _useWeekDays)
  {
    if( _weekMon == 1)
    { snprintf(_weekDays +strlen(_weekDays), sizeof(_weekDays), WEEK_MON);
      fieldSet=true;
    }

    if( _weekTue  == 1)
    { if(fieldSet) snprintf(_weekDays +strlen(_weekDays), sizeof(_weekDays), ",");
      snprintf(_weekDays +strlen(_weekDays), sizeof(_weekDays), WEEK_TUE);
      fieldSet=true;
    }

    if( _weekWed  == 1)
    { if(fieldSet) snprintf(_weekDays +strlen(_weekDays), sizeof(_weekDays), ",");
    snprintf(_weekDays +strlen(_weekDays), sizeof(_weekDays), WEEK_WED);
    fieldSet=true;
    }

    if( _weekThu  == 1)
    { if(fieldSet) snprintf(_weekDays +strlen(_weekDays), sizeof(_weekDays), ",");
    snprintf(_weekDays +strlen(_weekDays), sizeof(_weekDays), WEEK_THU);
    fieldSet=true;
    }

    if( _weekFri  == 1)
    { if(fieldSet) snprintf(_weekDays +strlen(_weekDays), sizeof(_weekDays), ",");
    snprintf(_weekDays +strlen(_weekDays), sizeof(_weekDays), WEEK_FRI);
    fieldSet=true;
    }

    if( _weekSat  == 1)
    { if(fieldSet) snprintf(_weekDays +strlen(_weekDays), sizeof(_weekDays), ",");
    snprintf(_weekDays +strlen(_weekDays), sizeof(_weekDays), WEEK_SAT);
    fieldSet=true;
    }

    if( _weekSun  == 1)
    { if(fieldSet) snprintf(_weekDays +strlen(_weekDays), sizeof(_weekDays), ",");
    snprintf(_weekDays +strlen(_weekDays), sizeof(_weekDays), WEEK_SUN);
    fieldSet=true;
    }
  }

  return(_weekDays);
}

//Sort Timers with Bubble Sort
void cXxvAutoTimers::Sort(enum cXxvAutoTimers::Order order)
{
  int nr= Count();
  if(order==DESC)
  {
    // Bubble up i'th record
    for (int i = 0; i < nr-1; i++)
      for (int j = nr-1; j > i; j--)
    {
      if ( *Get(j) > *Get(j-1) )
        Move(j, j-1 );
    }
  }
  else
  {
    for (int i = 0; i < nr-1; i++)
      for (int j = nr-1; j > i; j--)
    {
      if( *Get(j) < *Get(j-1) )
        Move(j, j-1 );
    }
  }
}

