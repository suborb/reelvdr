#include <iostream>
#include <sstream>
#include <string>

using namespace std;

	string shell_escape(const string& text, const string& escape = "\\\" " )
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

	class shell_program
	{
		friend ostream& operator<<( ostream& stream, const shell_program& call );

	public:
		shell_program( const std::string& program );
		
		shell_program operator+( const std::string& arg );

		std::string str( void ) const { return m_cmdline; }
		
	private:
		std::string m_cmdline;
	};

	shell_program::shell_program( const string& program ):
			m_cmdline( program )
	{
	}

	shell_program shell_program::operator+( const string& arg )
	{
		static const char* simpleChars( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_" );

		ostringstream cmdline;
		cmdline << m_cmdline;

		if ( arg.find_first_not_of( simpleChars ) != string::npos )
			cmdline << " \"" << shell_escape( arg ) << "\"";
		else
			cmdline << " " << arg;

		m_cmdline = cmdline.str();
		return *this;
	}

	ostream& operator<<( ostream& stream, const shell_program& call )
	{
		return stream << call.m_cmdline;
	}


int main(void)
{
	cout << ( shell_program("rm") + "-rf" + "bla hallo" ) << endl;

}
