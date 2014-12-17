
#include <iostream>
#include "displayvolume.h"
#include "config.h"
#include "reel.h"

/**
 * cSkinReelDisplayVolume
 */ 

cSkinReelDisplayVolume::cSkinReelDisplayVolume()
{
    struct ReelOsdSize OsdSize;
    ReelConfig.GetOsdSize(&OsdSize);

    int imgWidth = 64, imgHeight = 64; // Height of the icons

    volumeBarLength = int(OsdSize.w * .7);
    // int totalLen = Roundness + .0*Roundness + imgWidth + Roundness + volumeBarLength + Roundness; // to center the volume OSD

    xVolumeBackBarLeft = 0; //( OsdSize.w - totalLen ) / 2; // center the OSD
    xIconLeft = int(xVolumeBackBarLeft + 1.0 * 25/*Roundness*/);
    xVolumeFrontBarLeft = xIconLeft + imgWidth + 25 /*Roundness*/;
    xVolumeFrontBarRight = xVolumeFrontBarLeft + volumeBarLength;
    xVolumeBackBarRight = xVolumeFrontBarRight + 25 /*Roundness*/;
    int totalLen = xVolumeBackBarRight - xVolumeBackBarLeft; // to center the volume OSD

    int h = 55; // height of the frontBar
    int H = 55 ;  // height of OSD excluding the icon height

    yIconTop = 0; // icon yTop
    yVolumeBackBarTop = yIconTop + imgHeight /2 - H/2;
    yVolumeFrontBarTop = yVolumeBackBarTop + H/2 - h/2; // top volumefront bar
    yVolumeBackBarBottom = yVolumeBackBarTop + H;
    yIconBottom = yIconTop + imgHeight;

    // osd = cOsdProvider::NewTrueColorOsd(Setup.OSDLeft, Setup.OSDTop + Setup.OSDHeight - yVolumeBackBarBottom,Setup.OSDRandom);
    debug("osdsize %d %d", OsdSize.x, OsdSize.y);
#ifdef REELVDR
    osd = cOsdProvider::NewTrueColorOsd(OsdSize.x + ( OsdSize.w - totalLen ) / 2, OsdSize.y + OsdSize.h - yIconBottom,Setup.OSDRandom);
#else
    osd = cOsdProvider::NewTrueColorOsd(OsdSize.x + ( OsdSize.w - totalLen ) / 2, OsdSize.y + OsdSize.h - yIconBottom,0);
#endif
    tArea Areas[] = { { xVolumeBackBarLeft, yIconTop, xVolumeBackBarRight - 1, yIconBottom - 1, 32 } };
    osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
    SetImagePaths(osd);
}

cSkinReelDisplayVolume::~cSkinReelDisplayVolume()
{
    delete osd;
}

void cSkinReelDisplayVolume::SetVolume(int Current, int Total, bool Mute)
{
    int percent = int((float)(Current) / Total * (xVolumeFrontBarRight - xVolumeFrontBarLeft));

    osd->DrawRectangle(xVolumeBackBarLeft, yIconTop, xVolumeBackBarRight - 1, yIconBottom - 1, clrTransparent);

    //debug("DisplayVolume::SetVolume ( %d, %d, %d)", Current, Total, Mute);

    DrawUnbufferedImage(osd, "volumebar_left.png", xVolumeBackBarLeft, yVolumeBackBarTop, false);
    DrawUnbufferedImage(osd, "volumebar_left_x.png",xVolumeBackBarLeft+25,yVolumeBackBarTop,false,xVolumeBackBarRight - xVolumeBackBarLeft - 50, 1);
    if(Current*100/Total <= 99)
       DrawUnbufferedImage(osd, "volumebar_right.png", xVolumeFrontBarRight, yVolumeBackBarTop, false);
    else
       DrawUnbufferedImage(osd, "volumebar_right_active.png", xVolumeFrontBarRight, yVolumeBackBarTop, false);

    // Draw the front Bar
    DrawUnbufferedImage(osd, "volumebar_middle_active_x.png", xVolumeFrontBarLeft, yVolumeFrontBarTop, false,  percent, 1);
    DrawUnbufferedImage(osd, "volumebar_middle_inactive_x.png", xVolumeFrontBarLeft + percent + 1, yVolumeFrontBarTop, false, xVolumeFrontBarRight - xVolumeFrontBarLeft - percent - 1, 1);

    if(Current*100/Total < 1)
       DrawUnbufferedImage(osd, "volumebar_middle_left_inactive.png", xVolumeFrontBarLeft-3, yVolumeBackBarTop, false);
    else
       DrawUnbufferedImage(osd, "volumebar_middle_left_active.png", xVolumeFrontBarLeft-3, yVolumeBackBarTop, false);

    //Draw Volume icons
    if (Mute) {
        DrawUnbufferedImage(osd, "icon_volume_mute.png", xIconLeft, yIconTop, true);
    } else {
        DrawUnbufferedImage(osd, "icon_volume.png", xIconLeft, yIconTop, true);
    }
}

void cSkinReelDisplayVolume::Flush(void)
{
    osd->Flush();
}

