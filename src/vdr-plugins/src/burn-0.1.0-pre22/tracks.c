#include "tracks.h"
#include "adaptor.h"
#include "filter.h"
#include "setup.h"
#include "proctools/logger.h"
#include "boost/bind.hpp"
#include <algorithm>
#include <iterator>
#include <map>
#include <boost/format.hpp>
#include <vdr/i18n.h>

namespace vdr_burn
{

   using namespace std;
   using proctools::logger;
   using boost::bind;

   //!--- language codes -----------------------------------------------------

   typedef std::pair< const char*, const char* > alpha_pair;

   static alpha_pair language_code_map[] =
   { alpha_pair("eng", "en"),
      alpha_pair("deu", "de"),
      alpha_pair("ger", "de"),
      alpha_pair("slv", "sl"),
      alpha_pair("slo", "sk"),
      alpha_pair("ita", "it"),
      alpha_pair("dut", "nl"),
      alpha_pair("nla", "nl"),
      alpha_pair("nld", "nl"),
      alpha_pair("por", "pt"),
      alpha_pair("fra", "fr"),
      alpha_pair("fre", "fr"),
      alpha_pair("nor", "no"),
      alpha_pair("fin", "fi"),
      alpha_pair("smi", "fi"),
      alpha_pair("pol", "pl"),
      alpha_pair("esl", "es"),
      alpha_pair("spa", "es"),
      alpha_pair("ell", "el"),
      alpha_pair("gre", "el"),
      alpha_pair("sve", "sv"),
      alpha_pair("swe", "sv"),
      alpha_pair("rom", "ro"),
      alpha_pair("rum", "ro"),
      alpha_pair("hun", "hu"),
      alpha_pair("cat", "ca"),
      alpha_pair("cln", "ca"),
      alpha_pair("rus", "ru"),
      alpha_pair("hrv", "hr"),
      alpha_pair("est", "et"),
      alpha_pair("dan", "da"),
      alpha_pair("cze", "cs"),
      alpha_pair("ces", "cs")};

   static const size_t language_count = sizeof(language_code_map) / sizeof(language_code_map[0]);

   //!--- track_info ---------------------------------------------------------

   track_info::track_info(int cid, streamtype type):
   cid( cid ),
   language( global_setup().DefaultLanguage ),
   type( type ),
   bitrate( 0 ),
   size( 0, 0 ),
   used( 1 )
   {
   }

   void track_info::set_language(const string& language_ )
   {
      // convert 3 char code to 2 char code (ISO 639)
      typedef std::map< std::string, std::string > alpha_map;
      static alpha_map alpha( language_code_map, language_code_map + language_count );

      // to prevent that empty elements are created in the map
      alpha_map::iterator it( alpha.find( language_ ) );
      if( it == alpha.end() ) {
         logger::info( str( boost::format( "language '%s' invalid or not found in iso 639, setting default" ) % language_ ) );
         language = global_setup().DefaultLanguage;
         return;
      }

      logger::info( str( boost::format( "language code for '%s' is '%s'" ) % language_ % it->second ) );
      string_list::const_iterator listIt( find( get_language_codes().begin(), get_language_codes().end(), it->second ) );
      language = distance( get_language_codes().begin(), listIt );
   }

   const string_list& track_info::get_language_codes()
   {
      static string_list languageCodes;
      if( languageCodes.size() == 0 ) {
         languageCodes.resize( language_count );
         transform( language_code_map, language_code_map + language_count, languageCodes.begin(),
                    bind( &alpha_pair::second, _1 ) );
         languageCodes.erase( unique( languageCodes.begin(), languageCodes.end() ), languageCodes.end() );
      }
      return languageCodes;
   }

   string track_info::get_type_description() const
   {
      ostringstream result;

      switch( type ) {
         case track_info::streamtype_video:
            result << tr("Video track");
            break;

         case track_info::streamtype_audio:
            result << tr("Audio track");
            break;

         default:
            result << tr("Unknown");
      }
      if( !description.empty() )
         result << " (" << description << ")";

      return result.str();
   }

   // --- track_info_list ----------------------------------------------------

   const track_info& track_info_list::operator[](int index) const
   {
      const_iterator it( begin() );
      advance(it, index);
      return *it;
   }

   track_info& track_info_list::operator[](int index)
   {
      iterator it( begin() );
      advance(it, index);
      return *it;
   }

   track_info_list::const_iterator track_info_list::find_cid(int cid) const
   {
      return find_if( begin(), end(), bind( equal_to<int>(), bind( &track_info::cid, _1 ), cid ) );
   }

   track_info_list::iterator track_info_list::find_cid(int cid)
   {
      return find_if( begin(), end(), bind( equal_to<int>(), bind( &track_info::cid, _1 ), cid ) );
   }

}
