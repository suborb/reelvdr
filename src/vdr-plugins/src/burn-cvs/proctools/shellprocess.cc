#include "shellprocess.h"
#include "format.h"
#include <unistd.h>

using std::string;

namespace proctools
{

	shellprocess::shellprocess(const string& name, const shellescape& command):
			process( name ),
			m_command( str( command ) )
	{
	}

	string shellprocess::info(pid_t pid)
	{
		return format("sh -c '{0}' (pid = {1})") % m_command % pid;
	}

	int shellprocess::run()
	{
		execl("/bin/sh", "sh", "-c", m_command.c_str(), NULL);
		return(127);
	}

};
