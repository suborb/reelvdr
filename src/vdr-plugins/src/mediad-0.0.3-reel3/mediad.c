/*
 * mediad.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * author: Matthias Haas
 */

#include "mediad.h"
#include "setup.h"
#include "status.h"
#include "plugins.h"
#include "hal.h"
#include "selectdevicemenu.h"
#include "optionsmenu.h"

#include "suspend.h"

#include <vdr/config.h>
#include <vdr/interface.h>
#include <vdr/plugin.h>
#include <vdr/menu.h>

#include <vdr/player.h>

#include <sys/wait.h>
#include <getopt.h>
#include <stdlib.h>
#include <vector>


extern const char *VideoDirectory;

static const char *VERSION        = "0.0.3-reel3";
static const char *DESCRIPTION    = trNOOP("autoplays identified media");
static const char *MAINMENUENTRY  = trNOOP("Play Disk");
static const char *MENUSETUPENTRY  = trNOOP("Play Medium");


bool switchedOn=false;;

class pluginMediaD : public cPlugin {
private:
    uint DirCount;
    char *MountScript;
    const char *tmpcdrom;

    cMediadStatus *status;
    bool checkStart(mediaplugin &plugin, bool alwaysAsk = false);
    hal halBinding;
    deviceSources _devices;
    void ShowSplashScreen();
    void HideSplashScreen(const char *pluginName);
    void CallFilebrowser(std::string path);
public:
      pluginMediaD(void);
      virtual ~pluginMediaD();
      virtual const char *Version(void) { return VERSION; }
      virtual const char *Description(void) { return tr(DESCRIPTION); }
      virtual const char *CommandLineHelp(void);
      virtual bool ProcessArgs(int argc, char *argv[]);
      virtual bool Start(void);
      virtual void Stop(void);
      virtual void Housekeeping(void);
      virtual const char* MainMenuEntry(void);
      virtual const char* MenuSetupPluginEntry(void)  { return tr(MENUSETUPENTRY); }
      virtual cOsdObject *MainMenuAction(void);
      virtual cMenuSetupPage *SetupMenu(void);
      virtual bool SetupParse(const char *Name, const char *Value);
      virtual bool Service(char const *id, void *data);
  };

const char* pluginMediaD::MainMenuEntry(void)
{
  return ((VdrcdSetup.HideMenu || !halBinding.hasPlayableMedia()) ? NULL : tr(MAINMENUENTRY)); 
}

pluginMediaD::pluginMediaD(void)
{
    DirCount = 0;
    MountScript = NULL;
    tmpcdrom = NULL;
    status = NULL;
    switchedOn = true;
}

pluginMediaD::~pluginMediaD()
{
    char path[1024];
    if(tmpcdrom) {
            snprintf(path, 1024, "%s/cdrom/%s",tmpcdrom,VDRSUBDIR);
            unlink(path);
            snprintf(path, 1024, "%s/cdrom",tmpcdrom);
            rmdir(path);
            rmdir(tmpcdrom);
     }

    free(MountScript);
    halBinding.Stop();
    delete status;
}

const char *pluginMediaD::CommandLineHelp(void)
{
    return "  -m MountScript, --mount=MountScript    mount.sh script\n";
}

bool pluginMediaD::ProcessArgs(int argc, char *argv[])
{
    static struct option long_options[] = {
        { "mount",       required_argument, NULL, 'm' },
            { NULL }
        };

    int c;
    while ((c = getopt_long(argc, argv, "m:c:", long_options, NULL)) != -1) {
        switch (c) {
        case 'm': 
            device::setMountScript(optarg);
            MountScript = strdup(optarg);
            break;

        default:  
            return false;
        }
    }
    return true;
}

bool pluginMediaD::Start(void) {


    if (MountScript == NULL)  
    {
        device::setMountScript("pmount.sh");
        MountScript = strdup("pmount.sh");
    }
    // TODO: make it configurable add devices
    _devices.Load(AddDirectory(ConfigDirectory(),"mediad.conf"));
    if(_devices.Count() < 1) {
      esyslog("ERROR: you should have defined at least one source in mediad.conf");
      cerr << "No source(s) defined in mediad.conf" << endl;
      return false;
    }
    halBinding.setDevices(&_devices);
    halBinding.setMountScript(MountScript);

    // add all known plugins
    status = new cMediadStatus;
    mediaplugin::setStatus(status);
    // delete ripit.process 
    rmdir("/tmp/ripit.process");
    mediaplugin::setMountCmd(MountScript);

    halBinding.Start();

    return true;
}

void pluginMediaD::Stop(void)
{
    halBinding.Stop();
}

void pluginMediaD::Housekeeping(void) {
}

cOsdObject *pluginMediaD::MainMenuAction()
{
    //cout << "------pluginMediaD::MainMenuAction---------------" << endl;
    bool matchedAnything = false;
    std::vector<mediaplugin*> matchedPlugins;
    device *pCurDevice;
    struct stat buf;

    if(!switchedOn)
        return NULL;

    //cerr << "mediad: MainMenuAction" << endl;
    if(hal::triggered)
    {
        pCurDevice = halBinding.getDevice();
        hal::triggered = false;
    } else if(selectDeviceMenu::triggered){
        pCurDevice = selectDeviceMenu::getDevice();
        selectDeviceMenu::triggered = false;
    } else {
                if(selectDeviceMenu::getDvdDevice(_devices)) {
                         //RMM hack: use dvd device as default, if present
                         pCurDevice = selectDeviceMenu::getDvdDevice(_devices);
                         //pCurDevice->setCurMediaType(unknown); //don't remember last media type
                } else { 
                 return new selectDeviceMenu(_devices);
                }
    }
    if(!pCurDevice) {
        cerr << "mediad: currently no device selected." << endl;
    }  
    mediaplugin::setCurrentDevice(pCurDevice);
    ripit ripit_plugin;
    ripit_plugin.searchPattern();
    mp3 mp3_plugin;
    mp3_plugin.searchPattern();
    cdda cdda_plugin;
    cdda_plugin.searchPattern(); 
    if(stat("/tmp/ripit.process", &buf) == 0 && ripit_plugin.matches() && mp3_plugin.matches() && cdda_plugin.matches()) {
        DPRINT("mediad: ripit process already started for audio cd\n");
        return NULL;
    }

    //only show when not restarted from requestPluginMenu
    if(requestPluginMenu::GetPlugin() == "")
    {
        Skins.Message(mtStatus, tr("Identifying Disc"));
        ShowSplashScreen(); 
    }
    int nrMatches = 0;

    // reset the state of all detection plugins
    mediaplugin* pPlugin = VdrcdSetup.mediaplugins.First();
    while(pPlugin) {
        pPlugin->reset();
        pPlugin = VdrcdSetup.mediaplugins.Next(pPlugin);
    }
    pCurDevice->mount();

    // now we know where it is really mounted
    mediaplugin::setCurrentDevice(pCurDevice);

    mediaplugin *lastMatched = VdrcdSetup.mediaplugins.First();
    pPlugin = VdrcdSetup.mediaplugins.First();
    while(pPlugin) 
    {
        //cerr << "mediad: current plugin " << pPlugin->getName() << endl;
        if(pPlugin->checkPluginAvailable()) 
        {
            pPlugin->searchPattern();
            if(strcmp(pPlugin->getName(), "filebrowser") != 0 && pPlugin->matches() || strcmp(pPlugin->getName(), "filebrowser")==0 && pPlugin->matches2(matchedPlugins)) {
                //cerr << "  found" << endl;
                lastMatched = pPlugin;
                matchedAnything = true;
                nrMatches++;
                matchedPlugins.push_back(pPlugin);
            }
        }
        pPlugin = VdrcdSetup.mediaplugins.Next(pPlugin);
    }
 
    //printf("----------pluginMediaD::MainMenuAction, vor switch(nrMatches) = %d-------\n", nrMatches);
    switch(nrMatches) {
        case 0:
        {
            break;
        }
        case 1:
        {
             //printf("----------pluginMediaD::MainMenuAction, vor checkStart, lastMactched = %d, %s----\n", (int)lastMatched, typeid(*lastMatched).name());
            if(checkStart(*lastMatched, AutoStartMode == 2)) { 
                Skins.Message(mtStatus, NULL);
                Skins.Flush();
                HideSplashScreen(lastMatched->getName());
                lastMatched->preStartUmount();
                return lastMatched->execute(); 
            }
            else
            {
                lastMatched->preStartUmount();
                HideSplashScreen("");
                return NULL;
            }
            break;
        }
        default:
        {
                    if(requestPluginMenu::GetPlugin() != "")
                    {
                            //finally execute the mediaplugin chosen in the last mediad call
                            std::string chosenPlugin = requestPluginMenu::GetPlugin();
                            //cout << "GetPlugin() = " << requestPluginMenu::GetPlugin().c_str() << endl;
                            requestPluginMenu::SetPlugin(""); //reset
                            pPlugin = VdrcdSetup.mediaplugins.First(); 
                            while(pPlugin) 
                            {   
                                    if(chosenPlugin == pPlugin->getName() && pPlugin->matches())
                                    {
                                            HideSplashScreen(pPlugin->getName());
                                            return pPlugin->execute();
                                    }
                                    pPlugin = VdrcdSetup.mediaplugins.Next(pPlugin);
                            }
                            matchedAnything = false;
                    }
                    else
                    {
                            //choose a mediaplugin from the list of matching plugins
                            //std::vector<mediaplugin*> matchedEntries;
                            /*pPlugin = VdrcdSetup.mediaplugins.First();
                            while(pPlugin) 
                            {
                                    if(pPlugin->matches())
                                    {
                                            //cout << "add matching plugin " << pPlugin->getName() << endl;
                                            matchedEntries.push_back(pPlugin);
                                    }
                                    pPlugin = VdrcdSetup.mediaplugins.Next(pPlugin);
                            }*/
                            Skins.Message(mtStatus, NULL);
                            Skins.Flush();
                            //return new requestPluginMenu(matchedEntries);
                            std::string title = std::string(tr("Choose action")) + ": "+ pCurDevice->getDeviceName();
                            return new requestPluginMenu(matchedPlugins, title);
                    }
        }
    } 
        bool mounted = pCurDevice->isMounted();
        const char *mountPoint = pCurDevice->getMountPoint();
        pCurDevice->umount(); //TB: why unmount it when it's already mounted??

        if(!matchedAnything && mounted && mountPoint)
        {
                Skins.Message(mtStatus, NULL);
                Skins.Flush();
                HideSplashScreen("");
        }
        else if(!matchedAnything && !mounted)
        { 
                Skins.Message(mtError, tr("Couldn't read disc!"));
                HideSplashScreen("");
        }
    return NULL;
}

cMenuSetupPage *pluginMediaD::SetupMenu(void) {
    return new cVdrcdMenuSetupPage;
}

bool pluginMediaD::SetupParse(const char *Name, const char *Value) {
    return VdrcdSetup.SetupParse(Name, Value);
}

bool pluginMediaD::checkStart(mediaplugin &plugin, bool alwaysAsk) {
    //printf("----------pluginMediaD::checkStart, alwaysAsk = %d, plugin.isAutoStart() = %d----------\n",  alwaysAsk, plugin.isAutoStart());
    if(!plugin.isAutoStart() || alwaysAsk) { 
        char* msg;
        asprintf(&msg, "%s %s?", tr("Start of"), tr(plugin.getDescription())); 
        DDD("Asking %s\n", msg);
        if(Interface->Confirm(msg)) {
            free(msg);
            return true;
        }
        free(msg);
        return false;
    } 
    return true;
} 

void pluginMediaD::ShowSplashScreen()
{ 
   //cout << endl << endl << endl << "########################ShowSplashScreen################" << endl << endl << endl;
   cControl::Shutdown(); //avoid deadlocks when replay paused
   cControl::Launch(new cSuspendCtl);
   cControl::Attach(); // make sure device is attach
}

void pluginMediaD::HideSplashScreen(const char *pluginName)
{
   //cout << "pluginMediaD::HideSplashScreen, pluginName = " << pluginName << endl;
   //Hide sreen if disc not identyfied and no plugin started that attaches new player
   if (std::string(pluginName) != "dvd" &&
       std::string(pluginName) != "vcd" &&
       std::string(pluginName) != "mplayer" &&
       std::string(pluginName) != "mp3" &&
       std::string(pluginName) != "cdda" &&
       std::string(pluginName) != "xinemediaplayer")
   {
       cControl::Shutdown();
   }
}

void pluginMediaD::CallFilebrowser(std::string path)
{
    //cout << "mediad:--------CallFilebrowser-------------" << endl;
    struct FileBrowserSetStartDir
    {
            char const *dir;
            char const *file;
    } startdir = { path.c_str(), "" };
    cPluginManager::CallAllServices("Filebrowser set startdir", &startdir);
    cRemote::CallPlugin("filebrowser");
}

bool pluginMediaD::Service(char const *id, void *data)
{
    if (strcmp(id, "SwitchOn") == 0 && data) {
        switchedOn = *(bool *)data;
        return true;
    }

    if (id && strcmp(id, "HideSplashScreen") == 0 ) {
        if( cSuspendCtl::IsActive() )
            cControl::Shutdown();
    }
    return false;
}

VDRPLUGINCREATOR(pluginMediaD); // Don't touch this!
