/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: menubase.h,v 1.10 2006/09/16 18:33:36 lordjaxom Exp $
 */

#ifndef VDR_BURN_MENUBASE_H
#define VDR_BURN_MENUBASE_H

#include "jobs.h"
#include "menuitems.h"
#include "proctools/format.h"
#include <string>
#include <vdr/osdbase.h>

namespace vdr_burn
{
   namespace menu
   {

      // --- starter --------------------------------------------------------

      namespace display
      {
         enum type {
            none,
            recordings,
            job,
            status
         };

         const int count = status + 1;
         extern const std::string strings[count];
      }

      class starter: public cOsdMenu {
      public:
         starter();
         virtual ~starter();

         static void cycle_display();
         static void set_display(display::type display_);
         static std::string get_next_display();

      protected:
         eOSState show_menu();

         virtual eOSState ProcessKey(eKeys key);

      private:
         static starter* m_instance;

         display::type m_display;
         display::type m_nextDisplay;
      };

      inline
      void starter::set_display(display::type display_)
      {
         m_instance->m_nextDisplay = display_;
      }

      // --- pagebase -------------------------------------------------------

      class pagebase: public cOsdMenu {
         // XXX remove when implementing proper event handling
         friend class track_editor;

         struct user_wait {
            bool        waiting;
            bool        result;
            std::string message;
            cMutex      lock;
            cCondVar    condition;

            user_wait(): waiting(false) {}
         };

      public:
         pagebase(const std::string& title, int tab0 = 0, int tab1 = 0, int tab2 = 0);
         virtual ~pagebase() = 0;

         static bool wait_for_user(const std::string& message);

      protected:
         void set_help(const std::string& red = std::string(),
                       const std::string& green = std::string(),
                       const std::string& yellow = std::string(),
                       bool showNext = true );
         void set_current(int current);
         void display();
         void check_waiting_user();

         virtual void set_help_keys() { set_help();}
         virtual eOSState dispatch_key(eKeys key);

         virtual bool menu_closing() { return true;}
         virtual eOSState menu_update() { return osContinue;}
         virtual eOSState ok_pressed() { return osContinue;}
         virtual eOSState red_pressed() { return osContinue;}
         virtual eOSState green_pressed() { return osContinue;}
         virtual eOSState yellow_pressed() { return osContinue;}
         virtual void submenu_closing() {}

         virtual eOSState ProcessKey(eKeys key);

      private:
         static user_wait m_waitInfo;

         using cOsdMenu::Display;

         // <rant> must be constants, thanks for mentioning INSIDE THE
         // ACTUAL IMPLEMENTATION. if you want to find out what I mean,
         // look into MY impl :> </rant>
         std::string m_red;
         std::string m_green;
         std::string m_yellow;
         std::string m_blue;
      };

   } // namespace menu
} // namespace vdr_burn

#endif // VDR_BURN_MENUBASE_H
