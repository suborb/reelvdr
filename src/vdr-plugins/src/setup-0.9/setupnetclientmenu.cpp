/***************************************************************************
 *   Copyright (C) 2008 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                 2009 additions by Tobias Bratfisch                      *
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
 ***************************************************************************
 *
 * setupnetclientmenu.cpp
 *
 ***************************************************************************/

#include <vdr/debug.h>
#include "vdr/menu.h"
#include <vdr/vdrmysql.h>
#include <vdr/videodir.h>
#include <vdr/plugin.h>
#include <vdr/sysconfig_vdr.h>
//#include "vdr/sysconfig_vdr.h"

#include "setupnetclientmenu.h"
#include "menueditrefreshitems.h"
#include "netinfo.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>

#ifdef RBMINI
  #define HDDEVICE "sdf"
#else
  #define HDDEVICE "sdb"
#endif

#define MAX_LETTER_PER_LINE 48

#define NETCLIENT_MODE_TEXT trNOOP("NetClient-Mode")
#define RECEIVER_TYPE_TEXT trNOOP("Receiver-Type")
#define STREAM_SERVER_TEXT trNOOP("Stream-Server")
#define RECORDING_SERVER_TEXT trNOOP("Recording-Server")

const char* ClientModeStrings[] = { trNOOP("Stand-alone"), "MultiRoom" };
const char* ClientModeInactiveStrings[] = { trNOOP("Stand-alone"), trNOOP("MultiRoom (inactive)") };
const char* ReceiverTypeStrings[] = { "DVB", "NetCeiver (mcli)", "Avantgarde (Streamdev)" };

//##################################################################################
// class cNetClientDatabaseChecker
//##################################################################################
cNetClientDatabaseChecker::cNetClientDatabaseChecker()
{
}

cNetClientDatabaseChecker::~cNetClientDatabaseChecker()
{
}

int cNetClientDatabaseChecker::Check(bool DisplayMessage)
{
    std::stringstream strBuff;

    // Test Connection to Host(AVG)
    strBuff << "ping " << Setup.NetServerIP << " -c 1 2>/dev/null 1>/dev/null";
    if(SystemExec(strBuff.str().c_str()))
    {
        if(DisplayMessage)
            Skins.QueueMessage(mtError, tr("Error: Connection to ReelBox Avantgarde failed!"));
        esyslog("%s(%i) ERROR (#1): Connection to host (%s) failed!", __FILE__, __LINE__, Setup.NetServerIP);
        return E_NETCLIENT_HOST_CONNECTION_FAILED;
    }
    strBuff.str("");

    // Test Connection to DB & reeluser Account
    cVdrMysqlAdmin *mysql = new cVdrMysqlAdmin(MYSQLREELUSER, MYSQLREELPWD, "vdr");
    if(!mysql->SetServer(Setup.NetServerIP))
    {
        if(DisplayMessage)
            Skins.QueueMessage(mtError, tr("Error: Cannot login with reeluser!"));
        esyslog("%s(%i) ERROR (#2): Cannot login with reeluser!", __FILE__, __LINE__);
        if(mysql)
            delete mysql;
        return E_NETCLIENT_DATABASE_LOGIN_FAILED;
    }

    // Test Tables
    if(!mysql->TestTables()) {
        if(DisplayMessage)
            Skins.QueueMessage(mtError, "Error: some database-table doesn't exist!");

        if(mysql)
            delete mysql;
        return E_NETCLIENT_DATABASE_TABLE_NOT_EXISTENT;
    }

    if(mysql)
        delete mysql;

#ifdef MYSQL_DEBUG
    std::cout << "\033[0;093m " << __FILE__ << '(' << __LINE__ << "): Database: OK! \033[0m\n";
#endif
    return 0;
}

//#########################################################################
// cSetupNetClientMenu
//#########################################################################
cSetupNetClientMenu::cSetupNetClientMenu(const char *title) : cOsdMenu(title)
{
    // Check if DB is available
    if(Setup.ReelboxMode && !Setup.ReelboxModeTemp)
        DatabaseAvailable_ = DatabaseAvailableOld_= false;
    else
        DatabaseAvailable_ = DatabaseAvailableOld_= true;

    WakeAVG_ = WakeAVGOld_ = true;
    cSysConfig_vdr &sysconfig = cSysConfig_vdr::GetInstance();
    if (sysconfig.GetVariable("WOL_WAKEUP_SERVER")) {
        if (strncmp(sysconfig.GetVariable("WOL_WAKEUP_SERVER"), "false", 5) == 0)
            WakeAVG_ = WakeAVGOld_ = false;
    }

    SetTitle(tr("MultiRoom Setup"));
    AVGChanged_ = false;
    ExpertSettings_ = false;
    if(GetUUID()=="")
        HasHarddisk_ = false;
    else
        HasHarddisk_ = true;

    ClientMode_ = ClientModeOld_ = Setup.ReelboxMode;

    if(ClientMode_ == eModeServer || ClientMode_ == eModeHotel) // eModeServer or eModeHotel
    {
        ClientMode_ = eModeClient;
        ClientModeOld_ = -1; // Since ClientMode>1 means somewhat is wrong here
    }

    // Get ReelBox-List from avahi-plugin
    cPlugin *p=cPluginManager::GetPlugin("avahi");
    if(p) {
        p->Service("Avahi MultiRoom-List", &ReelBoxes);
    } else {
        printf("ERROR: could not get the avahi-Plugin, menu will be broken\n");
        esyslog("ERROR: could not get the avahi-Plugin, menu will be broken\n");
    }

    DDD("ReceiverType_: %d", Setup.ReceptionMode);

    //ReceiverType_ = ReceiverTypeOld_ = eModeMcli;
    ReceiverTypeOld_ = ReceiverType_ = (eReceiverType) Setup.ReceptionMode;
    /*
    if (cPluginManager::GetPlugin("streamdev-client"))
    {
        if(Setup.Get("StartClient", "streamdev-client"))
            ReceiverType_ = ReceiverTypeOld_ = eModeStreamdev;
    }
    */
    // Mode: NetCeiver (=mcli)
    nrTuners.sat =  nrTuners.satS2 = nrTuners.cable = nrTuners.terr = 1;
    LoadMcliConf();
    nrTunersOld = nrTuners;

    // Mode: Avantgarde (=streamdev)
    NetServer.IP = Setup.NetServerIP;
    NetServer.Name = Setup.NetServerName;
    NetServer.MAC = Setup.NetServerMAC;

	// Preparations for AddInfo()
	Clear();
	Display();
	currText = tr(NETCLIENT_MODE_TEXT);

    Set();
    if(1 /*ReelBoxes*/)
        SetHelp(NULL, NULL, NULL, tr("Expert"));

//    if((ClientMode_==eModeClient) && (Setup.NetServerIP))
//        cNetClientDatabaseChecker::Check(true);
}

cSetupNetClientMenu::~cSetupNetClientMenu()
{
}

void cSetupNetClientMenu::Set()
{
    DatabaseAvailableOld_ = DatabaseAvailable_;
    std::string strBuff;
    int current = Current();
    Clear();
    SetCols(20);

    if(1 /*ReelBoxes*/) {
#if 0
        if(NetServerIP_ && strlen(NetServerIP_)) {
            for(unsigned int i=0; i<ReelBoxes->size(); ++i)
                if(!strcmp(ReelBoxes->at(i).Ip.c_str(), NetServerIP_))
                    DatabaseAvailable = true;
        }
#endif
        Add(new cMenuEditStraRefreshItem(tr(NETCLIENT_MODE_TEXT), &ClientMode_, 2, DatabaseAvailable_?ClientModeStrings:ClientModeInactiveStrings), IsCurrent(tr(NETCLIENT_MODE_TEXT)));

        if((ClientMode_ == eModeClient) || ExpertSettings_) {
            if(ExpertSettings_)
                Add(new cMenuEditStraRefreshItem(tr(RECEIVER_TYPE_TEXT), (int*)&ReceiverType_, 3, ReceiverTypeStrings), IsCurrent(tr(RECEIVER_TYPE_TEXT)));
            switch(ReceiverType_) {
                case eModeDVB:
                    // dummy, just to prevent compiler warning
                    break;
                case eModeStreamdev:
                    strBuff = std::string(STREAM_SERVER_TEXT) + ":\t";
                    if(ReelBoxes.size() == 0)
                        strBuff += tr("Not available!");
                    else if(NetServer.IP.size()) {
                        if(NetServer.Name.size())
                            strBuff += NetServer.Name;
                        else
                            strBuff += NetServer.IP;
                    } else
                        strBuff += tr("No Avantgarde");
                    Add(new cOsdItem(strBuff.c_str()), IsCurrent(tr(STREAM_SERVER_TEXT)));

                    break;
                case eModeMcli:
                    if(ClientMode_ == eModeClient) {
                        strBuff = std::string(tr(RECORDING_SERVER_TEXT)) + ":\t";
                        if(ReelBoxes.size() == 0)
                            strBuff += tr("Not available!");
                        else if(NetServer.IP.size()) {
                            if(NetServer.Name.size()) {
                                if(DatabaseAvailable_)
                                    strBuff += NetServer.Name;
                                else
                                    strBuff += std::string("(") + NetServer.Name + ")";
                            } else
                                strBuff += NetServer.IP;
                        } else
                            strBuff += tr("No Avantgarde");
                        Add(new cOsdItem(strBuff.c_str()), IsCurrent(tr(RECORDING_SERVER_TEXT)));
                    }

                    if(ExpertSettings_) {
                        Add(new cOsdItem("", osUnknown, false));
                        Add(new cMenuEditBoolItem(tr("Wakeup Avantgarde"), &WakeAVG_));
                        Add(new cOsdItem("", osUnknown, false));
                        Add(new cOsdItem(tr("Please enter the number of tuners to use:"), osUnknown, false));
                        Add(new cMenuEditIntItem(tr("Satellite (DVB-S)"), &nrTuners.sat, 0, 6));
                        Add(new cMenuEditIntItem(tr("Satellite (DVB-S2)"), &nrTuners.satS2, 0, 6));
                        Add(new cMenuEditIntItem(tr("Cable"), &nrTuners.cable, 0, 6));
                        Add(new cMenuEditIntItem(tr("Terrestrial"), &nrTuners.terr, 0, 6));
                    }
                    break;
            }
        }

        SetCurrent(Get(current));

        if (currText.size())
            AddInfo();
    } else
        AddFloatingText(tr("Error! Something went wrong in the Avahi-Plugin!"), MAX_LETTER_PER_LINE);

    Display();
}

void cSetupNetClientMenu::AddInfo() {
    //Adds info depending on the current item
    //printf("%s %s\n",__PRETTY_FUNCTION__, currText.c_str());

    char strBuff[512];
    int count = Count();
#if VDRVERSNUM < 10716
    int max_items = displayMenu->MaxItems();
#else
    int max_items = dynamic_cast<cSkinDisplayMenu *>(cSkinDisplay::Current())->MaxItems();
#endif
    int num_blank_lines = max_items - count - 7;
    while (--num_blank_lines > 0) Add(new cOsdItem("",osUnknown, false));

    Add(new cOsdItem("____________________________________________________________",osUnknown, false));
    Add(new cOsdItem("",osUnknown, false));

    if(IsCurrent(tr(NETCLIENT_MODE_TEXT))) {
        if(ClientMode_==ClientModeOld_)
        { // ClientMode unchanged
            if(ClientMode_==eModeStandalone) { // MultiRoom is disabled
                if(HasHarddisk_) {
                    Add(new cOsdItem(tr("Info:"),osUnknown, false));
                    AddFloatingText(tr("MultiRoom is inactive."), MAX_LETTER_PER_LINE);
                    AddFloatingText(tr("Connection to ReelBox Avantgarde is disabled. Recording will be done locally."), MAX_LETTER_PER_LINE);
                } else {
                    Add(new cOsdItem(tr("Warning:"),osUnknown, false));
                    AddFloatingText(tr("MultiRoom is disabled and no harddisk was found - only LiveTV is available!"), MAX_LETTER_PER_LINE);
                    AddFloatingText(tr("Recording and timeshift are disabled."), MAX_LETTER_PER_LINE);
                }
            } else { // MultiRoom is enabled
                if(DatabaseAvailable_) {
                    Add(new cOsdItem(tr("Info:"),osUnknown, false));
                    AddFloatingText(tr("MultiRoom is active. Recording will be done by the ReelBox Avantgarde."), MAX_LETTER_PER_LINE);
                } else {
                    //Add(new cOsdItem(tr("Error!"),osUnknown, false));
                    AddFloatingText(tr("Error: The ReelBox Avantgarde is not accessible now.\n"
                                "Possible reasons are: network hardware error, wrong network settings, Avantgarde is off.\n"
                                "Please check the system settings and the network or select a different recording server."), MAX_LETTER_PER_LINE);
                }
            }
        }
        else
        { // ClientMode modified
            if(ClientMode_==eModeStandalone) { // MultiRoom will be disabled
                if(HasHarddisk_) {
                    Add(new cOsdItem(tr("Info:"),osUnknown, false));
                    AddFloatingText(tr("If confirm now, MultiRoom will be deactivated."), MAX_LETTER_PER_LINE);
                    AddFloatingText(tr("Connection to the ReelBox Avantgarde will be disabled. Recording will be done locally."), MAX_LETTER_PER_LINE);
                } else {
                    Add(new cOsdItem(tr("Warning:"),osUnknown, false));
                    AddFloatingText(tr("If confirm now, MultiRoom will be disabled. No harddisk was found - only LiveTV will be available."), MAX_LETTER_PER_LINE);
                    AddFloatingText(tr("Recording and timeshift will be disabled."), MAX_LETTER_PER_LINE);
                }
            } else { // MultiRoom will be enabled
                if(DatabaseAvailable_) {
                    Add(new cOsdItem(tr("Info:"),osUnknown, false));
                    AddFloatingText(tr("If confirm now, MultiRoom will be activated. Recording will be done by the ReelBox Avantgarde."), MAX_LETTER_PER_LINE);
                } else {
                    //Add(new cOsdItem(tr("Error!"),osUnknown, false));
                    AddFloatingText(tr("Error: The ReelBox Avantgarde is not accessible now.\n"
                                "Possible reasons are: network hardware error, wrong network settings, Avantgarde is off.\n"
                                "Please check the system settings and the network or select a different recording server."), MAX_LETTER_PER_LINE);
                }
            }
        }
    } else if(IsCurrent(tr(RECEIVER_TYPE_TEXT))) {
        if(ReceiverType_ == eModeMcli)
            AddFloatingText(tr("DummyText RECEIVER_TYPE_TEXT MCLI$"), MAX_LETTER_PER_LINE);
        else if(ReceiverType_ == eModeStreamdev)
            AddFloatingText(tr("DummyText RECEIVER_TYPE_TEXT Streamdev$"), MAX_LETTER_PER_LINE);
    } else if(IsCurrent(tr(STREAM_SERVER_TEXT)) || IsCurrent(tr(RECORDING_SERVER_TEXT))) {
        if(ReelBoxes.size() && !DatabaseAvailable_) {
            //Add(new cOsdItem(tr("Error!"),osUnknown, false));
            AddFloatingText(tr("Error: The ReelBox Avantgarde is not accessible now.\n"
                                   "Possible reasons are: network hardware error, wrong network settings, Avantgarde is off.\n"
                                   "Please check the system settings and the network or select a different recording server."), MAX_LETTER_PER_LINE);
            Add(new cOsdItem("",osUnknown, false));
        }
        if(ReelBoxes.size()==0) {
            Add(new cOsdItem(tr("Error!"),osUnknown, false));
            AddFloatingText(tr("Please check the network settings and the network connection else please search in network."), MAX_LETTER_PER_LINE);
        } else {
            if(NetServer.IP.size()) {
                Add(new cOsdItem(tr("Info:"),osUnknown, false));
                snprintf(strBuff, 512, tr("The ReelBox Avantgarde with the IP address \"%s\" will be used for MultiRoom services."), NetServer.IP.c_str());
                AddFloatingText(strBuff, MAX_LETTER_PER_LINE);
            } else
                AddFloatingText(tr("Please select a ReelBox Avantgarde."), MAX_LETTER_PER_LINE);
        }
    }
}

eOSState cSetupNetClientMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    if(!HasSubMenu()) {
        // Check if DB is available
        if(Setup.ReelboxMode && !Setup.ReelboxModeTemp)
            DatabaseAvailable_ = false;
        else
            DatabaseAvailable_ = true;

        if(1 /*ReelBoxes*/) {
            currText = Get(Current()) && Get(Current())->Text()?std::string(Get(Current())->Text()):"";
            if ((Key == kRight) || (Key == kLeft))
            {
                if (ReceiverType_ == eModeDVB)  // we don't wanna see the DVB state
                    ReceiverType_ = eModeMcli;
                if (ReceiverType_ == eModeStreamdev && ! cPluginManager::GetPlugin("streamdev-client"))
                    ReceiverType_ = eModeMcli;
            }
            if(state == osUnknown) {
                if (Key == kRed && (IsCurrent(tr(STREAM_SERVER_TEXT)) || IsCurrent(tr(RECORDING_SERVER_TEXT)))) {
                        return AddSubMenu(new cSetupNetClientServerMenu(GetTitle(), &NetServer));
                } else
		if (Key == kBlue) {
                    if( ExpertSettings_ )
                        ExpertSettings_ = false;
                    else {
                        ExpertSettings_ = true;
                        SetTitle(tr("MultiRoom Setup (Expert)"));
                    }
                    Set();
                    currText = Get(Current()) && Get(Current())->Text()?std::string(Get(Current())->Text()):"";
                    state = osContinue;
                } else
		if (Key == kOk) {
                    if(NetServer.IP.size() || (ClientMode_==eModeStandalone)) {
                        //DDD("%s ReceiverType_: %d", __PRETTY_FUNCTION__, ReceiverType_);
                        Setup.ReceptionMode = ReceiverType_;
                        Skins.Message(mtStatus, tr("Storing settings - please wait..."));
                        Save();
                        Skins.Message(mtInfo, tr("Settings stored"));
                        Set();
                    } else
                        Skins.Message(mtError, tr("Invalid settings: Client without Server!"));
                    return osContinue;
                }
                else if (Key == kRed || Key == kGreen || Key == kYellow || Key == kBlue)
                    state = osContinue;
            } else if(state == os_User) { // Refresh menu
                Set();
                return osContinue;
            } else if(state == osBack) {
//                if(AVGChanged_ ||
                if ((ClientMode_ != ClientModeOld_) || (ReceiverType_ != ReceiverTypeOld_)
                       || (nrTuners.sat != nrTunersOld.sat) || (nrTuners.satS2 != nrTunersOld.satS2)
                       || (nrTuners.terr != nrTunersOld.terr) || (nrTuners.cable != nrTunersOld.cable)) {
                    if(NetServer.IP.size()) {
                        if(Interface->Confirm(tr("Save changes?"))) {
                            Skins.Message(mtStatus, tr("Storing settings - please wait..."));
                            Save();
                            Skins.Message(mtInfo, tr("Settings stored"));
                        }
                    }
                }
            } else if (Key == kNone) {
                state = osUnknown;
            }

            if ((Key == kUp) || (Key == kDown) || (Key == (kUp|k_Repeat)) || (Key == (kDown|k_Repeat)
                || (DatabaseAvailable_ != DatabaseAvailableOld_)))
                Set(); // Call AddInfo()

            if(IsCurrent(tr(STREAM_SERVER_TEXT)) || IsCurrent(tr(RECORDING_SERVER_TEXT)))
                SetHelp(ReelBoxes.size()?tr("Selection"):tr("search"), NULL, NULL, ExpertSettings_?tr("Normal"):tr("Expert"));
            else
                SetHelp(NULL, NULL, NULL, ExpertSettings_?tr("Normal"):tr("Expert"));
        } else {
            if(state==osUnknown) {
                if((Key == kOk) || (Key == kBack))
                    return osBack;
            }
        }
    } else if(state == osUser1) { // Close Submenu and refresh menu
        if(NetServer.IP.size()) {
            Skins.Message(mtStatus, tr("Storing settings - please wait..."));
            Save();
            Skins.Message(mtInfo, tr("Settings stored"));
        }
        CloseSubMenu();
        Set();
        Display();
        AVGChanged_ = true;
        return osContinue;
    }

    return state;
}

int cSetupNetClientMenu::GetFreeMountNumber()
{
    std::vector<int> usedNumbers;
    int freeNumber = 0;
    std::stringstream buffer;
    char *buffer2;
    FILE *process;
    FILE *process2;

    // Get Numbers of used NetworkMounts
    process = popen("grep HOST /etc/default/sysconfig |grep ^MOUNT| sed -e 's/MOUNT//' | awk -F'_' '{print $1 }'", "r");
    if(process)
    {
        cReadLine readline;
        buffer2 = readline.Read(process);
        while(buffer2)
        {
            usedNumbers.push_back(atoi(buffer2));
            buffer2 = readline.Read(process);
        }
        pclose(process);
    }
    sort( usedNumbers.begin(), usedNumbers.end());

    // Checks if the mount already exists
    for(unsigned int i = 0; i < usedNumbers.size(); ++i)
    {
        buffer << "grep MOUNT" << usedNumbers.at(i) << "_NFS_HOST /etc/default/sysconfig | grep %s" << Setup.NetServerIP;
        process = popen(buffer.str().c_str(), "r");
        if(process)
        {
            cReadLine readline;
            buffer2 = readline.Read(process);
            if(buffer2)
            {
                buffer.str("");
                buffer << "grep MOUNT" << usedNumbers.at(i) << "_NFS_SHARENAME /etc/default/sysconfig | grep /media/hd";
                process2 = popen(buffer.str().c_str(), "r");
                if(process2)
                {
                    cReadLine readline;
                    buffer2 = readline.Read(process2);
                    if(buffer2)
                        freeNumber = -1;
                    pclose(process2);
                }
            }
            pclose(process);
            if(freeNumber == -1)
                return -1;
        }
    }

    // check for gaps
    for(unsigned int i = 0; i < usedNumbers.size(); ++i)
        if (freeNumber == usedNumbers.at(i) )
            ++freeNumber;

    return freeNumber;
}

std::string cSetupNetClientMenu::GetUUID()
{
    std::string UUID;
    char command[64];
    FILE *file;
    char *buffer;

    sprintf(command, "ls -l /dev/disk/by-uuid/ | grep %s", HDDEVICE);
    file = popen(command, "r");
    if(file)
    {
        cReadLine readline;
        buffer = readline.Read(file);
        if(buffer)
        {
            char *ptr = strrchr(buffer, '>');
            if(ptr)
                *(ptr-2) = '\0';
            UUID = strrchr(buffer, ' ')+1;
        }
        else
            UUID = "";
        pclose(file);
    }

    return UUID;
}

void cSetupNetClientMenu::parseAVGsSysconfig(bool *hasNasAsMediaDir) {
    std::ifstream ifile("/tmp/configs/sysconfig");
    char buf[256];
    bool vlan = false;
    std::string vlan_str;
    std::string mediadev_str, mdev_only_rec_str;
    int mountNr = -1;
    bool isNFS = false;
    bool videos = false, recordings = false, pictures = false, music = false, softlock = false, optionrw = false, optionlock = false, startclient = false;
    std::string username, password, displayname, host, sharename, protocol;
    std::stringstream strBuff;
    int blkSize = 0;
    int nfsVersion = 3;

    while (ifile.good()) {
        ifile.getline(buf, 256);
        if (!strncmp(buf, "VLAN=", 5))
            vlan = (buf[6] == 'y');
        else if (!strncmp(buf, "VLAN_ID=", 8))
            vlan_str = buf + 8;
        else if (!strncmp(buf, "MEDIADEVICE=", 12))
            mediadev_str = buf + 12;
        else if (!strncmp(buf, "MEDIADEVICE_ONLY_RECORDING=", 28))
            mdev_only_rec_str = buf + 29;
    }
    ifile.close();

    unsigned int pos = mediadev_str.find("/media/.mount");
    if (pos != std::string::npos) {
        mountNr = atoi(mediadev_str.c_str()+14);
        *hasNasAsMediaDir = true;
    }
    ifile.open("/tmp/configs/sysconfig");
    while (ifile.good()) {
        ifile.getline(buf, 256);
        if (strlen(buf) > 10 && !strncmp(buf, "MOUNT", 5) && buf[5] == '0' + mountNr) {
            if (!strncmp(buf+7, "NFS", 3))
                isNFS = true;
            if (!strncmp(buf+11, "BLOCKSIZE", 9)) {
                blkSize = atoi(buf+22);
            } else if (!strncmp(buf+11, "DISPLAYNAME", 11)) {
                displayname = std::string(buf+24);
                displayname.erase(displayname.size()-1);
            } else if (!strncmp(buf+11, "HOST", 4)) {
                host = std::string(buf+17);
                host.erase(host.size()-1);
            } else if (!strncmp(buf+11, "MUSIC", 5)) {
                if (!strncmp(buf+18, "yes", 3))
                    music = true;
            } else if (!strncmp(buf+11, "NETWORKPROTOCOL", 15)) {
                protocol = std::string(buf+28);
                protocol.erase(protocol.size()-1);
            } else if (!strncmp(buf+11, "NFSVERSION", 10)) {
                nfsVersion = atoi(buf+23);
            } else if (!strncmp(buf+11, "OPTIONHS", 8)) {
                if (!strncmp(buf+21, "soft", 4))
                    softlock = true;
            } else if (!strncmp(buf+11, "OPTIONLOCK", 10)) {
                if (!strncmp(buf+23, "yes", 3))
                    optionlock = true;
            } else if (!strncmp(buf+11, "OPTIONRW", 8)) {
                if (!strncmp(buf+21, "yes", 3))
                    optionrw = true;
            } else if (!strncmp(buf+11, "PICTURES", 8)) {
                if (!strncmp(buf+21, "yes", 3))
                    pictures = true;
            } else if (!strncmp(buf+11, "RECORDINGS", 10)) {
                if (!strncmp(buf+23, "yes", 3))
                    recordings = true;
            } else if (!strncmp(buf+11, "SHARENAME", 9)) {
                sharename = std::string(buf+22);
                sharename.erase(sharename.size()-1);
            } else if (!strncmp(buf+11, "STARTCLIENT", 10)) {
                if (!strncmp(buf+24, "yes", 3))
                    startclient = true;
            } else if (!strncmp(buf+11, "VIDEOS", 6)) {
                if (!strncmp(buf+19, "yes", 3))
                    videos = true;
            } else if (!strncmp(buf+11, "USERNAME", 8)) {
                username = std::string(buf+21);
                username.erase(username.size()-1);
            } else if (!strncmp(buf+11, "PASSWORD", 8)) {
                password = std::string(buf+21);
                password.erase(password.size()-1);
            }
        }
    }
    ifile.close();
    cSysConfig_vdr &sysconfig = cSysConfig_vdr::GetInstance();

    if (mountNr != -1) {
        if (isNFS) {
            char strBuff2[16];
            int Number = 0;
            SystemExec("setup-shares stop 0");
            // Store NFS-Shares
            strBuff << "MOUNT" << Number << "_NFS_STARTCLIENT";
            sysconfig.SetVariable(strBuff.str().c_str(), startclient ? "yes" : "no");
            strBuff.str(""); strBuff << "MOUNT" << Number << "_NFS_HOST";
            sysconfig.SetVariable(strBuff.str().c_str(), host.c_str());
            strBuff.str(""); strBuff << "MOUNT" << Number << "_NFS_SHARENAME";
            sysconfig.SetVariable(strBuff.str().c_str(), sharename.c_str());
            strBuff.str(""); strBuff << "MOUNT" << Number << "_NFS_DISPLAYNAME";
            sysconfig.SetVariable(strBuff.str().c_str(), displayname.c_str());
            strBuff.str(""); strBuff << "MOUNT" << Number << "_NFS_PICTURES";
            sysconfig.SetVariable(strBuff.str().c_str(), pictures ? "yes" : "no");
            strBuff.str(""); strBuff << "MOUNT" << Number << "_NFS_MUSIC";
            sysconfig.SetVariable(strBuff.str().c_str(), music ? "yes" : "no");
            strBuff.str(""); strBuff << "MOUNT" << Number << "_NFS_RECORDINGS";
            sysconfig.SetVariable(strBuff.str().c_str(), recordings ? "yes" : "no");
            strBuff.str(""); strBuff << "MOUNT" << Number << "_NFS_VIDEOS";
            sysconfig.SetVariable(strBuff.str().c_str(), videos ? "yes" : "no");

            // NFS-specified options
            strBuff.str(""); strBuff << "MOUNT" << Number << "_NFS_NETWORKPROTOCOL";
            sysconfig.SetVariable(strBuff.str().c_str(), protocol.c_str());
            strBuff.str(""); strBuff << "MOUNT" << Number << "_NFS_BLOCKSIZE";
            snprintf(strBuff2, 16, "%i", blkSize);
            sysconfig.SetVariable(strBuff.str().c_str(), strBuff2);
            strBuff.str(""); strBuff << "MOUNT" << Number << "_NFS_OPTIONHS";
            sysconfig.SetVariable(strBuff.str().c_str(), softlock ? "soft" : "hard");
            strBuff.str(""); strBuff << "MOUNT" << Number << "_NFS_OPTIONLOCK";
            sysconfig.SetVariable(strBuff.str().c_str(), optionlock ? "yes" : "no");
            strBuff.str(""); strBuff << "MOUNT" << Number << "_NFS_OPTIONRW";
            sysconfig.SetVariable(strBuff.str().c_str(), optionrw ? "yes" : "no");
            strBuff.str(""); strBuff << "MOUNT" << Number << "_NFS_NFSVERSION";
            snprintf(strBuff2, 16, "%i", nfsVersion);
            sysconfig.SetVariable(strBuff.str().c_str(), strBuff2);

            // Use NFS-Mount as MediaDevice
            strBuff.str(""); strBuff << "/media/.mount" << Number;
            sysconfig.SetVariable("MEDIADEVICE", strBuff.str().c_str());
            if (mdev_only_rec_str.empty())
                sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");
            else
                sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", mdev_only_rec_str.c_str());
            sysconfig.Save();
            SystemExec("setup-shares start 0");
        } else {
            int Number = 0;
            SystemExec("setup-shares stop 0");
            // Store SMB-Shares
            strBuff << "MOUNT" << Number << "_SMB_STARTCLIENT";
            sysconfig.SetVariable(strBuff.str().c_str(), startclient ? "yes" : "no");
            strBuff.str(""); strBuff << "MOUNT" << Number << "_SMB_HOST";
            sysconfig.SetVariable(strBuff.str().c_str(), host.c_str());
            strBuff.str(""); strBuff << "MOUNT" << Number << "_SMB_SHARENAME";
            sysconfig.SetVariable(strBuff.str().c_str(), sharename.c_str());
            strBuff.str(""); strBuff << "MOUNT" << Number << "_SMB_DISPLAYNAME";
            sysconfig.SetVariable(strBuff.str().c_str(), displayname.c_str());
            strBuff.str(""); strBuff << "MOUNT" << Number << "_SMB_PICTURES";
            sysconfig.SetVariable(strBuff.str().c_str(), pictures ? "yes" : "no");
            strBuff.str(""); strBuff << "MOUNT" << Number << "_SMB_MUSIC";
            sysconfig.SetVariable(strBuff.str().c_str(), music ? "yes" : "no");
            strBuff.str(""); strBuff << "MOUNT" << Number << "_SMB_RECORDINGS";
            sysconfig.SetVariable(strBuff.str().c_str(), recordings ? "yes" : "no");
            strBuff.str(""); strBuff << "MOUNT" << Number << "_SMB_VIDEOS";
            sysconfig.SetVariable(strBuff.str().c_str(), videos ? "yes" : "no");

            // SMB-specified options
            strBuff.str(""); strBuff << "MOUNT" << Number << "_SMB_PASSWORD";
            sysconfig.SetVariable(strBuff.str().c_str(), password.c_str());
            strBuff.str(""); strBuff << "MOUNT" << Number << "_SMB_USERNAME";
            sysconfig.SetVariable(strBuff.str().c_str(), username.c_str());

            // Use SMB-Mount as MediaDevice
            strBuff.str(""); strBuff << "/media/.mount" << Number;
            sysconfig.SetVariable("MEDIADEVICE", strBuff.str().c_str());
            if (mdev_only_rec_str.empty())
                sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");
            else
                sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", mdev_only_rec_str.c_str());
            sysconfig.Save();
            SystemExec("setup-shares start 0");
        }
    }

    if (vlan) {
        sysconfig.SetVariable("VLAN", "yes");
        sysconfig.SetVariable("VLAN_ID", vlan_str.substr(1,1).c_str());
        sysconfig.Save();
        SystemExec("setup-net restart");
        cCondWait::SleepMs(1000); // wait a bit until the network is up
    } else {
        sysconfig.SetVariable("VLAN", "no");
        sysconfig.Save();
    }
}

void cSetupNetClientMenu::MountDefaultDir(std::string NetServerIP, int Number, bool hasNasAsMediaDir)
{
    if (!hasNasAsMediaDir) {
        printf("(%s:%d)\033[7;93mhasNasasmediadir?%d\033[0m (%s, num:%d)\n", 
               __FILE__, __LINE__, hasNasAsMediaDir, NetServerIP.c_str(), Number);
        cSysConfig_vdr &sysconfig = cSysConfig_vdr::GetInstance();
    	char strBuff[512];	        
        if((Number != -1) && NetServerIP.size()) {
            // Store the NFS-Shares
            sprintf(strBuff, "MOUNT%i_NFS_STARTCLIENT", Number);
            sysconfig.SetVariable(strBuff, "yes");
            sprintf(strBuff, "MOUNT%i_NFS_HOST", Number);
            sysconfig.SetVariable(strBuff, NetServerIP.c_str());
            sprintf(strBuff, "MOUNT%i_NFS_SHARENAME", Number);
            sysconfig.SetVariable(strBuff, "/media/hd");
            sprintf(strBuff, "MOUNT%i_NFS_DISPLAYNAME", Number);
            sysconfig.SetVariable(strBuff, "Avantgarde");
            sprintf(strBuff, "MOUNT%i_NFS_PICTURES", Number);
            sysconfig.SetVariable(strBuff, "yes");
            sprintf(strBuff, "MOUNT%i_NFS_MUSIC", Number);
            sysconfig.SetVariable(strBuff, "yes");
            sprintf(strBuff, "MOUNT%i_NFS_RECORDINGS", Number);
            sysconfig.SetVariable(strBuff, "yes");
            sprintf(strBuff, "MOUNT%i_NFS_VIDEOS", Number);
            sysconfig.SetVariable(strBuff, "yes");

            // NFS-specified options
            sprintf(strBuff, "MOUNT%i_NFS_NETWORKPROTOCOL", Number);
            sysconfig.SetVariable(strBuff, "tcp");
            sprintf(strBuff, "MOUNT%i_NFS_BLOCKSIZE", Number);
            sysconfig.SetVariable(strBuff, "8192");
            sprintf(strBuff, "MOUNT%i_NFS_OPTIONHS", Number);
            sysconfig.SetVariable(strBuff, "soft");
            sprintf(strBuff, "MOUNT%i_NFS_OPTIONLOCK", Number);
            sysconfig.SetVariable(strBuff, "yes");
            sprintf(strBuff, "MOUNT%i_NFS_OPTIONRW", Number);
            sysconfig.SetVariable(strBuff, "yes");
            sprintf(strBuff, "MOUNT%i_NFS_NFSVERSION", Number);
            sysconfig.SetVariable(strBuff, "3");

            // Use NFS-Mount as MediaDevice
            sprintf(strBuff, "/media/.mount%i", Number);
            sysconfig.SetVariable("MEDIADEVICE", strBuff);
            sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");

            sysconfig.Save();

            sprintf(strBuff, "setup-shares start %i &", Number);
            SystemExec(strBuff);
        } else {
            // Use NFS-Mount as MediaDevice
            sprintf(strBuff, "/media/.mount%i", Number);
            sysconfig.SetVariable("MEDIADEVICE", strBuff);
            sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");
            //snprintf(strBuff, 512, "/media/.mount0/recordings");
            //VideoDirectory = strdup(strBuff);
            sysconfig.Save();
        }
    }
}

bool cSetupNetClientMenu::Save()
{
    cSysConfig_vdr &sysconfig = cSysConfig_vdr::GetInstance();
    sysconfig.Load("/etc/default/sysconfig");
    bool save = true;

    if(ClientMode_ == eModeStandalone) {
        Setup.ReelboxMode = Setup.ReelboxModeTemp = eModeStandalone; // 0=Standalone => use local Harddisk

        if(HasHarddisk_) {
            // Set UUID for Recording Media
            std::string UUID = GetUUID();
            if(UUID.size()) {
                // Use local harddisk
                char buf[256];
                snprintf(buf, 256, "UUID=%s", UUID.c_str());
                sysconfig.SetVariable("MEDIADEVICE", buf);
                sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");
            } else {
                Skins.Message(mtError, "Storing failed! (missing UUID)");
                save = false;
            }
        } else {
            sysconfig.SetVariable("MEDIADEVICE", "");
            sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");
        }
    } else {
        Setup.ReelboxMode = eModeClient; // 1=Client => use Database

        std::string cmd = std::string("getConfigsFromAVGServer.sh ") + NetServer.IP + std::string(" sysconfig ; ");
        SystemExec(cmd.c_str());
        bool hasNasAsMediaDir = false;
        parseAVGsSysconfig(&hasNasAsMediaDir);

        DDD("\033[0;92m Storing recordings on the AVG! \033[0;m");
        int Number = 0; // use #0 (reserved for NetClient-AVG-Mount)
        if (ClientMode_ == eModeClient) {
            if (WakeAVG_)
                sysconfig.SetVariable("WOL_WAKEUP_SERVER", "true");
            else
                sysconfig.SetVariable("WOL_WAKEUP_SERVER", "false");
        } else
            sysconfig.SetVariable("WOL_WAKEUP_SERVER", "false");

        MountDefaultDir(NetServer.IP, Number, hasNasAsMediaDir);
        SystemExec("setup-mediadir");
        // Recordings.Update(false);
    }

    //NetServerIP
    if(NetServer.IP.size())
        strcpy(Setup.NetServerIP, NetServer.IP.c_str());
    else
        strcpy(Setup.NetServerIP, "");
    //NetServerName
    if(NetServer.Name.size())
        strcpy(Setup.NetServerName, NetServer.Name.c_str());
    else
        strcpy(Setup.NetServerName, "");

    //NetServerMAC
    if(GetNetserverMAC(&NetServer))
        strcpy(Setup.NetServerMAC, NetServer.MAC.c_str());
    else
        strcpy(Setup.NetServerMAC, "");

    if(ClientMode_ == eModeClient) {
        //check and set ReelboxModeTemp
        bool DatabaseAvailable = false;
        cPlugin *p = cPluginManager::GetPlugin("avahi");
        if(p)
            p->Service("Avahi check NetServer", &DatabaseAvailable);
        Setup.ReelboxModeTemp = DatabaseAvailable?eModeClient:eModeStandalone;
    }

    // Load Timers if necessary
    if(ClientMode_ == eModeClient) {
        Timers.Load(); // Clear Timers
#ifdef USEMYSQL
        cPluginManager::CallAllServices("MultiRoom disable");
        cPluginManager::CallAllServices("MultiRoom enable");
        Timers.LoadDB();
#endif
    } else if(ClientMode_ == eModeStandalone) {
        if(ClientModeOld_ != ClientMode_)
            Timers.Load("/etc/vdr/timers.conf");
    }

    if(ReceiverType_ == eModeStreamdev) {
        //Streamdev-Client settings
        if(NetServer.IP.size()) {
            DDD("\033[0;92m Use NetServer! \033[0;m");
            // Setup Streamdev-Client
            if (cPlugin *StreamdevPlugin = cPluginManager::GetPlugin("streamdev-client")) {
                StreamdevPlugin->SetupStore("RemoteIp", NetServer.IP.c_str());
                StreamdevPlugin->SetupParse("RemoteIp", NetServer.IP.c_str());
                StreamdevPlugin->SetupStore("StartClient", 1);
                StreamdevPlugin->SetupParse("StartClient", "1");
                StreamdevPlugin->SetupStore("SyncEPG", 1);
                StreamdevPlugin->SetupParse("SyncEPG", "1");
                StreamdevPlugin->Service("Streamdev Client Reinit", NULL);
            }
        } else {
            DDD("\033[0;92m No NetServer! \033[0;m");
            strcpy(Setup.NetServerIP, "");
            strcpy(Setup.NetServerName, "");
            strcpy(Setup.NetServerMAC, "");

            // Disable Streamdev-Client
            if (cPlugin *StreamdevPlugin = cPluginManager::GetPlugin("streamdev-client")) {
                StreamdevPlugin->SetupStore("RemoteIp", "");
                StreamdevPlugin->SetupParse("RemoteIp", "");
                StreamdevPlugin->SetupStore("StartClient", 0);
                StreamdevPlugin->SetupParse("StartClient", "0");
                StreamdevPlugin->Service("Streamdev Client Reinit", NULL);
            }
        }
    } else if(ReceiverType_ == eModeMcli) {
        // Disable Streamdev-Client
        if (cPlugin *StreamdevPlugin = cPluginManager::GetPlugin("streamdev-client")) {
            StreamdevPlugin->SetupStore("RemoteIp", "");
            StreamdevPlugin->SetupParse("RemoteIp", "");
            StreamdevPlugin->SetupStore("StartClient", 0);
            StreamdevPlugin->SetupParse("StartClient", "0");
            StreamdevPlugin->Service("Streamdev Client Reinit", NULL);
        }
    }

    StoreMcliConf(&nrTuners, ReceiverType_); // Write settings to /etc/default/mcli

    if(save) {
        AVGChanged_ = false;
        ClientModeOld_ = ClientMode_;
        WakeAVGOld_ = WakeAVG_;
        ReceiverTypeOld_ = ReceiverType_;
        nrTunersOld.sat   = nrTuners.sat;
        nrTunersOld.satS2 = nrTuners.satS2;
        nrTunersOld.terr  = nrTuners.terr;
        nrTunersOld.cable = nrTuners.cable;
        sysconfig.Save();
        if(ClientMode_ == eModeStandalone) {
            // Hint: do this *after* sysconfig.Save()!
            // Stop & Remove NetClient-AVG-Mount
            SystemExec("setup-shares stop 0");
            SystemExec("sed -i \"/^MOUNT0_/d\" /etc/default/sysconfig");
            // TODO: If we use vdr-sysconfig, then do cSysConfig_vdr::Load() here!
            //VideoDirectory = strdup("/media/reel/recordings");
            Recordings.Update(false);
        }
        Setup.Save();
    }
    return save;
}

void cSetupNetClientMenu::LoadMcliConf()
{
    char *buffer;
    std::string strBuff;
    FILE *process = NULL;

    process = fopen("/etc/default/mcli", "r");
    if(process) {
        cReadLine readline;
        buffer = readline.Read(process);
        while(buffer) {
            strBuff = buffer;
#if 0
            if(strstr(buffer, "MCLI_ENABLED=\"") && !strstr(buffer, "\"\"")) {
                if(strstr(buffer, "true"))
                    ReceiverType_ = eModeMcli;
                else
                    ReceiverType_ = eModeStreamdev;
            }
#endif
            if(strstr(buffer, "DVB_C_DEVICES=\"") && !strstr(buffer, "\"\""))
                nrTuners.cable = atoi(buffer+15);
            if(strstr(buffer, "DVB_S_DEVICES=\"") && !strstr(buffer, "\"\""))
                nrTuners.sat   = atoi(buffer+15);
            if(strstr(buffer, "DVB_S2_DEVICES=\"") && !strstr(buffer, "\"\""))
                nrTuners.satS2 = atoi(buffer+16);
            if(strstr(buffer, "DVB_T_DEVICES=\"") && !strstr(buffer, "\"\""))
                nrTuners.terr  = atoi(buffer+15);

            mclifile.push_back(strBuff);
            buffer = readline.Read(process);
        }
        fclose(process);
    }

#ifdef DEBUG_SETUP
    for(unsigned int i=0; i<mclifile.size(); ++i)
        printf("\033[0;93m %s \033[0m\n", mclifile.at(i).c_str());
#endif
}

void cSetupNetClientMenu::StoreMcliPlugParams(nrTuners_t *nrTuners) {
    if (cPlugin *McliPlugin = cPluginManager::GetPlugin("mcli")) {
        std::stringstream buf;
        buf << nrTuners->cable;
        McliPlugin->SetupStore("DVB-C", nrTuners->cable);
        McliPlugin->SetupParse("DVB-C", buf.str().c_str());
        buf.str(""); buf << nrTuners->sat;
        McliPlugin->SetupStore("DVB-S", nrTuners->sat);
        McliPlugin->SetupParse("DVB-S", buf.str().c_str());
        buf.str(""); buf << nrTuners->terr;
        McliPlugin->SetupStore("DVB-T", nrTuners->terr);
        McliPlugin->SetupParse("DVB-T", buf.str().c_str());
        buf.str(""); buf << nrTuners->satS2;
        McliPlugin->SetupStore("DVB-S2", nrTuners->satS2);
        McliPlugin->SetupParse("DVB-S2", buf.str().c_str());
    } else
        esyslog("ERROR: couldn't get the mcli-plugin!");
}

void cSetupNetClientMenu::StoreMcliConf(nrTuners_t *nrTuners, eReceiverType ReceiverType_) // temporary till mcli-plugin is released
{
    StoreMcliPlugParams(nrTuners);

    std::ifstream ifile("/etc/default/mcli");
    char buf[256];
    std::stringstream lineBuf;
    while (ifile.good()) {
        ifile.getline(buf, 256);
        if(!strncmp(buf, "NETWORK_INTERFACE=", 18) || !strncmp(buf, "MCLI_ENABLED=", 13)) {
            lineBuf << buf << std::endl;
        }
    }
    ifile.close();

    std::ofstream file("/etc/default/mcli", std::ios_base::trunc);

    //file << "MCLI_ENABLED=\"" << ReceiverType_==eModeMcli?"true":"false" << "\"" << std::endl;
    if(lineBuf.str().size()) file << lineBuf.str();
    file << "DVB_C_DEVICES=\""  << nrTuners->cable << "\"" << std::endl;
    file << "DVB_S_DEVICES=\""  << nrTuners->sat   << "\"" << std::endl;
    file << "DVB_S2_DEVICES=\"" << nrTuners->satS2 << "\"" << std::endl;
    file << "DVB_T_DEVICES=\""  << nrTuners->terr  << "\"" << std::endl;

    file.close();

    cPlugin *mcliPlugin = cPluginManager::GetPlugin("mcli");
    if (mcliPlugin) {
        mcliPlugin->Service("Reinit");
    } else {
        if(ReceiverType_ == eModeMcli)
            SystemExec("/etc/init.d/mcli restart");
        else
            SystemExec("/etc/init.d/mcli stop");
    }
}

bool cSetupNetClientMenu::IsCurrent(std::string text)
{
	if (currText.size())
		return currText.find(text) == 0;
	else
		return text.find(tr(NETCLIENT_MODE_TEXT)) == 0;
}

//#########################################################################
// cSetupNetClientServerMenu
//#########################################################################
cSetupNetClientServerMenu::cSetupNetClientServerMenu(const char *title, server_t *NetServer) : cOsdMenu(title)
{
    DDD("\033[0;41m %s(%i): %s \033[0m", __FILE__, __LINE__, __PRETTY_FUNCTION__);
    NetServer_ = NetServer;

    Skins.Message(mtStatus, tr("Please wait..."));

    // Gather infos about Network
    cNetInfo netinfo;
    int i=0;
    bool found = false;
    while(i<netinfo.IfNum() && !found)
    {
        if(strcmp(netinfo[i]->IfName(), "eth0") == 0)
        {
            int Ipaddress = ConvertIp2Int(netinfo[i]->IpAddress());
            Netmask_ = ConvertIp2Int(netinfo[i]->NetMask());
            Network_ = Ipaddress & Netmask_;
            found=true;
        }
    }

    // Init pointer to ReelBoxes of avahi-plugin
    cPlugin *p=cPluginManager::GetPlugin("avahi");
    if(p) {
        p->Service("Avahi MultiRoom-List", &ReelBoxes);
    }

    ScanNetwork();
    Skins.Message(mtStatus, NULL);
    Set();
}

cSetupNetClientServerMenu::~cSetupNetClientServerMenu()
{
    vHosts_.clear();
}

void cSetupNetClientServerMenu::ScanNetwork()
{
    DDD("\033[0;41m %s(%i): %s \033[0m", __FILE__, __LINE__, __PRETTY_FUNCTION__);
    vHosts_.clear();
    ReelBoxes.clear();

    // Get ReelBox-List from avahi-plugin
    cPlugin *p=cPluginManager::GetPlugin("avahi");
    if(p) {
        p->Service("Avahi MultiRoom-List", &ReelBoxes);
    } else {
        printf("ERROR: could not get the avahi-Plugin, menu will be broken\n");
        esyslog("ERROR: could not get the avahi-Plugin, menu will be broken\n");
    }

    // Refresh local ReelBox-List
    unsigned int i = 0;
    for(i=0; i<ReelBoxes.size(); i++) {
        if(IsLocalNetwork(ReelBoxes.at(i).Ip.c_str()))
        {
            server_t newHost = { ReelBoxes.at(i).Ip, ReelBoxes.at(i).Name };
            vHosts_.push_back(newHost);
        }
    }
    // If there is only 1 AVG, then call it "ReelBox Avantgarde"
    if(vHosts_.size()==1)
        vHosts_.at(0).Name = "ReelBox Avantgarde";
    cCondWait::SleepMs(2000);
}

bool cSetupNetClientServerMenu::IsLocalNetwork(const char *Ip)
{
    int IpInt = ConvertIp2Int(Ip);
    int Network = IpInt & Netmask_;

    if(Network == Network_)
        return true;
    else
        return false;
}

int cSetupNetClientServerMenu::ConvertIp2Int(const char *Ip)
{
    int retValue = 0;
    char *index = (char*)Ip;

    for(int i=0; i<4; ++i)
    {
        retValue = retValue*0x100+atoi(index);
        index = strchr(index, '.')+1;
    }
    return retValue;
}

void cSetupNetClientServerMenu::Set()
{
    DDD("\033[0;41m %s(%i): %s \033[0m", __FILE__, __LINE__, __PRETTY_FUNCTION__);
    int current = Current();
    Clear();
    std::string buffer;

    if(vHosts_.size()) {
        if(vHosts_.size()==1)
            AddFloatingText(tr("Following ReelBox Avantgarde is found in network."), MAX_LETTER_PER_LINE);
        else
            AddFloatingText(tr("Following ReelBox Avantgarde are found in network."), MAX_LETTER_PER_LINE);
        Add(new cOsdItem("", osUnknown, false));
        AddFloatingText(tr("Please select a ReelBox Avantgarde for MultiRoom services:"), MAX_LETTER_PER_LINE);
        Add(new cOsdItem("", osUnknown, false));

        hostnameStart = Count();
        for(unsigned int i=0; i<vHosts_.size(); ++i) {
            buffer = vHosts_.at(i).Name + " (" + vHosts_.at(i).IP + ")";
            Add(new cOsdItem(buffer.c_str(), osUnknown, true));
        }
    } else {
        Add(new cOsdItem(tr("Note:"), osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        AddFloatingText(tr("No ReelBox Avantgarde found in network."), MAX_LETTER_PER_LINE);
    }

    SetHelp(tr("Rescan"));
    SetCurrent(Get(current));
    Display();
}

eOSState cSetupNetClientServerMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    if(!HasSubMenu()) {
        if(state == osUnknown) {
            switch (Key) {
              case kOk:
                {
                if(vHosts_.size()) {
                    if(vHosts_.size() == 1) {
                        *NetServer_ = vHosts_.at(0);
                    } else if(vHosts_.size()>1) {
                        int host_index = Current() - hostnameStart;
#ifdef DEBUG_SETUP
                        if (host_index < 0 || host_index > (int)vHosts_.size()) printf("\033[1;92m Error: Out of range \033[0m\n");

                        printf("Current()->Text '%s'\n", Get(Current())->Text());
                        printf("vhostnames.at(%i) '%s'\n", Current() - hostnameStart, vHosts_.at(host_index).Name.c_str());
                        printf("vhosts.at(%i) '%s'\n", Current() - hostnameStart, vHosts_.at(host_index).IP.c_str());
#endif
                        NetServer_->IP = vHosts_.at(host_index).IP;
                        NetServer_->Name = vHosts_.at(host_index).Name;
                    }
                    return osUser1; // Close menu and refresh parent-menu
                }
                break;
                }
              case kRed:
                {
                Skins.Message(mtStatus, tr("Please wait..."));
                Clear();
                cPlugin *p=cPluginManager::GetPlugin("avahi");
                if(p)
                    p->Service("Avahi Restart");
                Display();
                ScanNetwork();
                Skins.Message(mtStatus, NULL);
                Set();
                return osContinue;
                }
              default:
                break;
            }
        }
    }

    return state;
}

//#########################################################################
// cSetupNetClientMissingServerMenu
//#########################################################################
cSetupNetClientMissingServerMenu::cSetupNetClientMissingServerMenu(const char *title) : cOsdMenu(title)
{
    NetServer.IP = Setup.NetServerIP;
    NetServer.Name = Setup.NetServerName;
    NetServer.MAC = Setup.NetServerMAC;
    Set();
}

cSetupNetClientMissingServerMenu::~cSetupNetClientMissingServerMenu()
{
}

void cSetupNetClientMissingServerMenu::Set()
{
    const int strlength=128;
    char strBuff[strlength];

    SetHasHotkeys();

    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem(tr("Note:"), osUnknown, false));

    /*
    if (Setup.NetServerIP)
        snprintf(strBuff, strlength, tr("ReelBox Avantgarde with following IP-address %s is not found in network!"), Setup.NetServerIP);
    else
        snprintf(strBuff, strlength, tr("No ReelBox Avantgarde is selected!"));
    AddFloatingText(strBuff, MAX_LETTER_PER_LINE);
    */

    if (Setup.NetServerIP)
    {
        snprintf(strBuff, strlength, tr("The ReelBox Avantgarde (%s) which is currently configured as MultiRoom Server "
                                        "was not found on the network."), Setup.NetServerIP);
        AddFloatingText(strBuff, MAX_LETTER_PER_LINE);
        AddFloatingText(tr("If your ReelBox Avantgarde is configured to use DHCP, we strongly recommend "
                           "using fixed IP addresses."), MAX_LETTER_PER_LINE);
    }
    else
    {
        AddFloatingText(tr("No ReelBox Avantgarde is currently configured."), MAX_LETTER_PER_LINE);
    }
    Add(new cOsdItem("", osUnknown, false));
    //AddFloatingText(tr("MultiRoom is temporarily disabled."), MAX_LETTER_PER_LINE);
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem(hk(tr("Search for ReelBox Avantgarde on the network")), osUser2, true));
    Add(new cOsdItem(hk(tr("Continue with MultiRoom deactivated")), osEnd, true));
    Display();
}

eOSState cSetupNetClientMissingServerMenu::ProcessKey(eKeys Key)
{
    bool HadSubMenu = HasSubMenu();
    eOSState state = cOsdMenu::ProcessKey(Key);

    // No return to this menu
    if(HadSubMenu && !HasSubMenu())
        return osEnd;

    if(!HasSubMenu())
    {
        switch(state)
        {
            case osUser2:
                return AddSubMenu(new cSetupNetClientServerMenu(tr("MultiRoom Setup"), &NetServer));
                break;
            default:
                return state;
        }
    }
    else
    {
        if(state==osUser1) // Close Submenu and refresh menu
        {
            if(Key == kOk)
            {
                if(NetServer.IP.size())
                {
                    Skins.Message(mtStatus, tr("Storing settings - please wait..."));
                    Save();
                    Skins.Message(mtInfo, tr("Settings stored"));
                    CloseSubMenu();
                    return osEnd;
                }
                else
                    Skins.Message(mtError, tr("Invalid settings: Client without Server!"));
            }
        }
    }
    return state;
}

bool cSetupNetClientMissingServerMenu::Save()
{
    cSysConfig_vdr &sysconfig = cSysConfig_vdr::GetInstance();
    sysconfig.Load("/etc/default/sysconfig");

    Setup.ReelboxMode = eModeClient; // Client => use Database

    DDD("\033[0;92m Storing recordings on the AVG! \033[0;m");
    char strBuff[512];
    // int Number = GetFreeMountNumber();
    int Number = 0; // use #0 (reserved for NetClient-AVG-Mount)

    if((Number != -1) && NetServer.IP.size()) {
        // Store the NFS-Shares
        sprintf(strBuff, "MOUNT%i_NFS_STARTCLIENT", Number);
        sysconfig.SetVariable(strBuff, "yes");
        sprintf(strBuff, "MOUNT%i_NFS_HOST", Number);
        sysconfig.SetVariable(strBuff, NetServer.IP.c_str());
        sprintf(strBuff, "MOUNT%i_NFS_SHARENAME", Number);
        sysconfig.SetVariable(strBuff, "/media/hd");
        sprintf(strBuff, "MOUNT%i_NFS_DISPLAYNAME", Number);
        sysconfig.SetVariable(strBuff, "Avantgarde");
        sprintf(strBuff, "MOUNT%i_NFS_PICTURES", Number);
        sysconfig.SetVariable(strBuff, "yes");
        sprintf(strBuff, "MOUNT%i_NFS_MUSIC", Number);
        sysconfig.SetVariable(strBuff, "yes");
        sprintf(strBuff, "MOUNT%i_NFS_RECORDINGS", Number);
        sysconfig.SetVariable(strBuff, "yes");
        sprintf(strBuff, "MOUNT%i_NFS_VIDEOS", Number);
        sysconfig.SetVariable(strBuff, "yes");

        // NFS-specified options
        sprintf(strBuff, "MOUNT%i_NFS_NETWORKPROTOCOL", Number);
        sysconfig.SetVariable(strBuff, "tcp");
        sprintf(strBuff, "MOUNT%i_NFS_BLOCKSIZE", Number);
        sysconfig.SetVariable(strBuff, "8192");
        sprintf(strBuff, "MOUNT%i_NFS_OPTIONHS", Number);
        sysconfig.SetVariable(strBuff, "soft");
        sprintf(strBuff, "MOUNT%i_NFS_OPTIONLOCK", Number);
        sysconfig.SetVariable(strBuff, "yes");
        sprintf(strBuff, "MOUNT%i_NFS_OPTIONRW", Number);
        sysconfig.SetVariable(strBuff, "yes");
        sprintf(strBuff, "MOUNT%i_NFS_NFSVERSION", Number);
        sysconfig.SetVariable(strBuff, "3");

        // Use NFS-Mount as MediaDevice
        sprintf(strBuff, "/media/.mount%i", Number);
        sysconfig.SetVariable("MEDIADEVICE", strBuff);
        sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");

        sysconfig.Save();

        sprintf(strBuff, "setup-shares start %i &", Number);
        SystemExec(strBuff);
    } else {
        // Use NFS-Mount as MediaDevice
        sprintf(strBuff, "/media/.mount%i", Number);
        sysconfig.SetVariable("MEDIADEVICE", strBuff);
        sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");
        /*snprintf(strBuff, 512, "/media/.mount0/recordings");
          VideoDirectory = strdup(strBuff);
         */
        sysconfig.Save();
    }
    SystemExec("setup-mediadir");
    // Recordings.Update(false);

    //NetServerIP
    if(NetServer.IP.size())
        strcpy(Setup.NetServerIP, NetServer.IP.c_str());
    else
        strcpy(Setup.NetServerIP, "");
    //NetServerName
    if(NetServer.Name.size())
        strcpy(Setup.NetServerName, NetServer.Name.c_str());
    else
        strcpy(Setup.NetServerName, "");

    //NetServerMAC
    if(GetNetserverMAC(&NetServer))
        strcpy(Setup.NetServerMAC, NetServer.MAC.c_str());
    else
        strcpy(Setup.NetServerMAC, "");

    //check and set ReelboxModeTemp
    bool DatabaseAvailable = false;
    cPlugin *p = cPluginManager::GetPlugin("avahi");
    if(p)
        p->Service("Avahi check NetServer", &DatabaseAvailable);
    Setup.ReelboxModeTemp = DatabaseAvailable?eModeClient:eModeStandalone;

    // Load Timers if necessary
    Timers.Load(); // Clear Timers
#ifdef USEMYSQL
    cPluginManager::CallAllServices("MultiRoom disable");
    cPluginManager::CallAllServices("MultiRoom enable");
    Timers.LoadDB();
#endif

//  Do we need it?
//    StoreMcliConf(&nrTuners, ReceiverType_); // Write settings to /etc/default/mcli

    sysconfig.Save();
    Setup.Save();

    return true;
}

bool GetNetserverMAC(server_t *NetServer)
{
    char command[64];
    char *buffer;
    char *index = NULL;
    FILE *process;

    // first ping, since arping is not available on NetClient now
    snprintf(command, 64, "ping -c 1 -q %s >/dev/null 2>/dev/null", NetServer->IP.c_str());
    SystemExec(command);

    snprintf(command, 64, "arp -an %s 2>/dev/null", NetServer->IP.c_str());
    process = popen(command, "r");
    if(process) {
        cReadLine readline;
        buffer = readline.Read(process);
        if(buffer) {
            index = strchr(buffer, ':');
            if(index && (strlen(index) >= 14) && (*(index+3)==':') && (*(index+6)==':') && (*(index+9)==':') && (*(index+12)==':')) {
                *(index+15) = '\0';
                index = index - 2;
            } else
                index = NULL;
        }
        pclose(process);
    } else
        return false;

    if(index) {
        NetServer->MAC = index;
        return true;
    } else
        return false;
}

