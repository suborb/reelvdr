#ifndef DISPLAYVOLUME_H
#define DISPLAYVOLUME_H

#include "reel.h"

// --- cSkinReelDisplayVolume ---------------------------------------------

class cSkinReelDisplayVolume : public cSkinDisplayVolume//, public cSkinReelBaseOsd
{ // TODO
    private:
        cOsd *osd;
        int xVolumeBackBarLeft,xIconLeft,xVolumeFrontBarLeft,xVolumeFrontBarRight,xVolumeBackBarRight;
        int yVolumeBackBarTop,yIconTop,yVolumeFrontBarTop,yVolumeBackBarBottom,yIconBottom;
      static int mute;
        int volumeBarLength;
    public:
        cSkinReelDisplayVolume();
        virtual ~cSkinReelDisplayVolume();
        virtual void SetVolume(int Current, int Total, bool Mute);
        virtual void Flush();
};

#endif

