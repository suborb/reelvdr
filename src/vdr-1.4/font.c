/*
 * font.c: Font handling for the DVB On Screen Display
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: font.c 1.13 2006/04/16 10:59:15 kls Exp $
 */

#include "font.h"
#include <ctype.h>
#include "debug.h"
#include <ft2build.h>
#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "config.h"
#include "osd.h"
#include "tools.h"

const char *DefaultFontOsd = "Sans Serif:Bold";
const char *DefaultFontSml = "Sans Serif";
const char *DefaultFontFix = "Courier:Bold";

#define KERNING_UNKNOWN  (-10000)

// --- cFreetypeFont ---------------------------------------------------------

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

//int cFont::generation = 0; // GT: incremented when a new font is loaded (used for font caching)

cGlyph::cGlyph(uint CharCode, FT_GlyphSlotRec_ *GlyphData)
{
  charCode = CharCode;
  advanceX = GlyphData->advance.x >> 6;
  advanceY = GlyphData->advance.y >> 6;
  left = GlyphData->bitmap_left;
  top = GlyphData->bitmap_top;
  width = GlyphData->bitmap.width;
  rows = GlyphData->bitmap.rows;
  pitch = GlyphData->bitmap.pitch;
  bitmap = MALLOC(uchar, rows * pitch);
  memcpy(bitmap, GlyphData->bitmap.buffer, rows * pitch);
}

cGlyph::~cGlyph()
{
  free(bitmap);
}

class cFreetypeFont : public cFont {
private:
  int height;
  int bottom;
  FT_Library library; ///< Handle to library
  FT_Face face; ///< Handle to face object
  mutable cHash<cGlyph> glyphCacheMonochrome;
  mutable cHash<cGlyph> glyphCacheAntiAliased;
public:
  int Kerning(cGlyph *Glyph, uint PrevSym) const;
  cFreetypeFont(const char *Name, int CharHeight, int CharWidth = 0);
  cGlyph* Glyph(uint CharCode, bool AntiAliased = false) const;
  inline int Bottom(void) const { return bottom; }
  virtual ~cFreetypeFont();
  virtual int Width(uint c) const;
  virtual int Width(const char *s) const;
  virtual inline int Height(void) const { return height; }
  virtual void DrawText(cBitmap *Bitmap, int x, int y, const char *s, tColor ColorFg, tColor ColorBg, int Width) const;
  };

cFreetypeFont::cFreetypeFont(const char *Name, int CharHeight, int CharWidth)
{
  height = 0;
  bottom = 0;
  FT_Matrix matrix;
  matrix.xx = (FT_Fixed)(0.9* 0x10000L);
  matrix.xy = (FT_Fixed)(0  * 0x10000L);
  matrix.yx = (FT_Fixed)(0  * 0x10000L);
  matrix.yy = (FT_Fixed)(1  * 0x10000L);

  int error = FT_Init_FreeType(&library);
  if (!error) {
     error = FT_New_Face(library, Name, 0, &face);
     if (!error) {
        FT_Set_Transform( face, &matrix, NULL);
        if (face->num_fixed_sizes && face->available_sizes) { // fixed font
           // TODO what exactly does all this mean?
           height = face->available_sizes->height;
           for (uint sym ='A'; sym < 'z'; sym++) { // search for descender for fixed font FIXME
               FT_UInt glyph_index = FT_Get_Char_Index(face, sym);
               error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
               if (!error) {
                  error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
                  if (!error) {
                     if (face->glyph->bitmap.rows-face->glyph->bitmap_top > bottom)
                        bottom = face->glyph->bitmap.rows-face->glyph->bitmap_top;
                     }
                  else
                     esyslog("ERROR: FreeType: error %d in FT_Render_Glyph", error);
                  }
               else
                  esyslog("ERROR: FreeType: error %d in FT_Load_Glyph", error);
               }
           }
        else {
           error = FT_Set_Char_Size(face, // handle to face object
                                    CharWidth * 64,  // CharWidth in 1/64th of points
                                    CharHeight * 64, // CharHeight in 1/64th of points
                                    0,    // horizontal device resolution
                                    0);   // vertical device resolution
           if (!error) {
              height = ((face->size->metrics.ascender-face->size->metrics.descender) + 63) / 64;
              bottom = abs((face->size->metrics.descender - 63) / 64);
              }
           else
              esyslog("ERROR: FreeType: error %d during FT_Set_Char_Size (font = %s)\n", error, Name);
           }
        }
     else
        esyslog("ERROR: FreeType: load error %d (font = %s)", error, Name);
     }
  else
     esyslog("ERROR: FreeType: initialization error %d (font = %s)", error, Name);
}

cFreetypeFont::~cFreetypeFont()
{
  FT_Done_Face(face);
  FT_Done_FreeType(library);
}

int cFreetypeFont::Kerning(cGlyph *Glyph, uint PrevSym) const
{
  int kerning = 0;
  if (Glyph && PrevSym) {
     kerning = Glyph->GetKerningCache(PrevSym);
     if (kerning == KERNING_UNKNOWN) {
        FT_Vector delta;
        FT_UInt glyph_index = FT_Get_Char_Index(face, Glyph->CharCode());
        FT_UInt glyph_index_prev = FT_Get_Char_Index(face, PrevSym);
        FT_Get_Kerning(face, glyph_index_prev, glyph_index, FT_KERNING_DEFAULT, &delta);
        kerning = delta.x / 64;
        Glyph->SetKerningCache(PrevSym, kerning);
        }
     }
  return kerning;
}

cGlyph* cFreetypeFont::Glyph(uint CharCode, bool AntiAliased) const
{
  // Lookup in cache:
  cHash<cGlyph> *glyphCache = AntiAliased ? &glyphCacheAntiAliased : &glyphCacheMonochrome;
  cGlyph *g = glyphCache->Get(CharCode);
      if (g)
         return g;

  FT_UInt glyph_index = FT_Get_Char_Index(face, CharCode);

  // Load glyph image into the slot (erase previous one):
  int error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
  if (error)
     esyslog("ERROR: FreeType: error during FT_Load_Glyph");
  else {
#if ((FREETYPE_MAJOR == 2 && FREETYPE_MINOR == 1 && FREETYPE_PATCH >= 7) || (FREETYPE_MAJOR == 2 && FREETYPE_MINOR == 2 && FREETYPE_PATCH <= 1))// TODO workaround for bug? which one?
     if (AntiAliased || CharCode == 32)
#else
     if (AntiAliased)
#endif
        error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
     else
        error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO);
     if (error)
        esyslog("ERROR: FreeType: error during FT_Render_Glyph %d, %d\n", CharCode, glyph_index);
     else { //new bitmap
        cGlyph *Glyph = new cGlyph(CharCode, face->glyph);
        glyphCache->Add(Glyph, CharCode);
        return Glyph;
        }
     }
  return NULL;
}

int cFreetypeFont::Width(uint c) const
{
  cGlyph *g = Glyph(c, Setup.AntiAlias);
  return g ? g->AdvanceX() : 0;
}

int cFreetypeFont::Width(const char *s) const
{
  int w = 0;
  if (s) {
     uint prevSym = 0;
     while (*s) {
           int sl = Utf8CharLen(s);
           uint sym = Utf8CharGet(s, sl);
           s += sl;
           cGlyph *g = Glyph(sym, Setup.AntiAlias);
           if (g)
              w += g->AdvanceX() + Kerning(g, prevSym);
           prevSym = sym;
           }
     }
  return w;
}

#define MAX_BLEND_LEVELS 256

void cFreetypeFont::DrawText(cBitmap *Bitmap, int x, int y, const char *s, tColor ColorFg, tColor ColorBg, int Width) const
{
  //printf("Font::DrawText: x: %i y: %i Text: %s w: %i\n", x, y, s, Width );
  if (s && height) { // checking height to make sure we actually have a valid font
     bool AntiAliased = Setup.AntiAlias && Bitmap->Bpp() >= 8;
     bool TransparentBackground = ColorBg == clrTransparent;
     static int16_t BlendLevelIndex[MAX_BLEND_LEVELS]; // tIndex is 8 bit unsigned, so a negative value can be used to mark unused entries
     if (AntiAliased && !TransparentBackground)
        memset(BlendLevelIndex, 0xFF, sizeof(BlendLevelIndex)); // initializes the array with negative values
     tIndex fg = Bitmap->Index(ColorFg);
     uint prevSym = 0;
     while (*s) {
           int sl = Utf8CharLen(s);
           uint sym = Utf8CharGet(s, sl);
           s += sl;
           cGlyph *g = Glyph(sym, AntiAliased);
           if (!g)
              continue;
           int kerning = Kerning(g, prevSym);
           prevSym = sym;
           uchar *buffer = g->Bitmap();
           int symWidth = g->Width();
           int symLeft = g->Left();
           int symTop = g->Top();
           int symPitch = g->Pitch();
           if (Width && x + symWidth + symLeft + kerning - 1 > Width)
              break; // we don't draw partial characters
           int px_tmp_sum = symLeft + kerning + x;
           int py_tmp_sum = y + (height - Bottom() - symTop);
           if (x + symWidth + symLeft + kerning > 0) {
              for (int row = 0; row < g->Rows(); row++) {
                  for (int pitch = 0; pitch < symPitch; pitch++) {
                      uchar bt = *(buffer + (row * symPitch + pitch));
                      if (AntiAliased) {
                         if (bt > 0x00) {
                            int px = pitch + px_tmp_sum;
                            int py = row + py_tmp_sum;
                            tColor bg;
                            if (bt == 0xFF)
                               bg = fg;
                            else if (TransparentBackground)
                               bg = Bitmap->Index(Bitmap->Blend(ColorFg, Bitmap->GetColor(px, py), bt));
                            else if (BlendLevelIndex[bt] >= 0)
                               bg = BlendLevelIndex[bt];
                            else
                               bg = BlendLevelIndex[bt] = Bitmap->Index(Bitmap->Blend(ColorFg, ColorBg, bt));
                            Bitmap->SetIndexFast(px, py, bg);
                            }
                         }
                      else { //monochrome rendering
                         for (int col = 0; col < 8 && col + pitch * 8 <= symWidth; col++) {
                             if (bt & 0x80)
                                Bitmap->SetIndex(x + col + pitch * 8 + symLeft + kerning, y + row + (height - Bottom() - symTop), fg);
                             bt <<= 1;
                             }
                         }
                      }
                  }
              }
           x += g->AdvanceX() + kerning;
           if (x > Bitmap->Width() - 1)
              break;
           }
     }
}

// --- cDummyFont ------------------------------------------------------------

// A dummy font, in case there are no fonts installed:

class cDummyFont : public cFont {
public:
  virtual int Width(uint c) const { return 10; }
  virtual int Width(const char *s) const { return 50; }
  virtual int Height(void) const { return 20; }
  virtual void DrawText(cBitmap *Bitmap, int x, int y, const char *s, tColor ColorFg, tColor ColorBg, int Width) const {}
  };

// --- cFont -----------------------------------------------------------------

cFont *cFont::fonts[eDvbFontSize] = { NULL };

void cFont::SetFont(eDvbFont Font, const char *Name, int CharHeight)
{
  PRINTF("=========== SetFont: %s ===========\n", Name);
  cFont *f = CreateFont(Name, CharHeight);
  if (!f || !f->Height())
     f = new cDummyFont;
  delete fonts[Font];
  fonts[Font] = f;
}

const cFont *cFont::GetFont(eDvbFont Font)
{
  if (Setup.UseSmallFont == 0 && Font == fontSml)
     Font = fontOsd;
  else if (Setup.UseSmallFont == 2)
     Font = fontSml;
  if (!fonts[Font]) {
     switch (Font) {
       case fontOsd: SetFont(Font, AddDirectory(FONTDIR, Setup.FontOsd), Setup.FontOsdSize); break;
       case fontSml: SetFont(Font, AddDirectory(FONTDIR, Setup.FontSml), Setup.FontSmlSize); break;
       case fontFix: SetFont(Font, AddDirectory(FONTDIR, Setup.FontFix), Setup.FontFixSize); break;
       }
     }
  return fonts[Font];
}

cFont *cFont::CreateFont(const char *Name, int CharHeight, int CharWidth)
{
  cString fn = GetFontFileName(Name);
  if (*fn)
     return new cFreetypeFont(fn, CharHeight, CharWidth);
  return NULL;
}

bool cFont::GetAvailableFontNames(cStringList *FontNames, bool Monospaced)
{
  if (!FontNames->Size()) {
     FcInit();
     FcObjectSet *os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, NULL);
     FcPattern *pat = FcPatternCreate();
     FcPatternAddBool(pat, FC_SCALABLE, FcTrue);
     if (Monospaced)
        FcPatternAddInteger(pat, FC_SPACING, FC_MONO);
     FcFontSet* fontset = FcFontList(NULL, pat, os);
     for (int i = 0; i < fontset->nfont; i++) {
         char *s = (char *)FcNameUnparse(fontset->fonts[i]);
         if (s) {
            // Strip i18n stuff:
            char *c = strchr(s, ':');
            if (c) {
               char *p = strchr(c + 1, ',');
               if (p)
                  *p = 0;
               }
            char *p = strchr(s, ',');
            if (p) {
               if (c)
                  memmove(p, c, strlen(c) + 1);
               else
                  *p = 0;
               }
            // Make it user presentable:
            s = strreplace(s, "\\", ""); // '-' is escaped
            s = strreplace(s, "style=", "");
            FontNames->Append(s); // takes ownership of s
            }
         }
     FcFontSetDestroy(fontset);
     FcPatternDestroy(pat);
     FcObjectSetDestroy(os);
     FcFini();
     FontNames->Sort();
     }
  return FontNames->Size() > 0;
}

cString cFont::GetFontFileName(const char *FontName)
{
  cString FontFileName;
  if (FontName) {
     char *fn = strdup(FontName);
     fn = strreplace(fn, ":", ":style=");
     fn = strreplace(fn, "-", "\\-");
     FcInit();
     FcPattern *pat = FcNameParse((FcChar8 *)fn);
     FcPatternAddBool(pat, FC_SCALABLE, FcTrue);
     FcConfigSubstitute(NULL, pat, FcMatchPattern);
     FcDefaultSubstitute(pat);
     FcFontSet *fontset = FcFontSort(NULL, pat, FcFalse, NULL, NULL);
     if (fontset) {
        for (int i = 0; i < fontset->nfont; i++) {
            FcBool scalable;
            FcPatternGetBool(fontset->fonts[i], FC_SCALABLE, 0, &scalable);
            if (scalable) {
               FcChar8 *s = NULL;
               FcPatternGetString(fontset->fonts[i], FC_FILE, 0, &s);
               FontFileName = (char *)s;
               break;
               }
            }
        FcFontSetDestroy(fontset);
        }
     else
        esyslog("ERROR: no usable font found for '%s'", FontName);
     FcPatternDestroy(pat);
     free(fn);
     FcFini();
     }
  return FontFileName;
}

// --- cTextWrapper ----------------------------------------------------------

cTextWrapper::cTextWrapper(void)
{
  text = eol = NULL;
  lines = 0;
  lastLine = -1;
}

cTextWrapper::cTextWrapper(const char *Text, const cFont *Font, int Width)
{
  text = NULL;
  Set(Text, Font, Width);
}

cTextWrapper::~cTextWrapper()
{
  free(text);
}

void cTextWrapper::Set(const char *Text, const cFont *Font, int Width)
{
  free(text);
  text = Text ? strdup(Text) : NULL;
  eol = NULL;
  lines = 0;
  lastLine = -1;
  if (!text)
     return;
  lines = 1;
  if (Width <= 0)
     return;

  char *Blank = NULL;
  char *Delim = NULL;
  int w = 0;

  stripspace(text); // strips trailing newlines

  for (char *p = text; *p; ) {
      int sl = Utf8CharLen(p);
      uint sym = Utf8CharGet(p, sl);
      if (sym == '\n') {
         lines++;
         w = 0;
         Blank = Delim = NULL;
         p++;
         continue;
         }
      else if (sl == 1 && isspace(sym))
         Blank = p;
      int cw = Font->Width(sym);
      if (w + cw > Width) {
         if (Blank) {
            *Blank = '\n';
            p = Blank;
            continue;
            }
         else {
            // Here's the ugly part, where we don't have any whitespace to
            // punch in a newline, so we need to make room for it:
            if (Delim)
               p = Delim + 1; // let's fall back to the most recent delimiter
            char *s = MALLOC(char, strlen(text) + 2); // The additional '\n' plus the terminating '\0'
            int l = p - text;
            strncpy(s, text, l);
            s[l] = '\n';
            strcpy(s + l + 1, p);
            free(text);
            text = s;
            p = text + l;
            continue;
            }
         }
      else
         w += cw;
      if (strchr("-.,:;!?_", *p)) {
         Delim = p;
         Blank = NULL;
         }
      p += sl;
      }
}

const char *cTextWrapper::Text(void)
{
  if (eol) {
     *eol = '\n';
     eol = NULL;
     }
  return text;
}

const char *cTextWrapper::GetLine(int Line)
{
  char *s = NULL;
  if (Line < lines) {
     if (eol) {
        *eol = '\n';
        if (Line == lastLine + 1)
           s = eol + 1;
        eol = NULL;
        }
     if (!s) {
        s = text;
        for (int i = 0; i < Line; i++) {
            s = strchr(s, '\n');
            if (s)
               s++;
            else
               break;
            }
        }
     if (s) {
        if ((eol = strchr(s, '\n')) != NULL)
           *eol = 0;
        }
     lastLine = Line;
     }
  return s;
}
