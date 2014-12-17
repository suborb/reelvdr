#include "chain.h"
#include "shellprocess.h"
#include "logger.h"
#include "poller.h"
#include "format.h"
#include "functions.h"
#include <algorithm>
#include <functional>
#include <string>
#include <iterator>
#include <iostream>
#include <memory>
#include <sys/stat.h>
#include <sys/types.h>

namespace proctools
{

	using namespace std;

	class chainreader: public unary_function<process*, bool>
	{
	public:
		chainreader(poller& poller):
				m_poller(poller) {}
		bool operator()(process* process) const;

	private:
		poller& m_poller;
	};

	bool chainreader::operator()(process* process) const
	{
		if (!m_poller.is_set(process))
			return true;
		return process->read_pipe();
	}

	chain::chain(const std::string& name):
			m_name(name),
			m_active(false),
			m_returnStatus(process::none),
			m_returnCode(0),
			m_canceled(false)
	{
	}

	void chain::execute(process& proc)
	{
		if (!proc.start_if_idle())
			return;

		while (true) {
			poller poll;
			poll.add(&proc);
			if (!poll.poll(1000))
				continue;

			chainreader reader(poll);
			if (!reader(&proc))
				break;
		}
	}

	void chain::execute(const shellescape& cmd)
	{
		shellprocess proc( "temp", cmd );
		logger::info("executing '" + str( cmd ) + "'");
		execute(proc);
	}

	bool chain::run()
	{
		m_active = true;

		if (!initialize()) {
			m_returnStatus = process::error;
			return false;
		}

		bool result = true;
		while (m_active) {
			if (find_if(m_processes.begin(), m_processes.end(),
						not1(mem_fun(&process::start_if_idle)))
					!= m_processes.end()) {
				result = m_active = false;
				break;
			}

			poller poll;
			for_each(m_processes.begin(), m_processes.end(),
					 bind1st(mem_fun(&poller::add), &poll));

			if (poll.poll(250)) {
				process::list finished;

				remove_copy_if(m_processes.begin(), m_processes.end(),
							   back_inserter(finished), chainreader(poll));

				if (find_if(finished.begin(), finished.end(),
							not1(bind1st(mem_fun(&chain::del_process), this)))
							!= finished.end()) {
					result = m_active = false;
					break;
				}
			}

			if (m_processes.size() == 0)
				m_active = false;
		}

		for_each(m_processes.begin(), m_processes.end(), &deleter<process>);
		all_done();

		return result;
	}

	void chain::add_process(process* proc)
	{
		proc->set_parent(this);
		m_processes.push_back(proc);
	}

	bool chain::del_process(process* proc)
	{
		logger::info(format("process \"{0}\" exited") % proc->name());
		m_processes.remove(proc);
		auto_ptr<process> dtemp( proc ); // auto-delete

		if (proc->return_status() == process::ok ||
				proc->return_status() == process::error) {
			bool result = finished(proc);
			// bad process ends chain, as does the last process
			if (m_returnStatus == process::none &&
					(!result || m_processes.size() == 0)) {
				m_returnStatus = result && proc->return_status() == process::ok
								 ? process::ok : process::error;
				m_returnCode = proc->return_code();
			}
			return result;
		}

		m_returnStatus = proc->return_status();
		return false;
	}

	void chain::make_dir(const string& path)
	{
		mkdir(path.c_str(), ACCESSPERMS);
	}

	void chain::make_fifo(const string& path)
	{
		mkfifo(path.c_str(), DEFFILEMODE);
	}

};
