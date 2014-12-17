#ifndef RBMOSD_H
#define RBMOSD_H

class cRBMOsd : public cOsd {
friend class cRBMOsdProvider;
private:
	int scaleToVideoPlane;
	int osd_fd;
	int bpp;
	int width;
	int height;
	int last_numColors;
	unsigned int palette[256];
	unsigned int last_palette[256];
	unsigned char * data; // fb mmaped
protected:
	///< Initializes the OSD with the given coordinates.
	///< By default it is assumed that the full area will be able to display
	///< full 32 bit graphics (ARGB with eight bit for each color and the alpha
	///< value, repectively). However, the actual hardware in use may not be
	///< able to display such a high resolution OSD, so there is an option to
	///< divide the full OSD area into several sub-areas with lower color depths
	///< and individual palettes. The sub-areas need not necessarily cover the
	///< entire OSD area, but only the OSD area actually covered by sub-areas
	///< will be available for drawing.
	///< At least one area must be defined in order to set the actual width and
	///< height of the OSD. Also, the caller must first try to use an area that
	///< consists of only one sub-area that covers the entire drawing space,
	///< and should require only the minimum necessary color depth. This is
	///< because a derived cOsd class may or may not be able to handle more
	///< than one area.
	//   vdr 1.6 starts
	///< There can be any number of cOsd objects at the same time, but only
	///< one of them will be active at any given time. The active OSD is the
	///< one with the lowest value of Level. If there are several cOsd objects
	///< with the same Level, the one that was created first will be active.
	cRBMOsd(int Left, int Top, uint Level, int fb_fd);
	///< Sets this OSD to be the active one.
	///< A derived class must call cOsd::SetActive(On).
	virtual void SetActive(bool On);
	bool Init();
	void Done();
	void Clear();
	void Draw8RGBLine(int x, int y, int w, const tIndex *srcData, const tColor *palette, int num_colors);
	void FlushBitmap(cBitmap &bitmap, bool full);
public:
	///< Shuts down the OSD.
	virtual ~cRBMOsd();
	///< Returns a pointer to the bitmap for the given Area, or NULL if no
	///< such bitmap exists.
	virtual cBitmap *GetBitmap(int Area);
	///< Checks whether the OSD can display the given set of sub-areas.
	///< The return value indicates whether a call to SetAreas() with this
	///< set of areas will succeed. CanHandleAreas() may be called with an
	///< OSD that is already in use with other areas and will not interfere
	///< with the current operation of the OSD.
	///< A derived class must first call the base class CanHandleAreas()
	///< to check the basic conditions, like not overlapping etc.
	virtual eOsdError CanHandleAreas(const tArea *Areas, int NumAreas);
	///< Sets the sub-areas to the given areas.
	///< The return value indicates whether the operation was successful.
	///< If an error is reported, nothing will have changed and the previous
	///< OSD (if any) will still be displayed as before.
	///< If the OSD has been divided into several sub-areas, all areas that
	///< are part of the rectangle that surrounds a given drawing operation
	///< will be drawn into, with the proper offsets.
	///< A new call overwrites any previous settings // vdr 1.6
	virtual eOsdError SetAreas(const tArea *Areas, int NumAreas);
	///< Saves the region defined by the given coordinates for later restoration
	///< through RestoreRegion(). Only one saved region can be active at any
	///< given time.
	virtual void SaveRegion(int x1, int y1, int x2, int y2);
	///< Restores the region previously saved by a call to SaveRegion().
	///< If SaveRegion() has not been called before, nothing will happen.
	virtual void RestoreRegion(void);
	///< Sets the Palette for the given Area (the first area is numbered 0).
	virtual eOsdError SetPalette(const cPalette &Palette, int Area);
	///< Sets the pixel at the given coordinates to the given Color, which is
	///< a full 32 bit ARGB value.
	///< If the OSD area has been divided into separate sub-areas, and the
	///< given coordinates don't fall into any of these sub-areas, no pixel will
	///< be set.
	virtual void DrawPixel(int x, int y, tColor Color);
	///< Sets the pixels in the OSD with the data from the given
	///< Bitmap, putting the upper left corner of the Bitmap at (x, y).
	///< If ColorFg or ColorBg is given, the first palette entry of the Bitmap
	///< will be mapped to ColorBg and the second palette entry will be mapped to
	///< ColorFg (palette indexes are defined so that 0 is the background and
	///< 1 is the foreground color). ReplacePalette controls whether the target
	///< area shall have its palette replaced with the one from Bitmap.
	///< If Overlay is true, any pixel in Bitmap that has color index 0 will
	///< not overwrite the corresponding pixel in the target area.
	virtual void DrawBitmap(int x, int y, const cBitmap &Bitmap, tColor ColorFg = 0, tColor ColorBg = 0, bool ReplacePalette = false, bool Overlay = false);
	virtual void DrawBitmapHor(int x, int y, int w, const cBitmap &Bitmap);
	virtual void DrawBitmapVert(int x, int y, int h, const cBitmap &Bitmap);
	///< Draws the given string at coordinates (x, y) with the given foreground
	///< and background color and font. If Width and Height are given, the text
	///< will be drawn into a rectangle with the given size and the given
	///< Alignment (default is top-left). If ColorBg is clrTransparent, no
	///< background pixels will be drawn, which allows drawing "transparent" text.
	virtual void DrawText(int x, int y, const char *s, tColor ColorFg, tColor ColorBg, const cFont *Font, int Width = 0, int Height = 0, int Alignment = taDefault);
	///< Draws a 32-Bit true color image at (x, y) (upper left).
	virtual void DrawImage(u_int imageId, int x, int y, bool blend, int horRepeat = 1, int vertRepeat = 1);
	///< Draws a filled rectangle defined by the upper left (x1, y1) and lower right
	///< (x2, y2) corners with the given Color.
	virtual void DrawRectangle(int x1, int y1, int x2, int y2, tColor Color);
	///< Draws a filled rectangle defined by the upper left (x1, y1) and lower right
	///< (x2, y2) corners with the given Color.
	///< //TB: The Alpha-Value is incremented all alphaGradStepH pixels by the value AlphaGradH in every line,
	///< and all alphaGradStepV pixels by the value alphaGradV in every row.
	virtual void DrawRectangle(int x1, int y1, int x2, int y2, tColor Color, int alphaGradH, int alphaGradV, int alphaGradStepH, int alphaGradStepV);
	///< Draws a filled ellipse defined by the upper left (x1, y1) and lower right
	///< (x2, y2) corners with the given Color. Quadrants controls which parts of
	///< the ellipse are actually drawn:
	///< 0       draws the entire ellipse
	///< 1..4    draws only the first, second, third or fourth quadrant, respectively
	///< 5..8    draws the right, top, left or bottom half, respectively
	///< -1..-8  draws the inverted part of the given quadrant(s)
	///< If Quadrants is not 0, the coordinates are those of the actual area, not
	///< the full circle!
	virtual void DrawEllipse(int x1, int y1, int x2, int y2, tColor Color, int Quadrants = 0); 
	///< Draws a "slope" into the rectangle defined by the upper left (x1, y1) and
	///< lower right (x2, y2) corners with the given Color. Type controls the
	///< direction of the slope and which side of it will be drawn:
	///< 0: horizontal, rising,  lower
	///< 1: horizontal, rising,  upper
	///< 2: horizontal, falling, lower
	///< 3: horizontal, falling, upper
	///< 4: vertical,   rising,  lower
	///< 5: vertical,   rising,  upper
	///< 6: vertical,   falling, lower
	///< 7: vertical,   falling, upper
	virtual void DrawSlope(int x1, int y1, int x2, int y2, tColor Color, int Type); 
	///< Actually commits all data to the OSD hardware.
	virtual void Flush(void);
	///< Set the path of an image to be user later with DrawImage().
	virtual void SetImagePath(u_int imageId, char const *path);
}; // cRBMOsd

#endif

