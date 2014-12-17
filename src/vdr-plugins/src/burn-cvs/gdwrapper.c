/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: gdwrapper.c,v 1.10 2006/09/16 18:33:36 lordjaxom Exp $
 */

#include "common.h"
#include "gdwrapper.h"
#include "iconvwrapper.h"
#include "proctools/format.h"
#include "proctools/logger.h"
#include <vdr/i18n.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace gdwrapper
{

	using namespace std;
	using proctools::format;
	using proctools::logger;

	inline char mytolower( char c ) { return tolower( c ); }

	// --- image --------------------------------------------------------------

	image::image( const string& filename, bool alphaBlending ):
			m_image( 0 )
	{
		if ( filename.length() < 4 ) {
			logger::error( format( "couldn't open image {0} for reading: filename too short to determine format" )
						   % filename, false );
			throw std::string( tr("Couldn't open image file!") );
		}

		FILE* fp = fopen( filename.c_str(), "rb" );
		if (fp == 0) {
			logger::error( format( "couldn't open image {0} for reading" ) % filename );
			throw std::string( tr("Couldn't open image file!") );
		}

		string extension;
		extension.resize( 4 );
		transform( filename.begin() + filename.length() - 4, filename.end(), extension.begin(), mytolower );
		if ( extension == ".png" )
			m_image = gdImageCreateFromPng( fp );
		else if ( extension == ".jpg" || extension == ".jpeg" )
			m_image = gdImageCreateFromJpeg( fp );
		else {
			logger::error( format( "couldn't read image {0}: unsupported file format" ) % filename, false );
			throw std::string( tr("Couldn't open image file!") );
		}
		fclose( fp );

		if (m_image == 0) {
			logger::error( format( "couldn't read image {0}: possibly corrupt" ) % filename, false );
			throw std::string( tr("Couldn't open image file!") );
		}
		gdImageSaveAlpha( m_image, 1 );
		gdImageAlphaBlending( m_image, alphaBlending ? 1 : 0 );
	}

	image::image( int width, int height, bool alphaBlending ):
			m_image( gdImageCreateTrueColor( width, height ) )
	{
		if (m_image == 0) {
			logger::error( "couldn't create new image: possibly out of memory" );
			throw std::string( tr("Couldn't create new image!") );
		}
		gdImageSaveAlpha( m_image, 1 );
		gdImageAlphaBlending( m_image, alphaBlending ? 1 : 0 );
	}

	image::~image()
	{
		if (m_image != 0)
			gdImageDestroy( m_image );
	}

	void image::draw_image( const point& position, const image& image_ )
	{
		gdImageCopy( m_image, image_.m_image, position.x, position.y, 0, 0, image_.m_image->sx, image_.m_image->sy );
	}

	string image::wrap_text( int width, int& lineHeight, const string& text, const font_spec& font )
	{
		static const string delimiters( "-.,:;!?_" );

		const string fontPath( setup::get_font_path() + "/" + font.face + ".ttf" );
		string result;
		string::const_iterator lastWord = text.begin();
		string::const_iterator thisLine = text.begin();
		lineHeight = 0;

		for ( string::const_iterator pos = text.begin(); pos != text.end(); ++pos ) {
			bool isSpace = isspace( *pos );

			if ( !isSpace && delimiters.find( *pos ) == string::npos && pos + 1 < text.end() )
				continue;

			bool isNewline = ( *pos == '\n' );

#warning TODO: check if there is a whole word of delimiters here

			string sample( thisLine, pos );
			if ( !isSpace ) {
				sample.append( pos, pos + 1 );
//				if ( pos + 1 < text.end() )
//					++pos;
			}

			int bounds[8];
			const char* error = gdImageStringFT( 0, bounds, 0, const_cast<char*>( fontPath.c_str() ), font.size, 0, 0, 0,
												 const_cast<char*>( sample.c_str() ) );
			if (error != 0) {
				string message( error );
				if ( message == "Could not find/open font" )
					message += " " + fontPath;
				logger::error( format( "Couldn't render text: {0}" ) % message, false );
				throw std::string( tr("Couldn't render menu images!") );
			}

			int lineWidth = bounds[2] - bounds[0];
			int currentLineHeight = bounds[3] - bounds[5];
			if ( sample != "" && currentLineHeight > lineHeight )
				lineHeight = currentLineHeight;
			if ( lineWidth > width || isNewline ) {
				if ( isNewline || lastWord == thisLine )
					lastWord = pos;

				result.append( thisLine, lastWord );
				if ( pos + 1 < text.end() ) {
					result.append( "\n" );
//					if ( isNewline )
//						++pos;
				}
				thisLine = lastWord + 1;
			}

			lastWord = pos;
		}

		if ( thisLine < text.end() )
			result.append( thisLine, text.end() );

		return result;

//		string result( text.substr( 0, text.find_last_not_of( '\n' ) ) );
//		string::iterator blank = result.end();
//		string::iterator delim = result.end();
//		int lines = 1;
//		int totalwidth = 0;
//		lineHeight = 0;
//
//		char* c_font = strdup( font_.filename.c_str() );
//
//		for (string::iterator pos = result.begin(); pos != result.end(); ) {
//			if (*pos == '\n') {
//				++lines; ++pos;
//				totalwidth = 0;
//				blank = delim = result.end();
//				continue;
//			} else if (isspace( *pos ))
//				blank = pos;
//
//			char buf[2] = { *pos, '\0' };
//			int bounds[8];
//			gdImageStringFT( 0, bounds, 0, c_font, font_.size, 0, 0, 0, buf );
//
//			int charwidth = bounds[2] - bounds[0] + 2; // char spacing
//			lineHeight = bounds[3] - bounds[1] + 1;
//			if (totalwidth + charwidth > width) {
//				if (blank != result.end()) {
//					*blank = '\n';
//					pos = blank;
//					continue;
//				}
//
//				if (delim != result.end())
//					pos = delim + 1;
//
//				result.insert( pos, '\n' );
//				continue;
//			}
//
//			totalwidth += charwidth;
//
//			if (delimiters.find( *pos ) != string::npos) {
//				delim = pos;
//				blank = result.end();
//			}
//
//			++pos;
//		}
//
//		free( c_font );
//
//		if (lineHeight > 0)
//			++lines;
//		return result;
	}

	int image::draw_text( const text_spec& region, const std::string& text, int firstLine )
	{
		if ( text.empty() )
			return -1;

		const string fontPath( setup::get_font_path() + "/" + region.font.face + ".ttf" );
		int lineh;
		string wrappedText = wrap_text( region.boundaries.width, lineh, text, region.font );

		int y_ = region.boundaries.y;
		int lines = 0, width_, height_;
		int white = gdImageColorResolveAlpha(m_image, 255, 255, 255, 0);
		int bounds[8];

		if (lineh == 0)
			return -1;

		string::size_type pos = 0;
		string::size_type next;
		while ( y_ + lineh < region.boundaries.y + region.boundaries.height - 1 &&
				( next = wrappedText.find( '\n', pos ) ) != string::npos ) {
			string line( wrappedText.substr( pos, next - pos ) );

			if (lines++ >= firstLine) {
				gdImageStringFT( 0, bounds, 0, const_cast<char*>( fontPath.c_str() ),
								 region.font.size, 0, 0, 0, const_cast<char*>( line.c_str() ) );
				width_ = bounds[2] - bounds[0] + 1;
				height_ = bounds[3] - bounds[1] + 1;
				if ( region.align == 0 )
					gdImageStringFT( m_image, bounds, white, const_cast<char*>( fontPath.c_str() ),
									 region.font.size, 0, region.boundaries.x, y_ + lineh,
									 const_cast<char*>( line.c_str() ) );
				else if (region.align == 2)
					gdImageStringFT( m_image, bounds, white, const_cast<char*>( fontPath.c_str() ),
									 region.font.size, 0, region.boundaries.x + region.boundaries.width - width_ - 1,
									 y_ + lineh, const_cast<char*>( line.c_str() ) );
				y_ += lineh;
			}

			pos = next + 1;
			if ( pos == wrappedText.length() ) {
				lines = -1;
				break;
			}
		}

		if ( pos < wrappedText.length() && y_ + lineh < region.boundaries.y + region.boundaries.height) {
			string rest( wrappedText.substr( pos ) );
			gdImageStringFT( 0, bounds, 0, const_cast<char*>( fontPath.c_str() ), region.font.size, 0, 0, 0,
							 const_cast<char*>( rest.c_str() ) );
			width_ = bounds[2] - bounds[0] + 1;
			height_ = bounds[3] - bounds[1] + 1;
			if (region.align == 0)
				gdImageStringFT( m_image, bounds, white, const_cast<char*>( fontPath.c_str() ),
								 region.font.size, 0, region.boundaries.x, y_ + lineh, const_cast<char*>( rest.c_str() ) );
			else if (region.align == 2)
				gdImageStringFT( m_image, bounds, white, const_cast<char*>( fontPath.c_str() ),
								 region.font.size, 0, region.boundaries.x + region.boundaries.width - width_ - 1, y_ + lineh,
								 const_cast<char*>( rest.c_str() ) );
			lines = -1;
		}

		return lines;
	}

	void image::fill_rectangle( const rectangle& rectangle, const color& color )
	{
		int bincolor = gdImageColorResolveAlpha( m_image, color.red, color.green, color.blue, color.alpha );
		gdImageFilledRectangle( m_image, rectangle.x, rectangle.y,
								rectangle.x + rectangle.width - 1, rectangle.y + rectangle.height - 1, bincolor );
	}

	void image::save( const std::string& filename )
	{
		FILE* fp = fopen( filename.c_str(), "wb" );
		if (fp == 0) {
			logger::error( format( "Couldn't open image {0} for writing" ) % filename );
			throw std::string( tr("Couldn't save image file!") );
		}

		gdImagePng( m_image, fp );
		fclose( fp );
	}

	// --- setup --------------------------------------------------------------

	setup& setup::instance()
	{
		static setup instance_;
		return instance_;
	}

	void setup::set_font_path( const std::string& path )
	{
		instance().m_fontPath = path;
	}

}
