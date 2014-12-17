#ifndef VGS_PROCTOOLS_LOGGER_H
#define VGS_PROCTOOLS_LOGGER_H

#include <list>
#include <string>

namespace proctools
{

	class logger
	{
		friend class logwriter;

	public:
		typedef std::list<logger*> list;

		enum loglevel { level_error, level_warning, level_info, level_debug,
						level_text, level_count };

	protected:
		class manager
		{
			friend class logger;

		private:
			list m_loggers;
			bool m_shutdown;

		protected:
			manager(): m_shutdown(false) {}
				// not constructible except by ptlogger
			~manager();

			void message(loglevel level, const std::string& message);

			manager& operator+=(logger* logger);
			manager& operator-=(logger* logger);
		};

	public:
		virtual ~logger();

		static void error(const std::string& message, bool add_syserror = true);
		static void warning(const std::string& message);
		static void info(const std::string& message);
		static void debug(const std::string& message);
		static void text(const std::string& message);

	protected:
		logger(); // not constructible

		virtual void log(loglevel level, const std::string& message) = 0;

	private:
		static manager m_manager;
	};

	inline logger::manager& logger::manager::operator+=(logger* logger)
	{
		m_loggers.push_back(logger);
		return *this;
	}

	inline logger::manager& logger::manager::operator-=(logger* logger)
	{
		if (!m_shutdown)
			m_loggers.remove(logger);
		return *this;
	}

};

#endif // VGS_PROCTOOLS_LOGGER_H
