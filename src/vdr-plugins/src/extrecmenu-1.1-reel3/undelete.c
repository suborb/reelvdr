/*
 *  little undelete functionality to undelete recordings
 *  replaces slow/uncomfortable undelete plugin
 *
 */

#include <vdr/plugin.h>
#include "service.h"

static const char VERSION[]        = "0.0.2";
static const char DESCRIPTION[]    = "Direct access to extrec's undelete menu";
static const char MAINMENUENTRY[]  = "Undelete Recordings";
static const char SETUPTEXT[]      = "Undelete Recordings";

class cPluginUndelete: public cPlugin
{
    public:
        virtual const char* Version() { return VERSION; }
        virtual const char* Description() { return tr(DESCRIPTION); }
        virtual bool Initialize();
        //virtual bool Start();
        virtual cOsdObject *MainMenuAction();
        //virtual const char *MainMenuEntry() { return tr(MAINMENUENTRY); }
        virtual const char *MainMenuEntry();
        virtual bool HasSetupOptions(void) { return false; }
        char buffer[128];

    protected:
        virtual const char* SetupText() { return tr(SETUPTEXT); }
        virtual const char* MainMenuText() { printf (" %s \n", __PRETTY_FUNCTION__); return tr(MAINMENUENTRY); }
};

bool cPluginUndelete::Initialize()
{
    return true;
}

const char *cPluginUndelete::MainMenuEntry()
{
    if (DeletedRecordings.Count() > 0)
    {
        snprintf(buffer, sizeof(buffer), "%s (%d)", tr(MAINMENUENTRY), DeletedRecordings.Count());
        return buffer;
    }
    else
        return NULL;
}


cOsdObject *cPluginUndelete::MainMenuAction()
{
    printf (" %s \n", __PRETTY_FUNCTION__);

    cOsdMenu *menu = NULL;
    cPlugin *p = cPluginManager::GetPlugin("extrecmenu");
    if (p)
    {
       printf (" try get extrecmenu-undelete-v1.0 \n");

        ExtrecMenuUndelete_v1_0 *serviceData = new ExtrecMenuUndelete_v1_0;

        if (p->Service("extrecmenu-undelete-v1.0", serviceData))
        {
            printf (" Service success \n");
            menu = serviceData->Menu;
            if (menu)
              printf (" get successfully menu \n");
        }
        else
        {
            Skins.Message(mtError, tr("This version of extrecmenu does not support this service!"));
        }
        delete serviceData;
    }
    else
    {
        Skins.Message(mtError, tr("ExtRecMenu does not exist!"));
    }
    return menu;
}


VDRPLUGINCREATOR(cPluginUndelete); // Don't touch this!


