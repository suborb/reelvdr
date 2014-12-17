/****************************************************************************
 * DESCRIPTION:
 *             Setup Dialog
 *
 * $Id: setupsetup.h,v 1.1 2005/10/03 14:05:20 ralf Exp $
 *
 * Contact:    ranga@teddycats.de
 *
 * Copyright (C) 2005 by Ralf Dotzert
 ****************************************************************************/


#ifndef SETUPSETUP_H
#define SETUPSETUP_H

#include "setup.h"
#include <vdr/plugin.h>

/**
@author Ralf Dotzert
*/
//*****************************************************************************
// Setup Configuration
//*****************************************************************************

class cSetupSetup
{
  public:                      // public /private??
    const char *_allowedChar;
    char *_strMenuSuffix;
    char *_strEntryPrefix;
    char _menuSuffix[20];
    char _entryPrefix[2];
  public:
      cSetupSetup();
    bool SetupParse(const char *Name, const char *Value);
};

extern cSetupSetup SetupSetup;

//*****************************************************************************
// Setup Page
//*****************************************************************************
class cSetupSetupPage:public cMenuSetupPage
{
  public:
    cSetupSetupPage();
    void Store(void);
    eOSState ProcessKey(eKeys Key);
};


#endif
