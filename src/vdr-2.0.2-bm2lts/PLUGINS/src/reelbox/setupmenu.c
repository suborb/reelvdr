#include "setupmenu.h"

cMenuReelBoxSetup::cMenuReelBoxSetup(cPlugin* pl)
{
    plugin = pl;
    SetHasHotkeys();
    Set();
}

void cMenuReelBoxSetup::Store(){}

void cMenuReelBoxSetup::Set()
{
    Add(new cOsdItem(hk(tr("Picture Settings")),osUnknown, true));
    Add(new cOsdItem(hk(tr("Video Settings")),osUnknown, true));
    Add(new cOsdItem(hk(tr("Audio Settings")),osUnknown, true));

    Display();
}

eOSState cMenuReelBoxSetup::ProcessKey(eKeys key)
{
    eOSState state;
    if (key != kOk) // to avoid "changes does message"
        state = cMenuSetupPage::ProcessKey(key);
    else state = osUnknown;

    if (state == osUnknown && key == kOk)
    {
        const char *text = Get(Current())->Text();

        if ( text && strstr(text, tr("Video Settings")) )
            return AddSubMenu(new Reel::cMenuVideoMode(plugin, true) );

        else if ( text && strstr(text, tr("Audio Settings")) )
            return AddSubMenu(new Reel::cMenuVideoMode(plugin, false));
        else if ( text && strstr(text, tr("Picture Settings")) )
            cRemote::CallPlugin("reelbox");
        state = osBack;
    }

    return state;
}
