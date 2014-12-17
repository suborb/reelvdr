/*
 * DVD Player plugin for VDR
 *
 * Copyright (C) 2001.2002 Andreas Schultz <aschultz@warp10.net>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

#ifndef __SETUP_DVD_H
#define __SETUP_DVD_H

#include <vdr/menuitems.h>

class cDVDSetup {
 public:
    int MenuLanguage;
    int AudioLanguage;
    int SpuLanguage;
    int PlayerRCE;
    int ShowSubtitles;
    int HideMainMenu;
    int ReadAHead;
    int Gain;

    // AC3 stuff
    int AC3dynrng;

 public:
    cDVDSetup(void);

    bool SetupParse(const char *Name, const char *Value);
};

class cMenuSetupDVD : public cMenuSetupPage {
 private:
    cDVDSetup data;
 protected:
    virtual void Store(void);
 public:
    cMenuSetupDVD(void);
};

extern cDVDSetup DVDSetup;

#endif // __SETUP_DVD_H

