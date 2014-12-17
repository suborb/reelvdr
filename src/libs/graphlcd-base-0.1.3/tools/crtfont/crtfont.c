/*
 * GraphLCD tool crtfont
 *
 * crtfont.c  -  a tool to create *.fnt files for use with the GraphLCD
 *               graphics library
 *
 * based on graphlcd plugin 0.1.1 for the Video Disc Recorder
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
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
#include <glcdgraphics/bitmap.h>
#include <glcdgraphics/fontheader.h>

static const char *prgname = "crtfont";
static const char *version = "0.1.5";

const int kMaxLineLength = 1024;

enum ePicFormat
{
	undefined,
	PBM
};


void usage(void);

char * trimleft(char * str)
{
	char * s = str;
	while (*s == ' ' || *s == '\t')
		s++;
	strcpy(str, s);
	return str;
}

char * trimright(char * str)
{
	char * s = str + strlen(str) - 1;
	while (s >= str && (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r'))
		*s-- = 0;
	return str;
}

char * trim(char * str)
{
	return trimleft(trimright(str));
}


int main(int argc, char *argv[])
{
	ePicFormat picFormat = undefined;
	GLCD::tFontHeader fhdr;
	GLCD::tCharHeader chdr;
	char * picName = NULL;
	char * descName = NULL;
	char * fontName = NULL;
	FILE * descFile;
	FILE * fontFile;
	bool error = false;
	GLCD::cBitmap * bitmap = NULL;
	GLCD::cBitmap * tmpBitmap = NULL;
	GLCD::cBitmap * charBitmap = NULL;
	char line[kMaxLineLength];
	int l = 0;
	char * token;
	int startOffset, endOffset;
	int spaceWidth;
	int version;
	char * ptr;


	memcpy(fhdr.sign, GLCD::kFontFileSign, 4);

	static struct option long_options[] =
	{
		{ "format",   required_argument, NULL, 'f'},
		{ "bmpfile",  required_argument, NULL, 'b'},
		{ "descfile", required_argument, NULL, 'd'},
		{ "outfile",  required_argument, NULL, 'o'},
		{ NULL}
	};

	int c, option_index = 0;
	while ((c = getopt_long(argc, argv, "f:b:d:o:", long_options, &option_index)) != -1)
	{
		switch (c)
		{
			case 'f':
				if (strcasecmp(optarg, "PBM") == 0)
					picFormat = PBM;
				break;

			case 'b':
				picName = strdup(optarg);
				break;

			case 'd':
				descName = strdup(optarg);
				break;

			case 'o':
				fontName = strdup(optarg);
				break;

			default:
				return 1;
		}
	}

	if (picFormat == undefined)
	{
		fprintf(stderr, "ERROR: You have to specify the format   (-f <format>)\n");
		error = true;
	}
	if (!picName)
	{
		fprintf(stderr, "ERROR: You have to specify the bmpfile  (-b bmpfile)\n");
		error = true;
	}
	if (!descName)
	{
		fprintf(stderr, "ERROR: You have to specify the descfile (-d descfile)\n");
		error = true;
	}
	if (!fontName)
	{
		fprintf(stderr, "ERROR: You have to specify the outfile  (-o outfile)\n");
		error = true;
	}

	if (error)
	{
		usage();
		return 1;
	}

	descFile = fopen(descName,"r");
	if (!descFile)
	{
		fprintf(stderr, "Cannot open file: %s\n",descName);
		return 2;
	}
	fontFile = fopen(fontName,"wb");
	if (!fontFile)
	{
		fprintf(stderr, "Cannot open file: %s\n",fontName);
		return 2;
	}

	// Load Picture
	switch (picFormat)
	{
		case PBM:
			bitmap = new GLCD::cBitmap(0, 0);
			bitmap->LoadPBM(picName);
			if (!bitmap)
			{
				fprintf(stderr, "Cannot open file: %s\n",picName);
				return 2;
			}
			break;

		default:
			return 2;
	}

	if (!bitmap)
		return 3;

	memset(&fhdr, 0, sizeof(GLCD::tFontHeader));
	memcpy(fhdr.sign, GLCD::kFontFileSign, 4);
	spaceWidth = 0;

	fwrite(&fhdr, sizeof(GLCD::tFontHeader), 1, fontFile);

	version = 0;
	fgets(line, sizeof(line), descFile);
	trim(line);
	if (strstr(line, "version") != NULL)
	{
		ptr = strstr(line, ":");
		version = atoi(ptr + 1);
	}
	if (version != 1)
	{
		fprintf(stderr, "Wrong description file format version (found %d, expected 1)!\n", version);
		return 2;
	}
	while (!feof(descFile))
	{
		fgets(line, sizeof(line), descFile);
		trim(line);
		if (strstr(line, "fontheight") != NULL)
		{
			ptr = strstr(line, ":");
			fhdr.height = atoi(ptr + 1);
		}
		else if (strstr(line, "fontascent") != NULL)
		{
			ptr = strstr(line, ":");
			fhdr.ascent = atoi(ptr + 1);
		}
		else if (strstr(line, "lineheight") != NULL)
		{
			ptr = strstr(line, ":");
			fhdr.line = atoi(ptr + 1);
		}
		else if (strstr(line, "spacebetween") != NULL)
		{
			ptr = strstr(line, ":");
			fhdr.space = atoi(ptr + 1);
		}
		else if (strstr(line, "spacewidth") != NULL)
		{
			ptr = strstr(line, ":");
			spaceWidth = atoi(ptr + 1);
		}
		else
		{
			token = strtok(line, " ");
			if (token)
			{
				startOffset = atoi(token);

				// get character
				token = strtok(NULL, " ");
				while (token)
				{
					if (strlen(token) == 1)
						chdr.character = (unsigned char) token[0];
					else
						chdr.character = atoi(token);

					// get EndOffset
					token = strtok(NULL, " ");
					endOffset = atoi(token);
					chdr.width = endOffset - startOffset;
					tmpBitmap = bitmap->SubBitmap(startOffset, l * fhdr.height, endOffset - 1, (l + 1) * fhdr.height - 1);
					if (spaceWidth > 0)
					{
						// calculate width of this character
						int x;
						int y;
						int left = 255;
						int right = 0;
						for (y = 0; y < tmpBitmap->Height(); y++)
						{
							for (x = 0; x < tmpBitmap->Width(); x++)
							{
								if (tmpBitmap->GetPixel(x, y))
									break;
							}
							if (x < tmpBitmap->Width() && x < left)
								left = x;
							for (x = tmpBitmap->Width() - 1; x >= 0; x--)
							{
								if (tmpBitmap->GetPixel(x, y))
									break;
							}
							if (x >= 0 && x > right)
								right = x;
						}
						if (left > right)
						{
							left = 0;
							right = spaceWidth - 1;
						}
						charBitmap = tmpBitmap->SubBitmap(left, 0, right, fhdr.height - 1);
						chdr.width = charBitmap->Width();
						fwrite(&chdr, sizeof(GLCD::tCharHeader), 1, fontFile);
						fwrite(charBitmap->Data(), fhdr.height * charBitmap->LineSize(), 1, fontFile);
						delete charBitmap;
						delete tmpBitmap;
					}
					else
					{
						chdr.width = tmpBitmap->Width();
						fwrite(&chdr, sizeof(GLCD::tCharHeader), 1, fontFile);
						fwrite(tmpBitmap->Data(), fhdr.height * tmpBitmap->LineSize(), 1, fontFile);
						delete tmpBitmap;
					}
					startOffset = endOffset;
					fhdr.count++;

					// get next character
					token = strtok(NULL, " ");
				}
			}
			l++;
		}
	}

	fseek(fontFile, 0, SEEK_SET);
	fwrite(&fhdr, sizeof(GLCD::tFontHeader), 1, fontFile);

	fclose(fontFile);
	fclose(descFile);

	delete bitmap;

	fprintf(stdout,"Font '%s' created successfully\n", fontName);

	return 0;
}

void usage(void)
{
	fprintf(stdout, "\n");
	fprintf(stdout, "%s v%s\n", prgname, version);
	fprintf(stdout, "%s is a tool to create *.fnt files that are used by the\n", prgname);
	fprintf(stdout, "        graphlcd plugin for VDR.\n\n");
	fprintf(stdout, "  Usage: %s -f <format> -b bmpfile -d descfile -o outfile\n\n", prgname);
	fprintf(stdout, "  -f  --format      specifies the format of the bitmap. Possible values are:\n");
	fprintf(stdout, "                    PBM : file is an binary PBM file\n" );
	fprintf(stdout, "  -b  --bmpfile     specifies the name of the bitmap file (*.pbm)\n");
	fprintf(stdout, "  -d  --descfile    specifies the name of the description file (*.desc)\n");
	fprintf(stdout, "  -o  --outfile     specifies the name of the output file (*.fnt)\n");
	fprintf(stdout, "\n" );
	fprintf(stdout, "  example: %s -f PBM -b f12.pbm -d f12.desc -o f12.fnt\n", prgname);
}
