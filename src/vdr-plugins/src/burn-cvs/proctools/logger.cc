#include "logger.h"
#include "functions.h"
#include "format.h"
#include <functional>
#include <iterator>
#include <sstream>
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>

namespace proctools
{

	using namespace std;

	logger::manager logger::m_manager;

	class logwriter
	{
	public:
		logwriter(logger::loglevel level, const string& message):
				m_level(level), m_message(message) {}
		void operator()(logger* logger) { logger->log(m_level, m_message); }

	private:
		logger::loglevel m_level;
		const string& m_message;
	};

	logger::manager::~manager()
	{
		m_shutdown = true;
		for_each(m_loggers.begin(), m_loggers.end(), &deleter<logger>);
	}

	void logger::manager::message(loglevel level, const string& message)
	{
		for_each(m_loggers.begin(), m_loggers.end(),
				 logwriter(level, message));
	}

	logger::logger()
	{
		m_manager += this;
	}

	logger::~logger()
	{
		m_manager -= this;
	}

	void logger::error(const string& message, bool add_syserror)
	{
		if (!add_syserror)
			m_manager.message(level_error, message);
		else
			m_manager.message(level_error,
							  format("{0}: {1}") % message % strerror(errno));
	}

	void logger::warning(const string& message)
	{
		m_manager.message(level_warning, message);
	}

	void logger::info(const string& message)
	{
		m_manager.message(level_info, message);
	}

	void logger::debug(const string& message)
	{
		m_manager.message(level_debug, message);
	}

	void logger::text(const string& message)
	{
		m_manager.message(level_text, message);
	}

};
