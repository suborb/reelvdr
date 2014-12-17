/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: chain-archive.h,v 1.4 2006/09/16 18:33:36 lordjaxom Exp $
 */

#ifndef VDR_BURN_CHAIN_ARCHIVE_H
#define VDR_BURN_CHAIN_ARCHIVE_H

#include "chain-vdr.h"
#include "jobs.h"

namespace vdr_burn
{

	class chain_archive: public chain_vdr
	{
	public:
		enum step { build, burn };

	protected:
		virtual bool initialize();
		virtual void process_line(const std::string& line);
		virtual bool finished(const proctools::process* proc);

                bool get_is_burning(int& progress) const { progress = m_progress; return m_progress; }

		bool prepare_device();
		bool prepare_burning();
		bool prepare_recording_mark();
		bool prepare_archive_mark();

	private:
		friend class chain_vdr;

		chain_archive(job& job);

		step m_step;
		recording_list::const_iterator m_current;
	};
}

#endif // VDR_BURN_CHAIN_ARCHIVE_H
