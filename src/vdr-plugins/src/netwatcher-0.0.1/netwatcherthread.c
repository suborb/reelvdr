
#include <sys/socket.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <linux/ethtool.h>
#include <linux/if_ether.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <linux/dvb/frontend.h>

#include <vdr/device.h>
#include <vdr/dvbdevice.h>
#include <vdr/remote.h>
#include <vdr/sysconfig_vdr.h>
#include <vdr/config.h>

#include "netwatcherthread.h"
#include "netwatchermenu.h"
//#include "helper.h"

/** time between sleep()s in ms */
#define CHECKINTERVAL 500
#define SAMPLES_TILL_ALARM 4

// true if NetWatcher's MainMenuHook() was called atleast once
// this variable indicates vdr's mainloop has run atleast once
// and is read-only in cNetWatcherThread::Action()
bool wasMainMenuHookCalled = false;

cNetWatcherThread *cNetWatcherThread::_instance = NULL;

cNetWatcherThread::cNetWatcherThread(const char *dev)
{
    firstRun = true;
#ifdef RBMINI
    LastReelboxMode = eModeClient;
#endif
    countEthCableNotConnected = 0;
    countTunersOccupied = 0;
    cNetWatcherThread::_instance = this;
    if (dev)
        strncpy((char *)&devname, dev, 16);
    else
        strncpy((char *)&devname, DEFAULT_DEV, 16);
    MultiRoomTimeoutCounter_ = 0;
}

cNetWatcherThread::~cNetWatcherThread()
{
}

void cNetWatcherThread::Action(void)
{
    int counter = 0;

    while (Running())
    {
        cCondWait::SleepMs(CHECKINTERVAL);

        /* let this thread sleep till vdr's main loop has run atleast once
         *
         * because: avahi plugin sets ReelboxModeTemp in it's MainMenuHook() 
         * ( MainMenuHook()s are called at the end of vdr's main loop )
         * so this thread does not have the information about the 'real' vdr mode
         * hence wait for first run of vdr's main loop before running checks
         * see issue #440 in bugtracker
         */
        if (!wasMainMenuHookCalled)
           continue; // skip the rest of the loop; sleep for now

        counter = (counter + 1) % 2;
#ifdef RBMINI
        if (!counter)
        {                       // Don't do it too often!
            if (firstRun)       // skip check in first run since variables aren't ready yet
                firstRun = false;
            else
                CheckDatabase();
        }
#endif
        CheckCable();
        if (countEthCableNotConnected == 0) /** there can't be reception without network */
            CheckTuner();
    }
}

bool cNetWatcherThread::CableConnected()
{
    return (countEthCableNotConnected == 0);
}

bool cNetWatcherThread::TunersOccupied()
{
    return (countTunersOccupied == 0);
}

bool cNetWatcherThread::NetceiverFound()
{
    std::string command;
    char *UUID = NULL;

    // First check the number of NetCeivers
    bool found = false;
    char *buffer;
    FILE *file = popen("netcvdiag -c", "r");
    if (file)
    {
        cReadLine readline;
        buffer = readline.Read(file);
        while (!found && buffer)
        {
            if (strstr(buffer, "Count: 0"))
                found = true;
            buffer = readline.Read(file);
        }
        pclose(file);
        if (found)
        {
            //printf("\033[0;93m %s(%i): %s \033[0m\n", __FILE__, __LINE__, "netcvdiag -c => Count = 0");
            return false;
        }
    }

    // Retrieve UUID
    found = false;
    buffer = NULL;
    file = popen("netcvdiag -u", "r");
    if (file)
    {
        cReadLine readline;
#define KEYWORD "UUID <"
        buffer = readline.Read(file);
        while (!found && buffer)
        {
            char *ptr = strstr(buffer, KEYWORD);
            if (ptr)
            {
                UUID = strdup(ptr + strlen(KEYWORD));
                found = true;
            }
            else
                buffer = readline.Read(file);
        }
        pclose(file);

        if (!found)
        {
            //printf("\033[0;93m %s(%i): %s \033[0m\n", __FILE__, __LINE__, "netcvdiag -u => No UUID found");
            return false;
        }

        char *ptr = strchr(UUID, '>');
        if (ptr)
            *ptr = '\0';
    }

    // ping NetCeiver
    command = std::string("ping6 -c1 ") + UUID + "%" + devname + "&>/dev/null";
    free(UUID);
    if (SystemExec(command.c_str()))
    {
        //printf("\033[0;93m %s(%i): %s \033[0m\n", __FILE__, __LINE__, "Couldn't ping NetCeiver");
        return false;
    }

    return true;
}

void cNetWatcherThread::ResetTunersOccupied()
{
    countEthCableNotConnected = 0;
}

#define HAS_DVB_INPUT(x) (x && (x->ProvidesSource(cSource::stSat) || x->ProvidesSource(cSource::stCable) || x->ProvidesSource(cSource::stTerr)))

void cNetWatcherThread::CheckTuner(void)
{
    fe_status_t status;

    if (cDevice::NumDevices() == 1) // only the output device exists
        return;

    bool allOccupied = true;
    for (int i = 0; i < cDevice::NumDevices(); i++)
    {
#ifdef DEVICE_ATTRIBUTES
        cDevice *dev = cDevice::GetDevice(i);
#if VDRVERSNUM < 10716
        if (dev && dev->HasInput())
#else
        if(HAS_DVB_INPUT(dev))
#endif
        {
            uint64_t v = 0;
            dev->GetAttribute("fe.status", &v);
            status = (fe_status_t) v;
#else
        cDvbDevice *dev = dynamic_cast < cDvbDevice * >(cDevice::GetDevice(i));
#if VDRVERSNUM < 10716
        if (dev && dev->HasInput())
#else
        if(HAS_DVB_INPUT(dev))
#endif
        {
            dev->GetTuner()->GetFrontendStatus(status, 0);
#endif
            //printf("dev %i recv: %i status: %x phys. tunernr.: %x\n", i, dev->Receiving(true), status, (status>>12)&7);
            //int tunerNr = (status>>12)&7;
            if (!(dev->Receiving(true) && status == 0))
            {                   // a free tuner is found
                allOccupied = false;
                countTunersOccupied = 0;
                tunerWarningShown = false;
            }
        }
    }
    if (allOccupied)
        countTunersOccupied++;
    //printf("AO: %i CTO: %i\n", allOccupied, countTunersOccupied);
    if (!tunerWarningShown && !cOsd::IsOpen() && countTunersOccupied >= SAMPLES_TILL_ALARM)
    {
        //std::cout << "calling netwatcher plugin" << std::endl;
        tunerWarningShown = true;
        cRemote::CallPlugin("netwatcher");
    }
}

bool cNetWatcherThread::CheckCable(void)
{

    int fd;
    struct ifreq ifr;
    struct ethtool_value edata;
    edata.data = NULL;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, devname, sizeof(ifr.ifr_name) - 1);
    edata.cmd = ETHTOOL_GLINK;
    ifr.ifr_data = (caddr_t) & edata;
    if (ioctl(fd, SIOCETHTOOL, &ifr) == -1)
    {
        //perror("ETHTOOL_GLINK");
        close(fd);
        return false;
    }
    close(fd);

    if (edata.data)
    {
        if (countEthCableNotConnected != 0)
        {
            //cNetWatcherMenu::instance()->ProcessKey(kOk);
            //std::cout << "Cable reinserted!" << std::endl;
        }
        countEthCableNotConnected = 0;
    }
    else
    {
        //std::cout << "ERROR: no network cable connected!" << std::endl;
        if (!cOsd::IsOpen() && countEthCableNotConnected == SAMPLES_TILL_ALARM)
        {
            //std::cout << "CALLING NETWATCHERMENU ERROR: no network cable connected!" << std::endl;
            cSysConfig_vdr::GetInstance().Load("/etc/default/sysconfig");
            if (cSysConfig_vdr::GetInstance().GetVariable("NC_BRIDGE") && strcmp(cSysConfig_vdr::GetInstance().GetVariable("NC_BRIDGE"), "yes") == 0)
            {
                if (!NetceiverFound())
                    cRemote::CallPlugin("netwatcher");
            }
            else if (!NetceiverFound())
                cRemote::CallPlugin("netwatcher");

            //countEthCableNotConnected = 0;
        }
        countEthCableNotConnected++;
    }

    return true;
}

#ifdef RBMINI
void cNetWatcherThread::CheckDatabase(void)
{
    if ((Setup.ReelboxModeTemp != LastReelboxMode) && (Setup.ReelboxMode == eModeClient))
    {
        if (Setup.ReelboxModeTemp == eModeStandalone)
        {
            if (++MultiRoomTimeoutCounter_ == SAMPLES_TILL_ALARM)
            {
                //printf("\033[0;94m MultiRoom is temporary inactive!\033[0m\n");
                Skins.QueueMessage(mtWarning, tr("MultiRoom is temporary inactive!"));
                LastReelboxMode = Setup.ReelboxModeTemp;
            }
        }
        else if (Setup.ReelboxModeTemp == eModeClient)
        {
            //printf("\033[0;94m MultiRoom is active!\033[0m\n");
            Skins.QueueMessage(mtInfo, tr("MultiRoom is active!"));
            LastReelboxMode = Setup.ReelboxModeTemp;
            MultiRoomTimeoutCounter_ = 0;
        }
    }
    else if ((Setup.ReelboxModeTemp == eModeClient) && (Setup.ReelboxMode == eModeClient))
        MultiRoomTimeoutCounter_ = 0;   // Reset Counter
}
#endif

void cNetWatcherThread::ResetTunerWarningShown()
{
    tunerWarningShown = false;
}
