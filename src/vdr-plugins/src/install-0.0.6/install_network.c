/***************************************************************************
 *   Copyright (C) 2009 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                 2009 Additions by Tobias Bratfisch                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 ***************************************************************************
 *
 * install_network.c
 *
 ***************************************************************************/

#include "install_network.h"
#include "ipnumitem.h"
#include <vdr/sysconfig_vdr.h>

#define DHCP_TIMEOUT 7

#define osConfigWLAN osUser4

bool IsWirelessInterfacePresent() 
{
    // TODO : Use libnl to get number of wireless interfaces available?
    cString wifi_count = "cat /proc/net/wireless";
    cReadLine line;
    int count = 0;
    FILE* f = popen(wifi_count, "r");
    
    if (f) {
        while (line.Read(f)) 
            count ++;
        
        pclose(f);
    } // if
    
    return count > 2;
}

#define WIFI_DEV_AVAILABLE IsWirelessInterfacePresent()

enum eNetworkState
{ eNoNetwork, eNoIP, eGotIP, eDHCP, eManualNetwork };
static const char *allowedChars = " .,:;+-*~?!\"$%&/\()=`'_abcdefghijklmnopqrstuvwxyz0123456789";

cInstallNet::cInstallNet():cInstallSubMenu(tr("Network"))
{

    loopMode = false;           /* fallback just to be sure */
    orig_use_internal = old_use_internal = 0;
    orig_use_dhcp = old_use_dhcp = 0;
    hostname[0] = domain[0] = ipaddress[0] = netmask[0] = gateway[0] = nameserver[0] = '\0';
    dhcp_ipaddress[0] = dhcp_netmask[0] = dhcp_gateway[0] = dhcp_nameserver[0] = '\0';
    orig_hostname[0] = orig_domain[0] = orig_ipaddress[0] = orig_netmask[0] = orig_gateway[0] = orig_nameserver[0] = '\0';

    Load();                     // Load settings

    old_use_internal = use_internal;
    old_use_dhcp = use_dhcp;
    strcpy(orig_hostname, hostname);
    strcpy(orig_domain, domain);
    strcpy(orig_ipaddress, ipaddress);
    strcpy(orig_netmask, netmask);
    strcpy(orig_gateway, gateway);
    strcpy(orig_nameserver, nameserver);

    if (use_internal)
    {
        if (CheckIP())
            NetworkState_ = eGotIP;
        else
            NetworkState_ = eNoIP;
    }
    else
        NetworkState_ = eNoNetwork;

    Set();
}

cInstallNet::~cInstallNet()
{
    // Delete old images from OSD
    cPluginManager::CallAllServices("setThumb", NULL);
}

eOSState cInstallNet::ProcessKey(eKeys Key)
{
    eOSState result = cOsdMenu::ProcessKey(Key);

    if (result != osContinue)
    {
        switch (NetworkState_)
        {
        case eNoNetwork:
            if (result == osUser1)  // Change settings
            {
                use_internal = true;
                use_dhcp = true;
                Skins.Message(mtStatus, tr("Please wait..."));
                Save();
                sleep(2);
                if (RunDHCP())
                    NetworkState_ = eDHCP;
                else
                {
                    use_dhcp = false;
                    NetworkState_ = eManualNetwork;
                    // Delete old images from OSD
                    cPluginManager::CallAllServices("setThumb", NULL);
                }
                Set();
                return osContinue;
            }
            else if (result == osUser2)
                return osUser1; // Next Menu
            break;
        case eNoIP:
            if (result == osUser1)  // Change settings
            {
                NetworkState_ = eManualNetwork;
                use_dhcp = false;
                Set();
                return osContinue;
            }
            else if (result == osUser2) // Keep settings
                return osUser1; // Next Menu
            else if (result == osUser3) // Disable Network
            {
                use_internal = false;
                Skins.Message(mtInfo, tr("Setting changes - please wait"));
                Save();
                Skins.Message(mtInfo, tr("Changes done"));
                return osUser1; // Next Menu
            }
            break;
        case eGotIP:
            if (result == osUser1)  // Keep settings
                return osUser1; // Next Menu
            else if (result == osUser2) // Change settings
            {
                if (use_dhcp)
                {
                    use_dhcp = false;
                    NetworkState_ = eManualNetwork;
                }
                else
                {
                    use_dhcp = true;
                    Skins.Message(mtStatus, tr("Please wait..."));
                    Save();
                    if (RunDHCP())
                        NetworkState_ = eDHCP;
                    else
                    {
                        use_dhcp = false;
                        NetworkState_ = eManualNetwork;
                    }
                }
                Set();
                return osContinue;
            }
            else if (result == osUser3) // Disable Network
            {
                use_internal = false;
                Skins.Message(mtInfo, tr("Setting changes - please wait"));
                Save();
                Skins.Message(mtInfo, tr("Changes done"));
                return osUser1; // Next Menu
            }
            break;
        case eDHCP:
            if (result == osUser1)  // Keep settings
                return osUser1; // Next Menu
            else if (result == osUser2) // Change settings
            {
                NetworkState_ = eManualNetwork;
                use_dhcp = false;
                Set();
                return osContinue;
            }
            break;
        case eManualNetwork:
            if (Key == kOk)
            {
                Skins.Message(mtInfo, tr("Setting changes - please wait"));

                /// HACK: force save
                // else settings are not saved here
                // when eGotIP -> DHCP -> eManualNetwork
                old_use_dhcp = !use_dhcp;

                Save();
                Skins.Message(mtInfo, tr("Changes done"));
                return osUser1;
            }
            break;
        }
        if (result == osConfigWLAN) {
            cPlugin *p = cPluginManager::GetPlugin("setup");
            if (p) {
                struct InstallWizMenu { cOsdMenu*menu; const char*title;};
                InstallWizMenu wiz;
                cString wlanTitle = cString::sprintf("%s - %s",
                                                     tr(MAINMENUENTRY),
                                                     tr("Network (WLAN)"));
                wiz.title = *wlanTitle;
                wiz.menu = NULL;
                p->Service("Install Wizard WLAN setup menu", &wiz);
                
                if (wiz.menu) 
                    AddSubMenu(wiz.menu);
                else 
                    esyslog("No menu got from setup plugin for wlan\n");
                
            } else 
                Skins.Message(mtError, 
                              tr("setup plugin missing: cannot configure WLAN"));
            
            return osContinue;
        }
    }

    if ((old_use_internal != use_internal) || (old_use_dhcp != use_dhcp))
    {
        int current = Current();
        Set();
        SetCurrent(Get(current));
        Display();
        old_use_internal = use_internal;
        old_use_dhcp = use_dhcp;
    }

    return result;
}

void cInstallNet::Set()
{
    Clear();
    char strBuff[64];

    switch (NetworkState_)
    {
    case eNoNetwork:
        Add(new cOsdItem(tr("Network disabled"), osUnknown, false));
        Add(new cOsdItem("", osUnknown, false), false, NULL);
        Add(new cOsdItem(tr("Do you want to enable the network?"), osUnknown, false));
        sprintf(strBuff, "   - %s", tr("Yes"));
        Add(new cOsdItem(strBuff, osUser1, true));
        sprintf(strBuff, "   - %s", tr("No"));
        Add(new cOsdItem(strBuff, osUser2, true));

        break;
    case eNoIP:
        Add(new cOsdItem(tr("Do you want to set up the network now?"), osUnknown, false));
        sprintf(strBuff, "   - %s", tr("Yes"));
        Add(new cOsdItem(strBuff, osUser1, true));
        sprintf(strBuff, "   - %s", tr("No"));
        Add(new cOsdItem(strBuff, osUser2, true));
        sprintf(strBuff, "   - %s", tr("Disable Network"));
        Add(new cOsdItem(strBuff, osUser3, true));
        break;
    case eGotIP:
    case eDHCP:
        if (use_dhcp)
        {
            Add(new cOsdItem(tr("These settings are retrieved by DHCP:"), osUnknown, false));
            Add(new cOsdItem("", osUnknown, false), false, NULL);
            sprintf(strBuff, "%s:\t%s", tr("IP Address"), dhcp_ipaddress);
            Add(new cOsdItem(strBuff, osUnknown, false));
            sprintf(strBuff, "%s:\t%s", tr("Network mask"), dhcp_netmask);
            Add(new cOsdItem(strBuff, osUnknown, false));
            Add(new cOsdItem("", osUnknown, false));
            sprintf(strBuff, "%s:\t%s", tr("Gateway"), dhcp_gateway);
            Add(new cOsdItem(strBuff, osUnknown, false));
            sprintf(strBuff, "%s:\t%s", tr("Nameserver"), dhcp_nameserver);
            Add(new cOsdItem(strBuff, osUnknown, false));
        }
        else
        {
            Add(new cOsdItem(tr("Current settings:"), osUnknown, false));
            sprintf(strBuff, "%s:\t%s", tr("IP Address"), ipaddress);
            Add(new cOsdItem(strBuff, osUnknown, false));
            sprintf(strBuff, "%s:\t%s", tr("Network mask"), netmask);
            Add(new cOsdItem(strBuff, osUnknown, false));
            Add(new cOsdItem("", osUnknown, false));
            sprintf(strBuff, "%s:\t%s", tr("Gateway"), gateway);
            Add(new cOsdItem(strBuff, osUnknown, false));
            sprintf(strBuff, "%s:\t%s", tr("Nameserver"), nameserver);
            Add(new cOsdItem(strBuff, osUnknown, false));
        }
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem(tr("Do you want to use these settings?"), osUnknown, false));
        sprintf(strBuff, "   - %s", tr("Yes, store"));
        Add(new cOsdItem(strBuff, osUser1, true));
        //if (use_dhcp)
            sprintf(strBuff, "   - %s", tr("No, configure network manually"));
       // else
       //     sprintf(strBuff, "   - %s", tr("No"));
        Add(new cOsdItem(strBuff, osUser2, true));
        
        if (WIFI_DEV_AVAILABLE) {
            cString text = cString::sprintf("   - %s", tr("No, configure WiFi"));
            Add(new cOsdItem(text, osConfigWLAN, true));
        }
        
#if 0
        if (NetworkState_ == eGotIP)
        {
            sprintf(strBuff, "   - %s", tr("Disable Network"));
            Add(new cOsdItem(strBuff, osUser3, true));
        }
#endif

        break;
    case eManualNetwork:
        Add(new cOsdItem(tr("Manual network setting"), osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cMenuEditBoolItem(tr("Use DHCP"), &use_dhcp, tr("no"), tr("yes")), true, NULL);
        if (!use_dhcp)
        {
            Add(new cMenuEditIpNumItem(tr("IP Address"), ipaddress), true, NULL);
            Add(new cMenuEditIpNumItem(tr("Network mask"), netmask), true, NULL);
            Add(new cOsdItem("", osUnknown, false));
            Add(new cMenuEditIpNumItem(tr("Gateway"), gateway), true, NULL);
            Add(new cMenuEditIpNumItem(tr("Nameserver"), nameserver), true, NULL);
        }
        Add(new cOsdItem("", osUnknown, false));
        Add(new cMenuEditStrItem(tr("Hostname"), hostname, 16, allowedChars), true, NULL);
        Add(new cMenuEditStrItem(tr("Domain"), domain, 16, allowedChars), true, NULL);
    }
    Display();
}


/**
 * @brief cInstallNet::Load() 
 *
 * load the settings of DEFAULT network device from sysconfig
 * 
 */

void cInstallNet::Load()
{
    cSysConfig_vdr & sysconfig = cSysConfig_vdr::GetInstance();

    const char *value = NULL;
    char dev[8] = {0} ; // the active network device, in UPPER case
    
    const char *def_dev = DefaultNetworkDevice();
    for (int i = 0; def_dev[i] && i < 7; ++i) dev[i] = toupper(def_dev[i]);
    
    if ((value = sysconfig.GetVariable(dev)) && !strcmp(value, "yes"))
        orig_use_internal = use_internal = 1;
    else
        orig_use_internal = use_internal = 0;

    cString eth_use_dhcp = cString::sprintf("%s_USE_DHCP", dev);
    cString eth_ip       = cString::sprintf("%s_IP", dev);
    cString eth_netmask  = cString::sprintf("%s_NETMASK", dev);
    
    if ((value = sysconfig.GetVariable(eth_ip)))
        strcpy(ipaddress, value);

    if ((value = sysconfig.GetVariable(eth_netmask)))
        strcpy(netmask, value);
    
    if ((value = sysconfig.GetVariable(eth_use_dhcp)) && !strcmp(value, "yes"))
        orig_use_dhcp = use_dhcp = 1;
    else
        orig_use_dhcp = use_dhcp = 0;

    if (sysconfig.GetVariable("HOSTNAME"))
        strcpy(hostname, sysconfig.GetVariable("HOSTNAME"));

    if (sysconfig.GetVariable("DOMAIN"))
        strcpy(domain, sysconfig.GetVariable("DOMAIN"));

    if (sysconfig.GetVariable("GATEWAY"))
        strcpy(gateway, sysconfig.GetVariable("GATEWAY"));

    if (sysconfig.GetVariable("NAMESERVER"))
        strcpy(nameserver, sysconfig.GetVariable("NAMESERVER"));
}


/**
 * @brief cInstallNet::Save :- Save settings to DEFAULT network device in sysconfig
 *
 * if DEFAULT network device is "eth0", 
 * make sure eth1 is set to "no" in sysconfig
 *
 * @return 
 */

bool cInstallNet::Save()
{
    cSysConfig_vdr & sysconfig = cSysConfig_vdr::GetInstance();

    sysconfig.SetVariable("HOSTNAME", hostname);
    sysconfig.SetVariable("NAMESERVER", nameserver);
    if (strcmp(nameserver, orig_nameserver))
    {
        FILE *resolv = fopen("/etc/resolv.conf", "a");  // TB: why append??
        if (resolv)
        {
            fprintf(resolv, "nameserver %s\n", nameserver);
            fclose(resolv);
        }
        else
            printf("error opening /etc/resolv.conf\n");
    }


    char dev[8] = {0} ; // the active network device, in UPPER case
    
    const char *def_dev = DefaultNetworkDevice();
    
    // capitalize the network device : ETH0
    for (int i = 0; def_dev[i] && i < 7; ++i) dev[i] = toupper(def_dev[i]);
    
    cString eth_use_dhcp = cString::sprintf("%s_USE_DHCP", dev);
    cString eth_ip       = cString::sprintf("%s_IP", dev);
    cString eth_netmask  = cString::sprintf("%s_NETMASK", dev);
    
    // make sure ETH1 = 'no' when ETH0 is the default device
    if (!strcmp(dev, "ETH0")) sysconfig.SetVariable("ETH1", "no");
    
    if (use_internal)
    {
        
        sysconfig.SetVariable(dev, "yes");

        if (use_dhcp)
            sysconfig.SetVariable(eth_use_dhcp, "yes");
        else
        {
            sysconfig.SetVariable("DOMAIN", domain);
            sysconfig.SetVariable("GATEWAY", gateway);
            sysconfig.SetVariable(eth_use_dhcp, "no");
            sysconfig.SetVariable(eth_ip, ipaddress);
            sysconfig.SetVariable(eth_netmask, netmask);
        }
    }
    else
    {
        sysconfig.SetVariable(dev, "no");
    }

    sysconfig.Save();
    sysconfig.Load(SYSCONFIGFNAME);
    if ((use_internal != orig_use_internal) || (use_dhcp != orig_use_dhcp) || strcmp(hostname, orig_hostname) || strcmp(domain, orig_domain) || strcmp(gateway, orig_gateway) || strcmp(nameserver, orig_nameserver) || strcmp(ipaddress, orig_ipaddress) || strcmp(netmask, orig_netmask))
    {
        //  Skins.Message(mtInfo, tr("Setting changes - please wait"));
        if (SystemExec("setup-net restart") == -1)
            printf("failed to execute command \"setup-net restart\"\n");
        //    Skins.Message(mtInfo, tr("Changes done"));
    }
    Setup.Save();
    return true;
}

bool cInstallNet::CheckIP()
{
    char *strBuff, *strBuff2, *strCutter;
    char *Trash = (char *)malloc(512 * sizeof(char));
    char command[64];
    FILE *process;

    sprintf(command, "ifconfig %s | grep 'inet '", NETWORK_DEVICE);   //TB: TODO: query it directly from the kernel, this way is cruelly inefficient
    process = popen(command, "r");
    if (process)
    {
        cReadLine readline;
        strBuff = readline.Read(process);
        while (strBuff)
        {
            if (strstr(strBuff, "inet "))
            {
                strCutter = strchr(strBuff, ':') + 1;
                strBuff2 = (strchr(strCutter, ' ')) + 1;
                *(strBuff2 - 1) = '\0';
                strcpy(dhcp_ipaddress, strCutter);
                strCutter = strBuff2;
                strCutter = strchr(strCutter, ':') + 1;
                strCutter = strchr(strCutter, ':') + 1; // skip Broadcast
                strcpy(dhcp_netmask, strCutter);

                pclose(process);

                // get Info about Gateway
                sprintf(command, "netstat -rn | grep UG | grep %s", NETWORK_DEVICE);  //TB: TODO: query it directly from the kernel, this way is cruelly inefficient
                process = popen(command, "r");
                if (process)
                {
                    cReadLine readline;
                    strBuff = readline.Read(process);
                    if (strBuff)
                        sscanf(strBuff, "%s %s", Trash, dhcp_gateway);
                    else
                        strcpy(dhcp_gateway, "0.0.0.0");
                }
                pclose(process);

                // get Info about DNS (only 1)
                process = popen("grep nameserver /etc/resolv.conf", "r");   //TB: TODO: improve this, this code is that ugly
                if (process)
                {
                    cReadLine readline;
                    strBuff = readline.Read(process);
                    if (strBuff)
                        sscanf(strBuff, "%s %s", Trash, dhcp_nameserver);
                    else
                        strcpy(dhcp_nameserver, "0.0.0.0");
                }
                pclose(process);

                free(Trash);
                return true;    // Got IP-settings
            }
            strBuff = readline.Read(process);
        }
    }
    return false;
}

bool cInstallNet::RunDHCP()
{
    for (int i = 0; i < DHCP_TIMEOUT; ++i)
    {
        if (CheckIP())
            return true;
        sleep(1);
    }

    return false;
}
