#ifndef __MENU_DVDSWITCH_H
#define __MENU_DVDSWITCH_H

#include <vdr/osdbase.h>
#include "menu-item.h"
#include "dvdlist.h"


#ifndef DBG
#define  DBG " MYDEBUG -- DVD-Switch: "
#endif

#define MAXOSDTEXTWIDTH 45

enum eMainMenuState
{
  mmsNone,
  mmsReInit,
  mmsReInitCur,
  mmsImgRename,
};

class cCMDImage;

class cMainMenu : public cOsdMenu
{
  private:
    int FirstSelectable;
    eMainMenuState State;
    cCMDImage *CMDImg;
    cDVDListThread *thread;

    void Init(void);
    void SetMenuTitle(void);
    void Build(char *dir);
    void SelectFirst(int first);
    eOSState MenuMove(eKeys Key);
    eOSState SelectItem(void);
    eOSState Commands(eKeys Key);
  public:
    cMainMenu(cDVDListThread *listThread);
    ~cMainMenu(void);
    void SetHelp(void);
    virtual eOSState ProcessKey(eKeys Key);

    static char *CreateOSDName(eMainMenuItem itype, char *file = NULL);
    void SetState(eMainMenuState state);
    eMainMenuState GetState(void) { return State; };
    void SetStatus(const char*s){cOsdMenu::SetStatus(s); };
};

class cDirHandlingOpt
{
  char *IDir;
  char *CDir;
  char *PDir;
  char *LSDir;
  char *LSItemName;
  eMainMenuItem LSItemType;

  public:
    cDirHandlingOpt(void)
    {
      IDir = NULL;
      CDir = NULL;
      PDir = NULL;
      LSDir = NULL;
      LSItemName = NULL;
      LSItemType = iCat;
    }
    ~cDirHandlingOpt(void)
    {
      free(IDir);
      free(CDir);
      free(PDir);
      free(LSDir);
      free(LSItemName);
    }
    char *ImageDir(char *dir = NULL)
    {
      if(dir)
      {
        FREENULL(IDir);
        IDir = strdup(dir);
      }
      return IDir;
    }
    char *CurrentDir(char *dir = NULL)
    {
      if(dir)
      {
        FREENULL(CDir);
        CDir = strdup(dir);
      }
      return CDir;
    }
    char *ParentDir(char *dir = NULL)
    {
      if(dir)
      {
        FREENULL(PDir);
        PDir = strdup(dir);
      }
      return PDir;
    }
    char *LastSelDir(char *dir = NULL)
    {
      if(dir)
      {
        FREENULL(LSDir);
        LSDir = strdup(dir);
      }
      return LSDir;
    }
    bool isParent(char *dir)
    {
      if(dir && PDir && strcasecmp(dir, PDir))
        return true;
      return false;
    }
    bool isLastSel(char *dir)
    {
      if(dir && LSDir && !strcasecmp(dir, LSDir))
        return true;
      return false;
    }
    void setLastSelectItemName(char *filename = NULL)
    {
      FREENULL(LSItemName);
      if(filename)
        LSItemName = strdup(filename);
    };
    char *getLastSelectItemName(void) { return LSItemName; }
    eMainMenuItem LastSelectItemType(eMainMenuItem type = iCat)
    {
      if(type != iCat)
        LSItemType = type;
      return LSItemType;
    }
};

class cDirHandling
{
  private:
    cOsdMenu *OsdObject;
    cDirHandlingOpt *DirObject;
  public:
    cDirHandling(cOsdMenu *osdobject, cDirHandlingOpt *dirobject);

    int Build(char *dir, bool emptydirs);
    void ProcessKey(cMainMenuItem *mItem);
};

extern cDirHandlingOpt MainMenuOptions;
extern cDVDList *DVDList;
extern bool refreshList;

#endif // __MENU_DVDSWITCH_H
