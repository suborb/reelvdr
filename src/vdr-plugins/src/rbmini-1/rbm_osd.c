/**************************************************************************
*   Copyright (C) 2008 by Reel Multimedia                                 *
*                                                                         *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include "rbmini.h"

#define DBG_FN   DBG_STDOUT

#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "rbm_osd.h"

static bool cRBMOsd_FirstRun=true;

cRBMOsd::cRBMOsd(int Left, int Top, uint Level, int fb_fd):cOsd(Left, Top, Level), scaleToVideoPlane(0), osd_fd(fb_fd) {
		DBG(DBG_FN,"cRBMOsd::cRBMOsd");
		Init();
		if(cRBMOsd_FirstRun) {
			Clear();
			cRBMOsd_FirstRun = false;
		} // if
		DBG(DBG_FN,"cRBMOsd::cRBMOsd DONE");
	} // cRBMOsd

	void cRBMOsd::SetActive(bool On) { 
//		DBG(DBG_FN,"cRBMOsd::SetActive(%b)", On);
		if( On != Active()) {
			cOsd::SetActive(On);
			Clear();
			if(On) Flush();
		} // if
	} // SetActive

	bool cRBMOsd::Init() {
		struct fb_var_screeninfo screeninfo;
		if(ioctl(osd_fd, FBIOGET_VSCREENINFO, &screeninfo)) {
			printf("[RBM-FB] Can't get VSCREENINFO\n");
			goto error;
		} // if
        printf("bits_per_pixel %d xres %d yres %d\n", screeninfo.bits_per_pixel, screeninfo.xres, screeninfo.yres);
		bpp    = screeninfo.bits_per_pixel/8; 
		width  = screeninfo.xres;
		height = screeninfo.yres;
		data = (unsigned char*) mmap(0, bpp * width * height, PROT_READ|PROT_WRITE, MAP_SHARED, osd_fd, 0);
		if(data == (unsigned char*)-1 || !data) {
			printf("[RBM-FB] Can't mmap\n");
			goto error;
		} // if
		return true;
	error:
		Done();
		return false;
	} // Init

	void cRBMOsd::Done() {
	} // Done

	void cRBMOsd::Clear() {
		if(!Init()) return;
		memset(data, 0x00, width*height*bpp);
	} // Clear

	void cRBMOsd::Draw8RGBLine(int x, int y, int w, const tIndex *srcData, const tColor *palette, int num_colors) {
//		DBG(DBG_FN,"cRBMOsd::Draw8RGBLine x %d y %d w %d", x, y, w);
		unsigned char *bm = (unsigned char *)srcData;
		int n = 0;
		if (w>width || x>width || y>height)
			return;
		unsigned int *px=(unsigned int*)(data +width*y*bpp+x*bpp);
#if OSD_EXTRA_CHECK
		if(WITHIN_OSD_BORDERS(px+w-1))
#endif
		for(n=0;n<w;n++)
			//TB *px++=palette[(*bm++)%(num_colors/*TB:?? -1*/)];
			*px++=palette[(*bm++)%num_colors];
	} // Draw8RGB

	void cRBMOsd::FlushBitmap(cBitmap &bitmap, bool full) {
//		DBG(DBG_FN,"cRBMOsd::FlushBitmap full %d", full);
		int x1 = 0;
		int y1 = 0;
		int x2 = bitmap.Width() - 1;
		int y2 = bitmap.Height() - 1;
		if (full || bitmap.Dirty(x1, y1, x2, y2)) {
			if (x1 < 0) x1 = 0;
			if (y1 < 0) y1 = 0;
			if (x2 < 0) x2 = 0;
			if (y2 < 0) y2 = 0;
			int x = Left() + bitmap.X0() + x1;
			int y = Top()  + bitmap.Y0() + y1;
			if (x >= width || y >= height) {
				bitmap.Clean();
				return;
			} // if
			int w = x2 - x1 + 1;
			int h = y2 - y1 + 1;
			if (x + w > width)
				w = width - x;
			if (y + h > height)
				h = height - y;
			int numColors;
			const tColor *palette = bitmap.Colors(numColors);
			for (int yy = y1; yy < y1+h; yy++)
				Draw8RGBLine(x, y++, w, bitmap.Data(x1,yy), palette, numColors);
			bitmap.Clean();
		} // if
	} // FlushBitmap

	///< Shuts down the OSD.
	cRBMOsd::~cRBMOsd(){
		DBG(DBG_FN,"cRBMOsd::~cRBMOsd");
		SetActive(false);
		Done();
	} // ~cRBMOsd

	cBitmap *cRBMOsd::GetBitmap(int Area){
//		DBG(DBG_FN,"cRBMOsd::GetBitmap");
		return cOsd::GetBitmap(Area);
	} // GetBitmap

	eOsdError cRBMOsd::CanHandleAreas(const tArea *Areas, int NumAreas){
//		DBG(DBG_FN,"cRBMOsd::CanHandleAreas");
		return cOsd::CanHandleAreas(Areas, NumAreas);
	} // CanHandleAreas

	eOsdError cRBMOsd::SetAreas(const tArea *Areas, int NumAreas){
//		DBG(DBG_FN,"cRBMOsd::SetAreas");
		if (Active())
			Clear();
		return cOsd::SetAreas(Areas, NumAreas);
	} // SetAreas

	void cRBMOsd::SaveRegion(int x1, int y1, int x2, int y2){ 
//		DBG(DBG_FN,"cRBMOsd::SaveRegion");
		cOsd::SaveRegion(x1, y1, x2, y2);
	} // SaveRegion

	void cRBMOsd::RestoreRegion(void){ 
//		DBG(DBG_FN,"cRBMOsd::RestoreRegion");
		cOsd::RestoreRegion();
	} // RestoreRegion

	eOsdError cRBMOsd::SetPalette(const cPalette &Palette, int Area){
//		DBG(DBG_FN,"cRBMOsd::SetPalette");
		return cOsd::SetPalette(Palette, Area);
	} // SetPalette

	void cRBMOsd::DrawPixel(int x, int y, tColor Color){ 
//		DBG(DBG_FN,"cRBMOsd::DrawPixel");
		cOsd::DrawPixel(x, y, Color);
	} // DrawPixel

	void cRBMOsd::DrawBitmap(int x, int y, const cBitmap &Bitmap, tColor ColorFg, tColor ColorBg, bool ReplacePalette, bool Overlay){ 
//		DBG(DBG_FN,"cRBMOsd::DrawBitmap");
		cOsd::DrawBitmap(x, y, Bitmap, ColorFg, ColorBg, ReplacePalette, Overlay);
	} // DrawBitmap

	void cRBMOsd::DrawBitmapHor(int x, int y, int w, const cBitmap &Bitmap){ 
//		DBG(DBG_FN,"cRBMOsd::DrawBitmapHor");
//		cOsd::DrawBitmapHor(x, y, w, Bitmap);
	} // DrawBitmapHor

	void cRBMOsd::DrawBitmapVert(int x, int y, int h, const cBitmap &Bitmap){ 
//		DBG(DBG_FN,"cRBMOsd::DrawBitmapVert");
//		cOsd::DrawBitmapVert(x, y, h, Bitmap);
	} // DrawBitmapVert

	void cRBMOsd::DrawText(int x, int y, const char *s, tColor ColorFg, tColor ColorBg, const cFont *Font, int Width, int Height, int Alignment){ 
//		DBG(DBG_FN,"cRBMOsd::DrawText");
		cOsd::DrawText(x, y, s, ColorFg, ColorBg, Font, Width, Height, Alignment);
	} // DrawText

	void cRBMOsd::DrawImage(u_int imageId, int x, int y, bool blend, int horRepeat, int vertRepeat){ 
//		DBG(DBG_FN,"cRBMOsd::DrawImage");
		cOsd::DrawImage(imageId, x, y, blend, horRepeat, vertRepeat);
	} // DrawImage

	void cRBMOsd::DrawRectangle(int x1, int y1, int x2, int y2, tColor Color){ 
//		DBG(DBG_FN,"cRBMOsd::DrawRectangle1");
		cOsd::DrawRectangle(x1, y1, x2, y2, Color);
	} // DrawRectangle

	void cRBMOsd::DrawRectangle(int x1, int y1, int x2, int y2, tColor Color, int alphaGradH, int alphaGradV, int alphaGradStepH, int alphaGradStepV){
//		DBG(DBG_FN,"cRBMOsd::DrawRectangle2");
//		cOsd::DrawRectangle(x1, y1, x2, y2, Color, alphaGradH, alphaGradV, alphaGradStepH, alphaGradStepV);
	} // DrawRectangle

	void cRBMOsd::DrawEllipse(int x1, int y1, int x2, int y2, tColor Color, int Quadrants){ 
//		DBG(DBG_FN,"cRBMOsd::DrawEllipse");
		cOsd::DrawEllipse(x1, y1, x2, y2, Color, Quadrants);
	} // DrawEllipse

	void cRBMOsd::DrawSlope(int x1, int y1, int x2, int y2, tColor Color, int Type){ 
//		DBG(DBG_FN,"cRBMOsd::DrawSlope");
		cOsd::DrawSlope(x1, y1, x2, y2, Color, Type);
	} // DrawSlope

	void cRBMOsd::Flush(void){
//		DBG(DBG_FN,"cRBMOsd::Flush");
		for (int area = 0; true; ++area) {
			cBitmap *pBitmap = GetBitmap(area);
			if (pBitmap == NULL)
				break;
			FlushBitmap(*pBitmap, false);
		} // for
	} // Flush

	void cRBMOsd::SetImagePath(u_int imageId, char const *path){ cOsd::SetImagePath(imageId, path);}

