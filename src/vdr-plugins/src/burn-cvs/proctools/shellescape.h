#ifndef VGS_PROCTOOLS_SHELLPROGRAM_H
#define VGS_PROCTOOLS_SHELLPROGRAM_H

#include <iostream>
#include <string>

namespace proctools
{

	std::string shell_escape( const std::string& text, const std::string& escape = "\"\\$" );

	class shellescape
	{
		friend std::ostream& operator<<( std::ostream& stream, const shellescape& call );
		friend const std::string& str( const shellescape& value );

	public:
		shellescape( const std::string& program );

		shellescape operator+( const std::string& arg );

	private:
		std::string m_cmdline;
	};

	inline
	const std::string& str( const shellescape& value )
	{
		return value.m_cmdline;
	}

}

#endif // VGS_PROCTOOLS_SHELLPROGRAM_H
