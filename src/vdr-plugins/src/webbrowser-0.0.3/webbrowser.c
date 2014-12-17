/*
 * webbrowser.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include "hdutil.h"
#include "webbrowser.h"
#include "setup.h"
#include "webbrowserosd.h"
#include "menuPercItem.h"

static const char *MAINMENUENTRY = trNOOP("Web Browser");
bool HasMode(int mode);
int NextMode(int mode);

cPluginWebbrowser::cPluginWebbrowser(void)
{
    // Initialize any member variables here.
    // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
    // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginWebbrowser::~cPluginWebbrowser()
{
    // Clean up after yourself!
}

const char *cPluginWebbrowser::CommandLineHelp(void)
{
    // Return a string that describes all known command line options.
    return NULL;
}

bool cPluginWebbrowser::ProcessArgs(int argc, char *argv[])
{
    // Implement command line argument processing here if applicable.
    return true;
}

bool cPluginWebbrowser::Initialize(void)
{
    // Initialize any background activities the plugin shall perform.

    // start with a valid mode
    if (!HasMode(WebbrowserSetup.mode))
        WebbrowserSetup.mode = NextMode(WebbrowserSetup.mode);

    return true;
}

bool cPluginWebbrowser::Start(void)
{
    // Start any background activities the plugin shall perform.
    return true;
}

void cPluginWebbrowser::Stop(void)
{
    // Stop any background activities the plugin shall perform.
}

void cPluginWebbrowser::Housekeeping(void)
{
    // Perform any cleanup or other regular tasks.
}

void cPluginWebbrowser::MainThreadHook(void)
{
    // Perform actions in the context of the main program thread.
    // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginWebbrowser::Active(void)
{
    // Return a message string if shutdown should be postponed
    return NULL;
}

cOsdObject *cPluginWebbrowser::MainMenuAction(void)
{
    // Perform the action when selected from the main VDR menu.

    // check if current mode is valid, if not find a valid mode
    if (!HasMode(WebbrowserSetup.mode))
        WebbrowserSetup.mode = NextMode(WebbrowserSetup.mode);

    return (MODE_INVALID != WebbrowserSetup.mode) ? new cWebbrowserOsd : NULL;
}

cMenuSetupPage *cPluginWebbrowser::SetupMenu(void)
{
    // Return a setup menu in case the plugin supports one.
    return new cMenuWebbrowserSetup;
}

const char *cPluginWebbrowser::MenuSetupPluginEntry(void)
{
    return tr(MAINMENUENTRY);
}

const char *cPluginWebbrowser::MainMenuEntry(void)
{
    switch (WebbrowserSetup.mode)
    {
    case MODE_LINKS:
        return tr("Smartbrowser");
    case MODE_FIREFOX:
        return tr("Webbrowser (Firefox)");
    case MODE_X11:
        return tr("KDE-Desktop");
    case MODE_CHROME:
         return tr("Webbrowser (Chrome)");
    case MODE_INVALID:
        return tr("No browser available");
    default:
        return tr(MAINMENUENTRY);
    }
    return tr(MAINMENUENTRY);
}

bool HasMode(int mode)
{
    struct stat stat_;
    switch (mode)
    {
    case MODE_LINKS:
        return stat("/usr/sbin/links2", &stat_) == 0;
    case MODE_FIREFOX:
        return stat("/usr/bin/firefox", &stat_) == 0;
    case MODE_X11:
        return stat("/usr/bin/kdesktop", &stat_) == 0
            || stat("/usr/bin/kde4", &stat_) == 0;
    case MODE_CHROME:
            return stat("/usr/lib/chrome-ice/chrome", &stat_) == 0;
    }                           // switch
    return false;
}                               // HasMode

int NextMode(int mode)
{
    int m = mode;
    while (!HasMode(++mode))
    {
        if (mode >= MODE_LAST)
            mode = MODE_INVALID;
        if (m == mode)
            return MODE_INVALID;
    }                           // while
    return mode;
}                               // NextMode

bool cPluginWebbrowser::SetupParse(const char *Name, const char *Value)
{
    // Parse your own setup parameters and store their values.
    if (!strcasecmp(Name, "startURL"))
    {
        strcpy(WebbrowserSetup.startURL, Value);
        return true;
    }
    else if (!strcasecmp(Name, "optimize"))
    {
        WebbrowserSetup.optimize = atoi(Value);
        return true;
    }
    else if (!strcasecmp(Name, "menuFontSize"))
    {
        WebbrowserSetup.menuFontSize = atoi(Value);
        return true;
    }
    else if (!strcasecmp(Name, "fontSize"))
    {
        WebbrowserSetup.fontSize = atoi(Value);
        return true;
    }
    else if (!strcasecmp(Name, "transparency"))
    {
        WebbrowserSetup.transparency = atoi(Value);
        return true;
    }
    else if (!strcasecmp(Name, "margin_x"))
    {
        WebbrowserSetup.margin_x = atoi(Value);
        return true;
    }
    else if (!strcasecmp(Name, "margin_y"))
    {
        WebbrowserSetup.margin_y = atoi(Value);
        return true;
    }
    else if (!strcasecmp(Name, "scalepics"))
    {
        WebbrowserSetup.scalepics = atoi(Value);
        return true;
    }
    else if (!strcasecmp(Name, "X11alpha"))
    {
        WebbrowserSetup.X11alpha = atoi(Value);
        return true;
    }
    else if (!strcasecmp(Name, "mode"))
    {
        WebbrowserSetup.mode = atoi(Value);
        if (!HasMode(WebbrowserSetup.mode))
            WebbrowserSetup.mode = NextMode(WebbrowserSetup.mode);
        return true;
    }
    else if (!strcasecmp(Name, "center_full"))
    {
        WebbrowserSetup.center_full = atoi(Value);
        return true;
    }
    else if (!strcasecmp(Name, "FFstartURL"))
    {
        strcpy(WebbrowserSetup.FFstartURL, Value);
        return true;
    }

    return false;
}

bool cPluginWebbrowser::Service(const char *Id, void *Data)
{
    // Handle custom service requests from other plugins
    return false;
}

const char **cPluginWebbrowser::SVDRPHelpPages(void)
{
    // Return help text for SVDRP commands this plugin implements
    return NULL;
}

cString cPluginWebbrowser::SVDRPCommand(const char *Command, const char *Option,
                                        int &ReplyCode)
{
    // Process SVDRP commands this plugin implements
    return NULL;
}

cMenuWebbrowserSetup::cMenuWebbrowserSetup(void)
{
    SetCols(20);
    Setup();
}

class cMenuEditModeItem:public cMenuEditStraItem
{
  public:
    cMenuEditModeItem(const char *Name, int *Value, int NumStrings,
                      const char *const *Strings):cMenuEditStraItem(Name, Value,
                                                                    NumStrings, Strings)
    {
    }
  protected:
      virtual void Set(void)
    {
        if (!HasMode(*value))
            *value = NextMode(*value);
        cMenuEditStraItem::Set();
    }                           // Set
};                              // cMenuEditModeItem

void cMenuWebbrowserSetup::Setup(void)
{
    static const char allowed[] = { "abcdefghijklmnopqrstuvwxyz0123456789-_/:." };

    int current = Current();

    Clear();
    if (!HasMode(MODE_LINKS) && !HasMode(MODE_FIREFOX) && !HasMode(MODE_X11) && !HasMode(MODE_CHROME))
    {
        Add(new cOsdItem(tr("No browser available")));
        WebbrowserSetup.mode = MODE_INVALID;
        SetCurrent(Get(current));
        Display();
        return;
    }                           // if
    static const char *optimizeStrings[3];
    optimizeStrings[0] = tr("CRT");
    optimizeStrings[1] = tr("LCD (RGB)");
    optimizeStrings[2] = tr("LCD (BGR)");

    static const char *modeStrings[MODE_LAST];
    modeStrings[MODE_LINKS] = tr("Simple Browser");
    modeStrings[MODE_X11] = tr("KDE-Desktop");
    modeStrings[MODE_FIREFOX] = tr("Webbrowser: Firefox");
    modeStrings[MODE_SNES] = tr("SNES-emu-only session");
    modeStrings[MODE_CHROME] = tr("Webbrowser: Chrome");

    static const char *centerFullStrings[2];
    centerFullStrings[0] = tr("Center");
    centerFullStrings[1] = tr("Full");

    Add(new cMenuEditModeItem(tr("Mode"), &WebbrowserSetup.mode, MODE_LAST, modeStrings));
    if (WebbrowserSetup.mode == MODE_LINKS)
    {
        Add(new
            cMenuEditStrItem(tr("start URL"), WebbrowserSetup.startURL, 255, allowed));
        Add(new
            cMenuEditIntItem(tr("font size in menu"), &WebbrowserSetup.menuFontSize, 8,
                             20));
        Add(new cMenuEditIntItem(tr("font size"), &WebbrowserSetup.fontSize, 8, 20));
        Add(new
            cMenuEditIntItem(tr("picture scaling (%)"), &WebbrowserSetup.scalepics, 1,
                             100));
        Add(new
            cMenuEditIntItem(tr("transparency"), &WebbrowserSetup.transparency, 50, 255));
        Add(new
            cMenuEditIntItem(tr("margin horizontal"), &WebbrowserSetup.margin_x, 1, 200));
        Add(new
            cMenuEditIntItem(tr("margin vertical"), &WebbrowserSetup.margin_y, 1, 200));
        Add(new
            cMenuEditStraItem(tr("optimize for"), &WebbrowserSetup.optimize, 3,
                              optimizeStrings));
    }
    else
    {
        if (WebbrowserSetup.mode == MODE_FIREFOX || WebbrowserSetup.mode == MODE_CHROME)
            Add(new
                cMenuEditStrItem(tr("start URL"), WebbrowserSetup.FFstartURL, 255,
                                 allowed));

        if (!HasDevice("80860709"))
        {
            Add(new cMenuEditIntPercItem(tr("transparency"), &WebbrowserSetup.X11alpha, 25, 255,
				    NULL, NULL, 5));
	    Add(new cMenuEditStraItem(tr("scaling mode"), &WebbrowserSetup.center_full, 2,
				centerFullStrings));
        }
    }

    SetCurrent(Get(current));
    Display();
}

void cMenuWebbrowserSetup::Store(void)
{
    //printf("storing: %s %i %i %i\n", WebbrowserSetup.startURL, WebbrowserSetup.fontSize,  WebbrowserSetup.scalepics, WebbrowserSetup.menuFontSize);
    SetupStore("mode", WebbrowserSetup.mode);
    SetupStore("startURL", WebbrowserSetup.startURL);
    SetupStore("FFstartURL", WebbrowserSetup.FFstartURL);
    SetupStore("fontSize", WebbrowserSetup.fontSize);
    SetupStore("scalepics", WebbrowserSetup.scalepics);
    SetupStore("menuFontSize", WebbrowserSetup.menuFontSize);
    SetupStore("transparency", WebbrowserSetup.transparency);
    SetupStore("margin_x", WebbrowserSetup.margin_x);
    SetupStore("margin_y", WebbrowserSetup.margin_y);
    SetupStore("optimize", WebbrowserSetup.optimize);
    SetupStore("X11alpha", WebbrowserSetup.X11alpha);
    SetupStore("center_full", WebbrowserSetup.center_full);
}

eOSState cMenuWebbrowserSetup::ProcessKey(eKeys Key)
{
    int oldMode = WebbrowserSetup.mode;
    eOSState state = cMenuSetupPage::ProcessKey(Key);

#if 0 // no idea what these checks are !
    if (((Current() != 0 && state != osContinue)
         || (Current() == 0 || state == osUnknown)) && Key != kNone
        && !((Key & k_Repeat) == kNone))
    {
        Setup();
    }
#else
    /* Mode has changed, so new options must be shown. Redraw menu. */
    if (WebbrowserSetup.mode != oldMode)
        Setup();
#endif

    return state;
}

VDRPLUGINCREATOR(cPluginWebbrowser);    // Don't touch this!
