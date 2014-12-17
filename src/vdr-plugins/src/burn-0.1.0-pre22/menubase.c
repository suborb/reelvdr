/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: menubase.c,v 1.8 2006/09/16 18:33:36 lordjaxom Exp $
 */

#include "menubase.h"
#include "manager.h"
#include "menuburn.h"
#include "menuitems.h"
#include "proctools/logger.h"
#include <vdr/interface.h>

namespace vdr_burn
{

   using namespace std;
   using proctools::logger;

   namespace menu
   {
      namespace display
      {
         const string strings[count] =
         { string( ), tr("Recordings"), tr("Burn"), tr("Status")};
      }

      // --- starter --------------------------------------------------------

      starter* starter::m_instance = 0;

      starter::starter():
      cOsdMenu("")
      {
         m_instance = this;
         if( manager::get_is_busy() )
            m_nextDisplay = display::status;
         else if( manager::get_pending().get_recordings().size() > 0 )
            m_nextDisplay = display::job;
         else
            m_nextDisplay = display::recordings;
         show_menu();
      }

      starter::~starter()
      {
         m_instance = 0;
      }

      void starter::cycle_display()
      {
         m_instance->m_nextDisplay =
         static_cast<display::type>((m_instance->m_display + 1) % display::count);
         if( m_instance->m_nextDisplay == display::none )
            m_instance->m_nextDisplay = display::recordings;
      }

      string starter::get_next_display()
      {
         display::type next(
                           static_cast<display::type>((m_instance->m_display + 1) % display::count) );
         if( next == display::none )
            next = display::recordings;
         return tr(display::strings[next].c_str());
      }

      eOSState starter::show_menu()
      {
         m_display = m_nextDisplay;
         m_nextDisplay = display::none;

         switch( m_display ) {
            case display::recordings:
               return AddSubMenu(new menu::recordings);

            case display::job:
               return AddSubMenu(new menu::job_editor);

            case display::status:
               return AddSubMenu(new menu::status);

            default:
               return osBack;
         }
      }

      eOSState starter::ProcessKey(eKeys key)
      {
         bool hadSubMenu = HasSubMenu();
         eOSState state = cOsdMenu::ProcessKey(key);

         if( hadSubMenu && !HasSubMenu() )
            return show_menu();
         return state;
      }

      // --- pagebase -------------------------------------------------------

      pagebase::user_wait pagebase::m_waitInfo;

      pagebase::pagebase(const string& title, int tab0, int tab1, int tab2):
      cOsdMenu(title.c_str(), tab0, tab1, tab2)
      {
      }

      pagebase::~pagebase()
      {
      }

      void pagebase::set_help(const string& red, const string& green,
                              const string& yellow, bool showNext)
      {
         // <rant> must be constants, which is mentioned inside the impl,
         // not any documentation or so... well we assume, since we are in
         // the foreground thread, that the constants are not used anymore
         // so it is safe to replace the strings here before giving the new
         // constants to the core
         m_red = red;
         m_green = green;
         m_yellow = yellow;
         if( showNext )
            m_blue = starter::get_next_display();

         SetHelp(m_red.empty() ? 0 : m_red.c_str(),
                 m_green.empty() ? 0 : m_green.c_str(),
                 m_yellow.empty() ? 0 : m_yellow.c_str(),
                 showNext ? m_blue.c_str() : 0 );
      }

      void pagebase::set_current(int current)
      {
         if( current == -1 || Get(current) == 0 ) {
            current = 0;
            while( Get(current) != 0 && !Get(current)->Selectable() )
               ++current;
         }
         if( Get(current) != 0 )
            SetCurrent(Get(current));
      }

      void pagebase::display()
      {
         Display();
         set_help_keys();
      }

      eOSState pagebase::dispatch_key(eKeys key)
      {
         switch( key ) {
            case kOk:     
               return ok_pressed();
            case kRed:    
               return red_pressed();
            case kGreen:  
               return green_pressed();
            case kYellow: 
               return yellow_pressed();
            case kBlue:
               if( starter::get_next_display() != tr("Burn") || manager::get_pending().get_recordings().size() > 0 )
                  starter::cycle_display();
               return osBack;
            case kNone:   
               check_waiting_user();
               return osContinue; //return menu_update();
            default:      
               break;
         }

         return osUnknown;
      }

      eOSState pagebase::ProcessKey(eKeys key)
      {
         bool hadSubMenu = HasSubMenu();
         if( !hadSubMenu && key == kBack && !menu_closing() )
            return osContinue;

         int current = Current();
         eOSState state = cOsdMenu::ProcessKey(key);

         if( hadSubMenu && HasSubMenu() )
            return state;

         if( state == osUnknown )
            state = dispatch_key(key);
         else if( state == osContinue &&
                  (current != Current() || key == kOk || key == kBack) )
            set_help_keys();

         if( current >= Count() )
            return state;

         menu_item_base& item = dynamic_cast< menu_item_base& >( *Get( current ) );
         if( !item.is_editing() )
            menu_update();

         return state;
      }

      bool pagebase::wait_for_user(const std::string& message)
      {
         logger::debug("wait_for_user");
         cMutexLock lock(&m_waitInfo.lock);

         m_waitInfo.waiting = true;
         m_waitInfo.message = message;
         m_waitInfo.condition.Wait(m_waitInfo.lock);
         logger::debug("finished waiting");
         m_waitInfo.waiting = false;

         return m_waitInfo.result;
      }

      void pagebase::check_waiting_user()
      {
         cMutexLock lock(&m_waitInfo.lock);

         if( m_waitInfo.waiting ) {
            eKeys k = Skins.Message(mtWarning, m_waitInfo.message.c_str(), 36);
            if( k != kNone ) {
               m_waitInfo.result = ( k == kOk );
               m_waitInfo.condition.Broadcast();
               if( k == kOk && (m_waitInfo.message == tr("Job still active - really cancel?")) ) {
                  starter::cycle_display();
               }
            }
         }
      }
   }
}
