/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: iconvwrapper.h,v 1.3 2006/09/16 18:33:36 lordjaxom Exp $
 */

#ifndef VDR_BURN_ICONVWRAPPER_H
#define VDR_BURN_ICONVWRAPPER_H

#include <string>
#include <iconv.h>

namespace iconvwrapper
{

	class iconv
	{
	public:
		iconv( const std::string& from, const std::string& to );
		~iconv();

		void clear();

		std::string operator()( const std::string& text );

		operator bool() const { return m_valid; }

	private:
		iconv( const iconv& );
		iconv& operator=( const iconv& );

		iconv_t m_descriptor;
		bool m_valid;
	};

}

#endif // VDR_BURN_ICONVWRAPPER_H
