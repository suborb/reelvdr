/**************************************************************************
*   Copyright (C) 2011 by Reel Multimedia                                 *
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

#include <png.h>
#include <vdr/font_ft.h>

#define ICE_GDL_SCALE
//#define OSD_EXTRA_CHECK 1
#define WITHIN_OSD_BORDERS(arg) ( (unsigned int*)arg >= (unsigned int*)data && (unsigned int*)arg <= (unsigned int*)(data + osd_pitch*osd_height))

#ifdef ICE_GDL_SCALE
#include <srb.h>
#include <x86_cache.h>

#endif

typedef struct {
	int x0, y0;
	int x1, y1;
} dirtyArea;

typedef struct CachedImage{
	int width, height;
	unsigned int *data;
} CachedImage_t;

#define MAX_CACHED_IMAGES 256
static bool ImageIdInRange(u_int imageId) {
    if(imageId<MAX_CACHED_IMAGES) return true;
    esyslog("ERROR: ImageId %d not in range (max %d)", imageId, MAX_CACHED_IMAGES);
    return false;
} // ImageIdInRange

static png_structp png_ptr;
static png_infop   info_ptr;
static struct CachedImage *cachedImages_[MAX_CACHED_IMAGES];
static std::string    imagePaths_[MAX_CACHED_IMAGES];
static bool           imageDirty_[MAX_CACHED_IMAGES];
/* buffer for DrawRectangle() */
static uint32_t draw_linebuf[1024];
static uint32_t draw_color;

int lastThumbWidth[2];
int lastThumbHeight[2];

static dirtyArea dirtyArea_; /** to be able remember where the OSD has changed, to only need to flush this area */

/* a buffer for the last result and last arguments of DoAlphaBlend() */
static uint32_t last_p1, last_p2, last_res;

static inline uint32_t DoAlphaBlend(uint32_t p2, uint32_t p1) {
	/* Don't do expensive calculations if the last calculation was
	 * done with the same arguments. Return the last result instead */
	if(p1 == last_p1 && p2 == last_p2)
		return last_res;
	else {
		last_p1 = p1;
		last_p2 = p2;
	}
	uint8_t const xa1 = (p1 >> 24);
	uint8_t const xa2 = (p2 >> 24);
	/* one color completely opaque, one completely transparent.
	 * Return the opaque one */
	if (xa2==255 || !xa1) {
		last_res = p2;
		return p2;
	} else if (!xa2) {
		last_res = p1;
		return p1;
	} else { /* now we really have to calculate */
		uint8_t const a1 = 255 - xa1;
		uint8_t const a2 = 255 - xa2;
		uint8_t const r2 = (p2 >> 16);
		uint8_t const g2 = (p2 >> 8);
		uint8_t const b2 = p2;
		uint8_t const r1 = (p1 >> 16);
		uint8_t const g1 = (p1 >> 8);
		uint8_t const b1 = p1;
		uint32_t const al1_al2 = a1 * a2;
		uint32_t const al2     = a2 * 255;
		uint32_t const one     = 255 * 255;
		uint32_t const al1_al2_inv = one - al1_al2; // > 0
		uint32_t const c1 = al2 - al1_al2;
		uint32_t const c2 = one - al2;
		uint8_t const a = al1_al2 / 255;
		uint8_t const r = (c1 * r1 + c2 * r2) / al1_al2_inv;
		uint8_t const g = (c1 * g1 + c2 * g2) / al1_al2_inv;
		uint8_t const b = (c1 * b1 + c2 * b2) / al1_al2_inv;
		uint32_t res = last_res = ((255 - a) << 24) | (r << 16) | (g << 8) | b;
		return res;
	} // if
} // DoAlphaBlend

#ifdef ICE_GDL_SCALE
static srb_context_t srb_cnt = NULL;
static srb_context_t src_hdl = NULL;
static gdl_surface_info_t src_info = {id:GDL_SURFACE_INVALID};
#endif
static unsigned char * data = NULL;   /* double buffer */
static unsigned char *mySavedRegion = NULL;
static int savedRegion_x0, savedRegion_y0, savedRegion_x1, savedRegion_y1;
static gdl_int32 osd_width   = 720;
static gdl_int32 osd_height  = 576;
static gdl_uint32 osd_bpp    = 4;
static gdl_uint32 osd_pitch  = osd_bpp*osd_width;
static gdl_surface_info_t surface_info = {id:GDL_SURFACE_INVALID};

class cICEOsdReel : public cOsd {
	friend class cICEOsdProvider;
protected:
	cICEOsdReel(cPluginICE *Plugin, int Left, int Top, uint Level)
			:cOsd(Left, Top, Level) {
		ICE_DBG(DBG_FN,"cICEOsdReel::cICEOsdReel(%d,%d,%d)", Left, Top, Level);
		left = Left;
		top = Top;
	}; // cICEOsdReel
	virtual bool ProvidesTrueColor() {return true;};
	virtual void SetActive(bool On) {
		ICE_DBG(DBG_FN,"cICEOsdReel::SetActive(%d)", On);
		if( On != Active()) {
			cOsd::SetActive(On);
			Clear();
			if(On) Flush();
		} // if
	}; // SetActive
	void LoadImage(u_int imageId) {
		//ICE_DBG(DBG_FN,"cICEOsdReel::LoadImage(%d)", imageId);
		if (!ImageIdInRange(imageId)) return;
		if (cachedImages_[imageId]) {
			if(cachedImages_[imageId]->data && !imageDirty_[imageId])
				return; /* Already cached. */
			if(cachedImages_[imageId]->data) free(cachedImages_[imageId]->data);
				free(cachedImages_[imageId]);
			cachedImages_[imageId]=0;
		} // if
		/* Not cached, load it. */
		std::string const &path_ = imagePaths_[imageId];
		if (path_.empty()) {
			printf("ERROR: Image id %u: No path set.\n", imageId);
			return;
		} // if
		const char *path = path_.c_str();
		unsigned char **rows;
		int  width, height;
		if (OpenPngFile(path, rows, width, height)) {
			void *buffer = malloc(width * height * sizeof(u_int));
			CachedImage *img = (struct CachedImage*)malloc(sizeof(struct CachedImage));
			img->data = (unsigned int*)buffer;
			img->height = height;
			img->width = width;
			cachedImages_[imageId] = img;
			unsigned char *p = static_cast<unsigned char *>(buffer);
			for (int y = 0; y < height; ++y) {
				unsigned char *r = rows[y];
				for (int x = 0; x < width; ++x) {
					p[0] = r[2];
					p[1] = r[1];
					p[2] = r[0];
					p[3] = r[3];
					r += 4;
					p += 4;
				}
				free(rows[y]);
			} // for
			free(rows);
			ClosePngFile();
			//free(buffer);
			if (imageId == 119) {  /* thumbnails */
				lastThumbWidth[0] = width;
				lastThumbHeight[0] = height;
			} else if (imageId == 120) {
				lastThumbWidth[1] = width;
				lastThumbHeight[1] = height;
			} // if
			imageDirty_[imageId] = false;
		} else
			printf("ERROR: failed opening %s ID %u\n", path, imageId);
	}; // LoadImage
	static bool Init() {
		ICE_DBG(DBG_FN,"cICEOsdReel::Init");
		memset(&png_ptr, 0, sizeof(png_ptr));
		memset(&info_ptr, 0, sizeof(info_ptr));
		for(int i=0; i<MAX_CACHED_IMAGES;i++) {
			cachedImages_[i]=0;
			imagePaths_[i]="";
			imageDirty_[i]=true;
		} // for
		memset(&lastThumbWidth, 0, sizeof(lastThumbWidth));
		memset(&lastThumbHeight, 0, sizeof(lastThumbHeight));
		memset(&dirtyArea_, 0, sizeof(dirtyArea_));
		last_p1 = last_p2 = last_res = 0;
#ifdef ICE_GDL_SCALE
		gdl_ret_t ret = GDL_SUCCESS;
		if(srb_cnt == NULL) {
			if(GDL_SUCCESS != srb_alloc_context(0, &srb_cnt))
				srb_cnt = NULL;
		} // if
		if(srb_cnt == NULL) {
			esyslog("cICEOsdReel: Can't alloc srb context\n");
			return false;
		} // if
		if(GDL_SURFACE_INVALID == src_info.id) {
			GDLCHECK(gdl_alloc_surface(GDL_PF_ARGB_32, osd_width, osd_height, 0, &src_info));
			src_hdl = data = NULL;
		} // if
		if(GDL_SURFACE_INVALID == src_info.id) {
			esyslog("cICEOsdReel: Can't alloc srb context\n");
			return false;
		} // if
		if(NULL == src_hdl) GDLCHECK(srb_attach_surface(&srb_cnt, src_info.id, &src_hdl));
		if(NULL == src_hdl) {
			esyslog("cICEOsdReel: Can't attach surface\n");
			return false;
		} // if
		if(NULL == data) GDLCHECK(gdl_map_surface( src_info.id, &data, &osd_pitch));
		if(NULL == data) {
			esyslog("cICEOsdReel: Can't map surface\n");
			return false;
		} // if
#else
		if (data == NULL) {
			data = (unsigned char*) malloc(osd_bpp*osd_width*osd_height);
			memset(data, 0x00, osd_bpp*osd_width*osd_height);
		} // if
		if(data == NULL) {
			esyslog("cICEOsdReel: Can't malloc data\n");
			return false;
		} // if
#endif
		if (mySavedRegion == NULL) {
			mySavedRegion = (unsigned char*) malloc(osd_pitch*osd_height);
			memset(mySavedRegion, 0x00, osd_pitch*osd_height);
		} // if
		if (mySavedRegion == NULL) {
			esyslog("cICEOsdReel: Can't malloc saved\n");
			return false;
		} // if
		surface_info.id = GDL_SURFACE_INVALID;
#ifdef ICE_GDL_SCALE
		return true;
#else
		gdl_ret_t ret = GDL_SUCCESS;
		GDLCHECK(gdl_alloc_surface ( GDL_PF_ARGB_32, 1280, 720, /*osd_width, osd_height,*/ 0/*GDL_SURFACE_CACHED*/, &surface_info));
		if(GDL_SUCCESS != ret) {
			esyslog("cICEOsdReel: Can't alloc surface\n");
			return false;
		} // if
		return (GDL_SURFACE_INVALID != surface_info.id);
#endif
	}; // Init
	static void Done() {
		ICE_DBG(DBG_FN,"cICEOsdReel::Done");
		gdl_ret_t ret = GDL_SUCCESS;
		switch(ICE_OSD_PLANE) {
			case GDL_PLANE_ID_UPP_A: GDLCHECK(gdl_event_unregister(GDL_APP_EVENT_RECT_CHANGE_UPP_A)); break;
			case GDL_PLANE_ID_UPP_B: GDLCHECK(gdl_event_unregister(GDL_APP_EVENT_RECT_CHANGE_UPP_B)); break;
			case GDL_PLANE_ID_UPP_C: GDLCHECK(gdl_event_unregister(GDL_APP_EVENT_RECT_CHANGE_UPP_C)); break;
			case GDL_PLANE_ID_UPP_D: GDLCHECK(gdl_event_unregister(GDL_APP_EVENT_RECT_CHANGE_UPP_D)); break;
			case GDL_PLANE_ID_UPP_E: GDLCHECK(gdl_event_unregister(GDL_APP_EVENT_RECT_CHANGE_UPP_E)); break;
		} // switch
		if(GDL_SURFACE_INVALID != surface_info.id) GDLCHECK(gdl_free_surface (surface_info.id));
		surface_info.id = GDL_SURFACE_INVALID;
		int i;
		for (i = 0; i<MAX_CACHED_IMAGES; i++) {
			if(cachedImages_[i]) {
				if(cachedImages_[i]->data) {
					free(cachedImages_[i]->data);
					cachedImages_[i]->data = NULL;
				}
				free(cachedImages_[i]);
				cachedImages_[i] = NULL;
			} // if
		} // for
#ifdef ICE_GDL_SCALE
		if (data && (GDL_SURFACE_INVALID != src_info.id)) GDLCHECK(gdl_unmap_surface( src_info.id));
		data = NULL;
		if(src_hdl) GDLCHECK(srb_detach_surface(&srb_cnt, &src_hdl));
		src_hdl = NULL;
		if (GDL_SURFACE_INVALID != src_info.id) GDLCHECK(gdl_free_surface ( src_info.id));
		if(srb_cnt) GDLCHECK(srb_free_context(&srb_cnt));
		srb_cnt = NULL;
#else
		if(data) free(data);
		data = NULL;
#endif
		if(mySavedRegion) free(mySavedRegion);
		mySavedRegion = NULL;
	}; // Done
	void Clear() {
		ICE_DBG(DBG_FN,"cICEOsdReel::Clear");
		if(data) memset(data, 0x00, osd_pitch*osd_height);
		gdl_ret_t ret = GDL_SUCCESS;
		gdl_color_t bg_color = {alpha_index:0, r_y:0, g_u:0, b_v:0};
		if(GDL_SURFACE_INVALID != surface_info.id) GDLCHECK(gdl_clear_surface ( surface_info.id, &bg_color));
	}; // Clear
	void Draw8RGBLine(int x, int y, int w, const tIndex *srcData, const tColor *palette, int num_colors) {
		ICE_DBG(DBG_FN,"cICEOsdReel::Draw8RGBLine x %d y %d w %d", x, y, w);
		unsigned char *bm = (unsigned char *)srcData;
		int n = 0;
		if (x+w>=osd_width || x>=osd_width || y>=osd_height)
			return;
		uint32_t *px=(uint32_t*)(data +osd_pitch*y+x*osd_bpp);
#if OSD_EXTRA_CHECK
		if(WITHIN_OSD_BORDERS(px+w-1))
#endif
		for(n=0;n<w;n++) {
			if(palette)
				*px++=palette[(*bm++)%num_colors];
		} // for
	}; // Draw8RGBLine
	void FlushBitmap(cBitmap &bitmap, bool full) {
		ICE_DBG(DBG_FN,"cICEOsdReel::FlushBitmap full %d", full);
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
			if (x >= osd_width || y >= osd_height) {
				bitmap.Clean();
				return;
			} // if
			int w = x2 - x1 + 1;
			int h = y2 - y1 + 1;
			if (x + w > osd_width)
				w = osd_width - x;
			if (y + h > osd_height)
				h = osd_height - y;
			int numColors;
			const tColor *palette = bitmap.Colors(numColors);
			for (int yy = y1; yy < y1+h; yy++)
				Draw8RGBLine(x, y++, w, bitmap.Data(x1,yy), palette, numColors);
			bitmap.Clean();
		} // if
	}; // FlushBitmap
	bool OpenPngFile(char const *path, unsigned char **&rows, int &width, int &height) {
		//ICE_DBG(DBG_FN,"cICEOsdReel::OpenPngFile \"%s\"", path);
		unsigned char header[8];
		FILE *fp = fopen(path, "rb");
		if (!fp || fread(header, 1, 8, fp) != 8) {
			if(fp)
				fclose(fp);
			return false;
		} // if
		if (png_sig_cmp(header, 0, 8)) {
			fclose(fp);
			return false;
		} // if
		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!png_ptr) {
			fclose(fp);
			return false;
		} // if
		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr) {
			png_destroy_read_struct(&png_ptr, NULL, NULL);
			fclose(fp);
			return false;
		} // if
		png_infop end_info = png_create_info_struct(png_ptr);
		if (!end_info) {
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
			fclose(fp);
			return false;
		} // if
		png_init_io(png_ptr, fp);
		png_set_sig_bytes(png_ptr, 8);
		if (setjmp(png_jmpbuf(png_ptr))) {
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
			fclose(fp);
			return false;
		} // if
		png_read_info(png_ptr, info_ptr);
		png_byte h = info_ptr->height;
		png_byte color_type = info_ptr->color_type;
//#define PRINT_COLOR_TYPE 1
#ifdef PRINT_COLOR_TYPE
		png_byte w = info_ptr->width;
		png_byte bit_depth = info_ptr->bit_depth;
		printf("bit depth: %i - ", bit_depth);
		switch (color_type) {
			case PNG_COLOR_TYPE_GRAY: puts("color_type: PNG_COLOR_TYPE_GRAY"); break;
			case PNG_COLOR_TYPE_PALETTE: puts("color_type: PNG_COLOR_TYPE_PALETTE"); break;
			case PNG_COLOR_TYPE_RGB: puts("color_type: PNG_COLOR_TYPE_RGB"); break;
			case PNG_COLOR_TYPE_RGB_ALPHA: puts("color_type: PNG_COLOR_TYPE_RGB_ALPHA"); break;
			case PNG_COLOR_TYPE_GRAY_ALPHA: puts("color_type: PNG_COLOR_TYPE_GRAY_ALPHA"); break;
			default: puts("ERROR: unknown color_type"); break;
		} // switch
#endif
		if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_gray_to_rgb(png_ptr);
		if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY)
			png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
		png_read_update_info(png_ptr, info_ptr);
		rows = (png_bytep*) malloc(sizeof(png_bytep) * h);
		int y;
		for (y=0; y<h; y++) {
			rows[y] = (png_byte*) malloc(info_ptr->rowbytes);
		} // for
		png_read_image(png_ptr, rows);
		width = png_get_image_width(png_ptr, info_ptr);
		height = png_get_image_height(png_ptr, info_ptr);
		fclose(fp);
		return true;
	}; // OpenPngFile
	void ClosePngFile() {
		//ICE_DBG(DBG_FN,"cICEOsdReel::ClosePngFile");
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	}; // ClosePngFile
public:
	virtual ~cICEOsdReel() {
#ifdef ICE_GDL_SCALE
		ICE_DBG(DBG_FN,"cICEOsdReel::~cRBMHdOsd GDL Scale");
#else
		ICE_DBG(DBG_FN,"cICEOsdReel::~cRBMHdOsd");
#endif
		SetActive(false);
	}; // ~cICEOsdReel
	virtual cBitmap *GetBitmap(int Area) {
		ICE_DBG(DBG_FN,"cICEOsdReel::GetBitmap(%d)", Area);
		return cOsd::GetBitmap(Area);
	}; // GetBitmap
	virtual eOsdError CanHandleAreas(const tArea *Areas, int NumAreas) {
		//ICE_DBG(DBG_FN,"cICEOsdReel::CanHandleAreas Num %d", NumAreas);
		if (NumAreas > MAXOSDAREAS)
			return oeTooManyAreas;
		eOsdError Result = oeOk;
		for (int i = 0; i < NumAreas; i++) {
			if (Areas[i].x1 > Areas[i].x2 || Areas[i].y1 > Areas[i].y2 || Areas[i].x1 < 0 || Areas[i].y1 < 0)
				return oeWrongAlignment;
			if (Areas[i].bpp != 1 && Areas[i].bpp != 2 && Areas[i].bpp != 4 && Areas[i].bpp != 8 && Areas[i].bpp != 32)
				return oeBppNotSupported;
#if 0
			for (int j = i + 1; j < NumAreas; j++) {
				if (Areas[i].Intersects(Areas[j])) {
					Result = oeAreasOverlap;
					break;
				} // if
			} // for
#endif
		} // for
		return Result;
	}; // CanHandleAreas
	virtual eOsdError SetAreas(const tArea *Areas, int NumAreas) {
#if 1
		ICE_DBG(DBG_FN,"cICEOsdReel::SetAreas Num %d", NumAreas);
		eOsdError Result = cOsd::SetAreas(Areas, NumAreas);
		if(!IsTrueColor()) {
			cBitmap* Bitmap;
			for(int i=0; (Bitmap = cOsd::GetBitmap(i)) != NULL; i++)
				Bitmap->Clean();
		} // if
		Clear();
		return Result;
#else
		ICE_DBG(DBG_FN,"cICEOsdReel::SetAreas Num %d", NumAreas);
		eOsdError Result = CanHandleAreas(Areas, NumAreas); // 1.6
		if (Result == oeOk) {
			width = height = 0;
			for (int i = 0; i < NumAreas; i++) {
				width = max(width, Areas[i].x2 + 1);
				height = max(height, Areas[i].y2 + 1);
			}
		} else // 1.6
			esyslog("ERROR: cICEOsdReel::SetAreas returned %d", Result);
		return Result;
#endif
	}; // SetAreas
	virtual void SaveRegion(int x1, int y1, int x2, int y2) {
		ICE_DBG(DBG_FN,"cICEOsdReel::SaveRegion(%d,%d,%d,%d)",x1, y1, x2, y2);
		x1 += left; y1 += top; x2 += left; y2 += top;
		savedRegion_x0 = x1; savedRegion_y0 = y1; savedRegion_x1 = x2; savedRegion_y1 = y2+1;
		int lines = y2 - y1+1;    
		int pixels = x2 - x1;
		int i;
		if (pixels > 0)
			for(i = 0; i < lines; i++)
				memcpy(mySavedRegion + (i+y1)*osd_pitch + x1*osd_bpp, data + (i+y1)*osd_pitch + x1*osd_bpp, pixels*osd_bpp);
	}; // SaveRegion
	virtual void RestoreRegion(void) {
		ICE_DBG(DBG_FN,"cICEOsdReel::RestoreRegion");
		int x1 = savedRegion_x0; int y1 = savedRegion_y0; int x2 = savedRegion_x1; int y2 = savedRegion_y1;
		int lines = y2 - y1;    
		int pixels = x2 - x1;
		int i;
		if (pixels > 0)
			for(i = 0; i < lines; i++)
				memcpy(data + (i+y1)*osd_pitch + x1*osd_bpp, mySavedRegion + (i+y1)*osd_pitch + x1*osd_bpp, pixels*osd_bpp);
	}; // RestoreRegion
	virtual eOsdError SetPalette(const cPalette &Palette, int Area) {
		ICE_DBG(DBG_FN,"cICEOsdReel::SetPalette Area %d", Area);
		return cOsd::SetPalette(Palette, Area);
	}; // SetPalette
	virtual void DrawPixel(int x, int y, tColor Color) {
		ICE_DBG(DBG_FN,"cICEOsdReel::DrawPixel");
		x += left;
		y += top;
		UpdateDirty(x, y, x+1, y+1);
		if (0 <= x && x < osd_width && 0 <= y && y < osd_height)
			*((uint32_t*)(data + osd_pitch * y   + x * osd_bpp )) = Color;
	}; // DrawPixel
	virtual void DrawBitmap(int x, int y, const cBitmap &Bitmap, tColor ColorFg = 0, tColor ColorBg = 0, bool ReplacePalette = false, bool Overlay = false) {
		ICE_DBG(DBG_FN,"cICEOsdReel::DrawBitmap %d %d", x, y);
		unsigned char const *srcData = (unsigned char const *)Bitmap.Data(0,0);
		unsigned char const *xs;
		int qx, qy, xt1, yt1, xt, yt, vfx, vfy, vfw, vfh, w, h, m, *px, n;
		x += left;
		y += top;
		w=Bitmap.Width();
		h=Bitmap.Height();
		if (w+x > osd_width || y+h > osd_height)
			return;
		UpdateDirty(x, y, x+w, y+h);
		if (Overlay) {
			vfx = 0;
			vfy = 0;
			vfw = osd_width-1;
			vfh = osd_height-1;
			xt1 = vfx * osd_width + x * vfw;
			xt = xt1 / osd_width;
			yt1 = vfy * osd_height + y * vfh;
			yt = yt1 / osd_height;
			qy = yt1 % osd_height;
			for(m=0;m<h;) {
				px=(int*)(data + osd_pitch * yt   + xt * osd_bpp );
				xs = srcData + m * w;
				qx = xt1 % osd_width;
#ifdef OSD_EXTRA_CHECK
				if(WITHIN_OSD_BORDERS(px+w-1))
#endif
				for(n=0;n<w;) {
					*px++=Bitmap.Color(*xs);
					qx += osd_width;
					while (qx >= osd_width) {
						++xs;
						++n;
						qx -= vfw;
					} // while
				} // for
				++yt;
				qy += osd_height;
				while (qy >= osd_height) {
					++m;
					qy -= vfh;
				} // while
			} // for
		} else {
			unsigned int *bm = (unsigned int*)Bitmap.Data(0,0);
			int o=0, val = 0;
			for (m=0;m<h;m++) {
				px=(int*)(data +osd_pitch*(y+m)+x*osd_bpp);
#if OSD_EXTRA_CHECK
				if(WITHIN_OSD_BORDERS(px+w-1))
#endif
				for(n=0;n<w;n++) {
					if ((o&3)==0)
						val=*bm++;
					*px++=Bitmap.Color(val&255);
					val>>=8;
					o++;
				} // for
			} // for
		} // fi
	}; // DrawBitmap
	virtual void DrawBitmap32(int x0, int y0, const cBitmap &Bitmap, tColor ColorFg, tColor ColorBg, bool ReplacePalette, bool Overlay, int width_, int height_) {
		ICE_DBG(DBG_FN,"cICEOsdReel::DrawBitmap32 %d %d", x0, y0);
		unsigned char const *srcData = Bitmap.Data(0,0);
		unsigned char const *xs;
		static int qx, qy, xt1, yt1, line, row;
		static uint32_t pxs, *px;
		UpdateDirty(x0, y0, x0+width_, y0+height_);
		xt1 = x0 * osd_width;
		yt1 = y0 * osd_height;
		qy = 0;
		int numColors;
		tColor const *colors = Bitmap.Colors(numColors);
		for (int n = 0; n < numColors; ++n)
			palette[n] = colors[n];
		for(line=0;line<height_; y0++) {
			px=(uint32_t*)(data + osd_pitch * y0   + x0 * osd_bpp );
			xs = srcData + line * width_;
			qx = xt1 % osd_width;
#if 0 //def OSD_EXTRA_CHECK
			if(WITHIN_OSD_BORDERS(px+w-1))
#endif
			for(row=0;row<width_ && ((int*)(px+width_) < (int*)(data+osd_pitch*osd_height)); px++) {
				pxs = palette[*xs];
				if(pxs&0x00FFFFFF && pxs!=0x01ffffff)
					*px = DoAlphaBlend(pxs, *px);
				else
					*px = DoAlphaBlend(*px, pxs);
				if (qx >= 0) {
					xs++;
					row++;
					qx = 0;
				} else
					qx += osd_width;
			} // for
			if (qy >= 0) {
				++line;
				qy = 0;
			} else
				qy += osd_height;
		} // for
	}; // DrawBitmap32
	virtual void DrawCropImage(u_int imageId, int x, int y, int x0, int y0, int x1, int y1, bool blend) {
		ICE_DBG(DBG_FN,"cICEOsdReel::DrawCropImage %d %d", x, y);
		if (!ImageIdInRange(imageId)) return;
		/* adjust coordinates with global OSD-margins */
		x+=left;
		y+=top;
		x0+=left;
		y0+=top;
		x1+=left;
		y1+=top;
		UpdateDirty(x0, y0, x1, y1);
		LoadImage(imageId);
		CachedImage const *img = cachedImages_[imageId];
		int m, n, h, v;
		int width_;    
		int height_;    
		uint32_t const *srcPixels;
		int vertRepeat = 1;
		int horRepeat = 1;
		if (img) {
			if(x0<x || y0<y) return; // invalid param
			if(x1<=x0 || y1<=y0) return; // inavlid param
			if(img->height<=0 || img->width<=0) return; // invalid image
			if(x0-x >= img->width) return; // invalid param for this image
			if(y0-y >= img->height) return; // invalid param for this image
			
			width_ = x1-x0;
			height_ = y1-y0;
			if(img->height)
				vertRepeat = (y1-y0)/img->height;
			if(vertRepeat < 1)
				vertRepeat = 1;
			if(img->width)
				horRepeat = (x1-x0)/img->width;
			if(horRepeat < 1)
				horRepeat = 1;
			if (blend) {
				int dy=y0;
				for (v = vertRepeat; v > 0; --v) {
					srcPixels = img->data + img->width*(y0-y) + (x0-x);
					for (n = min(height_, img->height); n > 0; --n) {
						uint32_t *tgtPixels = (uint32_t*)(data + osd_pitch * dy++ + x0*osd_bpp);
						for (h = horRepeat; h > 0; --h) {
							uint32_t const *src = srcPixels;
							for (m = min(width_, img->width); m > 0; --m) {
								*tgtPixels = DoAlphaBlend((*src++), (*tgtPixels) );
								++tgtPixels;
							} // for
						} // for
						srcPixels += img->width;
					} // for
				} // for
			} else {
				int dy=y0;
				for (v = vertRepeat; v > 0; --v) {
					srcPixels = img->data + img->width*(y0-y) + (x0-x);
					for (n = min(height_, img->height); n > 0; --n) {
						uint32_t *tgtPixels = (uint32_t*)(data + osd_pitch * dy++ + x0*osd_bpp);
						for (h = horRepeat; h > 0; --h) {
							uint32_t const *src = srcPixels;
							memcpy(tgtPixels, src, min(width_, img->width)*sizeof(int));
							tgtPixels += img->width;
						} // for
						srcPixels += img->width;
					} // for
				} // for
			} // if
		} else {
			printf("Image %d not cached.\n", imageId);
		} // if
	}; // DrawCropImage
	virtual void DrawText(int x, int y, const char *s_in, tColor colorFg, tColor colorBg, const cFont *font, int w = 0, int h = 0, int alignment = taDefault) {
//		ICE_DBG(DBG_FN,"cICEOsdReel::DrawText %d %d \"%s\"", x, y, s_in);
		if (s_in) {
			/* adjust coordinates with global OSD-margins */        
			x+=left;
			y+=top;
			/* check for empty string */
			unsigned int i;
			unsigned int len = strlen(s_in);
			for(i=0; i<len; i++){ /* count the spaces */
				if(s_in[i] == ' ')
					continue;
				else
					break;
			} // for
			if(w == 0) w = font->Width(s_in);
			if(h == 0) h = font->Height();
			if(i == len) { /* every char is a space */
				if((colorBg >> 24) != 0) /* not transparent */
					DrawRectangle(x-left, y-top, x + w - left, y + h - top, colorBg); /* clear the background */
				return;
			} // if
			UpdateDirty(x, y, x+w, y+w);
			if((colorBg >> 24 == 0) || ((colorBg&0x00ffffff) == 0x00000000)){ /* TB: transparent as bgcolor is evil */
				colorBg = colorFg&0x01ffffff; 
			} // if
			int old_x = x; int old_y = y; y=0; x=0;
			int w_ = font->Width(s_in);
			int h_ = font->Height();
			int limit = 0;
			if (w || h) {
				int cw = w ? w : w_;
				limit = x + cw;
				if (w) {
					if ((alignment & taLeft) != 0)
						;
					else if ((alignment & taRight) != 0) {
						if (w_ < w)
							x += w - w_;
					} else { // taCentered
						if (w_ < w)
							x += (w - w_) / 2;
					} // if
				} // if
				if (h) {
					if ((alignment & taTop) != 0)
						;
					else if ((alignment & taBottom) != 0) {
						if (h_ < h)
							y += h - h_;
					} else { // taCentered
						if (h_ < h)
							y += (h - h_) / 2;
					} // if
				} // if
			} // if
			bool AntiAliased = Setup.AntiAlias;
			bool TransparentBackground = (colorBg == clrTransparent);
			static int16_t BlendLevelIndex[MAX_BLEND_LEVELS]; // tIndex is 8 bit unsigned, so a negative value can be used to mark unused entries
			if (AntiAliased && !TransparentBackground)
				memset(BlendLevelIndex, 0xFF, sizeof(BlendLevelIndex)); // initializes the array with negative values
			cPalette palette;
			palette.Index(colorFg);
			uint prevSym = 0;
			while (*s_in) {
				int sl = Utf8CharLen(s_in);
				uint sym = Utf8CharGet(s_in, sl);
				s_in += sl;
				cGlyph *g = ((cFreetypeFont*)font)->Glyph(sym, AntiAliased);
				if (!g)
					continue;
				int kerning = ((cFreetypeFont*)font)->Kerning(g, prevSym);
				prevSym = sym;
				uchar *buffer = g->Bitmap();
				int symWidth = g->Width();
				int symLeft = g->Left();
				int symTop = g->Top();
				int symPitch = g->Pitch();
				if (limit && x + symWidth + symLeft + kerning - 1 > limit)
					break; // we don't draw partial characters
				int px_tmp_sum = symLeft + kerning + x;
				int py_tmp_sum = y + (font->Height() - ((cFreetypeFont*)font)->Bottom() - symTop);
				if (x + symWidth + symLeft + kerning > 0) {
					for (int row = 0; row < g->Rows(); row++) {
						for (int pitch = 0; pitch < symPitch; pitch++) {
							uchar bt = *(buffer + (row * symPitch + pitch));
							if (AntiAliased) {
								if (bt > 0x00) {
									int px = pitch + px_tmp_sum;
									int py = row + py_tmp_sum;
									uint32_t *dstPx = (uint32_t*)(data + osd_pitch * (py+old_y)   + (px+old_x) * osd_bpp );
									if (bt == 0xFF)
										*dstPx = colorFg;
									else if (TransparentBackground)
										*dstPx = palette.Blend(colorFg, *dstPx, bt);
									else if (BlendLevelIndex[bt] >= 0)
										*dstPx = palette.Blend(palette.Color(BlendLevelIndex[bt]), *dstPx, bt);
									else
										*dstPx = palette.Blend(colorFg, *dstPx, bt);
								} // if
							} else { //monochrome rendering
								for (int col = 0; col < 8 && col + pitch * 8 <= symWidth; col++) {
									if (bt & 0x80) {
										uint32_t *dstPx = (uint32_t*)(data + osd_pitch * (old_y + y + row + (font->Height() - ((cFreetypeFont*)font)->Bottom() - symTop))   + (old_x + x + col + pitch * 8 + symLeft + kerning) * osd_bpp );
										*dstPx = colorFg;
									} // if
									bt <<= 1;
								} // for
							} // if
						} // for
					} // for
				} // if
				x += g->AdvanceX() + kerning;
				if (x > w - 1)
					break;
			} // while
		} // if
	}; // DrawText
	virtual void DrawImage(const cPoint &Point, const cImage &Image) {
		cOsd::DrawImage(Point, Image);
	} // DrawImage
	virtual void DrawImage(const cPoint &Point, int ImageHandle) {
		cOsd::DrawImage(Point, ImageHandle);
	} // DrawImage
	virtual void DrawImage(u_int imageId, int x, int y, bool blend, int horRepeat = 1, int vertRepeat = 1) {
		//ICE_DBG(DBG_FN,"cICEOsdReel::DrawImage id %d x %d y %d blend %d h %d v %d data %p", imageId, x, y, blend, horRepeat, vertRepeat, data);
		if (!ImageIdInRange(imageId)) return;
		/* adjust coordinates with global OSD-margins */
		x+=left;
		y+=top;
		LoadImage(imageId);
		CachedImage const *img = cachedImages_[imageId];
		int m, n, h_, v;
		int w, h;
		uint32_t const *srcPixels;
		if (img) {
			w = img->width;    
			h = img->height;
			int w_all = vertRepeat ? vertRepeat * w : w;
			int h_all = horRepeat ? horRepeat * h : h;
			UpdateDirty(x, y, x+w_all, y+h_all);
			if (blend) {
				for (v = vertRepeat; v > 0; --v) {
					srcPixels = img->data;
					for (n = h; n > 0; --n) {
						uint32_t *tgtPixels = (uint32_t*)(data + osd_pitch * y++ + x*osd_bpp);
						for (h_ = horRepeat; h_ > 0; --h_) {
							uint32_t const *src = srcPixels;
#if OSD_EXTRA_CHECK
							if(WITHIN_OSD_BORDERS(tgtPixels+w))
#endif
							for (m = w; m > 0; --m) {
								int res = DoAlphaBlend((*src++), (*tgtPixels) );          // where they come from (libpng?) - so let's filter them out
								if( (res>>24)!=0 || (res&0xffffff) != 0xffffff){ // TB: there are white pixels on the left corner of the OSD - I don't know
									*tgtPixels = res;          // where they come from (libpng?) - so let's filter them out
								} else                            // explanation: those pixels are completely transparent, but there 
									*tgtPixels = 0x00000000;    // is no alpha-blending done as long as there isn't any video data
								++tgtPixels;
							} // for
						} // for
						srcPixels += w;
					} // for
				} // for
			} else {
				for (v = vertRepeat; v > 0; --v) {
					srcPixels = img->data;
					for (n = h; n > 0; --n) {
						uint32_t *tgtPixels = (uint32_t*)(data + osd_pitch * y++ + x*osd_bpp);
						for (h_ = horRepeat; h_ > 0; --h_) {
							uint32_t const *src = srcPixels;
#if OSD_EXTRA_CHECK
							if(WITHIN_OSD_BORDERS(tgtPixels+w*(sizeof(int))))
#endif
							for (m = w; m > 0; --m) {
								if((*src>>24)!=0 || (*src&0xffffff) != 0xffffff){ // TB: there are white pixels on the left corner of the OSD - I don't know
									*tgtPixels = *src;          // where they come from (libpng?) - so let's filter them out
								} else                            // explanation: those pixels are completely transparent, but there 
									*tgtPixels = 0x00000000;    // is no alpha-blending done as long as there isn't any video data
								++src;
								++tgtPixels;
							} // for
						} // for
						srcPixels += w;
					} // for
				} // for
			} // if
		} else {
			printf("Image %d not cached.\n", imageId);
		} // if
	}; // DrawImage
	virtual void DrawRectangle(int l, int t, int r, int b, tColor Color) {
		//ICE_DBG(DBG_FN,"cICEOsdReel::DrawRectangle1 %d %d %d %d", l, t, r, b);
		/* adjust coordinates with global OSD-margins */
		l+=left;
		t+=top;
		r+=left+1;
		b+=top+1;
		if (ClipArea(&l, &t, &r, &b)) {
			UpdateDirty(l, t, r, b);
			unsigned int m, n;
			unsigned int w = r - l;
			unsigned int h = b - t;
			uint32_t *pixels = (uint32_t*)(data + osd_pitch * t   + l*osd_bpp);
			if (draw_color!=Color) {
				for(n=0;n<1024;n++)
					draw_linebuf[n]=Color;
				draw_color=Color;
			} // if
			for (m = h; m; --m) {
#if OSD_EXTRA_CHECK
				if(WITHIN_OSD_BORDERS(pixels+w*sizeof(int)))
#endif
				memcpy(pixels,draw_linebuf,w*sizeof(int));
				pixels+=osd_pitch/osd_bpp;//osd_width;
			} // for
		} // if
	}; // DrawRectangle
	virtual void DrawRectangle(int l, int t, int r, int b, tColor color, int alphaGradH, int alphaGradV, int alphaGradStepH, int alphaGradStepV) {
		//ICE_DBG(DBG_FN,"cICEOsdReel::DrawRectangle2 %d %d %d %d", l, t, r, b);
		/* adjust coordinates with global OSD-margins */
		l+=left;
		t+=top;
		r+=left+1;
		b+=top+1;
		if (ClipArea(&l, &t, &r, &b)) {
			UpdateDirty(l, t, r, b);
			unsigned int m, n;
			unsigned int w = r - l;
			unsigned int h = b - t;
			unsigned int mod = osd_width - w;
			uint32_t *pixels = (uint32_t*)(data + osd_pitch * t   + l*osd_bpp);
			unsigned int curAlphaH = 0, curAlphaV = 0;
			for(n=0; n<h; n++){
				if(alphaGradStepV && n%alphaGradStepV==0)
					curAlphaV++;
#if OSD_EXTRA_CHECK
				if(WITHIN_OSD_BORDERS(pixels+w-1))
#endif
				for (m=0; m<w; m++){
					if(alphaGradStepH && m%alphaGradStepH==0)
						curAlphaH++;
					*pixels++ = ( (color&0x00FFFFFF) | (((color>>24) + ((curAlphaH*alphaGradH + curAlphaV*alphaGradV)))<<24) );
				} // for
				pixels += mod;
				curAlphaH = 0;
			} // for
		} // if
	}; // DrawRectangle
	virtual void DrawEllipse(int x1, int y1, int x2, int y2, tColor Color, int Quadrants = 0) {
		ICE_DBG(DBG_FN,"cICEOsdReel::DrawEllipse %d %d %d %d", x1, y1, x2, y2);
		x1 += left;
		y1 += top;
		x2 += left;
		y2 += top;
		UpdateDirty(x1, y1, x2, y2);
		// Algorithm based on http://homepage.smc.edu/kennedy_john/BELIPSE.PDF
		int rx = x2 - x1;
		int ry = y2 - y1;
		int cx = (x1 + x2) / 2;
		int cy = (y1 + y2) / 2;
		switch (abs(Quadrants)) {
			case 0: rx /= 2; ry /= 2; break;
			case 1: cx = x1; cy = y2; break;
			case 2: cx = x2; cy = y2; break;
			case 3: cx = x2; cy = y1; break;
			case 4: cx = x1; cy = y1; break;
			case 5: cx = x1;          ry /= 2; break;
			case 6:          cy = y2; rx /= 2; break;
			case 7: cx = x2;          ry /= 2; break;
			case 8:          cy = y1; rx /= 2; break;
		} // switch
		int TwoASquare = 2 * rx * rx;
		int TwoBSquare = 2 * ry * ry;
		int x = rx;
		int y = 0;
		int XChange = ry * ry * (1 - 2 * rx);
		int YChange = rx * rx;
		int EllipseError = 0;
		int StoppingX = TwoBSquare * rx;
		int StoppingY = 0;
		while (StoppingX >= StoppingY) {
			switch (Quadrants) {
				case  5: DrawRectangle(cx,     cy + y, cx + x, cy + y, Color); // no break
				case  1: DrawRectangle(cx,     cy - y, cx + x, cy - y, Color); break;
				case  7: DrawRectangle(cx - x, cy + y, cx,     cy + y, Color); // no break
				case  2: DrawRectangle(cx - x, cy - y, cx,     cy - y, Color); break;
				case  3: DrawRectangle(cx - x, cy + y, cx,     cy + y, Color); break;
				case  4: DrawRectangle(cx,     cy + y, cx + x, cy + y, Color); break;
				case  0:
				case  6: DrawRectangle(cx - x, cy - y, cx + x, cy - y, Color); if (Quadrants == 6) break;
				case  8: DrawRectangle(cx - x, cy + y, cx + x, cy + y, Color); break;
				case -1: DrawRectangle(cx + x, cy - y, x2,     cy - y, Color); break;
				case -2: DrawRectangle(x1,     cy - y, cx - x, cy - y, Color); break;
				case -3: DrawRectangle(x1,     cy + y, cx - x, cy + y, Color); break;
				case -4: DrawRectangle(cx + x, cy + y, x2,     cy + y, Color); break;
			} // switch
			y++;
			StoppingY += TwoASquare;
			EllipseError += YChange;
			YChange += TwoASquare;
			if (2 * EllipseError + XChange > 0) {
				x--;
				StoppingX -= TwoBSquare;
				EllipseError += XChange;
				XChange += TwoBSquare;
			} // if
		} // while
		x = 0;
		y = ry;
		XChange = ry * ry;
		YChange = rx * rx * (1 - 2 * ry);
		EllipseError = 0;
		StoppingX = 0;
		StoppingY = TwoASquare * ry;
		while (StoppingX <= StoppingY) {
			switch (Quadrants) {
				case  5: DrawRectangle(cx,     cy + y, cx + x, cy + y, Color); // no break
				case  1: DrawRectangle(cx,     cy - y, cx + x, cy - y, Color); break;
				case  7: DrawRectangle(cx - x, cy + y, cx,     cy + y, Color); // no break
				case  2: DrawRectangle(cx - x, cy - y, cx,     cy - y, Color); break;
				case  3: DrawRectangle(cx - x, cy + y, cx,     cy + y, Color); break;
				case  4: DrawRectangle(cx,     cy + y, cx + x, cy + y, Color); break;
				case  0:
				case  6: DrawRectangle(cx - x, cy - y, cx + x, cy - y, Color); if (Quadrants == 6) break;
				case  8: DrawRectangle(cx - x, cy + y, cx + x, cy + y, Color); break;
				case -1: DrawRectangle(cx + x, cy - y, x2,     cy - y, Color); break;
				case -2: DrawRectangle(x1,     cy - y, cx - x, cy - y, Color); break;
				case -3: DrawRectangle(x1,     cy + y, cx - x, cy + y, Color); break;
				case -4: DrawRectangle(cx + x, cy + y, x2,     cy + y, Color); break;
			} // switch
			x++;
			StoppingX += TwoBSquare;
			EllipseError += XChange;
			XChange += TwoBSquare;
			if (2 * EllipseError + YChange > 0) {
				y--;
				StoppingY -= TwoASquare;
				EllipseError += YChange;
				YChange += TwoASquare;
			} // if
		} // while
	}; // DrawEllipse
	virtual void DrawSlope(int x1, int y1, int x2, int y2, tColor Color, int Type) {
		ICE_DBG(DBG_FN,"cICEOsdReel::DrawSlope %d %d %d %d", x1, y1, x2, y2);
		x1 += left;
		y1 += top;
		x2 += left;
		y2 += top;       
		UpdateDirty(x1, y1, x2, y2);
		/* TODO This is just a quick and dirty implementation of a slope drawing
		 * machanism. If somebody can come up with a better solution, let's have it! */
		bool upper    = Type & 0x01;
		bool falling  = Type & 0x02;
		bool vertical = Type & 0x04;
		if (vertical) {
			for (int y = y1; y <= y2; y++) {
				double c = cos((y - y1) * M_PI / (y2 - y1 + 1));
				if (falling)
					c = -c;
				int x = int((x2 - x1 + 1) * c / 2);
				if ((upper && !falling) || (!upper && falling))
					DrawRectangle(x1, y, (x1 + x2) / 2 + x, y, Color);
				else
					DrawRectangle((x1 + x2) / 2 + x, y, x2, y, Color);
			} // for
		} else {
			for (int x = x1; x <= x2; x++) {
				double c = cos((x - x1) * M_PI / (x2 - x1 + 1));
				if (falling)
					c = -c;
				int y = int((y2 - y1 + 1) * c / 2);
				if (upper)
					DrawRectangle(x, y1, x, (y1 + y2) / 2 + y, Color);
				else
					DrawRectangle(x, (y1 + y2) / 2 + y, x, y2, Color);
			} // for
		} // if
	}; // DrawSlope
	static cMutex FlushMutex;
	virtual void Flush(void) {
		if(!Active()) return;
		if(!IsTrueColor()) {
			cBitmap* Bitmap;
			for(int i=0; (Bitmap = cOsd::GetBitmap(i)) != NULL; i++) {
				int x1, x2, y1, y2;
				bool isDirty = Bitmap->Dirty(x1, y1, x2, y2);
				if(isDirty) DrawBitmap(0, 0, *Bitmap);
				Bitmap->Clean();
			}
		}
#ifdef ICE_GDL_SCALE
		if(!srb_cnt) return;
		cMutexLock FlushLock(&FlushMutex);
		gdl_ret_t ret = GDL_SUCCESS;
		gdl_rectangle_t dst_rect;
		GDLCHECK(gdl_plane_get_rect(ICE_OSD_PLANE, GDL_PLANE_DST_RECT, &dst_rect));
		static srb_context_t dst_hdl = NULL;
		if((GDL_SURFACE_INVALID == surface_info.id) || 
		   (dst_rect.width      != surface_info.width) ||
		   (dst_rect.height     != surface_info.height)) {
			if(GDL_SURFACE_INVALID != surface_info.id) {
				if(dst_hdl) srb_detach_surface(&srb_cnt, &dst_hdl);
				dst_hdl = NULL;
				GDLCHECK(gdl_free_surface (surface_info.id));
			} // if
			surface_info.id = GDL_SURFACE_INVALID;
			GDLCHECK(gdl_alloc_surface ( GDL_PF_ARGB_32, dst_rect.width, dst_rect.height, 0/*GDL_SURFACE_CACHED*/, &surface_info));
			printf("Allocated new surface %dx%d\n", dst_rect.width, dst_rect.height);
		} // if
		if(!dst_hdl) GDLCHECK(srb_attach_surface(&srb_cnt, surface_info.id, &dst_hdl));
		if(!src_hdl || !dst_hdl) return;
		cache_flush_buffer(data, osd_pitch*osd_height);
		srb_blit_info_t blit_info;
		if(Osd3D() && (dst_rect.width > 576)) {
			memset(&blit_info, 0, sizeof(blit_info));
			blit_info.src_surface_handle  = &src_hdl;
			blit_info.dest_surface_handle = &dst_hdl;
			blit_info.src_rect.origin.x   = 0;
			blit_info.src_rect.origin.y   = 0;
			blit_info.src_rect.width      = osd_width;
			blit_info.src_rect.height     = osd_height;
			blit_info.dest_rect           = dst_rect;
			blit_info.clip_rect           = dst_rect;
			blit_info.blend.flags         = SRB_BLEND_ENABLE_BLEND_EQUATION;
			blit_info.blend.src_rgb       = SRB_BLEND_FUNC_ONE;
			blit_info.blend.src_alpha     = SRB_BLEND_FUNC_ONE;
			blit_info.blend.dest_rgb      = SRB_BLEND_FUNC_ZERO;
			blit_info.blend.dest_alpha    = SRB_BLEND_FUNC_ZERO;
			blit_info.filter              = SRB_FILTER_LINEAR;
			blit_info.dest_rect.width = blit_info.clip_rect.width = dst_rect.width/2;
			GDLCHECK(srb_blit(&srb_cnt, &blit_info));
			blit_info.dest_rect.origin.x = blit_info.clip_rect.origin.x = (dst_rect.width/4)*2;
			GDLCHECK(srb_blit(&srb_cnt, &blit_info));
			GDLCHECK(srb_wait(&srb_cnt, blit_info.dest_surface_handle));
		} else {
			memset(&blit_info, 0, sizeof(blit_info));
			blit_info.src_surface_handle  = &src_hdl;
			blit_info.dest_surface_handle = &dst_hdl;
			blit_info.src_rect.origin.x   = 0;
			blit_info.src_rect.origin.y   = 0;
			blit_info.src_rect.width      = osd_width;
			blit_info.src_rect.height     = osd_height;
			blit_info.dest_rect           = dst_rect;
			blit_info.clip_rect           = dst_rect;
			blit_info.blend.flags         = SRB_BLEND_ENABLE_BLEND_EQUATION;
			blit_info.blend.src_rgb       = SRB_BLEND_FUNC_ONE;
			blit_info.blend.src_alpha     = SRB_BLEND_FUNC_ONE;
			blit_info.blend.dest_rgb      = SRB_BLEND_FUNC_ZERO;
			blit_info.blend.dest_alpha    = SRB_BLEND_FUNC_ZERO;
			blit_info.filter              = SRB_FILTER_LINEAR;
			GDLCHECK(srb_blit(&srb_cnt, &blit_info));
			GDLCHECK(srb_wait(&srb_cnt, blit_info.dest_surface_handle));
		} // if
		ret = gdl_flip (ICE_OSD_PLANE, surface_info.id,  GDL_FLIP_SYNC /*GDL_FLIP_ASYNC*/);
if(GDL_SUCCESS != ret) {
	gdl_display_info_t di;
	memset(&di, 0,sizeof(di));
	gdl_get_display_info(GDL_DISPLAY_ID_0, &di);
	isyslog("osd %d %d dst %d %d sur %d %d dev %d %d", osd_width, osd_height, dst_rect.width, dst_rect.height, surface_info.width, surface_info.height, di.tvmode.width, di.tvmode.height);
} // if
#else
		//ICE_DBG(DBG_FN,"cICEOsdReel::Flush %d %d", ICE_OSD_PLANE, surface_info.id);
		if(GDL_SURFACE_INVALID == surface_info.id) return; 
		gdl_ret_t ret = GDL_SUCCESS;
		gdl_uint8 *ptr = NULL;
		gdl_uint32 pitch;
		GDLCHECK(gdl_map_surface( surface_info.id, &ptr, &pitch));
		if(!ptr) return;
		int lines = dirtyArea_.y1 - dirtyArea_.y0;    
		int pixels = dirtyArea_.x1 - dirtyArea_.x0;
		int i, ii;
		if(pixels > 0 && pixels <= (int)(osd_width - dirtyArea_.x0))
			for(ii = 0; ii < lines; ii++) {
				if(ii&1)
					i = lines/2-ii/2;
				else
					i = lines/2+ii/2;
				memcpy(ptr + (i+dirtyArea_.y0)*pitch + dirtyArea_.x0*osd_bpp, data + (i+dirtyArea_.y0)*osd_width*osd_bpp + dirtyArea_.x0*osd_bpp, pixels*osd_bpp);
			} // for
		dirtyArea_.x0 = osd_width-1;
		dirtyArea_.y0 = osd_height-1;
		dirtyArea_.x1 = 1;
		dirtyArea_.y1 = 1;
		
		GDLCHECK(gdl_unmap_surface( surface_info.id));
		gdl_flip (ICE_OSD_PLANE, surface_info.id, GDL_FLIP_ASYNC);
#endif /*ICE_GDL_SCALE*/
	}; // Flush
	void SetImagePath(u_int imageId, char const *path) {
		//ICE_DBG(DBG_FN,"cICEOsdReel::SetImagePath %d \"%s\"", imageId, path);
		if (!ImageIdInRange(imageId)) return;
		if (strcmp(path, imagePaths_[imageId].c_str())) { /* TB: no need to reload if path hasn't changed */
			imagePaths_[imageId] = path;
			imageDirty_[imageId] = true;
		} // if
	}; // SetImagePath
	bool inline ClipArea(int *l, int *t, int *r, int *b) {
		//ICE_DBG(DBG_FN,"cICEOsdReel::ClipArea");
		if (*r >= osd_width)
			*r = osd_width-1;
		if (*b >= osd_height)
			*b = osd_height-1;
		return *l < *r && *t < *b;
	}; // ClipArea
	void UpdateDirty(int x0, int y0, int x1, int y1) {
		//ICE_DBG(DBG_FN,"cICEOsdReel::UpdateDirty");
		if(x0 >= osd_width)  x0 = osd_width-1;
		if(x1 >= osd_width)  x1 = osd_width-1;
		if(y0 >= osd_height) y0 = osd_height-1;
		if(y1 >= osd_height) y1 = osd_height-1;
		if (dirtyArea_.x0 > x0)  dirtyArea_.x0 = x0;
		if (dirtyArea_.y0 > y0)  dirtyArea_.y0 = y0-1; //TB: -1 is needed, 'cause else the top line of the progress bar is missing in the replay display
		if (dirtyArea_.x1 < x1)  dirtyArea_.x1 = x1;
		if (dirtyArea_.y1 < y1)  dirtyArea_.y1 = y1;
	}; // UpdateDirty
private:
	int left;       /** left margin of the OSD actually used */
	int top;        /** top margin of the OSD actually used */
	unsigned int palette[256];
}; // cICEOsdReel

cMutex cICEOsdReel::FlushMutex;


