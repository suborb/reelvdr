#include "format.h"
#include "shellescape.h"

namespace proctools
{

	using std::string;
	using std::ostream;

	string shell_escape(const string& text, const string& escape)
	{
		string result( text );
		for (string::size_type pos = 0; pos < result.length(); ++pos) {
			string::size_type charPos = escape.find(result[pos]);
			if (charPos != string::npos) {
				char replacement[3] = "\\ ";
				replacement[1] = escape[charPos];
				result.replace(pos++, 1, replacement);
			}
		}
		return result;
	}

	shellescape::shellescape( const string& program ):
			m_cmdline( program )
	{
	}

	shellescape shellescape::operator+( const string& arg )
	{
		static const char* simpleChars( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.,:+%/#~" );

		if ( arg.find_first_not_of( simpleChars ) != string::npos )
			m_cmdline += format( " \"{0}\"" ) % shell_escape( arg );
		else
			m_cmdline += format( " {0}" ) % arg;

		return *this;
	}

	ostream& operator<<( ostream& stream, const shellescape& call )
	{
		return stream << call.m_cmdline;
	}

}
