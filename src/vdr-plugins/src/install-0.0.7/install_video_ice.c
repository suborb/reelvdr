#include "install_video_ice.h"

cInstallVidoutICE::cInstallVidoutICE() : cInstallSubMenu(tr("Video Settings"))
{
    menu = NULL;
    
    Set();
}

void cInstallVidoutICE::Set()
{
    Clear();
   
    cPlugin *p = cPluginManager::GetPlugin("reelice");
    
    if (p) {
        struct { cOsdMenu *m; cString title;} data;
        data.m = NULL;
        data.title = Title();
        p->Service("Get VideoSetting InstallWizard Menu", &data); // extra 'params' to indicate install wizard ?
        menu=data.m;
        
        if (menu)
            AddSubMenu(menu);
        else 
            Add(new cOsdItem(tr("Error: no menu got from reelice plugin"),
                             osUnknown, false));
    } else {
        Add(new cOsdItem(tr("reelice plugin not found"), osUnknown, false));
    }

    Display();
}

bool cInstallVidoutICE::Save()
{
    return true;
}

eOSState cInstallVidoutICE::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    
    // kOk press and the setting was not cancelled 
    if (Key == kOk && state == osUser1) 
        state = osUser1;
    
    if (Key == kYellow || Key == kGreen) // allow skip/back
        state = osUnknown;
    
    return state;
}

cInstallVidoutICE::~cInstallVidoutICE()
{
    // Delete old images from OSD
    cPluginManager::CallAllServices("setThumb", NULL);
}
