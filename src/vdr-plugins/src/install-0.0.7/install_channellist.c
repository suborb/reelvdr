/***************************************************************************
 *   Copyright (C) 2009 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                     Based on an older Version by: Tobias Bratfisch      *
 *                 2009 Additions by Tobias Bratfisch                      *
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
 * install_channellist.c
 *
 ***************************************************************************/

#include "install_channellist.h"
#include "install_channelscan.h"

#include <vdr/device.h>
#include <vdr/config.h>
#include <vdr/s2reel_compat.h>
#include "../mcli/mcli_service.h"

#if 0
cInstallChannelList::cInstallChannelList(bool showMsg)
{
    Skins.Flush();              /* TB: Hack: avoid the "clear thumbnail"-problem */

    cPlugin *p = cPluginManager::GetPlugin("channelscan");
    if (p)
    {
        struct
        {
            char *newTitle;
            char *Path;
              std::string AdditionalText;
            bool WizardMode;
            cOsdMenu *pSelectMenu;
        } ChannelList;
        
        asprintf(&ChannelList.newTitle, "%s", Title());

        ChannelList.Path = NULL;
        for (int i = 0; i < MAXTUNERS; i++)
        {
            cDevice *cdev = cDevice::GetDevice(i);  //TODO: do not only look at the first tuner
            if (cdev && !cdev->HasInput())
                continue;
            else if (cdev)
            {
                uint64_t v = 0;
                cdev->GetAttribute("fe.type", &v);
                //printf("XXXXXXXXXXXXX: i: %i type: %i\n", i, v);
                if ((fe_type_t) v == FE_OFDM)
                    asprintf(&ChannelList.Path, "%s/%s", "/etc/vdr", "channels/Terrestrial");
                else if ((fe_type_t) v == FE_QAM)
                    asprintf(&ChannelList.Path, "%s/%s", "/etc/vdr", "channels/Cable");
                else
                    asprintf(&ChannelList.Path, "%s/%s", "/etc/vdr", "channels/Satellite");
                break;
            }
        }
        if (ChannelList.Path == NULL)
            asprintf(&ChannelList.Path, "%s/%s", "/etc/vdr", "channels");

        ChannelList.WizardMode = true;
        if (showMsg || (Setup.ReelboxMode != Setup.ReelboxModeTemp && Setup.ReelboxMode != eModeStandalone))
            ChannelList.AdditionalText = tr("The channellist could not be installed automatically from the ReelBox Avantgarde.");
        p->Service("ChannelList", (void *)&ChannelList);
        free(ChannelList.newTitle);
        free(ChannelList.Path);
        AddSubMenu(ChannelList.pSelectMenu);
    }
    else
    {
        printf("ERROR: channelscan-plugin could not found!\n");
        esyslog("ERROR: channelscan-plugin could not be found!\n");
    }
}
#else
cInstallChannelList::cInstallChannelList(bool showMsg) :
    cInstallSubMenu(tr("Channellist"))
{
    Skins.Flush();              /* TB: Hack: avoid the "clear thumbnail"-problem */
    
    cPlugin *p = cPluginManager::GetPlugin("channelscan");
    if (p) {
        struct {
            char *newTitle;
            char *Path;
              std::string AdditionalText;
            bool WizardMode;
            cOsdMenu *pSelectMenu;
        } ChannelList;
#ifdef RBMINI
        asprintf(&ChannelList.newTitle, "%s - %s", tr(MAINMENUENTRY), tr("Channellist"));
#else
        asprintf(&ChannelList.newTitle, "%s", Title());
#endif
        ChannelList.Path = NULL;
        int nSat=0;
        int nCab=0;
        int nTer=0;
        cPlugin *mcliPlugin = cPluginManager::GetPlugin("mcli");
        if (mcliPlugin) {
            mclituner_info_t info;
            for (int i = 0; i < MAX_TUNERS_IN_MENU; i++)
                info.name[i][0] = '\0';
            mcliPlugin->Service("GetTunerInfo", &info);
            for (int i = 0; i < MAX_TUNERS_IN_MENU; i++) {
                if(info.preference[i] == -1 || strlen(info.name[i]) == 0) {
                    break;
                } else {
                    switch(info.type[i]) {
                        case FE_QPSK : nSat++; break; // DVB-S
                        case FE_DVBS2: nSat++; break; // DVB-S2
                        case FE_OFDM : nTer++; break; // DVB-T
                        case FE_QAM  : nCab++; break; // DVB-C
                    } // switch
                } // if
            } // for
        } else {
            for (int i = 0; i < MAXTUNERS; i++) {
                cDevice *cdev = cDevice::GetDevice(i);  //TODO: do not only look at the first tuner
#if VDRVERSNUM < 10716
                if (!cdev || !cdev->HasInput())
#else
                if(!HAS_DVB_INPUT(cdev))
#endif
                    continue;
                uint64_t v = 0;
                if(0==cdev->GetAttribute("fe.type", &v)) {
                    if ((fe_type_t) v == FE_OFDM)
                        nTer++;
                    else if ((fe_type_t) v == FE_QAM)
                        nCab++;
                    else if ((fe_type_t) v == FE_QPSK)
                        nSat++;
                    else if ((fe_type_t) v == FE_DVBS2)
                        nSat++;
                } // if
            } // for
        } // if
        if(nSat && !nCab && !nTer)
            asprintf(&ChannelList.Path, "%s/%s", "/etc/vdr", "channels/Satellite");
        if(!nSat && nCab && !nTer)
            asprintf(&ChannelList.Path, "%s/%s", "/etc/vdr", "channels/Cable");
        if(!nSat && !nCab && nTer)
            asprintf(&ChannelList.Path, "%s/%s", "/etc/vdr", "channels/Terrestrial");
        if (ChannelList.Path == NULL)
            asprintf(&ChannelList.Path, "%s/%s", "/etc/vdr", "channels");
        isyslog("Found %d S, %d C, %d T @ %s -> %s\n", nSat, nCab, nTer, mcliPlugin?"MCLI":"DVB", ChannelList.Path);

        ChannelList.WizardMode = true;
        if (showMsg || (Setup.ReelboxMode != Setup.ReelboxModeTemp && Setup.ReelboxMode != eModeStandalone))
            ChannelList.AdditionalText = tr("The channellist could not be installed automatically from the ReelBox Avantgarde.");
        p->Service("ChannelList", (void *)&ChannelList);
        free(ChannelList.newTitle);
        free(ChannelList.Path);
        AddSubMenu(ChannelList.pSelectMenu);
    } else {
        printf("ERROR: channelscan-plugin could not found!\n");
        esyslog("ERROR: channelscan-plugin could not be found!\n");
    } // if
} // cInstallChannelList::cInstallChannelList
#endif

eOSState cInstallChannelList::ProcessKey(eKeys Key)
{
    bool HadSubMenu = HasSubMenu();
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (!HasSubMenu() && (HadSubMenu))
    {
        Save();
        return osUser1;
    }
    return state;
}

bool cInstallChannelList::Save()
{
    return false;               // Here we don't save
}

void cInstallChannelList::Set()
{
}
