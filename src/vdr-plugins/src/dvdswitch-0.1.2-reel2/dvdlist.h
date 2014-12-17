#ifndef __DVDLIST_H_
#define __DVDLIST_H_

#include <vdr/tools.h>
//#include <vdr/i18n.h>
#include "dvdlist-item.h"
#include "helpers.h"

class cMainMenu;
class cMainMenuItem;

class cDVDList : public cList<cDVDListItem>
{
  private:
    char *DVDExts;
    char *DVDDirs;
    bool *RunningMenu;

    bool Load(char *dir, eFileList smode, bool sub);
  public:
    cDVDList(bool *runningmenu)
    {
      RunningMenu = runningmenu;
      DVDExts = NULL;
      DVDDirs = NULL;
    }
    ~cDVDList(void)
    {
      free(DVDExts);
      free(DVDDirs);
    }
    //bool Create(char *dir, DVDType dtype = dtAll, DLSortMode smode = dlAsc, bool subdirs = false);
    bool Create(char *dir, char *exts, char *dirs, eFileList smode = sNone, bool sub = false);
    
};


class cDVDListThread : public cThread
{
  private:
    cMutex mutex;
    cMainMenu *menu;
    bool RunningMenu;
    char *dir;
    int FirstSelectable;
    void (cMainMenu::*SelectFirst) (int);
    void Add(cMainMenuItem *item);
    void BuildDisp0(void);
    void BuildDisp1(void);
    void BuildDisp2(void);
    
  protected:
    virtual void Action(void);

  public:
    cDVDListThread(void);
    void SetCB(cMainMenu *menu, void (cMainMenu::*SelectFirst) (int));
    bool BuildDisp(cMainMenu *menu, void (cMainMenu::*SelectFirst) (int), char *dir);
    void Stop() { RunningMenu = false; menu = NULL; };
};

#endif // __DVDLIST_H_

