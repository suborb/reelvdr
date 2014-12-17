
#include "mytextscroller.h"
#include "reel.h"

#include <iostream>

/** cTextScroller */ 
cMyTextScroller::cMyTextScroller(void) {
  osd = NULL;
  left = top = width = height = 0;
  font = NULL;
  colorFg = 0;
  colorBg = 0;
  offset = 0;
  shown = 0;
}

cMyTextScroller::cMyTextScroller(cOsd *Osd, int Left, int Top, int Width, int Height, const char *Text, const cFont *Font, tColor ColorFg, tColor ColorBg) {
  bodyLeft = bodyTop = bodyRight = 0;
  Set(Osd, Left, Top, Width, Height, Text, Font, ColorFg, ColorBg);
}

void cMyTextScroller::Set(cOsd *Osd, int Left, int Top, int Width, int Height, const char *Text, const cFont *Font, tColor ColorFg, tColor ColorBg) {
  osd = Osd;
  left = Left;
  top = Top;
  width = Width;
  height = Height;
  font = Font;
  colorFg = ColorFg;
  colorBg = ColorBg;
  offset = 0;
  textWrapper.Set(Text, Font, Width);
  shown = min(Total(), height / font->Height());
  height = shown * font->Height(); /* sets height to the actually used height, which may be less than Height */
  DrawText();
}

void cMyTextScroller::Reset(void) {
  osd = NULL; /* just makes sure it won't draw anything */
}

void cMyTextScroller::DrawText(void) {
  int lineHeight = font->Height();
  if (osd) {
     for (int i = 0; i < shown; i++) {
         /** Clear the line to prevent overwriting */
         int y = top + i * lineHeight;

         int x = left; //bodyLeft + IMGMENUWIDTH-10;

         if(y+lineHeight <= bodyTop+205) /* completely in the upper part */
           osd->DrawCropImage(imgMenuBodyUpperPart, x, bodyTop, x, y, bodyRight-10, y+lineHeight, false);
         else if(y >= bodyTop+205) /* completely in the lower part */
           osd->DrawCropImage(imgMenuBodyLowerPart, x, bodyTop+205, x, y, bodyRight-10, y+lineHeight, false);
         else { /* between */ 
           osd->DrawCropImage(imgMenuBodyUpperPart, x, bodyTop, x, y, bodyRight-10, bodyTop+205, false);
           osd->DrawCropImage(imgMenuBodyLowerPart, x, bodyTop+205, x, bodyTop+205, bodyRight-10, y+lineHeight, false);
         }
         osd->DrawText(left, y, textWrapper.GetLine(offset + i), colorFg, clrTransparent, font, width);
         }
     }
}

void cMyTextScroller::Scroll(bool Up, bool Page) {
  if (Up) {
     if (CanScrollUp()) {
        offset -= Page ? shown : 1;
        if (offset < 0)
           offset = 0;
        DrawText();
        }
     }
  else {
     if (CanScrollDown()) {
        offset += Page ? shown : 1;
        if (offset + shown > Total())
           offset = Total() - shown;
        DrawText();
        }
     }
}

