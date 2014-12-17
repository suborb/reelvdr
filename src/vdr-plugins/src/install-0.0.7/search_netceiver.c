#include "installmenu.h"
#include "search_netceiver.h"
#include "sysconfig_backup.h"
#include <vdr/sysconfig_vdr.h>
#include <vdr/tools.h>


#define RUN_SETUP_NET_FOR_VLAN 1

/** NetWorkRestartedRecently(seconds) 
 *  Checks if the network was restarted in the last 'before' seconds and returns true
 *  else returns false
 *
 *  current implementation looks at the modified-timestamp of /etc/network/interfaces
 */
bool NetworkRestartedRecently(int before)
{
    struct stat64 st;
    if (stat64("/etc/network/interfaces", &st) != 0)
        return false; // failed to get status info of file
    
    return (time(NULL) - st.st_mtime <= before);
}

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

void cSearchNetCeiver::WaitForNetceiver(int TimeoutSecs)
{
    // wait for netceiver to be found : ~30secs
    
    while (TimeoutSecs-- > 0 && !found && Running()) {
        printf("Waiting for netceiver\n");
        cCondWait::SleepMs(1000); // 1 sec
        
        // check for netceiver
        found = CheckNetceiver();
    }    
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
    
    // network restart is need to write out the configuration into /etc/network/interfaces
    bool needsNetworkRestart = true;
    
    cSysConfig_vdr& sysconfig = cSysConfig_vdr::GetInstance();
    
    // If the network was restarted in the last 20 seconds
    // wait for it to "stabilize" and check for NetCeiver
    if (NetworkRestartedRecently(20)) {
        
        const char* v = sysconfig.GetVariable("VLAN");
        if (v || strcmp(v, "no")==0)
            status.local_search = 1; // no vlan ==> local NetCeiver
        else
            status.local_search = 0; // vlan ==> non-local netCeiver
        
        WaitForNetceiver();
    }
    
    // found the NetCeiver without chaning VLAN settings
    // therefore no need to restart NetCeiver
    if (found)
        needsNetworkRestart = false;
    else 
        // start searching for NetCeiver locally
        status.local_search = 1;
    
    // If vlan device is up and still netceiver is not found, 
    // lets check if there is a local netceiver
    /// check for netceiver in eth0
    if (HaveVlanDevice() && !found && Running()) {
        // remove vlan device 
        sysconfig.SetVariable("VLAN", "no");
        sysconfig.SetVariable("ETH0_BRIDGE", "-");
        sysconfig.SetVariable("NC_BRIDGE", "no");
        sysconfig.Save();
        SystemExec("setup-net restart");
        needsNetworkRestart = false;
        
        // reinit mcli to find netceiver in eth0
        char mcli_dev[] = "eth0";
        cPlugin *mcli = cPluginManager::GetPlugin("mcli");
        if (mcli) mcli->Service("Reinit", mcli_dev);
        
        status.local_search = 1; /* searching locally */
        
        WaitForNetceiver();
    }
    
    if (!found) {
        status.local_search = 0; /* searching network */
    
#if !RUN_SETUP_NET_FOR_VLAN
    
    /** remove the bridge, beacuse devices that belong to a bridge should 
     * not be used for anything.(clarification due to Deti)
     *
     * Also, removing a vlan device, and creating it later automatically 
     * adds it to the bridge! So, netceiver is not found. 
     */
        isyslog("Removing bridge");
        //SystemExec("ifconfig br0 down; brctl delbr br0");
        sysconfig.SetVariable("NC_BRIDGE", "no");
        sysconfig.Save();
        SystemExec("setup-net restart");
        needsNetworkRestart = false;
#endif
    }
    
    
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
            
            needsNetworkRestart = true;
        } else {
#if RUN_SETUP_NET_FOR_VLAN
            /* sets up vlan and adds it to bridge */
             SystemExec("setup-net restart");
             needsNetworkRestart = false;
#else
            needsNetworkRestart = true;
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
        
        WaitForNetceiver();
        
        if (found)
            break;
        else 
            id++;
    } // while
    
    // write all configs correctly to /default/mcli or /network/interfaces
    // so that when vdr/box restart all configs are valid and recent
    if (found && needsNetworkRestart) 
        SystemExec("setup-net restart");
    
    /// TODO if !found, revert vlan settings in sysconfig to "orig"
    if(!found) {
        isyslog("Netceiver search failed. Restore sysconfig.");
        sysconfigBackup.RestoreSysConfig();
    }
    
    status.finished = true;
}
