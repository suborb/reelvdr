
#ifndef MYTEXTSCROLLER_H
#define MYTEXTSCROLLER_H

#include <vdr/osd.h>
#include <vdr/skins.h>

class cMyTextScroller : public cTextScroller {
private:
  cOsd *osd;
  int left, top, width, height;
  int bodyLeft, bodyTop, bodyRight;
  const cFont *font;
  tColor colorFg, colorBg;
  int offset, shown;
  cTextWrapper textWrapper;
  void DrawText(void);
public:
  cMyTextScroller(void);
  cMyTextScroller(cOsd *Osd, int Left, int Top, int Width, int Height, const char *Text, const cFont *Font, tColor ColorFg, tColor ColorBg);
  void Set(cOsd *Osd, int Left, int Top, int Width, int Height, const char *Text, const cFont *Font, tColor ColorFg, tColor ColorBg);
  void Reset(void);
  int Left(void) { return left; }
  int Top(void) { return top; }
  int Width(void) { return width; }
  int Height(void) { return height; }
  int Total(void) { return textWrapper.Lines(); }
  int Offset(void) { return offset; }
  int Shown(void) { return shown; }
  bool CanScroll(void) { return CanScrollUp() || CanScrollDown(); }
  bool CanScrollUp(void) { return offset > 0; }
  bool CanScrollDown(void) { return offset + shown < Total(); }
  void Scroll(bool Up, bool Page);
  void SetGlobals(int BodyLeft, int BodyTop, int BodyRight) { bodyLeft = BodyLeft; bodyTop = BodyTop; bodyRight = BodyRight;}
  };

#endif


