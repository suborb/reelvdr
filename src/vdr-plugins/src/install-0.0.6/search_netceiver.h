#ifndef SEARCH_NETCEIVER__H
#define SEARCH_NETCEIVER__H

#include <vdr/thread.h>

#define MIN_VLAN_ID 2
#define MAX_VLAN_ID 3

/* set appropriate params in sysconfig so that vlan_id = id is set by setup-net script */
void SetVlanInSysconfig(int id);

typedef struct SearchStatus {
    int vlan_id;
    int local_search; /* 1: true, not searching on vlan , 0:false searching on vlans*/
    bool finished;    /* true if search has ended, else false*/
} SearchStatus_t;



/**
 * @brief The cSearchNetCeiver class 
 *
 * Thread to search for netceiver
 *      - Wait for NetCeiver, if necessary
 *      - Try eth0 : local/internal NetCeiver
 *      - Try vlans vlan_id = 1...9
 *      - write network configs, if necessary
 *
 *    1. If network was recently restarted ( < 20 seconds)
 *          wait for netceiver to appear
 *
 *    2. If non-local NetCeiver was searched for, search for local NetCeiver
 *          ie. if vlan was set and no NetCeiver was found
 *          disable bridge and search for NetCeiver in 'eth0'
 *
 *    3. If no NetCeiver was found, search for NetCeiver in the network ie. 
 *          in the vlan (vlan id : 1...9)
 *
 *          create vlan on "default" interface.
 *            default interface is eth1 if it exists else eth0.
 *
 *          On reelboxes based on ICE, there should be no bridge device.
 *              ICEboxes have problems with bridge devices.
 *
 *    4. Still no NetCeiver ? Rescan if user chooses to. ie. GOTO #1
 *
 *    5. NetCeiver found ?
 *          Write configuration into /etc/network/interfaces & /etc/default/mcli
 *          so that after reboot, the system still finds the NetCeiver
 */

class cSearchNetCeiver: public cThread
{
private:
    cSearchNetCeiver();
    
    SearchStatus_t status;
    bool found;
     
protected:
    void Action();
    
public:
    static cSearchNetCeiver& GetInstance();
    bool SearchStatus(SearchStatus_t& status, bool& found);
    void Stop();
};

#endif /*SEARCH_NETCEIVER__H*/
