/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: chain-vdr.h,v 1.8 2006/09/16 18:33:36 lordjaxom Exp $
 */

#ifndef VDR_BURN_CHAIN_VDR_H
#define VDR_BURN_CHAIN_VDR_H

#include "common.h"
#include "jobs.h"
#include "proctools/chain.h"
#include <vdr/thread.h>

namespace vdr_burn
{

	class chain_vdr: public proctools::chain,
					 public cThread
	{
	public:
		// factory method
		static chain_vdr* create_chain(job& job);

		virtual ~chain_vdr();

		// starts and stops the thread controlling the processes
		void start() { cThread::Start(); }
		void stop();

		// monitoring and member access
		const path_pair& get_paths() const { return m_paths; }
		bool get_is_active() { return Active(); }
		int get_progress() const { return m_progress; }
		virtual bool get_is_burning(int& progress) const { progress = m_burnProgress; return m_burning; }

		// convenience functions
		//std::string get_log_path() const { return m_paths.data + "/dvd.log"; }
		std::string get_log_path() const { return "/var/log/dvd.log"; }

		// TODO: move rendering into chain
		using proctools::chain::execute;
		//void execute( proctools::process& proc ) { proctools::chain::execute( proc ); }

	protected:
		chain_vdr(const std::string& name, job& job_);
		time_t start_time;
                int       m_progress;

		virtual void all_done();

		virtual void Action();

		void add_process(proctools::process* proc);
		std::string create_temp_path( const std::string& pathPrefix );

		void set_progress(int percent) { m_progress = std::min(percent, 90); }
		void set_is_burning() { m_burning = true; }
		void set_burn_progress(int percent) { m_burnProgress = std::min(percent, 90); }

		job& get_job() { return m_job; }
		recording_list& get_recordings() { return m_job.get_recordings(); }

	private:
		job&      m_job;
		path_pair m_paths;
		bool      m_burning;
		int       m_burnProgress;
		bool      m_canceled;
	};

};

#endif // VDR_BURN_CHAIN_VDR_H
