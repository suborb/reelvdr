/****************************************************************************
 * DESCRIPTION:
 *            
 *
 * $Id: menu.cpp,v 1.9 2006/08/15 20:35:22 ralf Exp $
 *
 * Contact:    ranga@vdrtools.de
 *
 * Copyright (C) 2005 by Ralf Dotzert
 ****************************************************************************/
#include <stdlib.h>

#include "menu.h"
#include "setup.h"
#include "mysocket.h"
#include <string>

static const char * AllowedChar = " aäbcdefghijklmnoöpqrsßtuüvwxyz0123456789-.#~_\'\"/";

static struct XXV_Command xxvCommands[] =
      {
        {"create timers", "au\r\n"},
        {"read EPG data", "er\r\n"},
        {"read channels", "cu\r\n"},
      };
static int nrXxxvCommands=sizeof(xxvCommands)/sizeof(struct XXV_Command);

cXxvAutotimerMenu::cXxvAutotimerMenu() : cOsdMenu(tr("Autotimer for XXV"), 2, 6,8,6,6)
{
   _state = UNDEF;
   _syslog =  new SysLog("vdr:xxvautotimer");
   _db = new Database(autotimerSetup.host,   autotimerSetup.user,
                      autotimerSetup.passwd, autotimerSetup.database, _syslog);

   if(!_db->Connected())
     Skins.Message(mtError,tr("Could not connect to Mysql Database"));
   else
    if( getTimers())
       Set();
}


cXxvAutotimerMenu::~cXxvAutotimerMenu()
{
  delete _db;
  delete _syslog;
}

void cXxvAutotimerMenu::Set( )
{
  int current = Current();
  int nr;
  Clear();
  _timers.Sort(cXxvAutoTimers::ASC);
  for (cXxvAutoTimer *timer = _timers.First(); timer; timer = _timers.Next(timer))
  {
    const char *tmp = timer->ToString();
    Add(new cOsdItem (tmp));
    free((void*)tmp);
  }

  nr = Count();
  if( current >= nr)
    SetCurrent(Get(nr-1));
  else
    SetCurrent(Get(current));

  if( nr > 0 )
    SetHelp( tr("Commands"), tr("New"), tr("Delete"), tr("On/Off"));
  else
    SetHelp( tr("Commands"), tr("New"), NULL, NULL);

  Display();
}

eOSState cXxvAutotimerMenu::ProcessKey( eKeys Key )
{
    bool HadSubMenu = HasSubMenu();
    cXxvAutoTimer *timer = NULL;
    eOSState state = cOsdMenu::ProcessKey(Key);
    int current = Current();

    if(!HasSubMenu())
        switch(Key)
        {
            case kRed:
                _state = COMMAND;
                return(AddSubMenu(new cXxvAutotimerCommand()));
                break;
            case kOk :
                switch (_state)
                {
                    case NEW:
                        timer = new cXxvAutoTimer();
                        *timer = _currTimer;
                        _timers.Add(timer);
                        insertTimer();
                        break;
                    case EDIT:
                        if( (timer = _timers.Get(current)) != NULL)
                        {
                            *timer = _currTimer;
                            updateTimer();
                        }
                        break;
                    case UNDEF:
                        if( (timer = _timers.Get(current)) != NULL)
                        {
                            _currTimer = *timer;
                            _state= EDIT;
                            return(AddSubMenu(new cXxvAutotimerEdit(&_currTimer)));
                        }
                        break;
                    default:
                        break;
                }
                Set();
                _state = UNDEF;
                return(osContinue);
                break;

                break;
            case kGreen://New Timer
                _currTimer.Reset();
                _state = NEW;
                return(AddSubMenu(new cXxvAutotimerEdit(&_currTimer)));
                break;
            case kYellow:
                //delete timer
                if(_timers.Count()> 0 && Interface->Confirm(tr("Delete autotimer?")))
                {
                    timer = _timers.Get(current);
                    if( timer != NULL)
                    {
                        _currTimer=*timer;
                        _timers.Del(timer);
                        deleteTimer();
                        Set();
                    }
                }
                _state=UNDEF;
                break;
            case kBlue:
                if(_timers.Count()>0)
                {
                    timer = _timers.Get(current);
                    if( timer != NULL)
                    {
                        timer->SetActive(!timer->GetActive());
                        _currTimer=*timer;
                        updateTimer();
                        Set();
                    }
                }

                break;
            case kBack:
                if(HadSubMenu)
                {
                    if(_state == NEW)
                    {
                        timer = _timers.Get(Count());
                        if( timer != NULL)
                            _timers.Del(timer);
                    }
                    Set();
                    _state = UNDEF;
                    return(osContinue);
                }
                else
                    return osBack;
                break;
            case kNone:
                break;

            default :
                break;
        }
    return state;
}


void cXxvAutotimerMenu::updateTimer( )
{
  const char *sql = _currTimer.CreateUpdateSqlString(_db);
  Query q(*_db);
  if(! q.execute(sql))
    Skins.Message(mtError,tr("Update of Database failed!"));
  q.free_result();
}


void cXxvAutotimerMenu::insertTimer( )
{
  const char *sql = _currTimer.CreateInsertSqlString(_db);
  Query q(*_db);
  if(! q.execute(sql))
    Skins.Message(mtError,tr("Can not Insert Record!"));
  q.free_result();
}

void cXxvAutotimerMenu::deleteTimer( )
{
  const char *sql = _currTimer.CreateDeleteSqlString(_db);
  Query q(*_db);
  if(! q.execute(sql))
    Skins.Message(mtError,tr("Can not Delete Record!"));
  q.free_result();
}


//-------------------------------------------------
bool cXxvAutotimerMenu::getTimers( )
{
  bool result = true;

  Query q(*_db);
  if(autotimerSetup.xxv020)
    q.get_result("select id, activ,done,search,infields,channels,start,stop,minlength,priority,lifetime,dir from AUTOTIMER ORDER BY search");
  else
    q.get_result("select id, activ,done,search,infields,channels,start,stop,minlength,priority,lifetime,dir,vps,prevminutes,afterminutes,weekdays from AUTOTIMER ORDER BY search");
  
  while (q.fetch_row())
  {
    cXxvAutoTimer *t = new cXxvAutoTimer;
    t->SetId ( q.getval(0));
    if(! q.is_null(1))   t->SetActive    ( q.getstr(1));
    if(! q.is_null(2))   t->SetDone      ( q.getstr(2));
    if(! q.is_null(3))   t->SetSearch    ( q.getstr(3));
    if(! q.is_null(4))   t->SetInfields  ( q.getstr(4));
    if(! q.is_null(5))   t->SetChannels  ( q.getstr(5));
    if(! q.is_null(6))   t->SetStart     ( atoi(q.getstr(6)));
    if(! q.is_null(7))   t->SetStop      ( atoi(q.getstr(7)));
    if(! q.is_null(8))   t->SetMinlength ( q.getval(8));
    if(! q.is_null(9))   t->SetPriority  ( q.getval(9));
    if(! q.is_null(10))  t->SetLifetime  ( q.getval(10));
    if(! q.is_null(11))  t->SetDir       ( q.getstr(11));

    if(! autotimerSetup.xxv020)
    {
      if(! q.is_null(12))  t->SetVPS          ( q.getstr(12));  // VPS
      if(! q.is_null(13))  t->SetPrevMinutes  ( q.getval(13));  // prevminutes
      if(! q.is_null(14))  t->SetAfterMinutes ( q.getval(14));  // afterminutes
      if(! q.is_null(15))  t->SetWeekDays     ( q.getstr(15));  // Weekdays
    }

    _timers.Add(t);
  }
  q.free_result();

  return(result);
}




//---------------------------------------------------------------------------------------
// AutotimerEdit
//---------------------------------------------------------------------------------------
cXxvAutotimerEdit::cXxvAutotimerEdit(cXxvAutoTimer *timer ): cOsdMenu(tr("Edit XXV Autotimers"), 35)
{
  _timer = timer;
  Set();
}

cXxvAutotimerEdit::~ cXxvAutotimerEdit( )
{
}

void cXxvAutotimerEdit::Set()
{
  int current = Current();
  Clear();

  Add(new cMenuEditBoolItem(tr("Active"),         _timer->GetActiveRef(), tr("no"), tr("yes")));
  Add(new cMenuEditStrItem(tr("Searchstring"),    _timer->GetSearch(), _timer->GetSearchLen(),
                                                   AllowedChar));
  Add(new cMenuEditBoolItem(tr("Search in title"),        _timer->GetSearchTitleRef(), tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Search in subtitle"),     _timer->GetSearchSubTitleRef(), tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Search in descritption"), _timer->GetSearchDescriptionRef(), tr("no"), tr("yes")));

  Add(new cMenuEditBoolItem(tr("Ignore recurrences"), _timer->GetDoneRef(), tr("no"), tr("yes")));

  Add(new cMenuEditBoolItem(tr("Use start time"),         _timer->GetUseStartRef(), tr("no"), tr("yes")));

  if( _timer->GetUseStart() )
     Add(new cMenuEditTimeItem(tr("  Start time of search"), _timer->GetStartRef()));

  Add(new cMenuEditBoolItem(tr("Use stop time"),     _timer->GetUseStopRef(), tr("no"), tr("yes")));
  if( _timer->GetUseStop() )
    Add(new cMenuEditTimeItem(tr("  End time of search"), _timer->GetStopRef()));

  Add(new cMenuEditBoolItem(tr("Use channel select"),     _timer->GetUseChannelRef(), tr("no"), tr("yes")));
  if( _timer->GetUseChannel())
  {
    Add(new cMenuEditIntItem(tr("Nr of channels"), _timer->GetNrChannlesToUseRef(), 1, 64));
    _timer->AdjustNrOfChannels();
    for(int i=0; i<_timer->GetNrOfChannels(); i++)
      Add(new cMenuEditChanItem(tr("  Channel"), _timer->GetChannelNumberRef(i)));
  }

  Add(new cMenuEditStrItem(tr("Directory for recording"),    _timer->GetDir(), _timer->GetDirLen(), AllowedChar));
  /*
  Add(new cMenuEditIntItem(tr("Minimum length"), _timer->GetMinlengthRef(), 0, 2000));
  Add(new cMenuEditIntItem(tr("Priority"),       _timer->GetPriorityRef(),  0, MAXPRIORITY));
  Add(new cMenuEditIntItem(tr("Lifetime"), _timer->GetLifetimeRef(),  0, MAXLIFETIME));
  */
  //XXV > Version 0.20
  if(! autotimerSetup.xxv020)
  {
    Add(new cMenuEditBoolItem(tr("Use VPS"),     _timer->GetVPSRef(), tr("no"), tr("yes")));
    Add(new cMenuEditBoolItem(tr("Weekdays"), _timer->GetUseWeekDaysRef(), tr("no"), tr("yes")));
    if( _timer->GetUseWeekDays())
    {
      Add(new cMenuEditBoolItem(tr("  Monday"),   _timer->GetWeekMonRef(), tr("no"), tr("yes")));
      Add(new cMenuEditBoolItem(tr("  Tuesday"),  _timer->GetWeekTueRef(), tr("no"), tr("yes")));
      Add(new cMenuEditBoolItem(tr("  Wednesday"),_timer->GetWeekWedRef(), tr("no"), tr("yes")));
      Add(new cMenuEditBoolItem(tr("  Thursday"), _timer->GetWeekThuRef(), tr("no"), tr("yes")));
      Add(new cMenuEditBoolItem(tr("  Friday"),   _timer->GetWeekFriRef(), tr("no"), tr("yes")));
      Add(new cMenuEditBoolItem(tr("  Saturday"), _timer->GetWeekSatRef(), tr("no"), tr("yes")));
      Add(new cMenuEditBoolItem(tr("  Sunday"),   _timer->GetWeekSunRef(), tr("no"), tr("yes")));
    }

    Add(new cMenuEditBoolItem(tr("Default Recording at start"),     _timer->GetUseStandardPrevMinutesRef(), tr("no"), tr("yes")));
    if(! _timer->GetUseStandardPrevMinutes())
       Add(new cMenuEditIntItem(tr("  Recording at start (min)"), _timer->GetPrevMinutesRef(),  0, 60));

    Add(new cMenuEditBoolItem(tr("Default Recording at stop"),     _timer->GetUseStandardAfterMinutesRef(), tr("no"), tr("yes")));
    if(! _timer->GetUseStandardAfterMinutes())
      Add(new cMenuEditIntItem(tr("  Recording at stop (min)"), _timer->GetAfterMinutesRef(), 0, 60));
  }
  
  if( Count() < current)
    SetCurrent(Get(Count()));
  else
    SetCurrent(Get(current));
  Display();
}

eOSState cXxvAutotimerEdit::ProcessKey( eKeys Key )
{
    bool prevUseStartTimer = _timer->GetUseStart();
    bool prevUseStopTimer  = _timer->GetUseStop();
    bool prevUseChannel    = _timer->GetUseChannel();
    int  prevNrChannlesToUse = _timer->GetNrChannlesToUse();
    bool prevUseWeekDays     = _timer->GetUseWeekDays();
    bool prevUseStandardPrevMinutes  = _timer->GetUseStandardPrevMinutes();
    bool prevUseStandardAfterMinutes = _timer->GetUseStandardAfterMinutes();

    eOSState state = cOsdMenu::ProcessKey(Key);

    if( prevUseStartTimer != _timer->GetUseStart() ||
        prevUseStopTimer  != _timer->GetUseStop()  ||
        prevUseChannel    != _timer->GetUseChannel() ||
        prevNrChannlesToUse  != _timer->GetNrChannlesToUse() ||
        prevUseWeekDays      != _timer->GetUseWeekDays() ||
        prevUseStandardPrevMinutes   != _timer->GetUseStandardPrevMinutes() ||
        prevUseStandardAfterMinutes  != _timer->GetUseStandardAfterMinutes()
      )
    {
      _timer->AdjustNrOfChannels();
      Set();
      Display();
    }

    switch(Key)
    {
            case kOk :
                  if( state == osUnknown)
                  {
                    int lenSearchText= strlen(_timer->GetSearch());
                    if (lenSearchText == 0)
                    {
                        Skins.Message(mtError,tr("Missing search text!"));
                    }
                    else
                      if(lenSearchText < 4 )
                      {
                        if(Interface->Confirm(tr("Search text to small - use anyway?")))
                          return(osBack);
                      }
                      else
                        return(osBack);
                  }
                  return(osContinue);
                break;
            case kRed:
                      break;
            case kGreen:
                      break;
            case kYellow:
                      break;
            case kBlue:
                      break;
            case kBack:
                      return osBack;
                      break;
           case kNone:
                      break;

            default :
                      break;
    }
    return state;
}

cXxvAutotimerCommand::cXxvAutotimerCommand( ): cOsdMenu(tr("XXV Commands"), 25)
{

  Set();
}

cXxvAutotimerCommand::~ cXxvAutotimerCommand( )
{
  mode = UNDEF;
}

void cXxvAutotimerCommand::Set( )
{
  Clear();

  for(int i=0; i<nrXxxvCommands; i++)
      Add( new cOsdItem(tr(xxvCommands[i].description)));

  SetHelp( tr("Reset Mysql"), NULL, NULL, NULL);
  Display();
}

eOSState cXxvAutotimerCommand::ProcessKey( eKeys Key )
{
  int current = Current();
  eOSState state = cOsdMenu::ProcessKey(Key);
  
  switch(Key)
  {
    case kOk :
         if(mode == COMMAND)
           return osBack;
         else
          if(current>=0 && current <nrXxxvCommands)
          {
            SendCommand(xxvCommands[current].command);
            mode = COMMAND;
          }
         return(osContinue);
      break;
    case kRed:
      CallSystem();
      return osEnd;
      break;
    case kGreen:
      break;
    case kYellow:
      break;
    case kBlue:
      break;
    case kBack:
      return osBack;
      break;
    case kNone:
      break;

    default :
      break;
  }
  return state;

}

void cXxvAutotimerCommand::SendCommand( char * command )
{
  cMySocket sock;


  Skins.Message(mtStatus,tr("Please Wait ..."));
  Skins.Flush();
  printf (" sock.Send(command) %s \n", command);

  if( sock.Connect(autotimerSetup.host, autotimerSetup.telnetPort) &&
      sock.Receive(20)   &&
      sock.Send(command) &&
      sock.Receive(20)   &&
      sock.Send("quit\r\n")
    )
  {
    AddSubMenu(new cMenuText("", sock.GetRecBuf().c_str(), fontOsd));
    Skins.Message(mtStatus,tr("done ..."));
  }
  else
  {
    SetStatus(tr("Error can not send XXV Command"));
  }
  sock.Disconnect();
}

void cXxvAutotimerCommand::CallSystem()
{
    Skins.Message(mtStatus,tr("Please Wait ..."));
    const char *myscript = "/usr/bin/sudo xxv-recover.sh";
    system(myscript);
    Skins.Message(mtStatus,tr("done ..."));
    ::usleep(5000000);
}
