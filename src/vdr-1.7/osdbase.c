/*
 * osdbase.c: Basic interface to the On Screen Display
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: osdbase.c 2.3 2010/12/12 13:41:28 kls Exp $
 */

#include "osdbase.h"
#include <string.h>
#include "device.h"
#include "i18n.h"
#include "remote.h"
#include "status.h"
#ifdef REELVDR
#include "help.h"
#include "reelboxbase.h"
#endif

// --- cOsdItem --------------------------------------------------------------

cOsdItem::cOsdItem(eOSState State)
{
  text = NULL;
  state = State;
  selectable = true;
  fresh = true;
#if defined (USE_SETUP) && defined (USE_PINPLUGIN)
  subMenu = 0;
#endif /* SETUP & PINPLUGIN */
}

cOsdItem::cOsdItem(const char *Text, eOSState State, bool Selectable)
{
  text = NULL;
  state = State;
  selectable = Selectable;
  fresh = true;
  SetText(Text);
#if defined (USE_SETUP) && defined (USE_PINPLUGIN)
  subMenu = 0;
#endif /* SETUP & PINPLUGIN */
}

#if defined (USE_SETUP) && defined (USE_PINPLUGIN)
cOsdItem::cOsdItem(const char *Text, eOSState State, cSubMenuNode* SubMenu)
{
  text = NULL;
  state = State;
  selectable = true;
  fresh = true;
  SetText(Text);
  subMenu = SubMenu;
}
#endif /* SETUP & PINPLUGIN */

cOsdItem::~cOsdItem()
{
  free(text);
}

void cOsdItem::SetText(const char *Text, bool Copy)
{
  free(text);
  text = Copy ? strdup(Text ? Text : "") : (char *)Text; // text assumes ownership!
}

void cOsdItem::SetSelectable(bool Selectable)
{
  selectable = Selectable;
}

void cOsdItem::SetFresh(bool Fresh)
{
  fresh = Fresh;
}

eOSState cOsdItem::ProcessKey(eKeys Key)
{
  return Key == kOk ? state : osUnknown;
}

// --- cOsdObject ------------------------------------------------------------

void cOsdObject::Show(void)
{
  if (isMenu)
     ((cOsdMenu *)this)->Display();
}

// --- cOsdMenu --------------------------------------------------------------

cSkinDisplayMenu *cOsdMenu::displayMenu = NULL;
int cOsdMenu::displayMenuCount = 0;
int cOsdMenu::displayMenuItems = 0;//XXX dynamic???

cOsdMenu::cOsdMenu(const char *Title, int c0, int c1, int c2, int c3, int c4)
{
  isMenu = true;
  digit = 0;
#ifdef USE_LIEMIEXT
  key_nr = -1;
#endif /* LIEMIEXT */
  hasHotkeys = false;
  title = NULL;
  SetTitle(Title);
  SetCols(c0, c1, c2, c3, c4);
  first = 0;
  current = marked = -1;
  subMenu = NULL;
  helpRed = helpGreen = helpYellow = helpBlue = NULL;
#ifdef REELVDR
  helpKeys = NULL;
#endif /*REELVDR*/
  status = NULL;
  if (!displayMenuCount++)
     SetDisplayMenu();

#ifdef REELVDR
  enableSideNote = false;
#endif
}

cOsdMenu::~cOsdMenu()
{
#if REELVDR
    /* close any channel previews that were running with this menu */
    if(cReelBoxBase::Instance()) {
        cReelBoxBase::Instance()->StartPip(false);
        printf("\033[0;92mStop pip\033[0m\n");
    }
#endif

  free(title);
  delete subMenu;
  free(status);
  displayMenu->Clear();
  cStatus::MsgOsdClear();
#ifdef USE_GRAPHTFT
  cStatus::MsgOsdMenuDestroy();
#endif /* GRAPHTFT */
  if (!--displayMenuCount)
     DELETENULL(displayMenu);
}


#ifdef REELVDR

void cOsdMenu::EnableSideNote(bool On) {
    enableSideNote = On;
}

void cOsdMenu::ClearSideNote() {
    if (displayMenu) {
        displayMenu->ClearSideNote();
    }
}

void cOsdMenu::SideNote(const cRecording* Rec) {
    if(displayMenu && enableSideNote) {
        displayMenu->EnableSideNote(true);
        displayMenu->SideNote(Rec);
    }
}


void cOsdMenu::SideNote(const cStringList *StrList) {
    if(displayMenu && enableSideNote) {
        displayMenu->EnableSideNote(true);
        displayMenu->SideNote(StrList);
    }
}


void cOsdMenu::SideNote(const cEvent* Event, const cStringList* slist) {
    if(displayMenu && enableSideNote) {
        displayMenu->EnableSideNote(true);
        displayMenu->SideNote(Event, slist);
    }
}


void cOsdMenu::SideNote(const cChannel* channel) {
    if(displayMenu && enableSideNote) {
        displayMenu->EnableSideNote(true);
        displayMenu->SideNote(channel);
    }
}

void cOsdMenu::SideNoteIcons(const char* icons) {
    if(displayMenu && enableSideNote) {
        displayMenu->EnableSideNote(true);
        displayMenu->SideNoteIcons(icons);
    }
}


#endif /* REELVDR */

void cOsdMenu::SetDisplayMenu(void)
{
  if (displayMenu) {
     displayMenu->Clear();
     delete displayMenu;
     }
  displayMenu = Skins.Current()->DisplayMenu();
  displayMenuItems = displayMenu->MaxItems();
}

const char *cOsdMenu::hk(const char *s)
{
  static cString buffer;
  if (s && hasHotkeys) {
     if (digit == 0 && '1' <= *s && *s <= '9' && *(s + 1) == ' ')
        digit = -1; // prevents automatic hotkeys - input already has them
     if (digit >= 0) {
        digit++;
#ifdef USE_LIEMIEXT
        //buffer = cString::sprintf(" %2d%s %s", digit, (digit > 9) ? "" : " ", s);

        // always two spaces between number and text
        buffer = cString::sprintf(" %2d  %s", digit, s);
#else
        buffer = cString::sprintf(" %c %s", (digit < 10) ? '0' + digit : ' ' , s);
#endif /* LIEMIEXT */
        s = buffer;
        }
     }
  return s;
}

void cOsdMenu::SetCols(int c0, int c1, int c2, int c3, int c4)
{
  cols[0] = c0;
  cols[1] = c1;
  cols[2] = c2;
  cols[3] = c3;
  cols[4] = c4;
}

void cOsdMenu::SetHasHotkeys(bool HasHotkeys)
{
  hasHotkeys = HasHotkeys;
  digit = 0;
}

void cOsdMenu::SetStatus(const char *s)
{
  free(status);
  status = s ? strdup(s) : NULL;
  displayMenu->SetMessage(mtStatus, s);
}

void cOsdMenu::SetTitle(const char *Title)
{
  free(title);
  title = strdup(Title);
}

#ifdef REELVDR
void cOsdMenu::SetHelp(const char *Red, const char *Green, const char *Yellow, const char *Blue, const eKeys *Keys)
{
  helpKeys   = Keys;
#else
void cOsdMenu::SetHelp(const char *Red, const char *Green, const char *Yellow, const char *Blue)
{
#endif /*REELVDR*/
  // strings are NOT copied - must be constants!!!
  helpRed    = Red;
  helpGreen  = Green;
  helpYellow = Yellow;
  helpBlue   = Blue;
  displayMenu->SetButtons(helpRed, helpGreen, helpYellow, helpBlue);
  cStatus::MsgOsdHelpKeys(helpRed, helpGreen, helpYellow, helpBlue);
#ifdef REELVDR
  displayMenu->SetKeys(helpKeys);
#endif
}

void cOsdMenu::Del(int Index)
{
  cList<cOsdItem>::Del(Get(Index));
  int count = Count();
  while (current < count && !SelectableItem(current))
        current++;
  if (current == count) {
     while (current > 0 && !SelectableItem(current))
           current--;
     }
  if (Index == first && first > 0)
     first--;
}

void cOsdMenu::Add(cOsdItem *Item, bool Current, cOsdItem *After)
{
  cList<cOsdItem>::Add(Item, After);
  if (Current)
     current = Item->Index();
}

void cOsdMenu::Ins(cOsdItem *Item, bool Current, cOsdItem *Before)
{
  cList<cOsdItem>::Ins(Item, Before);
  if (Current)
     current = Item->Index();
}

#ifdef REELVDR
// create a new string on the heap that has a maximum 'maxlen' number of
// characters (each can span multiple bytes)
//
// tries to find end of words to break the string
// returned char* (if not NULL) must be freed by caller
//
// bytesCopied used by the caller to jump in the input string
char* BreakString(const char*str, int maxlen, int &bytesCopied)
{
	bytesCopied = 0;
	if (!str) return NULL;

	// number of chars
	int symCount = Utf8StrLen(str);

	// convert UTF-8 multi-byte char to a single uint number
	uint *inp_sym = new uint[symCount+1]; // +1 for the '\0'
	int inp_sym_len = Utf8ToArray(str, inp_sym, symCount+1);

	// number of chars in the string less than the maxlen
	// TODO: if there is a '\n' in this part!
	if (inp_sym_len <= maxlen)
	{
		// copy the input string
		bytesCopied = strlen(str);

		delete[] inp_sym;
		//printf("** just copied **\n");
		return strdup(str);
	}

	// create string on heap that can hold maxlen (multi-byte)chars
	char *s = (char*) malloc((4*maxlen+1)*sizeof(char)); //+1 for '\0'

	int i = 0;

	// if no wordbreaks found, break after maxlen characters
	int line_end = maxlen;

	// donot show the char that was used to break the string
	// for eg. ' ' is unnecessary at the end/beginning of a line
	int skip_symbol = 0;

	//the string here is longer than requested. Break it down.
	for (i = 0; i < maxlen; ++i)
	{
		//look for word breaks and new lines
		if (inp_sym[i] == '\0' || inp_sym[i] == '\n')
		{
			line_end = i;
			if (inp_sym[i] != '\0')
				skip_symbol = 1;
			break; // found end of line exit loop
		}
		else if (inp_sym[i] == ' ') //space
		{
			line_end = i;
			skip_symbol = 1;
		}
	}//for

	// create char from uint array, since the char* is large enough
	// 	it will be '\0' terminated
	bytesCopied = Utf8FromArray(inp_sym, s, 4*maxlen-1,line_end);

	if (skip_symbol)// jump over the last symbol since it is a ' ' or '\n'
		++bytesCopied;

	delete[] inp_sym;
	return s;
}

// breaks given text into smaller 'maxChar' length strings and
// displays them as unselectable text on OSD
void cOsdMenu::AddFloatingText(const char* text, int maxChars)
{
	// empty string; display nothing
	if(!text) return;

	char *p = NULL;
	int copied = 0;

	//printf("\tFloating Text (%i): '%s'\n", maxChars, text);
	while(*text)
	{
		// returns a pointer to a char string with atmost maxChars characters (not bytes)
		// p has to be freed; p is NULL if text was null
		p = BreakString(text, maxChars, copied);
		text += copied; //jump bytes not chars
		if(p)
		{
			Add(new cOsdItem(p, osUnknown,false)); // unselectable text
			free(p); p = NULL;
		}
	}
}
#endif /* REELVDR */

void cOsdMenu::Display(void)
{
  if (subMenu) {
     subMenu->Display();
     return;
     }
  displayMenu->SetMessage(mtStatus, NULL);
  displayMenu->Clear();
  cStatus::MsgOsdClear();
#ifdef USE_GRAPHTFT
  cStatus::MsgOsdMenuDisplay(MenuKind());
#endif /* GRAPHTFT */

#ifdef REELVDR
  /* EnableSideNote here, before SetTitle() since
     SetTitle draws the SideNote-background */
  displayMenu->EnableSideNote(enableSideNote);
#endif

  displayMenu->SetTabs(cols[0], cols[1], cols[2], cols[3], cols[4]);//XXX
  displayMenu->SetTitle(title);
  cStatus::MsgOsdTitle(title);
#ifdef REELVDR
  // No scrolling ONLY in Main Menu
  if (strcasestr(title, tr("Main Menu")))
      displayMenuItems = 20; //show atleast 6 icons (4 stop rec, 1 play DVD, 1 stop rec. replay) + 4  menu items in one screen without scrolling
  else
      displayMenuItems = displayMenu->MaxItems();
  // Enable SideNote Again since SetTitle() resets xItemLeft in skin
  // TODO XXX eliminate the need for second call to EnableSideNote()
  displayMenu->EnableSideNote(enableSideNote);
#endif
  displayMenu->SetButtons(helpRed, helpGreen, helpYellow, helpBlue);
  cStatus::MsgOsdHelpKeys(helpRed, helpGreen, helpYellow, helpBlue);
#ifdef REELVDR
  displayMenu->SetKeys(helpKeys);
#endif
  int count = Count();
  if (count > 0) {
     int ni = 0;
     for (cOsdItem *item = First(); item; item = Next(item)) {
         cStatus::MsgOsdItem(item->Text(), ni++);
         if (current < 0 && item->Selectable())
            current = item->Index();
         }
     if (current < 0)
        current = 0; // just for safety - there HAS to be a current item!
     first = min(first, max(0, count - displayMenuItems)); // in case the menu size has changed
     if (current - first >= displayMenuItems || current < first) {
        first = current - displayMenuItems / 2;
        if (first + displayMenuItems > count)
           first = count - displayMenuItems;
        if (first < 0)
           first = 0;
        }
     int i = first;
     int n = 0;
     for (cOsdItem *item = Get(first); item; item = Next(item)) {
         bool CurrentSelectable = (i == current) && item->Selectable();
         displayMenu->SetItem(item->Text(), i - first, CurrentSelectable, item->Selectable());
         if (CurrentSelectable)
            cStatus::MsgOsdCurrentItem(item->Text());
         if (++n == displayMenuItems)
            break;
         i++;
         }
     }
  displayMenu->SetScrollbar(count, first);
  if (!isempty(status))
     displayMenu->SetMessage(mtStatus, status);
}

void cOsdMenu::SetCurrent(cOsdItem *Item)
{
  current = Item ? Item->Index() : -1;
}

void cOsdMenu::RefreshCurrent(void)
{
  cOsdItem *item = Get(current);
  if (item)
     item->Set();
}

void cOsdMenu::DisplayCurrent(bool Current)
{
  cOsdItem *item = Get(current);
  if (item) {
     displayMenu->SetItem(item->Text(), current - first, Current && item->Selectable(), item->Selectable());
     if (Current && item->Selectable())
        cStatus::MsgOsdCurrentItem(item->Text());
     if (!Current)
        item->SetFresh(true); // leaving the current item resets 'fresh'
     }
}

void cOsdMenu::DisplayItem(cOsdItem *Item)
{
  if (Item) {
     int Index = Item->Index();
     int Offset = Index - first;
     if (Offset >= 0 && Offset < first + displayMenuItems) {
        bool Current = Index == current;
        displayMenu->SetItem(Item->Text(), Offset, Current && Item->Selectable(), Item->Selectable());
        if (Current && Item->Selectable())
           cStatus::MsgOsdCurrentItem(Item->Text());
        }
     }
}

void cOsdMenu::Clear(void)
{
  if (marked >= 0)
     SetStatus(NULL);
  first = 0;
  current = marked = -1;
  cList<cOsdItem>::Clear();
}

bool cOsdMenu::SelectableItem(int idx)
{
  cOsdItem *item = Get(idx);
  return item && item->Selectable();
}

void cOsdMenu::CursorUp(void)
{
  int tmpCurrent = current;
  int lastOnScreen = first + displayMenuItems - 1;
  int last = Count() - 1;
  if (last < 0)
     return;
  while (--tmpCurrent != current) {
        if (tmpCurrent < 0) {
           if (first > 0) {
              // make non-selectable items at the beginning visible:
              first = 0;
              Display();
              return;
              }
           if (Setup.MenuScrollWrap)
              tmpCurrent = last + 1;
           else
              return;
           }
        else if (SelectableItem(tmpCurrent))
           break;
        }
  if (first <= tmpCurrent && tmpCurrent <= lastOnScreen)
     DisplayCurrent(false);
  current = tmpCurrent;
  if (current < first) {
     first = Setup.MenuScrollPage ? max(0, current - displayMenuItems + 1) : current;
     Display();
     }
  else if (current > lastOnScreen) {
     first = max(0, current - displayMenuItems + 1);
     Display();
     }
  else
     DisplayCurrent(true);
}

void cOsdMenu::CursorDown(void)
{
  int tmpCurrent = current;
  int lastOnScreen = first + displayMenuItems - 1;
  int last = Count() - 1;
  if (last < 0)
     return;
  while (++tmpCurrent != current) {
        if (tmpCurrent > last) {
           if (first < last - displayMenuItems) {
              // make non-selectable items at the end visible:
              first = last - displayMenuItems + 1;
              Display();
              return;
              }
           if (Setup.MenuScrollWrap)
              tmpCurrent = -1;
           else
              return;
           }
        else if (SelectableItem(tmpCurrent))
           break;
        }
  if (first <= tmpCurrent && tmpCurrent <= lastOnScreen)
     DisplayCurrent(false);
  current = tmpCurrent;
  if (current > lastOnScreen) {
     first = Setup.MenuScrollPage ? current : max(0, current - displayMenuItems + 1);
     if (first + displayMenuItems > last)
        first = max(0, last - displayMenuItems + 1);
     Display();
     }
  else if (current < first) {
     first = current;
     Display();
     }
  else
     DisplayCurrent(true);
}

void cOsdMenu::PageUp(void)
{
  int oldCurrent = current;
  int oldFirst = first;
  current -= displayMenuItems;
  first -= displayMenuItems;
  int last = Count() - 1;
  if (current < 0)
     current = 0;
  if (first < 0)
     first = 0;
  int tmpCurrent = current;
  while (!SelectableItem(tmpCurrent) && --tmpCurrent >= 0)
        ;
  if (tmpCurrent < 0) {
     tmpCurrent = current;
     while (++tmpCurrent <= last && !SelectableItem(tmpCurrent))
           ;
     }
  current = tmpCurrent <= last ? tmpCurrent : -1;
  if (current >= 0) {
     if (current < first)
        first = current;
     else if (current - first >= displayMenuItems)
        first = current - displayMenuItems + 1;
     }
  if (current != oldCurrent || first != oldFirst) {
     Display();
     DisplayCurrent(true);
     }
  else if (Setup.MenuScrollWrap)
     CursorUp();
}

void cOsdMenu::PageDown(void)
{
  int oldCurrent = current;
  int oldFirst = first;
  current += displayMenuItems;
  first += displayMenuItems;
  int last = Count() - 1;
  if (current > last)
     current = last;
  if (first + displayMenuItems > last)
     first = max(0, last - displayMenuItems + 1);
  int tmpCurrent = current;
  while (!SelectableItem(tmpCurrent) && ++tmpCurrent <= last)
        ;
  if (tmpCurrent > last) {
     tmpCurrent = current;
     while (--tmpCurrent >= 0 && !SelectableItem(tmpCurrent))
           ;
     }
  current = tmpCurrent > 0 ? tmpCurrent : -1;
  if (current >= 0) {
     if (current < first)
        first = current;
     else if (current - first >= displayMenuItems)
        first = current - displayMenuItems + 1;
     }
  if (current != oldCurrent || first != oldFirst) {
     Display();
     DisplayCurrent(true);
     }
  else if (Setup.MenuScrollWrap)
     CursorDown();
}

void cOsdMenu::Mark(void)
{
  if (Count() && marked < 0) {
     marked = current;
     SetStatus(tr("Up/Dn for new location - OK to move"));
     }
}

#ifdef USE_LIEMIEXT
#define MENUKEY_TIMEOUT 1500
#endif /* LIEMIEXT */

eOSState cOsdMenu::HotKey(eKeys Key)
{
#ifdef USE_LIEMIEXT
  bool match = false;
  bool highlight = false;
  int  item_nr;
  int  i;

  if (Key == kNone) {
     if (lastActivity.TimedOut())
        Key = kOk;
     else
        return osContinue;
     }
  else {
     lastActivity.Set(MENUKEY_TIMEOUT);
     }
  for (cOsdItem *item = Last(); item; item = Prev(item)) {
#else
  for (cOsdItem *item = First(); item; item = Next(item)) {
#endif /* LIEMIEXT */
      const char *s = item->Text();
#ifdef USE_LIEMIEXT
      i = 0;
      item_nr = 0;
      if (s && (s = skipspace(s)) != '\0' && '0' <= s[i] && s[i] <= '9') {
         do {
            item_nr = item_nr * 10 + (s[i] - '0');
            }
         while ( !((s[++i] == '\t')||(s[i] == ' ')) && (s[i] != '\0') && ('0' <= s[i]) && (s[i] <= '9'));
         if ((Key == kOk) && (item_nr == key_nr)) {
#else
      if (s && (s = skipspace(s)) != NULL) {
         if (*s == Key - k1 + '1') {
#endif /* LIEMIEXT */
            current = item->Index();
            RefreshCurrent();
            Display();
            cRemote::Put(kOk, true);
#ifdef USE_LIEMIEXT
            key_nr = -1;
#endif /* LIEMIEXT */
            break;
            }
#ifdef USE_LIEMIEXT
         else if (Key != kOk) {
            if (!highlight && (item_nr == (Key - k0))) {
               highlight = true;
               current = item->Index();
               }
            if (!match && (key_nr == -1) && ((item_nr / 10) == (Key - k0))) {
               match = true;
               key_nr = (Key - k0);
               }
            else if (((key_nr == -1) && (item_nr == (Key - k0))) || (!match && (key_nr >= 0) && (item_nr == (10 * key_nr + Key - k0)))) {
               current = item->Index();
               cRemote::Put(kOk, true);
               key_nr = -1;
               break;
               }
            }
#endif /* LIEMIEXT */
         }
      }
#ifdef USE_LIEMIEXT
  if ((!match) && (Key != kNone)) {
     key_nr = -1;
     }
#endif /* LIEMIEXT */
#if REELVDR
  // RC: returning osContinue prevents the main menu and prob. others from automatic closing. side effects?
  //DDD("return osUnknown");
  return osUnknown;
#else
  return osContinue;
#endif
}

eOSState cOsdMenu::AddSubMenu(cOsdMenu *SubMenu)
{
  delete subMenu;
  subMenu = SubMenu;
#if REELVDR
  /* close any preview/pip channels when adding a submenu, since it does
     not belong to the submenu*/
  if(cReelBoxBase::Instance()) {
      cReelBoxBase::Instance()->StartPip(false);
      printf("\033[0;92mStop pip\033[0m\n");
  }

  // Clear ID3 tags and cover-art/thumbnails that are stored in skinreel3's
  // global variables,
  // new osd should set the necessary thumbnails and id3 infos itself.
#if 0 // thumbnails in install wizard were not shown!
  cPlugin *skinPlugin = cPluginManager::GetPlugin("skinreel3");
  if (skinPlugin) {
      skinPlugin->Service("setThumb", NULL);
      skinPlugin->Service("setId3Infos", NULL);
  }
#endif
#endif
  subMenu->Display();
  return osContinue; // convenience return value
}

eOSState cOsdMenu::CloseSubMenu()
{
  delete subMenu;
  subMenu = NULL;
  RefreshCurrent();
  Display();
  return osContinue; // convenience return value
}

#ifdef REELVDR
//#define SEPARATORS ":-\0"
#define SEPARATORS ":-"
///< take menu-title substrings befor one of this chars

eOSState cOsdMenu::DisplayHelpMenu(const char *Title)
{
  char title[128];
  // if we get Menu at first we assume Main Menu
  if (strstr(Title,tr("Main Menu")) == Title)
  {
     strncpy(title,tr("Main Menu"),128);
  }
  else
  {
     const char *sep =  SEPARATORS;
     while (*sep != '\0')
     {
        //printf (" \t\t --- sep %c   \n", *sep);
        char *s = NULL;
        strncpy(title,Title,128);
        title[127] = '\0';

        s = strchr(title,*sep);
        if (s)
        {
           *s = '\0';
           //break;
        }
     sep++;
     }
  }
  //cHelpSection *hs = HelpMenus.GetSectionByTitle(title);
  //return AddSubMenu(new cMenuHelp(hs, title));
}
#endif /* REELVDR */

eOSState cOsdMenu::ProcessKey(eKeys Key)
{
  if (subMenu) {
     eOSState state = subMenu->ProcessKey(Key);
     if (state == osBack)
        return CloseSubMenu();
     return state;
     }

  cOsdItem *item = Get(current);
  if (marked < 0 && item) {
     eOSState state = item->ProcessKey(Key);
     if (state != osUnknown) {
        DisplayCurrent(true);
        return state;
        }
     }
  switch (int(Key)) {
#ifdef USE_LIEMIEXT
    case kNone:
    case k0...k9: return hasHotkeys ? HotKey(Key) : osUnknown;
#else
    case k0:      return osUnknown;
    case k1...k9: return hasHotkeys ? HotKey(Key) : osUnknown;
#endif /* LIEMIEXT */
    case kUp|k_Repeat:
    case kUp:   CursorUp();   break;
    case kDown|k_Repeat:
    case kDown: CursorDown(); break;
    case kLeft|k_Repeat:
    case kLeft: PageUp(); break;
    case kRight|k_Repeat:
    case kRight: PageDown(); break;
    case kBack: return osBack;
#ifdef REELVDR
    case kInfo: return DisplayHelpMenu(title);
#endif /* REELVDR */
    case kOk:   if (marked >= 0) {
                   SetStatus(NULL);
                   if (marked != current)
                      Move(marked, current);
                   marked = -1;
                   break;
                   }
                // else run into default
    default: if (marked < 0)
                return osUnknown;
    }
  return osContinue;
}
