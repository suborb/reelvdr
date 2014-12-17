/*
 * osdbase.h: Basic interface to the On Screen Display
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: osdbase.h 2.4 2012/04/23 09:40:07 kls Exp $
 */

#ifndef __OSDBASE_H
#define __OSDBASE_H

#include "config.h"
#include "osd.h"
#include "skins.h"
#include "tools.h"

#if defined (USE_SETUP) && defined (USE_PINPLUGIN)
#include "submenu.h"
#endif /* SETUP & PINPLUGIN */

enum eOSState { osUnknown,
                osContinue,
                osSchedule,
                osChannels,
                osTimers,
                osRecordings,
                osPlugin,
                osSetup,
                osCommands,
                osPause,
                osRecord,
                osReplay,
                osStopRecord,
                osStopReplay,
                osCancelEdit,
                osSwitchDvb,
                osBack,
                osEnd,
#ifdef REELVDR
                osEditChannels,
                osOSDSetup,
                osLanguage,
                osTimezone,
                osTimeshift,
                osActiveEvent,
                osLiveBuffer,
                osSearchtimers,
#endif /* REELVDR */
#ifdef USE_LIVEBUFFER
                osSwitchChannel,
#endif /*USE_LIVEBUFFER*/
#ifdef USE_BOUQUETS
                osBouquets,
                osActiveBouquet,
                osFavourites,
                osAddFavourite,
#endif
                os_User, // the following values can be used locally
                osUser1,
                osUser2,
                osUser3,
                osUser4,
                osUser5,
                osUser6,
                osUser7,
                osUser8,
                osUser9,
                osUser10,
              };

class cOsdItem : public cListObject {
private:
  char *text;
  eOSState state;
  bool selectable;
#if defined (USE_SETUP) && defined (USE_PINPLUGIN)
  cSubMenuNode* subMenu;
#endif /* SETUP & PINPLUGIN */
protected:
  bool fresh;
public:
  cOsdItem(eOSState State = osUnknown);
  cOsdItem(const char *Text, eOSState State = osUnknown, bool Selectable = true);
#if defined (USE_SETUP) && defined (USE_PINPLUGIN)
  cOsdItem(const char *Text, eOSState State, cSubMenuNode* SubMenu);
#endif /* SETUP & PINPLUGIN */
  virtual ~cOsdItem();
  bool Selectable(void) const { return selectable; }
  void SetText(const char *Text, bool Copy = true);
  void SetSelectable(bool Selectable);
  void SetFresh(bool Fresh);
#if defined (USE_SETUP) && defined (USE_PINPLUGIN)
  void SetSubMenu(cSubMenuNode* SubMenu) { subMenu = SubMenu; }
  cSubMenuNode* SubMenu() { return subMenu; }
#endif /* SETUP & PINPLUGIN */
  const char *Text(void) const { return text; }
  virtual void Set(void) {}
  virtual eOSState ProcessKey(eKeys Key);
  };

class cOsdObject {
  friend class cOsdMenu;
private:
  bool isMenu;
  bool needsFastResponse;
protected:
  void SetNeedsFastResponse(bool NeedsFastResponse) { needsFastResponse = NeedsFastResponse; }
public:
  cOsdObject(bool FastResponse = false) { isMenu = false; needsFastResponse = FastResponse; }
  virtual ~cOsdObject() {}
  virtual bool NeedsFastResponse(void) { return needsFastResponse; }
  bool IsMenu(void) const { return isMenu; }
  virtual void Show(void);
  virtual eOSState ProcessKey(eKeys Key) { return osUnknown; }
  };

class cOsdMenu : public cOsdObject, public cList<cOsdItem> {
#ifdef REELVDR
public:
  static cSkinDisplayMenu *displayMenu;
private:
    bool enableSideNote;
#else
private:
  static cSkinDisplayMenu *displayMenu;
#endif
  static int displayMenuCount;
  int displayMenuItems;
  char *title;
  int cols[cSkinDisplayMenu::MaxTabs];
  int first, current, marked;
  eMenuCategory menuCategory;
  cOsdMenu *subMenu;
  const char *helpRed, *helpGreen, *helpYellow, *helpBlue;
  bool helpDisplayed;
#ifdef REELVDR
  const eKeys *helpKeys;
#endif /*REELVDR*/
  char *status;
  int digit;
  bool hasHotkeys;
#ifdef USE_LIEMIEXT
  int key_nr;
  cTimeMs lastActivity;
#endif /* LIEMIEXT */
  void DisplayHelp(bool Force = false);
protected:
  void SetDisplayMenu(void);
  cSkinDisplayMenu *DisplayMenu(void) { return displayMenu; }
  const char *hk(const char *s);
  void SetCols(int c0, int c1 = 0, int c2 = 0, int c3 = 0, int c4 = 0);
  void SetHasHotkeys(bool HasHotkeys = true);
  virtual void Clear(void);
  const char *Title(void) { return title; }
  bool SelectableItem(int idx);
  void SetCurrent(cOsdItem *Item);
  void RefreshCurrent(void);
  void DisplayCurrent(bool Current);
  void DisplayItem(cOsdItem *Item);
  void CursorUp(void);
  void CursorDown(void);
  void PageUp(void);
  void PageDown(void);
  void Mark(void);
  eOSState HotKey(eKeys Key);
  eOSState AddSubMenu(cOsdMenu *SubMenu);
#ifdef REELVDR
  eOSState DisplayHelpMenu(const char *Title);
  void EnableSideNote(bool);
  void ClearSideNote();
  void SideNote(const cRecording *);
  void SideNote(const cEvent*, const cStringList* sL=NULL);
  void SideNote(const cChannel* channel=NULL);
  void SideNote(const cStringList*);
  void SideNoteIcons(const char *);
#endif /* REELVDR */
  eOSState CloseSubMenu();
  bool HasSubMenu(void) { return subMenu; }
  cOsdMenu *SubMenu(void) { return subMenu; }
  void SetStatus(const char *s);
  void SetTitle(const char *Title);
#ifdef REELVDR
  // if not NULL, Keys is an array and the last entry has to be kNone
  // as with Red, Green, Yellow and Blue, the array for Keys is _not_ copied local - so don´t use local variables!
  void SetHelp(const char *Red, const char *Green = NULL, const char *Yellow = NULL, const char *Blue = NULL, const eKeys *Keys = NULL);
  static bool HasHelpKey(const eKeys *Keys, eKeys Key) {return cSkinDisplay::HasKey(Keys, Key);}
#else
  void SetHelp(const char *Red, const char *Green = NULL, const char *Yellow = NULL, const char *Blue = NULL);
#endif /*REELVDR*/
  virtual void Del(int Index);
public:
  cOsdMenu(const char *Title, int c0 = 0, int c1 = 0, int c2 = 0, int c3 = 0, int c4 = 0);
  virtual ~cOsdMenu();
  virtual bool NeedsFastResponse(void) { return subMenu ? subMenu->NeedsFastResponse() : cOsdObject::NeedsFastResponse(); }
  void SetMenuCategory(eMenuCategory MenuCategory);
  int Current(void) const { return current; }
  void Add(cOsdItem *Item, bool Current = false, cOsdItem *After = NULL);
  void Ins(cOsdItem *Item, bool Current = false, cOsdItem *Before = NULL);
#ifdef REELVDR
  void AddFloatingText(const char* text, int maxlen); 
  // breaks the given string into smaller strings of maxlen chars and displays them as unselectable text
#endif /* REELVDR */
  virtual void Display(void);
  virtual eOSState ProcessKey(eKeys Key);
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind() { return "MenuUnknown"; }
#endif /* GRAPHTFT */
  };

#endif //__OSDBASE_H
