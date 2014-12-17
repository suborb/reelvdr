#include "setup.h"

int zapAllFavChannels;

cMenuReelChannelListSetup::cMenuReelChannelListSetup() : wantChOnOkTmp(Setup.WantChListOnOk)
{
    wantChOnOkModes.Append(strdup(tr("Channel Info")));
    wantChOnOkModes.Append(strdup(tr("Standard")));
    wantChOnOkModes.Append(strdup(tr("Favourites"))); // like kYellow

    Set();
}

void cMenuReelChannelListSetup::Set()
{
    Clear();
    newZapAllFavChannels = zapAllFavChannels;
    wantChOnOkTmp = Setup.WantChListOnOk;

    /*
    Add(new cMenuEditBoolItem(tr("Zapping in favourites list"),
                              &newZapAllFavChannels,
                              tr("Current folder"),
                              tr("All folders")));
    */

    Add(new cMenuEditStraItem(tr("Ok key in TV Mode"), &wantChOnOkTmp,
                              wantChOnOkModes.Size(), &wantChOnOkModes.At(0)));
}

void cMenuReelChannelListSetup::Store()
{
    zapAllFavChannels = newZapAllFavChannels;
    Setup.WantChListOnOk = wantChOnOkTmp;

    SetupStore("ZapAllFavChannels", zapAllFavChannels);
}

