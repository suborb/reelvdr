/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: chain-dvd.h,v 1.9 2006/09/16 18:33:36 lordjaxom Exp $
 */

#ifndef VDR_BURN_CHAIN_DVD_H
#define VDR_BURN_CHAIN_DVD_H

#include "chain-vdr.h"
#include "jobs.h"

namespace vdr_burn
{

   class chain_dvd: public chain_vdr {
   public:
      enum step {
         build, burn
      };

   protected:
      virtual bool initialize();
      virtual void process_line(const std::string& line);
      virtual bool finished(const proctools::process* proc);

      void create_files();
      void create_files(const recording& rec);

      bool prepare_job();
      void prepare_recording();
      void prepare_demux();
      void prepare_mplex();
      bool prepare_device();
      bool prepare_burning();
      bool prepare_cutmarks();
      bool prepare_dmh_archive();
      bool prepare_archive_mark();

   private:
      friend class chain_vdr;

      chain_dvd(job& job);

      step                     m_step;
      recording_list::iterator m_currentRecording;
      int                      m_pxAudioIndex;
      int64_t                  m_totalDone;
      int64_t                  m_lastRecSize;
      time_t                   m_lastProgress;
   };

}

#endif // VDR_BURN_CHAIN_DVD_H
