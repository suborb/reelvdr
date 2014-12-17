/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: i18n.h,v 1.8 2006/09/16 18:33:36 lordjaxom Exp $
 */

#ifndef VDR_BURN_I18N_H
#define VDR_BURN_I18N_H

#include "common.h"
#include <vdr/i18n.h>

namespace vdr_burn
{

   // --- i18n ---------------------------------------------------------------

   // Implemented as a Meyers-Singleton
   class i18n {
   private:
      static const tI18nPhrase m_phrases[];

      int m_osdLanguage;

   protected:
      static i18n& get();

   public:
      i18n();

      static const tI18nPhrase* get_phrases() { return m_phrases;}

      static const char* translate( const char* text );
   };

   inline
   const char* i18n::translate( const char* text )
   {
      return tr(text);
   }

}

#endif // VDR_BURN_I18N_H
