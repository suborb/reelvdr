/*
 * OSD Picture in Picture plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */

//#include <vdr/device.h>
#include "pesassembler.h"
#include <vdr/channels.h>

#ifndef PIP_PLAYER_H
#define PIP_PLAYER_H

class cReelBoxBase;

class cPipPlayer
{
  private:
    cReelBoxBase * device;
    cPesAssembler *pesAssembler;

    int PlayPipPesPacket(int type, const uchar * Data, int Length);

  public:
      cPipPlayer();
     ~cPipPlayer()
    {
    };
    int PlayPipPes(int type, const uchar * Data, int Length);
    bool IsPipChannel(const cChannel *channel);
    void SetPipDimensions(const uint x, const uint y, const uint width,
                          const uint height);
};


#endif //PIP_PLAYER_H
