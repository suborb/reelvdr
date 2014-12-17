/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: render.h,v 1.9 2006/09/16 18:33:36 lordjaxom Exp $
 */

#ifndef VDR_BURN_RENDER_H
#define VDR_BURN_RENDER_H

#include "jobs.h"
#include "proctools/process.h"
#include "proctools/format.h"

namespace vdr_burn
{

	class chain_vdr;

	class renderer
	{
	private:
		const job& m_job;
		chain_vdr& m_chain;
		recording_list::const_iterator m_current;

	protected:
		bool CreateMainMenuPage(int Page, int Count);
		bool CreateMainMenuButtons(int Page, int Count);
		bool CreateMainMenu();

		bool CreateTitleMenuPage(int &Lines, int Page);
		bool CreateTitleMenuButtons(int Lines, int Page);
		bool CreateTitleMenu();

		template< typename Type >
		bool execute_render( const Type& item, int page );

	public:
		renderer( const job& job_, chain_vdr& chain_ );

		bool operator()();
	};

}

#endif // VDR_BURN_RENDER_H
