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

#include "reelice.h"

#define DBG_FN	DBG_STDOUT
//DBG_STDOUT
#define DBG_GDL DBG_STDOUT
//DBG_SYSLOG
//DBG_NONE

#include <vdr/osd.h>
#include <x86_cache.h>

gdl_color_t bg_color = {alpha_index:0, r_y:0, g_u:0, b_v:0};

class cICEOsd : public cOsd {
	friend class cICEOsdProvider;
private:
		bool refresh;
protected:
	cICEOsd(cPluginICE *Plugin, int Left, int Top, uint Level)
			:cOsd(Left, Top, Level) {
		refresh = true;
		surface_info.id = GDL_SURFACE_INVALID;
	}; // cICEOsd
public:
	virtual ~cICEOsd() {
		ICE_DBG(DBG_FN,"cICEOsd::~cICEOsd");
		gdl_ret_t ret = GDL_SUCCESS;
		if(GDL_SURFACE_INVALID != surface_info.id) {
			GDLCHECK(gdl_clear_surface ( surface_info.id, &bg_color));
			GDLCHECK(gdl_flip (IsTrueColor() ? ICE_OSD_PLANE : ICE_TXT_PLANE, surface_info.id, GDL_FLIP_SYNC));
			GDLCHECK(gdl_free_surface (surface_info.id));
		} // if
		surface_info.id = GDL_SURFACE_INVALID;
	}; // ~cICEOsd
	virtual void SetActive(bool On) {
		ICE_DBG(DBG_FN,"cICEOsd::SetActive %s  %s", On ? "ON" : "OFF", IsTrueColor() ? "TrueColor" : "Indexed");
		if (On != Active()) {
			cOsd::SetActive(On);
			if (On) {
				gdl_ret_t ret = GDL_SUCCESS;
				if (GDL_SURFACE_INVALID == surface_info.id) {
					int Width, Height;
					double Aspect;
					cDevice::PrimaryDevice()->GetOsdSize(Width, Height, Aspect);
					GDLCHECK(gdl_alloc_surface ( IsTrueColor() ? GDL_PF_ARGB_32 : GDL_PF_ARGB_8, Width, Height, 0/*GDL_SURFACE_CACHED*/, &surface_info));
					if(GDL_SUCCESS != ret) {
						esyslog("cICEOsd: Can't alloc surface\n");
						surface_info.id = GDL_SURFACE_INVALID;
					} // if
					GDLCHECK(gdl_clear_surface ( surface_info.id, &bg_color));
					refresh = true;
					if (GetBitmap(0)) // only flush here if there are already bitmaps
						Flush();
				} else {
					if (GDL_SURFACE_INVALID != surface_info.id) {
						GDLCHECK(gdl_clear_surface ( surface_info.id, &bg_color));
						GDLCHECK(gdl_flip (IsTrueColor() ? ICE_OSD_PLANE : ICE_TXT_PLANE, surface_info.id, GDL_FLIP_SYNC));
						GDLCHECK(gdl_free_surface (surface_info.id));
					} // if
					surface_info.id = GDL_SURFACE_INVALID;
				} // if
			} // if
		} // if
	} // SetActive
	virtual eOsdError SetAreas(const tArea *Areas, int NumAreas) {
		gdl_ret_t ret = GDL_SUCCESS;
		if (GDL_SURFACE_INVALID != surface_info.id)
			GDLCHECK(gdl_clear_surface ( surface_info.id, &bg_color));
		refresh = true;
		eOsdError result = cOsd::SetAreas(Areas, NumAreas);
		ICE_DBG(DBG_FN,"cICEOsd::SetAreas %d %d", NumAreas, (NumAreas > 0) ? Areas[0].bpp: 0);
		return result;
	} // SetAreas
	virtual void Flush(void) {
//		ICE_DBG(DBG_FN,"cICEOsd::Flush");
		if(!Active()) return;
		if(GDL_SURFACE_INVALID == surface_info.id) return; 
		gdl_ret_t ret = GDL_SUCCESS;
		bool render = false;
		if (IsTrueColor()) {
			LOCK_PIXMAPS;
			while (cPixmapMemory *pm = RenderPixmaps()) {
				gdl_rectangle_t rect = {origin:{x:Left()+pm->ViewPort().X(), y:Top()+pm->ViewPort().Y()}, 
				                        width:pm->ViewPort().Width(), height:pm->ViewPort().Height()};
				GDLCHECK(gdl_put( surface_info.id, &rect, pm->ViewPort().Width()*sizeof(tColor), (gdl_void*)pm->Data(), 0));
				delete pm;
				render = true;
			} // while
			if(render) gdl_flip (ICE_OSD_PLANE, surface_info.id, GDL_FLIP_ASYNC);
		} else {
			cBitmap * bitmap;
			for (int i = 0; (bitmap = GetBitmap(i)) != NULL; i++) {
				int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
				if (refresh || bitmap->Dirty(x1, y1, x2, y2)) {
					if (refresh) {
						x2 = bitmap->Width()  - 1;
						y2 = bitmap->Height() - 1;
					} // if
					int numColors;
					const tColor * colors = bitmap->Colors(numColors);
					if (colors) {
						gdl_palette_t palette;
						memset(&palette, 0, sizeof(palette));
						palette.length = numColors;
						for (int c = 0; c < numColors; c++) {
							palette.data[c].a   = (colors[c] >> 24) & 0xFF;
							palette.data[c].r_y = (colors[c] >> 16) & 0xFF; 
							palette.data[c].g_u = (colors[c] >>  8) & 0xFF;
							palette.data[c].b_v = (colors[c]      ) & 0xFF;
						} // for
printf("Set palette %d\n", numColors);
						GDLCHECK(gdl_set_palette(surface_info.id, &palette));
					} // if
					gdl_uint8 *data  = NULL; 
					gdl_uint32 pitch = 0;
					int width = x2 - x1 + 1;
					GDLCHECK(gdl_map_surface(surface_info.id, &data, &pitch));
					if(data && pitch) {
						for (int y = y1; y <= y2; y++)
							memcpy(data + ((Top() + bitmap->Y0() + y) * pitch) + (Left() + bitmap->X0() + x1), bitmap->Data(x1, y), width);
						cache_flush_buffer(data, pitch*OsdHeight());
						GDLCHECK(gdl_unmap_surface(surface_info.id));
						render = true;
					} // if
				} // if
				bitmap->Clean();
			} // for
			if(render) {printf("FLIP\n");gdl_flip (ICE_TXT_PLANE, surface_info.id, GDL_FLIP_ASYNC);}
		} // if
		refresh = false;
	}; // Flush
protected:
	gdl_surface_info_t surface_info;
}; // cICEOsd

#ifdef ICE_REEL_OSD
#include "ice_osd_reel.inc"
#endif /*ICE_REEL_OSD*/

cICEOsdProvider::cICEOsdProvider(cPluginICE *Plugin)
                :plugin(Plugin) {
	ICE_DBG(DBG_FN, "cICEOsdProvider::cICEOsdProvider");
#ifdef ICE_REEL_OSD
	hasReelOsd = cICEOsdReel::Init();
	if(!hasReelOsd) esyslog("cICEOsdProvider: Failed to create reel osd");
#endif /*ICE_REEL_OSD*/
} // cICEOsdProvider::cICEOsdProvider

cICEOsdProvider::~cICEOsdProvider(){
	ICE_DBG(DBG_FN, "cICEOsdProvider::~cICEOsdProvider");
#ifdef ICE_REEL_OSD
	if(hasReelOsd) cICEOsdReel::Done();
#endif /*ICE_REEL_OSD*/
} // cICEOsdProvider::~cICEOsdProvider

cOsd *cICEOsdProvider::CreateOsd(int Left, int Top, uint Level) {
	ICE_DBG(DBG_FN,"cICEOsdProvider::CreateOsd %d/%d %d", Left, Top, Level);
	if(hasReelOsd) return new cICEOsdReel(plugin, Left, Top, Level);
	return new cICEOsd(plugin, Left, Top, Level);
} // cICEOsdProvider::CreateOsd

#ifdef ICE_REEL_OSD
cOsd *cICEOsdProvider::CreateTrueColorOsd(int Left, int Top, uint Level) {
	ICE_DBG(DBG_FN,"cICEOsdProvider::CreateTrueColorOsd %d/%d %d", Left, Top, Level);
#ifdef ICE_REEL_OSD
	if(hasReelOsd) return new cICEOsdReel(plugin, Left, Top, Level);
#endif /*ICE_REEL_OSD*/
	return cOsdProvider::CreateTrueColorOsd(Left, Top, Level);
} // cICEOsdProvider::CreateTrueColorOsd
#endif /*ICE_REEL_OSD*/

uchar *cICEOsdProvider::GetOsdImage(int &SizeRow, int &SizeBpp, int &SizeX, int &SizeY) {
#ifdef ICE_REEL_OSD
	SizeRow = osd_pitch;
	SizeBpp = osd_bpp;
	SizeX   = osd_width;
	SizeY   = osd_height;
	return data;
#endif /*ICE_REEL_OSD*/
	SizeRow = SizeX = SizeY = 0;
	return NULL;
} // cICEOsdProvider::GetOsdImage

