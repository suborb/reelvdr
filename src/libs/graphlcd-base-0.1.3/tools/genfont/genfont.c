/*
 * GraphLCD tool genfont
 *
 * genfont.c  -  a tool to create *.fnt files for use with the GraphLCD
 *               graphics library
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glcdgraphics/bitmap.h>
#include <glcdgraphics/fontheader.h>

static const char *prgname = "genfont";
static const char *version = "0.0.1";

struct tCharRel
{
	unsigned char character;
	unsigned long unicode;
};

tCharRel kISO_8859_1[] =
{
	{ 32,  32}, // Space
	{ 33,  33},
	{ 34,  34},
	{ 35,  35},
	{ 36,  36},
	{ 37,  37},
	{ 38,  38},
	{ 39,  39},
	{ 40,  40},
	{ 41,  41},
	{ 42,  42},
	{ 43,  43},
	{ 44,  44},
	{ 45,  45},
	{ 46,  46},
	{ 47,  47},
	{ 48,  48},
	{ 49,  49},
	{ 50,  50},
	{ 51,  51},
	{ 52,  52},
	{ 53,  53},
	{ 54,  54},
	{ 55,  55},
	{ 56,  56},
	{ 57,  57},
	{ 58,  58},
	{ 59,  59},
	{ 60,  60},
	{ 61,  61},
	{ 62,  62},
	{ 63,  63},
	{ 64,  64},
	{ 65,  65},
	{ 66,  66},
	{ 67,  67},
	{ 68,  68},
	{ 69,  69},
	{ 70,  70},
	{ 71,  71},
	{ 72,  72},
	{ 73,  73},
	{ 74,  74},
	{ 75,  75},
	{ 76,  76},
	{ 77,  77},
	{ 78,  78},
	{ 79,  79},
	{ 80,  80},
	{ 81,  81},
	{ 82,  82},
	{ 83,  83},
	{ 84,  84},
	{ 85,  85},
	{ 86,  86},
	{ 87,  87},
	{ 88,  88},
	{ 89,  89},
	{ 90,  90},
	{ 91,  91},
	{ 92,  92},
	{ 93,  93},
	{ 94,  94},
	{ 95,  95},
	{ 96,  96},
	{ 97,  97},
	{ 98,  98},
	{ 99,  99},
	{100, 100},
	{101, 101},
	{102, 102},
	{103, 103},
	{104, 104},
	{105, 105},
	{106, 106},
	{107, 107},
	{108, 108},
	{109, 109},
	{110, 110},
	{111, 111},
	{112, 112},
	{113, 113},
	{114, 114},
	{115, 115},
	{116, 116},
	{117, 117},
	{118, 118},
	{119, 119},
	{120, 120},
	{121, 121},
	{122, 122},
	{123, 123},
	{124, 124},
	{125, 125},
	{126, 126},
	{160, 160},
	{161, 161},
	{162, 162},
	{163, 163},
	{164, 164},
	{165, 165},
	{166, 166},
	{167, 167},
	{168, 168},
	{169, 169},
	{170, 170},
	{171, 171},
	{172, 172},
	{173, 173},
	{174, 174},
	{175, 175},
	{176, 176},
	{177, 177},
	{178, 178},
	{179, 179},
	{180, 180},
	{181, 181},
	{182, 182},
	{183, 183},
	{184, 184},
	{185, 185},
	{186, 186},
	{187, 187},
	{188, 188},
	{189, 189},
	{190, 190},
	{191, 191},
	{192, 192},
	{193, 193},
	{194, 194},
	{195, 195},
	{196, 196},
	{197, 197},
	{198, 198},
	{199, 199},
	{200, 200},
	{201, 201},
	{202, 202},
	{203, 203},
	{204, 204},
	{205, 205},
	{206, 206},
	{207, 207},
	{208, 208},
	{209, 209},
	{210, 210},
	{211, 211},
	{212, 212},
	{213, 213},
	{214, 214},
	{215, 215},
	{216, 216},
	{217, 217},
	{218, 218},
	{219, 219},
	{220, 220},
	{221, 221},
	{222, 222},
	{223, 223},
	{224, 224},
	{225, 225},
	{226, 226},
	{227, 227},
	{228, 228},
	{229, 229},
	{230, 230},
	{231, 231},
	{232, 232},
	{233, 233},
	{234, 234},
	{235, 235},
	{236, 236},
	{237, 237},
	{238, 238},
	{239, 239},
	{240, 240},
	{241, 241},
	{242, 242},
	{243, 243},
	{244, 244},
	{245, 245},
	{246, 246},
	{247, 247},
	{248, 248},
	{249, 249},
	{250, 250},
	{251, 251},
	{252, 252},
	{253, 253},
	{254, 254},
	{255, 255}
};

void usage(void)
{
	fprintf(stdout, "\n");
	fprintf(stdout, "%s v%s\n", prgname, version);
	fprintf(stdout, "%s is a tool to create *.fnt files that are used by the\n", prgname);
	fprintf(stdout, "        graphlcd plugin for VDR.\n\n");
	fprintf(stdout, "  Usage: %s -f <format> -i infile -o outfile -s size\n\n", prgname);
	fprintf(stdout, "  -f  --format  specifies the format of the output files:\n");
	fprintf(stdout, "                  0 - fnt (default)\n");
	fprintf(stdout, "                  1 - pbm & desc\n");
	fprintf(stdout, "  -i  --input   specifies the name of the input font file (*.ttf)\n");
	fprintf(stdout, "  -o  --output  specifies the base name of the output files\n");
	fprintf(stdout, "  -s  --size    font size of the generated font file\n");
	fprintf(stdout, "\n" );
	fprintf(stdout, "  example: %s -i verdana.ttf -o verdana20 -s 20\n", prgname);
	fprintf(stdout, "           %s -f 1 -i verdana.ttf -o verdana20 -s 20\n", prgname);
}


int main(int argc, char *argv[])
{
	static struct option long_options[] =
	{
		{ "format", required_argument, NULL, 'f'},
		{ "input",  required_argument, NULL, 'i'},
		{ "output", required_argument, NULL, 'o'},
		{ "size",   required_argument, NULL, 's'},
		{ NULL}
	};

	int c;
	int option_index = 0;
	int format = 0;
	std::string inputFontFile = "";
	std::string outputName = "";
	int size = 30;

	while ((c = getopt_long(argc, argv, "f:i:o:s:", long_options, &option_index)) != -1)
	{
		switch (c)
		{
			case 'f':
				format = atoi(optarg);
				break;

			case 'i':
				inputFontFile = optarg;
				break;

			case 'o':
				outputName = optarg;
				break;

			case 's':
				size = atoi(optarg);
				break;

			default:
				usage();
				break;
		}
	}
	if (format > 1)
	{
		usage();
		return 1;
	}
	if (inputFontFile == "")
	{
		usage();
		return 1;
	}
	if (outputName == "")
	{
		outputName = inputFontFile;
	}

	FT_Library library;
	FT_Face face;
	int error;

	error = FT_Init_FreeType(&library);
	if (error)
	{
		// an error occurred during library initialization
		fprintf(stderr, "Error during library initialization.\n");
		return 1;
	}

	error = FT_New_Face(library, inputFontFile.c_str(), 0, &face);
	if (error == FT_Err_Unknown_File_Format)
	{
		// the font file could be opened and read, but it appears
		// that its font format is unsupported
		fprintf(stderr, "Error opening font file. Unknown file format.\n");
		return 1;
	}
	else if (error)
	{
		// another error code means that the font file could not 
		// be opened or read, or simply that it is broken
		fprintf(stderr, "Error opening font file.\n");
		return 1;
	}

	error = FT_Set_Pixel_Sizes(face, 0, size);
	if (error)
	{
		fprintf(stderr, "Error setting font size.\n");
		return 1;
	}

	GLCD::tFontHeader fhdr;
	GLCD::tCharHeader chdr;
	GLCD::cBitmap * bitmap = NULL;
	std::string fileName;
	FILE * fontFile = NULL;
	FILE * descFile = NULL;
	int height = (face->size->metrics.ascender >> 6) - (face->size->metrics.descender >> 6);
	int posX = 0;
	int posY = 0;

	if (format == 0)
	{
		fileName = outputName + ".fnt";
		fontFile = fopen(fileName.c_str(), "wb");
		if (!fontFile)
		{
			fprintf(stderr, "Cannot open file: %s\n", fileName.c_str());
			return 1;
		}
		memcpy(fhdr.sign, GLCD::kFontFileSign, 4);
		fhdr.height = height;
		fhdr.ascent = face->size->metrics.ascender >> 6;
		fhdr.line = face->size->metrics.height >> 6;
		fhdr.reserved = 0;
		fhdr.space = 0;
		fhdr.count = 0;

		fwrite(&fhdr, sizeof(GLCD::tFontHeader), 1, fontFile);
	}
	else
	{
		fileName = outputName + ".desc";
		descFile = fopen(fileName.c_str(), "wb");
		if (!descFile)
		{
			fprintf(stderr, "Cannot open file: %s\n", fileName.c_str());
			return 1;
		}
		bitmap = new GLCD::cBitmap(32 * (face->size->metrics.max_advance >> 6), 6 * height);
		fprintf(descFile, "version:1\n");
		fprintf(descFile, "fontheight:%d\n", height);
		fprintf(descFile, "fontascent:%d\n", face->size->metrics.ascender >> 6);
		fprintf(descFile, "lineheight:%d\n", face->size->metrics.height >> 6);
		fprintf(descFile, "spacebetween:%d\n", 0);
		fprintf(descFile, "spacewidth:%d\n", 0);
	}

	for (unsigned int i = 0; i < sizeof(kISO_8859_1) / sizeof(tCharRel); i++)
	{
		error = FT_Load_Char(face, kISO_8859_1[i].unicode, FT_LOAD_RENDER | FT_LOAD_MONOCHROME);
		if (error)
			continue; // ignore errors

		GLCD::cBitmap * charBitmap = new GLCD::cBitmap(face->glyph->advance.x >> 6, height);
		charBitmap->Clear();
		unsigned char * bufPtr = face->glyph->bitmap.buffer;
		unsigned char pixel;
		for (int y = 0; y < face->glyph->bitmap.rows; y++)
		{
			for (int x = 0; x < face->glyph->bitmap.width; x++)
			{
				pixel = (bufPtr[x / 8] >> (7 - x % 8)) & 1;
				if (pixel)
					charBitmap->DrawPixel((face->glyph->metrics.horiBearingX >> 6) + x, (face->size->metrics.ascender >> 6) - (face->glyph->metrics.horiBearingY >> 6) + y, GLCD::clrBlack);
			}
			bufPtr += face->glyph->bitmap.pitch;
		}
		//char name[16];
		//sprintf(name, "%03d.pbm", kISO_8859_1[i].character);
		//charBitmap->SavePBM(name);

		if (format == 0)
		{
			chdr.character = kISO_8859_1[i].character;
			chdr.width = charBitmap->Width();
			fwrite(&chdr, sizeof(GLCD::tCharHeader), 1, fontFile);
			fwrite(charBitmap->Data(), fhdr.height * charBitmap->LineSize(), 1, fontFile);
			fhdr.count++;
		}
		else
		{
			bitmap->DrawBitmap(posX, posY, *charBitmap, GLCD::clrBlack);
			fprintf(descFile, "%d %d ", posX, kISO_8859_1[i].character);
			posX += face->glyph->advance.x >> 6;
			if ((i % 32) == 31)
			{
				fprintf(descFile, "%d\n", posX);
				posY += height;
				posX = 0;
			}
		}
		delete charBitmap;
	}

	if (format == 0)
	{
		fseek(fontFile, 0, SEEK_SET);
		fwrite(&fhdr, sizeof(GLCD::tFontHeader), 1, fontFile);

		fclose(fontFile);
	}
	else
	{
		if (posX > 0) // write last end marker
			fprintf(descFile, "%d\n", posX);
		fileName = outputName + ".pbm";
		bitmap->SavePBM(fileName);
		delete bitmap;
		fclose(descFile);
	}

	fprintf(stdout, "Font successfully generated.\n");

	return 0;
}
