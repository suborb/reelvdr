/***************************************************************************
 *                                                                         *
 *   display.c - Actual implementation of OSD display variants and         *
 *               Display:: namespace that encapsulates a single cDisplay.  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Changelog:                                                            *
 *     2005-03    initial version (c) Udo Richter                          *
 *                                                                         *
 ***************************************************************************/

#include <strings.h>
#include <vdr/config.h>
#include "setup.h"
#include "display.h"
#include "txtfont.h"
    
// Static variables of Display:: namespace
Display::Mode Display::mode=Display::Full;
cDisplay *Display::display=NULL;


void Display::SetMode(Display::Mode NewMode) {
    // (re-)set display mode.
    
    if (display!=NULL && NewMode==mode) return;
    // No change, nothing to do

    // OSD origin, centered on VDR OSD
    int x0=Setup.OSDLeft+(Setup.OSDWidth-ttSetup.OSDwidth)/2;
    int y0=Setup.OSDTop +(Setup.OSDHeight-ttSetup.OSDheight)/2;
    
    switch (NewMode) {
    case Display::Full:
        // Need to re-initialize *display:
        Delete();
        // Try 3BPP display first:
        display=new cDisplay3BPP(x0,y0,ttSetup.OSDwidth,ttSetup.OSDheight);
        if (!display->Valid()) {
            // Failed, possibly out of memory 
            delete display;
            // Try 2BPP display
            display=new cDisplay2BPP(x0,y0,ttSetup.OSDwidth,ttSetup.OSDheight);
        }
        break;
    case Display::HalfUpper:
        // Shortcut to switch from HalfUpper to HalfLower:
        if (mode==Display::HalfLower) {
            // keep instance.
            ((cDisplay3BPPHalf*)display)->SetUpper(true);
            break;
        }
        // Need to re-initialize *display:
        Delete();
        display=new cDisplay3BPPHalf(x0,y0,ttSetup.OSDwidth,ttSetup.OSDheight,true);
        break;
    case Display::HalfLower:
        // Shortcut to switch from HalfUpper to HalfLower:
        if (mode==Display::HalfUpper) {
            // keep instance.
            ((cDisplay3BPPHalf*)display)->SetUpper(false);
            break;
        }
        // Need to re-initialize *display:
        Delete();
        display=new cDisplay3BPPHalf(x0,y0,ttSetup.OSDwidth,ttSetup.OSDheight,false);
        break;
    }           
    mode=NewMode;
    // If display is invalid, clean up immediately:
    if (!display->Valid()) Delete();
    // Pass through OSD black transparency
    SetBackgroundColor((tColor)ttSetup.configuredClrBackground);    
}

void Display::ShowUpperHalf() {
    // Enforce upper half of screen to be visible
    if (GetZoom()==cDisplay::Zoom_Lower)
        SetZoom(cDisplay::Zoom_Upper);
    if (mode==HalfLower)
        SetMode(HalfUpper);
}






cDisplay2BPP::cDisplay2BPP(int x0, int y0, int width, int height) 
    : cDisplay(width,height) {
    // 2BPP display with color mapping

    osd = cOsdProvider::NewOsd(x0, y0);
    if (!osd) return;
   
    width=(width+3)&~3;
    // Width has to end on byte boundary, so round up
    
    tArea Areas[] = { { 0, 0, width - 1, height - 1, 2 } };
    if (osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea)) != oeOk) {
        delete osd;
        osd=NULL;
        return;
    }   
    osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
    
    InitPalette();

    InitScaler();
}


void cDisplay2BPP::InitPalette() {
    if (!osd) return;

    cBitmap *bm=osd->GetBitmap(0);
    bm->Reset(); // cPalette::Reset

    // Map the teletext colors down to just 4 osd colors

    // Define color mapping teletext color -> bitmap color index
    ColorMap2BPP[ttcBlack]  =bm->Index(Background);
    ColorMap2BPP[ttcRed]    =bm->Index(clrRed);
    ColorMap2BPP[ttcGreen]  =bm->Index(clrYellow);
    ColorMap2BPP[ttcYellow] =bm->Index(clrYellow);
    ColorMap2BPP[ttcBlue]   =bm->Index(Background);
    ColorMap2BPP[ttcMagenta]=bm->Index(clrRed);
    ColorMap2BPP[ttcCyan]   =bm->Index(clrCyan);
    ColorMap2BPP[ttcWhite]  =bm->Index(clrCyan);
    
    // Alternate colors for color collisions:
    AlternateColorMap2BPP[ttcBlack]  =bm->Index(clrCyan);
    AlternateColorMap2BPP[ttcRed]    =bm->Index(clrYellow);
    AlternateColorMap2BPP[ttcGreen]  =bm->Index(clrRed);
    AlternateColorMap2BPP[ttcYellow] =bm->Index(clrRed);
    AlternateColorMap2BPP[ttcBlue]   =bm->Index(clrCyan);
    AlternateColorMap2BPP[ttcMagenta]=bm->Index(clrYellow);
    AlternateColorMap2BPP[ttcCyan]   =bm->Index(Background);
    AlternateColorMap2BPP[ttcWhite]  =bm->Index(Background);
    // These just need to be different to the previous assignment
}

int cDisplay2BPP::GetColorIndex(enumTeletextColor ttc, int Area) {
    if (ttc<8) return ColorMap2BPP[ttc];
    return ColorMap2BPP[0];
}

int cDisplay2BPP::GetColorIndexAlternate(enumTeletextColor ttc, int Area) {
    if (ttc<8) return AlternateColorMap2BPP[ttc];
    return AlternateColorMap2BPP[0];
}





cDisplay3BPP::cDisplay3BPP(int x0, int y0, int width, int height) 
    : cDisplay(width,height) {
    // 3BPP display for memory-modded DVB cards and other OSD providers
    // Actually uses 4BPP, because DVB does not support 3BPP.

    osd = cOsdProvider::NewOsd(x0, y0);
    if (!osd) return;
   
    width=(width+1)&~1;
    // Width has to end on byte boundary, so round up

    tArea Areas[] = { { 0, 0, width - 1, height - 1, 4 } };
    if (osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea)) != oeOk) {
        delete osd;
        osd=NULL;
        return;
    }   
    osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
    
    InitPalette();

    InitScaler();
}

void cDisplay3BPP::InitPalette() {
    if (!osd) return;
    cBitmap *bm=osd->GetBitmap(0);
    bm->Reset(); // cPalette::Reset

    // Set straight forward 1:1 palette
    bm->Index(Background);
    bm->Index(clrRed);
    bm->Index(clrGreen);
    bm->Index(clrYellow);
    bm->Index(clrBlue);
    bm->Index(clrMagenta);
    bm->Index(clrCyan);
    bm->Index(clrWhite);
}





cDisplay3BPPHalf::cDisplay3BPPHalf(int x0, int y0, int width, int height, bool upper) 
    : cDisplay(width,height) {

    OsdX0=x0;
    OsdY0=y0;
    Upper=upper;
    osd=NULL;
    
    // Redirect all real init work to method
    InitOSD();
}

void cDisplay3BPPHalf::InitOSD() {
    if (osd) delete osd;
    osd = cOsdProvider::NewOsd(OsdX0, OsdY0);
    if (!osd) return;
   
    int width=(Width+1)&~1;
    // Width has to end on byte boundary, so round up
    
    tArea Areas[] = { { 0, 0, width - 1, Height - 1, 4 } };
    // Try full-size area first
    
    while (osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea)) != oeOk) {
        // Out of memory, so shrink
        if (Upper) {
            // Move up lower border
            Areas[0].y2=Areas[0].y2-1;
        } else {
            // Move down upper border
            Areas[0].y1=Areas[0].y1+1;
        }
        if (Areas[0].y1>Areas[0].y2) {
            // Area is empty, fail miserably
            delete osd;
            osd=NULL;
            return;
        }
    }
    // Add some backup
    // CanHandleAreas is not accurate enough
    if (Upper) {
        Areas[0].y2=Areas[0].y2-10;
    } else {
        Areas[0].y1=Areas[0].y1+10;
    }

    osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));

    InitPalette();

    InitScaler();

    // In case we switched on the fly, do a full redraw
    Dirty=true;
    DirtyAll=true;
    Flush();
}


void cDisplay3BPPHalf::InitPalette() {
    if (!osd) return;
    cBitmap *bm=osd->GetBitmap(0);
    bm->Reset(); // cPalette::Reset

    // Set straight forward 1:1 palette
    bm->Index(Background);
    bm->Index(clrRed);
    bm->Index(clrGreen);
    bm->Index(clrYellow);
    bm->Index(clrBlue);
    bm->Index(clrMagenta);
    bm->Index(clrCyan);
    bm->Index(clrWhite);
}

