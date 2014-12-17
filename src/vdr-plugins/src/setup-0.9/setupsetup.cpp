/****************************************************************************
 * DESCRIPTION:
 *             Setup Dialog
 *
 * $Id: setupsetup.cpp,v 1.1 2005/10/03 14:05:20 ralf Exp $
 *
 * Contact:    ranga@teddycats.de
 *
 * Copyright (C) 2005 by Ralf Dotzert
 ****************************************************************************/


#include <vdr/menu.h>
#include <vdr/submenu.h>
#include <string.h>

#include "setupsetup.h"

//*****************************************************************************
// Setup Configuration
//*****************************************************************************

cSetupSetup::cSetupSetup()
{
    // Set Default Values
//FIXME: use FileNameChars
    _allowedChar = " .,:;+-*~?!\"$%&/\()=`'_abcdefghijklmnopqrstuvwxyz0123456789";
    _strMenuSuffix = (char*)"Menu Suffix";
    _strEntryPrefix = (char*)"Entry Prefix";
    //strcpy(_menuSuffix, " ...");
    strcpy(_menuSuffix, "");
    strcpy(_entryPrefix, "");
}

bool
cSetupSetup::SetupParse(const char *Name, const char *Value)
{
    if (strcmp(Name, _strMenuSuffix) == 0)
        snprintf(_menuSuffix, sizeof(_menuSuffix), Value);
    else if (strcmp(Name, _strEntryPrefix) == 0)
        snprintf(_entryPrefix, sizeof(_entryPrefix), Value);

    else
        return false;

    return true;
}

//*****************************************************************************
// Setup Page
//*****************************************************************************
cSetupSetupPage::cSetupSetupPage()
{
    Add(new
        cMenuEditStrItem(tr(SetupSetup._strMenuSuffix),
                         SetupSetup._menuSuffix,
                         sizeof(SetupSetup._menuSuffix),
                         SetupSetup._allowedChar));
    Add(new
        cMenuEditStrItem(tr(SetupSetup._strEntryPrefix),
                         SetupSetup._entryPrefix,
                         sizeof(SetupSetup._entryPrefix),
                         SetupSetup._allowedChar));
}


void
cSetupSetupPage::Store(void)
{
    SetupStore(SetupSetup._strMenuSuffix, SetupSetup._menuSuffix);
    SetupStore(SetupSetup._strEntryPrefix, SetupSetup._entryPrefix);
}

eOSState
cSetupSetupPage::ProcessKey(eKeys Key)
{
    //printf("\033[0;93m %s(%i): %s \033[0m\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);

    cSubMenu vdrSubMenu;
    char *menuXML = NULL;
    eOSState state = cOsdMenu::ProcessKey(Key);

    switch (Key)
    {
    case kOk:
        // Load Menu Configuration // TODO use FileNameFactory
        asprintf(&menuXML, "%s/setup/vdr-menu.xml",
                 cPlugin::ConfigDirectory());
        if (vdrSubMenu.LoadXml(menuXML))
        {
            vdrSubMenu.SetMenuSuffix(SetupSetup._menuSuffix);
            vdrSubMenu.SaveXml();
        }
        free(menuXML);
        Store();
        break;
    default:
        break;
    }
    return state;
}
