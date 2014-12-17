/***************************************************************************
 *                                                                         *
 *   displaybase.c - Base class for rendering a teletext cRenderPage to    *
 *                   an actual VDR OSD.                                    *
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
#include <time.h>
#include "displaybase.h"
#include "txtfont.h"


cDisplay::cDisplay(int width, int height) {
    Concealed=false;
    Blinked=false;
    FlushLock=0;
    Zoom=Zoom_Off;
    osd=NULL;
    ScaleX=1;
    ScaleY=1;
    OffsetX=0;
    OffsetY=0;
    Width=width;
    Height=height;
    Background=clrGray50;
    
    MessageX=0;
    MessageY=0;
    MessageW=0;
    MessageH=0;
    MessageFont=cFont::GetFont(fontSml);
}

cDisplay::~cDisplay() {
    if (osd) delete osd;
    osd=NULL;   
}

void cDisplay::InitScaler() {
    // Set up the scaling factors. Also do zoom mode by
    // scaling differently.

    if (!osd) return;
    
    int height=Height-6;
    int width=Width-6;
    OffsetX=3;
    OffsetY=3;
    
    switch (Zoom) {
    case Zoom_Upper:
        height=height*2;
        break;
    case Zoom_Lower:
        OffsetY=OffsetY-height;
        height=height*2;
        break;
    default:;
    }
    
    ScaleX=(480<<16)/width;
    ScaleY=(250<<16)/height;
}

bool cDisplay::SetBlink(bool blink) {
    int x,y;
    bool Change=false;
    
    if (blink==Blinked) return false;
    
    // touch all blinking chars
    for (y=0;y<25;y++) {
        for (x=0;x<40;x++) {
            if (Page[x][y].GetBlink())  {
                Page[x][y].SetDirty(true);
                Change=true;
            }
        }
    }
    Blinked=blink;
    if (Change) Dirty=true;
    
    Flush();

    return Change;
}

bool cDisplay::SetConceal(bool conceal) {
    int x,y;
    bool Change=false;
    
    if (conceal==Concealed) return false;
    
    // touch all concealed chars
    for (y=0;y<25;y++) {
        for (x=0;x<40;x++) {
            if (Page[x][y].GetConceal()) {
                Page[x][y].SetDirty(true);
                Change=true;
            }
        }
    }
    Concealed=conceal;
    if (Change) Dirty=true;
    
    Flush();
    
    return Change;
}

void cDisplay::SetZoom(enumZoom zoom) {
    cBitmap *bm;
    
    if (!osd) return;
    if (Zoom==zoom) return;
    Zoom=zoom;

    // Re-initialize scaler to let zoom take effect 
    InitScaler();
    
    // Clear screen - mainly clear border
    int Area=0;
    while ((bm=osd->GetBitmap(Area))) {
        bm->DrawRectangle(0, 0, Width-1, Height-1, bm->Color(GetColorIndex(ttcBlack,Area)));
        Area++; 
    }
    
    // repaint all
    Dirty=true;
    DirtyAll=true;
    
    Flush();
}

void cDisplay::SetBackgroundColor(tColor c) {
    Background=c;
    InitPalette();
    Flush();
}

int cDisplay::GetColorIndex(enumTeletextColor ttc, int Area) {
    return (int)ttc;    
}

int cDisplay::GetColorIndexAlternate(enumTeletextColor ttc, int Area) {
    return (int)ttc;    
}

void cDisplay::DrawDisplay() {
    int x,y;
    int cnt=0;
    
    if (!IsDirty()) return;
    // nothing to do

    #ifdef timingdebug
        cTime t;
        t.Start();
    #endif

    for (y=0;y<25;y++) {
        for (x=0;x<40;x++) {
            if (IsDirty(x,y)) {
                // Need to draw char to osd
                cnt++;
                cTeletextChar c=Page[x][y];
                c.SetDirty(false);
                if ((Blinked && c.GetBlink()) || (Concealed && c.GetConceal())) {
                    c.SetChar(0x20);
                    c.SetCharset(CHARSET_LATIN_G0_DE);
                }
                DrawChar(x,y,c);
                Page[x][y]=c;
            }
        }
    }
    #ifdef timingdebug
        t.Stop("Draw Display");
    #endif

    Dirty=false;
    DirtyAll=false;
}


inline unsigned int LeftBits(unsigned int bits) {
    // Scale bit positions 0xfc00 to 0xfff0 positions
    unsigned int res=0;
    if (bits&0x8000) res|=0xC000;
    if (bits&0x4000) res|=0x3000;
    if (bits&0x2000) res|=0x0C00;
    if (bits&0x1000) res|=0x0300;
    if (bits&0x0800) res|=0x00C0;
    if (bits&0x0400) res|=0x0030;
    return res;
}
inline unsigned int RightBits(unsigned int bits) {
    // Scale bit positions 0x03f0 to 0xfff0 positions
    unsigned int res=0;
    if (bits&0x0200) res|=0xC000;
    if (bits&0x0100) res|=0x3000;
    if (bits&0x0080) res|=0x0C00;
    if (bits&0x0040) res|=0x0300;
    if (bits&0x0020) res|=0x00C0;
    if (bits&0x0010) res|=0x0030;
    return res;
}

inline bool IsPureChar(unsigned int *bitmap) {
    // Check if character is pure foreground or
    // pure background color
    int i;
    if (bitmap[0]==0x0000) {
        for (i=1;i<10;i++) {
            if (bitmap[i]!=0x0000) return false;
        }
    } else if (bitmap[0]==0xfff0) {
        for (i=1;i<10;i++) {
            if (bitmap[i]!=0xfff0) return false;
        }
    } else {
        return false;
    }
    return true;
}

unsigned int* cDisplay::GetFontChar(cTeletextChar c, unsigned int *buffer) {
    // Get character bitmap for character/charset

    enumCharsets font=c.GetCharset();
    int chr=c.GetChar();
    unsigned int *bitmap=NULL;
    int i;
    
    switch (font) {
        // FIXME: Need to support more than just the G0 DE font
    case CHARSET_LATIN_G0_DE:
        if (chr>=0x20 && chr<0x80) {
            bitmap=TXT_Font[chr-0x20];
        }
        break;
    case CHARSET_GRAPHICS_G1:
        if (chr>=0x20 && chr<0x40) {
            bitmap=TXT_Font[chr-0x20+0x80];
        } else if (chr>=0x60 && chr<0x80) {
            bitmap=TXT_Font[chr-0x60+0xE0];
        }
        break;
    case CHARSET_GRAPHICS_G1_SEP:
        if (chr>=0x20 && chr<0x40) {
            bitmap=TXT_Font[chr-0x20+0x80];
        } else if (chr>=0x60 && chr<0x80) {
            bitmap=TXT_Font[chr-0x60+0xE0];
        }
        if (bitmap) {
            for (i=0;i<10;i++) buffer[i]=bitmap[i]&TXT_Mask[i];
            bitmap=buffer;
        }
        break;
    default: ;
    }
    
    if (!buffer) {
        printf("Warning: Undefined char: %x %x\n",font,chr);
        return NULL;
    }
    
    switch (c.GetDblHeight()) {
    case dblh_Top:
        // Scale top 5 lines to full height
        buffer[8]=buffer[9]=bitmap[4];
        buffer[6]=buffer[7]=bitmap[3];
        buffer[4]=buffer[5]=bitmap[2];
        buffer[2]=buffer[3]=bitmap[1];
        buffer[1]=buffer[0]=bitmap[0];
        bitmap=buffer;
        break;
    case dblh_Bottom:
        // Scale bottom 5 lines to full height
        buffer[0]=buffer[1]=bitmap[5];
        buffer[2]=buffer[3]=bitmap[6];
        buffer[4]=buffer[5]=bitmap[7];
        buffer[6]=buffer[7]=bitmap[8];
        buffer[8]=buffer[9]=bitmap[9];
        bitmap=buffer;
    default:;
    }

    switch (c.GetDblWidth()) {
    case dblw_Left:
        // Scale 6 left columns to full width
        buffer[0]=LeftBits(bitmap[0]);
        buffer[1]=LeftBits(bitmap[1]);
        buffer[2]=LeftBits(bitmap[2]);
        buffer[3]=LeftBits(bitmap[3]);
        buffer[4]=LeftBits(bitmap[4]);
        buffer[5]=LeftBits(bitmap[5]);
        buffer[6]=LeftBits(bitmap[6]);
        buffer[7]=LeftBits(bitmap[7]);
        buffer[8]=LeftBits(bitmap[8]);
        buffer[9]=LeftBits(bitmap[9]);
        bitmap=buffer;
        break;
    case dblw_Right:
        // Scale 6 right columns to full width
        buffer[0]=RightBits(bitmap[0]);
        buffer[1]=RightBits(bitmap[1]);
        buffer[2]=RightBits(bitmap[2]);
        buffer[3]=RightBits(bitmap[3]);
        buffer[4]=RightBits(bitmap[4]);
        buffer[5]=RightBits(bitmap[5]);
        buffer[6]=RightBits(bitmap[6]);
        buffer[7]=RightBits(bitmap[7]);
        buffer[8]=RightBits(bitmap[8]);
        buffer[9]=RightBits(bitmap[9]);
        bitmap=buffer;
    default:;
    }
    
    return bitmap;      
}
    
void cDisplay::DrawChar(int x, int y, cTeletextChar c) {
    unsigned int buffer[10];
    unsigned int *charmap;
    cBitmap *bm;
    
    // Get character face:
    charmap=GetFontChar(c,buffer);
    if (!charmap) {
        // invalid - clear buffer
        bzero(&buffer,sizeof buffer);
        charmap=buffer;
    }
    
    // Get colors
    enumTeletextColor ttfg=c.GetFGColor();
    enumTeletextColor ttbg=c.GetBGColor();
        
    // Virtual box area of the character
    cBox box;
    box.SetToCharacter(x,y);
    
    // OSD top left pixel of char
    cVirtualCoordinate TopLeft;
    TopLeft.VirtualToPixel(this,box.XMin,box.YMin);
    // This pixel overlaps the box, but may be almost outside.
    
    // Move in OSD pixel units until we are inside the box
    while (TopLeft.VirtX<box.XMin) TopLeft.IncPixelX(this);
    while (TopLeft.VirtY<box.YMin) TopLeft.IncPixelY(this);

    // Move through all areas
    int Area=0;
    while ((bm=osd->GetBitmap(Area))) {
        cVirtualCoordinate BMTopLeft=TopLeft;
        
        // Correct for bitmap offset
        BMTopLeft.OsdX-=bm->X0();
        BMTopLeft.OsdY-=bm->Y0();
    
        // Map color to local
        int fg=GetColorIndex(ttfg,Area);
        int bg=GetColorIndex(ttbg,Area);
        if (ttfg!=ttbg && fg==bg && !IsPureChar(charmap)) {
            // Color collision
            bg=GetColorIndexAlternate(ttbg,Area);
        }
    
        // Now draw the character. Start at the top left corner, and walk
        // through all pixels on OSD. To speed up, keep one pointer to OSD pixel
        // and one to virtual box coordinates, and move them together.
        
        cVirtualCoordinate p=BMTopLeft;
        while (p.VirtY<=box.YMax) {
            // run through OSD lines
            
            // OSD line in this bitmap?
            if (0<=p.OsdY && p.OsdY<bm->Height()) {
                // bits for this line
                int bitline;
                bitline=charmap[(p.VirtY-box.YMin)>>16];
        
                p.OsdX=BMTopLeft.OsdX;
                p.VirtX=BMTopLeft.VirtX;
                while (p.VirtX<=box.XMax) {
                    // run through line pixels
                    
                    // pixel insied this bitmap?
                    if (0<=p.OsdX && p.OsdX<bm->Width()) {
                        // pixel offset in bitline:
                        int bit=(p.VirtX-box.XMin)>>16;
                        
                        if (bitline&(0x8000>>bit)) {
                            bm->SetIndex(p.OsdX,p.OsdY,fg);
                        } else {
                            bm->SetIndex(p.OsdX,p.OsdY,bg);
                        }
                    }
                    p.IncPixelX(this);
                }
            }
            p.IncPixelY(this);
        }
        Area++;
    }
}

void cDisplay::DrawText(int x, int y, const char *text, int len) {
    // Copy text to teletext page

    cTeletextChar c;
    c.SetFGColor(ttcWhite);
    c.SetBGColor(ttcBlack);
    c.SetCharset(FirstG0);

    // Copy chars up to len or null char
    while (len>0 && *text!=0x00) {
        c.SetChar(*text);
        SetChar(x,y,c);
        text++;
        x++;
        len--;
    }
    
    // Fill remaining chars with spaces
    c.SetChar(' ');
    while (len>0) {
        SetChar(x,y,c);
        x++;
        len--;
    }
    // .. and display
    Flush();
}

void cDisplay::DrawClock() {
    char text[9];
    time_t t=time(0);
    struct tm loct;
    
    localtime_r(&t, &loct);
    sprintf(text, "%02d:%02d:%02d", loct.tm_hour, loct.tm_min, loct.tm_sec);

    DrawText(32,0,text,8);
}

void cDisplay::DrawMessage(const char *txt) {
    const int border=5;
    cBitmap *bm;
    
    if (!osd) return;
    
    HoldFlush();
    // Hold flush until done
    
    ClearMessage();
    // Make sure old message is gone
    
    if (IsDirty()) DrawDisplay();
    // Make sure all characters are out, so we can draw on top
    
    int w=MessageFont->Width(txt)+4*border;
    int h=MessageFont->Height(txt)+4*border;
    int x=(Width-w)/2;
    int y=(Height-h)/2;

    int Area=0;
    while ((bm=osd->GetBitmap(Area))) {
        // Walk through all OSD areas

        // Get local color mapping      
        tColor fg=bm->Color(GetColorIndex(ttcWhite,Area));
        tColor bg=bm->Color(GetColorIndex(ttcBlack,Area));
        if (fg==bg) bg=bm->Color(GetColorIndexAlternate(ttcBlack,Area));
        
        // Draw framed box
        osd->DrawRectangle(x         ,y         ,x+w-1       ,y+border-1  ,fg);
        osd->DrawRectangle(x         ,y+h-border,x+w-1       ,y+h-1       ,fg);
        osd->DrawRectangle(x         ,y         ,x+border-1  ,y+h-1       ,fg);
        osd->DrawRectangle(x+w-border,y         ,x+w-1       ,y+h-1       ,fg);
        osd->DrawRectangle(x+border  ,y+border  ,x+w-border-1,y+h-border-1,bg);

        // Draw text
        osd->DrawText(x+2*border,y+2*border,txt, fg, bg, MessageFont);

        Area++;
    }

    // Remember box
    MessageW=w;
    MessageH=h;
    MessageX=x;
    MessageY=y;

    // And flush all changes
    ReleaseFlush();
}

void cDisplay::ClearMessage() {
    if (!osd) return;
    if (MessageW==0 || MessageH==0) return;
    
    // map OSD pixel to virtual coordinate, use center of pixel
    int x0=(MessageX-OffsetX)*ScaleX+ScaleX/2;
    int y0=(MessageY-OffsetY)*ScaleY+ScaleY/2;
    int x1=(MessageX+MessageW-1-OffsetX)*ScaleX+ScaleX/2;
    int y1=(MessageY+MessageH-1-OffsetY)*ScaleY+ScaleY/2;
    
    // map to character
    x0=x0/(12<<16);
    y0=y0/(10<<16);
    x1=(x1+(12<<16)-1)/(12<<16);
    y1=(y1+(10<<16)-1)/(10<<16);
    
    for (int x=x0;x<=x1;x++) {
        for (int y=y0;y<=y1;y++) {
            MakeDirty(x,y);
        }
    }
    
    MessageW=0;
    MessageH=0;
    
    Flush();
}


