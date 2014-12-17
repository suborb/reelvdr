/***************************************************************************
 *                                                                         *
 *   txtrender.c - Teletext display abstraction and teletext code          *
 *                 renderer                                                *
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
#include "txtrender.h"
    
cRenderPage::cRenderPage() {
    Dirty=false;
    DirtyAll=false;
    
    // Todo: make this configurable
    FirstG0=CHARSET_LATIN_G0_DE;
    SecondG0=CHARSET_LATIN_G0_DE;
}

enum enumSizeMode {
    // Possible size modifications of characters
    sizeNormal,
    sizeDoubleWidth,
    sizeDoubleHeight,
    sizeDoubleSize
};

/*
// Debug only: List of teletext spacing code short names
const char *(names[0x20])={
    "AlBk","AlRd","AlGr","AlYl","AlBl","AlMg","AlCy","AlWh",
    "Flsh","Stdy","EnBx","StBx","SzNo","SzDh","SzDw","SzDs",
    "MoBk","MoRd","MoGr","MoYl","MoBl","MoMg","MoCy","MoWh",
    "Conc","GrCn","GrSp","ESC", "BkBl","StBk","HoMo","ReMo"};
*/

void cRenderPage::RenderTeletextCode(unsigned char *PageCode) {
    int x,y;
    bool EmptyNextLine=false;
    // Skip one line, in case double height chars were/will be used
    
    // Move over header
    PageCode+=12;
    
    for (y=0;y<24;(EmptyNextLine?y+=2:y++)) {
        // Start of line: Set start of line defaults
        
        // Hold Mosaics mode: Remember last mosaic char/charset 
        // for next spacing code
        bool HoldMosaics=false;
        unsigned char HoldMosaicChar=' ';
        enumCharsets HoldMosaicCharset=FirstG0;

        enumSizeMode Size=sizeNormal;
        // Font size modification
        bool SecondCharset=false;
        // Use primary or secondary G0 charset
        bool GraphicCharset=false;
        // Graphics charset used?
        bool SeparateGraphics=false;
        // Use separated vs. contiguous graphics charset
        bool NoNextChar=false;
        // Skip display of next char, for double-width
        EmptyNextLine=false;
        // Skip next line, for double-height

        cTeletextChar c;
        // auto.initialized to everything off
        c.SetFGColor(ttcWhite);
        c.SetBGColor(ttcBlack);
        c.SetCharset(FirstG0);

        // Pre-scan for double-height and double-size codes
        for (x=0;x<40;x++) {
            if (y==0 && x<8) x=8;
            if ((PageCode[x+40*y] & 0x7f)==0x0D || (PageCode[x+40*y] & 0x7f)==0x0F)
                EmptyNextLine=true;
        }

        // Move through line
        for (x=0;x<40;x++) {
            unsigned char ttc=PageCode[x+40*y] & 0x7f;
            // skip parity check

            if (y==0 && x<8) continue;
            // no displayable data here...
            
/*          // Debug only: Output line data and spacing codes
            if (y==6) {
                if (ttc<0x20)
                    printf("%s ",names[ttc]);
                else
                    printf("%02x ",ttc);
                if (x==39) printf("\n");
            }
*/          
            
            // Handle all 'Set-At' spacing codes
            switch (ttc) {
            case 0x09: // Steady
                c.SetBlink(false);
                break;
            case 0x0C: // Normal Size
                if (Size!=sizeNormal) {
                    Size=sizeNormal;
                    HoldMosaicChar=' ';
                    HoldMosaicCharset=FirstG0;
                }                   
                break;
            case 0x18: // Conceal
                c.SetConceal(true);
                break;
            case 0x19: // Contiguous Mosaic Graphics
                SeparateGraphics=false;
                if (GraphicCharset)
                    c.SetCharset(CHARSET_GRAPHICS_G1);
                break;
            case 0x1A: // Separated Mosaic Graphics
                SeparateGraphics=true;
                if (GraphicCharset)
                    c.SetCharset(CHARSET_GRAPHICS_G1_SEP);
                break;
            case 0x1C: // Black Background
                c.SetBGColor(ttcBlack);
                break;
            case 0x1D: // New Background
                c.SetBGColor(c.GetFGColor());
                break;
            case 0x1E: // Hold Mosaic
                HoldMosaics=true;               
                break;
            }

            // temporary copy of character data:
            cTeletextChar c2=c;
            // c2 will be text character or space character or hold mosaic
            // c2 may also have temporary flags or charsets
            
            if (ttc<0x20) {
                // Spacing code, display space or hold mosaic
                if (HoldMosaics) {
                    c2.SetChar(HoldMosaicChar);
                    c2.SetCharset(HoldMosaicCharset);
                } else {
                    c2.SetChar(' ');
                }
            } else {
                // Character code               
                c2.SetChar(ttc);
                if (GraphicCharset) {
                    if (ttc&0x20) {
                        // real graphics code, remember for HoldMosaics
                        HoldMosaicChar=ttc;
                        HoldMosaicCharset=c.GetCharset();
                    } else {
                        // invalid code, pass-through to G0
                        c2.SetCharset(SecondCharset?SecondG0:FirstG0);
                    }   
                }
            }
            
            // Handle double-height and double-width extremes
            if (y>=23) {
                if (Size==sizeDoubleHeight) Size=sizeNormal;
                if (Size==sizeDoubleSize) Size=sizeDoubleWidth;
            }
            if (x>=38) {
                if (Size==sizeDoubleWidth) Size=sizeNormal;
                if (Size==sizeDoubleSize) Size=sizeDoubleHeight;
            }
            
            // Now set character code
            
            if (NoNextChar) {
                // Suppress this char due to double width last char
                NoNextChar=false;
            } else {
                switch (Size) {
                case sizeNormal:
                    // Normal sized
                    SetChar(x,y,c2);
                    if (EmptyNextLine && y<23) {
                        // Clean up next line
                        SetChar(x,y+1,c2.ToChar(' ').ToCharset(FirstG0));
                    }
                    break;
                case sizeDoubleWidth:
                    // Double width
                    SetChar(x,y,c2.ToDblWidth(dblw_Left));
                    SetChar(x+1,y,c2.ToDblWidth(dblw_Right));
                    if (EmptyNextLine && y<23) {
                        // Clean up next line
                        SetChar(x  ,y+1,c2.ToChar(' ').ToCharset(FirstG0));
                        SetChar(x+1,y+1,c2.ToChar(' ').ToCharset(FirstG0));
                    }
                    NoNextChar=true;
                    break;
                case sizeDoubleHeight:
                    // Double height
                    SetChar(x,y,c2.ToDblHeight(dblh_Top));
                    SetChar(x,y+1,c2.ToDblHeight(dblh_Bottom));
                    break;
                case sizeDoubleSize:
                    // Double Size
                    SetChar(x  ,  y,c2.ToDblHeight(dblh_Top   ).ToDblWidth(dblw_Left ));
                    SetChar(x+1,  y,c2.ToDblHeight(dblh_Top   ).ToDblWidth(dblw_Right));
                    SetChar(x  ,y+1,c2.ToDblHeight(dblh_Bottom).ToDblWidth(dblw_Left ));
                    SetChar(x+1,y+1,c2.ToDblHeight(dblh_Bottom).ToDblWidth(dblw_Right));
                    NoNextChar=true;
                    break;
                }
            }
                
            // Handle all 'Set-After' spacing codes
            switch (ttc) {
            case 0x00 ... 0x07: // Set FG color
                if (GraphicCharset) {
                    // Actual switch from graphics charset
                    HoldMosaicChar=' ';
                    HoldMosaicCharset=FirstG0;
                }
                c.SetFGColor((enumTeletextColor)ttc);
                c.SetCharset(SecondCharset?SecondG0:FirstG0);
                GraphicCharset=false;
                c.SetConceal(false);
                break;
            case 0x08: // Flash
                c.SetBlink(true);
                break;
            case 0x0A: // End Box
                // Not supported yet
                break;
            case 0x0B: // Start Box
                // Not supported yet
                break;
            case 0x0D: // Double Height
                if (Size!=sizeDoubleHeight) {
                    Size=sizeDoubleHeight;
                    HoldMosaicChar=' ';
                    HoldMosaicCharset=FirstG0;
                }                   
                break;
            case 0x0E: // Double Width
                if (Size!=sizeDoubleWidth) {
                    Size=sizeDoubleWidth;
                    HoldMosaicChar=' ';
                    HoldMosaicCharset=FirstG0;
                }                   
                break;
            case 0x0F: // Double Size
                if (Size!=sizeDoubleSize) {
                    Size=sizeDoubleSize;
                    HoldMosaicChar=' ';
                    HoldMosaicCharset=FirstG0;
                }                   
                break;
            case 0x10 ... 0x17: // Mosaic FG Color
                if (!GraphicCharset) {
                    // Actual switch to graphics charset
                    HoldMosaicChar=' ';
                    HoldMosaicCharset=FirstG0;
                }
                c.SetFGColor((enumTeletextColor)(ttc-0x10));
                c.SetCharset(SeparateGraphics?CHARSET_GRAPHICS_G1_SEP:CHARSET_GRAPHICS_G1);
                GraphicCharset=true;
                c.SetConceal(false);
                break;
            case 0x1B: // ESC Switch
                SecondCharset=!SecondCharset;
                if (!GraphicCharset) c.SetCharset(SecondCharset?SecondG0:FirstG0);
                break;
            case 0x1F: // Release Mosaic
                HoldMosaics=false;
                break;
            }
        } // end for x
    } // end for y
}


