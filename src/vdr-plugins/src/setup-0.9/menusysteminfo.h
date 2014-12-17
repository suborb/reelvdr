
#ifndef  MENU_SYSTEM_INFO_H
#define  MENU_SYSTEM_INFO_H

#include <vdr/menu.h>
#include <vdr/osdbase.h>
#include <vdr/menuitems.h>
#include <vdr/interface.h>
#include <vdr/submenu.h>

#include "meminfo.h"
#include "netinfo.h"
#include "systeminfo.h"

bool HasDevice(const char *deviceID);

class cMenuInfoItem;
class cCredits;

/**
 *  background thread that checks the internet connectivity 
 */
class cNetworkCheckerThread : public cThread {
    private:
        bool hasFinishedRoutingTest;
        bool hasFinishedDnsTest;
        bool isRoutingOK;
        bool isDnsOK;
    public:
        cNetworkCheckerThread(void);
        bool HasFinished(void) const { return (hasFinishedRoutingTest && hasFinishedDnsTest); }
        bool HasFinishedRoutingTest(void) const { return hasFinishedRoutingTest; }
        bool HasFinishedDnsTest(void) const { return hasFinishedDnsTest; }
        bool IsRoutingOK(void) const { return isRoutingOK; }
        bool IsDnsOK(void) const { return isDnsOK; }
        virtual void Action(void);
};

// ---- Class cMenuSystemInfo ---------------------------------------------

class cMenuSystemInfo : public cOsdMenu
{
  public:
    cMenuSystemInfo();
 //   ~cMenuSystemInfo();
    void Uname();

    char *GenerateChecksum(); // -> sysInfo
    ///< needs free()
    eOSState ProcessKey(eKeys Key);
    typedef std::vector<std::string>::const_iterator vConstIter;
  private:
    cMemInfo meminfo;
    cSysInfo *sysInfo;

    cCredits *credits;
    char *thankTo;
    void Set();
    bool expert;
};

// ---- Class cMenuNetInfo  ---------------------------------------------
//
class cMenuNetInfo : public cOsdMenu
{
  public:
    cMenuNetInfo();
    void Set(); /** Set the menuitems */
    eOSState ProcessKey(eKeys Key);
    typedef std::vector <std::string>::const_iterator vConstIter;
  private:
#ifndef RBLITE
    cNetworkCheckerThread networkChecker;
    cOsdItem *netStatItem;
    int ticker;
#endif
    bool showDetails;
    cNetInfo netinfo;
};

// ---- Class cMenuInfoItem  ---------------------------------------------
class cMenuInfoItem:public cOsdItem
{
  public:
    cMenuInfoItem(const char *text, const char *textValue = NULL);
    cMenuInfoItem(const char *text, int intValue);
    cMenuInfoItem(const char *text, bool boolValue);
};


class cMenuInfoItem;

// ---- Class cCredits  ---------------------------------------------

class cCredits
{
  public:
    cCredits();
    ~cCredits();
    void Load();
    const char *Text();
  private:
    int editableWidth_;
    std::vector<char>text_;
    std::vector<std::string > strText_;
};

inline const char *cCredits::Text()
{
    return &text_[0];
}


#endif //MENU_SYSTEM_INFO_H
