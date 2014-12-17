/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia                                 *
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
 ***************************************************************************/
 
//taken from streamdev server plugin 

#include "suspend.h"

#include <linux/dvb/video.h>  // video_still_picture
#include <string>

cSuspendLive::cSuspendLive(void)
: cThread("Mediad: player suspend")
{
    Activate(true);
}

cSuspendLive::~cSuspendLive() 
{
    Activate(false);
}

void cSuspendLive::Activate(bool On) 
{
    printf("Activate cSuspendLive %d\n", On);
    if (On)
        Start();
    else
        Stop();
}

void cSuspendLive::Stop(void) 
{
    if (m_Active) 
    {
        m_Active = false;
        Cancel(1000);
    }
}

void cSuspendLive::Action(void) 
{
    m_Active = true;
    while (m_Active) 
    {
        ShowSplashScreen();
        usleep(100000);
    }
}

void cSuspendLive::ShowSplashScreen()
{
    //printf("pluginMediaD::ShowSplashScreen() \n");
    std::string stillPicture = cPlugin::ConfigDirectory();
    // depends of mrl 
    stillPicture += "/xinemediaplayer/splash.mpg";

    uchar *buffer;
    int fd;
    struct stat st;
    struct video_still_picture sp;

    if ((fd = open (stillPicture.c_str(), O_RDONLY)) >= 0)
    {
        fstat (fd, &st);
        sp.iFrame = new char[st.st_size];  
        if (sp.iFrame)
        {
            sp.size = st.st_size;
            if (read (fd, sp.iFrame, sp.size) > 0)
            {
                buffer = (uchar *) sp.iFrame;  
                DeviceStillPicture(buffer, sp.size);
                //cDevice::PrimaryDevice()->StillPicture(buffer, sp.size);
            }
            delete [] sp.iFrame; 
        }
        close (fd);
    }
}

bool cSuspendCtl::m_Active = false;

cSuspendCtl::cSuspendCtl(void)
: cControl(m_Suspend = new cSuspendLive) 
{
    m_Active = true;
}

cSuspendCtl::~cSuspendCtl() 
{
    m_Active = false;
    DELETENULL(m_Suspend);
}

