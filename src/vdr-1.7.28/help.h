#ifdef REELVDR
/*
 * sources.h: Source handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: sources.h 1.4 2005/05/14 09:30:41 kls Exp $
 */

#ifndef __HELP_H
#define __HELP_H


#include <string>
#include <sstream>
#include "tools.h"
#include "font.h"
#include "status.h"
#include "osdbase.h"
#include <tinyxml/tinyxml.h>

typedef std::vector<std::string>::const_iterator  iterString;

inline std::string vStringToString(std::vector<std::string>& s)
{
  std::ostringstream o(std::ostringstream::out);
  for (iterString iter = s.begin(); iter != s.end(); ++iter)
  {
      o << *iter;
  }
  std::string tmp = o.str();
  //printf (" vStringToString ret %s ", tmp.c_str());
  return o.str();
}

class cHelp
{
public:
  cHelp();
  ~cHelp();
  void Load(std::string Filename);
  const char *Text();
  const char *Title();
private:
  int editableWidth_;
  std::vector<char> text_;
  std::vector<std::string> strText_;
  std::string title_;
};

inline const char *cHelp::Text()
{
  return &text_[0];
}

inline const char *cHelp::Title()
{
   return title_.c_str();
}

// --- class cHelpPage 
class cHelpPage : public cListObject
{
public:

cHelpPage(std::string Title, std::string Text)
  :title_(Title),text_(Text)
{
  section_ = "";
};
  const char *Title() const;
  const char *Text() const;
private:
  std::string title_;
  std::string text_;
  std::string section_;
};

inline const char *cHelpPage::Title() const
{
   return strdup(title_.c_str());
}
inline const char *cHelpPage::Text() const
{
   return strdup(text_.c_str());
}
/*
inline const char *cHelpPage::Section() const
{
   return strdup(section_.c_str());
}
*/

// --- class cHelpSection  ------

class cHelpSection : public cList<cHelpPage>, public cListObject
{
public:
  cHelpSection(const char *Section)
    :section_(Section)
  {
  };
  const char *Section() const;
  cHelpPage *GetHelpByTitle(const char *Title) const;
private:
 std::string section_;
    
};

inline const char *cHelpSection::Section() const
{
  return strdup(section_.c_str());
}

// --- class cHelpPages   ------
class cHelpPages : public cList<cHelpSection> 
{
public:
  cHelpPage *GetByTitle(const char *Title) const;
  cHelpPage *GetBySection(const char *Section) const;
  cHelpSection *GetSectionByTitle(const char *Title) const;
  ///< returns first help page of section
  bool Load();
  cHelpPages() { lastModified_ = 0; };
private:
  void ParseSection(TiXmlHandle HandleSection, int Level = 0);
  ///< recursiv parsing of xmlNodes "section" 
  void DumpHelp();
  ///< for debug purposes 
  time_t lastModified_; /** the time of the last modification on the file */
};


extern cHelpPages HelpMenus;
///< unfortunately "HelpPages" is reserved in svdrp already 

class cMenuHelp : public cOsdMenu {
private:
  char *text;
  cHelpSection *section;
  cHelpPage *helpPage;
  eDvbFont font;
public:
  cMenuHelp(cHelpSection *Section, const char *Title);
  virtual ~cMenuHelp();
  void SetText(const char *Text);
  void SetNextHelp();
  void SetPrevHelp();
  virtual void Display(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

#endif //__HELP_H
#endif /*REELVDR*/
