/*
*  C Implementation: pngutil.c
*
* Description: 
*
*
* Author: RealQuick <realquick@doublequick.de>, (C) 2009
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include <curl/curl.h>

#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <vdr/osdbase.h>
#include <vdr/config.h>
#include "pngutils.h"
#include <string.h>
#include <string>

#include <png.h>
#include <jpeglib.h>

#define HDOSD_MAX_IMAGES 128


#define OSDHEIGHT                 576                     // in pixels
#define OSDWIDTH                  720                     // in pixels

#define DEFAULT_RAW_WIDTH 160
#define DEFAULT_RAW_HEIGHT 120

#define IMAGE_COUNT 2

#ifndef RBLITE 
#define START_IMAGE 119
#else
#define START_IMAGE 124
#endif

#define PNG_MAX_WIDTH_EHD	250	// max width for DrawImage send to OSD (EHD)
#define PNG_MAX_HEIGHT_EHD	110	// max height for DrawImage send to OSD (EHD)

#define PNG_MAX_WIDTH_BSP	25	// max width for DrawImage send to OSD (BSP)
#define PNG_MAX_HEIGHT_BSP	25	// max height for DrawImage send to OSD (BSP


cPngUtils::cPngUtils()
{
  bmp = NULL;
}


cPngUtils::cPngUtils(int w, int h)
{
    nMaxPNGWidth = width = w;
    nMaxPNGHeight = height = h;
    HasTransparency = true; // Default: images have transparency
    bmp = NULL;
}

cPngUtils::cPngUtils(unsigned char *xbmp, int w, int h)
{
    HasTransparency = true; // Default: images have transparency
    nMaxPNGWidth = width = w;
    nMaxPNGHeight = height = h;

    bmp = xbmp;
}

cPngUtils::~cPngUtils()
{
   if(bmp != NULL)
   {
      delete [] bmp;

      bmp = NULL;
   }
}


/********** download URL from network *********/
int cPngUtils::downloadURL ( const char *URL, const char *filename )
{
    CURL *curl;
    CURLcode res;
    int returnFlag = 1;

    curl = curl_easy_init ();
    if ( curl )
    {
        FILE *fp = fopen ( filename, "w" ); // should overwrite file
        curl_easy_setopt ( curl, CURLOPT_URL, URL );
        curl_easy_setopt ( curl, CURLOPT_USERAGENT,  "libcurl-agent/1.0" );
        curl_easy_setopt ( curl, CURLOPT_WRITEDATA, fp );
        res = curl_easy_perform ( curl );
        if ( res )
        {
            returnFlag = 0;
        }

        fclose ( fp );
    }
    /* always cleanup */
    curl_easy_cleanup ( curl );

    return returnFlag;
}

int cPngUtils::ReadJpeg(const char *sPath)
{
  HasTransparency = false; // JEPGs do not have transparency


//  jpeg_destination_mgr jdm;

  unsigned char *linha;
  int ImageSize;

  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  FILE * infile;

  if ((infile = fopen(sPath, "rb")) == NULL) 
  {
        esyslog("can't open %s\n", sPath);
        return(-1);
  }
  jpeg_stdio_src(&cinfo, infile);
  jpeg_read_header(&cinfo, TRUE);

  jpeg_start_decompress(&cinfo);

//TODO Allocate buffer (bmp) for decoded image to read in

// jpeg_read_scanlines(...); 

    // delete bitmap if resolution has changed
    if(bmp != NULL && !((unsigned int)width == cinfo.image_width && (unsigned int)height == cinfo.image_height))
    {
       delete [] bmp;
       bmp = NULL;
    }

    width = cinfo.image_width;
    height = cinfo.image_height;
    rowsize = cinfo.image_width * cinfo.out_color_components;

    ImageSize = cinfo.image_height * rowsize;

    if(bmp == NULL)
    {
        bmp = new unsigned char[ImageSize];
    }

    linha=bmp;
    for(unsigned int i=0;i < cinfo.output_height; i++)
    { 
       linha = bmp + (i * rowsize);
       jpeg_read_scanlines(&cinfo,&linha,1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return 0;
}

int cPngUtils::WriteFileOsd(cOsd *Osd, unsigned char nTransparenz, int x, int y, int show_lines, int *nImageIdPos, bool bHasTransparency)
{
#ifdef RBLITE 
    nMaxPNGWidth = PNG_MAX_WIDTH_BSP;
    nMaxPNGHeight = PNG_MAX_HEIGHT_BSP;
#else
    nMaxPNGWidth = PNG_MAX_WIDTH_EHD;
    nMaxPNGHeight = PNG_MAX_HEIGHT_EHD;
#endif

    int xpos=0;
    int ypos=0;

    char sPath[512];

    if(bmp == NULL)
       return -1;

    if(show_lines == 0)
	show_lines = height;

    while(ypos < min(height, show_lines))
    {
        while(xpos < width)
        {
	    char* buf;
	    buf = tempnam("/tmp", "repg_");
            sprintf(sPath, "%s.png", buf /* (*nImageIdPos)%IMAGE_COUNT */);
// dsyslog("pngutils: Transparency=%d, x=%d, y=%d, Path=%s, Transparency=%s, w=%d, h=%d, rowsize=%d", nTransparenz,xpos,ypos,sPath,bHasTransparency?"TRUE":"FALSE",min(nMaxPNGWidth, width-xpos), min(nMaxPNGHeight , height-ypos), rowsize);
            int rc = WriteFile(nTransparenz, xpos, ypos, sPath, bHasTransparency, min(nMaxPNGWidth, width-xpos), min(nMaxPNGHeight , height-ypos), rowsize);

            free(buf);

	    if(rc != 0)
                return rc;

            Osd->SetImagePath(/* HDOSD_MAX_IMAGES-11 */ START_IMAGE  + (*nImageIdPos)%IMAGE_COUNT, sPath);
            Osd->DrawImage(/* HDOSD_MAX_IMAGES-11 */ START_IMAGE + (*nImageIdPos)%IMAGE_COUNT, x+xpos, y+ypos, false);

            (*nImageIdPos)++;
            xpos += nMaxPNGWidth;
        }
        ypos += nMaxPNGHeight ;
        xpos = 0;
    }

    return 0;
}

int cPngUtils::WriteFile ( unsigned char nTransparenz, int xpos, int ypos, const char *sPath, bool bHasTransparency, int rel_width, int rel_height, int rel_rowsize )
{
    png_structp  png_ptr;       /* note:  temporary variabloutfile = fopen(sPath, "wb");es! */
    png_infop  info_ptr;
    int color_type, interlace_type;

    int offset;

    int x, y;

    int nWidth, nHeight, nRowsize;

    // use relative width instead of real image with?
    if (rel_width != 0)
        nWidth = rel_width;
    else
        nWidth = width;

    // use relative height instead of real image height?
    if (rel_height != 0)
        nHeight = rel_height;
    else
        nHeight = height;

    // use relative rowsize instead of real image rowsize?
    if (rel_rowsize != 0)
        nRowsize = rel_rowsize;
    else
        nRowsize = rowsize;


    FILE *outfile;

    png_bytep *row_pointers;
    png_bytep *row_in_pointers;

    if ( bmp == NULL )
        return -1;

    /* could also replace libpng warning-handler (final NULL), but no need: */

    png_ptr = png_create_write_struct ( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
    if ( !png_ptr )
        return 4;   /* out of memory */

    info_ptr = png_create_info_struct ( png_ptr );
    if ( !info_ptr )
    {
        png_destroy_write_struct ( &png_ptr, NULL );
        return 4;   /* out of memory */
    }


    outfile = fopen ( sPath, "wb" );
    if (outfile == NULL)
    {
        png_destroy_info_struct ( png_ptr, &info_ptr );
        png_destroy_write_struct ( &png_ptr, NULL );
        return 10;
    }

    if ( setjmp ( png_jmpbuf ( png_ptr ) ) )
    {
        png_destroy_info_struct ( png_ptr, &info_ptr );
        png_destroy_write_struct ( &png_ptr, NULL );
        fclose ( outfile );
        return 5;
    }
    png_init_io ( png_ptr, outfile );

    /* set the compression levels--in general, always want to leave filtering
     * turned on (except for palette images) and allow all of the filters,
     * which is the default; want 32K zlib window, unless entire image buffer
     * is 16K or smaller (unknown here)--also the default; usually want max
     * compression (NOT the default); and remaining compression flags should
     * be left alone */

    if ( setjmp ( png_jmpbuf ( png_ptr ) ) )
    {
        png_destroy_info_struct ( png_ptr, &info_ptr );
        png_destroy_write_struct ( &png_ptr, NULL );
        fclose ( outfile );
        return 6;
    }
//    png_set_compression_level ( png_ptr, Z_BEST_COMPRESSION );

    // performace issues MORE speed less compression
    png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, PNG_FILTER_SUB);
    png_set_compression_level(png_ptr, Z_BEST_SPEED);
    png_set_compression_strategy(png_ptr, Z_HUFFMAN_ONLY);

    color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    interlace_type = PNG_INTERLACE_NONE;

    if ( setjmp ( png_jmpbuf ( png_ptr ) ) )
    {
        png_destroy_info_struct ( png_ptr, &info_ptr );
        png_destroy_write_struct ( &png_ptr, NULL );
        fclose ( outfile );
        return 7;
    }

    png_set_IHDR ( png_ptr, info_ptr, nWidth, nHeight,
                   /* sample_depth */ 8, color_type, interlace_type,
                   PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT );

//                case PNG_COLOR_TYPE_RGB_ALPHA: puts("color_type: PNG_COLOR_TYPE_RGB_ALPHA"); break;
    png_write_info ( png_ptr, info_ptr );

    row_pointers = ( png_bytep* ) malloc ( sizeof ( png_bytep ) * nHeight );
    for ( y=0; y<nHeight; y++ )
        row_pointers[y] = ( png_byte* ) malloc ( info_ptr->rowbytes );

    png_byte* row;
    //png_byte* ptr;

    png_byte* row_in;
    //png_byte* ptr_in;


    if ( bHasTransparency )
    {
// esyslog("quickbrowser: offset=%d W=%d,h=%d,rowsize=%d,  xpos=%d,ypos=%d", offset, w,h,rowsize,xpos,ypos);

        offset = nMaxPNGWidth * ( xpos / nMaxPNGWidth ) * 4;
        row_in_pointers = ( png_bytep * ) bmp;
        for ( y=0; y<nHeight; y++ )
        {
            row = row_pointers[y];
            row_in = row_in_pointers[y+ypos];
            for ( x=0; x<nWidth; x++ )
            {
/*
                ptr = & ( row[x*4] );
                ptr_in = & ( row_in[ ( x+xpos ) *4] );

                ptr[0] = ptr_in[0];
                ptr[1] = ptr_in[1];
                ptr[2] = ptr_in[2];
                ptr[3] = ptr_in[3];
*/

	   long* ptr = (long *)&(row[x*sizeof(long)]);
//           *ptr = (250L << 24)
           *ptr = (((long)nTransparenz) << 24)
                 | (*(bmp + (y+ypos) * rowsize + 4*(x+xpos) + 0) << 16)
                 | (*(bmp + (y+ypos) * rowsize + 4*(x+xpos) + 1) << 8)
                 | (*(bmp + (y+ypos) * rowsize + 4*(x+xpos) + 2));
            }
        }
    }
    else
    {
        // Copy and convert pixel  max nMaxPNGWidth * nMaxPNGHeight
        offset = nMaxPNGWidth * ( xpos / nMaxPNGWidth ) * 3;
        for ( y=0; y<nHeight; y++ )
        {
            row = row_pointers[y];
            for ( x=0; x<nWidth; x++ )
            {
                png_byte* ptr = & ( row[x*4] );

                ptr[0] = * ( bmp + ( y + ypos ) * nRowsize + 3*x + offset );
                ptr[1] = * ( bmp + ( y + ypos ) * nRowsize + 3*x + offset + 1 );
                ptr[2] = * ( bmp + ( y + ypos ) * nRowsize + 3*x + offset + 2 );
                ptr[3] = nTransparenz;
            }
        }
    }


    if ( setjmp ( png_jmpbuf ( png_ptr ) ) )
    {
        png_destroy_info_struct ( png_ptr, &info_ptr );
        png_destroy_write_struct ( &png_ptr, NULL );
        fclose ( outfile );
        return 8;
    }

    png_write_image ( png_ptr, row_pointers );

    /* since that's it, we also close out the end of the PNG file now--if we
     * had any text or time info to write after the IDATs, second argument
     * would be info_ptr, but we optimize slightly by sending NULL pointer: */

    if ( setjmp ( png_jmpbuf ( png_ptr ) ) )
    {
        png_destroy_info_struct ( png_ptr, &info_ptr );
        png_destroy_write_struct ( &png_ptr, NULL );
        fclose ( outfile );
        return 9;
    }
    png_write_end ( png_ptr, NULL );

    fclose ( outfile );

    /* cleanup heap allocation */
    for ( y=0; y<nHeight; y++ )
        free ( row_pointers[y] );
    free ( row_pointers );

    png_destroy_info_struct ( png_ptr, &info_ptr );
    png_destroy_write_struct ( &png_ptr, NULL );

    return 0;
}


bool cPngUtils::PngIsReadable(const char *path)
{
    Byte header[8];

    FILE *fp = fopen(path, "rb");
    if (!fp)
        return false;
    if(fread(header, 1, 8, fp) != 8)
    {
        fclose(fp);
        return false;
    }
    fclose(fp);
    return true;
}

bool cPngUtils::ReadPng(const char *path)
{
    png_structp png_ptr;
    png_infop   info_ptr;

    unsigned char **rows;

        unsigned char header[8];

        FILE *fp = fopen(path, "rb");
        if (!fp || fread(header, 1, 8, fp) != 8)
        {
            return false;
        }

        if (png_sig_cmp(header, 0, 8))
        {
            fclose(fp);
            return false;
        }

        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                                     NULL, NULL, NULL);
        if (!png_ptr)
        {
            fclose(fp);
            return false;
        }
    
        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
        {
            png_destroy_read_struct(&png_ptr,
                                    NULL, NULL);
            fclose(fp);
            return false;
        }
    
        png_infop end_info = png_create_info_struct(png_ptr);
        if (!end_info)
        {
            png_destroy_read_struct(&png_ptr, &info_ptr,
                                    (png_infopp)NULL);
            fclose(fp);
            return false;
        }

        png_init_io(png_ptr, fp);
        png_set_sig_bytes(png_ptr, 8);
        if (setjmp(png_jmpbuf(png_ptr)))
        {
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            fclose(fp);
            return false;
        }

	png_read_info(png_ptr, info_ptr);

	png_byte color_type = info_ptr->color_type;
	png_byte bit_depth = info_ptr->bit_depth;

        if (bit_depth == 16)
            png_set_strip_16(png_ptr);
        if (color_type == PNG_COLOR_TYPE_GRAY ||
            color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
            png_set_gray_to_rgb(png_ptr);


        if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY)
//          png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
            HasTransparency = false;
	else
            HasTransparency = true;

	png_read_update_info(png_ptr, info_ptr);
//	png_byte w = info_ptr->width;
	png_byte h = info_ptr->height;

	rows = (png_bytep*) malloc(sizeof(png_bytep) * h);
	int y;
	for (y=0; y<h; y++) {
		rows[y] = (png_byte*) malloc(info_ptr->rowbytes);
        }
	rowsize = info_ptr->rowbytes;

	png_read_image(png_ptr, rows);
        fclose(fp);

	// delete bitmap if resolution has changed
        if(bmp != NULL && !((unsigned int)width == png_get_image_width(png_ptr, info_ptr) && (unsigned int)height == png_get_image_height(png_ptr, info_ptr)))
        {
            delete [] bmp;
            bmp = NULL;
        }
        width = png_get_image_width(png_ptr, info_ptr);
        height = png_get_image_height(png_ptr, info_ptr);

	if(bmp == NULL)
	{
            bmp = new unsigned char [(HasTransparency ? 4 : 3) * height * width];
	}

        png_byte* row;
        png_byte* ptr;
// dsyslog("png-Picture %dx%d, rowsize=%d Transparency=%s", height, width, rowsize, HasTransparency ? "Yes" : "False");
	//int outbytes=rowsize / width;

        for (int y=0; y<height; y++ )
        {
            row = rows[y];
	    ptr = & ( bmp[y*width * (HasTransparency ? 4 : 3)] );
            for (int x=0; x<width; x++ )
            {
		if(HasTransparency)
		{
                ptr[0] = row[2];
                ptr[1] = row[1];
                ptr[2] = row[0];
                ptr[3] = row[3];
		ptr+=4;
		row+=4;
		} else
		{
                ptr[0] = row[0];
                ptr[1] = row[1];
                ptr[2] = row[2];
		ptr+=3;
		row+=3;
		}
            }
	    free(rows[y]);
        }
	free(rows);

//	png_read_end(png_ptr, NULL);
//	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

/*
        for (int y = 0; y < height; ++y)
        {
            memcpy(bmp + y * info_ptr->rowbytes,rows[y], info_ptr->rowbytes); 
	    free(rows[y]);
        }
	free(rows);
*/

  return true;
}

void cPngUtils::DrawBitmap(cOsd *Osd, int xpos, int ypos, int step, int maxheight)
{
   long nPixel;

   int maxh;
   if(maxheight == 0)
      maxh=height;
   else
      maxh=maxheight;

   for(int y=0;y<maxh;y+=step)
       for(int x=0;x<width;x+=step)
       {
           nPixel = (250L << 24)
                 | (*(bmp + y * width * 4 + 4*x + 2) << 16)
                 | (*(bmp + y * width * 4 + 4*x + 1) << 8)
                 | (*(bmp + y * width * 4 + 4*x + 0));

            Osd->DrawRectangle(xpos+x/step, ypos+y/step, xpos+x/step, ypos+y/step, (tColor)nPixel );
        }
}

bool cPngUtils::ReadRawBitmap(const char *path)
{
   long nPixel;
   imgheader h;

  int old_width = width;
  int old_height = height;

  FILE * infile;
  if ((infile = fopen(path, "rb")) == NULL) 
  {
        esyslog("pngutils: can't open %s for reading", path);
        return(false);
  }

  if(fread(&h, sizeof(h), 1, infile) != 1)
  {
        fclose(infile);
        esyslog("pngutils: can't open %s for reading", path);
	return false;
  }
  if(memcmp(h.tag, FILEHEADER_TAG, sizeof(h.tag)) == 0 && h.xres>0 && h.yres>0)
  {
      width = h.xres;
      height = h.yres;
  } else
  {
      // old style rawfile without header -> step back and read from begin
      width = DEFAULT_RAW_WIDTH;
      height = DEFAULT_RAW_HEIGHT;

      fseek(infile, 0L, SEEK_SET);
  }

  if(!(width == old_width && height == old_height) && bmp != NULL)
  {
      delete [] bmp;
      bmp = NULL;
  }

   if (bmp == NULL)
   {
       bmp = new unsigned char[height * width * sizeof(long)];
   }
   long *ptr = (long *)bmp;

   HasTransparency = true;
   rowsize = width * sizeof(long);	// Always 4 bytes per pixel

   for(int y=0;y<height;y++)
   {
       for(int x=0;x<width;x++)
       {
           if(fread(&nPixel,sizeof(long), 1, infile) < 0)
           {
                fclose(infile);
		return(false);
           }
            *ptr = nPixel;
	    ptr++;
        }
   }
   fclose(infile);

   return(true);
}

#if 0
bool cPngUtils::DrawRawBitmap(cOsd *Osd, char *path, int xpos, int ypos, int maxheight)
{
   long nPixel;
// dsyslog("pngutil: DrawRawBitmap %s Pos %d-%d %dx%d", path, xpos, ypos, width, height);

  FILE * infile;

  if ((infile = fopen(path, "rb")) == NULL) 
  {
//        esyslog("can't open %s\n for reading", path);
        return(false);
  }

   if (bmp == NULL)
   {
       bmp = new unsigned char[height * width * 4];
   }
   long *ptr = (long *)bmp;

   for(int y=0;y<120;y++)
   {
       for(int x=0;x<160;x++)
       {
           if(fread(&nPixel,sizeof(long), 1, infile) < 0)
           {
                fclose(infile);
		return(false);
           }
 
	    if(y < maxheight)
            	Osd->DrawRectangle(xpos+x, ypos+y, xpos+x, ypos+y, (tColor)nPixel);

            *ptr = nPixel;
	    ptr++;

        }
        if(y %10 == 0 && y < maxheight)
        {
            Osd->Flush();
        }
   }
   fclose(infile);

   return(true);
}
#endif

int cPngUtils::DumpRaw ( const char *sPath, int step )
{
    FILE *outfile;
    long nPixel;

    if ( ( outfile = fopen ( sPath, "wb" ) ) == NULL )
    {
        esyslog ( "can't open %s\n", sPath );
        return ( -1 );
    }

    if ( bmp == NULL )
        return -2;

    // Write header
    imgheader h;
    memcpy(h.tag, FILEHEADER_TAG, sizeof(h.tag));
    h.xres = width;
    h.yres = height;
    fwrite ( (void *)&h, sizeof ( h ), 1, outfile );

    for ( int y=0;y<height;y+=step )
        for ( int x=0;x<width;x+=step )
        {
            png_byte* ptr = ( png_byte* ) &nPixel;

            ptr[2] = * ( bmp + ( y ) * rowsize + 3*x );
            ptr[1] = * ( bmp + ( y ) * rowsize + 3*x + 1 );
            ptr[0] = * ( bmp + ( y ) * rowsize + 3*x + 2 );
            ptr[3] = 250 /* nTransparenz */;
            fwrite ( &nPixel, sizeof ( nPixel ), 1, outfile );


        }

    fclose ( outfile );
    return 0;
}

int cPngUtils::read(int intx,int inty,int colour)
{
    long *inp;
    unsigned char* out;

    inp = (long *)bmp;
    inp += (inty -1) * width + (intx - 1);

    out = (unsigned char *)inp;

    return (int)*(out + colour);
}

int cPngUtils::bilinear_interpolation_read(double x, double y, int colour)
{

   int inty, intx;
   inty = (int) ceil(y);
   intx = (int) ceil(x);

   bool attop, atright;
   attop = inty==this->height;
   atright =  intx==this->width;

   if( (!attop)&&(!atright) )
     {

	double f,g,f1,g1;
	f = 1.0 + x - ((double) intx);
	g = 1.0 + y - ((double) inty);
	f1 = 1.0 - f;
	g1 = 1.0 - g;

	return (int) (
		      f1*g1*this->read(intx, inty,colour)
		      + f*g1*this->read(intx+1,inty,colour)
		      +f1*g*this->read(intx,inty+1,colour)
		      + f*g*(this->read(intx+1,inty+1,colour))
		      );
     }

   if( (atright)&&(!attop))
     {

	double f,g,f1,g1;
	f = 1.0 + x - ((double) intx);
	g = 1.0 + y - ((double) inty);
	f1 = 1.0 - f;
	g1 = 1.0 - g;

	return (int) (
		      f1*g1*this->read(intx, inty,colour)
		      + f*g1*( 2*(this->read(intx,inty,colour)) - (this->read(intx-1,inty,colour)) )
		      +f1*g*this->read(intx,inty+1,colour)
		      + f*g*(2*(this->read(intx,inty+1,colour)) - (this->read(intx-1,inty+1,colour)))
		      );
     }

   if((attop)&&(!atright))
     {
	double f,g,f1,g1;
	f = 1.0 + x - ((double) intx);
	g = 1.0 + y - ((double) inty);
	f1 = 1.0 - f;
	g1 = 1.0 - g;

	return (int) (
		      f1*g1*this->read(intx, inty,colour)
		      + f*g1*this->read(intx+1,inty,colour)
		      +f1*g*( 2*(this->read(intx,inty,colour))  - this->read(intx,inty-1,colour) )
		      + f*g*( 2*(this->read(intx+1,inty,colour))  - this->read(intx+1,inty-1,colour))
		      );
     }

   double f,g,f1,g1;
   f = 1.0 + x - ((double) intx);
   g = 1.0 + y - ((double) inty);
   f1 = 1.0 - f;
   g1 = 1.0 - g;

   return (int) (
		 f1*g1*this->read(intx, inty,colour)
		 + f*g1*( 2*(this->read(intx,inty,colour)) - (this->read(intx-1,inty,colour)) )
		 +f1*g*( 2*(this->read(intx,inty,colour))  - this->read(intx,inty-1,colour) )
		 + f*g*( 2*( 2*(this->read(intx,inty,colour)) - (this->read(intx-1,inty,colour)) )  - ( 2*(this->read(intx,inty-1,colour)) - (this->read(intx-1,inty-1,colour)) ))
		 );


};

bool cPngUtils::resize_image(int newWidth, int newHeight)
{
   if(width <= 0 || newWidth <= 0 or bmp == NULL)
       return false;

   

   if(!newHeight)
       newHeight = height * newWidth / width;

   double k = (double)newWidth / (double)width;
   if(k <= 0.0)
     {
	 return false;
     }

   // Calculate the new scaled height and width
   int scaledh, scaledw;
   scaledw = (int) ceil(k*width);
   scaledh = (int) ceil(k*height);

   // Create image storage.
   unsigned char *new_bmp = new unsigned char[scaledw * scaledh * 4];

   int red, green, blue;

   double spacingx = ((double)width)/(2*scaledw);
   double spacingy = ((double)height)/(2*scaledh);

   double readx, ready;

   for(int x = 1; x<= scaledw; x++)
     {
	for(int y = 1; y <= scaledh; y++)
	  {
	     readx = (2*x-1)*spacingx;
	     ready = (2*y-1)*spacingy;
	     red = this->bilinear_interpolation_read(readx, ready, 2);
	     green = this->bilinear_interpolation_read(readx, ready, 1);
	     blue = this->bilinear_interpolation_read(readx, ready, 0);

//	     temp.plot(x, y, red, green, blue);
	     unsigned char* out = new_bmp +  4 * ((y - 1) * scaledw + (x-1));

             *out++ = (unsigned char)blue; 
             *out++ = (unsigned char)green; 
             *out++ = (unsigned char)red; 
             *out++ = (unsigned char)250;
 	  }
     }

     delete [] bmp;
     bmp = new_bmp;

     width = scaledw;
     height = scaledh;
     rowsize = width * 4;   // RGBA

     return true;

}

bool cPngUtils::resize_image_nearest_neighbour(int new_rwidth)
{
   if (bmp == NULL || height <= 0 || width <= 0)
   {
       return false;
   }

// ensure even dimensions
   int new_width = new_rwidth - new_rwidth % 2;
   int new_height = height * new_width / width;
   new_height -= new_height % 2; 

   unsigned char *new_bmp = new unsigned char[new_height * new_width * 4];

   long *in_ptr = (long *)bmp;
   long *out_ptr = (long *)new_bmp;

   for(int y=0;y<new_height;y++)
   {
       int ref_y = y * width / new_width;
       for(int x=0;x<new_width;x++)
       {
	  *out_ptr = *(in_ptr + (ref_y * width + (x * width) / new_width));
	  out_ptr++;
       }
   }

   delete [] bmp;
   bmp = new_bmp;

   width = new_width;
   height = new_height;

   rowsize = width * 4;   // RGBA

   return true;
}
