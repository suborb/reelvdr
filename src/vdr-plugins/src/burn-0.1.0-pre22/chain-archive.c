/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: chain-archive.c,v 1.13 2006/09/16 18:33:36 lordjaxom Exp $
 */

#include "burn.h"
#include "chain-archive.h"
#include "setup.h"
#include "boost/bind.hpp"
#include "proctools/shellescape.h"
#include "proctools/shellprocess.h"
#include "proctools/functions.h"
#include "proctools/logger.h"
#include <string>
#include <sstream>
#include <vdr/plugin.h>

namespace vdr_burn
{

   using namespace std;
   using boost::bind;
   using proctools::process;
   using proctools::shellescape;
   using proctools::shellprocess;
   using proctools::format;
   using proctools::logger;

   chain_archive::chain_archive(job& job):
   chain_vdr("Archive", job),
   m_step(build),
   m_current(get_job().get_recordings().begin())
   {
   }

   void chain_archive::process_line(const string& line)
   {
      static const string burn_done("% done, estimate finish");

      istringstream parser(line);
      double percentDone;
      parser >> percentDone;
      char rest[burn_done.length() + 1];
      parser.get(rest, burn_done.length() + 1, '\n');
      if( rest == burn_done )
         set_progress(static_cast<int>(percentDone));
      NotifyBgProcess("Burn Plugin", __PRETTY_FUNCTION__, 2, static_cast<int >(percentDone));
   }

   bool chain_archive::initialize()
   {
      return prepare_burning();
   }

   bool chain_archive::prepare_burning()
   {
      string graftPoints =
      accumulate(get_recordings().begin(),
                 get_recordings().end(),
                 string(""),
                 bind( plus<string>(), _1,
                       bind( plus<string>(),
                             bind( &recording::get_graft_point, _2 ),
                             " "
                           )));
      graftPoints = format("{0} /dvd.vdr={1}/counters/standard") % graftPoints % plugin::get_config_path();

      switch( get_job().get_store_mode() ) {
         case storemode_create:
            {
               shellprocess* burn = new shellprocess( "burn", shellescape( "vdrburn-archive.sh" ) + "mkiso", get_job().get_proc_prio() );
               burn->put_environment("ISO_FILE",     get_job().get_iso_path());
               burn->put_environment("GRAFT_POINTS", graftPoints);
               burn->put_environment("DISC_ID",      get_job().get_volume_id());
               add_process(burn);
            }
            break;

         case storemode_burn:
            {
               shellprocess *burn = new shellprocess( "burn", shellescape( "vdrburn-archive.sh" ) + "burndir", get_job().get_proc_prio() );
               burn->put_environment("DVD_DEVICE",   BurnParameters.DvdDevice);
               burn->put_environment("GRAFT_POINTS", graftPoints);
               burn->put_environment("BURN_SPEED",   get_job().get_burn_speed());
               burn->put_environment("DISC_ID",      get_job().get_volume_id());
               add_process(burn);
            }
            break;

         case storemode_createburn:
            {
               string fifo = format("{0}/burnfifo") % get_paths().temp;
               make_fifo(fifo);

               shellprocess* pipe = new shellprocess( "pipe", shellescape( "vdrburn-archive.sh" ) + "pipeiso", get_job().get_proc_prio() );
               pipe->put_environment("CONFIG_PATH",  plugin::get_config_path());
               pipe->put_environment("GRAFT_POINTS", graftPoints);
               pipe->put_environment("ISO_FILE",     get_job().get_iso_path());
               pipe->put_environment("ISO_PIPE",     fifo);
               pipe->put_environment("DISC_ID",      get_job().get_volume_id());
               add_process(pipe);

               shellprocess* burn = new shellprocess( "burn", shellescape( "vdrburn-archive.sh" ) + "burniso", get_job().get_proc_prio() );
               burn->put_environment("DVD_DEVICE",   BurnParameters.DvdDevice);
               burn->put_environment("ISO_PIPE",     fifo);
               burn->put_environment("BURN_SPEED",   get_job().get_burn_speed());
               add_process(burn);
            }
            break;
      }
      return true;
   }

   bool chain_archive::prepare_recording_mark()
   {
      shellprocess* recordingmark = new shellprocess( "recordingmark", shellescape( "vdrburn-archive.sh" ) + "recordingmark", get_job().get_proc_prio() );
      recordingmark->put_environment("RECORDING_PATH", m_current->get_filename());
      recordingmark->put_environment("CONFIG_PATH", plugin::get_config_path());

      add_process(recordingmark);
      return true;
   }

   bool chain_archive::prepare_archive_mark()
   {
      shellprocess* archivemark = new shellprocess( "archivemark", shellescape( "vdrburn-archive.sh" ) + "archivemark", get_job().get_proc_prio() );
      archivemark->put_environment("CONFIG_PATH", plugin::get_config_path());

      add_process(archivemark);
      return true;
   }

   bool chain_archive::finished(const process* proc)
   {
      logger::debug("process ended: " + proc->name());
      if( proc->return_status() == process::ok ) {
         // positive return
         if( proc->name() == "burn" ) {
            if( m_current != get_job().get_recordings().end() ) {
               return prepare_recording_mark();
            }
            else {
               // this should definately not happen!
               // TODO: To think about what should be done here
               return true;
            }
         }
         else if( proc->name() == "recordingmark" ) {
            ++m_current;
            if( m_current != get_job().get_recordings().end() ) {
               return prepare_recording_mark();
            }
            else {
               return prepare_archive_mark();
            }
         }
         else if( proc->name() == "archivemark" ) {
            return true;
         }
         return true;
      }

      // process failed
      if( proc->name() == "burn" ) {
      }

      return false;
   }

}
