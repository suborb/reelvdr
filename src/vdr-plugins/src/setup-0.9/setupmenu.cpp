/*************************************************************************
 * DESCRIPTION:
 *             Creates VDR Menus
 *
 * $Id: setupmenu.cpp,v 1.16 2005/10/03 14:05:20 ralf Exp $
 *
 * Contact:    ranga@teddycats.de
 *
 * Copyright (C) 2004 by Ralf Dotzert
 ****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <unistd.h>

#include <vdr/config.h>
#include <vdr/debug.h>
#include <vdr/menu.h>
#include <vdr/menuitems.h>
#include <vdr/status.h>
#include <vdr/videodir.h>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#include "config.h"
#include "energyoptionsmenu.h"
#include "menueditrefreshitems.h"
#include "menusetup_vdr.h"
#include "menusysteminfo.h"
#include "netinfo.h"
#include "plugins.h"
#include "setup.h"
#include "setupmenu.h"
#include "setupsetup.h"
#include "setuprecordingdevicemenu.h"
#include "setupwlanmenu.h"
#include "setupnetworkfilesystemmenu.h"
#include "setupnetclientmenu.h"
#include "setupmultiroom.h"
#include "resetsettingsmenu.h"
#ifdef USEMYSQL
#  include "setupmysqlmenu.h"
#endif
#include "util.h"
#include "vdrsetupclasses.h"
#if VDRVERSNUM >= 10716
#  include "mainmenu.h"
#endif

#include <sys/ioctl.h>

using std::vector;
using std::string;

#define WITH_PLUGINMENU 0

#ifndef BUILD
#define BUILD unknown;
#endif

#define INFOTIMEOUT 3
#define WLAN_DRIVER "ath_pci"

// directories in /usr/share/zoneinfo
// Africa  America  Antarctica  Arctic  Asia  Atlantic  Australia  Brazil  Canada  Chile  Etc  Europe  Indian  Mexico  Mideast  Pacific  SystemV  US  posix  right
static string AllowedContinents = "Africa  America  Antarctica  Arctic  Asia  Atlantic  Australia  Brazil  Canada  Chile Europe  Indian  Mexico  Mideast  Pacific US";

// Replace all occurances of oldChar in str with newChar
string ReplaceChar(string str_, char oldChar, char newChar)
{
    string result = str_;
    for (unsigned int i = 0; i < result.length(); ++i)
        if (result[i] == oldChar) result[i] = newChar;

    return result;
}


/* returns false even if one of the devices is not a satellite tuner*/
bool AreAllTunersSat()
{
    cDevice *device;
    for (int tuner = 0; tuner < MAXDEVICES; tuner++) {
        device = cDevice::GetDevice(tuner);
        if (device) {
            if (device->ProvidesSource(cSource::stCable)
                    || device->ProvidesSource(cSource::stTerr))
                return false;
        } // if
    } // for

    return true;
}

// --- class cExecuteMenu --------------------------------------------------------

cExecuteMenu::cExecuteMenu(const char *Command):
cOsdMenu("")
{
    command_ = Command;
}

void
cExecuteMenu::Execute()
{
    subMenu_.ExecuteCommand(command_);
}


eOSState cSetupVdrMenu::ProcessKey(eKeys Key)
{
    cSubMenuNode *node = NULL;
    eOSState state = cOsdMenu::ProcessKey(Key);

    if (HasSubMenu())
        return state;

    switch (Key)
    {
    case kRed:
        if (menuState_ == UNDEFINED)
        {
            menuState_ = CREATE;
            createEditNodeIndex_ = Current();
            Set();
        }
        break;
    case kGreen:
        if (menuState_ == MOVE)
        {
            if ((node = vdrSubMenu_.GetAbsNode(Current())) != NULL
                && node->GetType() == cSubMenuNode::MENU)
            {
                vdrSubMenu_.MoveMenu(startIndex_, Current(), cSubMenu::INTO);
                menuState_ = UNDEFINED;
                createEditNodeIndex_ = Current();
                SetStatus(NULL);
                Set();
            }
        }
        else if (menuState_ == UNDEFINED)
        {
            if ((node = vdrSubMenu_.GetAbsNode(Current())) != NULL
                && node->GetType() == cSubMenuNode::MENU)
            {
                menuState_ = EDIT;
                createEditNodeIndex_ = Current();
                Set();
            }
        }
        setHelp();
        break;
    case kYellow:
        if (menuState_ == MOVE)
        {
            vdrSubMenu_.MoveMenu(startIndex_, Current(), cSubMenu::BEFORE);
            menuState_ = UNDEFINED;
            createEditNodeIndex_ = Current();
            SetStatus(NULL);
            Set();
        }
        else if (menuState_ == UNDEFINED)
        {
            if (Interface->Confirm(tr("Delete Menu?")))
            {
                createEditNodeIndex_ = Current();
                vdrSubMenu_.DeleteMenu(createEditNodeIndex_);
                menuState_ = UNDEFINED;
                Set();
            }
        }
        break;
    case kBlue:
        if (menuState_ == MOVE)
        {
            vdrSubMenu_.MoveMenu(startIndex_, Current(), cSubMenu::BEHIND);
            createEditNodeIndex_ = Current();
            menuState_ = UNDEFINED;
            SetStatus(NULL);
        }
        else if (menuState_ == UNDEFINED)
        {
            startIndex_ = Current();
            createEditNodeIndex_ = startIndex_;
            menuState_ = MOVE;
            SetStatus(tr("Up/Dn for new location - OK to move"));
        }
        Set();
        setHelp();
        break;

    case kOk:
        switch (menuState_)
        {
        case UNDEFINED:
            // Save Menus to file and exit submenu
            state = osBack;
            vdrSubMenu_.SetMenuSuffix(SetupSetup._menuSuffix);
            vdrSubMenu_.SaveXml();
            break;
        case CREATE:
            vdrSubMenu_.CreateMenu(createEditNodeIndex_, editTitle_);
            menuState_ = UNDEFINED;
            Set();
            break;
        case EDIT:
            vdrSubMenu_.GetAbsNode(createEditNodeIndex_)->SetName(editTitle_);
            menuState_ = UNDEFINED;
            Set();
            break;
        default:
            break;
        }
        break;
    case kBack:
        if (menuState_ == CREATE || menuState_ == EDIT)
        {
            menuState_ = UNDEFINED;
            Set();
            state = osContinue;
        }
    case kDown:
    case kUp:
    case kRight:
    case kLeft:
        if (menuState_ == MOVE)
            setHelp();
        break;
    default:
        break;
    }

    //DDD("return state: %d", state);
    return state;
}

// --------------- Private Methods ---------------------

void
cSetupVdrMenu::setHelp()
{
    cSubMenuNode *node = NULL;

    if (menuState_ == MOVE)
    {
        if ((node = vdrSubMenu_.GetAbsNode(Current())) != NULL
            && node->GetType() == cSubMenuNode::MENU)
            SetHelp(NULL, tr("Into"), tr("Before"), tr("After"));
        else
            SetHelp(NULL, NULL, tr("Before"), tr("After"));
    }
    else
        SetHelp(tr("Create"), tr("Edit"), tr("Delete"), tr("Move"));
}

char *
cSetupVdrMenu::createMenuName(cSubMenuNode * node)
{
    char *prefix = NULL;
    char *tmp = NULL;
    int level = node->GetLevel();

    // Set Prefix
    prefix = (char *)malloc(1);
    prefix[0] = '\0';
    for (int i = 0; i < level; i++)
        asprintf(&prefix, "|   %s", prefix);


    cSubMenuNode::Type type = node->GetType();

    if (type == cSubMenuNode::MENU)
        asprintf(&tmp, "%s+%s", prefix, node->GetName());
    else if (type == cSubMenuNode::SYSTEM)
        asprintf(&tmp, "%s %s", prefix, tr(node->GetName()));
    else if (type == cSubMenuNode::PLUGIN)
    {
        // plugin is already translated
        asprintf(&tmp, "%s %s", prefix, node->GetName());
    }
    else
        asprintf(&tmp, "%s %s", prefix, node->GetName());

    //printf (" free(prefix) %s %p \n", prefix, &prefix);
    free(prefix);

    return (tmp);
}

//#########################################################################################
//  cSetupMenuTimezone
//  taken from install wizard  Plugin
//#########################################################################################

cSetupMenuTimezone::cSetupMenuTimezone():cOsdMenu(tr("Timezone & Clock"), 28)
{

    currentCon = 0;
    currentCity = 0;
    nr_con_values = 0;
    nr_city_values = 1;
    DIR * zoneDir = NULL;
    struct dirent * entry = NULL;
    struct stat buf;
    std::string path = "/usr/share/zoneinfo";
    for (int k = 0; k < 64; k++)
    {
        con_values[k] = (char *)malloc(32);
        con_values[k][0] = '\0';
    }
    for (int k = 0; k < 256; k++)
    {
        city_values[k] = (char *)malloc(32);
        city_values[k][0] = '\0';
    }

    //opendir ISDIR
    if ((zoneDir = opendir(path.c_str())) != NULL)
    {
        while ((entry = readdir(zoneDir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") != 0
                && strcmp(entry->d_name, "..") != 0
                && AllowedContinents.find(entry->d_name) != string::npos //allow only certain entries
                )
            {
                std::string tmp = path + "/" + entry->d_name;
                stat(tmp.c_str(), &buf);
                if (S_ISDIR(buf.st_mode))
                {
                    con_values[nr_con_values] =
                        strcpy(con_values[nr_con_values], entry->d_name);
                    nr_con_values++;
                }
            }
        }
        closedir(zoneDir);
    }
    else
        esyslog("Can not read directory \"%s\" because of the error \"%s\"\n", path.c_str(), strerror(errno));

    char zoneInfo[64] = {0};
    const char* zi = cSysConfig_vdr::GetInstance().GetVariable("ZONEINFO");
    if (zi)
        strncpy(zoneInfo, zi, 63);

    char * buf2 = (char *)malloc(256);
    char * cp_buf2 = buf2;
    char * continent = strtok_r(zoneInfo, "/", &buf2);     /* go to first '/' */
    char * city = strtok_r(NULL, "/", &buf2);      /* get the rest */
    //printf (" free(cp_buf2) %s %p \n", cp_buf2, &cp_buf2);
    free(cp_buf2);

    if (continent != NULL && city != NULL)
    {
        //printf("city: %s, continent %s\n", city, continent);
        for (int i = 0; i < nr_con_values; i++)
            if (!strcmp(con_values[i], continent))
                currentCon = i;

        nr_city_values = 0;
        std::string path_prefix = "/usr/share/zoneinfo";
        DIR *conDir = NULL;
        struct dirent *entry = NULL;
        struct stat buf;
        std::string path = path_prefix + "/" + con_values[currentCon];
        if ((conDir = opendir(path.c_str())) != NULL)
        {
            while ((entry = readdir(conDir)) != NULL)
            {
                std::string tmp = path + "/" + entry->d_name;
                stat(tmp.c_str(), &buf);
                if (S_ISREG(buf.st_mode))
                {
                    city_values[nr_city_values] =
                        strcpy(city_values[nr_city_values], ReplaceChar(entry->d_name,'_',' ').c_str() );

                    nr_city_values++;
                }
                //printf (" zoneinfo free(tmp) %s %s \n", tmp, &tmp);
            }
            closedir(conDir);
        }
        else
            esyslog("Can not read directory:%s  errno=%d\n", path.c_str(), errno);

        for (int i = 0; i < nr_city_values; i++)
            if (!strcmp(city_values[i], city))
                currentCity = i;
    }
    else
    {
        for (int i = 0; i < nr_con_values; i++)
            if (!strcmp(con_values[i], "Europe"))       /* Europe is the default continent */
                currentCon = i;
    }

    for (numLanguages = 0;
#if VDRVERSNUM < 10727
         numLanguages < I18nNumLanguages
#else
         numLanguages < I18nLanguages()->Size()
#endif
         && Setup.EPGLanguages[numLanguages] >= 0; numLanguages++);

    originalNumLanguages = numLanguages;
    optLanguages = numLanguages - 1;

    setSystemTime =::Setup.SetSystemTime;
    timeTransponder =::Setup.TimeTransponder;
    timeSource =::Setup.TimeSource;

    systemTimeValues[0] = tr("off");
    systemTimeValues[1] = tr("Broadcast");
    systemTimeValues[2] = tr("Internet");

    Set();
}

void cSetupMenuTimezone::Set()
{
    nr_city_values = 0;
    std::string path_prefix = "/usr/share/zoneinfo";
    DIR *conDir = NULL;
    struct dirent *entry = NULL;
    struct stat buf;
    std::string path = path_prefix + "/" + con_values[currentCon];
    if ((conDir = opendir(path.c_str())) != NULL)
    {
        while ((entry = readdir(conDir)) != NULL)
        {
            std::string tmp = path + "/" + entry->d_name;
            stat(tmp.c_str(), &buf);
            if (S_ISREG(buf.st_mode))
            {
                city_values[nr_city_values] =
                    strcpy(city_values[nr_city_values], ReplaceChar(entry->d_name,'_',' ').c_str());
                nr_city_values++;
            }
        }
        closedir(conDir);
    }
    else
        esyslog("Can not read directory:%s  errno=%d\n", path.c_str(), errno);

    int current = Current();
    Clear();
    Add(new cOsdItem(tr("Please select your timezone"), osUnknown, false), false, NULL);
    Add(new cOsdItem(" ", osUnknown, false), false, NULL);
    Add(new cMenuEditStraItem(tr("Area"), &currentCon, nr_con_values, con_values));
    Add(new cMenuEditStraItem(tr("City/Region"), &currentCity, nr_city_values, city_values));
    Add(new cOsdItem("", osUnknown, false), false, NULL);  //blank line
    Add(new cMenuEditStraItem(tr("Setup.EPG$Set system time by"), &setSystemTime, 3, systemTimeValues));
    if (setSystemTime == 1)
        Add(new cMenuEditTranItem(tr("Setup.EPG$Use time from transponder"), &timeTransponder, &timeSource));

    SetCurrent(Get(current));
    Display();

}

eOSState cSetupMenuTimezone::ProcessKey(eKeys Key)
{

    int oldSetSystemTime = setSystemTime;
    int oldContinent = currentCon;

    eOSState state = cOsdMenu::ProcessKey(Key);

    if (Key == kNone)
    {
       state = osUnknown;
    }
    else if (Key == kOk)
    {
        Save();
        state = osBack;
    }
    else
    {
        if (setSystemTime != oldSetSystemTime || oldContinent != currentCon)
            Set();
    }
    return state;
}

void cSetupMenuTimezone::Save()
{

    Skins.Message(mtInfo, tr("Setting changes - please wait..."), 1);
    std::string buf = std::string(con_values[currentCon]) + "/" + ReplaceChar(city_values[currentCity],' ','_');

    cSysConfig_vdr::GetInstance().SetVariable("ZONEINFO", buf.c_str());
    cSysConfig_vdr::GetInstance().Save();

    if (unlink("/etc/localtime") < 0)
        esyslog("ERROR Setup: can`t unlink /etc/localtime!");

    std::string command = std::string("cp /usr/share/zoneinfo/") + con_values[currentCon] + "/" + ReplaceChar(city_values[currentCity],' ', '_') + " /etc/localtime";
    if (SystemExec(command.c_str()) == -1)
    {
        esyslog("failed to execute command \"%s\"\n", command.c_str());
        Skins.Message(mtError, tr("Error - changes unsuccessful"), 3);
    }

    ::Setup.SetSystemTime = setSystemTime;
    if (setSystemTime == 2) // "via internet"
        SystemExec("settime_ntp on");
    else
        SystemExec("settime_ntp off");
    ::Setup.TimeTransponder = timeTransponder;
    ::Setup.TimeSource = timeSource;
    ::Setup.Save();
    Skins.Message(mtInfo, tr("Changes done"), 1);

}

//#########################################################################################
//  cSetupGenericMenu
//#########################################################################################

cSetupGenericMenu::cSetupGenericMenu(const char *title, MenuNode * node): cOsdMenu(tr(title)), execState_(eCSCommandNoOp)
{
    printf("GenericMenu ('%s')\n", title);
    xmlNode_ = node;
    wasEdited_ = false;
    init_ = true;
    title_ = title;

    if (xmlNode_)
        Set();

    init_ = false;
}

void cSetupGenericMenu::Set()
{
    //dsyslog (DBG " cSetupGenericMenu::Set  %d", execState_);
    //printf (DBG " cSetupGenericMenu::Set  %d", execState_);

    int current = Current();
    Clear();
    SetHasHotkeys();

    BuildMenu(xmlNode_);

    // Set status here so that there is no delay before the status message
    // is displayed by processkey

    // make sure we get the first node, if current it not set
    if(current < 0)
        current = 0;

    MenuNode *currentNode = xmlNode_->GetNode_openIfs(current);
    DisplayStatus(currentNode);

    SetCurrent(Get(current));
    SetTitle(tr(title_)); // OSD Lang might have changed, lets not break banner image

    Display();

}

void cSetupGenericMenu::BuildMenu(MenuNode* xmlNode)
{
    int current = Current();

    for (int i = 0; i < xmlNode->GetNr(); i++)
    {
        eMenuType type = xmlNode->GetNode(i)->GetType();

        if (type == mtEntry)
        {
#if APIVERSNUM < 10700
            SetCols(23);
#else
            SetCols(28);
#endif
            MenuEntry *entry = xmlNode->GetNode(i)->GetMenuEntry();

            /*if (current + 1 == i) // Status is set by DisplayStatus() not from inside BuildMenu()
            {
                if (entry && entry->GetInfo())
                {
                    //printf (" ---------------------- Generic:Set() Menu  GetInfo  -------------- \n");
                    SetStatus(xmlNode->GetNode(i)->GetMenu()->GetInfo()); //RC: no
                }
            }*/

            const char *tmp;

            if(init_)
            {
                cSysConfig_vdr & sysConfig = cSysConfig_vdr::GetInstance();

                if ((tmp = sysConfig.GetVariable(entry->SysconfigKey())) != NULL)
                    entry->SetValue(Util::UNDEFINED, tmp);  // keeps old EntryType
            }

            switch (entry->GetType())
            {
                case Util::BOOL:
                    if(entry->GetID())
                        Add(new cMenuEditBoolRefreshItem(nohk(tr(entry->GetName())), entry->GetValueBoolRef(), tr("off"), tr("on")));
                    else
                        Add(new cMenuEditBoolItem(nohk(tr(entry->GetName())), entry->GetValueBoolRef(), tr("off"), tr("on")));
                    break;

                case Util::NUMBER:
                    if(entry->GetID())
                        Add(new cMenuEditIntRefreshItem(nohk(tr(entry->GetName())), entry->GetValueNumberRef(), 0, 999999999));
                    else
                        Add(new cMenuEditIntItem(nohk(tr(entry->GetName())), entry->GetValueNumberRef(), 0, 999999999));
                    break;

                case Util::TEXT:
                    Add(new
                            cMenuEditStrItem(nohk(tr(entry->GetName())),
                                const_cast < char *>(entry->GetValue()),
                                entry->GetValueTextMaxLen(),
                                trVDR(FileNameChars)));
                    break;
                case Util::CONSTANT_TEXT:
                    Add(new cOsdItem(tr(entry->GetName()),osUnknown, false));
                    break;

                case Util::NUMBER_TEXT:
                    Add(new
                            cMenuEditStrItem(nohk(tr(entry->GetName())),
                                entry->GetValue(),
                                entry->GetValueTextMaxLen(),
                                "0123456789"));
                    break;

                case Util::IP:
                    Add(new
                            cMenuEditIpNumItem(nohk(tr(entry->GetName())),
                                entry->GetValueIp()));
                    break;

                case Util::HEX:
                    Add(new
                            cMenuEditStrItem(nohk(tr((char *)entry->GetName())),
                                const_cast < char *>(entry->GetValue()),
                                entry->GetValueTextMaxLen(),
                                "0123456789ABCDEF:"));
                    break;
                case Util::PIN:
                    Add(new
                            cMenuEditNumItem(nohk(tr(entry->GetName())),
                                entry->GetValue(), 4, false));
                    break;
                case Util::SELECTION:
                    if(1 || entry->GetID()) /* TB: cMenuEditStraItem doesn't translate */
                        Add(new cMenuEditStraRefreshItem(nohk(tr(entry->GetName())), entry->GetReferenceSelection(), entry->GetNrOfSelectionValues(), entry->GetSelectionValues()));
                    else
                        Add(new cMenuEditStraItem(nohk(tr(entry->GetName())), entry->GetReferenceSelection(), entry->GetNrOfSelectionValues(), entry->GetSelectionValues()));
                    break;
                case Util::SPACE:
                    Add(new cOsdItem(" ", osUnknown, false), false, NULL);
                    break;
                default:
                    break;
            }
        }
        else if(type == mtIfblock)
        {
            IfBlock *ifBlock = xmlNode->GetNode(i)->GetIfBlock();

            if(ifBlock->isOpen())
                BuildMenu(xmlNode->GetNode(i));
        }
        else
        {
            char *tmp = NULL;
            // Plugins MenuEntry are already translated
            if (type != mtMenuplugin || type != mtEntry || type != mtUndefined)
            {
                asprintf(&tmp, "%s%s", tr(xmlNode->GetNode(i)->GetName()), SetupSetup._menuSuffix);
            }
            else
            {
                asprintf(&tmp, "%s%s", xmlNode->GetNode(i)->GetName(), SetupSetup._menuSuffix);
            }
            /*if (current + 1 == i) // Status is set by DisplayStatus()
            {
                Menu *xmlMenu = xmlNode->GetNode(i)->GetMenu();
                if (xmlMenu && xmlMenu->GetInfo())
                {
                    SetStatus(tr(xmlMenu->GetInfo())); //RC
                }
            }*/
            Add(new cOsdItem(hk(tmp)));
            //printf (" GenericMenu Set  free(tmp) %s %p \n", tmp, &tmp);
            free(tmp);
        }
    }
}

eOSState cSetupGenericMenu::ProcessKey(eKeys Key)
{
    //   bool execState = myState(Key);
    execState_ = command_.ExecState();

    if (execState_ == eCSCommandRunning)
    {
        Skins.Message(mtInfo, tr("Setting changes - please wait..."), 10);
        return osContinue;
    }
/*
    else if (Key == kNone && (execState_ == eCSCommandError || execState_ == eCSCommandSuccess))
    {
        Set();
    }
*/
    bool HadSubMenu = HasSubMenu();

    eOSState state = cOsdMenu::ProcessKey(Key);

    /* redraw menu, OSD Lang might have been changed */
    if(HadSubMenu && !HasSubMenu())
    {
        printf("=========== submenu closed, Calling Generic Set state=%d  ===========\n", state);
        Set();
        //return osContinue;
    }

    if (HadSubMenu || HasSubMenu())
        return state;

    if(state == os_User) // os_User = Refresh menu
    {
        Set();
        Display();
        return osContinue;
    }

    MenuNode *currentNode = xmlNode_->GetNode_openIfs(Current());

    DisplayStatus(currentNode);

    if (state == osUnknown)
    {
        if (Key == kOk)
        {
            // printf (" \t ####################   ---   osUnknown && kOk ---  ####################  \n");
            if (currentNode && (currentNode->GetType() == mtEntry))
            {
                //printf (" \t ####################   ---   kOk && mtEntry    ####################  \n");
                //wasEdited_ = false;
                SaveValues(xmlNode_);

                MenuEntry *currentEntry = currentNode->GetMenuEntry();
                currentEntry->SetSysConfValue();

                cSysConfig_vdr::GetInstance().Save();

                if (currentEntry->GetCommand())
                {
                    //DDD(" ####################   ---    Execute %s #################### ", currentEntry->GetCommand());
                    command_.Execute(currentEntry->GetCommand());
                }
                else if (xmlNode_->GetMenu()->GetCommand())
                {
                    command_.Execute(xmlNode_->GetMenu()->GetCommand());
                }
            }
            else if (currentNode && (currentNode->GetType() == mtExecute))
            {
                //printf (" \t ####################   --- state == mtExecute ---  ####################  \n");
                MenuNode *currentNode = xmlNode_->GetNode(Current());
                if (currentNode->GetType() == mtExecute)
                {
                    const char *Result = setup::ExecuteCommand(currentNode->GetMenu()-> GetExecute());
                    Skins.Message(mtStatus, NULL);
                    if (Result)
                        return AddSubMenu(new cMenuText(currentNode->GetMenu()-> GetName(), Result, fontFix));

                    //state = osEnd;
                }
            }
            else if (currentNode && (currentNode->GetType() > mtEntry)) //
            {
                //printf (" \t ######## ---  AddMenu Type %d ---  #######\n", currentNode->GetType());
                return AddMenu(currentNode, currentNode->GetType());
            }
            else
            {
                esyslog(ERR " Error in XmlFile: at Menu: %s ", xmlNode_->GetMenu()->GetName());
                // Display xml Error
                state = osContinue;
            }
        }                       //kOk
        else if (Key == kInfo)
        {
            return DisplayHelp(currentNode);
            state = osContinue; // state
        }
    }                           //  osUnknown

    if (Key == kNone)
       state = osUnknown;
    else
    {
        DisplayStatus(currentNode);

        if (command_.ExecState() == eCSCommandError || command_.ExecState() == eCSCommandSuccess)
        {
            command_.Reset();
        }
    }

    execState_ = command_.ExecState();
    //DDD("execState: %d", execState_);
    switch (execState_)
    {
	case eCSCommandRunning:
	    //SetStatus(tr("Setting changes - please wait..."));
	    Skins.Message(mtInfo, tr("Setting changes - please wait..."), 1);
	    state = osContinue;
	    break;
	case eCSCommandError:
	    //SetStatus(tr("Error - changes unsuccessful"));
	    Skins.Message(mtError, tr("Error - changes unsuccessful"), 3);
	    command_.Reset();
	    state = osContinue;
	    break;
	case eCSCommandSuccess:
	    //SetStatus(tr("Changes done"));
	    Skins.Message(mtInfo, tr("Changes done"), 1);
	    command_.Reset();
	    state = osBack;
	    break;
	case eCSCommandNoOp:
	default:
	    //        printf(tr("------- SetStatus(NULL) ---------- \n"));
	    break;
    }

    //DDD("return state: %d", state);
    return state;
}

eOSState cSetupGenericMenu::AddMenu(MenuNode * node, eMenuType type)
{
    eOSState state = osContinue;
    switch (type)
    {
    case mtMenu:
        return AddSubMenu(new cSetupGenericMenu(node->GetName(), node));
    case mtMenusystem:
        {
            cOsdMenu *menu = cSetupSystemMenu::GetSystemMenu(node->GetMenu()->GetSystem());
            if (menu)
                return (AddSubMenu(menu));

            else
                return osContinue;

        }
    case mtMenuplugin:
        return CallPlugin(node->GetMenu());

    default:
        state = osBack;
    }
    return state;
}

eOSState cSetupGenericMenu::DisplayHelp(MenuNode * currentNode)
{
    if (currentNode && currentNode->GetType() == mtEntry)
    {
        //printf ("SetupMenu NodeName %s \n", currentNode->GetName());
        //printf ("Setup MenuName(%s)  Info (%s) \n", currentNode->GetName(), currentNode->GetMenuEntry()->GetHelp());

        if (currentNode->GetMenuEntry()->GetHelp())
        {
            help_.Load(currentNode->GetMenuEntry()->GetHelp());
            char *buffer;
            asprintf(&buffer, "%s - %s", tr("Help"), currentNode->GetMenuEntry()->GetName());
            return AddSubMenu(new cMenuText(buffer, help_.Text()));
            Set();
        }
        else
        {
            SetStatus(tr("No help available"));
        }
    }
    else if (currentNode && currentNode->GetType() > mtEntry)
    {
        //printf ("SetupMenu NodeName %s \n", currentNode->GetName());
        //printf ("Setup MenuName(%s)  Info (%s) \n", currentNode->GetMenu()->GetName(), currentNode->GetMenu()->GetHelp());

        if (currentNode->GetMenu()->GetHelp())
        {
            help_.Load(currentNode->GetMenu()->GetHelp());
            char *buffer;
            asprintf(&buffer, "%s - %s", tr("Help"), currentNode->GetMenu()->GetName());
            return AddSubMenu(new cMenuText(buffer, help_.Text()));
            Set();
        }
        else
        {
            SetStatus(tr("No help available"));
        }
    }
    return osContinue;
}

void cSetupGenericMenu::DisplayStatus(MenuNode * xmlNode)
{
    if (xmlNode)
    {
        if (xmlNode->GetType() == mtEntry)
        {
            if (xmlNode->GetMenuEntry())
                SetStatus(xmlNode->GetMenuEntry()->GetInfo()); //RC
        }
        else if (xmlNode->GetType() > mtEntry)  // some menu
        {
            if (xmlNode->GetMenu())
                SetStatus(tr(xmlNode->GetMenu()->GetInfo())); //RC
        }
        else
            SetStatus(NULL);
    }
}

void cSetupGenericMenu::ExecuteCommand(const char *cmd)
{
    if (cmd)
    {
        if ((SystemExec(cmd)) != 0)
            Skins.Message(mtError, tr("Configuration error"));
        else
            Skins.Message(mtInfo, tr("Changes done"));
    }
}


void cSetupGenericMenu::SaveValues(MenuNode *node)
{
    if (!node) return;

    for (int i = 0; i < node->GetNr(); i++)
    {
        eMenuType type = node->GetNode(i)->GetType();
        if (type == mtEntry)
        {
            MenuEntry *menuEntry = node->GetNode(i)->GetMenuEntry();
            if (menuEntry && menuEntry->GetType() != Util::CONSTANT_TEXT)
            {
                menuEntry->SetSysConfValue();
                if (node->GetNode(i)->GetMenuEntry()->GetSetupCommand())
                {
                    if (strcmp
                        (node->GetNode(i)->GetMenuEntry()->
                         GetSetupCommand(), "CHANNELLIST") == 0)
                    {
                        //printf(" SaveValues %s \n", cSysConfig_vdr::GetInstance().  GetVariable("CHANNELLIST"));
                        setup::SwitchChannelList(cSysConfig_vdr::GetInstance().
                                                 GetVariable("CHANNELLIST"));
                    }
                }
            }
        }
        else if ( type == mtIfblock && node->GetNode(i)->GetIfBlock()->isOpen())
            SaveValues(node->GetNode(i)); // Save recursively through all open if block
    }
}

eOSState cSetupGenericMenu::CallPlugin(Menu * menu)
{
    cPlugin *p = cPluginManager::GetPlugin(menu->GetPluginName());
    if (p)
    {
        cOsdObject *menu = p->MainMenuAction();
        if (menu)
        {
            if (menu->IsMenu())
                return AddSubMenu(static_cast < cOsdMenu * >(menu));
            else
            {
                cMenuMain::SetPluginOsdObject(menu);
                return osPlugin;
            }
        }
        else
            return osBack;      //
    }
    esyslog("setup: No vaild Plugin %s found ", menu->GetPluginName());
    return osBack;              // No valid Plugin found
}


// Alignes item entries without hotkey numbering
const char * cSetupGenericMenu::nohk(const char *str) const
{
    static std::string tmp;
    tmp = SetupSetup._entryPrefix;

    if (strlen(SetupSetup._entryPrefix) == 0 || SetupSetup._entryPrefix[0] == ' ')
        tmp = "      ";
    else
    {
        tmp = "  ";
        tmp += SetupSetup._entryPrefix;
        tmp += "  ";
    }
    tmp += str;

    return tmp.c_str();
}



//#################################################################################################
// cSetupSystemMenu
//#################################################################################################
cSetupSystemMenu::cSetupSystemMenu()
{
}

cSetupSystemMenu::~cSetupSystemMenu()
{
}

// Get SystemMenu
cOsdMenu * cSetupSystemMenu::GetSystemMenu(const char *InternalMenu)
{
    //printf (" -----------------------  GetSystemMenu: %s  ----------------- \n", InternalMenu);
    cOsdMenu *menu = NULL;
    struct service_menu { cOsdMenu* menu;} serviceData;

    if (InternalMenu != NULL)
    {
        if (strcmp(InternalMenu, "OSD") == 0)
#if APIVERSNUM > 10700
            menu = new cSetupmenuSetupOSD;
#else
            menu = new cMenuSetupOSD;
        else if (strcmp(InternalMenu, "Language") == 0)
            menu = new cMenuSetupLang;
#endif
        else if (strcmp(InternalMenu, "EPG") == 0)
            menu = new cMenuSetupEPG;
        else if (strcmp(InternalMenu, "DVB") == 0)
            menu = new cMenuSetupDVB;
        else if (strcmp(InternalMenu, "Video") == 0) {
                if(cPluginManager::CallAllServices("Get VideoSetting Menu", &serviceData))
                    menu = serviceData.menu;
        } else if (strcmp(InternalMenu, "Audio") == 0) {
                if(cPluginManager::CallAllServices("Get AudioSetting Menu", &serviceData))
                    menu = serviceData.menu;
        } else if (strcmp(InternalMenu, "LNB") == 0)
            menu = new cMenuSetupLNB;
#if VDRVERSNUM < 10716
        else if (strcmp(InternalMenu, "CICAM") == 0)
            menu = new cMenuSetupCICAM;
        else if (strcmp(InternalMenu, "Record") == 0)
            menu = new cMenuSetupRecord;
        else if (strcmp(InternalMenu, "Replay") == 0)
            menu = new cMenuSetupReplay;
        else if (strcmp(InternalMenu, "Misc") == 0)
            menu = new cMenuSetupMisc;
        else if (strcmp(InternalMenu, "Plugins") == 0)
            menu = new cMenuSetupPlugins;
#else
//TODO: Needs implementation for SetupCICAM?!?
        else if (strcmp(InternalMenu, "Record") == 0)
            menu = new cSetupmenuRecording;
        else if (strcmp(InternalMenu, "Replay") == 0)
            menu = new cSetupmenuReplay;
        else if (strcmp(InternalMenu, "Misc") == 0)
            menu = new cSetupmenuBackground;
        else if (strcmp(InternalMenu, "Plugins") == 0)
            menu = new cSetupmenuSetupPlugins;
#endif
        else if (strcmp(InternalMenu, "Timeshift") == 0)
            menu = new cMenuSetupLiveBuffer;
        else if (strcmp(InternalMenu, "Timezone") == 0)
            menu = new cSetupMenuTimezone;
        else if (strcmp(InternalMenu, "RecordingDevice") == 0)
            menu = new cSetupRecordingDeviceMenu(tr("Media Device"));
        else if (strcmp(InternalMenu, "WLAN") == 0)
        {
            int CountDevices = 0;
            char *strBuff;
            FILE *process;

            // first modprobe drivers
            asprintf(&strBuff, "modprobe %s 1>/dev/null 2>/dev/null", WLAN_DRIVER);
            SystemExec(strBuff);
            free(strBuff);

            // List wlan-devices
            process = popen("iwconfig | grep -E 'IEEE|unassociated'", "r");
            if (process)
            {
                cReadLine readline;
                strBuff = readline.Read(process);
                while(strBuff)
                {
                    ++CountDevices;
                    strBuff = readline.Read(process);
                }
                pclose(process);
            }

            if(CountDevices)
                menu = new cSetupWLANMenu(tr("Wireless LAN"));
            else
                Skins.Message(mtWarning, tr("No wlan device found!"));
        }
        else if (strcmp(InternalMenu, "EnergyOptions") == 0)
            menu = new cEnergyOptionsMenu(tr("Energy Options"));
        else if (strcmp(InternalMenu, "NetworkFilesystem") == 0)
            menu = new cSetupNetworkFilesystemMenu(tr("Network Drives"));
#ifdef RBMINI
        else if (strcmp(InternalMenu, "MultiRoom") == 0)
            menu = new cSetupNetClientMenu("NetClient");
#else
        else if (strcmp(InternalMenu, "MultiRoom") == 0)
        {
#ifdef USEMYSQL
            if(cPluginSetup::MysqlInstallThread_ && cPluginSetup::MysqlInstallThread_->Active())
                Skins.Message(mtWarning, tr("Database is being initialized, retry later please!"));
            else
                menu = new cMultiRoomSetupMenu();
#else
            menu = new cMultiRoomSetupMenu();
#endif
        }
        else if (strcmp(InternalMenu, "NetClient") == 0)
            menu = new cSetupNetClientMenu("NetClient");
#endif
        else if (strcmp(InternalMenu, "ResetSettings") == 0)
            menu = new cMenuSetupResetSettings();
#ifdef USEMYSQL
        else if (strcmp(InternalMenu, "MySQL") == 0)
            menu = new cSetupMysqlMenu(tr("Database Setup"));
#endif
#if WITH_PLUGINMENU
        else if (strcmp(InternalMenu, "ActPlugins") == 0)
            menu = new cSetupPluginMenu;
#endif
        else if (strcmp(InternalMenu, "VDRMenu") == 0)
        {
            //printf (" ----------------------- Edit MainMenu ----------------- \n");
            menu = new cSetupVdrMenu(tr("Menu Edit"));
        }
        else if (strcmp(InternalMenu, "ChannelList") == 0)
        {
            struct {
                const char *title;
                const char *path;
                std::string AdditionalText;
                bool WizardMode;
                cOsdMenu* menu;
            } Data;
            Data.title = NULL;
            Data.path = NULL;
            // show only 'load pre-defined channel list option', if user has only sat tuners
            if (AreAllTunersSat()) {
                const char *configDir = cPlugin::ConfigDirectory();
                // "/etc/vdr/plugins", but needed it "/etc/vdr/channels/Satellite"
                static cString path;
                path = cString::sprintf("%s/../channels/Satellite", configDir);

                Data.path = *path; //"/etc/vdr/channels/Satellite";
            }
            Data.WizardMode = false;
            if (cRecordControls::Active())
                Skins.Message(mtError, tr("Please stop recording and retry"),4);
            else {
                cPlugin *plug = cPluginManager::GetPlugin("channelscan");
                if (plug) {
                    plug->Service("ChannelList", &Data);
                    menu = Data.menu;
                } else
                    esyslog("ERROR: channelscan-plugin not found!");
            }
        }
        //else if (strcmp(InternalMenu, "Channels") == 0)
        //    menu = new cMenuEditChannels();
    }
    else
        esyslog(ERR " No SystemComand set");
    return menu;
}

//#############################################################################################
// Class cSetupMenu
//#############################################################################################

#define CODELENGTH 4            //pin code length
// Todo:  Remove this and use  only GenericMenu /use var Level to control instances

cSetupConfig * cSetupConfig::instance_ = NULL;

cSetupMenu::cSetupMenu(const char *label, const char *submenu):cOsdMenu(tr("Title$Setup"), 25)
{
    HelpMenus.Load(); // Load help information

    calledSubmenu_ = false;
    menuLabel_ = label;
    cSetupConfig::Create();
    //  cSetupConfig::Destroy()
    //  is called in ~cPluginSetup()
    if (!cSetupConfig::GetInstance().Load())
    {
        SetStatus(tr("Error in configuration files"));
        return;
    }

    childLock_.Init();
    //TB credits_.Load();

    setupLocked_ = childLock_.IsLocked();
    childLockEntered_[0] = '\0';

    Set();

    if (cPluginSetup::MenuLink && menuLabel_ && strlen(menuLabel_) > 0)
    {
        Menus * xmlMainMenus = cSetupConfig::GetInstance().GetMainMenus();
        for (int i = 0; i < xmlMainMenus->GetNr(); i++)
        {
            if (strcmp(xmlMainMenus->GetMenuNode(i)->GetName(), menuLabel_) == 0)
            {
                cSetupGenericMenu * menu;
                AddSubMenu(menu = new cSetupGenericMenu(xmlMainMenus->
                                                 GetMenuNode(i)->GetName(),
                                                 xmlMainMenus->
                                                 GetMenuNode(i)));
            }
        }
    }

    if(submenu)
    {
        cOsdMenu *menu = cSetupSystemMenu::GetSystemMenu(submenu);
        if (menu)
        {
            calledSubmenu_ = true;
            AddSubMenu(menu);
        }
    }
}


cSetupMenu::~cSetupMenu()
{
    menuLabel_ = NULL;
#ifndef TIXML_USE_STL
    cSetupConfig::Destroy();
#endif
    cPluginSetup::MenuLink = false;
}

void cSetupMenu::Set()
{
    int current = Current();
    Clear();

    if (setupLocked_)
        Add(new cMenuEditNumItem("Enter PIN", childLockEntered_, CODELENGTH, true));
    else
    {
        SetStatus(false);

        Menus *xmlMenus = cSetupConfig::GetInstance().GetMenus();

        MenuNode *currentNode = NULL;

        SetHasHotkeys();

        for (int i = 0; i < xmlMenus->GetNr(); i++)
        {
            currentNode = xmlMenus->GetMenuNode(i);
            char *tmp = NULL;

            if (currentNode->GetType() != mtMenuplugin)
            {
                asprintf(&tmp, "%s%s", tr(currentNode->GetName()), SetupSetup._menuSuffix);
                if (current == i)
                {
                    Menu *xmlMenu = currentNode->GetMenu();
                    if (xmlMenu->GetInfo())
                        SetStatus(tr(xmlMenu->GetInfo()));
                    else
                        SetStatus(NULL);
                }
            }
            else
            {
                //  plugin entryStrings are translated already
                asprintf(&tmp, "%s%s", currentNode->GetName(), SetupSetup._menuSuffix);
                if (current == i)
                {
                    Menu *xmlMenu = currentNode->GetMenu();
                    if (xmlMenu->GetInfo())
                    {
                        //printf("---------- %s   %s --------- \n", tr(currentNode->GetName()), xmlMenu->GetInfo());
                        SetStatus(tr(xmlMenu->GetInfo()));
                    }
                    else
                        SetStatus(NULL);
                }
            }

            Add(new cOsdItem(hk(tmp)));
            free(tmp);
        }
    }
    Add(new cOsdItem(hk(tr("System Information")), osUser1));

    SetCurrent(Get(current));
    SetHelp(NULL);
    SetTitle(tr("Title$Setup")); // Language might have changed => no banner

#if VDRVERSNUM >= 10716
    Display();
#endif
    //Display(); //TB: don't do this twice - this is called by vdr.c around line 1315 via menu->Show()
}


/**
 * Procss Key
 * @param Key
 * @return
 */

eOSState
cSetupMenu::ProcessKey(eKeys Key)
{
    if (cPluginSetup::MenuLink && Key == kBack)
    {
        cPluginSetup::MenuLink = false;
        return osBack;
    }

    if (calledSubmenu_ && Key == kBack)
        return osBack;

    bool HadSubMenu = HasSubMenu();

    eOSState state = cOsdMenu::ProcessKey(Key);

    //prevent color keys to work within "system programs"
    if (state == osUnknown && (Key == kRed || Key == kGreen || Key == kYellow || Key == kBlue))
	return osContinue;

//XXX =======

    // redraw menu, OSD Lang might have been changed /
    if(HadSubMenu && !HasSubMenu()){
        printf("************* subMenu closed, Calling cSetupMenu::Set()  state=%d************\n ", state);
        Set();
    }

//XXX

    if (HasSubMenu() || HadSubMenu)
        return state;

    if (Key != kNone)
    {
        if (setupLocked_)
        {
            if (strlen(childLockEntered_) == CODELENGTH)
            {
                setupLocked_ = childLock_.CompareInput(childLockEntered_) ? false : true;
                if (setupLocked_)
                {
                    SetStatus(tr("Wrong PIN entered"));
                    childLockEntered_[0] = '\0';
                }
                //printf (" +++ ProcKey Key != kNone Call Set() +++ \n");
                Set();
            }
        }
        if (Key == kBack)
        {
            return osBack;
        }
    }

    if (state == osUnknown && setupLocked_)
    {
        switch (Key)
        {
        case kOk:              //printf (" +++ ProcKey kOk && setupLocked_ Call Set() \n");
            Set();
            return osContinue;
        default:
            return osUnknown;
        }
    }
    else if (state == osUser1)
    {
        return AddSubMenu(new cMenuSystemInfo);
    }

    int current = Current();

    MenuNode *node =
        cSetupConfig::GetInstance().GetMenus()->GetMenuNode(current);
    // SetupMenu
    if (node && node->GetMenu())
    {
        if (node->GetMenu()->GetInfo())
            SetStatus(tr(node->GetMenu()->GetInfo()));
        else
            SetStatus(NULL);
    }
    /*else
    {
        SetStatus(tr("System information"));
    }
    */


    switch (Key)
    {
/*
    case kInfo:
        {
            //printf ("SetupMenu kHelp? \n");
            if (node && node->GetMenu())
            {
                //printf ("SetupMenu NodeName %s \n", node->GetName());
                //printf ("Setup MenuName(%s)  Info (%s) \n", node->GetMenu()->GetName(), node->GetMenu()->GetHelp());

                if (node->GetMenu()->GetHelp())
                {
                    help_.Load(node->GetMenu()->GetHelp());
                    char *buffer;
                    asprintf(&buffer, "%s - %s", tr("Help"), tr(node->GetMenu()->GetName()));
                    return AddSubMenu(new cMenuText(buffer, help_.Text()));
                }
                else
                    SetStatus(tr("No help available"));
            }
            else
                SetStatus(tr("System Help"));   // FIXIT
            return osContinue;
        }
*/
    case kOk:
        {
            SetStatus(NULL);

            if (node)
            {

                //printf (" -------------------  Name %s  mtType %d ------------- \n", node->GetName(),  node->GetType());
                if (node->GetType() == mtExecute)
                {
                    //printf (" -------------------  %s ExecuteCommand %s ------------- \n", node->GetMenu()->GetName(), node->GetMenu()->GetExecute());
                    const char *Result = setup::ExecuteCommand(node->GetMenu()->GetExecute());
                    Skins.Message(mtStatus, NULL);
                    if (Result)
                        return AddSubMenu(new cMenuText(node->GetMenu()-> GetName(), Result, fontFix));

                    return osEnd;
                }
                if (node->GetType() == mtMenu)
                {
                    cSetupGenericMenu *menu;
                    AddSubMenu(menu = new cSetupGenericMenu(node->GetName(), node));
                }
                else if (node->GetType() == mtMenusystem)
                {
                    //printf (" -------------------  %s GetSystem Menu %s ------------- \n", node->GetMenu()->GetName(), node->GetMenu()->GetSystem());
                    cOsdMenu *menu = cSetupSystemMenu::GetSystemMenu(node->GetMenu()-> GetSystem());
                    if (menu)
                        AddSubMenu(menu);

                }

                else if (node->GetType() == mtMenuplugin)       // MARKUS
                    return CallPlugin(node->GetMenu());
            }

            SetCurrent(Get(current));

            return osContinue;
        }
    case kUp:
    case kDown:
        {
            if (node && node->GetMenu() && node->GetMenu()->GetInfo())
                SetStatus(tr(node->GetMenu()->GetInfo()));
            else
                SetStatus(NULL);
        }
        break;

    case kBack:
        return osBack;

    case kRed:
    case kGreen:
    case kYellow:
    case kBlue:
    case kInfo:
    case kStop:
    case kPlay:
    case kChanUp:
    case kChanDn:
        return osContinue;
    default:
        return osUnknown;
    }
    //DDD("return state %d", state);
    return state;
}

eOSState cSetupMenu::CallPlugin(Menu * menu)
{
    //Service();
    cPlugin *p = cPluginManager::GetPlugin(menu->GetPluginName());
    if (p)
    {
        cOsdObject *menu = p->MainMenuAction();
        if (menu)
        {
            if (menu->IsMenu())
                return AddSubMenu(static_cast < cOsdMenu * >(menu));
            else
            {
                cMenuMain::SetPluginOsdObject(menu);
                return osPlugin;
            }

        }
        else
            return osBack;      //

    }
    esyslog("setup: No vaild Plugin %s found ", menu->GetPluginName());
    return osBack;              // No valid Plugin found
}



#if  WITH_PLUGINMENU

// --- Class cSetupPluginMenu  ------------------------------------------------------

cSetupPluginMenu::cSetupPluginMenu()
 :cOsdMenu(tr("Plugins Enable / Disable"), 25)
{
    //dsyslog(DBG " cSetupPluginMenu ");
    moveMode_ = false;
    sortMode_ = false;

    cSetupPlugins & setupPlugins = cSetupPlugins::GetInstance();
    Skins.Message(mtInfo, tr("Loading external plugins"), 3);
    setupPlugins.LoadPlugins();
    SetStatus(NULL);

    Set();
}

cSetupPluginMenu::~cSetupPluginMenu()
{
}

void
cSetupPluginMenu::Set()
{

    int current = Current();

    Clear();

    for (int i = 0; i < setupPlugins.Size(); i++)
    {

        //dsyslog (DBG  " cSetupPlugins:  %s:  protect %d active %d", setupPlugins.Get(i).GetPluginName(), *setupPlugins.Get(i).Protected(), *setupPlugins.Get(i).Active());

        if (*setupPlugins.Get(i).Protected() == 1)
        {
            char *title = NULL;

            //dsyslog (DBG  " cSetupPlugins:  %s:\t%s",setupPlugins.Get(i).GetPluginName(),tr("protected"));
            asprintf(&title, "%s:\t%s", setupPlugins.Get(i).GetPluginName(),
                     tr("protected"));
            Add(new cOsdItem(title));
            free(title);
        }
        else
        {
            Add(new
                cMenuEditBoolItem(setupPlugins.Get(i).GetPluginName(),
                                  const_cast <
                                  int *>(setupPlugins.Get(i).Active()),
                                  tr("off"), tr("on")));
            //dsyslog (DBG  " cSetupPlugins:  %s:\t%s", setupPlugins.Get(i).GetPluginName(), "Activate");
        }
    }
    if (current == -1 && setupPlugins.Size() > 0)
        current = 0;

    SetCurrent(Get(current));

    SetHelp(tr("Edit"));
    Display();
}

eOSState cSetupPluginMenu::ProcessKey(eKeys Key)
{
    bool HadSubMenu = HasSubMenu();

    eOSState state = cOsdMenu::ProcessKey(Key);

    if (HasSubMenu() || HadSubMenu)
        return state; // DO return the state or osContinue will not be handled

    switch (Key)
    {
    case kOk:                  //setupConfig->SaveSysconfig();  //??
        cSetupPlugins::GetInstance().Save();
        return osBack;
        break;
    case kYellow:
        if (moveMode_)
        {
            _plugins->MovePlugin(startIndex_, Current(), Plugins::BEFORE);
            moveMode_ = !moveMode_;
            Set();
        }
        else                    // Edit Parameter
        {
            return (AddSubMenu
                    (new
                     cSetupPluginParameter(tr("Plugin-Parameter"),
                                           _plugins->Get(Current()))));
        }
        break;
    case kBlue:
        if (moveMode_)
        {
            _plugins->MovePlugin(startIndex_, Current(), Plugins::BEHIND);
        }
        else
        {
            startIndex_ = Current();
        }
        moveMode_ = !moveMode_;
        Set();
        break;
        //case kDown:
        //case kUp: setHelp(); break;
        // case kRed:   return cSetupEditPlugin(index);
    default:
        //state = osUnknown;
        break;
    }
    return state;
}

#endif

//#################################################################################################
//  Edit Plugin Parameter
//################################################################################################

#if 0

//FIXME: use FileNameChars
static char *ALLOW_ALL_PARAM_CHARS =
    " a?bcdefghijklmno?pqrs?tu?vwxyz0123456789+-={}[]().,;#*?!@/~\"\'";
cSetupPluginParameter::cSetupPluginParameter(const char *title,
                                             Plugin * plugin):
cOsdMenu(title, 25),
_plugin(plugin)
{
    _edit = false;
    Set();
}

cSetupPluginParameter::~cSetupPluginParameter()
{
}

void cSetupPluginParameter::Set()
{
    char *tmp = NULL;
    const char *param = _plugin->GetParameter();
    Clear();
    if (param == NULL)
        _editParameter[0] = '\0';
    else
    {
        strncpy(_editParameter, param, sizeof(_editParameter));
        _editParameter[sizeof(_editParameter)] = '\0';
    }
    Add(new
        cMenuEditStrItem(tr("Plugin-Parameter"), _editParameter,
                         sizeof(_editParameter), ALLOW_ALL_PARAM_CHARS));

    asprintf(&tmp, "%s: %s", tr("Plugin"), _plugin->GetName());
    SetStatus(tmp);
    free(tmp);
    Display();
}

eOSState cSetupPluginParameter::ProcessKey(eKeys Key)
{

    eOSState state = cOsdMenu::ProcessKey(Key);

    switch (Key)
    {
    case kOk:
        if (!_edit)
            return osBack;
        else
        {
            _plugin->SetParameter(_editParameter);
            _edit = false;
        }
        break;
    case kRight:
        _edit = true;
        break;
    default:
        break;
    }
    return state;
}
#endif

//#################################################################################################
//  cSetupVdrMenu
//################################################################################################

//static char *ALLOW_ALL_CHARS = trNOOP(" aäbcdefghijklmnoöpqrsßtuüvwxyz0123456789-.!/");
static const char *ALLOW_ALL_CHARS = trNOOP(" abcdefghijklmnopqrstuvwxyz0123456789-.!/");

cSetupVdrMenu::cSetupVdrMenu(const char *title):
cOsdMenu(title, 25)
{
    startIndex_ = 0;
    createEditNodeIndex_ = 0;

    // Load Menu Configuration
    string menuXML = setup::FileNameFactory("setup");
    menuXML += "/vdr-menu.xml";
    vdrSubMenu_.LoadXml(menuXML.c_str());

    menuState_ = UNDEFINED;
    Set();
}

cSetupVdrMenu::~cSetupVdrMenu()
{
}

void cSetupVdrMenu::Set()
{
    int current = Current();
    cSubMenuNode *node = NULL;
    int nr = vdrSubMenu_.GetNrOfNodes();

    Clear();

    switch (menuState_)
    {
    case UNDEFINED:
    case MOVE:
        for (int i = 0; i < nr; i++)
        {
            if ((node = vdrSubMenu_.GetAbsNode(i)) != NULL)
            {
                char *tmp = createMenuName(node);
                Add(new cOsdItem(tmp));
                free(tmp);
            }
        }
        current = createEditNodeIndex_;
        if (current >= nr)
            current = nr - 1;
        break;
    case EDIT:
        strncpy(editTitle_,
                vdrSubMenu_.GetAbsNode(createEditNodeIndex_)->GetName(),
                sizeof(editTitle_));
        editTitle_[sizeof(editTitle_)-1] = '\0';
        Add(new
            cMenuEditStrItem(tr("change menu name"), editTitle_,
                             sizeof(editTitle_), tr(ALLOW_ALL_CHARS)));
        break;
    case CREATE:
        strncpy(editTitle_, "", sizeof(editTitle_));
        Add(new
            cMenuEditStrItem(tr("change menu name"), editTitle_,
                             sizeof(editTitle_), tr(ALLOW_ALL_CHARS)));
        break;
    default:
        break;
    }

    SetCurrent(Get(current));
    setHelp();
    Display();

}
// --- Class cMenuEditIpNumItem ------------------------------------------------------

cMenuEditIpNumItem::cMenuEditIpNumItem(const char *Name, char *Value)
 :cMenuEditItem(Name)
{
    val_type = Util::IP;
    value = Value;

    pos = 0;
    digit = 0;

    Init(); // split ipAdress into seperate segments
    Set();
}

cMenuEditIpNumItem::~cMenuEditIpNumItem()
{
    pos = 0;
    digit = 0;
}

void
cMenuEditIpNumItem::Set(void)
{
    // pos 0 indicates edit mode
    char cursor[6];
    sprintf(cursor, "[%3d]", val[pos]);
    char *buffer = (char*)"";

#if 0
    dsyslog(DBG " +++ cursor %s in feld %d ", cursor, pos);
    dsyslog(DBG " +++ segments  %d.%d.%d.%d\n", val[1], val[2], val[3],
            val[4]);
    dsyslog(DBG " +++ build String! +++ buf %s", buffer);
#endif

    snprintf(value, VALUEIPMAXLEN, "%d.%d.%d.%d", val[1], val[2], val[3],
             val[4]);

    if (pos != 0)
    {
        for (int i = 1; i < 5; i++)
        {
            if (pos == i)
                asprintf(&buffer, "%s%s%s", buffer, i == 1 ? "" : ".",
                         cursor);
            else
            {
                asprintf(&buffer, "%s%s%3d", buffer, i == 1 ? "" : ".",
                         val[i]);
            }
        }

        SetValue(buffer);
        free(buffer);
    }
    else
    {
        SetValue(value);
        this->SetFresh(true);
    }
}

unsigned char cMenuEditIpNumItem::Validate(char *Val)
{
    if (atoi(Val) <= 255)
    {
        return atoi(Val);
    }
    else
    {
        Val[2] = '\0';
        return atoi(Val);
    }
    return 1;
}

// this methode extracts the string into the u_int fields
void
cMenuEditIpNumItem::Init()
{
    pos = 0;
    char *s, *tok, *tmp;
    int i;
    if (strchr(value, '.'))
        tmp = tok = strdup(value);
    else
        tmp = tok = strdup("0.0.0.0");

    val[0] = 0;                 // leave first field empty
    for (i = 1; i <= 4; i++)
    {
        if (i == 4)
        {
            val[i] = atoi(tok);
            continue;
        }
        s = strchr(tok, '.');
        *s = '\0';
        val[i] = atoi(tok);
        *s = ' ';
        s++;
        tok = s;
    }
    free(tmp);
}

eOSState cMenuEditIpNumItem::ProcessKey(eKeys Key)
{
    eOSState state = cMenuEditItem::ProcessKey(Key);

    if (state == osUnknown)
    {

        Key = NORMALKEY(Key);

        switch (Key)
        {
        case kBack:
            if (pos != 0)
                pos = 0;
            else
                return cMenuEditItem::ProcessKey(Key);
            break;
        case kLeft:
            {                   // left shift numblock
                if (pos > 1)
                    pos--;
                if (pos == 0)
                    pos = 4;

                digit = 0;
            }
            //else { dsyslog (" kLeft Was Jetzt" ); }
            break;
        case kRight:
            {
                if (pos < 4)
                    pos++;

                digit = 0;
            }
            //else { dsyslog (" kRight Was Jetzt" ); }
            break;
        case kUp:
            if (pos != 0)
            {
                val[pos]++;
                digit = 0;
            }
            else                // if no edit mode go up next menuitem
                return cMenuEditItem::ProcessKey(Key);
            break;
        case kDown:
            if (pos != 0)
            {
                val[pos]--;
                digit = 0;
            }
            else                // if no edit mode go down next menuitem
                return cMenuEditItem::ProcessKey(Key);

            break;
        case k0...k9:
            {
                //dsyslog(DBG  "k0 ... k9 digit %d val[%d]  ", digit, pos);
                if (pos != 0)   // if edit mode
                {
                    if (digit > 2)      // got to  next element
                    {
                        digit = 0;
                        if (pos < 4)
                            pos++;
                    }

                    if (digit == 0)
                        val[pos] = 0;

                    char tmp[4];
                    snprintf(tmp, 4, "%d", val[pos]);   //XXX  ???
                    tmp[digit] = Key - k0 + '0';
                    digit++;
                    tmp[digit] = '\0';

#if 0
                    dsyslog(DBG "digit %d val[%d]  ", digit, pos);
                    dsyslog(DBG "val %d tmp %s", val[pos], tmp);
#endif

                    val[pos] = Validate(tmp);

#if 0
                    int len = strlen(tmp);
                    dsyslog(DBG
                            "key: k%d, val[pos]:%d, digit: %d pos %d, len:%d ",
                            Key - k0, val[pos], digit, pos, len);
                    dsyslog(DBG ">>> tmp %s ", tmp);
#endif
                }
                else
                    return cMenuEditItem::ProcessKey(Key);
            }
            break;
        case kOk:
#if 0
            dsyslog(DBG " kOk:  val[pos]:%d, digit: %d pos %d,  ",
                    val[pos], digit, pos);
#endif
            if(pos!=0)
            {
                pos = 0;
                digit = 0;

                //dsyslog(DBG "  call Set()");

                Set();
                return osContinue;
            }
            else
                return cMenuEditItem::ProcessKey(kNone);

        default:
            return cMenuEditItem::ProcessKey(kNone);
        }

        Set();
        state = osContinue;
    }
    return state;

}

