#ifndef VGS_PROCTOOLS_PROCCHAIN_H
#define VGS_PROCTOOLS_PROCCHAIN_H

#include "process.h"
#include "shellescape.h"

namespace proctools
{

	class chain
	{
		friend class process;

	public:
		chain(const std::string& name);
		virtual ~chain() {}

		bool run();
		void stop() { m_active = false; m_canceled = true; }

		process::status_value return_status() const;
		bool is_canceled() const { return m_canceled; }

	protected:
		virtual bool initialize() = 0;
		virtual void process_line(const std::string& message) = 0;
		virtual bool finished(const process* proc) = 0;
		virtual void all_done() = 0;

		void add_process(process* proc);
		bool del_process(process* proc);

		// tools
		void execute(process& proc);
		void execute(const shellescape& cmd);
		void make_dir(const std::string& path);
		void make_fifo(const std::string& fifo);

	private:
		std::string           m_name;
		process::list         m_processes;
		bool                  m_active;
		process::status_value m_returnStatus;
		int                   m_returnCode;
		bool                  m_canceled;
	};

	inline
	process::status_value chain::return_status() const
	{
		return m_returnStatus;
	}

}

#endif // VGS_PROCTOOLS_PROCCHAIN_H
