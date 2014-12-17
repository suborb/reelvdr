#ifndef REELCHANNELLIST_SETUP_H
#define REELCHANNELLIST_SETUP_H

#include <vdr/menuitems.h>
#include <vdr/tools.h>
extern int zapAllFavChannels;

class cMenuReelChannelListSetup : public cMenuSetupPage
{
private:
    int newZapAllFavChannels;
    cStringList wantChOnOkModes;
    int wantChOnOkTmp;

public:
    cMenuReelChannelListSetup();
    void Store();
    void Set();
};


#endif
