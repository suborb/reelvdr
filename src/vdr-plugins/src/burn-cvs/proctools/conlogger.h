#ifndef VGS_PROCTOOLS_CONLOGGER_H
#define VGS_PROCTOOLS_CONLOGGER_H

#include "logger.h"
#include "startable.h"

namespace proctools
{

	class conlogger: public logger, public startable<conlogger>
	{
		friend class startable<conlogger>;

	protected:
		conlogger() {}

		virtual void log(loglevel level, const std::string& message);

	private:
		static const char* m_levels[level_count];
	};

};

#endif // VGS_PROCTOOLS_CONLOGGER_H
