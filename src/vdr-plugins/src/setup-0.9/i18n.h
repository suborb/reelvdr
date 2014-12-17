/*********************************************************
 * DESCRIPTION:
 *             Header File
 *
 * $Id: i18n.h,v 1.2 2005/01/09 14:30:37 ralf Exp $
 *
 * Contact:    ranga@teddycats.de
 *
 * Copyright (C) 2004 by Ralf Dotzert
 *********************************************************/

#ifndef _I18N__H
#define _I18N__H

#if VDRVERSNUM < 10727
#include <vdr/i18n.h>
#endif
#include <tinyxml/tinyxml.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>



extern const tI18nPhrase *Phrases;
// ------- Class LoadXmlPhrases  ---------------------------------------------------

class LoadXmlPhrases
{
  public:
    LoadXmlPhrases();
    int GetLangIndex(const char *lang_abr);
    void PushPhrase(std::string entryStr, std::string stranslatedEntryStr,
                    int idx);
    void AddLanguagePhrases(TiXmlElement * langElement);
    void AddDefaultPhrases();
    void LoadPhrases();

    void DumpMap();             // XML Map
    void DumpPhrases();         // the finishd phrases array
    void DumpDefaultPhrases();
  private:

};

#endif //_I18N__H
