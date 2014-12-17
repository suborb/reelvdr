/*
 * tools.c: The 'ReelNG' VDR skin
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "common.h"

#include <sstream>
#include <string.h>

#include "tools.h"

#ifdef USE_WAREAGLEICON
#include <langinfo.h>
#endif

#define AUX_HEADER_EPGSEARCH             "EPGSearch: "
#define AUX_TAGS_EPGSEARCH_START         "<epgsearch>"
#define AUX_TAGS_EPGSEARCH_ITEM_1A_START "<channel>"
#define AUX_TAGS_EPGSEARCH_ITEM_1A_END   "</channel>"
#define AUX_TAGS_EPGSEARCH_ITEM_2A_START "<searchtimer>"
#define AUX_TAGS_EPGSEARCH_ITEM_2A_END   "</searchtimer>"
#define AUX_TAGS_EPGSEARCH_ITEM_3A_START "<Search timer>"
#define AUX_TAGS_EPGSEARCH_ITEM_3A_END   "</Search timer>"
#define AUX_TAGS_EPGSEARCH_ITEM_1B_START "<update>"
#define AUX_TAGS_EPGSEARCH_ITEM_1B_END   "</update>"
#define AUX_TAGS_EPGSEARCH_ITEM_2B_START "<eventid>"
#define AUX_TAGS_EPGSEARCH_ITEM_2B_END   "</eventid>"
#define AUX_TAGS_EPGSEARCH_END           "</epgsearch>"

#define AUX_HEADER_VDRADMIN            "VDRAdmin-AM: "
#define AUX_TAGS_VDRADMIN_START        "<vdradmin-am>"
#define AUX_TAGS_VDRADMIN_ITEM1_START  "<pattern>"
#define AUX_TAGS_VDRADMIN_ITEM1_END    "</pattern>"
#define AUX_TAGS_VDRADMIN_END          "</vdradmin-am>"

#define AUX_HEADER_PIN                 "Protected: "
#define AUX_TAGS_PIN_START             "<pin-plugin>"
#define AUX_TAGS_PIN_ITEM1_START       "<protected>"
#define AUX_TAGS_PIN_ITEM1_END         "</protected>"
#define AUX_TAGS_PIN_END               "</pin-plugin>"

std::string parseaux(const char *aux)
{
  bool founditem = false;
  std::stringstream sstrReturn;
  char *start, *end;
  // check if egpsearch
  start = (char *) strcasestr(aux, AUX_TAGS_EPGSEARCH_START);
  end =   (char *) strcasestr(aux, AUX_TAGS_EPGSEARCH_END);
  if (start && end) {
    // add header
    sstrReturn << AUX_HEADER_EPGSEARCH;
    // parse first item
    char *tmp;
    if ((tmp = strcasestr(start, AUX_TAGS_EPGSEARCH_ITEM_1A_START)) != NULL) {
      if (tmp < end) {
        tmp += strlen(AUX_TAGS_EPGSEARCH_ITEM_1A_START);
        char *tmp2;
        if ((tmp2 = strcasestr(tmp, AUX_TAGS_EPGSEARCH_ITEM_1A_END)) != NULL) {
          // add channel
          sstrReturn << tr("Channel:") << " " << std::string(tmp, tmp2 - tmp);
          founditem = true;
        } else {
          founditem = false;
        }
      }
    }
    if (founditem) { // Channel tag found
      // parse second item
      if ((tmp = strcasestr(start, AUX_TAGS_EPGSEARCH_ITEM_2A_START)) != NULL) {
        if (tmp < end) {
          tmp += strlen(AUX_TAGS_EPGSEARCH_ITEM_2A_START);
          char *tmp2;
          if ((tmp2 = strcasestr(tmp, AUX_TAGS_EPGSEARCH_ITEM_2A_END)) != NULL) {
            // add separator
            if (founditem) {
              sstrReturn << ", ";
            }
            // add search item
            sstrReturn << tr("Search pattern:") << " " << std::string(tmp, tmp2 - tmp);
            founditem = true;
          } else {
            founditem = false;
          }
        }
      } else {
        // parse second item
        if ((tmp = strcasestr(start, AUX_TAGS_EPGSEARCH_ITEM_3A_START)) != NULL) {
          if (tmp < end) {
            tmp += strlen(AUX_TAGS_EPGSEARCH_ITEM_3A_START);
            char *tmp2;
            if ((tmp2 = strcasestr(tmp, AUX_TAGS_EPGSEARCH_ITEM_3A_END)) != NULL) {
              // add separator
              if (founditem) {
                sstrReturn << ", ";
              }
              // add search item
              sstrReturn << tr("Search pattern:") << " " << std::string(tmp, tmp2 - tmp);
              founditem = true;
            } else {
              founditem = false;
            }
          }
        }
      }
    }
    // timer check?
    if ((tmp = strcasestr(start, AUX_TAGS_EPGSEARCH_ITEM_1B_START)) != NULL) {
      if (tmp < end) {
        tmp += strlen(AUX_TAGS_EPGSEARCH_ITEM_1B_START);
        char *tmp2;
        if ((tmp2 = strcasestr(tmp, AUX_TAGS_EPGSEARCH_ITEM_1B_END)) != NULL) {
          if (std::string(tmp, tmp2 - tmp) != "0") {
            // add separator
            if (founditem) {
              sstrReturn << ", ";
            }
            // add search item
            sstrReturn << tr("Timer check");

            // parse second item
            if ((tmp = strcasestr(start, AUX_TAGS_EPGSEARCH_ITEM_2B_START)) != NULL) {
              if (tmp < end) {
                tmp += strlen(AUX_TAGS_EPGSEARCH_ITEM_2B_START);
                char *tmp2;
                if ((tmp2 = strcasestr(tmp, AUX_TAGS_EPGSEARCH_ITEM_2B_END)) != NULL) {
                  // add separator
                  if (founditem) {
                    sstrReturn << ", ";
                  }
                  // add search item
                  sstrReturn << "eventid=" << std::string(tmp, tmp2 - tmp);
                }
              }
            }
          } else {
            if (founditem) {
              sstrReturn << ", ";
            }
            sstrReturn << tr("No timer check");
          }
          founditem = true;
        } else {
          founditem = false;
        }
      }
    }

    // use old syntax
    if (!founditem) {
      start += strlen(AUX_HEADER_EPGSEARCH);
      sstrReturn << std::string(start, end - start);
    }
    sstrReturn << std::endl;
  }
  // check if VDRAdmin-AM
  start = (char *) strcasestr(aux, AUX_TAGS_VDRADMIN_START);
  end   = (char *) strcasestr(aux, AUX_TAGS_VDRADMIN_END);
  if (start && end) {
    // add header
    sstrReturn << AUX_HEADER_VDRADMIN;
    // parse first item
    char *tmp;
    if ((tmp = strcasestr(start, AUX_TAGS_VDRADMIN_ITEM1_START)) != NULL) {
      if (tmp < end) {
        tmp += strlen(AUX_TAGS_VDRADMIN_ITEM1_START);
        char *tmp2;
        if ((tmp2 = strcasestr(tmp, AUX_TAGS_VDRADMIN_ITEM1_END)) != NULL) {
          // add search item
          sstrReturn << std::string(tmp, tmp2 - tmp) << std::endl;
        }
      }
    }
  }
  // check if pin
  start = (char *) strcasestr(aux, AUX_TAGS_PIN_START);
  end   = (char *) strcasestr(aux, AUX_TAGS_PIN_END);
  if (start && end) {
    // add header
    sstrReturn << AUX_HEADER_PIN;
    // parse first item
    char *tmp;
    if ((tmp = strcasestr(start, AUX_TAGS_PIN_ITEM1_START)) != NULL) {
      if (tmp < end) {
        tmp += strlen(AUX_TAGS_PIN_ITEM1_START);
        char *tmp2;
        if ((tmp2 = strcasestr(tmp, AUX_TAGS_PIN_ITEM1_END)) != NULL) {
          // add search item
          sstrReturn << std::string(tmp, tmp2 - tmp) << std::endl;
        }
      }
    }
  }

  if (!sstrReturn.str().empty())
    return sstrReturn.str();

  return std::string(aux);
}

bool ischaracters(const char *str, const char *mask)
{
  bool match = true;
  const char *p = str;
  for (; *p; ++p) {
    const char *m = mask;
    bool tmp = false;
    for (; *m; ++m) {
      if (*p == *m)
        tmp = true;
    }
    match = match && tmp;
  }
  return match;
}

bool IsProgressBarStr(const char*s)
{
  if ( !s) return false;
  if ( strlen(s) < 3 ) return false;
  
  int len = strlen (s);
  if ( s[0] != '[' || s[len-1] != ']') return false;
  
  enum { PIPE_MODE, SPACE_MODE};

  bool earlier_char_was_pipe = true;
  int i = 1;
  while (i < len -1 )
  {
    switch (s[i])
    {
    case ' ':
          earlier_char_was_pipe = false;
      break;

    case '|':
          if ( !earlier_char_was_pipe ) return false;
      break;

    default:
      return false;
    }// switch

  ++i;  
  } // while
  return true;
}

#ifdef USE_WAREAGLEICON
// --- Icons ------------------------------------------------------------------
bool Icons::IsUTF8=false;

void Icons::InitCharSet()
{
  // Taken from VDR's vdr.c
  char *CodeSet=NULL;
  if(setlocale(LC_CTYPE, ""))
    CodeSet=nl_langinfo(CODESET);
  else
  {
    char *LangEnv=getenv("LANG"); // last resort in case locale stuff isn't installed
    if(LangEnv)
    {
      CodeSet=strchr(LangEnv,'.');
      if(CodeSet)
        CodeSet++; // skip the dot
    }
  }

  if(CodeSet && strcasestr(CodeSet,"UTF-8")!=0)
    IsUTF8=true;
}
#endif

// vim:et:sw=2:ts=2:
