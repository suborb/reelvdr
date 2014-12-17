/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: jobs.c,v 1.35 2006/10/01 21:22:27 lordjaxom Exp $
 */

#include "burn.h"
#include "common.h"
#include "jobs.h"
#include "setup.h"
#include "scanner.h"
#include "chain-dvd.h"
#include "chain-archive.h"
#include "adaptor.h"
#include "filter.h"
#include "tracks.h"
#include "logger-vdr.h"
#include "proctools/format.h"
#include "proctools/functions.h"
#include "proctools/logger.h"
#include "boost/bind.hpp"
#include <string>
#include <functional>
#include <numeric>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <vdr/recording.h>
#include <vdr/videodir.h>
#include <vdr/plugin.h>

namespace vdr_burn
{
   using namespace std;
   using boost::bind;
   using proctools::format;
   using proctools::convert;
   using proctools::logger;
   using proctools::sum;

   // --- recording -----------------------------------------------------------

   recording::recording( job* owner_, const cRecording* recording_):
   m_owner( owner_ ),
   m_fileName( recording_->FileName() ),
   m_summary( get_recording_description(recording_) ),
   m_datetime( get_recording_datetime(recording_, ' ') ),
   m_title( recording_->Name() ),
   m_totalSize( 0, 0 ),
   m_totalLength( 0, 0 ),
#if defined USE_ISHD
   m_isHD( recording_->IsHD() ),
   m_isTS( recording_->IsTS() )
#else
   m_isHD( false ),
   m_isTS( false )
#endif   
   {
      std::string::size_type pos;
      if( global_setup().RemovePath && ( pos = m_title.rfind( '~' ) ) != std::string::npos )
         m_title.erase( 0, pos + 1 );

      for( pos = 0;; ++pos ) {
         trim_left( m_title, "%@", pos );
         if( ( pos = m_title.find( '~', pos ) ) == std::string::npos )
            break;
      }
   }

// size_pair::size_type recording::get_total_size() const
// {
//    return m_owner.get_cut_on_demux() ? m_totalSize.cut : m_totalSize.uncut;
// }
//
// length_pair::size_type recording::get_total_length() const
// {
//    return m_owner.get_cut_on_demux() ? m_totalLength.cut : m_totalLength.uncut;
// }
//
   size_pair::size_type recording::get_tracks_size( bool cut_, track_info::streamtype type_ ) const
   {
      const_track_filter tracks( m_tracks, type_, track_predicate::used );
      if( ! m_isHD && ! m_isTS ) {
         return accumulate_size( tracks.begin(), tracks.end(), bind( cut_ ? &size_pair::cut : &size_pair::uncut, bind( &track_info::size, _1 ) ) );
      }
      else {
         return(size_pair::size_type)m_totalSize.uncut;
      }
   }

   size_pair::size_type recording::get_tracks_size( track_info::streamtype type_ ) const
   {
      return get_tracks_size( m_owner->get_options().CutOnDemux, type_ );
   }

   void recording::set_paths(const path_pair& paths, unsigned int index)
   {
      m_paths = path_pair(format("{0}/VDRSYNC.{1}") % paths.temp % index,
                          format("{0}/VDRSYNC.{1}") % paths.data % index);
   }

   void recording::set_track_path(int cid, const std::string& path)
   {
      track_filter audioTracks( m_tracks, track_info::streamtype_audio, track_predicate::used );
      track_filter::iterator it =
      find_if(audioTracks.begin(), audioTracks.end(),
              bind( equal_to<int>(), bind( &track_info::cid, _1 ), cid ));
      if( it != audioTracks.end() )
         it->filename = path;
   }

   /*int64_t recording::get_video_size() const
   {
      const_track_filter videoTracks( m_tracks, track_info::streamtype_video, track_predicate::used );
      if (videoTracks.begin() != videoTracks.end())
         return videoTracks.begin()->size.cut;
      return 0;
   }

   int64_t recording::get_audio_size() const
   {
      const_track_filter audioTracks( m_tracks, track_info::streamtype_audio, track_predicate::used );
      return sum(audioTracks.begin(), audioTracks.end(),
               int64_t(0),
               bind( &size_pair::cut, bind( &track_info::size, _1 )));
   }*/

   string recording::get_track_path(const track_info& track) const
   {
      if( !track.used ) return "";

      return proctools::format("{0}/{1}")
      % (global_setup().DemuxType != demuxtype_vdrsync ? m_paths.data : m_paths.temp)
      % track.filename;
   }

   string recording::get_video_track_path() const
   {
      const_track_filter videoTracks( m_tracks, track_info::streamtype_video, track_predicate::used );
      if( videoTracks.begin() != videoTracks.end() )
         return get_track_path(*videoTracks.begin());
      return string( );
   }

   string recording::get_audio_paths() const
   {
      const_track_filter audioTracks( m_tracks, track_info::streamtype_audio, track_predicate::used );
      return join_strings(audioTracks.begin(), audioTracks.end(),
                          bind( &recording::get_track_path, this, _1 ), " ");
   }

   string recording::get_used_cids() const
   {
      const_track_filter allTracks( m_tracks, track_predicate::used );
      return join_strings(allTracks.begin(), allTracks.end(),
                          bind( int_to_string, bind( &track_info::cid, _1 ), 16, true ),
                          ",");
      // XXX too much knowledge: used_cids, used by ProjectX, must be
      // standard-parseable, so prefix with 0x
      // additionally, ProjectX needs video here, too
   }

   string recording::get_ignored_cids() const
   {
      const_track_filter allTracks( m_tracks, track_predicate::unused );
      return join_strings(allTracks.begin(), allTracks.end(),
                          bind( int_to_string, bind( &track_info::cid, _1 ), 16, false ),
                          ",");
      // XXX too much knowledge: ignored_cids, used by vdrsync.pl, must be
      // pure hex, so do not prefix
   }

   string recording::get_chapters(int mode) const
   {
#define MAXCHAPTER (4 * 60 * 60)

      switch( mode ) {
         case chaptersmode_marks:
            {
               cMarks marks;
               if( !marks.Load(m_fileName.c_str()) ) {
                  logger::warning("loading marks failed, setting chapters every "
                                  "ten minutes.");
                  return get_chapters(chaptersmode_10);
               }
               marks.Sort();

               if( marks.Count() % 2 != 0 )
                  logger::warning("an odd number of marks has been loaded, which "
                                  "usually leads to undefined behaviour.");

               cMark *mark = marks.GetNext(0); // get 2nd mark
               int count = 0;
               stringstream result("0");
               while( mark != NULL ) {
                  if( ++count % 2 == 0 && marks.GetNext(mark->position) != NULL )
                     result << "," << *IndexToHMSF(mark->position, false);
                  mark = marks.GetNext(mark->position);
               }
               logger::info("chapter marks generated: " + result.str());
               return result.str();
            }

         case chaptersmode_none:
            return "";

         default:
            break;
      }

      stringstream result;
      for( int i = 0; i <= MAXCHAPTER; i += chaptersmode_intervals[mode] * 60 ) {
         if( i > 0 )
            result << ",";
         result << i / 3600 << ":" << (i / 60) % 60 << ":"
         << i % 60;
      }
      return result.str();
   }

   std::string job::get_iso_path() const
   {
      return proctools::format("{0}/{1}.iso")
      % BurnParameters.IsoPath % string_replace( m_title, '/', '_' );
   }

   std::string recording::get_graft_point() const
   {
      return proctools::format("{0}={1}")
      % m_fileName.substr(std::string(VideoDirectory).length())
      % m_fileName;
   }

   // --- job ---------------------------------------------------------------

   // do not remove empty c'tor
   // since chain_vdr is forward declarated in jobs.h and definition of c'tor needs
   // visibility of chain_vdr
   job::job()
   {
      m_options = job_defaults();
   }

   // do not remove empty d'tor
   job::~job()
   {
   }

   void job::start()
   {
      if( m_process.get() != 0 ) {
         logger::error("process already running (this shouldn't happen)");
         return;
      }

      m_process.reset( chain_vdr::create_chain(*this) );
      m_process->start();
   }

   void job::stop()
   {
      if( m_process.get() != 0 )
         m_process->stop();
   }

   void job::reset()
   {
      m_process.reset( 0 );
   }

   void job::clear()
   {
      m_title.clear();
      m_options = job_defaults();
      m_recordings.clear();
      m_paths = path_pair();
      m_process.reset( 0 );
   }

   void job::append_recording(const cRecording* vdrRecording)
   {
      recording_scanner scanner( this, vdrRecording );
      scanner.scan();

      recording newRec( scanner.get_result() );
      if( m_title.empty() )
         m_title = newRec.get_title();
      m_recordings.push_back(newRec);

#if defined USE_ISHD
      if( vdrRecording->Info()->IsHD() )
         m_options.DiskType = disktype_archive;
#endif
      change_media();
      if( get_requant_factor() > 1 && 
          (get_disk_type() >= disktype_countrequant || global_setup().RequantType == requanttype_never  ) ) {
         m_recordings.pop_back();
         throw std::string(( tr("Recording too big for disc type!") ));
      }
   }

   void job::erase_recording(recording_list::iterator recording)
   {
      m_recordings.erase(recording);
      if( m_recordings.size() == 0 ) {
         m_options.DiskSize = disksize_none;
         clear();
      }
      else
         change_media();
   }

   void job::change_media(){
      if( m_options.DiskType == disktype_archive )
         m_options.DiskSize = disksize_cdr;
      else
         m_options.DiskSize = disksize_singlelayer;

      while( get_requant_factor() > 1 && (m_options.DiskSize + 1) < disksize_count )
         ++m_options.DiskSize;
   }

   bool job::set_options( const job_options& options_, std::string& error_ )
   {
      job_options optionsCopy;
      optionsCopy = options_;

      if( get_requant_factor( optionsCopy.CutOnDemux ) > 1 && 
          ( optionsCopy.DiskType >= disktype_countrequant || global_setup().RequantType == requanttype_never  ) ) {
         error_ = tr("Disc too big for disc type!");
         return false;
      }
      m_options = optionsCopy;
      change_media();
      return true;
   }

   double job::get_requant_factor( bool cut_ ) const
   {
      size_pair::size_type disk_size = size_pair::size_type( get_disk_size_mb() ) * MEGABYTE(1);
      size_pair::size_type tracks_size = get_tracks_size( cut_, track_info::streamtype_audio );
      size_pair::size_type diskFree;
      if( disk_size > tracks_size )
         diskFree = disk_size - tracks_size;
      else
         return 2; // the media is too small!
      size_pair::size_type videoSize =
      size_pair::size_type( double( get_tracks_size( cut_, track_info::streamtype_video ) ) * 1.06 );

      return videoSize > diskFree
      ? double( get_tracks_size( cut_, track_info::streamtype_video ) ) / diskFree + .12
      : 1;
   }

   double job::get_requant_factor() const
   {
      return get_requant_factor( m_options.CutOnDemux );
   }

   size_pair::size_type job::get_tracks_size( bool cut_, track_info::streamtype type_ ) const
   {
      return accumulate_size( m_recordings.begin(), m_recordings.end(), bind( &recording::get_tracks_size, _1, cut_, type_ ) );
   }

   size_pair::size_type job::get_tracks_size( track_info::streamtype type_ ) const
   {
      return get_tracks_size( m_options.CutOnDemux, type_ );
   }

   struct path_setter {
      path_setter(const path_pair& paths): m_paths(paths), m_index(0) {}
      void operator()(recording& rec) { rec.set_paths(m_paths, m_index++);}
      const path_pair& m_paths;
      int m_index;
   };

   void job::set_paths(const path_pair& paths)
   {
      m_paths = paths;
      for_each(m_recordings.begin(), m_recordings.end(), path_setter( m_paths ));
   }

   recording_list::iterator job::get_by_filename(const string& fileName)
   {
      return find_if(m_recordings.begin(), m_recordings.end(),
                     bind(
                         equal_to<string>(),
                         bind( &recording::get_filename, _1 ),
                         fileName
                         ));
   }

   int job::get_progress() const
   {
      return m_process.get() != 0 ? m_process->get_progress() : 0;
   }

   bool job::get_is_burning(int& progress) const
   {
      return m_process.get() != 0 && m_process->get_is_burning(progress);
   }

   bool job::get_is_active() const
   {
      return m_process.get() != 0 && m_process->get_is_active();
   }

   bool job::get_is_canceled() const
   {
      return m_process.get() != 0 && m_process->is_canceled();
   }

   proctools::process::status_value job::get_return_status() const
   {
      return m_process.get() != 0 ? m_process->return_status() : proctools::process::none;
   }

   int job::get_disk_size_mb() const
   {
//      if (m_options.DiskSize == disksize_custom)
//         return global_setup().CustomDiskSize;
      return disksize_values[m_options.DiskSize];
   }

   const char* job::get_disk_size_capacity() const
   {
      return tr(disksize_strings_capacity[m_options.DiskSize]);
   }

   std::string job::get_archive_id()
   {
      std::string archive_id = "";
      std::string counter_path = format("{0}/counters/standard") % plugin::get_config_path();
      std::ifstream f(counter_path.c_str());
      if( !f ) {
         proctools::logger::error(proctools::format("couldn't read {0}") % counter_path);
         return "";
      }
      f >> archive_id;
      f.close();
      return archive_id;
   }

}
