#ifndef FONT_FT_INCLUDEF
#define FONT_FT_INCLUDEF
#ifdef REELVDR

//Use this header to access cFreetypeFont without duplication of the declaration

#include <ctype.h>
#include <fontconfig/fontconfig.h>
#ifdef BIDI
#include <fribidi.h>
#endif
#include <ft2build.h>
#include FT_FREETYPE_H

#define KERNING_UNKNOWN  (-10000)

struct tKerning {
  uint prevSym;
  int kerning;
  tKerning(uint PrevSym, int Kerning = 0) { prevSym = PrevSym; kerning = Kerning; }
  };

class cGlyph : public cListObject {
private:
  uint charCode;
  uchar *bitmap;
  int advanceX;
  int advanceY;
  int left;  ///< The bitmap's left bearing expressed in integer pixels.
  int top;   ///< The bitmap's top bearing expressed in integer pixels.
  int width; ///< The number of pixels per bitmap row.
  int rows;  ///< The number of bitmap rows.
  int pitch; ///< The pitch's absolute value is the number of bytes taken by one bitmap row, including padding.
  cVector<tKerning> kerningCache;
public:
  cGlyph(uint CharCode, FT_GlyphSlotRec_ *GlyphData);
  virtual ~cGlyph();
  uint CharCode(void) const { return charCode; }
  uchar *Bitmap(void) const { return bitmap; }
  int AdvanceX(void) const { return advanceX; }
  int AdvanceY(void) const { return advanceY; }
  int Left(void) const { return left; }
  int Top(void) const { return top; }
  int Width(void) const { return width; }
  int Rows(void) const { return rows; }
  int Pitch(void) const { return pitch; }
  int GetKerningCache(uint PrevSym) const;
  void SetKerningCache(uint PrevSym, int Kerning);
  };

class cFreetypeFont : public cFont {
private:
  cString fontName;
  int size;
  int height;
  int bottom;
  FT_Library library; ///< Handle to library
  FT_Face face; ///< Handle to face object
  mutable cList<cGlyph> glyphCacheMonochrome;
  mutable cList<cGlyph> glyphCacheAntiAliased;
public:
  int Bottom(void) const { return bottom; }
  int Kerning(cGlyph *Glyph, uint PrevSym) const;
  cGlyph* Glyph(uint CharCode, bool AntiAliased = false) const;
public:
  cFreetypeFont(const char *Name, int CharHeight, int CharWidth = 0);
  virtual ~cFreetypeFont();
  virtual const char *FontName(void) const { return fontName; }
  virtual int Size(void) const { return size; }
  virtual int Width(uint c) const;
  virtual int Width(const char *s) const;
  virtual int Height(void) const { return height; }
  virtual void DrawText(cBitmap *Bitmap, int x, int y, const char *s, tColor ColorFg, tColor ColorBg, int Width) const;
  virtual void DrawText(cPixmap *Pixmap, int x, int y, const char *s, tColor ColorFg, tColor ColorBg, int Width) const;
  };

  #define MAX_BLEND_LEVELS 256

#endif /* REELVDR */
#endif /*FONT_FT_INCLUDEF*/
