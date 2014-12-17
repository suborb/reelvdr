/*
 * GraphLCD graphics library
 *
 * font.c  -  font handling
 *
 * based on graphlcd plugin 0.1.1 for the Video Disc Recorder
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 *  
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#include <stdio.h>
#include <syslog.h>

#include <algorithm>

#include "font.h"
#include "fontheader.h"

#ifdef HAVE_FREETYPE2
#include <ft2build.h>
#include FT_FREETYPE_H
#include <iconv.h>
#endif

namespace GLCD
{

cFont::cFont()
{
	Init();
}

cFont::~cFont()
{
	Unload();
}

bool cFont::LoadFNT(const std::string & fileName)
{
	// cleanup if we already had a loaded font
	Unload();
        SetFontType(1); //original fonts

	FILE * fontFile;
	tFontHeader fhdr;
	tCharHeader chdr;
	int i;
	unsigned char buffer[10000];
	int maxWidth = 0;

	fontFile = fopen(fileName.c_str(), "rb");
	if (!fontFile)
		return false;

	fread(&fhdr, sizeof(tFontHeader), 1, fontFile);
	if (fhdr.sign[0] != kFontFileSign[0] ||
		fhdr.sign[1] != kFontFileSign[1] ||
		fhdr.sign[2] != kFontFileSign[2] ||
		fhdr.sign[3] != kFontFileSign[3])
	{
		fclose(fontFile);
		return false;
	}

	for (i = 0; i < fhdr.count; i++)
	{
		fread(&chdr, sizeof(tCharHeader), 1, fontFile);
		fread(buffer, fhdr.height * ((chdr.width + 7) / 8), 1, fontFile);
		if (characters[chdr.character])
			delete characters[chdr.character];
		characters[chdr.character] = new cBitmap(chdr.width, fhdr.height, buffer);
		if (characters[chdr.character]->Width() > maxWidth)
			maxWidth = characters[chdr.character]->Width();
	}
	totalWidth = maxWidth;
	totalHeight = fhdr.height;
	totalAscent = fhdr.ascent;
	lineHeight = fhdr.line;
	spaceBetween = fhdr.space;
	fclose(fontFile);

	return true;
}

bool cFont::SaveFNT(const std::string & fileName) const
{
	FILE * fontFile;
	tFontHeader fhdr;
	tCharHeader chdr;
	int i;

	fontFile = fopen(fileName.c_str(),"w+b");
	if (!fontFile)
	{
		syslog(LOG_ERR, "cFont::SaveFNT(): Cannot open file: %s for writing\n",fileName.c_str());
		return false;
	}

	memcpy(fhdr.sign, kFontFileSign, 4);
	fhdr.reserved = 0;
	fhdr.height = totalHeight;
	fhdr.ascent = totalAscent;
	fhdr.space = spaceBetween;
	fhdr.line = lineHeight;
	fhdr.count = 0; // just preliminary value

	// write font file header
	fwrite(&fhdr, sizeof(tFontHeader), 1, fontFile);

	for (i = 0; i < 256; i++)
	{
		if (characters[i])
		{
			chdr.character = i;
			chdr.width = characters[i]->Width();
			fwrite(&chdr, sizeof(GLCD::tCharHeader), 1, fontFile);
			fwrite(characters[i]->Data(), fhdr.height * characters[i]->LineSize(), 1, fontFile);
			fhdr.count++;
		}
	}

	// write again font header with actual count achieved
	fseek(fontFile, 0, SEEK_SET);
	fwrite(&fhdr, sizeof(tFontHeader), 1, fontFile);

	fclose(fontFile);

	syslog(LOG_DEBUG, "cFont::SaveFNT(): Font file '%s' written successfully\n", fileName.c_str());

	return true;
}

bool cFont::LoadFT2(const std::string & fileName, const std::string & encoding,
	int size, bool dingBats)
{
	// cleanup if we already had a loaded font
	Unload();
        SetFontType(2); // ft2 fonts
#ifdef HAVE_FREETYPE2
	if (access(fileName.c_str(), F_OK) != 0)
	{
		syslog(LOG_ERR, "cFont::LoadFT2: Font file (%s) does not exist!!", fileName.c_str());
		return false;
	}
	// file exists

	int error = FT_Init_FreeType(&library);
	if (error)
	{
		syslog(LOG_ERR, "cFont::LoadFT2: Could not init freetype library");
		return false;
	}
	error = FT_New_Face(library, fileName.c_str(), 0, &face);
	// everything ok?
	if (error == FT_Err_Unknown_File_Format)
	{
		syslog(LOG_ERR, "cFont::LoadFT2: Font file (%s) could be opened and read, but it appears that its font format is unsupported", fileName.c_str());
		error = FT_Done_Face(face);
		syslog(LOG_ERR, "cFont::LoadFT2: FT_Done_Face(..) returned (%d)", error);
		error = FT_Done_FreeType(library);
		syslog(LOG_ERR, "cFont::LoadFT2: FT_Done_FreeType(..) returned (%d)", error);	    		
		return false;
	}
	else if (error)
	{
		syslog(LOG_ERR, "cFont::LoadFT2: Font file (%s) could not be opened or read, or simply it is broken,\n error code was %x", fileName.c_str(), error);
		error = FT_Done_Face(face);
		syslog(LOG_ERR, "cFont::LoadFT2: FT_Done_Face(..) returned (%d)", error);
		error = FT_Done_FreeType(library);
		syslog(LOG_ERR, "cFont::LoadFT2: FT_Done_FreeType(..) returned (%d)", error);	    		
		return false;
	}

	// set Size
	FT_Set_Char_Size(face, 0, size * 64, 0, 0);

	// get some global parameters
        SetTotalHeight( (face->size->metrics.ascender >> 6) - (face->size->metrics.descender >> 6) );
        SetTotalWidth ( face->size->metrics.max_advance >> 6 );
        SetTotalAscent( face->size->metrics.ascender >> 6 );
        SetLineHeight ( face->size->metrics.height >> 6 );
        SetSpaceBetween( 0 );

        characters_cache=new cBitmapCache();

	return true;
#else
	syslog(LOG_ERR, "cFont::LoadFT2: glcdgraphics was compiled without FreeType2 support!!!");
	return false;
#endif
}

int cFont::Width(int ch) const
{
#ifdef HAVE_FREETYPE2
      if ( FontType() == 2){
              cBitmapFt2 *cmybitmap=GetGlyph(ch);
      if (cmybitmap)
              return cmybitmap->Width();
      else
              return 0;
      }else{
#endif
      if (characters[(unsigned char) ch])
              return characters[(unsigned char) ch]->Width();
      else
              return 0;
#ifdef HAVE_FREETYPE2
      }
#endif
}

int cFont::Width(const std::string & str) const
{
	unsigned int i = 0;
        int sum = 0;
        int c,c0,c1,c2,c3,symcount=0;

	for (i = 0; i < str.length(); i++)
	{
                c = str[i];
                c0 = str[i];
                c1 = str[i+1];
                c2 = str[i+2];
                c3 = str[i+3];
                c0 &=0xff; c1 &=0xff; c2 &=0xff; c3 &=0xff;
 
                if( c0 >= 0xc2 && c0 <= 0xdf && c1 >= 0x80 && c1 <= 0xbf ){ //2 byte UTF-8 sequence found
                i+=1;
                        c = ((c0&0x1f)<<6) | (c1&0x3f);
                }else if(  (c0 == 0xE0 && c1 >= 0xA0 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf)
                        || (c0 >= 0xE1 && c1 <= 0xEC && c1 >= 0x80 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf)
                        || (c0 == 0xED && c1 >= 0x80 && c1 <= 0x9f && c2 >= 0x80 && c2 <= 0xbf)
                        || (c0 >= 0xEE && c0 <= 0xEF && c1 >= 0x80 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf) ){  //3 byte UTF-8 sequence found
                        c = ((c0&0x0f)<<4) | ((c1&0x3f)<<6) | (c2&0x3f);
                        i+=2;
            }else if(  (c0 == 0xF0 && c1 >= 0x90 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf && c3 >= 0x80 && c3 <= 0xbf)
                        || (c0 >= 0xF1 && c0 >= 0xF3 && c1 >= 0x80 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf && c3 >= 0x80 && c3 <= 0xbf)
                        || (c0 == 0xF4 && c1 >= 0x80 && c1 <= 0x8f && c2 >= 0x80 && c2 <= 0xbf && c3 >= 0x80 && c3 <= 0xbf) ){  //4 byte UTF-8 sequence found
                        c = ((c0&0x07)<<2) | ((c1&0x3f)<<4) | ((c2&0x3f)<<6) | (c3&0x3f);
                        i+=3;
            }
                symcount++;
          sum += Width(c);
      }
      sum += spaceBetween * (symcount - 1);
	return sum;
}

int cFont::Width(const std::string & str, unsigned int len) const
{
	unsigned int i = 0;
	int sum = 0;

      int c,c0,c1,c2,c3,symcount=0;

     for (i = 0; i < str.length() && symcount < len; i++)
	if (std::min(str.length(), (size_t) len) > 1)
	{
                c = str[i];
                c0 = str[i];
                c1 = str[i+1];
                c2 = str[i+2];
                c3 = str[i+3];
                c0 &=0xff; c1 &=0xff; c2 &=0xff; c3 &=0xff;
 
                if( c0 >= 0xc2 && c0 <= 0xdf && c1 >= 0x80 && c1 <= 0xbf ){ //2 byte UTF-8 sequence found
                i+=1;
                        c = ((c0&0x1f)<<6) | (c1&0x3f);
                }else if(  (c0 == 0xE0 && c1 >= 0xA0 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf)
                        || (c0 >= 0xE1 && c1 <= 0xEC && c1 >= 0x80 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf)
                        || (c0 == 0xED && c1 >= 0x80 && c1 <= 0x9f && c2 >= 0x80 && c2 <= 0xbf)
                        || (c0 >= 0xEE && c0 <= 0xEF && c1 >= 0x80 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf) ){  //3 byte UTF-8 sequence found
                        c = ((c0&0x0f)<<4) | ((c1&0x3f)<<6) | (c2&0x3f);
                        i+=2;
            }else if(  (c0 == 0xF0 && c1 >= 0x90 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf && c3 >= 0x80 && c3 <= 0xbf)
                        || (c0 >= 0xF1 && c0 >= 0xF3 && c1 >= 0x80 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf && c3 >= 0x80 && c3 <= 0xbf)
                        || (c0 == 0xF4 && c1 >= 0x80 && c1 <= 0x8f && c2 >= 0x80 && c2 <= 0xbf && c3 >= 0x80 && c3 <= 0xbf) ){  //4 byte UTF-8 sequence found
                        c = ((c0&0x07)<<2) | ((c1&0x3f)<<4) | ((c2&0x3f)<<6) | (c3&0x3f);
                        i+=3;
            }
                symcount++;
          sum += Width(c);
      }
      sum += spaceBetween * (symcount - 1);
 

	return sum;
}

int cFont::Height(int ch) const
{
 #ifdef HAVE_FREETYPE2
       if ( FontType() == 2){
               cBitmapFt2 *cmybitmap=GetGlyph(ch);
       if (cmybitmap)
               return cmybitmap->Height();
       else
               return 0;
       }else{
 #endif
       if (characters[(unsigned char) ch])
               return characters[(unsigned char) ch]->Height();
       else
               return 0;
 #ifdef HAVE_FREETYPE2
       }
 #endif
}

int cFont::Height(const std::string & str) const
{
	unsigned int i;
	int sum = 0;

	for (i = 0; i < str.length(); i++)
		sum = std::max(sum, Height(str[i]));
	return sum;
}

int cFont::Height(const std::string & str, unsigned int len) const
{
	unsigned int i;
	int sum = 0;

	for (i = 0; i < str.length() && i < len; i++)
		sum = std::max(sum, Height(str[i]));
	return sum;
}

const cBitmap * cFont::GetCharacter(int ch) const
{
 #ifdef HAVE_FREETYPE2
       if ( FontType() == 2){
               return GetGlyph(ch);
       }else{
 #endif
	return characters[(unsigned char) ch];
 #ifdef HAVE_FREETYPE2
       }
 #endif
}

 #ifdef HAVE_FREETYPE2
 cBitmapFt2* cFont::GetGlyph(int c) const
 {
     //lookup in cache
       cBitmapCache *ptr = characters_cache->start;
       while(ptr != NULL){
               int charcode = ((cBitmapFt2 *)(ptr->pointer))->GetCharcode();
       if(c == charcode){
                       return ((cBitmapFt2 *)(ptr->pointer));
               }
               ptr = ptr->next;
       }

     FT_UInt glyph_index;
     //Get FT char index
     glyph_index = FT_Get_Char_Index(face, c);

     //Load the char
     int error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
     if (error)
     {
       syslog(LOG_ERR, "cFont::LoadFT2: ERROR when calling FT_Load_Glyph: %x", error);
     }

       FT_Render_Mode  rmode = FT_RENDER_MODE_MONO;
 #if ( (FREETYPE_MAJOR == 2 && FREETYPE_MINOR == 1 && FREETYPE_PATCH >= 7) || (FREETYPE_MAJOR == 2 && FREETYPE_MINOR == 2 && FREETYPE_PATCH <= 1) )
       if(c == 32) rmode = FT_RENDER_MODE_NORMAL;
 #endif

     // convert to a mono bitmap
     error = FT_Render_Glyph(face->glyph, rmode);
     if (error)
     {
         syslog(LOG_ERR, "cFont::LoadFT2: ERROR when calling FT_Render_Glyph: %x", error);
     }else{
         // now, fill our pixel data
       cBitmapFt2 *charBitmap = new cBitmapFt2(face->glyph->advance.x >> 6, TotalHeight(), c);
       charBitmap->Clear();
       unsigned char * bufPtr = face->glyph->bitmap.buffer;
       unsigned char pixel;
               for (int y = 0; y < face->glyph->bitmap.rows; y++)
       {
               for (int x = 0; x < face->glyph->bitmap.width; x++)
               {
                       pixel = (bufPtr[x / 8] >> (7 - x % 8)) & 1;
                       if (pixel)
                   charBitmap->DrawPixel((face->glyph->metrics.horiBearingX >> 6) + x,
                                       (face->size->metrics.ascender >> 6) - (face->glyph->metrics.horiBearingY >> 6) + y,
                                       GLCD::clrBlack);
               }
           bufPtr += face->glyph->bitmap.pitch;
               }

               // adjust maxwidth if necessary
       //if (totalWidth < charBitmap->Width())
         //    totalWidth = charBitmap->Width();

               characters_cache->PushBack(charBitmap);
               return charBitmap;
     }
       return NULL;
 }
 #endif

void cFont::SetCharacter(char ch, cBitmap * bitmapChar)
{
	// adjust maxwidth if necessary
	if (totalWidth < bitmapChar->Width())
		totalWidth = bitmapChar->Width();

	// delete if already allocated
	if (characters[(unsigned char) ch])
		delete characters[(unsigned char) ch];

	// store new character
	characters[(unsigned char) ch] = bitmapChar;
}

void cFont::Init()
{
	totalWidth = 0;
	totalHeight = 0;
	totalAscent = 0;
	spaceBetween = 0;
	lineHeight = 0;
	for (int i = 0; i < 256; i++)
	{
		characters[i] = NULL;
	}
}

void cFont::Unload()
{
	// cleanup
	for (int i = 0; i < 256; i++)
	{
		if (characters[i])
		{
			delete characters[i];
		}
	}
	// re-init
	Init();
}


} // end of namespace
