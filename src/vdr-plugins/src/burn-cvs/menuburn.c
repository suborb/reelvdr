/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: menuburn.c,v 1.43 2006/10/24 17:47:04 lordjaxom Exp $
 */

#include "common.h"
#include "manager.h"
#include "menuburn.h"
#include "menuitems.h"
#include "setup.h"
#include "skins.h"
#include "filter.h"
#include "etsi-const.h"
#include "proctools/logger.h"
#include "proctools/format.h"
#include <algorithm>
#include <iterator>
#include <sstream>
#include <utility>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <vdr/interface.h>
#include <vdr/videodir.h>
#include <vdr/menu.h>
#include <vdr/status.h>
#include <vdr/remote.h>

namespace vdr_burn
{

   using namespace std;
   using proctools::logger;
   using proctools::process;
   using boost::bind;

   namespace menu
   {

      // --- recording_info -------------------------------------------------

      recording_info::recording_info(const cRecording* recording):
            cOsdMenu( get_recording_name( recording ).c_str() ),
            m_recording( recording )
      {
         SetCols(28);
         Display();
      }

      void recording_info::Display()
      {
         cOsdMenu::Display();
         DisplayMenu()->SetRecording( m_recording );
         cStatus::MsgOsdTextItem( m_recording->Info()->Description() );
      }

      eOSState recording_info::ProcessKey(eKeys key)
      {
         eOSState state( osUnknown );

         switch (NORMALKEY(key)) {
         case kUp:
         case kDown:
         case kLeft:
         case kRight:
            DisplayMenu()->Scroll(NORMALKEY(key) == kUp || NORMALKEY(key) == kLeft,
                             NORMALKEY(key) == kLeft || NORMALKEY(key) == kRight);
            cStatus::MsgOsdTextItem(NULL, NORMALKEY(key) == kUp);
            state = osContinue;
            break;

         case kOk:
            state = osBack;
            break;

         case kRed:
         case kGreen:
         case kYellow:
         case kBlue:
            state = osContinue;
            break;

         default:
            break;
         }

         if (state == osUnknown)
            state = cOsdMenu::ProcessKey( key );
         return state;
      }

      // --- track_editor ---------------------------------------------------

      track_editor::track_editor(pagebase& parent, recording& recording_):
            m_parent( parent ),
            m_recording( recording_ ),
            m_tracks( recording_.get_tracks() )
      {
         refresh();
      }

      void track_editor::refresh()
      {
         int current = Current();
         Clear();
	 SetCols(28);
         m_indices.clear();
         Add(new string_edit_item(tr("Title of recording"), m_recording.m_title, TitleChars));
         Add(new text_item()); // blank line

         for_each(m_tracks.begin(), m_tracks.end(),
                bind( &track_editor::add_track, this, _1 ));
         SetCurrent(Get(current == -1 ? 0 : current));
         set_help_keys();
         SetTitle(tr("Edit recording details"));

         Display();
      }

      bool track_editor::can_move_down()
      {
         track_info_list::size_type current( m_indices[Current()] );
         if (m_tracks.size() > 2 || current == m_tracks.size() - 1 || m_tracks[current].get_is_video())
            return false;
         return true;
      }

      void track_editor::set_help_keys()
      {
          menu_item_base *item = dynamic_cast<menu_item_base*> (Get(Current()));

          if (item && item->is_editing()) {
              //item is in edit mode. not drawning set help
              return;
          }

         const char* green = 0;
         if (can_move_down())
            green = tr("Move down");
         SetHelp(0, green);
      }

      void track_editor::add_track(track_info& track)
      {
         int index = m_indices.size() == 0 ? 0 : m_indices.back() + 1;

         Add( new bool_edit_item( track.get_type_description(), track.used, tr("Track$Off"), tr("Track$On") ) );
         m_indices.push_back(index);

         switch (track.type) {
         case track_info::streamtype_audio:
            Add( new list_edit_item( str( boost::format("- %1$s") % tr("Language code") ), track.language, track_info::get_language_codes() ) );
            m_indices.push_back(index);
            break;

         case track_info::streamtype_video:
            Add( new aspect_item( track.video.aspect ) );
            m_indices.push_back(index);

         default: break;
         }
      }

      eOSState track_editor::dispatch_key(eKeys key)
      {
         switch (key)
         {
         case kGreen:
            move_down();
            break;

         default:
            break;
         }

         return osContinue;
      }

      void track_editor::move_down()
      {
         if (!can_move_down())
            return;

         track_info_list::size_type current( m_indices[Current()] );
         std::swap(m_tracks[current], m_tracks[current + 1]);
         refresh();
      }

      void track_editor::Store()
      {
         m_recording.get_tracks() = m_tracks;
      }

      eOSState track_editor::ProcessKey(eKeys key)
      {
         eOSState state = cMenuSetupPage::ProcessKey(key);
         switch (state) {
         case osBack: m_parent.submenu_closing(); return state;

         case osUnknown: 
            set_help_keys();  // removes 'old' help keys from parent menu
            return dispatch_key(key);

         default: 
            set_help_keys(); 
            return state;
         }
      }

      // --- title_chooser --------------------------------------------------

      title_chooser::title_chooser(const string& fileName):
            cOsdMenu(tr("Choose title"))
      {
         string::size_type last = 0, pos;
         while ((pos = fileName.find('~', last)) != string::npos) {
            m_data.push_back(fileName.substr(last, pos - last));
            ++last;
         }
         refresh();
      }

      void title_chooser::refresh()
      {
         Clear();
         for_each(m_data.begin(), m_data.end(), bind( &title_chooser::add_title, this, _1 ));
         Display();
      }

      void title_chooser::add_title(const std::string& title)
      {
         Add(new text_item(title, true));
      }

      eOSState title_chooser::ProcessKey(eKeys key)
      {
         eOSState state = cOsdMenu::ProcessKey(key);
         return state;
      }

      // --- recordings -----------------------------------------------------
      std::string recordings::dirName = "";
      bool recordings::startedFromRecordigs = false;
      //cRecording *recordings::selectedRecording = NULL;
      std::vector<cRecording *> recordings::selectedRecordings;

      recordings::recordings():
            pagebase( "", 4, 8, 6),
            m_pathLevel(0),
            m_pathChanged(false),
            m_recordingCount(0)
      {

         if(startedFromRecordigs && !selectedRecordings.empty())
         {    
             //Skins.Message(mtStatus, tr("Analyzing recording..."));
             try {
                for (int i = 0; i < selectedRecordings.size(); i++)
                {
                    manager::get_pending().append_recording(selectedRecordings[i]); 
                } 
             } catch (const std::string& ex) 
             {
                display();
                Skins.Message( mtError, ex.c_str() );
             } 
             if(dirName != "")
             {
                 manager::get_pending().get_title() = dirName;
             }

             printf("---------vor  cRemote::Put(kRed)---------------\n");
             cRemote::Put(kRed);
             startedFromRecordigs = false;
         }

         display();
      }

      std::string recordings::get_info_text( const job& job_ )
      {
         const recording_list& recordings( job_.get_recordings() );
         if ( recordings.size() > 0 )
            return str( boost::format( tr("$Recordings: %1$d, total size: %2$.1f MB") )
                     % recordings.size() % ( double( job_.get_tracks_size() ) / MEGABYTE(1) ) );
         else
            //return tr("No recordings marked for burning"); // this text shown in "recommended media" line
             return "";
      }

      std::string recordings::get_info_bar( const job& job_ )
      {
         return progress_bar( job_.get_tracks_size() / MEGABYTE(1), job_.get_disk_size_mb(), 90 );
      }

      void recordings::display()
      {
         cThreadLock RecordingsLock(&Recordings);
         Recordings.Sort();

         int current = Current();
         Clear();
         m_recordingCount = 0;

         if (m_pathChanged) {
            current = -1;
            m_pathChanged = false;
         }

         SetTitle( str( boost::format( "%1$s - %2$s" ) % tr("Write DVD")
                     % ( m_basePath.empty() ? tr("Recordings") : m_basePath ) ).c_str() );

         job& pending = manager::get_pending();
         pending.set_recommended_disk();
         Add( new menu::text_item( get_info_text( pending ) ) );
         Add( new menu::text_item( get_info_bar( pending ) ) );
         
         recording_list& recordings = pending.get_recordings();

         char buff[1024];

         if (recordings.size() == 0)
             snprintf(buff, 1024, tr("Select recordings for burning with 'OK'") );
         else
             snprintf(buff, 1024, tr("Recommended media: %s"),
                      pending.get_disk_size_capacity() );

         Add( new menu::text_item(buff));

         if( recordings.size() > 0 ) {
            for( recording_list::iterator rec = recordings.begin(); rec != recordings.end(); ++rec ) {
               if( rec->isHD() ) {
                  Add( new menu::text_item(tr("Only archive disk possible")) );
                  break;
               }
            }
         }

         Add( new menu::text_item() );


         //add directory entries first
         recording_item *lastItem = 0;
         string lastText;

         for (cRecording *rec = Recordings.First(); rec != NULL; rec = Recordings.Next(rec)) 
         {
            string recName( rec->Name() );

            if ( manager::is_recording_queued( rec->FileName() ) )
               continue;

            if (!m_basePath.empty() &&
                  (recName.find(m_basePath) != 0 || recName[m_basePath.length()] != '~'))
               continue;

            menu::recording_item* item = new menu::recording_item(rec, m_pathLevel);
            bool added = false;
            string itemText( item->Text() );
            if (itemText.length() != 0 && (lastItem == 0 || itemText != lastText)) 
            {
                // select directory we are coming from as current item
                if (recName.find(m_lastPath) == 0 && rec->Name()[m_lastPath.length()] == '~') 
                {
                    current = Count();
                    m_lastPath.clear();
                }
                if(item->is_directory())
                {
                    Add(item);
                    added = true;
                    ++m_recordingCount;
                }
                lastText = itemText;
                lastItem = item;
            } 
            if(!added)
            {
                // do not hold pointer to a deleted memory location
                if (lastItem == item) 
                   lastItem = 0;

                delete item;
            }

            if (lastItem != 0 && lastItem->is_directory())
               lastItem->add_entry(rec->IsNew());
         }


         //add file entries
         lastItem = 0;
         lastText = "";

         for (cRecording *rec = Recordings.First(); rec != NULL; rec = Recordings.Next(rec)) 
         {
            string recName( rec->Name() );

            if ( manager::is_recording_queued( rec->FileName() ) )
               continue;

            if (!m_basePath.empty() &&
                  (recName.find(m_basePath) != 0 || recName[m_basePath.length()] != '~'))
               continue;

            menu::recording_item* item = new menu::recording_item(rec, m_pathLevel);
            bool added = false;
            string itemText( item->Text() ); 
            if (itemText.length() != 0 && (lastItem == 0 || itemText != lastText)) 
            {
                if(!item->is_directory())
                {
                    Add(item);
                    added = true;
                    ++m_recordingCount;
                }
                lastText = itemText;
            } 
            if(!added)
            {
                delete item;
            }
         }

         set_current(current);
         pagebase::display();
      }

      void recordings::set_help_keys()
      {
         if ( m_recordingCount == 0 )
            return;

	 string red, green, yellow = tr("Info");
	 if(manager::get_pending().get_recordings().size() > 0){
		red = tr("Button$Burn");
		red += " 1/3";

		// some recordings selected: so, user knows how to select;
		// do not show "Select recording with > OK <" message
		SetStatus(NULL);
	 }
	 else
	     //  no recordings selected; remind user how to select
	     SetStatus(tr("Select recording with > OK <"));
/*
         if (current != 0) {
            if (!dynamic_cast<menu::recording_item*>( current )
                  ->is_directory())
               green = tr("Mark");
         }
*/
         set_help(red, green, yellow, false);
      }

      bool recordings::menu_closing()
      {
         if (m_pathLevel == 0)
            return true;

         string newPath;
         if (m_pathLevel > 1) {
            string::size_type pos;
            if ((pos = m_basePath.rfind('~')) != string::npos)
               newPath = m_basePath.substr(0, pos);
         }
         m_lastPath = m_basePath;
         m_basePath = newPath;
         m_pathChanged = true;
         --m_pathLevel;
         display();
         return false;
      }

      eOSState recordings::ok_pressed()
      {
        try {
         recording_item* item = dynamic_cast<recording_item*>( Get(Current()) );

         if (item == 0)
            return osUnknown;

	 if (item->is_directory()) {
            ++m_pathLevel;
            ostringstream newPath;
            if (!m_basePath.empty())
               newPath << m_basePath << "~";
            newPath << item->get_name();
            m_basePath = newPath.str();
            m_pathChanged = true; 
            display();
            return osContinue;
         }

         Skins.Message(mtStatus, tr("Analyzing recording..."));
         manager::get_pending().append_recording(item->get_recording());
         display();

         if ( m_recordingCount == 0 ) {
             if ( m_pathLevel == 0 ){
                starter::set_display( display::job );
               }
            if ( menu_closing() )
                {
                return osBack;
                }
        }
       } catch (const std::string& ex) {
            display();
             Skins.Message( mtError, ex.c_str() );
        }

         return osContinue;
      }

      eOSState recordings::yellow_pressed()
      {
         recording_item* item =
               dynamic_cast<recording_item*>( Get(Current()) );

         if (item == 0)
            return osUnknown;

         if (!item->is_directory())
            return AddSubMenu( new recording_info( item->get_recording() ) );

         ++m_pathLevel;
         ostringstream newPath;
         if (!m_basePath.empty())
            newPath << m_basePath << "~";
         newPath << item->get_name();
         m_basePath = newPath.str();
         m_pathChanged = true;
         display();
         return osContinue;
      }

      eOSState recordings::red_pressed()
      {
        printf("--------eOSState recordings::red_pressed()------------\n"); 
#if 0
        try {
         recording_item* item = dynamic_cast<recording_item*>( Get(Current()) );

         if (item == 0 || item->is_directory())
            return osUnknown;

         Skins.Message(mtStatus, tr("Analyzing recording..."));
	 manager::get_pending().append_recording(item->get_recording());
	 display();
	 if ( m_recordingCount == 0 ) {
	     if ( m_pathLevel == 0 )
                starter::set_display( display::job );
            if ( menu_closing() )
                return osBack;
        }
       } catch (const std::string& ex) {
            display();
	     Skins.Message( mtError, ex.c_str() );
	}
#endif
         return osContinue;
      }

      // --- job_editor -----------------------------------------------------

      job_editor::job_editor():
            pagebase( str( boost::format( "%1$s - %2$s" ) % tr("Write DVD") % tr("Job") ), 16 ),
            m_archiveIdItem( 0 )
      {
	refresh();
      }


      void job_editor::refresh()
      {

          /* remember last selected recording (if one was selected)
            and try to make it the currently selected osditem after
            redrawing the menu */
          cOsdItem * lastItem = Get(Current());
          menu::recording_text_item* lastSelectedItem =
                  dynamic_cast<menu::recording_text_item*>(lastItem);

          std::string lastSelectedRecFilename;
          if (lastSelectedItem)
          {
              lastSelectedRecFilename =
                      lastSelectedItem->get_recording()->get_filename();
          }
          else
          {
              lastSelectedRecFilename.clear();
          }


          Clear();

          job& pending( manager::get_pending() );
          pending.set_recommended_disk();
          Add( m_infoTextItem = new menu::size_text_item( pending ) );
          Add( m_infoBarItem = new menu::size_bar_item( pending ) ); // TODO Width of menu
          Add( m_mediaTextItem = new menu::media_text_item( pending ));
          Add( new menu::text_item( "" ) );

          Add( new menu::string_edit_item( tr("DVD title"), pending.get_title(), TitleChars ) );
          Add( m_archiveIdItem = new menu::text_item( str( boost::format( "%1$s:\t%2$s" ) % tr("Archive-ID") % pending.get_archive_id() ), true ) );


          recording_list& recordings = pending.get_recordings();
          if (recordings.size() == 0) {
              show_empty_list();
              pagebase::display();
              return;
          }
          Add( new menu::text_item( "" ) );
          Add( new menu::text_item( "" ) );
          Add( new menu::text_item( tr("Recordings scheduled to be burnt are:") ) );
          Add( new menu::text_item( "" ) );

          m_recordingItems.clear();
          m_recordingItems.reserve( recordings.size() );
          for ( recording_list::iterator rec = recordings.begin(); rec != recordings.end(); ++rec ) {
              menu::recording_text_item* item;
              Add( item = new menu::recording_text_item( rec ),
                   rec->get_filename() == lastSelectedRecFilename );

              m_recordingItems.push_back( item );
          }

          /*Add( new menu::text_item( "" ) );
       Add( new menu::string_edit_item( tr("DVD title"), pending.get_title(), TitleChars ) );
       Add( m_archiveIdItem = new menu::text_item( str( boost::format( "%1$s:\t%2$s" ) % tr("Archive-ID") % pending.get_archive_id() ), true ) );*/

          menu_update();
      }



      void job_editor::show_empty_list()
      {
         while ( Count() > 4 )
            Del( 4 );
         m_recordingItems.clear();
         m_archiveIdItem = 0;

         text_item* item;
         Add( item = new text_item( tr("No recordings marked"), false ) );
         Add( new menu::text_item( "" ) );
         Add( new menu::text_item( "" ) );
         Add( new menu::text_item( "" ) );
         Add( new menu::text_item( "" ) );
         Add( item = new text_item( tr("Info:"), false ) );
         Add( item = new text_item( tr("Please press the blue key to select recording."), false ) );
         SetCurrent( item );
      }

      bool job_editor::is_recording_item( cOsdItem* item )
      {
         recording_items::const_iterator it = find( m_recordingItems.begin(), m_recordingItems.end(), item );
         return it != m_recordingItems.end();
      }

      void job_editor::set_help_keys()
      {
         string red, green, yellow;
         if ( m_recordingItems.size() > 0 ) {
            red = tr("Button$Burn"); //tr("Button$Start");
            red += " 2/3";
            //red = tr("Button$Start");

            cOsdItem* current = Get( Current() );
            if ( find( m_recordingItems.begin(), m_recordingItems.end(), Get( Current() ) ) != m_recordingItems.end() ) {
               yellow = tr("Remove");
               green  = tr("Details");
               /*
               if ( is_recording_item( current ) && current != m_recordingItems.back() && Current() < m_recordingItems.size() )
                  green = tr("Move down"); */
            }
         }
         set_help(red, green, yellow, true, tr("Button$Cancel"));
      }

      eOSState job_editor::menu_update()
      {
         m_infoTextItem->update();
         m_infoBarItem->update();
         m_mediaTextItem->update();

         if ( m_archiveIdItem != 0 )
            m_archiveIdItem->SetSelectable( manager::get_pending().get_options().DmhArchiveMode );

         display();
         return osContinue;
      }

      eOSState job_editor::ok_pressed()
      {
         cOsdItem* current = Get( Current() );

         if ( !is_recording_item( current ) )
            return osContinue;
	 job& pending = manager::get_pending();

         recording_text_item& item = static_cast< recording_text_item& >( *current );
         if ( pending.get_disk_type() >= disktype_archive )
            return osContinue;

         return AddSubMenu( new track_editor( *this, *item.get_recording() ) );
      }

      eOSState job_editor::red_pressed()
      {
         printf("---------eOSState job_editor::red_pressed()-------------\n");
         job& pending = manager::get_pending();
         recording_list& recordings = pending.get_recordings();

         if (recordings.size() == 0)
            return osContinue;

         if ( pending.get_disk_type() < disktype_archive ) {
            recording_list::const_iterator rec;
            for (rec = recordings.begin(); rec != recordings.end(); ++rec) {
               const_track_filter videoTracks( rec->get_tracks(), track_info::streamtype_video, track_predicate::used );
               if (videoTracks.begin() == videoTracks.end()) {
                  Skins.Message(mtError, tr("No video track selected!"));
                  // XXX Should be moved to job control to determine whether
                  // a video track is necessary (audio-dvd or so)
                  return osContinue;
               }

               const_track_filter audioTracks( rec->get_tracks(), track_info::streamtype_audio, track_predicate::used );
               if (audioTracks.begin() == audioTracks.end()) {
                  Skins.Message(mtError, tr("No audio track selected!"));
                  return osContinue;
               }
            }
         }

         if ( pending.get_tracks_size() * 2.1 / MEGABYTE(1) >= FreeDiskSpaceMB( BurnParameters.DataPath.c_str() ) ) {
            Skins.Message(mtError, tr("Not enough free disk space!"));
            return osContinue;
         }

         manager::start_pending();
         starter::set_display(display::status);
         return osBack;
      }

      eOSState job_editor::green_pressed()
      {
          return ok_pressed();

         cOsdItem* current = Get( Current() );
         if ( !is_recording_item( current ) || current == m_recordingItems.back() )
            return osContinue;

         recording_text_item& item = static_cast< recording_text_item& >( *current );
         recording_list::iterator thisRec( item.get_recording() );
         recording_list::iterator nextRec( thisRec );
         ++nextRec;

         iter_swap(thisRec, nextRec);
         Move( current, Next( current ) );

	 /* TB: HERE'S A BUG!!!: TODO: also swap items in m_recordingItems */

         pagebase::display();
         return osContinue;
      }

      eOSState job_editor::yellow_pressed()
      {
         cOsdItem* current = Get( Current() );
         if ( !is_recording_item( current ) || !Interface->Confirm( tr("Remove recording from list?") ) )
            return osContinue;

         job& pending = manager::get_pending();
         recording_text_item& item = static_cast< recording_text_item& >( *current );

         pending.erase_recording( item.get_recording() );

         m_recordingItems.erase( find( m_recordingItems.begin(), m_recordingItems.end(), current ) );
         Del( Current() );

         // no more selected recordings, go back to recordings list 
         // user must start selecting recordings
         if ( m_recordingItems.size() == 0 ) {

          //  show_empty_list();
             starter::set_display(display::recordings);
             return osBack;
         }

         pagebase::display();
         return osContinue;
      }      

      eOSState job_editor::blue_pressed()
      {
         cOsdItem* item;
         job& pending = manager::get_pending();

         if (m_recordingItems.size() > 0
                 && Interface->Confirm( tr("Remove all recordings from list?") ) )
         {
             // clear recordings list
             for(int i = 0;i < Count(); ++i)
             {
                 item = Get(i);
                 //printf("-------i = %d, Text = %s--\n", i, item->Text());
                 if (is_recording_item(item))
                 {
                     //job& pending = manager::get_pending();
                     recording_text_item& recitem = static_cast< recording_text_item& >( *item );
                     pending.erase_recording(recitem.get_recording());

                     m_recordingItems.erase( find( m_recordingItems.begin(), m_recordingItems.end(), item ) );
                 }
             }
         } // if

         starter::set_display(display::recordings);
         return osBack;
      }

      // --- status ---------------------------------------------------------

      status::status():
            pagebase( str( boost::format( "%1$s - %2$s" ) % tr("Write DVD") % tr("Status") ), 16 )
      {
         display();
      }

      void status::display()
      {
         int current = Current();
         Clear();

         manager::lock man_lock;

         int number = 1;
         if (manager::get_active() != 0) {
            Add(new menu::job_item(manager::get_active(), number++));
            Add(new menu::text_item());
            int percent;
            if (manager::get_active()->get_is_burning(percent))
               Add( new menu::text_item( str( boost::format( tr("$Job active (Writing: %1$d%%)") ) % percent ) ) );
            else
               Add( new menu::text_item( str( boost::format( tr("$Job active (Converting: %1$d%%)") ) % manager::get_active()->get_progress() ) ) );
               //Add(new menu::text_item());

            Add(new menu::text_item(
                  progress_bar(manager::get_active()->get_progress(),
                            100, 90)));
            Add(new menu::text_item());
            Add(new menu::text_item());
            if( manager::get_active()->get_is_burning(percent) )
               Add(new menu::text_item(tr("Job active (Writing)")));
            else
               Add(new menu::text_item(tr("Job active (Converting)")));
            //XXX Width of menu
         }
         else
            Add(new menu::text_item(tr("No active or waiting jobs present"),
                                true));

         for (job_queue::const_iterator job = manager::get_queued().begin();
             job != manager::get_queued().end(); ++job)
            Add(new menu::job_item(*job, number++));

         if (manager::get_canceled().size() > 0 && manager::get_active() == 0) {
            Add(new menu::text_item());
            Add(new menu::text_item(tr("Canceled jobs")));
            Add(new menu::text_item());

            number = 1;
            job_queue::const_iterator job;
            for (job = manager::get_canceled().begin();
                job != manager::get_canceled().end(); ++job)
               Add(new menu::job_item(*job, number++, true));
         }

         if (manager::get_erroneous().size() > 0 && manager::get_active() == 0) {
            Add(new menu::text_item());
            Add(new menu::text_item(tr("Erroneous jobs")));
            Add(new menu::text_item());

            number = 1;
            job_queue::const_iterator job;
            for (job = manager::get_erroneous().begin();
                job != manager::get_erroneous().end(); ++job) {
               Add(new menu::job_item(*job, number++, true));
            }
         }
         set_current(current);
         pagebase::display();
      }

      void status::set_help_keys()
      {
         job_item* item =
               dynamic_cast<job_item*>( Get(Current()) );

         manager::lock man_lock;

         string green, red, blue = tr("Button$Recordings");
         if (item != 0) {
            if (item->get_job()->get_is_canceled())
               green = tr("Restart");
            else if (item->get_job()->get_return_status() == process::error)
               green = tr("Retry");

            if (item->get_job() == manager::get_active())
               red = tr("Button$Cancel");
            else
               red = tr("Button$Remove");
         }
         set_help(red, green, string(), true, blue);
      }

      eOSState status::menu_update()
      {
         display();
         return osContinue;
      }

      eOSState status::green_pressed()
      {
         job_item* item =
               dynamic_cast<job_item*>( Get(Current()) );

         if (item == 0)
            return osUnknown;

         manager::lock man_lock;

         if (item->get_job() == manager::get_active())
            return osUnknown;

         if (manager::get_pending().get_recordings().size() > 0 &&
               !Interface->Confirm(tr("Replace current job?")))
            return osContinue;

         manager::replace_pending(item->get_job());
         starter::set_display(display::job);
         return osBack;
      }

      eOSState status::blue_pressed() {
          starter::set_display(display::recordings);
          return osBack;
      }

      eOSState status::red_pressed()
      {
         printf("------------eOSState status::red_pressed()------------\n");
         job_item* item =
               dynamic_cast<job_item*>( Get(Current()) );

         if (item == 0)
            return osUnknown;

         manager::lock man_lock;

         if (item->get_job() == manager::get_active()) {
            if (Interface->Confirm(tr("Job still active - really cancel?")))
               item->get_job()->stop();

            // do not show status menu after cancelling job
            starter::set_display(display::recordings);
            return osBack;
         }
         else if (Interface->Confirm(tr("Remove job from list?"))) {
            manager::erase_job(item->get_job());
            delete item->get_job();
         }
         display();
         return osContinue;
      }
   }

}
