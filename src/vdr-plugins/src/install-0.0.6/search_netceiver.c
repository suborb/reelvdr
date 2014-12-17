#include "installmenu.h"
#include "search_netceiver.h"
#include "sysconfig_backup.h"
#include <vdr/sysconfig_vdr.h>
#include <vdr/tools.h>

bool HaveVlanDevice()
{
    cSysConfig_vdr& sysconfig = cSysConfig_vdr::GetInstance();
    const char* vlan = sysconfig.GetVariable("VLAN");
    if (vlan && strcasecmp(vlan, "yes")==0)
        return true;
    
    return false;
}

bool IsBridgeRequired()
{
    // bridge does not work correctly on ICE boards
    if (cPluginManager::GetPlugin("reelice"))
        return false;
    
    return IsNetworkDevicePresent("eth1");
}

bool VlanOnEth1()
{
    // should vlan device be created on eth1?
    // must be created on eth0 for netclients
    // eth1 for avantgardes
    
    return IsNetworkDevicePresent("eth1");
    
}

void SetVlanInSysconfig(int id)
{
    cString id_str = itoa(id);
    
    cSysConfig_vdr& sysconfig = cSysConfig_vdr::GetInstance();
    
    const char* oldId = sysconfig.GetVariable("VLAN_ID");
    
    // remove both eth0.X and vlanX
    cString rm_prev_vlan_id = cString::sprintf("vconfig rem eth0.%s; vconfig rem vlan%s; vconfig rem eth1.%s", oldId, oldId, oldId);
    SystemExec(rm_prev_vlan_id);
    
    sysconfig.SetVariable("VLAN", "yes");
    sysconfig.SetVariable("VLAN_ID", id_str);
    
    if (IsBridgeRequired())
        sysconfig.SetVariable("NC_BRIDGE", "yes");
    else
        sysconfig.SetVariable("NC_BRIDGE", "no");
    
    // vlan on which network interface?
    if (VlanOnEth1())
        sysconfig.SetVariable("ETH0_BRIDGE", "Ethernet 1");
    else
        sysconfig.SetVariable("ETH0_BRIDGE", "-");
    
    sysconfig.Save();
}


/*
 * cSearchNetCeiver : Search for netceiver in a thread
 */
cSearchNetCeiver::cSearchNetCeiver() : cThread("search netceiver"),  found(false) 
{
  status.local_search = 1;
  status.vlan_id = MIN_VLAN_ID;
}

cSearchNetCeiver& cSearchNetCeiver::GetInstance()
{
    static cSearchNetCeiver* instance = NULL; 
    
    if (!instance) 
        instance = new cSearchNetCeiver;;
    
    return *instance;
}

bool cSearchNetCeiver::SearchStatus(SearchStatus_t &status_, bool &Found)
{
    status_ = status;
    status_.finished = !Running();
    Found   = found && !Running(); /* wait for setup-net restart to complete*/
    
    return true;
}

void cSearchNetCeiver::Stop()
{
    Cancel(1);
}


void cSearchNetCeiver::Action()
{
    cSysConfigBackup sysconfigBackup;
    const char* keys[] = {"VLAN", "VLAN_ID", "NC_BRIDGE", "ETH0_BRIDGE"};
    SysconfigKeys_t keysToBackup(keys, keys+4);
    sysconfigBackup.SetBackupKeys(keysToBackup);
    sysconfigBackup.BackupSysConfig();
    
    status.local_search = 1;
    status.finished = false;
    
    int &id = status.vlan_id;
    
    id    = MIN_VLAN_ID;
    found = false;
    
    // If vlan device is up and still netceiver is not found, 
    // lets check if there is a local netceiver
    /// check for netceiver in eth0
    if (HaveVlanDevice()) {
        // remove vlan device 
        cSysConfig_vdr& sysconfig = cSysConfig_vdr::GetInstance();
        sysconfig.SetVariable("VLAN", "no");
        sysconfig.SetVariable("ETH0_BRIDGE", "-");
        sysconfig.Save();
        
        // reinit mcli to find netceiver in eth0
        char mcli_dev[] = "eth0";
        cPlugin *mcli = cPluginManager::GetPlugin("mcli");
        if (mcli) mcli->Service("Reinit", mcli_dev);
        
        status.local_search = 1; /* searching locally */
        
        // wait for netceiver to be found : ~30secs
        int count = 30;
        while (count-- > 0 && !found && Running()) {
            cCondWait::SleepMs(1000); // 1 sec
            
            // check for netceiver
            found = CheckNetceiver();
        }
    }
    
    if (!found)
        status.local_search = 0; /* searching network */
    
    
    /** remove the bridge, beacuse devices that belong to a bridge should 
     * not be used for anything.(clarification due to Deti)
     *
     * Also, removing a vlan device, and creating it later automatically 
     * adds it to the bridge! So, netceiver is not found. 
     */
    SystemExec("ifconfig br0 down; brctl delbr br0");
    
    
    while (!found && Running() && id <= MAX_VLAN_ID) {
        // save vlan setting from sysconfig    
        SetVlanInSysconfig(id);


        
        if (!VlanOnEth1()) {
            cString eth_dev = cString::sprintf("eth0.%d", id);
            char *eth_dev_str = strdup(*eth_dev);
            
            cString up_vlan_id = cString::sprintf("vconfig set_name_type DEV_PLUS_VID_NO_PAD; vconfig add eth0 %d; ifconfig eth0.%d up ", id, id);
            printf("exec: %s", *up_vlan_id);
            SystemExec(up_vlan_id);
            
            cPlugin *mcli = cPluginManager::GetPlugin("mcli");
            if (mcli) mcli->Service("Reinit", eth_dev_str);
            
            else esyslog("mcli plugin not available to reconfigure");
            
            if (eth_dev_str) { 
                free(eth_dev_str); 
            }
            
        } else {
#if 0
            /* sets up vlan and adds it to bridge */
             SystemExec("setup-net restartvlan");
            
#else
            /* just create a vlan on eth1 and see if netceiver shows up*/
            cString up_vlan_id = cString::sprintf("vconfig set_name_type VLAN_PLUS_VID_NO_PAD; vconfig add eth1 %d; ifconfig vlan%d up ", id, id);
            
            printf("\033[0;91mexec: %s\033[0m\n", *up_vlan_id);
            SystemExec(up_vlan_id);
            char vlanDev[16];
            snprintf(vlanDev, 15, "vlan%d", id);
            cPlugin *mcli = cPluginManager::GetPlugin("mcli");
            if (mcli) mcli->Service("Reinit", vlanDev);
            else esyslog("mcli plugin not available to reconfigure");
#endif
        }
        
        // wait for netceiver to be found : ~30secs
        int count = 30;
        while (count-- > 0 && !found && Running()) {
            cCondWait::SleepMs(1000); // 1 sec
            
            // check for netceiver
            found = CheckNetceiver();
        }
        
        if (found)
            break;
        else 
            id++;
    } // while
    
    // write all configs correctly to /default/mcli or /network/interfaces
    // so that when vdr/box restart all configs are valid and recent
    if (found) 
        SystemExec("setup-net restart");
    
    // if !found, revert vlan settings in sysconfig to "orig"
    if (!found) {
        isyslog("Netceiver search failed. Restore sysconfig.");
        sysconfigBackup.RestoreSysConfig();
    }
    
    status.finished = true;
}
