/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: menuitems.h,v 1.15 2006/09/19 18:51:35 lordjaxom Exp $
 */

#ifndef VDR_BURN_MENUITEMS_H
#define VDR_BURN_MENUITEMS_H

#include "common.h"
#include "i18n.h"
#include "jobs.h"
#include "tracks.h"
#include <algorithm>
#include <iterator>
#include <numeric>
#include <memory>
#include <string>
#include <vector>
#include "boost/bind.hpp"
#include <vdr/menuitems.h>

namespace vdr_burn {
   namespace menu {

      //!--- menu_item_base -----------------------------------------------------

      class menu_item_base {
      public:
         virtual bool is_editing() { return false;}

      protected:
         virtual ~menu_item_base() {}
      };

      //!--- data_edit_wrapper --------------------------------------------------

      template< class Base, typename Value, typename Data >
      class data_edit_wrapper {
         typedef Base base_type;
         typedef Value value_type;
         typedef Data data_type;

      protected:
         data_edit_wrapper( base_type* base_, value_type& value_ );

         data_type& data() { return m_data;}
         void set_value( const value_type& value_ ) { set_data( value_ );}

         eOSState process_key( eKeys key_ );

      private:
         base_type* m_base;
         value_type& m_value;
         data_type m_data;
         bool m_changed;

         void set_data( const value_type& value_ ) { m_data = value_;}
         void set_value( const data_type& data_ ) { m_value = data_;}
      };

      template< class Base, typename Value, typename Data >
      data_edit_wrapper< Base, Value, Data >::data_edit_wrapper( base_type* base_, value_type& value_ ):
      m_base( base_ ),
      m_value( value_ ),
      m_changed( false )
      {
         set_data( m_value );
      }

      template< class Base, typename Value, typename Data >
      eOSState data_edit_wrapper< Base, Value, Data >::process_key( eKeys key_ )
      {
         eOSState state( m_base->base_type::ProcessKey( key_ ) );
         if( m_base->Base::Selectable() )
            set_value( m_data );
         return state;
      }

      //!--- bool_edit_item -----------------------------------------------------

      class bool_edit_item: public data_edit_wrapper< cMenuEditBoolItem, bool, int >,
      public menu_item_base,
      public cMenuEditBoolItem {
         typedef data_edit_wrapper< cMenuEditBoolItem, bool, int > wrapper_type;

      public:
         bool_edit_item( const std::string& text_, bool& value_, const char* false_ = 0, const char* true_ = 0 );

         void set_value( bool value_ ) { wrapper_type::set_value( value_ ); Set();}

      protected:
         eOSState ProcessKey( eKeys key_ ) { return wrapper_type::process_key( key_ );}
      };

      //!--- number_edit_item ---------------------------------------------------

      class number_edit_item: public menu_item_base,
      public cMenuEditIntItem {
      public:
         number_edit_item( const std::string& text_, int& value_, int min_ = 0, int max_ = std::numeric_limits<int>::max(),
                           const char* minText_ = 0, const char* maxText_ = 0 );
      };

      //!--- string_edit_item ---------------------------------------------------

      const int max_string_edit_length = 256;
      typedef data_edit_wrapper< cMenuEditStrItem, std::string, char[ max_string_edit_length ] > string_edit_wrapper;

      template<>
      void string_edit_wrapper::set_data( const std::string& value_ );

      class string_edit_item: public data_edit_wrapper< cMenuEditStrItem, std::string, char[ max_string_edit_length ] >,
      public menu_item_base,
      public cMenuEditStrItem {
      public:
         typedef data_edit_wrapper< cMenuEditStrItem, std::string, char[ max_string_edit_length ] > wrapper_type;

         string_edit_item( const std::string& text_, std::string& value_, const char* allowed_ );

         virtual bool is_editing() { return InEditMode();}

      protected:
         eOSState ProcessKey( eKeys key_ ) { return wrapper_type::process_key( key_ );}
      };

      //!--- list_edit_item -----------------------------------------------------

      class list_edit_item : public menu_item_base,
      public cMenuEditStraItem {
      public:
         // strings must be constants
         template< size_t N >
         list_edit_item( const std::string& text_, int& value_, const char* ( &strings )[ N ], bool translate = true );

         // strings must be constants
         list_edit_item( const std::string& text_, int& value_, const string_list& strings, bool translate = true );

      private:
         const char** m_strings;

         template< size_t N >
         const char** get_strings( const char* ( &strings_ ) [ N ], bool translate_ );

         const char** get_strings( const string_list& strings_, bool translate_ );
      };

      template< size_t N >
      list_edit_item::list_edit_item( const std::string& text_, int& value_, const char* ( &strings_ )[ N ], bool translate_ ):
      cMenuEditStraItem( text_.c_str(), &value_, N, get_strings( strings_, translate_ ) )
      {
      }

      template< size_t N >
      const char** list_edit_item::get_strings( const char* ( &strings_ ) [ N ], bool translate_ )
      {
         m_strings = new const char*[ N ];
         if( translate_ )
            std::transform( strings_, strings_ + N, m_strings, boost::bind( i18n::translate, _1 ) );
         else
            std::copy( strings_, strings_ + N, m_strings );
         return m_strings;
      }

      //!--- text_item ----------------------------------------------------------

      class text_item: public menu_item_base,
      public cOsdItem {
      public:
         text_item(const std::string& text = "", bool selectable = false);

      protected:
         virtual eOSState ProcessKey(eKeys key);
      };

      //!--- size_text_item -----------------------------------------------------

      class size_text_item : public text_item {
      public:
         size_text_item( const job& job_ ): m_job( job_ ) { update();}

         void update();
         void update( bool cut_ );

      private:
         const job& m_job;
      };

      //!--- size_bar_item -----------------------------------------------------

      class size_bar_item : public text_item {
      public:
         size_bar_item( const job& job_ ): m_job( job_ ) { update();}

         void update();
         void update( bool cut_ );

      private:
         const job& m_job;
      };

      //!--- media_text_item ----------------------------------------------------

      class media_text_item : public text_item {
      public:
         //media_text_item( const job& job_ ): m_job( job_ ) { update(); } das gleiche wie  // implizit inline 
         media_text_item( const job& job_ );
         void update();

      private:
         const job& m_job;
      };

      inline media_text_item::media_text_item( const job& job_ )
      :m_job( job_ )  // c++ ctor initialisierer liste 
      {
         update(); 
      }

      //!--- job_item -----------------------------------------------------------

      class job_item: public menu_item_base,
      public cOsdItem {
      public:
         job_item(job* job_, int number, bool done = false);

         job* get_job() const { return m_job;}

      private:
         job* m_job;
      };

      //!--- recording_item -------------------------------------------------

      class recording_item: public menu_item_base,
      public cOsdItem {
      private:
         const cRecording* m_recording;
         std::string       m_name;
         int               m_total;
         int               m_new;

      public:
         recording_item(const cRecording *recording_, int level);

         void add_entry(bool isNew);

         const std::string& get_name() const { return m_name;}
         const cRecording* get_recording() const { return m_recording;}
         bool is_directory() const { return !m_name.empty();}
      };

      //!--- recording_edit_item --------------------------------------------

      class recording_edit_item: public string_edit_item {
      public:
         recording_edit_item( const recording_list::iterator recording_ );

         const recording_list::iterator get_recording() const { return m_recording;}

      private:
         const recording_list::iterator m_recording;
      };

      //!--- aspect_item --------------------------------------------------------

      typedef data_edit_wrapper< cMenuEditBoolItem, track_info::aspectratio, int > aspect_item_wrapper;

      template<>
      inline void aspect_item_wrapper::set_data( const track_info::aspectratio& aspect_ )
      {
         m_data = aspect_ == track_info::aspectratio_16_9;
      }

      template<>
      inline void aspect_item_wrapper::set_value( const int& aspect_ )
      {
         m_value = aspect_ ? track_info::aspectratio_16_9 : track_info::aspectratio_4_3;
      }

      class aspect_item: public aspect_item_wrapper,
      public menu_item_base,
      public cMenuEditBoolItem {
      public:
         aspect_item( track_info::aspectratio& aspect_ );

      protected:
         virtual eOSState ProcessKey( eKeys key_ ) { return aspect_item_wrapper::process_key( key_ );}

         static std::string get_item_name();
      };

   } // namespace menu
} // namespace vdr_burn

#endif // VDR_BURN_MENUITEMS_H
