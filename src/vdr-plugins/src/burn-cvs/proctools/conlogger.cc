#include "conlogger.h"
#include <iostream>

using std::string;
using std::cerr;
using std::endl;

namespace proctools 
{

	const char* conlogger::m_levels[level_count] = 
		{ " error ", "warning", " info  ", " debug ", " text  " };

	void conlogger::log(loglevel level, const string& message)
	{
		cerr << "[" << m_levels[level] << "] " << message << endl;
	}

};
