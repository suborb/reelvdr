#ifndef MENUMOVECHANNEL_H
#define MENUMOVECHANNEL_H

#include <vdr/menu.h>
#include <vdr/channels.h>
#include <vector>
#include <algorithm>

class cOsdChannelItem;
enum eMenuMode { eSelectMode, eMoveMode };

class cMenuMoveChannels : public cOsdMenu
{
private:
    eMenuMode mode;
    std::vector<int> selectedChannels; // list of channels (channelnumbers) to be moved

    void SelectChannel(int chNr) {
        selectedChannels.push_back(chNr);
    }

    void UnSelectChannel(int chNr) {
        std::vector<int>::iterator it = std::find(selectedChannels.begin(),
                                                  selectedChannels.end(),
                                                  chNr);
        if (selectedChannels.end() != it)
            selectedChannels.erase(it);
    }

    bool IsSelectedChannel(int chNr) const
    {
        std::vector<int>::const_iterator it = std::find(selectedChannels.begin(),
                                                  selectedChannels.end(),
                                                  chNr);
        return selectedChannels.end() != it;
    }

    cChannel* initialChannel;
public:
    cMenuMoveChannels(cChannel* channel=NULL);

    eOSState ProcessKey(eKeys Key);
    void Set(bool startWithGivenChannel=false);

    void SetMode(eMenuMode mode_) { mode = mode_; SetModeMessage(); }
    eMenuMode Mode()      const { return mode; }

    bool IsInMoveMode()   const { return eMoveMode   == mode; }
    bool IsInSelectMode() const { return eSelectMode == mode; }

    cChannel*        CurrentChannel();
    cOsdChannelItem* CurrentChannelItem();

    void MoveChannelItem(eKeys Key);
    // move selected channels  in 'Channels' to 'position'
    void MoveChannelsToPosition(int position);

    // set status message according to 'mode'
    // eg. In move mode, exit key unselects selected channel &
    // Up/Down to move selected channel.
    void SetModeMessage();
};


//
/// finds the channel that is in 'pos' position if vdr's channel list
/// is displayed without 'bouquet'/Group Sep.
//
// A translation function is needed because, some bouquets can "jump"
// channel numbers.
// Eg.
// 102 Arte
// .. RadioSender (bouquet)
// 200 DW radio
cChannel* Position2Channel        (int pos);
int       Position2ChannelNumber  (int pos);

#endif // MENUMOVECHANNEL_H
