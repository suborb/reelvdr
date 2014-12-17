/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: gdwrapper.h,v 1.4 2006/09/16 18:33:36 lordjaxom Exp $
 */

#ifndef VDR_BURN_GDWRAPPER_H
#define VDR_BURN_GDWRAPPER_H

#include <string>
#include <gd.h>

namespace gdwrapper
{
	// --- font_spec ----------------------------------------------------------

	struct font_spec
	{
		std::string face;
		double size;

		font_spec( const std::string& face_, double size_ ): face( face_ ), size( size_ ) {}
	};

	// --- point --------------------------------------------------------------

	struct point
	{
		int x;
		int y;

		point( int x_, int y_ ): x( x_ ), y( y_ ) {}
	};

	// --- rectangle ----------------------------------------------------------

	struct rectangle
	{
		int x;
		int y;
		int width;
		int height;

		rectangle( int x_, int y_, int width_, int height_ ): x( x_ ), y( y_ ), width( width_ ), height( height_ ) {}
	};

	// --- color --------------------------------------------------------------

	struct color
	{
		int red;
		int green;
		int blue;
		int alpha;

		color( int red_, int green_, int blue_, int alpha_ = 0 ): red( red_ ), green( green_ ), blue( blue_ ), alpha( alpha_ ) {}
	};

	// --- text_spec ----------------------------------------------------------

	struct text_spec
	{
		rectangle boundaries;
		font_spec font;
		color forecolor;
		int align;

		text_spec( const rectangle& boundaries_, const font_spec& font_, const color& forecolor_, int align_ = 0 ):
				boundaries( boundaries_ ), font( font_ ), forecolor( forecolor_ ), align( align_ ) {}
	};

	// --- image --------------------------------------------------------------

	class image
	{
	public:
		image( int width, int height, bool alphaBlending = false );
		image( const std::string& filename, bool alphaBlending = false );
		~image();

		void draw_image( const point& position, const image& image_ );
		void fill_rectangle( const rectangle& rectangle, const color& color );
		int draw_text( const text_spec& region, const std::string& text, int firstLine = 0 );

		void save( const std::string& filename );

	protected:
		static std::string wrap_text( int width, int& lineHeight, const std::string& text, const font_spec& font );

	private:
		gdImagePtr m_image;
	};

	// --- setup --------------------------------------------------------------

	class setup
	{
	public:
		static void set_font_path( const std::string& path );
		static const std::string& get_font_path() { return instance().m_fontPath; }

	private:
		std::string m_fontPath;

		static setup& instance();
	};
}

#endif // VDR_BURN_GDWRAPPER_H
