/***************************************************************************
 *                                                                         *
 *   display.h - Actual implementation of OSD display variants and         *
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

#ifndef OSDTELETEXT_DISPLAY_H_
#define OSDTELETEXT_DISPLAY_H_

#include "displaybase.h"
#include <vdr/osd.h>

namespace Display {
    // The Display:: namespace mainly encapsulates a cDisplay *display variable
    // and allows NULL-safe access to display members.
    // Additionally, selects via mode the actually used instance for *display.
    
    enum Mode { Full, HalfUpper, HalfLower };
    // Full mode: 2BPP or 3BPP full screen display, depending on memory constrains
    // HalfUpper: 3BPP display of upper half, drop lower half if out of memory
    // HalfLower: 3BPP display of lower half, drop upper half if out of memory
    
    extern Mode mode;
    extern cDisplay *display;

    // Access to display mode:
    void SetMode(Display::Mode mode);
    inline void Delete()
        { if (display) { delete display; display=NULL; } }
    
    void ShowUpperHalf();
    // Make sure the upper half of screen is visible
    // eg. for entering numbers etc.


    // Wrapper calls for various *display members:
    inline bool GetBlink() 
        { if (display) return display->GetBlink(); else return false; }
    inline bool SetBlink(bool blink) 
        { if (display) display->SetBlink(blink); else return false; }
    inline bool GetConceal() 
        { if (display) return display->GetConceal(); else return false; }
    inline bool SetConceal(bool conceal) 
        { if (display) display->SetConceal(conceal); else return false; }
    inline cDisplay::enumZoom GetZoom() 
        { if (display) return display->GetZoom(); else return cDisplay::Zoom_Off; }
    inline void SetZoom(cDisplay::enumZoom zoom)
        { if (display) display->SetZoom(zoom); }

    inline void SetBackgroundColor(tColor c)
        { if (display) display->SetBackgroundColor(c); }
        
    inline tColor GetBackgroundColor() 
        { if (display) return display->GetBackgroundColor(); else return 0; }

    inline void HoldFlush() 
        { if (display) display->HoldFlush(); }
    inline void ReleaseFlush() 
        { if (display) display->ReleaseFlush(); }

    inline void RenderTeletextCode(unsigned char *PageCode)
        { if (display) display->RenderTeletextCode(PageCode); }
    
    inline void DrawClock()
        { if (display) display->DrawClock(); }
    inline void DrawPageId(const char *text)
        { if (display) display->DrawPageId(text); }
    inline void DrawMessage(const char *txt)
        { if (display) display->DrawMessage(txt); }
    inline void ClearMessage()
        { if (display) display->ClearMessage(); }
}




class cDisplay2BPP : public cDisplay {
    // 2BPP (4 color) OSD display
    // Use static color mapping to limit color depth
    
    int ColorMap2BPP[8];
    int AlternateColorMap2BPP[8];
    
public:
    cDisplay2BPP(int x0, int y0, int width, int height);

    virtual int GetColorIndex(enumTeletextColor ttc, int Area);
    virtual int GetColorIndexAlternate(enumTeletextColor ttc, int Area);

protected:
    virtual void InitPalette();
};



class cDisplay3BPP : public cDisplay {
    // 3BPP (8 color) OSD display
    // No need for color mapping
public:
    cDisplay3BPP(int x0, int y0, int width, int height);
protected:
    virtual void InitPalette();
};




class cDisplay3BPPHalf : public cDisplay {
    // 3BPP (8 color) OSD display with auto size reduction on memory constrains
    // Automatically tries to make visible area as big as possible
    // No need for color mapping
    bool Upper;
    // Prefer to show upper half or lower half?
    
    int OsdX0,OsdY0; 
    // Needed to re-initialize osd

public:
    cDisplay3BPPHalf(int x0, int y0, int width, int height, bool upper);
    bool GetUpper() { return Upper; }
    void SetUpper(bool upper) 
        { if (Upper!=upper) { Upper=upper; InitOSD(); } }
protected:
    void InitOSD();
    virtual void InitPalette();
};




#endif
