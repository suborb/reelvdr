#ifndef FONT_HELPER_H
#define FONT_HELPER_H

/**
 * just copies from VDR's *.c-files
 */

#include <ft2build.h>
#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define KERNING_UNKNOWN  (-10000)

#define MAX_BLEND_LEVELS 256

class tKerning : public cListObject {
public:
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
  cHash<tKerning> kerningCache;
public:
  cGlyph(uint CharCode, FT_GlyphSlotRec_ *GlyphData);
  virtual ~cGlyph();
  inline uint CharCode(void) const { return charCode; }
  inline uchar *Bitmap(void) const { return bitmap; }
  inline int AdvanceX(void) const { return advanceX; }
  inline int AdvanceY(void) const { return advanceY; }
  inline int Left(void) const { return left; }
  inline int Top(void) const { return top; }
  inline int Width(void) const { return width; }
  inline int Rows(void) const { return rows; }
  inline int Pitch(void) const { return pitch; }
  inline int GetKerningCache(uint PrevSym) const
  {
    tKerning *kern = kerningCache.Get(PrevSym);
    if(kern)
         return kern->kerning;
    return KERNING_UNKNOWN;
  }

  inline void SetKerningCache(uint PrevSym, int Kerning)
  {
    tKerning *kern = new tKerning(PrevSym, Kerning);
    kerningCache.Add(kern, PrevSym);
  }
};

class cFreetypeFont : public cFont {
private:
  int height;
  int bottom;
  FT_Library library; ///< Handle to library
  FT_Face face; ///< Handle to face object
  mutable cHash<cGlyph> glyphCacheMonochrome;
  mutable cHash<cGlyph> glyphCacheAntiAliased;
public:
  cFreetypeFont(const char *Name, int CharHeight, int CharWidth = 0);
  cGlyph* Glyph(uint CharCode, bool AntiAliased = false) const;
  inline int Bottom(void) const { return bottom; }
  int Kerning(cGlyph *Glyph, uint PrevSym) const;
  virtual ~cFreetypeFont();
  virtual int Width(uint c) const;
  virtual int Width(const char *s) const;
  virtual inline int Height(void) const { return height; }
  virtual void DrawText(cBitmap *Bitmap, int x, int y, const char *s, tColor ColorFg, tColor ColorBg, int Width) const;
  };

#endif
