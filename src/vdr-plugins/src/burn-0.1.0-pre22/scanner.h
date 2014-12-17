/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: scanner.h,v 1.4 2006/09/16 18:33:37 lordjaxom Exp $
 */

#ifndef VDR_BURN_SCANNER_H
#define VDR_BURN_SCANNER_H

#include "jobs.h"
#include <vector>
#include <utility>
#include <vdr/recording.h>

namespace vdr_burn
{

   class recording;

   // --- recording_index -----------------------------------------------------

   class recording_index {
   public:
      static const uint64_t npos = uint64_t( -1 );

      recording_index(const std::string& fileName);

      uint64_t get_bytepos(int index, bool frameEnd = false);

      std::pair<int, int> get_position(int index);
      int get_last_index() { return m_index.Last() - 1;}

   protected:
      void scan_file_sizes();

   private:
      std::string m_fileName;
      std::vector<uint64_t> m_videoFileSizes;
      cIndexFile m_index;
   };

   // --- recording_scanner ---------------------------------------------------

   class recording_scanner {
   public:
      recording_scanner(job* owner, const cRecording* recording);

      // scans the recording
      // throws std::runtime_error on error
      void scan();

      const recording& get_result() const  { return m_scanResult;}

   protected:
      void scan_track_description(track_info& track);
      void scan_total_sizes(recording_index& index, cMarks& marks);
      void scan_audio_track_size(track_info& track);
      void scan_video_track_size(track_info& track);

   private:
      static const int frames_into_movie = 10 * FRAMESPERSEC;
      static const int frames_to_scan = 10 * FRAMESPERSEC;

      const cRecording* m_itemToScan;
      size_pair m_totalSize;
      length_pair m_totalLength;
      recording m_scanResult;
   };

}

#endif // VDR_BURN_SCANNER_H
