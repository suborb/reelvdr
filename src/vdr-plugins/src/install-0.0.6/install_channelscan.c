/***************************************************************************
 *   Copyright (C) 2009 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                     Based on an older Version by: Tobias Bratfisch      *
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
 * install_channelscan.c
 *
 ***************************************************************************/

#include "install_channelscan.h"

#include <vdr/device.h>
#include <vdr/remote.h>
#include <vdr/config.h>
#include <vdr/s2reel_compat.h>

#define FRONTEND_DEVICE "/dev/dvb/adapter%d/frontend%d"

void cInstallChannelscan::GetFrontendTypes()
{
    for (int i = 0; i < MAXTUNERS; i++)
    {
#ifndef DEVICE_ATTRIBUTES
        char dev[256];
        snprintf(dev, sizeof(dev), FRONTEND_DEVICE, i, 0);
        m_Frontend[i] = open(dev, O_RDONLY | O_NONBLOCK);
        if (m_Frontend[i] < 0)
        {
            esyslog("ERROR: install-plugin: cannot open frontend device.");
            m_Frontend[i] = -1;
            return;
        }
        if (ioctl(m_Frontend[i], FE_GET_INFO, &m_FrontendInfo[i]) < 0)
        {
            esyslog("ERROR: install-plugin: cannot read frontend info.");
            close(m_Frontend[i]);
            m_Frontend[i] = -1;
            return;
        }
        close(m_Frontend[i]);
#else
        cDevice *cdev = cDevice::GetDevice(i);
        if (cdev)
        {
            uint64_t v = 0;
            cdev->GetAttribute("fe.type", &v);
            m_Frontend[i] = (fe_type_t) v;
        }
        else
            return;
#endif
#define DBG_FRONTEND
#ifdef DBG_FRONTEND
        switch (m_FrontendInfo[i].type)
        {
        case FE_QPSK:          /* sat */
            printf("used tuner nr. %i is: satellite\n", i);
            break;
        case FE_QAM:           /* cable */
            printf("used tuner nr. %i is: cable\n", i);
            break;
        case FE_OFDM:          /* terr. */
            printf("used tuner nr. %i is: terrestrial\n", i);
            break;
        default:
            printf("used tuner nr. %i is: unknown type or missing\n", i);
        }
#endif
    }
}

bool cInstallChannelscan::FirstTunerIsSat()
{
    if ((m_Frontend[0]) && (m_Frontend[0] != -1) && (m_FrontendInfo[0].type == FE_QPSK || m_FrontendInfo[0].type == FE_DVBS2))
        return true;

    return false;
}

bool cInstallChannelscan::FirstTunerIsCable()
{
    if ((m_Frontend[0]) && (m_Frontend[0] != -1) && (m_FrontendInfo[0].type == FE_QAM))
        return true;

    return false;
}

bool cInstallChannelscan::FirstTunerIsTerr()
{
    if ((m_Frontend[0]) && (m_Frontend[0] != -1) && (m_FrontendInfo[0].type == FE_OFDM))
        return true;

    return false;
}

cInstallChannelscan::cInstallChannelscan() : cInstallSubMenu(tr("Channelscan"))
{
    Skins.Flush();              /* TB: Hack: avoid the "clear thumbnail"-problem */

    Add(new cOsdItem(tr("We shall now scan for channels."), osUnknown, false), false, NULL);
    Add(new cOsdItem(tr("This will take a few minutes."), osUnknown, false), false, NULL);
    Add(new cOsdItem("", osUnknown, false), false, NULL);
    Add(new cOsdItem(tr("Press OK to continue"), osUnknown, false), false, NULL);
    Display();
}

eOSState cInstallChannelscan::ProcessKey(eKeys Key)
{
  //  if (HasSubMenu())
  //      return osContinue;

    eOSState result = cOsdMenu::ProcessKey(Key);
    if (HasSubMenu())
    {
        if (result == osUser1)
        {
            CloseSubMenu();
            return osUser1;
        }
        else
        {
            if ((Key == kYellow) || (Key == kGreen))
                return osUnknown;
            else
                return osContinue;
        }
    }

    switch (Key)
    {
    case kOk:
        {
            cPlugin *p = cPluginManager::GetPlugin("channelscan");
            // printf (" -- get Channelscan  %s -- \n", p?"Valid Pointer":"!! -- broken Pointer -- !!");
            if (p)
            {
                if (FirstTunerIsSat())
                    p->Service("AutoScan", (void *)"DVB-S");
                else if (FirstTunerIsCable())
                    p->Service("AutoScan", (void *)"DVB-C");
                else if (FirstTunerIsTerr())
                    p->Service("AutoScan", (void *)"DVB-T");

#if VDRVERSNUM < 10716
//TODO: Do we need this?
                cRemote::SetReturnToPlugin("install");
#endif
                struct DataMenu{ cOsdMenu *pmenu; };
                DataMenu m;
                if (p->Service("Channelscan menu Install Wizard" ,&m) && m.pmenu)
                    return AddSubMenu(m.pmenu);

                //else call channelscan plugin
                cRemote::CallPlugin("channelscan");
            }
            else
            {
                esyslog("ERROR: channelscan plugin is missing!\n");
                Skins.Message(mtError, "channelscan plugin is missing!");
            }
            result = osUser1;
        }
        break;
    default:
        result = osUnknown;
        break;
    }
    return result;
}

bool cInstallChannelscan::Save()
{
    return false;
}

void cInstallChannelscan::Set()
{
}
