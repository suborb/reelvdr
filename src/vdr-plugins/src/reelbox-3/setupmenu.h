#ifndef REELBOX_SETUPMENU_H
#define REELBOX_SETUPMENU_H

#include "ReelBoxMenu.h"
#include <vdr/plugin.h>

#include <vdr/menuitems.h>
#include <vdr/menu.h>
#include <vdr/remote.h>
#include <string.h>


class cMenuReelBoxSetup : public cMenuSetupPage
{
    private:
        cPlugin* plugin;

    public:
        cMenuReelBoxSetup(cPlugin*);
        void Set();
        void Store();
        eOSState ProcessKey(eKeys key);
};
#endif
