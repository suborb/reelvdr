#include "install_client.h"
#include "install_multiroom.h"
#include <vdr/tools.h>
#include <vdr/sysconfig_vdr.h>


#define IS_CLIENT IsClient()
#define IS_AVANTGARDE IsAvantgarde()

#define osScanMultiRoomServer osUser8

cInstallMultiroom::~cInstallMultiroom()
{
    // Delete old images from OSD
    cPluginManager::CallAllServices("setThumb", NULL);
    
    Setup.Save();
    
    
    /** wait for netceiver to "appear", since the network was 
     *  restarted in this menu
     */
    int count = 0;
    while(!CheckNetceiver() && count < 10) {
        printf("\033[0;91m waiting for netceiver... %d\033[0m\n", count);
        
        Skins.Message(mtInfo, 
                      cString::sprintf(tr("Reception check... %d%%"), count*5),
                      1); // skins.message waits for 1 second
        
        // sleep another second
        sleep(1);
        
        count ++;
    }
}


cInstallMultiroom::cInstallMultiroom(std::string harddiskDevice_, bool hddAvail) : 
    cInstallSubMenu(tr("MultiRoom")),
   hddDevice(harddiskDevice_), hddAvailable(hddAvail), 
   enableMultiroom(false), useAsServer(false), noOfClients(Setup.MaxMultiRoomClients)
{
    enableMultiroom = (Setup.ReelboxModeTemp != eModeStandalone);
    useAsServer = (Setup.ReelboxModeTemp == eModeServer);
    
    SetCols(35);
    
    Set();
}

eOSState cInstallMultiroom::ProcessKey(eKeys Key)
{
    bool hadSubMenu = HasSubMenu();
    eOSState state = cOsdMenu::ProcessKey(Key);
    
     // multiroom-server scan menu has configured clientmode, go to next step
    if (hadSubMenu && state == osUser1) {
        return osUser1; // go to next step
    }
    
    if (HasSubMenu()) return state;
    
    // redraw menu, might have to shown/blend options
    if ((NORMALKEY(Key) == kLeft || NORMALKEY(Key) == kRight) && state != osUnknown)  {
        printf("redraw menu\n");
        Set();
    }
    
    if (NORMALKEY(Key) == kOk) {

        eReelboxMode choice;
        
        if (!enableMultiroom)
            choice = eModeStandalone;
        else if (useAsServer) 
            choice = eModeServer;
        else if ((!useAsServer || IS_CLIENT) && state == osScanMultiRoomServer) 
            choice = eModeClient;
        else if (!useAsServer && state != osScanMultiRoomServer)
            return osContinue; // ignore that kOk
             
        switch(choice) {
        case eModeStandalone:
            Setup.ReelboxMode = eModeStandalone;
            Skins.Message(mtInfo, tr("Please wait..."));
            ConfigureAsStandalone();
            break;
            
        case eModeClient:
        {
            cPluginManager::CallAllServices("setThumb", NULL); // clear images
            cOsdMenu* menu = new cInstallNetclient(hddDevice, hddAvailable, NULL);
            return AddSubMenu(menu);
        }
            break;
            
        case eModeServer:
            Setup.ReelboxMode = eModeServer;
            printf("Configuring as server ...\n");
            Skins.Message(mtInfo, tr("Please wait..."));
            ConfigureAsServer();
            break;
            
        default:
            esyslog("(%s:%d) unknown 'choice' %d", __FILE__, __LINE__, choice);
            return osContinue;
            break;
        } // switch
        
        return osUser1;
    } // if (kOk)
    
    return state;
    
} // ProcessKey()


bool cInstallMultiroom::Save()
{
    return false;
}


void ShowBackPlateImage(bool MultiRoomEnabled)
{
    struct StructImage Image;

    Image.x = 209;
    Image.y = 284;
    Image.h = 116;
    Image.w = 200;
    Image.blend = true;
    Image.slot = 0; 
    
    bool isICE = (cPluginManager::GetPlugin("reelice") != NULL);
    
    if (IsClient())
    {
        
    } else if (IsAvantgarde()) {
        if (isICE)
            snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install/Reelbox3", MultiRoomEnabled?"Reelbox3-left-plain.png":"Reelbox3-left-NetCeiver-connection.png");
        else
            snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install", MultiRoomEnabled?"Ethernet-left.png":"TV-Connections-System-left.png");
    } else {
        esyslog("(%s:%d)ERROR: Neither Client nor Avantgarde ? ", __FILE__, __LINE__);
    }
    
    cPluginManager::CallAllServices("setThumb", (void *)&Image);
    Image.x = 209+161;
    Image.slot = 1;

    
    if (IsClient())
    {
        
    } else if (IsAvantgarde()) {
        if (isICE)
            snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install/Reelbox3", MultiRoomEnabled?"Reelbox3-right-Ethernet.png":"Reelbox3-right-plain.png");
        else
            snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install", MultiRoomEnabled?"Ethernet-right.png":"TV-Connections-System-right.png");
    } else {
        esyslog("(%s:%d)ERROR: Neither Client nor Avantgarde ? ", __FILE__, __LINE__);
    }
    

    cPluginManager::CallAllServices("setThumb", (void *)&Image);
    
}


void cInstallMultiroom::Set()
{
    int currentItem = Current();
    cOsdItem *item = NULL;
    
    Clear();

    cString buf;
    buf = tr("In this step you can configure the Reel MultiRoom system.");
    AddFloatingText(buf, 45);
    Add(new cOsdItem("", osUnknown, false));
    
    buf = tr("Enable MultiRoom");
    Add(new cMenuEditBoolItem(buf, &enableMultiroom));
    
    if (enableMultiroom)
    {
        Add(new cOsdItem("", osUnknown, false));
        
        if (IS_AVANTGARDE) 
        {
            buf = tr("Use this Reelbox as recording server?");
            Add(item = new cMenuEditBoolItem(buf, &useAsServer));
            if (useAsServer) {
                buf = tr("No. of Clients in the network");
                Add(item = new cMenuEditIntItem(buf, &noOfClients, 1, 6));
            }
        } 
        
        if (!useAsServer)
        {
            Add(new cOsdItem("", osUnknown, false));
            
            buf = tr("Scan for MultiRoom server now");
            item = new cOsdItem(buf, osScanMultiRoomServer, true);
            Add(item);
        }
        SetCurrent(item);
    }
    
    // Set the next item as current
    item = Get(currentItem+1);
    
    // skip all non-selectables items like blanklines and texts
    while(item && !item->Selectable()) item = Next(item);
    
    if (item) 
        SetCurrent(item);
    else 
        SetCurrent(Last());
    
    // Draw Image
    ShowBackPlateImage(enableMultiroom);
    
    Display();
}



/* this function is no longer used, configured as client in cInstallClientMenu() 
*/
void cInstallMultiroom::ConfigureAsClient()
{
    // Remove symlinks in /etc/rcX.d/ and stop MySQL-Server
    SystemExec("/etc/init.d/mysql stop");
    SystemExec("update-rc.d -f mysql remove");
    
    std::string buf = std::string("getConfigsFromAVGServer.sh ") + clientServer.IP + " all";
    SystemExec(buf.c_str());
    cPluginManager::CallAllServices("reload favourites list");

    cPluginManager::CallAllServices("MultiRoom disable");
    Setup.ReelboxModeTemp = eModeStandalone;
    Setup.ReelboxMode     = eModeClient;

    strcpy(Setup.NetServerIP, clientServer.IP.c_str());
    strcpy(Setup.NetServerName, clientServer.Name.c_str());
    strcpy(Setup.NetServerMAC, clientServer.MAC.c_str());
    
    //check and set ReelboxModeTemp
    bool DatabaseAvailable = false;
    cPlugin *p = cPluginManager::GetPlugin("avahi");
    if(p)
        p->Service("Avahi check NetServer", &DatabaseAvailable);
    
    Setup.ReelboxModeTemp = DatabaseAvailable?eModeClient:eModeStandalone;
#ifdef USE_MYSQL
            Timers.LoadDB();
#endif
}


void cInstallMultiroom::ConfigureAsStandalone()
{
    cPluginManager::CallAllServices("MultiRoom disable");

    UseAllTuners();
     
    SystemExec("avahi-publish-avg-mysql.sh -2"); // -2 means kill avahi-publish

    cSysConfig_vdr& sysconfig = cSysConfig_vdr::GetInstance();
    // Use local harddisk    
    std::string UUID = GetUUID(hddDevice);
    sysconfig.SetVariable("MEDIADEVICE", UUID.c_str());
    sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");
    
    sysconfig.Save();
    
    SystemExec("setup-mediadir", true);
    
    Setup.ReelboxModeTemp = eModeStandalone;
    Setup.ReelboxMode     = eModeStandalone;
    
    // Remove symlinks in /etc/rcX.d/ and stop MySQL-Server
    SystemExec("/etc/init.d/mysql stop", true);
    SystemExec("update-rc.d -f mysql remove", true);
    
    Timers.Save();
    
}




void cInstallMultiroom::ConfigureAsServer()
{
    printf("\033[0;91mConfigureAsServer\033[0m\n");
    Timers.Save();
    Setup.ReelboxModeTemp = eModeServer;
    Setup.ReelboxMode     = eModeServer;
    
    Setup.MaxMultiRoomClients = noOfClients;
    AllocateTunersForServer(noOfClients);

    cPluginManager::CallAllServices("MultiRoom disable");
    cPluginManager::CallAllServices("MultiRoom enable");
    // wait before restarting network.
    // because vlan2 is removed otherwise!
    Skins.Message(mtInfo,
                  cString::sprintf(tr("Configuring as server: %d%%"), 20), 1);
    sleep(3);
    

    Skins.Message(mtInfo,
                  cString::sprintf(tr("Configuring as server: %d%%"), 30), 1);
#ifdef USE_MYSQL
            Timers.LoadDB();

#endif
            
#if 0 // done in a thread in setup when Service("MultiRoom enable") is called
    if(!SystemExec("mysqld --version"))
    {
        printf("\033[0;41m %s(%i): %s \033[0m\n", __FILE__, __LINE__, "enabling MySQL");
        // Create symlinks in /etc/rcX.d/ and start MySQL-Server
        SystemExec("update-rc.d mysql start 19 2 3 4 5 . stop 21 0 1 6 .");
        SystemExec("/etc/init.d/mysql start");
    } else {
        esyslog("No mysql");
    }
#endif

    cSysConfig_vdr& sysconfig = cSysConfig_vdr::GetInstance();
    
    // restart of network should be necessary only if sysconfig settings were changed
    // count the changes
    int sysconfigChanged = 0;
    
    if (!strcasecmp(DefaultNetworkDevice(), "eth0"))
        sysconfig.SetVariable("ETH0_BRIDGE", "-", sysconfigChanged);
    else
    {
        sysconfig.SetVariable("ETH0_BRIDGE", "Ethernet 1", sysconfigChanged);
        sysconfig.SetVariable("NC_BRIDGE", "yes", sysconfigChanged);
    }

    
    sysconfig.SetVariable("VLAN", "yes", sysconfigChanged);
    sysconfig.SetVariable("VLAN_ID", "2", sysconfigChanged);
    
    sysconfig.Save();
    Skins.Message(mtInfo,
                  cString::sprintf(tr("Configuring as server: %d%%"), 50), 1);
    if (sysconfigChanged) {
        isyslog("Sysconfig changed %d. Restarting network", sysconfigChanged);
        SystemExec("setup-net restart");
    } else
        isyslog("Sysconfig not changed. NOT restarting network");
                
    Skins.Message(mtInfo,
                  cString::sprintf(tr("Configuring as server: %d%%"), 80), 1);
}
