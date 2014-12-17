/*
 * GraphLCD graphics library
 *
 * glcd.c  -  GLCD file loading and saving
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

#include <string.h>

#include "bitmap.h"
#include "glcd.h"
#include "image.h"


namespace GLCD
{

using namespace std;

const char * kGLCDFileSign = "GLC";

#pragma pack(1)
struct tGLCDHeader
{
	char sign[3];          // = "GLC"
	char format;           // D - single image, A - animation
	unsigned short width;  // width in pixels
	unsigned short height; // height in pixels
	// only for animations
	unsigned short count;  // number of pictures
	unsigned long delay;   // delay in ms
};
#pragma pack()

cGLCDFile::cGLCDFile()
{
}

cGLCDFile::~cGLCDFile()
{
}

bool cGLCDFile::Load(cImage & image, const string & fileName)
{
	bool ret = false;
	FILE * fp;
	long fileSize;

	fp = fopen(fileName.c_str(), "rb");
	if (fp)
	{
		// get len of file
		if (fseek(fp, 0, SEEK_END) != 0)
		{
			fclose(fp);
			return false;
		}
		fileSize = ftell(fp);

		// rewind and get Header
		if (fseek(fp, 0, SEEK_SET) != 0)
		{
			fclose(fp);
			return false;
		}

		tGLCDHeader hdr;
		const unsigned int singleHeaderSize = sizeof(hdr.sign) + sizeof(hdr.format) + sizeof(hdr.width) + sizeof(hdr.height);

		// Read standard header
		if (fread(&hdr, singleHeaderSize, 1, fp) != 1)
		{
			fclose(fp);
			return false;
		}

		// check Header
		if (strncmp(hdr.sign, kGLCDFileSign, sizeof(hdr.sign)) != 0 || hdr.width == 0 || hdr.height == 0)
		{
			syslog(LOG_ERR, "glcdgraphics: load %s failed, wrong header (cGLCDFile::Load).", fileName.c_str());
			fclose(fp);
			return false;
		}

		if (hdr.format == 'D')
		{
			hdr.count = 1;
			hdr.delay = 10;
			// check file length
			if (fileSize != (long) (hdr.height * ((hdr.width + 7) / 8) + singleHeaderSize))
			{
				syslog(LOG_ERR, "glcdgraphics: load %s failed, wrong size (cGLCDFile::Load).", fileName.c_str());
				fclose(fp);
				return false;
			}
		}
		else if (hdr.format == 'A')
		{
			// rewind and get Header
			if (fseek(fp, 0, SEEK_SET) != 0)
			{
				fclose(fp);
				return false;
			}
			if (fread(&hdr, sizeof(hdr), 1, fp) != 1)
			{
				syslog(LOG_ERR, "glcdgraphics: load %s failed, wrong header (cGLCDFile::Load).", fileName.c_str());
				fclose(fp);
				return false;
			}
			// check file length
			if (hdr.count == 0 ||
			    fileSize != (long) (hdr.count * (hdr.height * ((hdr.width + 7) / 8)) + sizeof(hdr)))
			{
				syslog(LOG_ERR, "glcdgraphics: load %s failed, wrong size (cGLCDFile::Load).", fileName.c_str());
				fclose(fp);
				return false;
			}
			// Set minimal limit for next image 
			if (hdr.delay < 10)
				hdr.delay = 10;
		}

		image.Clear();
		image.width = hdr.width;
		image.height = hdr.height;
		image.delay = hdr.delay;
		unsigned char * bmpdata = new unsigned char[hdr.height * ((hdr.width + 7) / 8)];
		if (bmpdata)
		{
			for (unsigned int n = 0; n < hdr.count; n++)
			{
				if (fread(bmpdata, hdr.height * ((hdr.width + 7) / 8), 1, fp) != 1)
				{
					delete[] bmpdata;
					fclose(fp);
					image.Clear();
					return false;
				}
				image.bitmaps.push_back(new cBitmap(hdr.width, hdr.height, bmpdata));
				ret = true;
			}
			delete[] bmpdata;
		}
		else
		{
			syslog(LOG_ERR, "glcdgraphics: malloc failed (cGLCDFile::Load).");
		}
		fclose(fp);
	}
	if (ret)
		syslog(LOG_DEBUG, "glcdgraphics: image %s loaded.", fileName.c_str());
	return ret;
}

bool cGLCDFile::Save(cImage & image, const string & fileName)
{
	bool ret = false;

	return ret;
}

} // end of namespace
