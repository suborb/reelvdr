/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: render.c,v 1.9 2006/09/16 18:33:36 lordjaxom Exp $
 */

#include "burn.h"
#include "chain-vdr.h"
#include "common.h"
#include "config.h"
#include "jobs.h"
#include "render.h"
#include "gdwrapper.h"
#include "iconvwrapper.h"
#include "proctools/logger.h"
#include "proctools/format.h"
#include "proctools/shellprocess.h"
#include <vdr/tools.h>

namespace vdr_burn
{

	using namespace std;
	using namespace gdwrapper;
	using proctools::logger;
	using proctools::format;
	using proctools::process;
	using proctools::shellescape;
	using proctools::shellprocess;

	renderer::renderer( const job& job_, chain_vdr& chain_ ):
			m_job( job_ ),
			m_chain( chain_ )
	{
	}

	bool renderer::CreateMainMenuPage(int Page, int Count)
	{
		int first = Page * m_job.get_skin().get_main_menu_titles().size();

		image menuPage( 720, 576, true );

		image background( m_job.get_skin().get_background() );
		menuPage.draw_image( point( 0, 0 ), background );

		menuPage.draw_text( m_job.get_skin().get_main_menu_header(), convert_to_utf8( m_job.get_title() ) );
		if (Page > 0)
			menuPage.draw_text( m_job.get_skin().get_main_menu_previous().region, convert_to_utf8( tr("Previous page") ) );
		if (Page < Count - 1)
			menuPage.draw_text( m_job.get_skin().get_main_menu_next().region, convert_to_utf8( tr("Next page") ) );

		recording_list::const_iterator rec = m_job.get_recordings().begin();
		advance(rec, first);
		for (skin::menu_list::const_iterator it = m_job.get_skin().get_main_menu_titles().begin();
			 rec != m_job.get_recordings().end() && it != m_job.get_skin().get_main_menu_titles().end();
			 ++it, ++rec)
			menuPage.draw_text( it->region, convert_to_utf8( rec->get_title() ) );

		menuPage.save( m_job.get_menu_background( Page ) );
		return true;
	}

	bool renderer::CreateMainMenuButtons(int Page, int Count)
	{
		int first = Page * m_job.get_skin().get_main_menu_titles().size();

		image buttonsNormal( 720, 576 );
		buttonsNormal.fill_rectangle( rectangle( 0, 0, 720, 576 ), color( 255, 255, 255, 255 ) );
		buttonsNormal.save( m_job.get_buttons_normal() );

		image buttonsHighlight( 720, 576 );
		buttonsHighlight.fill_rectangle( rectangle( 0, 0, 720, 576 ), color( 255, 255, 255, 255 ) );

		if (Page < Count - 1) {
			image button( m_job.get_skin().get_main_menu_previous().image );
			buttonsHighlight.draw_image( m_job.get_skin().get_main_menu_previous().position, button );
		}
		if (Page > 0) {
			image button( m_job.get_skin().get_main_menu_next().image );
			buttonsHighlight.draw_image( m_job.get_skin().get_main_menu_next().position, button );
		}

		recording_list::const_iterator rec = m_job.get_recordings().begin();
		advance(rec, first);
		for (skin::menu_list::const_iterator it = m_job.get_skin().get_main_menu_titles().begin();
			 rec != m_job.get_recordings().end() && it != m_job.get_skin().get_main_menu_titles().end();
			 ++it, ++rec) {
			image button( it->image );
			buttonsHighlight.draw_image( it->position, button );
		}

		buttonsHighlight.save( m_job.get_buttons_highlight( Page ) );
		return true;
	}

	template< typename Type >
	bool renderer::execute_render( const Type& item, int page )
	{
		submux_xml< Type > config( item );
		config.write( page );

		// TODO: move rendering into chain
		shellprocess render( "render", shellescape( "vdrburn-dvd.sh" ) + "render" );
		render.put_environment( "MENU_BACKGROUND", item.get_menu_background( page ) );
		render.put_environment( "MENU_SOUNDTRACK", plugin::get_config_path() + "/menu-silence.mp2" );
		render.put_environment( "MENU_XML", config.get_xml_path( page ) );
		render.put_environment( "MENU_M2V", item.get_paths().data + "/menu-background.m2v" );
		render.put_environment( "MENU_MPEG", item.get_menu_mpeg( page ) );
		m_chain.execute( render );

		// TODO error message passing
		return render.return_status() == process::ok;
	}

	bool renderer::CreateMainMenu()
	{
		int count = m_job.get_recordings().size() / m_job.get_skin().get_main_menu_titles().size();
		if (m_job.get_recordings().size() % m_job.get_skin().get_main_menu_titles().size() > 0)
			++count;

		for (int page = 0; page < count; ++page) {
			if ( !CreateMainMenuPage(page, count) ||
				 !CreateMainMenuButtons(page, count) ||
				 !execute_render( m_job, page ) )
				return false;
		}
		return true;
	}

	bool renderer::CreateTitleMenuPage(int &Lines, int Page)
	{
		image menuPage( 720, 576, true );

		image background( m_job.get_skin().get_background() );
		menuPage.draw_image( point( 0, 0 ), background );

		menuPage.draw_text( m_job.get_skin().get_title_menu_header(), convert_to_utf8( m_current->get_title() ) );
		menuPage.draw_text( m_job.get_skin().get_title_menu_play().region, convert_to_utf8( tr("Play movie") ) );

		// Create "Back"-Button unless SkipMainmenu or if there are other tracks
		if( !m_job.get_options().SkipMainmenu || m_job.get_recordings().size() > 1 )
			menuPage.draw_text( m_job.get_skin().get_title_menu_exit().region, convert_to_utf8( tr("Button$Back") ) );

		Lines = menuPage.draw_text( m_job.get_skin().get_title_menu_text(), convert_to_utf8( m_current->get_summary() ), Lines );

		if (Lines != -1)
			menuPage.draw_text( m_job.get_skin().get_title_menu_next().region, convert_to_utf8( tr("Next page") ) );
		if (Page > 0)
			menuPage.draw_text( m_job.get_skin().get_title_menu_previous().region, convert_to_utf8( tr("Previous page") ) );

		menuPage.save( m_current->get_menu_background(Page) );
		return true;
	}

	bool renderer::CreateTitleMenuButtons(int Lines, int Page)
	{
		image buttonsNormal( 720, 576 );
		buttonsNormal.fill_rectangle( rectangle( 0, 0, 720, 576 ), color( 255, 255, 255, 255 ) );
		buttonsNormal.save( m_current->get_buttons_normal() );

		image buttonsHighlight( 720, 576 );
		buttonsHighlight.fill_rectangle( rectangle( 0, 0, 720, 576 ), color( 255, 255, 255, 255 ) );

		{
			image button( m_job.get_skin().get_title_menu_play().image );
			buttonsHighlight.draw_image( m_job.get_skin().get_title_menu_play().position, button );
		}

		// Create "Back"-Button unless SkipMainmenu or if there are other tracks
		if( !m_job.get_options().SkipMainmenu || m_job.get_recordings().size() > 1 ) {
			image button( m_job.get_skin().get_title_menu_exit().image );
			buttonsHighlight.draw_image( m_job.get_skin().get_title_menu_exit().position, button );
		}

		if (Lines != -1) {
			image button( m_job.get_skin().get_title_menu_next().image );
			buttonsHighlight.draw_image( m_job.get_skin().get_title_menu_next().position, button );
		}

		if (Page > 0) {
			image button( m_job.get_skin().get_title_menu_previous().image );
			buttonsHighlight.draw_image( m_job.get_skin().get_title_menu_previous().position, button );
		}

		buttonsHighlight.save( m_current->get_buttons_highlight(Page) );
		return true;
	}

	bool renderer::CreateTitleMenu()
	{
		/* Still quite ugly, but how else?? */
		int lines = 0, page = 0;

		while (lines != -1) {
			if ( !CreateTitleMenuPage( lines, page ) ||
				 !CreateTitleMenuButtons( lines, page ) ||
				 !execute_render( *m_current, page ) )
				return false;

			++page;
		}
		return true;
	}

	bool renderer::operator()()
	{
		try {
			/* runs in a separate process */
			if ( !CreateMainMenu() )
				return false;

			for (m_current = m_job.get_recordings().begin(); m_current != m_job.get_recordings().end(); ++m_current) {
				if ( !CreateTitleMenu() )
					return false;
			}
		} catch ( const std::string& ex ) {
			logger::error( string( "Error while rendering: " ) + ex.c_str() );
			return false;
		}

		return true;
	}

}
