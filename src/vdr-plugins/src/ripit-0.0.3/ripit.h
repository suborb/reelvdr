#ifndef __RIPIT_H
#define __RIPIT_H

#include <vdr/plugin.h>
#include <string>
#include <vdr/osd.h>
#include "ripitosd.h"

static const char *VERSION = "0.0.3";
static const char *DESCRIPTION = trNOOP("Audio-CD ripper");
static const char *MAINMENUENTRY = trNOOP("Archive Audio-CD");
static const char *MENUSETUPPLUGINENTRY = trNOOP("Audio-CD archive ");   // TB: a space at the end to be able to distinguish it from the main menu entry "Audio-CD archiv"


class cPluginRipit:public cPlugin
{
    // Add any member variables or functions you may need here.
  private:
    cRipitOsd * ripitosd;
  public:
    cPluginRipit(void);
      virtual ~ cPluginRipit();
    virtual const char *Version(void)
    {
        return VERSION;
    }
    virtual const char *Description(void)
    {
        return tr(DESCRIPTION);
    }
    virtual const char *MenuSetupPluginEntry()
    {
        return tr(MENUSETUPPLUGINENTRY);
    }
    virtual const char *CommandLineHelp(void);
    virtual bool ProcessArgs(int argc, char *argv[]);
    virtual bool Initialize(void);
    virtual bool Start(void);
    virtual void Housekeeping(void);
    virtual const char *MainMenuEntry(void);
    virtual cOsdObject *MainMenuAction(void);
    virtual cMenuSetupPage *SetupMenu(void);
    virtual bool SetupParse(const char *Name, const char *Value);
#if APIVERSNUM >= 10331
    virtual const char **SVDRPHelpPages(void);
    virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
#endif
};



class cMenuRipitSetup:public cMenuSetupPage
{
  private:
    const char *bitrateStrings[10], *preset[5];
    const char *encoders[5];
    virtual void Setup(void);
  protected:
      virtual void Store(void);
    virtual eOSState ProcessKey(eKeys Key);
  public:
      cMenuRipitSetup(void);
};

#endif // __WEATHERNG_H
