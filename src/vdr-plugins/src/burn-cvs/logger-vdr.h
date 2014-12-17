/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: logger-vdr.h,v 1.4 2006/09/16 18:33:36 lordjaxom Exp $
 */

#include "proctools/logger.h"
#include "proctools/startable.h"
#include <fstream>

namespace vdr_burn
{

	class logger_vdr: public proctools::logger,
					  public proctools::startable<logger_vdr>
	{
		friend class proctools::startable<logger_vdr>;

	public:
		static void set_logfile(const std::string& filename);
		static void close_logfile();
		static void startup_finished() { m_startup = false; }

	protected:
		logger_vdr() {}

		virtual void log(loglevel level, const std::string& message);

	private:
		static std::ofstream m_logfile;
		static bool m_startup;
	};
}
