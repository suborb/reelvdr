#include "environment.h"
#include "logger.h"//XXX
#include <algorithm>
#include <cstdlib>

namespace proctools
{

	using namespace std;

	static void environment_setter(const pair<string,string>& variable)
	{
		logger::info(variable.first+"="+variable.second);
		setenv( variable.first.c_str(), variable.second.c_str(), 1 );
	}

	void environment::set()
	{
		for_each( m_variables.begin(), m_variables.end(), environment_setter );
	}

};
