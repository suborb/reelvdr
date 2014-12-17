/*
 * menu.c: This reelblog plugin is a special version of the RSS Reader Plugin (Version 1.0.2) for the Video Disk Recorder (VDR).
 *
 * See the README file for copyright information and how to reach the author.
 *
 * Copyright (C) 2008 Heiko Westphal
*/
 
#include <vdr/status.h>
#include "parser.h"
#include "menu.h"
#include "common.h"

const char *Impressum = trNOOP(\
"Responsible person for contents of reelblog in terms of press law:\n"\
"Mr. Robert M.K. Schmidt\n"\
"Feuerbachstr. 5\n"\
"D-69126 Heidelberg\n"\
"Germany\n"\
"\n"\
"\n"\
"While we endeavor to provide the most accurate, up-to-date information available, the information on this Site may be out of date or include omissions, inaccuracies or other errors. This Site and the materials therein are provided \"AS IS.\" We make no representations or warranties, either express or implied, of any kind with respect to this Site, its operations, contents, information or materials as this site is managed by a third party (s. above). WE EXPRESSLY DISCLAIM ALL WARRANTIES, EXPRESS OR IMPLIED, OF ANY KIND WITH RESPECT TO THIS SITE OR ITS USE, INCLUDING BUT NOT LIMITED TO MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.\n"\
"\n"\
"\n"\
"According Paragraph 7 sect.1 TMG Reel Multimedia is only reliable for contents provided by itself. According to Paragraph 8 to 10 TMG Reel Multimedia is not obliged to ensure legitimacy of transferred or saved data or to investigate circumstances that imply illicit actions.  Other obligations to delete or block usage or information according to applicable law still apply. We will remove any illicit contents promptly one informed about it.\n"\
"\n"\
"\n"\
"Certain content from third party vendors may be made available as part of reelblog. This content is believed to be reliable, but we do not endorse or guarantee the accuracy or completeness of this content. Should we recognize any breach of applicable laws we will remove all concerned links immediately."\
);

// ***** Globals  *************************************************************
cReelblogItems ReelblogItems;

// ***** cReelblogImpressumMenu ***********************************************

cReelblogImpressumMenu::cReelblogImpressumMenu(void)
                       :cMenuText(tr("Reelblog-Impressum"), tr(Impressum)/*, fontFix*/)
{
  printf("XX: %s\n\n\n XXX: %s\n", Impressum, tr(Impressum));
}

// ***** cReelblogItem (s) ****************************************************
cReelblogItem::cReelblogItem(void)
{ 
  title = url = NULL;
}

cReelblogItem::~cReelblogItem()
{
  free(title);
  free(url);
}

bool cReelblogItem::Parse(const char *s)
{
  const char *p = strchr(s, ':');
  if (p) {
    int l = p - s;
    if (l > 0) {
      title = MALLOC(char, l + 1);
      stripspace(strn0cpy(title, s, l + 1));
      if (!isempty(title)) {
        url = stripspace(strdup(skipspace(p + 1)));
        return true;
      }
    }
  }
  return false;
}

bool cReelblogItems::Load(const char *filename)
{
  if (cConfig<cReelblogItem>::Load(filename, true)) {
    return true;
  }
  return false;
}
// ***** cReelblogMenuItem ****************************************************
// Ausgabe von Date, Title, Link und Description in Buffer->text
// ***** cReelblogMenuItem ****************************************************
cReelblogMenuItem::cReelblogMenuItem(const char *Date, const char *Title, const char *Link, const char *Description)
:cOsdMenu(tr("Reelblog item"))
{
  text = cString::sprintf("%s%s%s%s%s",
           Date,        "\n\n",
           Title,       "\n\n",
           Description);
  Display();
}

cReelblogMenuItem::~cReelblogMenuItem()
// Freigabe &text
{
  //free(text);
}

void cReelblogMenuItem::Display(void)
{
  cOsdMenu::Display();
  DisplayMenu()->SetText(text, false);
  cStatus::MsgOsdTextItem(text);
  SetHelp(NULL, NULL, NULL, tr("Impressum"));
}

eOSState cReelblogMenuItem::ProcessKey(eKeys Key)
{
  switch (Key) {
    case kUp|k_Repeat:
    case kUp:
    case kDown|k_Repeat:
    case kDown:
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight:
         DisplayMenu()->Scroll(NORMALKEY(Key) == kUp || NORMALKEY(Key) == kLeft, NORMALKEY(Key) == kLeft || NORMALKEY(Key) == kRight);
         cStatus::MsgOsdTextItem(NULL, NORMALKEY(Key) == kUp);
         return osContinue;
    default:
         break;
  }
  eOSState state = cOsdMenu::ProcessKey(Key);
// Auswertung des Keys von cReelblogMenuItem
// Bei kOk Anzeige Item Beenden -> alle anderen Keys zurück zu cReelblogItemsMenu (osContinue)
  // TODO HelpKey
  if (state == osUnknown) {
    switch (Key) {
      case  kOk:
            return osBack;
      case kBlue:
            return AddSubMenu(new cReelblogImpressumMenu);
      default:
            state = osContinue;
    }
    state = osContinue;
  }
  return state;
}

// ***** cReelblogItemsMenu ***************************************************
// Aufbau der Feeds von http://reelblog.de über cItem
// Nach Auswahl eines Items -> ShowDetails
// Nur der erste Eintrag in reelblog.conf wird ausgewertet und muss http://reeblog.de sein
// Weitere Einträge, wie im RSS Reader, werden ignoriert
// ***** cReelblogItemsMenu ***************************************************
cReelblogItemsMenu::cReelblogItemsMenu()
:cOsdMenu(tr("Select Reelblog item"))
{
  cReelblogItem *reelblogItem = ReelblogItems.First();
  if (reelblogItem) {
    switch  (Parser.DownloadAndParse(reelblogItem->Url())) {
      case  (cParser::RSS_PARSING_OK):
            for (cItem *reelblogItem = Parser.Items.First(); reelblogItem; reelblogItem = Parser.Items.Next(reelblogItem))
              Add(new cOsdItem(reelblogItem->GetTitle()));
            Display();
            break;
      case  (cParser::RSS_PARSING_ERROR):
            Skins.Message(mtError, tr("Can't parse Reelblog item!"));
            break;
      case  (cParser::RSS_DOWNLOAD_ERROR):
            Skins.Message(mtError, tr("Can't download Reelblog item!"));
            break;
      case  (cParser::RSS_UNKNOWN_ERROR):
      default:
            Skins.Message(mtError, tr("Unknown error!"));
            break;
    }
  }
  SetHelp(NULL, NULL, NULL, tr("Impressum"));
}
eOSState cReelblogItemsMenu::ProcessKey(eKeys Key)
// Auswertung des Keys von cReelblogItemsMenu
// Bei kOk Aufruf ShowDetails -> alle anderen Keys zurück zu cReelblogItemsMenu
{
  eOSState state = cOsdMenu::ProcessKey(Key);
  if (state == osUnknown) {
    switch  (Key) {
      case  kOk:
            return ShowDetails();
      case kBlue:
            return AddSubMenu(new cReelblogImpressumMenu);
      default:
            break;
    }
    state = osContinue;
  }
  return state;
}
eOSState cReelblogItemsMenu::ShowDetails(void)
// Nach Auswahl eines cItems -> Aufruf Submenu ->cReelblogMenuItem für die Ausgabe des Feeds
{
  cItem *reelblogItem = (cItem *)Parser.Items.Get(Current());
  return AddSubMenu(new cReelblogMenuItem(reelblogItem->GetDate(), reelblogItem->GetTitle(), reelblogItem->GetLink(), reelblogItem->GetDescription()));
}
