
#include "netwatchermenu.h"
#include "netwatcherthread.h"

cNetWatcherMenu::cNetWatcherMenu():cOsdMenu(tr("Warning"))
{

    if (!cNetWatcherThread::instance()->CableConnected())
        warningType = eWarnNoNetwork;
    else
        warningType = eWarnNoTuner;

    Display();
}

cNetWatcherMenu::~cNetWatcherMenu()
{
    //cNetWatcherThread::instance()->ResetTunerWarningShown();
}

void cNetWatcherMenu::Display(void)
{

    Clear();

    if (warningType != eWarnNoNetwork)
    {
        //Add(new cOsdItem(tr("The network cable is connected."), osUnknown, false));
        if (warningType == eWarnNoTuner)
        {
            //cNetWatcherThread::instance()->ResetTunersOccupied();
            Add(new cOsdItem(tr("Warning!"), osUnknown, false));
            Add(new cOsdItem("", osUnknown, false));
            AddFloatingText(tr("No available DVB tuners for Live TV streaming found."), 50);
            Add(new cOsdItem("", osUnknown, false));
            AddFloatingText(tr("In order to watch Live TV on your NetClient, please check your MultiRoom configuration." "The appropriate setup screen is to be found on your ReelBox Avantgarde " " via \"Setup / Installation / MultiRoom-Setup\"."), 60);
        }
    }
    else
    {
        Add(new cOsdItem(tr("Warning!"), osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
#ifndef RBMINI
        AddFloatingText(tr("System Cable is not connected. You will not be able to watch Live TV."), 60);
#else
        AddFloatingText(tr("NetClient is NOT connected to your local network. Without an active network connection, " "you will not be able to watch Live TV or stream other media files through the network."), 60);
        Add(new cOsdItem("", osUnknown, false));
        AddFloatingText(tr("Please check if the network cable is connected properly to the NetClient " "and to your switch / router or NetCeiver."), 60);
#endif

    }

    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem(tr("Press \"OK\" to close this message window."), osUnknown, false));

    cOsdMenu::Display();
}

eOSState cNetWatcherMenu::ProcessKey(eKeys Key)
{

    switch (warningType)
    {
    case (eWarnNoNetwork):
            /** cable is back in again, so close the menu */
        if (cNetWatcherThread::instance()->CableConnected())
            return osEnd;
        break;
    case (eWarnNoTuner):
            /** a free tuner appeared, so close the menu */
        if (cNetWatcherThread::instance()->TunersOccupied())
            return osEnd;
        break;
    default:
        break;
    }

    eOSState state = cOsdMenu::ProcessKey(Key);

    switch (Key)
    {
    case kOk:
        return osEnd;
    default:
        return state;
    }
}
