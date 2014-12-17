/***************************************************************************
 *   Copyright (C) 2007 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                                                                         *
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
 * netcvdiseqcmenu.c
 *
 ***************************************************************************/

#include "netcvdiseqcmenu.h"
#include "netcvdiag.h"

// class cNetCVDisqecMenu----------------------------------------------
cNetCVDiseqcMenu::cNetCVDiseqcMenu()
    : cOsdMenu(tr("DiSEqC settings"))
{
    cPlugin *NetcvPlugin = cPluginManager::GetPlugin("netcv");
    if (NetcvPlugin)
    {
        struct {
            bool *bRunningUpdate;
        } data;

        NetcvPlugin->Service("Netceiver update status", &data);
        if (*data.bRunningUpdate)
        {
            Add(new cOsdItem(tr("NetCeiver is being updated."), osUnknown, false));
            Add(new cOsdItem(tr("Try again later."), osUnknown, false));
            Display();
        }
        else
        {
            directory_ = "/etc/reel/netceiver";
            netcvState_ = eInit;
            Skins.Message(mtStatus, tr("Please wait..."));

            // Run netcvdiag to get informations about netceivers & tuners
            cNetCVDiag *netcvdiag = new cNetCVDiag();

            int NetceiverCount = netcvdiag->GetNetceiverCount();
            if(NetceiverCount < 1)
                netcvState_ = eError;

            netcvdevices_ = new cNetCVDevices(NULL, NULL);

            if (netcvState_ != eError && netcvdiag->GetInformation(netcvdevices_))
            {
                currentnetceiver_ = netcvdevices_->Begin();
                netcvState_ = eDiagnose;

                DIR *dp;
                struct dirent * dirp;
                if ((dp = opendir(directory_.c_str())) == NULL)
                    SetErrorPage(1);
                else
                {
                    while ((dirp = readdir(dp)) != NULL)
                        if (std::string(dirp->d_name).rfind(".xml") == std::string(dirp->d_name).length()-4)
                            files_.push_back(std::string(dirp->d_name));
                    closedir(dp);
                    if (files_.size() == 0)
                        SetErrorPage(2);
                    else
                        Set();
                }
            }
            else
                netcvState_ = eError;
        }
    }


}

cNetCVDiseqcMenu::~cNetCVDiseqcMenu()
{
}

void cNetCVDiseqcMenu::SetErrorPage(int error)
{
    std::string strBuff = tr("Error!");
    Add(new cOsdItem(strBuff.c_str(), osUnknown, false));
    switch (error)
    {
        case 1:
            strBuff = tr("Failed to open directory ");
            strBuff.append(directory_);
            Add(new cOsdItem(strBuff.c_str(), osUnknown, false));
            break;
        case 2:
            strBuff = tr("No file found in ");
            strBuff.append(directory_);
            Add(new cOsdItem(strBuff.c_str(), osUnknown, false));
            break;
        default:
            break;
    }
}

void cNetCVDiseqcMenu::Set()
{
    std::string strBuff;
    Clear();

// Commented out for install wizard; We expect only 1 netceiver (builtin reelbox)
//    Add(new cNetCVCurrentDeviceItem(&currentnetceiver_, netcvdevices_->Begin(), netcvdevices_->End()));
//    Add(new cOsdItem("", osUnknown, false));

    strBuff = tr("Please select a setting for upload: ");
    Add(new cOsdItem(strBuff.c_str(), osUnknown, false));

    for (std::vector<std::string>::iterator File = files_.begin(); File != files_.end(); ++File)
        Add(new cNetCVDiseqcItem((*currentnetceiver_)->GetUUID(), directory_, *File, this));
}

eOSState cNetCVDiseqcMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (netcvState_ != eRestart)
    {
        cNetCVDiseqcItem *diseqcitem = dynamic_cast<cNetCVDiseqcItem*>(Get(Current()));
        if (diseqcitem)
            SetHelp(tr("Save"));
        else
            SetHelp(NULL);
    }

    if(netcvState_ == eDone)
        return osBack;

    if (state == os_User)
    {
        Set();
        Display();
        return osContinue;
    }
    else
        return state;
}

// class cNetCVDisqecItem----------------------------------------------
cNetCVDiseqcItem::cNetCVDiseqcItem(std::string uuid, std::string directory, std::string filename, cNetCVDiseqcMenu *menu)
{
    std::string strBuff;
    int index;
    uuid_ = uuid;
    directory_ = directory;
    filename_ = filename;
    menu_ = menu;
    strBuff = "  - ";
    strBuff.append(filename);
    index = strBuff.rfind(".");
    strBuff.assign(strBuff, 0, index);
    SetText(strBuff.c_str());
}

cNetCVDiseqcItem::~cNetCVDiseqcItem()
{
}

int cNetCVDiseqcItem::Upload()
{
    std::string strBuff = "netcvupdate -i ";
    strBuff.append(uuid_);
    strBuff.append(" -U ");
    strBuff.append(directory_);
    strBuff.append("/");
    strBuff.append(filename_);
    strBuff.append(" -K");
    return SystemExec(strBuff.c_str());
}

void cNetCVDiseqcItem::Restart(bool bRestartVDR)
{
	Skins.Message(mtStatus, tr("Restarting NetCeiver..."));
    sleep(20);
    SystemExec("killall -HUP mcli");
/*
    if (bRestartVDR)
    {
        Skins.Message(mtStatus, tr("Restarting VDR..."));
        SystemExec("killall -HUP vdr"); // dirty hack
    }
*/
    Skins.Message(mtStatus, tr("Finished!"));
    sleep(3);

    Diseqcs.Clear();
    Diseqcs.Load(AddDirectory("/etc/vdr", "diseqc.conf"), true, Setup.DiSEqC);
    cPlugin *plug = cPluginManager::GetPlugin("netcvrotor");
    const char *cmd = "netcvrotor.loadConfig";
    if(plug)
       plug->Service(cmd, (void*)cmd);

    menu_->netcvState_ = eDone;
}

eOSState cNetCVDiseqcItem::ProcessKey(eKeys Key)
{
    switch(menu_->netcvState_)
    {
        case eDiagnose:
            switch (Key)
            {
                case kOk:
                case kRed:
                    menu_->Sethelp(NULL);
                    Skins.Message(mtStatus, tr("Please wait..."));
                    if (Upload() == 0)
                    {
                        Restart(false);
//                        menu_->netcvState_ = eRestart;
//                        menu_->Sethelp(tr("Yes"), NULL, NULL, tr("No"));
//                        Skins.Message(mtStatus, tr("Restart VDR?"));
                    }
                    else
                        Skins.Message(mtError, tr("Uploading failed!"));
                    return osContinue;
                    break;
                default:
                    return osUnknown;
                    break;
            }
            break;
/*
        case eRestart:
            switch (Key)
            {
                case kOk:
                case kRed:
                    menu_->Sethelp();
                    Restart(true);
                    menu_->netcvState_ = eDiagnose;
                    break;
                case kBack:
                case kBlue:
                    menu_->Sethelp();
                    Restart(false);
                    menu_->netcvState_ = eDiagnose;
                    menu_->Display();
                    break;
                default:
                    return osContinue;
                    break;
            }
            break;
*/
        default:
            break;
    }
    return osUnknown;
}

// class cNetCVCurrentDeviceItem---------------------------------------
cNetCVCurrentDeviceItem::cNetCVCurrentDeviceItem(devIt_t *currentnetceiver, devIt_t firstnetceiver, devIt_t lastnetceiver)
{
    currentnetceiver_ = currentnetceiver;
    firstnetceiver_ = firstnetceiver;
    lastnetceiver_ = lastnetceiver;
    std::string strBuff(tr("Current"));
    strBuff.append(" Netceiver: ");
    strBuff.append((**currentnetceiver)->GetName());
    ++strBuff.at(strBuff.length()-1);
    SetText(strBuff.c_str());
}

cNetCVCurrentDeviceItem::~cNetCVCurrentDeviceItem()
{
}

eOSState cNetCVCurrentDeviceItem::ProcessKey(eKeys Key)
{
    bool bChanged = false;
    switch(Key)
    {
        case kLeft:
            if (*currentnetceiver_ != firstnetceiver_)
            {
                --*currentnetceiver_;
                bChanged = true;
            }
            break;
        case kRight:
            if (*currentnetceiver_ != lastnetceiver_-1)
            {
                ++*currentnetceiver_;
                bChanged = true;
            }
            break;
        default:
            break;
    }
    if (bChanged)
        return os_User;
    else
        return osUnknown;
}
