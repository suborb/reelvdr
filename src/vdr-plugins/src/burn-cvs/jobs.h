/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: jobs.h,v 1.27 2006/09/16 18:33:36 lordjaxom Exp $
 */

#ifndef VDR_BURN_JOBS_H
#define VDR_BURN_JOBS_H

#include "common.h"
#include "setup.h"
#include "skins.h"
#include "tracks.h"
#include "proctools/format.h"
#include "proctools/process.h"
#include <deque>
#include <list>
#include <memory>
#include <numeric>
#include <string>

namespace vdr_burn {

	class recording;
	class job;
	class chain_vdr;

	namespace menu
	{
		class track_editor;
		class job_editor;
		class recording_edit_item;
		class recording_text_item;
	}

	// --- recording ----------------------------------------------------------

	class recording
	{
	private:
		friend class menu::job_editor;
		friend class menu::track_editor;
		friend class menu::recording_edit_item;
		friend class menu::recording_text_item;


		job*                   m_owner;
		std::string            m_fileName;
		std::string            m_summary;
		std::string            m_datetime;
		path_pair              m_paths;
		std::string            m_title;
		track_info_list        m_tracks;

		size_pair              m_totalSize;
		length_pair            m_totalLength;

		bool                   m_isHD;
		bool                   m_isTS;

    protected:
        friend class recording_scanner;

        void take_tracks(track_info_list& tracks) { m_tracks.swap( tracks ); }
        void set_total_size(const size_pair& value) { m_totalSize = value; }
        void set_total_length(const length_pair& value) { m_totalLength = value; }

	public:
		recording( job* owner_, const cRecording *recording_ );

		void set_paths(const path_pair& paths, unsigned int index);
		void set_track_path(int cid, const std::string& path);

		const path_pair& get_paths() const { return m_paths; }
		const std::string& get_filename() const { return m_fileName; }
		const std::string& get_summary() const { return m_summary; }
		const std::string& get_datetime() const { return m_datetime; }
		const std::string& get_title() const { return m_title; }
		const track_info_list& get_tracks() const { return m_tracks; }
		track_info_list& get_tracks() { return m_tracks; }
		bool isHD() { return m_isHD;}
		bool isTS() { return m_isTS;}

		std::string get_chapters(int mode) const;
		std::string get_track_path(const track_info& track) const;
		std::string get_video_track_path() const;
		std::string get_requant_path() const;
		std::string get_audio_paths() const;
		std::string get_used_cids() const;
		std::string get_ignored_cids() const;
		std::string get_movie_path() const;
		std::string get_menu_background(int Page) const;
		std::string get_buttons_normal() const;
		std::string get_buttons_highlight(int Page) const;
		std::string get_menu_mpeg(int Page) const;
		std::string get_graft_point() const;

		// size information (original recording)
//		size_pair::size_type get_total_size() const;
//		length_pair::size_type get_total_length() const;

		// size information (only used tracks)
		size_pair::size_type get_tracks_size( bool cut_, track_info::streamtype type_ = track_info::streamtype( 0 ) ) const;
		size_pair::size_type get_tracks_size( track_info::streamtype type_ = track_info::streamtype( 0 ) ) const;
	};

	inline
	std::string recording::get_requant_path() const
	{
		return proctools::format( "{0}/requant.mpv" ) % m_paths.temp;
	}

	inline
	std::string recording::get_movie_path() const
	{
		return proctools::format( "{0}/movie.mpg" ) % m_paths.temp;
	}

	inline
	std::string recording::get_menu_background(int page) const
	{
		return proctools::format("{0}/menu-bg-{1}.png") % m_paths.data % page;
	}

	inline
	std::string recording::get_buttons_normal() const
	{
		return proctools::format("{0}/menu-buttons-ns.png") % m_paths.data;
	}

	inline
	std::string recording::get_buttons_highlight(int page) const
	{
		return proctools::format("{0}/menu-buttons-h-{1}.png") % m_paths.data % page;
	}

	inline
	std::string recording::get_menu_mpeg(int page) const
	{
		return proctools::format("{0}/menu-bg-{1}.mpg") % m_paths.data % page;
	}

	typedef std::list<recording> recording_list;

	// --- job -----------------------------------------------------------

	class job
	{
//	public:
//		struct options
//		{
//			int diskType;
//			bool dmhArchiveMode;
//			int storeMode;
//			int skinIndex;
//			int chaptersMode;
//			int diskSize;
//			bool cutOnDemux;
//			bool skipTitlemenu;
//
//			options();
//
//			bool operator==( const options& other ) const;
//		};

	private:
		std::string m_title;
		job_options m_options;
		recording_list m_recordings;
		path_pair m_paths;
		std::auto_ptr<chain_vdr> m_process;
		void change_media(); // called to get suitable media for recording

	public:
		job();
		virtual ~job();

		void reset();  // called by manager when job is re-edited
		void start();  // called by manager when job should start
		void stop();   // called by manager when job is to be canceled
		void clear();      // called when job is to be reset to defaults

		void append_recording(const cRecording* recording);
		void erase_recording(recording_list::iterator recording);
		recording_list::iterator get_by_filename(const std::string& FileName);
		// TODO: separate infos on osd and stored infos
		recording_list& get_recordings() { return m_recordings; }
		const recording_list& get_recordings() const { return m_recordings; }

//		job_options& get_options() { return m_options; }
		const job_options& get_options() const { return m_options; }
		bool set_options( const job_options& options_, std::string& error_ );

		void set_paths(const path_pair& paths);
		//void set_title(const char* title) { strncpy(m_title, title, JOBNAMELEN); }

		// progress monitoring
		int get_progress() const;
		bool get_is_burning(int& progress) const;
		bool get_is_active() const;
		bool get_is_canceled() const;

		// information about job facts
		size_pair::size_type get_tracks_size( bool cut_, track_info::streamtype type = track_info::streamtype( 0 ) ) const;
		size_pair::size_type get_tracks_size( track_info::streamtype type = track_info::streamtype( 0 ) ) const;

		const path_pair& get_paths() const { return m_paths; }
		std::string& get_title() { return m_title; }
		const std::string& get_title() const { return m_title; }
		int get_disk_type() const { return m_options.DiskType; }
		bool get_dmh_archive_mode() const { return m_options.DmhArchiveMode; }
		int get_store_mode() const { return m_options.StoreMode; }
		int get_chapters_mode() const { return m_options.ChaptersMode; }
		int get_disk_size() const { return m_options.DiskSize; }
		int get_burn_speed() const { return m_options.BurnSpeed; }
		bool get_cut_on_demux() const { return m_options.CutOnDemux; }
		bool get_skip_titlemenu() const { return m_options.SkipTitlemenu; }
		int get_disk_size_mb() const;
		const char* get_disk_size_capacity() const;
                void set_recommended_disk();
		proctools::process::status_value get_return_status() const;
		const skin& get_skin() const { return skin_list::get()[m_options.SkinIndex]; }
		double get_requant_factor( bool cut_ ) const;
		double get_requant_factor() const;

		std::string get_iso_path() const;
		std::string get_menu_background(int Page) const;
		std::string get_buttons_normal() const;
		std::string get_buttons_highlight(int Page) const;
		std::string get_menu_mpeg(int Page) const;
		std::string get_volume_id() const;

		static std::string get_archive_id();
	};

//	inline
//	double job::get_size_mb() const
//	{
//		return double( get_tracks_size() ) / MEGABYTE(1);
//	}

	inline
	std::string job::get_menu_background(int page) const
	{
		return proctools::format("{0}/menu-bg-{1}.png") % m_paths.data % page;
	}

	inline
	std::string job::get_buttons_normal() const
	{
		return proctools::format("{0}/menu-buttons-ns.png") % m_paths.data;
	}

	inline
	std::string job::get_buttons_highlight(int page) const
	{
		return proctools::format("{0}/menu-buttons-h-{1}.png") % m_paths.data % page;
	}

	inline
	std::string job::get_menu_mpeg(int page) const
	{
		return proctools::format("{0}/menu-bg-{1}.mpg") % m_paths.data % page;
	}

	inline
	std::string job::get_volume_id() const
	{
		return std::string( m_title ).substr( 0, 32 );
	}

	typedef std::deque<job*> job_queue;

}

#endif // VDR_BURN_JOBS_H
