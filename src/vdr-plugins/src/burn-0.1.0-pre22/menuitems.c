/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: menuitems.c,v 1.17 2006/10/24 17:47:04 lordjaxom Exp $
 */

#include "menuitems.h"
#include "jobs.h"
#if APIVERSNUM < 10507
#include "i18n.h"
#endif
#include "common.h"
#include <algorithm>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <vdr/remote.h>
#include <vdr/status.h>
#include <ctype.h>

#define SHIFTTIMEOUT 2

namespace vdr_burn
{
   namespace menu
   {

      using namespace std;
      using proctools::format;
      using boost::bind;

      //!--- bool_edit_item -------------------------------------------------

      bool_edit_item::bool_edit_item( const std::string& text_, bool& value_, const char* false_, const char* true_ ):
      wrapper_type( this, value_ ),
      cMenuEditBoolItem( text_.c_str(), &wrapper_type::data(), false_, true_ )
      {
      }

      //! --- list_edit_item ------------------------------------------------

      list_edit_item::list_edit_item( const std::string& text_, int& value_, const string_list& strings_, bool translate_ ):
      cMenuEditStraItem( text_.c_str(), &value_, strings_.size(), get_strings( strings_, translate_ ) )
      {
      }

      const char** list_edit_item::get_strings( const string_list& strings_, bool translate_ )
      {
         m_strings = new const char*[ strings_.size() ];
         if( translate_ )
            std::transform( strings_.begin(), strings_.end(), m_strings,
                            boost::bind( &i18n::translate, boost::bind( &std::string::c_str, _1 ) ) );
         else
            std::transform( strings_.begin(), strings_.end(), m_strings,
                            boost::bind( &std::string::c_str, _1 ) );
         return m_strings;
      }

      //! --- number_edit_item -----------------------------------------------

      number_edit_item::number_edit_item( const std::string& text_, int& value_, int min_, int max_,
                                          const char* minText_, const char* maxText_ ):
      cMenuEditIntItem( text_.c_str(), &value_, min_, max_, minText_, maxText_ )
      {
      }

      //!--- string_edit_item ---------------------------------------------------

      template<>
      void string_edit_wrapper::set_data( const std::string& value_ )
      {
         strncpy( m_data, value_.c_str(), max_string_edit_length );
         m_data[ max_string_edit_length - 1 ] = '\0';
      }

      string_edit_item::string_edit_item( const std::string& text_, std::string& value_, const char* allowed_ ):
      wrapper_type( this, value_ ),
      cMenuEditStrItem( text_.c_str(), wrapper_type::data(), max_string_edit_length, allowed_ )
      {
      }

      //!--- text_item ----------------------------------------------------------

      text_item::text_item(const std::string& text, bool selectable):
      cOsdItem( text.c_str() )
      {
         SetSelectable(selectable);
      }

      eOSState text_item::ProcessKey(eKeys key)
      {
         eKeys plainKey( static_cast<eKeys>(key & ~k_Flags) );
         return plainKey == kLeft || plainKey == kRight
         ? osContinue
         : cOsdItem::ProcessKey(key);
      }

      //!--- size_text_item -----------------------------------------------------

      void size_text_item::update()
      {
         update( m_job.get_options().CutOnDemux );
      }

      void size_text_item::update( bool cut_ )
      {
         SetText( str( boost::format( tr("$Total size: %1$.1f MB %2$s") )
                       % ( double( m_job.get_tracks_size( cut_ ) ) / MEGABYTE(1) )
                       % ( m_job.get_requant_factor( cut_ ) > 1 ? tr("(would be shrinked)") : "" ) ).c_str() );
      }

      //!--- size_bar_item -----------------------------------------------------

      void size_bar_item::update()
      {
         update( m_job.get_options().CutOnDemux );
      }

      //!--- disk size ----------------------------------------------------

      void size_bar_item::update( bool cut_ )
      {
         SetText( progress_bar( m_job.get_tracks_size( cut_ ) / MEGABYTE(1), m_job.get_disk_size_mb(), 90 ).c_str() );
      }

      //!--- media_text_item ----------------------------------------------------

      void media_text_item::update( )
      {
         char buff[1024];
         snprintf(buff, 1024, tr("Recommended media: %s"), m_job.get_disk_size_capacity());
         SetText( buff );
      }

      //!--- job_item -------------------------------------------------------

      job_item::job_item(job* job_, int number, bool done):
      cOsdItem(),
      m_job(job_)
      {
         SetText(cString::sprintf("%d. %s", number, m_job->get_title().c_str()));
      }

      //!--- recording_item -------------------------------------------------

      recording_item::recording_item(const cRecording* recording_, int level):
      cOsdItem(get_recording_title(recording_, level).c_str()),
      m_recording(recording_),
      m_total(0),
      m_new(0)
      {
         if( level < recording_->HierarchyLevels() )
            m_name = Text() + 2;
      }

      void recording_item::add_entry(bool isNew)
      {
         ++m_total;
         if( isNew )
            ++m_new;
         SetText(cString::sprintf("%c\t  %d (%d)\t\t%s", char(130), m_total, m_new, m_name.c_str()));
      }

      // --- recording_edit_item --------------------------------------------

      recording_edit_item::recording_edit_item( const recording_list::iterator recording_ ):
      string_edit_item( recording_->get_datetime().c_str(), recording_->m_title, TitleChars ),
      m_recording( recording_ )
      {
      }

      //!--- aspect_item --------------------------------------------------------

      aspect_item::aspect_item( track_info::aspectratio& aspect_ ):
      aspect_item_wrapper( this, aspect_ ),
      cMenuEditBoolItem( get_item_name().c_str(), &aspect_item_wrapper::data(), "4:3", "16:9" )
      {
      }

      string aspect_item::get_item_name()
      {
         return string( "- " ) + tr("Aspect ratio");
      }

   } // namespace menu
} // namespace vdr_burn
