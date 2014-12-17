#ifndef __PREVIEW_CHANNEL__H__
#define __PREVIEW_CHANNEL__H__

#include "player.h"
#include "receiver.h"

class cPreviewChannel 
{

private:
    cPipPlayer      *pipPlayer;
    cOsdPipReceiver *pipReceiver;

    int      channelNumber;
    cDevice  *device;


public:
    cPreviewChannel(int chNumber);
    ~cPreviewChannel();

    bool SetChannel(int chNumber);
    bool SetDimensions(int x1, int y1, int width, int height);
    bool StartPreview();
    bool StopPreview();

}; // cPreviewChannel

#endif 
