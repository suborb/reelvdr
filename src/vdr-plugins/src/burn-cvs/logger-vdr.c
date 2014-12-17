/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: logger-vdr.c,v 1.5 2006/09/16 18:33:36 lordjaxom Exp $
 */

#include "logger-vdr.h"
#include "proctools/format.h"
#include <iostream>
#include <vdr/tools.h>

namespace vdr_burn
{

	using namespace std;

	ofstream logger_vdr::m_logfile;
	bool logger_vdr::m_startup = true;

	void logger_vdr::set_logfile(const std::string& filename)
	{
		m_logfile.close();
		m_logfile.clear();
		m_logfile.open(filename.c_str(), ios::out);
	}

	void logger_vdr::close_logfile()
	{
		m_logfile.close();
	}

	void logger_vdr::log(loglevel level, const std::string& message)
	{
		switch (level)
		{
		case level_error:
			esyslog("ERROR[%s]: %s", PLUGIN_NAME, message.c_str());
			if (m_logfile) m_logfile << "[vdr] ERROR: " << message << endl;
			if (m_startup) cerr << "ERROR[" << PLUGIN_NAME << "]: " << message << endl;
			break;

		case level_warning:
		case level_info:
			isyslog("%s: %s", PLUGIN_NAME, message.c_str());
			if (m_logfile) m_logfile << "[vdr] " << message << endl;
			break;

		case level_debug:
			dsyslog("%s: %s", PLUGIN_NAME, message.c_str());
			if (m_logfile) m_logfile << "[vdr] " << message << endl;
			break;

		default:
			if (m_logfile) m_logfile << message << endl;
			break;
		}
	}

};
