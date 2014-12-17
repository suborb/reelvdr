/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: manager.h,v 1.9 2006/09/16 18:33:36 lordjaxom Exp $
 */

#ifndef VDR_BURN_MANAGER_H
#define VDR_BURN_MANAGER_H

#include "jobs.h"
#include "proctools/startable.h"
#include <memory>
#include <vdr/thread.h>

namespace vdr_burn
{

	class manager: public cThread,
				   public proctools::startable<manager>
	{
	public:
		class lock
		{
		public:
			lock();
			~lock();

		private:
			cMutex* m_mutex;
		};

		manager();
		virtual ~manager();

		static bool get_is_busy();

		// the following methods may only be called and their results only be
		// used while manager is locked
		static void start_pending();
		static void replace_pending(job* job_);
		static void erase_job(job* job_);
		static std::string get_status_message();

		static job& get_pending() { return *m_instance->m_pending; }
		static job* get_active() { return m_instance->m_active; }
		static const job_queue& get_queued();
		static const job_queue& get_erroneous();
		static const job_queue& get_canceled();

		static bool is_recording_queued( const std::string& filename );

	protected:
		virtual void Action();

		void start_if_idle();
		void check_rotate();

	private:
		cMutex    m_mutex;
		cCondVar  m_condition;

		std::auto_ptr<job> m_pending;
		job*      m_active;
		job_queue m_queued;
		job_queue m_erroneous;
		job_queue m_canceled;
	};

	inline
	const job_queue& manager::get_queued()
	{
		return m_instance->m_queued;
	}

	inline
	const job_queue& manager::get_erroneous()
	{
		return m_instance->m_erroneous;
	}

	inline
	const job_queue& manager::get_canceled()
	{
		return m_instance->m_canceled;
	}

}

#endif // VDR_BURN_MANAGER_H
