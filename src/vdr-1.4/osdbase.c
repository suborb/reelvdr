/*
 * osdbase.c: Basic interface to the On Screen Display
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: osdbase.c 1.29 2006/02/05 14:37:03 kls Exp $
 */

#include "osdbase.h"
#include <string.h>
#include "device.h"
#include "i18n.h"
#include "remote.h"
#include "status.h"
#include "menu.h" // cMenuHelp

// --- cOsdItem --------------------------------------------------------------

cOsdItem::cOsdItem(eOSState State)
{
  text = NULL;
  state = State;
  selectable = true;
  fresh = true;
}

cOsdItem::cOsdItem(const char *Text, eOSState State, bool Selectable)
{
  text = NULL;
  state = State;
  selectable = Selectable;
  fresh = true;
  SetText(Text);
}

cOsdItem::~cOsdItem()
{
  free(text);
}

void cOsdItem::SetText(const char *Text, bool Copy)
{
  free(text);
  text = Copy ? strdup(Text) : (char *)Text; // text assumes ownership!
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
  key_nr = -1;
  hasHotkeys = false;
  title = NULL;
  SetTitle(Title);
  SetCols(c0, c1, c2, c3, c4);
  first = 0;
  current = marked = -1;
  subMenu = NULL;
  helpRed = helpGreen = helpYellow = helpBlue = NULL;
  status = NULL;
  if (!displayMenuCount++)
     SetDisplayMenu();
}

cOsdMenu::~cOsdMenu()
{
  free(title);
  delete subMenu;
  free(status);
  displayMenu->Clear();
  cStatus::MsgOsdClear();
  if (!--displayMenuCount)
     DELETENULL(displayMenu);
}

void cOsdMenu::SetDisplayMenu(void)
{
  if (displayMenu) {
     displayMenu->Clear();
     delete displayMenu;
     }
  displayMenu = Skins.Current()->DisplayMenu();
  displayMenuItems = displayMenu->MaxItems();
  //Display();
}

const char *cOsdMenu::hk(const char *s)
{
  static cString buffer;
  if (s && hasHotkeys) {
     if (digit == 0 && '1' <= *s && *s <= '9' && *(s + 1) == ' ')
        digit = -1; // prevents automatic hotkeys - input already has them
     if (digit >= 0) {
        digit++;
        //buffer = cString::sprintf(" %c %s", (digit < 10) ? '0' + digit : ' ' , s);
        buffer = cString::sprintf(" %2d%s %s", digit, ( digit > 9 )? "" :" " , s);
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

void cOsdMenu::SetHasHotkeys(void)
{
  hasHotkeys = true;
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

void cOsdMenu::SetHelp(const char *Red, const char *Green, const char *Yellow, const char *Blue)
{
  // strings are NOT copied - must be constants!!!
  helpRed    = Red;
  helpGreen  = Green;
  helpYellow = Yellow;
  helpBlue   = Blue;
  displayMenu->SetButtons(helpRed, helpGreen, helpYellow, helpBlue);
  cStatus::MsgOsdHelpKeys(helpRed, helpGreen, helpYellow, helpBlue);
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

void cOsdMenu::Display(void)
{
  //printf("------------cOsdMenu::Display-----------------\n");
  if (subMenu) {
     subMenu->Display();
     return;
     }
  displayMenu->SetMessage(mtStatus, NULL);
  //printf("cOsdMenu::Display, vor----------cStatus::MsgOsdClear()----------\n");
  displayMenu->Clear();
  cStatus::MsgOsdClear(); // comment out to avoid flickering of graphlcd display

  displayMenu->SetTabs(cols[0], cols[1], cols[2], cols[3], cols[4]);//XXX
  displayMenu->SetTitle(title);

  // No scrolling in Main Menu ONLY
  if (strcasestr(title, tr("Main Menu")))
      displayMenuItems = 20;
  else
      displayMenuItems = displayMenu->MaxItems(); // This is necessary if the lineHeight differs in pages

  cStatus::MsgOsdTitle(title);
  displayMenu->SetButtons(helpRed, helpGreen, helpYellow, helpBlue);
  cStatus::MsgOsdHelpKeys(helpRed, helpGreen, helpYellow, helpBlue);
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
     displayMenu->SetScrollbar(count, first);
     }
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

#define MENUKEY_TIMEOUT 1500

eOSState cOsdMenu::HotKey(eKeys Key)
{
  bool match = false;
  bool highlight = false;
  int  item_nr;
  int  i;

  if (Key == kNone) {
     if (lastActivity.TimedOut())
        Key = kOk;
     else
        return osUnknown;
     }
  else {
     lastActivity.Set(MENUKEY_TIMEOUT);
     }
  for (cOsdItem *item = Last(); item; item = Prev(item)) {
      const char *s = item->Text();
      i = 0;
      item_nr = 0;
      if (s && (s = skipspace(s)) != '\0' && '0' <= s[i] && s[i] <= '9') {
         do {
            item_nr = item_nr * 10 + (s[i] - '0');
            }
         while ( !((s[++i] == '\t')||(s[i] == ' ')) && (s[i] != '\0') && ('0' <= s[i]) && (s[i] <= '9'));
         if ((Key == kOk) && (item_nr == key_nr)) {
            current = item->Index();
            cRemote::Put(kOk, true);
            key_nr = -1;
            break;
            }
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
         }
      }
  if ((!match) && (Key != kNone)) {
     key_nr = -1;
     }
  return match ? osContinue : osUnknown;
}

eOSState cOsdMenu::AddSubMenu(cOsdMenu *SubMenu)
{
  delete subMenu;
  subMenu = SubMenu;
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
  cHelpSection *hs = HelpMenus.GetSectionByTitle(title);
  return AddSubMenu(new cMenuHelp(hs, title));
}


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
  switch (Key) {
    case kNone:
    case k0...k9: return hasHotkeys ? HotKey(Key) : osUnknown;
    case kUp|k_Repeat:
    case kUp:   CursorUp();   break;
    case kDown|k_Repeat:
    case kDown: CursorDown(); break;
    case kLeft|k_Repeat:
    case kLeft: PageUp(); break;
    case kRight|k_Repeat:
    case kRight: PageDown(); break;
    case kBack: return osBack;
    case kInfo: return DisplayHelpMenu(title);
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

