#ifndef __SETUP_DVDSWITCH_H
#define __SETUP_DVDSWITCH_H

#include <vdr/menuitems.h>

class cDVDSwitchSetup
{
  public:
    int HideMenuEntry;
    char MenuName[50];
    bool CustomMenuName;
    char ImageDir[MaxFileName];
    bool ImageDirPerParam;
    int ViewFreeDiskSpace;
    
    int MaxDModes;
    const char *DModes[10];
    int DisplayMode;
    int MaxCTypes;
    const char *CTypes[10];
    int CategorieType;
    int HideEmptyDirs;
    int MaxSModes;
    const char *SModes[10];
    int SortMode;
    int DisplayDVDDevice;

    int HideTypeCol;
    int CountTypCol;
    int HideImgSizeCol;
    char CatLineChar;
    char *CatLineChars;
    int CharCountBeforeCat;
    int SpacesBeforeAfterCat;
    char SubCatCutter[7];
    char ChangeCharsOSDName[20];

    int JumpCatByKey;

    char ConfigDirectory[MaxFileName];
    bool HaveXinePlugin;
    char DVDLink[MaxFileName];
    char DVDLinkOrg[MaxFileName];
    char DVDReadScript[MaxFileName];
    char DVDWriteScript[MaxFileName];

    int MaxCommands;
    const char *Commands[15];
    const char *CommandsShortName[15];
    int k1;
    int k2;
    int k3;
    int k4;
    int k5;
    int k6;
    int k7;
    int k8;
    int k9;
    int k0;
    int kRed;
    int kGreen;
    int kYellow;
    int kBlue;
    int kOk;
    
    cDVDSwitchSetup(void);
    ~cDVDSwitchSetup(void);
    void Init(void);
    bool SetupParse(const char *Name, const char *Value);
    void Debug(void);

    void SetConfDir(char *value)
    {
      strcpy(ConfigDirectory, value);
      strcpy(DVDLink, ConfigDirectory);
      strcat(DVDLink, "/dvdlink");
    }
    void SetDVDDevice(char *value) { strcpy(DVDLinkOrg, value); }
};

class cMenuSetupDVDSwitch : public cMenuSetupPage
{
  private:
    cDVDSwitchSetup data;
    bool ViewGeneric;
    int ViewGenericPos;
    bool ViewDisplay;
    int ViewDisplayPos;
    bool ViewSpecialDisplay;
    int ViewSpecialDisplayPos;
    bool ViewNav;
    int ViewNavPos;
    bool ViewKey;
    int ViewKeyPos;
  protected:
    void Set(void);
    virtual void Store(void);
    virtual eOSState ProcessKey(eKeys Key);
  public:
    cMenuSetupDVDSwitch(void);
};

class cMenuEditCatItem : public cOsdItem
{
  private:
    char *Name;
  public:
    cMenuEditCatItem(const char *name, bool view = false);
    ~cMenuEditCatItem(void);
};

extern cDVDSwitchSetup DVDSwitchSetup;

#endif // __SETUP_DVDSWITCH_H
