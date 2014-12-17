/*********************************************************
 * DESCRIPTION: 
 *             Header File
 *
 * $Id: setupmenu.h,v 1.13 2005/10/03 14:05:20 ralf Exp $
 *
 * Contact:    ranga@teddycats.de        
 *
 * Copyright (C) 2004 by Ralf Dotzert 
 *********************************************************/

#ifndef SETUPMENU_H
#define SETUPMENU_H

#include <string>
#include <vector>

#if VDRVERSNUM >= 10716
#  include "vdr/help.h"
#endif
#include <vdr/menu.h>
#include <vdr/osdbase.h>
#include <vdr/menuitems.h>
#include <vdr/interface.h>
#include <vdr/submenu.h>

#include <vdr/debugmacros.h>

#include "config.h"
#include "menusysteminfo.h"
#include "util.h"


/**
@author Ralf Dotzert and Markus Hahn
**/

// ---- Class cExecuteMenu  ---------------------------------------------

class cExecuteMenu:public cOsdMenu
{
  public:
    cExecuteMenu(const char *Command);
  private:
    void Execute();
    cSubMenu subMenu_;          //.ExecuteCommand(Command);
    const char *command_;
};

// ---- Class cSetupVdrMenu  ---------------------------------------------

class cSetupVdrMenu:public cOsdMenu
{
  private:
    enum MenuState
    { UNDEFINED, MOVE, CREATE, EDIT, DELETE };
    cSubMenu vdrSubMenu_;
    char *createMenuName(cSubMenuNode * node);
    void setHelp();
    MenuState menuState_;

    int startIndex_;
    int createEditNodeIndex_;

    char editTitle_[35];
  public:
      cSetupVdrMenu(const char *title);
     ~cSetupVdrMenu();
    void Set();
    eOSState ProcessKey(eKeys Key);
};

// ---- Class cSetupSystemMenu  ------------------------------------------
class cSetupSystemMenu          // ???
{
  public:
    cSetupSystemMenu();
    ~cSetupSystemMenu();
    static cOsdMenu *GetSystemMenu(const char *InternalName);
};


// ---  Class cSetupMenuTimezone  --------------------------------
// taken from Bratfisch`s install  cInstallTimezoneSecLang
class cSetupMenuTimezone:public cOsdMenu
{
  private:
    int originalNumLanguages;
    int numLanguages;
    int optLanguages;
    void Set();
    void Save();
    int setSystemTime;
    const char *systemTimeValues[3];
    int timeTransponder;
    int timeSource;
    char *con_values[64];
    int nr_con_values;
    int currentCon;
    char *city_values[256];
    int nr_city_values;
    int currentCity;
    int currentTimezone;
    static const int stepNumber = 3;
  public:
    cSetupMenuTimezone();
    eOSState ProcessKey(eKeys Key);
};

// ---- Class cSetupMenu  ---------------------------------------------
class cSetupMenu:public cOsdMenu
{
  public:
    cSetupMenu(const char *link = NULL, const char *submenu = NULL);
    ~cSetupMenu();
    void Set();
    eOSState ProcessKey(eKeys Key);
    eOSState CallPlugin(Menu * xmlMenu);
  private:
    bool setupLocked_;
    cChildLock childLock_;
    ///< must be after setupConfig!
    char childLockEntered_[5];
    cExecuteCommand command_;

// TB    cCredits credits_;
    cHelp help_;
    bool linkMenu_;
    bool calledSubmenu_;
    const char *menuLabel_;
};

// ---- Class cSetupGenericMenu ---------------------------------------------
class cSetupGenericMenu:public cOsdMenu
{
  public:
    cSetupGenericMenu(const char *Title, MenuNode * Node);
    void Set();
    eOSState ProcessKey(eKeys Key);
    void ExecuteCommand(const char *cmd);
    eOSState AddMenu(MenuNode * node, eMenuType type);
    eOSState CallPlugin(Menu * xmlMenu);
    void SaveValues(MenuNode*);
    void DisplayStatus(MenuNode * xmlNode);
    eOSState DisplayHelp(MenuNode * currentNode);
  private:
    MenuNode * xmlNode_;
    cString title_;
    bool wasEdited_;            //XXX
    cExecuteCommand command_;
    int execState_;
    bool init_;
    cHelp help_;
    const char *nohk(const char *str) const;
    void BuildMenu(MenuNode*);
};

// ---- Class cMenuEditIpNumItem  ---------------------------------------------
class cMenuEditIpNumItem:public cMenuEditItem
{
  private:
    cMenuEditIpNumItem(const cMenuEditIpNumItem &);     // just for save

    Util::Type val_type;
    int pos;
    int digit;
    char *value;     // comment
    unsigned char val[5];
    virtual void Set(void);

    void Init();
    unsigned char Validate(char *field);
  public:
    cMenuEditIpNumItem(const char *Name, char *Value);
    ~cMenuEditIpNumItem();
    virtual eOSState ProcessKey(eKeys Key);
};

#if  1 // not used with reelvdr 
// ---- Class cSetupPluginParameter ---------------------------------------------
class cSetupPluginParameter:public cOsdMenu
{
  private:
    char editParameter_[500];
    bool _edit_;
  public:
    cSetupPluginParameter(const char *title);
    ~cSetupPluginParameter();
    void Set();
    eOSState ProcessKey(eKeys Key);
};
#endif

#if  1 // not used with reelvdr 
// ---- Class cSetupPluginMenu ---------------------------------------------
class cSetupPluginMenu:public cOsdMenu
{
  private:
    int startIndex_;
    bool moveMode_;
    bool sortMode_;
  public:
    cSetupPluginMenu();
    ~cSetupPluginMenu();
    void Set();
    eOSState ProcessKey(eKeys Key);
};

#endif

#endif  // SETUPMENU_H
