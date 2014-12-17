/***************************************************************************
 *   Copyright (C) 2010 by Reel Multimedia                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 ***************************************************************************/


#include <vdr/cutter.h>
#include <vdr/debug.h>
#include <vdr/help.h>
#include <vdr/interface.h>
#include <vdr/menu.h>
#include <vdr/plugin.h>
#include <vdr/status.h>
#include <vdr/vdrmysql.h>
#include <vdr/videodir.h>

#include "mainmenu.h"
#include "setupmenu.h"

#include <algorithm>

#if VDRVERSNUM >= 10716

#define CHNUMWIDTH  (numdigits(Channels.MaxNumber()) + 1)
#define LOAD_RANGE 30
#define CHANNELNUMBERTIMEOUT 1500       //ms
#define NEWTIMERLIMIT   120     // seconds until the start time of a new timer created from the Schedule menu,
                            // within which it will go directly into the "Edit timer" menu to allow
                            // further parameter settings

/*
#define IS_RADIO_CHANNEL(c)           (c->Vpid()==0||c->Vpid()==1)
#define IS_ENCRYPTED_RADIO_CHANNEL(c) (IS_RADIO_CHANNEL(c) && (c->Ca() > 0))
#define IS_ENCRYPTED_VIDEO_CHANNEL(c) (!IS_RADIO_CHANNEL(c) && (c->Ca() > 0))
#define IS_HD_CHANNEL(c)              (c->IsSat() && (cDvbTransponderParameters(c->Parameters()).System() == SYS_DVBS2))
*/

// --- Tools -------------------------------------------------------

const char *first_non_space_char(const char *s)
{
    int i;
    if (!s)
        return NULL;
    for (i = 0; s[i]; ++i)
        if (s[i] != ' ')
            return s + i;
    return NULL;
}

// --- cMenuPluginItem -------------------------------------------------------

class cMenuPluginItem:public cOsdItem
{
  private:
    int pluginIndex;
      std::string Link_;
  public:
      cMenuPluginItem(const char *Name, int Index, std::string Link);
    int PluginIndex(void)
    {
        return pluginIndex;
    }
    std::string Link()
    {
        return Link_;
    }
};

cMenuPluginItem::cMenuPluginItem(const char *Name, int Index, std::string Link):cOsdItem(Name,
         osPlugin)
{
    Link_ = Link;
    pluginIndex = Index;
    std::string strBuff;
    strBuff.assign(Link, 0, std::string(Link).length() - std::string(Name).length());
    strBuff.append(tr(Name));
    SetText(strBuff.c_str());
}

// --- cMenuActiveEvent ------------------------------------------------------------
// removed

// --- cMenuEditCaItem -------------------------------------------------------

class cMenuEditCaItem:public cMenuEditIntItem
{
  protected:
    virtual void Set(void);
  public:
    cMenuEditCaItem(const char *Name, int *Value, bool EditingBouquet);
    eOSState ProcessKey(eKeys Key);
};

/* the lite has 3 ci-slots, the avantgarde 2 */
#ifdef RBLITE
#define NUMCIS 3
#else
#define NUMCIS 2
#endif

cMenuEditCaItem::cMenuEditCaItem(const char *Name, int *Value,
                                 bool EditingBouquet):cMenuEditIntItem(Name, Value, 0,
                                                                       EditingBouquet ?
                                                                       NUMCIS :
                                                                       CA_ENCRYPTED_MAX)
{
    Set();
}

void cMenuEditCaItem::Set(void)
{
    char s[64];
    if (*value == CA_FTA)
        strcpy(s, tr("Free To Air"));
#ifdef RBLITE
    else if (*value == 3)
        sprintf(s, "%s (%s)",
                (cDevice::GetDevice(0))->CiHandler()->
                GetCamName(2) ? "Neotion" : tr("no"), tr("internal CAM"));
#endif
    else if (*value == 2)
    {
#if VDRVERSNUM < 10716
        if (cDevice::GetDevice(0)->CiHandler())
            sprintf(s, "%s (%s)",
                    (cDevice::GetDevice(0))->CiHandler()->
                    GetCamName(1) ? (cDevice::GetDevice(0))->CiHandler()->
                    GetCamName(1) : tr("No CI at"), tr("upper slot"));
        else
#endif
            sprintf(s, "%s", tr("upper slot"));
    }
    else if (*value == 1)
    {
#if VDRVERSNUM < 10716
        if (cDevice::GetDevice(0)->CiHandler())
            sprintf(s, "%s (%s)",
                    (cDevice::GetDevice(0))->CiHandler()->
                    GetCamName(0) ? (cDevice::GetDevice(0))->CiHandler()->
                    GetCamName(0) : tr("No CI at"), tr("lower slot"));
        else
#endif
            sprintf(s, "%s", tr("lower slot"));
    }

    if (*value <= NUMCIS)
        SetValue(s);
    else if (*value >= CA_ENCRYPTED_MIN)
        SetValue(tr("encrypted"));
    else
        cMenuEditIntItem::Set();
}

eOSState cMenuEditCaItem::ProcessKey(eKeys Key)
{
    eOSState state = cMenuEditItem::ProcessKey(Key);

    if (state == osUnknown)
    {
        if ((NORMALKEY(Key) == kLeft || NORMALKEY(Key) == kRight)
            && *value >= CA_ENCRYPTED_MIN)
            *value = CA_FTA;
        else
            return cMenuEditIntItem::ProcessKey(Key);
        Set();
        state = osContinue;
    }
    return state;
}

#ifdef DEBUG_TIMES
extern struct timeval menuTimeVal1;
#endif

// --- cMenuSetupLang ---------------------------------------------------------

#if 0
cMenuSetupLang::cMenuSetupLang(void)
{
    //printf("\n%s\n",__PRETTY_FUNCTION__);
    for (numLanguages = 0;
         numLanguages < I18nLanguages()->Size() && data.EPGLanguages[numLanguages] >= 0;
         numLanguages++)
        ;
    originalNumLanguages = numLanguages;
    optLanguages = originalNumLanguages - 1;
    //oldOSDLanguage = Setup.OSDLanguage;
    strn0cpy(oldOSDLanguage, Setup.OSDLanguage, 6);
    oldOSDLanguageIndex = osdLanguageIndex = I18nCurrentLanguage();
    stored = false;
    //SetSection(tr("Setup.OSD$Language"));
    //SetHelp(tr("Button$Scan"));
    DrawMenu();
}

cMenuSetupLang::~cMenuSetupLang(void)
{
    if (!stored)
    {
        strn0cpy(Setup.OSDLanguage, oldOSDLanguage, 6);
        Setup.EPGLanguages[0] = oldOSDLanguageIndex;
        I18nSetLanguage(Setup.EPGLanguages[0]);
        DrawMenu();
    }
}

void cMenuSetupLang::DrawMenu(void)
{
    SetCols(25);
    SetSection(tr("Setup.OSD$Language"));
    int current = Current();

    Clear();

    Add(new
        cMenuEditStraItem(tr("Setup.OSD$Language"), &data.EPGLanguages[0],
                          I18nNumLanguagesWithLocale(), &I18nLanguages()->At(0)));
    Add(new
        cMenuEditIntItem(tr("Setup.OSD$Optional languages"), &optLanguages, 0,
                         I18nLanguages()->Size(), tr("none"), NULL));
    for (int i = 1; i <= optLanguages; i++)
        Add(new
            cMenuEditStraItem(tr("Setup.OSD$ Optional language"), &data.EPGLanguages[i],
                              I18nLanguages()->Size(), &I18nLanguages()->At(0)));
    SetCurrent(Get(current));
    Display();
}

eOSState cMenuSetupLang::ProcessKey(eKeys Key)
{
    bool Modified = (optLanguages + 1 != originalNumLanguages
                     || osdLanguageIndex != data.EPGLanguages[0]);

    int oldnumLanguages = optLanguages + 1;
    int oldPrefLanguage = data.EPGLanguages[0];

    eOSState state = cMenuSetupBase::ProcessKey(Key);

    if (Key != kNone)
    {
        if (optLanguages + 1 != oldnumLanguages)
        {
            Modified = true;
            for (int i = oldnumLanguages; i <= optLanguages; i++)
            {
                Modified = true;
                data.EPGLanguages[i] = 0;
                for (int l = 0; l < I18nNumLanguages; l++)
                {
                    int k;
                    for (k = 0; k < oldnumLanguages; k++)
                    {
                        if (data.EPGLanguages[k] == l)
                            break;
                    }
                    if (k >= oldnumLanguages)
                    {
                        data.EPGLanguages[i] = l;
                        break;
                    }
                }
            }
            data.EPGLanguages[optLanguages + 1] = -1;
        }

        if (oldPrefLanguage != data.EPGLanguages[0])
        {
            Modified = true;
            strn0cpy(data.OSDLanguage, I18nLocale(data.EPGLanguages[0]),
                     sizeof(data.OSDLanguage));
            strn0cpy(Setup.OSDLanguage, I18nLocale(data.EPGLanguages[0]),
                     sizeof(data.OSDLanguage));
            stored = false;
            //int OriginalOSDLanguage = I18nCurrentLanguage();
            I18nSetLanguage(data.EPGLanguages[0]);
            DrawMenu();

            //I18nSetLanguage(OriginalOSDLanguage);
        }

        if (!Modified)
        {
            for (int i = 0; i <= optLanguages; i++)
            {
                if (data.EPGLanguages[i] !=::Setup.EPGLanguages[i])
                {
                    Modified = true;
                    break;
                }
            }
        }

        if (Modified)
        {
            for (int i = 0; i <= I18nNumLanguages; i++)
            {
                data.AudioLanguages[i] = data.EPGLanguages[i];
                if (data.EPGLanguages[i] == -1)
                    break;
            }
            if (Key == kOk)
                cSchedules::ResetVersions();
            else
                DrawMenu();
        }
    }
    if (Key == kOk)
    {
        strn0cpy(oldOSDLanguage, Setup.OSDLanguage, 6);
        stored = true;
    }
    return state;
}

#endif // 0

// --- cMenuSetupLiveBuffer --------------------------------------------------
#if 0
cMenuSetupLiveBuffer::cMenuSetupLiveBuffer(void)
{
    SetSection(tr("Permanent Timeshift"));
    SetCols(25);

    static const char *cmd = "cat /proc/mounts |awk '{ print $2 }'|grep -q '/media/hd'";
    hasHD = !SystemExec(cmd);

    Setup();
}

void cMenuSetupLiveBuffer::Setup(void)
{
    int current = Current();
    Clear();

    if (!hasHD)
    {
        std::string buf = std::string(tr("Permanent Timeshift")) + ":\t" + tr("no");
        Add(new cOsdItem(buf.c_str(), osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem(tr("Info:"), osUnknown, false));
        AddFloatingText(tr("Permanent Timeshift can only be activated with a harddisk."),
                        48);
    }
    else
    {
        Add(new cMenuEditBoolItem(tr("Permanent Timeshift"), &data.LiveBuffer));
        if (data.LiveBuffer)
        {
            Add(new
                cMenuEditIntItem(tr("Setup.LiveBuffer$Buffer (min)"),
                                 &data.LiveBufferSize, 1, 240));
        }
    }
    SetCurrent(Get(current));
    Display();
}

eOSState cMenuSetupLiveBuffer::ProcessKey(eKeys Key)
{
    int oldLiveBuffer = data.LiveBuffer;
    eOSState state = cMenuSetupBase::ProcessKey(Key);

    static const char *command = "reelvdrd --livebufferdir";
    char *strBuff = NULL;
    FILE *process;
    bool dirok;

    if (Key != kNone && (data.LiveBuffer != oldLiveBuffer))
    {
        Setup();
    }


    if (Key == kOk && data.LiveBuffer == 1)
    {
        Store();

        process = popen(command, "r");
        if (process)
        {
            cReadLine readline;
            //DDD("strBuff = readline...");
            strBuff = readline.Read(process);
            if (strBuff != NULL)
                BufferDirectory = strdup(strBuff);
            //DDD("Permanent timeshift on, BufferDirectory: %s", BufferDirectory);
        }

        dirok = !pclose(process);

    }

    // TODO: re-tune to activate/deactivate Live-Buffer?

    return state;
}
#endif


// --- cMainMenu -------------------------------------------------------------

cMainMenu::cMainMenu(eOSState State):cMenuMain("")
{

    /* TB: no help pages in channels-menu */
    if (State != osChannels && State != osTimers)
    {
        // Load Menu Configuration
        //HelpMenus.Load();
    }
    /* TB: no need to parse vdr-menu.xml for channels- or setup-menu */
    if (State != osChannels && State != osSetup && State != osTimers)
    {
        char menuXML[256];
        snprintf(menuXML, 256, "%s/setup/vdr-menu.xml", cPlugin::ConfigDirectory());
        subMenu.LoadXml(menuXML);
    }

    if (!subMenu.isTopMenu())
    {
        int level = 0;
        bool ret;
        do
        {
            ret = subMenu.Up(&level);
        }
        while (ret && level != 0);
    }

    nrDynamicMenuEntries = 0;

    lastDiskSpaceCheck = 0;
    lastFreeMB = 0;
    replaying = false;
    stopReplayItem = NULL;
    cancelEditingItem = NULL;
    stopRecordingItem = NULL;
    recordControlsState = 0;

    // Initial submenus:
    switch (State)
    {
    case osSchedule:
        AddSubMenu(new cMenuSchedule);
        break;
/*  TODO
    case osChannels:   //AddSubMenu(new cMenuChannels); //test
                       if (Setup.UseBouquetList==1)
                           AddSubMenu(new cMenuBouquets(0));
                       else if(Setup.UseBouquetList==2)
                           AddSubMenu(new cMenuBouquets(1));
                       else
                           AddSubMenu(new cMenuBouquets(4));
                       break;
    case osTimers:
        AddSubMenu(new cMenuTimers);
        break;
    case osRecordings:
        AddSubMenu(new cMenuRecordings(NULL, 0, true));
        break;
//    case osSetup:      AddSubMenu(new cMenuSetup); break;
    case osCommands:
        AddSubMenu(new cMenuCommands(tr("Functions"), &Commands));
        break;
    case osOSDSetup:
        AddSubMenu(new cMenuSetupOSD);
        break;
//    case osLanguage:   AddSubMenu(new cMenuSetupLang); break;
        //case osTimezone:   AddSubMenu(new cSetupMenu(NULL, "Timezone")); break;
    case osTimezone:
        AddSubMenu(new cSetupMenuTimezone());
        break;
//    case osTimeshift:
//        AddSubMenu(new cMenuSetupLiveBuffer);
        break;
//    case osActiveEvent:
//        AddSubMenu(new cMenuActiveEvent);
        break;
*/
    default:
        Set();
        break;
    }
}                               // cMainMenu::cMainMenu

void cMainMenu::Set(int current)
{
    Clear();
    //TB SetTitle("VDR");
    SetHasHotkeys();

// *** START PATCH SETUP
    stopReplayItem = NULL;
    cancelEditingItem = NULL;
    stopRecordingItem = NULL;

    // remember initial dynamic MenuEntries added
    int index = 0;
    nrDynamicMenuEntries = Count();
    int icon_number = 0;
    for (cSubMenuNode * node = subMenu.GetMenuTree()->First(); node;
         node = subMenu.GetMenuTree()->Next(node))
    {
        cSubMenuNode::Type type = node->GetType();
        if (node->GetIconNumber())
            icon_number = atoi(node->GetIconNumber());
        else
            icon_number = 0;

        if (type != cSubMenuNode::UNDEFINED)
            if (!HasSubMenu() && current == index)
                SetStatus(node->GetInfo());
        if (type == cSubMenuNode::PLUGIN)
        {
            const char *item = node->GetPluginMainMenuEntry();
            char *link = NULL;
            if (item)
            {
                if (icon_number)
                {
                    asprintf(&link, "%c %s", char (icon_number), item);
                    Add(new cMenuPluginItem(item, node->GetPluginIndex(), link));
                    free(link);
                }
                else
                    Add(new cMenuPluginItem(item, node->GetPluginIndex(), hk(item)));
            }
        }
        else if (type == cSubMenuNode::MENU)
        {
            char *item = NULL;
            if (icon_number)
            {
                asprintf(&item, "%c %s%s", icon_number, tr(node->GetName()),
                         (const char *)subMenu.GetMenuSuffix());
                Add(new cOsdItem(item));
            }
            else
            {
                asprintf(&item, "%s%s", tr(node->GetName()),
                         (const char *)subMenu.GetMenuSuffix());
                Add(new cOsdItem(hk(item)));
            }
            free(item);
        }
        else if (type == cSubMenuNode::COMMAND)
        {
            char *link = NULL;
            if (icon_number)
            {
                asprintf(&link, "%c %s", icon_number, tr(node->GetName()));
                Add(new cOsdItem(link));
                free(link);
            }
            else
                Add(new cOsdItem(hk(tr(node->GetName()))));
        }
        else if (type == cSubMenuNode::SYSTEM)
        {
            const char *item = node->GetName();
            char *link = NULL;
            if (icon_number)
                asprintf(&link, "%c %s", icon_number, tr(item));
            else
                asprintf(&link, "%s", hk(tr(item)));

            if (strcmp(item, "Schedule") == 0)
                Add(new cOsdItem(link, osSchedule));
            else if (strcmp(item, trNOOP("Channel list")) == 0)
                Add(new cOsdItem(link, osChannels));
            else if (strcmp(item, trNOOP("Edit Channels")) == 0)
                Add(new cOsdItem(link, osChannels));
            else if (strcmp(item, "Timers") == 0)
                Add(new cOsdItem(link, osTimers));
            else if (strcmp(item, "Recordings") == 0)
                Add(new cOsdItem(link, osRecordings));
            else if (strcmp(item, "Setup") == 0)
                Add(new cOsdItem(link, osSetup));
            else if (strcmp(item, "Commands") == 0 && Commands.Count() > 0)
                Add(new cOsdItem(link, osCommands));
            else if (strcmp(item, "OSD Settings") == 0)
                Add(new cOsdItem(link, osOSDSetup));
            else if (strcmp(item, "Language Settings") == 0)
                Add(new cOsdItem(link, osLanguage));
            else if (strcmp(item, "Timezone Settings") == 0)
                Add(new cOsdItem(link, osTimezone));
            else if (strcmp(item, "Permanent Timeshift") == 0)
                Add(new cOsdItem(link, osTimeshift));
        }
        index++;
    }
    if (current >= 0 && current < Count())
        SetCurrent(Get(current));

    Update(true);

    Display();
}                               // cMainMenu::Set

#define STOP_RECORDING trVDR(" Stop recording ")
#define MB_PER_MINUTE 25.75     // this is just an estimate!

bool cMainMenu::Update(bool Force)
{
    bool result = false;
    cOsdItem *fMenu = NULL;
    if (Force && subMenu.isTopMenu())
    {
        fMenu = First();
        nrDynamicMenuEntries = 0;
    }

    if (subMenu.isTopMenu())
    {

#ifdef RBLITE
        // Title with disk usage:
        if (Force || time(NULL) - lastDiskSpaceCheck > DISKSPACECHEK)
        {
            int FreeMB;
            int Percent = VideoDiskSpace(&FreeMB);
            if (Force || FreeMB != lastFreeMB)
            {
                int Minutes = int (double (FreeMB) / MB_PER_MINUTE);
                int Hours = Minutes / 60;
                Minutes %= 60;
                char buffer[60];

                snprintf(buffer, sizeof(buffer), "%s - %s %d%% %2d:%02dh %s",
                         tr("Title$Menu"), tr("Disk"), Percent, Hours, Minutes,
                         tr("free"));
                SetTitle(buffer);
                result = true;
            }
            lastDiskSpaceCheck = time(NULL);
        }

#else
#if 0
        if (strcmp(Skins.Current()->Name(), "Reel") == 0)
        {
            //percent string
            char percent_str[13];
            percent_str[0] = '[';
            percent_str[13] = 0;
            percent_str[12] = ']';
            for (int j = 1; j < 12; ++j)
                if (j * 10 < Percent)
                    percent_str[j] = '|';
                else
                    percent_str[j] = ' ';

            snprintf(buffer, sizeof(buffer), "%s - %s %s  %2d:%02dh %s",
                     tr("Title$Main Menu"), tr("Disk"), percent_str, Hours, Minutes,
                     tr("free"));
        }
        else
            snprintf(buffer, sizeof(buffer), "%s - %s %d%% %2d:%02dh %s",
                     tr("Title$Main Menu"), tr("Disk"), Percent, Hours, Minutes,
                     tr("free"));
        //XXX -> skin function!!!
#endif

        SetTitle(tr("Title$Main Menu"));

#endif
    }
    else
    {
        SetTitle(tr(subMenu.GetParentMenuTitel()));
        return (true);
    }

    bool NewReplaying = cControl::Control() != NULL;
    if (Force || NewReplaying != replaying)
    {
        replaying = NewReplaying;
        // Replay control:
        if (replaying && !stopReplayItem)
            Add(stopReplayItem = new cOsdItem(tr(" Stop replaying"), osStopReplay));
        else if (stopReplayItem && !replaying)
        {
            Del(stopReplayItem->Index());
            stopReplayItem = NULL;
        }
        // Color buttons:
//     SetHelp(!replaying ? tr("Button$Record") : NULL, tr("Button$Audio"), replaying ? NULL : tr("Button$Pause"), replaying ? tr("Button$Stop") : cReplayControl::LastReplayed() ? tr("Button$Resume") : NULL);
        if (!replaying)
            SetHelp(tr("Button$Recordings"), tr("EPG"), tr("Favorites"),
                    tr("Button$Multifeed"));
        else
            SetHelp(NULL, NULL, NULL, NULL);
        result = true;
    }

    // Editing control:
    bool CutterActive = cCutter::Active();
    if (CutterActive && !cancelEditingItem)
    {
        Add(cancelEditingItem = new cOsdItem(tr(" Cancel editing"), osCancelEdit));
        result = true;
    }
    else if (cancelEditingItem && !CutterActive)
    {
        Del(cancelEditingItem->Index());
        cancelEditingItem = NULL;
        result = true;
    }

    // Record control:
    if (1 || cRecordControls::StateChanged(recordControlsState))
    {                           // balaji: this condition does not redraw recordings-items on mainmenu when a submenu is entered into and returned back to main-menu via kExit
        while (stopRecordingItem)
        {
            cOsdItem *it = Next(stopRecordingItem);
            Del(stopRecordingItem->Index());
            stopRecordingItem = it;
        }
        const char *s = NULL;
        int count = 4;          // show only 4 current-recordings
        // GetInstantId(s,true) true == in the reverse, Last started is give out first
#ifdef USEMYSQL
        if (Setup.ReelboxModeTemp != eModeServer)
        {
#endif
            //printf("Last recording %s\n", cRecordControls::GetInstantId(NULL,false));
            while (count && (s = cRecordControls::GetInstantId(s, true)) != NULL)
            {
                char *buffer = NULL;
                asprintf(&buffer, "%s%s", STOP_RECORDING, s);
                cOsdItem *item = new cOsdItem(osStopRecord);
                --count;
                item->SetText(buffer, false);
                Add(item);
                if (!stopRecordingItem)
                    stopRecordingItem = item;
            }
#ifdef USEMYSQL
        }
        if ((Setup.ReelboxModeTemp == eModeClient)
            || (Setup.ReelboxModeTemp == eModeServer))
        {
            std::vector < cTimer * >InstantRecordings;
            Timers.GetInstantRecordings(&InstantRecordings);
            if (InstantRecordings.size())
            {
                std::vector < cTimer * >::iterator TimerIterator =
                    InstantRecordings.begin();
                while (count && TimerIterator != InstantRecordings.end())
                {
                    if ((*TimerIterator)->Recording())
                    {
                        char *buffer = NULL;
                        asprintf(&buffer, "%s%s", STOP_RECORDING,
                                 (*TimerIterator)->Channel()->Name());
                        cOsdItem *item = new cOsdItem(osStopRecord);
                        --count;
                        item->SetText(buffer, false);
                        Add(item);
                        if (!stopRecordingItem)
                            stopRecordingItem = item;
                    }
                    ++TimerIterator;
                }
            }
        }
#endif
        result = true;
    }

    // adjust nrDynamicMenuEntries
    if (fMenu != NULL)
        nrDynamicMenuEntries = fMenu->Index();

    return result;
}                               // cMainMenu::Update

eOSState cMainMenu::ProcessKey(eKeys Key)
{
    bool HadSubMenu = HasSubMenu();
    eOSState state = cOsdMenu::ProcessKey(Key);
    HadSubMenu |= HasSubMenu();
    //DDD("************* state=%d************", state);
    //if (state != osEnd && state != osBack && state != osUnknown && state != osContinue)
    switch (state)
    {
    case osSchedule:
    case osChannels:
    case osTimers:
    case osRecordings:
    case osSetup:
    case osCommands:
    case osPause:
        //case osRecord:
        //case osReplay:
        //case osStopRecord:
        //case osStopReplay:
    case osCancelEdit:
    case osSwitchDvb:
    case osEditChannels:
    case osOSDSetup:
    case osLanguage:
    case osTimezone:
    case osTimeshift:
    case osActiveEvent:
    case osLiveBuffer:
    case osFavourites:
        cOsdMenu * Menu;
        Menu = NULL;
        CREATE_MENU_MAIN(state, Menu);
        if (Menu)
        {
            DDD("got a menu for %d", state);
            return AddSubMenu(Menu);
        }
        break;
    default:
        break;
    }
    //DDD("state: %d no menu", state);

    switch (state)
    {
    case osSchedule:
        return AddSubMenu(new cMenuSchedule);
/* TODO
    case osChannels:   if (Setup.UseBouquetList == 1)
                           return AddSubMenu(new cMenuBouquets(0));
                       else if (Setup.UseBouquetList == 2)
                           return AddSubMenu(new cMenuBouquets(1));
                       else
                           return AddSubMenu(new cMenuChannels);
                       //return AddSubMenu(new cMenuBouquets(0));
*/
        //TODO
        //case osEditChannels:   return AddSubMenu(new cMenuBouquets(0, cMenuBouquets::mode_edit)); //TB cMenuChannels);
    case osTimers:
        return AddSubMenu(new cMenuTimers);
    case osRecordings:
        return AddSubMenu(new cMenuRecordings(NULL, 0, true));
//    case osSetup:      return AddSubMenu(new cMenuSetup);
    case osCommands:
        return AddSubMenu(new cMenuCommands(tr("Functions"), &Commands));
    case osOSDSetup:
        return AddSubMenu(new cMenuSetupOSD);
//    case osLanguage:   return AddSubMenu(new cMenuSetupLang);
    case osTimezone:           //TODO
        {
            cPlugin *SetupTimePlugin = cPluginManager::GetPlugin("setup");
            if (SetupTimePlugin)
            {
                struct
                {
                    cOsdMenu *menu;
                } data;

                SetupTimePlugin->Service("Setup Timezone", &data);

                return AddSubMenu(data.menu);
            }
        }
//    case osTimeshift:
//        return AddSubMenu(new cMenuSetupLiveBuffer);
    case osStopRecord:
        if (Interface->Confirm(tr("Stop recording?")))
        {
            cOsdItem *item = Get(Current());
            if (item)
            {
#ifdef USEMYSQL
                if ((Setup.ReelboxModeTemp == eModeClient)
                    || (Setup.ReelboxModeTemp == eModeServer))
                {               // Client & Server
                    bool SetServer = false;
                    bool res = false;
                    // Remove Timer from Database
                    cTimersMysql *TimersMysql =
                        new cTimersMysql(MYSQLREELUSER, MYSQLREELPWD, "vdr");
                    if ((Setup.ReelboxModeTemp == eModeClient) && Setup.NetServerIP && strlen(Setup.NetServerIP))       // Client
                        SetServer = TimersMysql->SetServer(Setup.NetServerIP);
                    else
                        SetServer = TimersMysql->SetServer("localhost");
                    if (SetServer)
                    {
                        std::vector < cTimer * >InstantRecordings;
                        Timers.GetInstantRecordings(&InstantRecordings);
                        bool found = false;
                        unsigned int i = 0;
                        while (!found && (i < InstantRecordings.size()))
                        {
                            if (!strcmp
                                (InstantRecordings.at(i)->Channel()->Name(),
                                 item->Text() + strlen(STOP_RECORDING)))
                            {
                                res =
                                    TimersMysql->DeleteTimer(InstantRecordings.at(i)->
                                                             GetID());
                                found = true;
                            }
                            ++i;
                        }
                    }
                    delete TimersMysql;
                    return osEnd;
                }
                else
#endif
                    cRecordControls::Stop(item->Text() + strlen(STOP_RECORDING));
                return osEnd;
            }
        }
        break;
    case osCancelEdit:
        if (Interface->Confirm(tr("Cancel editing?")))
        {
            cCutter::Stop();
            return osEnd;
        }
        break;
    case osPlugin:
        {
            cMenuPluginItem *item = (cMenuPluginItem *) Get(Current());
            if (item)
            {
                cPlugin *p = cPluginManager::GetPlugin(item->PluginIndex());
                if (p)
                {
                    if (strcmp(p->Name(), "setup") == 0)
                    {
                        /* eg. '  3  link name'
                         * looking for the second word */
                        const char *tmp =
                            strchr(first_non_space_char(item->Link().c_str()), ' ');
                        tmp = first_non_space_char(tmp);
                        //printf("(%s:%i)\033[0;91mlink='%s'\033[0m '%s'\n",  __FILE__,__LINE__, tmp,item->Link().c_str());
                        p->Service("link", (void *)tmp);
                    }
#ifdef USE_PINPLUGIN
                    if (!cStatus::MsgPluginProtected(p))
                    {           // PIN PATCH
#endif
                        cOsdObject *menu = p->MainMenuAction();
                        if (menu)
                        {
                            if (menu->IsMenu())
                                return AddSubMenu((cOsdMenu *) menu);
                            else
                            {
                                pluginOsdObject = menu;
                                return osPlugin;
                            }
                        }
#ifdef USE_PINPLUGIN
                    }
#endif
                }
            }
            state = osEnd;
        }
        break;
    case osBack:
        {
            int newCurrent = 0;
            if (subMenu.Up(&newCurrent))
            {
                Set(newCurrent);
                return osContinue;
            }
            else
                return osEnd;
        }
        break;
    default:
        switch (Key)
        {

            //case kInfo: if (!HadSubMenu) return DisplayHelp((Current() - nrDynamicMenuEntries));
        case kRecord:
        case kRed:
            if (!HadSubMenu)
            {
                if (!replaying)
                {
                    cRemote::Put(kRecordings, true);
                    state = osUnknown; // We have to set osUnknown, otherwise extrecmenu will show
                                       // an empty menu. Strange.
                }
                else
                    state = osContinue;
            }
            break;
        case kGreen:
            if (!HadSubMenu)
            {
                if (!replaying)
                    cRemote::Put(kSchedule, true);
                state = osContinue;
            }
            break;
        case kYellow:
            if (!HadSubMenu)
            {
                if (!replaying)
                    cRemote::Put(kFavourites, true);
                state = osContinue;
            }
            break;
        case kBlue:
            if (!HadSubMenu)
            {
                if (!replaying)
                    cRemote::CallPlugin("arghdirector");
                state = osContinue;
//              state = replaying ? osStopReplay : cReplayControl::LastReplayed() ? osReplay : osContinue;
            }
            break;
        case kOk:
            if (state == osUnknown)
            {
                int index = Current() - nrDynamicMenuEntries;
                cSubMenuNode *node = subMenu.GetNode(index);

                if (node != NULL)
                {
                    if (node->GetType() == cSubMenuNode::MENU)
                    {
#ifdef USE_PINPLUGIN
                        subMenu.Down(node, Current());
#else
                        subMenu.Down(index);
#endif
                    }
                    else if (node->GetType() == cSubMenuNode::COMMAND)
                    {
                        char *buffer = NULL;
                        bool confirmed = true;
                        if (node->CommandConfirm())
                        {
                            asprintf(&buffer, "%s?", node->GetName());
                            confirmed = Interface->Confirm(buffer);
                            free(buffer);
                        }
                        if (confirmed)
                        {
                            asprintf(&buffer, "%s...", node->GetName());
                            Skins.Message(mtStatus, buffer);
                            free(buffer);
                            const char *Result =
                                subMenu.ExecuteCommand(node->GetCommand());
                            Skins.Message(mtStatus, NULL);
                            if (Result)
                                return AddSubMenu(new
                                                  cMenuText(node->GetName(), Result,
                                                            fontFix));

                            return osEnd;
                        }
                    }
                }

                Set();
                return state;
            }
            break;
	case kInfo:
	case kStop:
	case kPlay:
	case kChanUp:
	case kChanDn:
            return osContinue;
        default:
            break;
        }
    }
#if 0                           //TB: why this??? this slows down rendering of submenus extremely
    if (!HasSubMenu() && Update(HadSubMenu))
        Display();
#endif
    if (Key != kNone)
    {

        int index = Current() - nrDynamicMenuEntries;
        cSubMenuNode *node = subMenu.GetNode(index);

        if (!HasSubMenu() && node)
        {
            if (node->GetInfo())
                SetStatus(node->GetInfo());
            else
                SetStatus(NULL);
        }
    }
    return state;
}                               // cMainMenu::ProcessKey

#endif /* VDRVERSNUM */
