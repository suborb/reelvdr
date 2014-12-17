#ifndef VGS_PROCTOOLS_PROCESS_H
#define VGS_PROCTOOLS_PROCESS_H

#include "environment.h"
#include "functions.h"
#include <string>
#include <list>
#include <clocale>
#include <sys/types.h>

namespace proctools
{

	class process;
	class chain;

	class process
	{
	public:
		typedef std::list<process*> list;
		enum status_value { none, ok, error, crash };

		process(const std::string& name);
		virtual ~process();

		void set_parent(chain* parent) { m_parent = parent; }
		void set_workdir(const std::string& workdir) { m_workdir = workdir; }
		bool start_if_idle();
		bool read_pipe();
		bool wait_for_exit(bool wait = true);

		template<typename From>
		void put_environment(const std::string& name, const From& value);
		void set_environment() { m_environment.set(); }

		const std::string& name() const { return m_name; }
		int pipe() const { return m_pipe; }

		status_value return_status() const { return m_returnStatus; }
		int return_code() const { return m_returnCode; }

	protected:
		virtual std::string info(pid_t pid) = 0;
		virtual int run() = 0;

	private:
		chain*       m_parent;
		std::string  m_name;
		std::string  m_workdir;
		std::string  m_linebuf;
		environment  m_environment;
		pid_t        m_pid;
		int          m_pipe;
		status_value m_returnStatus;
		int          m_returnCode;
	};

	template<typename From>
	void process::put_environment(const std::string& name, const From& value)
	{
		std::setlocale(LC_ALL, "C");
		m_environment.put(name, convert<std::string>(value));
		std::setlocale(LC_ALL, "");
	}

	template<>
	void process::put_environment(const std::string& name, const std::string& value);

}

#endif // VGS_PROCTOOLS_PROCESS_H
