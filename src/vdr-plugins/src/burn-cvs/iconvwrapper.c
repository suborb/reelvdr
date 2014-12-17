/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: iconvwrapper.c,v 1.3 2006/09/16 18:33:36 lordjaxom Exp $
 */

#include <cstring>
#include <alloca.h>
#include <cstdlib>
#include "iconvwrapper.h"
#include <cerrno>
#include <sstream>

namespace iconvwrapper
{

	using namespace std;

	iconv::iconv( const string& from, const string& to ):
			m_descriptor( iconv_open( to.c_str(), from.c_str() ) ),
			m_valid( m_descriptor != iconv_t( -1 ) )
	{
	}

	iconv::~iconv()
	{
		if ( m_descriptor != iconv_t( -1 ) )
			iconv_close( m_descriptor );
	}

	void iconv::clear()
	{
		if ( m_descriptor == iconv_t( -1 ) )
			return;

		::iconv( m_descriptor, 0, 0, 0, 0 );
		m_valid = true;
	}

	std::string iconv::operator()( const string& text )
	{
		size_t inbytes = text.length();
		char* inbuf = strdup( text.c_str() );
		char* inpos = inbuf;
		char* outbuf = (char*) malloc( inbytes );
		ostringstream result;

		while ( inbytes > 0 ) {
			char* outpos = outbuf;
			size_t outbytes = text.length();

			if ( ::iconv( m_descriptor, &inpos, &inbytes, &outpos, &outbytes ) == (size_t) -1 && errno != E2BIG ) {
				m_valid = false;
				break;
			}

			result.write( outbuf, outpos - outbuf );
		}

		free(outbuf);
		free(inbuf);
		return result.str();
	}

}
