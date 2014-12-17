#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <vdr/osdbase.h>
#include <vdr/config.h>
#include "webbrowserosd.h"
#include <vdr/sysconfig_vdr.h>
#include "webbrowser.h"
#include "hdutil.h"
#if VDRVERSNUM < 10716
#include <string.h>
#else
#include <sstream>
#endif
#include <string>
#include <iostream>
#include <vdr/remote.h>

#define LINKS "/usr/sbin/links2 "
#define STARTFBX "startfbx"

#define HD_VM_RESOLUTION_1080  0
#define HD_VM_RESOLUTION_720   1
#define HD_VM_RESOLUTION_576   2
#define HD_VM_RESOLUTION_480   3

cWebbrowserOsd::cWebbrowserOsd():cOsdMenu("")
{
    //SystemExec("hdctrld -X 1");
    std::stringstream buf;
    char buf2[512];
    char *proxy_ip = NULL, *proxy_port = NULL, *proxy_enable = NULL;
    int resolution = -1;
    char resString[64];

    cSetupLine *setupLine;
    if ((setupLine = Setup.Get("HDresolution", "reelbox")))
        resolution = atoi(setupLine->Value());

    switch (resolution)
    {
    case HD_VM_RESOLUTION_1080:
        strcpy(resString, "1080");
        break;
    case HD_VM_RESOLUTION_720:
        strcpy(resString, "720");
        break;
    case HD_VM_RESOLUTION_576:
        strcpy(resString, "576");
        break;
    case HD_VM_RESOLUTION_480:
        strcpy(resString, "480");
        break;
    default:
        printf("ERROR\n");
        break;
    }

    cSysConfig_vdr & sysconfig = cSysConfig_vdr::GetInstance();
    proxy_ip = (char *)sysconfig.GetVariable("PROXY_IP");
    proxy_port = (char *)sysconfig.GetVariable("PROXY_PORT");
    proxy_enable = (char *)sysconfig.GetVariable("USE_PROXY");

    switch (WebbrowserSetup.mode)
    {
    case MODE_LINKS:
        {
            buf << "hdctrld -X 1 ;" << LINKS;

            if (proxy_ip && proxy_port && proxy_enable && !strcmp(proxy_enable, "yes"))
            {
                buf << " -http-proxy " << proxy_ip << ":" << proxy_port;
                buf << " -ftp-proxy " << proxy_ip << ":" << proxy_port;
            }

            buf << " --dfb:bg-color=00000000 -driver directfb --dfb:system=fbdev ";
            buf << "--dfb:disable-module=keyboard --dfb:disable-module=x11 --dfb:disable-module=i810 ";
            buf << "--dfb:disable-module=i830 --dfb:disable-module=ps2mouse --dfb:disable-module=keyboard -g -enable-javascript 1 ";
            sprintf(buf2, " -html-user-font-size %i -html-image-scale %i -menu-font-size %i -transparency %i -margin_x %i -margin_y %i -display-optimize %i ", WebbrowserSetup.fontSize, WebbrowserSetup.scalepics, WebbrowserSetup.menuFontSize, WebbrowserSetup.transparency, WebbrowserSetup.margin_x, WebbrowserSetup.margin_y, WebbrowserSetup.optimize);
            buf << buf2;
#if 0
            switch (Setup.OSDLanguage)
            {
            case 0:
                buf << " -language english ";
                break;
            case 1:
                buf << " -language german ";
                strcat(buf, buf2);
                break;
            case 2:
                buf << " -language slovak ";
                break;
            case 3:
                buf << " -language italian ";
                break;
            case 4:
                buf << " -language dutch ";
                break;
            case 5:
                buf << " -language portuguese ";
                break;
            case 6:
                buf << " -language french ";
                break;
            case 7:
                buf << " -language norwegian ";
                break;
            case 8:
                buf << " -language finnish ";
                break;
            case 9:
                buf << " -language polish ";
                break;
            case 10:
                buf << " -language spanish ";
                break;
            case 11:
                buf << " -language greek ";
                break;
            case 12:
                buf << " -language romanian ";
                break;
            case 13:
                buf << " -language croatian ";
                break;
            case 14:
                buf << " -language estonian ";
                break;
            case 15:
                buf << " -language danish ";
                break;
            case 16:
                buf << " -language czech ";
                break;
            default:
                break;
            }
#endif
            buf << WebbrowserSetup.startURL << " &";
            std::cout << "executing: " << buf.str() << std::endl;
            SystemExec(buf.str().c_str());
            break;
        }
    case MODE_FIREFOX:
        {
            switchToHDifAutoFormat();
            buf << STARTFBX << " " << resString << " firefox " << WebbrowserSetup.FFstartURL;
            std::cout << "executing: " << buf.str() << std::endl;
            SystemExec(buf.str().c_str());
            SetFbParams();
            break;
        }
    case MODE_X11:
        {
            switchToHDifAutoFormat();
            buf << STARTFBX << " " << resString << " kde";
            std::cout << "executing: " << buf.str() << std::endl;
            SystemExec(buf.str().c_str());
            SetFbParams();
            break;
        }
    case MODE_SNES:
        {
            switchToHDifAutoFormat();
            buf << STARTFBX << " " << resString << " snes";
            std::cout << "executing: " << buf.str() << std::endl;
            SystemExec(buf.str().c_str());
            SetFbParams();
            break;
        }
    default:
        break;
    }
}

cWebbrowserOsd::~cWebbrowserOsd()
{
    std::string buf;
    if (WebbrowserSetup.mode == MODE_LINKS)
        buf = "killall links2; hdctrld -X 0";
    else
        buf = "chvt 7";
    std::cout << "executing: " << buf << std::endl;
    SystemExec(buf.c_str());
    switchBack();
    //cRemote::Put(kMenu);
}

eOSState cWebbrowserOsd::ProcessKey(eKeys Key)
{

    //eOSState state = cOsdMenu::ProcessKey(Key);
    cOsdMenu::ProcessKey(Key);

    switch (Key)
    {
    case kBack:
        return osEnd;
    default:
        break;
    }
    Display();
    return osContinue;
}


void cWebbrowserOsd::Display(void)
{
    cOsdMenu::Display();

    //DisplayMenu()->SetText(buffer.c_str(), 0);
}

void cWebbrowserOsd::SetFbParams(void)
{
    for (int i = 0; i < 3; i++) {
        setalpha(WebbrowserSetup.X11alpha);
        if (WebbrowserSetup.center_full == 0)
            setcenter();
        else
            setfull();
        cCondWait::SleepMs(1000);
    }

    setalpha(WebbrowserSetup.X11alpha);
    if (WebbrowserSetup.center_full == 0)
        setcenter();
    else
        setfull();
}
