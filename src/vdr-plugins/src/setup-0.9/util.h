/*********************************************************
 na1* DESCRIPTION:
 *             Header File
 *
 * $Id: util.h,v 1.4 2005/10/03 14:05:20 ralf Exp $
 *
 * Contact:    ranga@teddycats.de
 *
 * Copyright (C) 2004 by Ralf Dotzert
 *********************************************************/

#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vdr/thread.h>

#if VDRVERSNUM >= 10716
#define ELEMENT TINYXML_ELEMENT
#endif

enum eEntryType
{
    etUndefined,
    etText,
    etNumberText,
    etHex,
    etIp,
    etBool,
    etNumber,
    etSelection
};

enum eComState
{
    eCSCommandNoOp = 0,
    eCSCommandRunning,
    eCSCommandError,
    eCSCommandSuccess,
};


namespace setup
{
    bool MakeBool(std::string, bool defaults);
    void SwitchChannelList(const char *selectedList);
    const char *ExecuteCommand(const char *Cmd);
//  std::string FileNameFactory(std::string Filename);
}

/**
@author Ralf Dotzert
*/
class Util
{
  public:
    Util();
    ~Util();
    enum Type { UNDEFINED, TEXT,CONSTANT_TEXT, NUMBER_TEXT, HEX, IP, BOOL, NUMBER, SELECTION, PIN, SPACE };
    static char *Strdupnew(const char *str);
    static char *Strdupnew(const char *prefix, const char *str);
    static char *Strdupnew(const char *str, int size);
    static bool isBool(const char *string, bool & flag);
    static bool isBool(const char *string, int &flag);
    static bool isType(const char *string, Util::Type & typ);
    static bool isNumber(const char *string, int &number);
    static const char *typeToStr(Type type);
    static const char *boolToStr(bool val);
};

// ----- Class cChildLock -----------------------------------

class cChildLock
{
  private:
    bool childLock_;
    std::string childLockSysConf_;
  public:
    cChildLock();
    void Init();
    bool IsLocked();
    bool CompareInput(std::string Input);
};

inline bool
cChildLock::IsLocked()
{
    return childLock_;
}

inline bool
cChildLock::CompareInput(std::string Input)
{
    //dsyslog(DBG " cChildLock::CompareInput  sys %s  input %s",
    //        childLockSysConf_.c_str(), Input.c_str());
    if (childLockSysConf_ == Input)
    {
        //dsyslog(DBG " cChildLock::CompareInput  return _true_");
        childLock_ = false;
        return true;
    }

    childLock_ = true;
    return false;
}


// ---- Class cExecuteCommand ---------------------------------------------
class cExecuteCommand:public cThread
{
  private:
    std::string command_;
    volatile eComState comState_;
  protected:
    void Action(void);
  public:
    cExecuteCommand();
    //~cExecuteCommand();
    eComState Execute(std::string Command);
    eComState ExecState();
    void Reset();
};

inline eComState cExecuteCommand::ExecState()
{
    return comState_;
}

inline void
cExecuteCommand::Reset()
{
    //dsyslog(DBG "  cExecuteCommand: RESET ");
    comState_ = eCSCommandNoOp;
}

// ---- Class TimerString --------------------------------------------------
class TimerString:public cListObject
{
  private:
    cString _timer;
  public:
    TimerString(cString timer)
    {
        _timer = timer;
    }
    cString GetTimerString()
    {
        return (_timer);
    }
};

// ---- Class TimerList  --------------------------------------------------
class TimerList
{
  private:
    cList < TimerString > myTimers;
  public:
    TimerList()
    {
    };
    ~TimerList()
    {
    };
    void SaveTimer();
    void RestoreTimer();
};


#endif
