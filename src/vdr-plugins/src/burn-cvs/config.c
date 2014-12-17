/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: config.c,v 1.17 2006/09/16 18:33:36 lordjaxom Exp $
 */

#include "config.h"
#include "filter.h"
#include "setup.h"
#include <vdr/config.h>
#include <vdr/i18n.h>

namespace vdr_burn
{

	using namespace std;
	using proctools::logger;
	using proctools::format;

	// --- dvdauthor_xml  -----------------------------------------------------

	dvdauthor_xml::dvdauthor_xml(const job& Job):
			m_job(Job)
	{
	}

#if 0
	const char* dvdauthor_xml::get_video_format(component_type::type type_)
	{
		switch (type_) {
		case component_type::hz25_4_3 ... component_type::hz25_21_9: return "pal";
		case component_type::hz30_4_3 ... component_type::hz30_21_9: return "ntsc";
		default: return 0;
		}
	}
#endif

	string dvdauthor_xml::get_video_aspect(track_info::aspectratio type)
	{
		switch (type) {
		case track_info::aspectratio_4_3: return "aspect=\"4:3\"";
		case track_info::aspectratio_16_9:
		case track_info::aspectratio_21_9: return "aspect=\"16:9\"";
		default: return "";
		}
	}

	string dvdauthor_xml::get_audio_language(int language)
	{
		return "lang=\"" + track_info::get_language_codes()[ language ] + "\"";
	}

	void dvdauthor_xml::write_main_menu()
	{
		m_file << "    <menus>" << endl;
		int pages = ScanPageCount(m_job.get_paths().data);
		for (int p = 0; p < pages; ++p) {
			if (p == 0) {
				m_file << "      <pgc entry=\"title\">" << endl // pause="inf"??
				  	   << "        <pre>" << endl;
				for (int q = 2; q <= pages; q++)
					m_file << "           if (g0 eq " << q << ") { jump menu " << q <<";}" << endl;
				m_file << "             button=g1;g3=0;" << endl
				  	   << "        </pre>" << endl;
			}
			else
			{
				m_file << "    <pgc>" << endl
				  	   << "      <pre>button=g1;g3=0;</pre>" << endl;
			}

			m_file << "        <vob file=\"" << m_job.get_paths().data << "/menu-bg-"
			  	   << p << ".mpg\" pause=\"inf\"/>" << endl; // TODO observe pause=inf

			for (uint i = p * m_job.get_skin().get_main_menu_titles().size();
				 i < std::min(static_cast<recording_list::size_type>((p + 1)
						 * m_job.get_skin().get_main_menu_titles().size()), m_job.get_recordings().size());
				 ++i)
				m_file << "        <button>g1=button;jump titleset " << (i + 1)
				  	   << " menu;</button>" << endl;

			if (p > 0)
				m_file << "        <button>g0=" << p << ";g1=" << (m_job.get_skin().get_main_menu_titles().size()*1024)
					   << ";jump menu " << p << ";</button>" << endl;

			if (p < pages - 1)
				m_file << "        <button>g0=" << (p + 2) << ";g1=1024;jump menu " << (p + 2)
					   << ";</button>" << endl;

			m_file << "      </pgc>" << endl;
		}
		m_file << "    </menus>" << endl;
	}

	void dvdauthor_xml::write_title_menu(recording_list::const_iterator rec)
	{
		int pages = ScanPageCount(rec->get_paths().data);

		m_file << "    <menus>" << endl;

		for (int i = 0; i < pages; ++i) {
			m_file << "      <pgc" << (i == 0 ? " entry=\"root\"" : "") << ">" << endl;
			if (rec->get_summary() == "" && m_job.get_skip_titlemenu())
				m_file << "        <pre>jump title 1 chapter 1;</pre>" << endl;
			else {
				if ( i == 0) {
					m_file << "        <pre>button=1024;</pre>" << endl;
				}
				else
				{
					m_file << "        <pre>if (g3 eq 1){button=3072;}" << endl
					  	   << "             if (g3 eq 2){button=1024;}" << endl
					  	   << "             if (g3 eq 3){button=2048;}" << endl
					  	   << "        </pre>" << endl;
				}
				m_file << "        <vob file=\"" << rec->get_menu_mpeg(i) << "\" pause=\"inf\"/>"
					// TODO observe pause=inf
					   << endl;

				if (i > 0)
					m_file << "        <button>g3=2;jump menu " << i << ";</button>"
					  	   << endl;

				m_file << "        <button>jump title 1 chapter 1;</button>" << endl;
				if (i < pages - 1)
					m_file << "        <button>g3=" << (i == pages - 2 ? 3 : 1) << ";jump menu "
						   << (i + 2) << ";</button>" << endl;

				// show the back button only unless SkipMainmenu or if we have more than one track on disk
				if( !m_job.get_options().SkipMainmenu || m_job.get_recordings().size() > 1 )
					m_file << "        <button>jump vmgm menu;</button>" << endl;
			}
			m_file << "      </pgc>" << endl;
		}

		m_file << "    </menus>" << endl;
	}

	void dvdauthor_xml::write()
	{
		m_file.open(get_xml_path().c_str(), ios::out);
		if (!m_file) {
			logger::error(format("couldn't create {0}") % get_xml_path());
			return;
		}
		m_file << "<?xml version=\"1.0\" encoding=\"" << plugin::get_character_encoding() << "\"?>" << endl;
		m_file << "<dvdauthor dest=\"" << get_author_path() << "\">" << endl
		  << "  <vmgm>" << endl;

		bool createMenu = (m_job.get_disk_type() == disktype_dvd_menu);
		if (createMenu) {
			// Go directly to the menu of the first title, if SkipMainMenu and this is the only item in the main menu
			if( m_job.get_options().SkipMainmenu && m_job.get_recordings().size() == 1 ) {
				// Jump to the title-menu of the only track
				m_file << "    <fpc>g0=1;g1=1024;jump titleset 1 menu;</fpc>" << endl;
				m_file << "    <menus>" << endl
				<< "      <pgc entry=\"title\">" << endl
				<< "        <pre>" << endl
				<< "          jump titleset 1 menu;" << endl;
				m_file << "        </pre>" << endl
				<< "        <!-- dummy vob required: vob file= -->" << endl
				<< "      </pgc>" << endl
				<< "    </menus>" << endl;
			}
			else {
				// Jump to the main-menu
				m_file << "    <fpc>g0=1;g1=1024;jump vmgm menu 1;</fpc>" << endl;
				write_main_menu();
			}
			m_file << "  </vmgm>" << endl;

			for (recording_list::const_iterator rec = m_job.get_recordings().begin();
					 rec != m_job.get_recordings().end(); ++rec) {

				m_file << "  <titleset>" << endl;
				write_title_menu(rec);
				m_file << "    <titles>" << endl;

				const_track_filter videoTracks( rec->get_tracks(), track_info::streamtype_video, track_predicate::used );
				if (videoTracks.begin() != videoTracks.end()) {
					const track_info& track = *videoTracks.begin();
					m_file << "<video " << get_video_aspect(track.video.aspect) << "/>" << endl;
				}

				const_track_filter audioTracks( rec->get_tracks(), track_info::streamtype_audio, track_predicate::used );
				const_track_filter::iterator audioTrack = audioTracks.begin();
				while  (audioTrack != audioTracks.end()) {
					const track_info& track = *audioTrack;
					m_file << "      <audio " << get_audio_language(track.language) << "/>"
					  << "  <!-- " << track.language << " - " << track.description << " -->" << endl;
					audioTrack++;
				}

// 				m_file << "<subpicture " << get_audio_language( track_info::get_language_codes()[ setup::get().DefaultLanguage ] )
//				  << "/>" << endl
				m_file << "      <pgc>" << endl
				  << "        <vob file=\"" << rec->get_movie_path() << "\" chapters=\""
				  << rec->get_chapters(m_job.get_chapters_mode()) << "\"/>" << endl;

				m_file << "        <post>call vmgm menu;</post>" << endl;
				m_file << "      </pgc>" << endl
				  << "    </titles>" << endl
				  << "  </titleset>" << endl;
			}
		}
		else
		{
			m_file << "    <fpc>g3=1;jump vmgm menu 1;</fpc>" << endl
			  << "    <menus>" << endl
			  << "      <pgc entry=\"title\">" << endl
			  << "        <pre>" << endl;
			int titleset = 1;
			for (recording_list::const_iterator rec = m_job.get_recordings().begin();
				 rec != m_job.get_recordings().end(); ++rec) {
				m_file << "          if (g3 eq " << titleset << ") {jump title " << titleset
					   << ";}" << endl;
				titleset++;
			}
			m_file << "        </pre>" << endl
			  << "        <!-- dummy vob required: vob file= -->" << endl
			  << "      </pgc>" << endl
			  << "    </menus>" << endl
			  << "  </vmgm>" << endl;
			int t = 1;
			for (recording_list::const_iterator rec = m_job.get_recordings().begin();
				 rec != m_job.get_recordings().end(); ++rec) {
				m_file << "  <titleset>" << endl;
				m_file << "    <titles>" << endl;

				const_track_filter videoTracks( rec->get_tracks(), track_info::streamtype_video, track_predicate::used );
				if (videoTracks.begin() != videoTracks.end()) {
					const track_info& track = *videoTracks.begin();
					m_file << "<video " << get_video_aspect(track.video.aspect) << "/>" << endl;
				}

				const_track_filter audioTracks( rec->get_tracks(), track_info::streamtype_audio, track_predicate::used );
				const_track_filter::iterator audioTrack = audioTracks.begin();
				while  (audioTrack != audioTracks.end()) {
					const track_info& track = *audioTrack;
					m_file << "      <audio " << get_audio_language(track.language) << "/>"
					  << "  <!-- " << track.language << " - " << track.description << " -->" << endl;
					audioTrack++;
				}

// 				m_file << "<subpicture " << get_audio_language( track_info::get_language_codes()[ setup::get().DefaultLanguage ] )
//				  << "/>" << endl
				m_file << "      <pgc>" << endl
				  << "        <vob file=\"" << rec->get_movie_path()
				  << "\" chapters=\""
				  << rec->get_chapters(m_job.get_chapters_mode()) << "\" pause=\"2\" />" << endl;
				if (t+1 < titleset) {
					m_file << "        <post>g3=" << ++t <<";call vmgm menu 1;</post>" << endl;
				}
				else {
					m_file << "        <post>exit;</post>" << endl;
				}

			m_file << "      </pgc>" << endl
			  << "    </titles>" << endl
			  << "  </titleset>" << endl;
			}
		}
		m_file << "</dvdauthor>\n";
	}

}
