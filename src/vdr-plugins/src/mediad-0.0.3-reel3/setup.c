#include "setup.h"

#include <vdr/menuitems.h>
#include "plugins.h"


cVdrcdSetup VdrcdSetup;
int AutoStartMode = 1;
int RequestDvdAction = 1;
int RequestCdAction = 1;

cVdrcdSetup::cVdrcdSetup(void) {
    // add all known plugins
    mediaplugins.Add(new dmharchivedvd());
    mediaplugins.Add(new vdrrecording());
    mediaplugins.Add(new dvd());
    mediaplugins.Add(new vcd());
    mediaplugins.Add(new mplayer());
    mediaplugins.Add(new mp3());
    mediaplugins.Add(new photocd());
    mediaplugins.Add(new cdda());
    mediaplugins.Add(new ripit());
    mediaplugins.Add(new burn());
    mediaplugins.Add(new dvdswitch());
    mediaplugins.Add(new filebrowser());
    mediaplugin *src = mediaplugins.First();
    while(src) {
        src=mediaplugins.Next(src);
    } //TODO why traverse?
    HideMenu = false;

}

cVdrcdSetup::cVdrcdSetup(const cVdrcdSetup &setup)
{
    mediaplugin *pPlugin = setup.mediaplugins.First();
    while(pPlugin) {
        mediaplugins.Add(pPlugin->clone());
        pPlugin = setup.mediaplugins.Next(pPlugin);
    }
    HideMenu = setup.HideMenu;
}

cVdrcdSetup& cVdrcdSetup::operator=(const cVdrcdSetup &setup)
{
    mediaplugins.Clear();
    mediaplugin *pPlugin = setup.mediaplugins.First();
    while(pPlugin) {
        mediaplugins.Add(pPlugin->clone());
        pPlugin = setup.mediaplugins.Next(pPlugin);
    }
    HideMenu = setup.HideMenu;
    return *this;
}

bool cVdrcdSetup::SetupParse(const char *Name, const char *Value) {

        if (strcmp(Name,"AutoStartMode")==0){
        AutoStartMode = atoi(Value);
        return true;
    } 

        if (strcmp(Name,"RequestDvdAction")==0){
        RequestDvdAction = atoi(Value);
        return true;
    } 
     
        if (strcmp(Name,"RequestCdAction")==0){
        RequestCdAction = atoi(Value);
        return true;
    }

    mediaplugin *pPlugin = mediaplugins.First();
    while(pPlugin) {
        char *pluginName;
        asprintf(&pluginName, "AutoStart%s", pPlugin->getName());
        if(strcmp(Name, pluginName) == 0) {
            pPlugin->setAutoStart(atoi(Value));
            free(pluginName);
            return true;
        }
        free(pluginName);
        asprintf(&pluginName, "Activated%s", pPlugin->getName());
        if(strcmp(Name, pluginName) == 0) {
            pPlugin->setActive(atoi(Value));
            free(pluginName);
            return true;
        }
        free(pluginName);
        pPlugin = mediaplugins.Next(pPlugin);
    }
  if (strcmp(Name, "HideMenu" )  == 0) {
        HideMenu = atoi(Value);
        return true;
    }
    return false;
}

cVdrcdMenuSetupPage::cVdrcdMenuSetupPage(void) {
    _setup = VdrcdSetup;
    _AutoStartMode = AutoStartMode;
    static const char *AutoStartMode_str[3]={trVDR("no"),trVDR("yes"),tr("ask each time")};
        Add(new cMenuEditStraItem(tr("AutoStart"), &_AutoStartMode, 3, AutoStartMode_str));
        Add(new cMenuEditBoolItem(tr("Request for DVD action"), &RequestDvdAction));
        Add(new cMenuEditBoolItem(tr("Request for CD action"), &RequestCdAction));
/* //TODO
    mediaplugin *pPlugin = _setup.mediaplugins.First();
    while(pPlugin) {
        if(pPlugin->checkPluginAvailable(true)) {
            char* menuEntry;
            asprintf(&menuEntry, "%s %s", tr(pPlugin->getName()), tr("AutoStart"));
            Add(new cMenuEditBoolItem(menuEntry, pPlugin->isAutoStartSetup()));
            free(menuEntry);
            
        //  asprintf(&menuEntry, "%s %s", tr(pPlugin->getName()), tr("activated"));
        //  Add(new cMenuEditBoolItem(menuEntry, pPlugin->isActiveSetup()));
        //  free(menuEntry);
            
        }
        pPlugin = _setup.mediaplugins.Next(pPlugin);
    }
    */
    //AutoStartMode = _AutoStartMode;
        //RequestDvdAction = RequestDvdAction_;
/*  
    Add(new cMenuEditBoolItem(tr("Hide main menu entry"), &_setup.HideMenu));
*/          
}

cVdrcdMenuSetupPage::~cVdrcdMenuSetupPage() {
}

void cVdrcdMenuSetupPage::Store(void) {
    mediaplugin *pPlugin = _setup.mediaplugins.First();
    while(pPlugin) {
        char *pluginName;
        asprintf(&pluginName, "AutoStart%s", pPlugin->getName());
        pPlugin->setAutoStart(_AutoStartMode==1);

        SetupStore(pluginName, pPlugin->isAutoStart());
        free(pluginName);
        asprintf(&pluginName, "Activated%s", pPlugin->getName());
        pPlugin->setActive(true); // never deactivate a plugin, handle !autostart in hal.c
        SetupStore(pluginName, pPlugin->isActive());
        free(pluginName);
        pPlugin = _setup.mediaplugins.Next(pPlugin);
    }
    SetupStore("HideMenu", _setup.HideMenu);
    SetupStore("AutoStartMode", _AutoStartMode);
#ifndef REELVDR 
        SetupStore("RequestDvdAction", RequestDvdAction);
#else 
        SetupStore("RequestDvdAction", "mediad", RequestDvdAction);
        SetupStore("RequestCdAction", "mediad", RequestCdAction);   
#endif

    AutoStartMode = _AutoStartMode;
    VdrcdSetup = _setup;
}
