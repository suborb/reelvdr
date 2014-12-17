#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <vdr/osdbase.h>
#include <vdr/config.h>
#include "amarokstarter.h"
#include "amarokstarterosd.h"
#include "hdutil.h"
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vdr/remote.h>

#define STARTFBX "startfbx"

#define HD_VM_RESOLUTION_1080  0
#define HD_VM_RESOLUTION_720   1
#define HD_VM_RESOLUTION_576   2
#define HD_VM_RESOLUTION_480   3

cAmarokStarterOsd::cAmarokStarterOsd():cOsdMenu("")
{
    //SystemExec("hdctrld -X 1");
    std::stringstream buf;
    int resolution = -1;
    std::string resString;

    cSetupLine *setupLine;
    if ((setupLine = Setup.Get("HDresolution", "reelbox")))
        resolution = atoi(setupLine->Value());

    switch (resolution)
    {
    case HD_VM_RESOLUTION_1080:
        resString = "1080";
        break;
    case HD_VM_RESOLUTION_720:
        resString = "720";
        break;
    case HD_VM_RESOLUTION_576:
        resString = "576";
        break;
    case HD_VM_RESOLUTION_480:
        resString = "480";
        break;
    default:
        printf("ERROR\n");
        break;
    }

    switchToHDifAutoFormat();
    buf << STARTFBX << " " << resString << " amarok";
    std::cout << "executing: " << buf.str() << std::endl;
    SystemExec(buf.str().c_str());
    SetFbParams();
}

cAmarokStarterOsd::~cAmarokStarterOsd()
{
    std::string buf = "chvt 7";
    std::cout << "executing: " << buf << std::endl;
    SystemExec(buf.c_str());
    switchBack();
    //cRemote::Put(kMenu);
}

eOSState cAmarokStarterOsd::ProcessKey(eKeys Key)
{
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


void cAmarokStarterOsd::Display(void)
{
    cOsdMenu::Display();
}

void cAmarokStarterOsd::SetFbParams(void)
{
    cSetupLine *setupLine;
    int X11alpha = 255;
    int center_full = 0;

    if ((setupLine = Setup.Get("X11alpha", "webbrowser")))
        X11alpha = atoi(setupLine->Value());
    if ((setupLine = Setup.Get("center_full", "webbrowser")))
        center_full = atoi(setupLine->Value());

    setalpha(X11alpha);
    if (center_full == 0)
        setcenter();
    else
        setfull();
    cCondWait::SleepMs(1000);
    setalpha(X11alpha);
    if (center_full == 0)
        setcenter();
    else
        setfull();
    cCondWait::SleepMs(1000);
    setalpha(X11alpha);
    if (center_full == 0)
        setcenter();
    else
        setfull();
    cCondWait::SleepMs(1000);
    setalpha(X11alpha);
    if (center_full == 0)
        setcenter();
    else
        setfull();
}
