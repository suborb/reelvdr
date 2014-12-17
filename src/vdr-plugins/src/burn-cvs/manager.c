/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: manager.c,v 1.12 2006/09/16 18:33:36 lordjaxom Exp $
 */

#include "chain-vdr.h"
#include "manager.h"
#include "proctools/logger.h"
#include "proctools/format.h"
#include "proctools/functions.h"
#include <stdexcept>
#include <algorithm>
#include <vdr/skins.h>

namespace vdr_burn
{

	using namespace std;
	using proctools::logger;
	using proctools::process;
	using proctools::deleter;
	using proctools::format;

	manager::lock::lock():
		m_mutex(&manager::m_instance->m_mutex)
	{
		m_mutex->Lock();
	}

	manager::lock::~lock()
	{
		m_mutex->Unlock();
	}

	manager::manager():
			cThread("burn-chain manager"),
			m_pending( new job ),
			m_active( 0 )
	{
		Start();
	}

	manager::~manager()
	{
		Cancel(10);
	}

	bool manager::get_is_busy()
	{
		lock man_lock;
		return m_instance->m_active != 0 || m_instance->m_queued.size() > 0;
	}

	void manager::start_pending()
	{
		lock man_lock;
		m_instance->m_queued.push_back(m_instance->m_pending.release());
		m_instance->m_pending.reset( new job );
		m_instance->m_condition.Broadcast();
	}

	void manager::replace_pending(job* job_)
	{
		lock man_lock;
		erase_job(job_);
		job_->reset();

		m_instance->m_pending.reset( job_ );
	}

	void manager::erase_job(job* job_)
	{
		job_queue::iterator item;
		if ((item = find(m_instance->m_queued.begin(),
				 m_instance->m_queued.end(), job_))
				!= m_instance->m_queued.end())
			m_instance->m_queued.erase(item);
		else if ((item = find(m_instance->m_erroneous.begin(),
					  m_instance->m_erroneous.end(), job_))
				!= m_instance->m_erroneous.end())
			m_instance->m_erroneous.erase(item);
		else if ((item = find(m_instance->m_canceled.begin(),
					  m_instance->m_canceled.end(), job_))
				!= m_instance->m_canceled.end())
			m_instance->m_canceled.erase(item);
	}

	string manager::get_status_message()
	{
		lock myLock;

		if (m_instance->m_active != 0)
			return format("{0} - {1}%") % (m_instance->m_queued.size() + 1) % m_instance->m_active->get_progress();
		return "";
	}

	void manager::start_if_idle()
	{
		if (m_active == 0 && m_queued.size() > 0) {
			logger::debug("manager is starting first job");
			m_active = m_queued.front();
			m_queued.pop_front();
			m_active->start();
		}
	}

	void manager::check_rotate()
	{
		if (m_active != 0 && !m_active->get_is_active()) {
			logger::debug("manager picked up finished job, removing it");

			// clear message queue
			Skins.QueueMessage(mtInfo, 0, 0, 0);

			if (m_active->get_is_canceled())
				m_canceled.push_back(m_active);
			else if (m_active->get_return_status() == process::error) {
				m_erroneous.push_back(m_active);
				Skins.QueueMessage(mtError, tr("A job failed!"), 10, 0);
			}
			else {
				delete m_active;
				Skins.QueueMessage(mtInfo, tr("A job was finished successfully."), 10, 0);
			}
			m_active = 0;
		}
	}

	bool manager::is_recording_queued( const std::string& filename )
	{
		manager::lock mlock;

		if ( ( m_instance->m_pending.get() != 0 && m_instance->m_pending->get_by_filename( filename ) != m_instance->m_pending->get_recordings().end() ) ||
				( m_instance->m_active != 0 && m_instance->m_active->get_by_filename( filename ) != m_instance->m_active->get_recordings().end() ) )
			return true;

		for ( job_queue::iterator it = m_instance->m_queued.begin(); it != m_instance->m_queued.end(); ++it) {
			if ( (*it)->get_by_filename( filename ) != (*it)->get_recordings().end() )
				return true;
		}
		return false;
	}

	void manager::Action()
	{
		m_mutex.Lock();
		SetPriority(19);
		while (Running()) {
			m_condition.TimedWait(m_mutex, 1000);

			check_rotate();
			start_if_idle();
		}

		delete m_active;
		for_each(m_queued.begin(), m_queued.end(), &deleter<job>);
		for_each(m_erroneous.begin(), m_erroneous.end(), &deleter<job>);
		for_each(m_canceled.begin(), m_canceled.end(), &deleter<job>);

		m_mutex.Unlock();
	}

}
