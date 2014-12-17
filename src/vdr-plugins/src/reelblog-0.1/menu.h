/*
 * menu.h: This reelblog plugin is a special version of the RSS Reader Plugin (Version 1.0.2) for the Video Disk Recorder (VDR).
 *
 * See the README file for copyright information and how to reach the author.
 *
 * Copyright (C) 2008 Heiko Westphal
*/

#ifndef __REELBLOG_MENU_H
#define __REELBLOG_MENU_H

#include <vdr/tools.h>
#include <vdr/menu.h>

// --- cReelblogItem(s) ------------------------------------------------------

class cReelblogItem : public cListObject {
private:
  char *title;
  char *url;
public:
  cReelblogItem(void);
  virtual ~cReelblogItem();
  bool Parse(const char *s);
  const char *Title(void) { return title; }
  const char *Url(void) { return url; }
};

class cReelblogItems : public cConfig<cReelblogItem> {
public:
  virtual bool Load(const char *filename);
};

extern cReelblogItems ReelblogItems;

// --- cReelblogMenuItem --------------------------------------------------------

class cReelblogMenuItem : public cOsdMenu {
private:
  cString text;
public:
  cReelblogMenuItem(const char *Date, const char *Title, const char *Link, const char *Description);
  virtual ~cReelblogMenuItem();
  virtual void Display(void);
  virtual eOSState ProcessKey(eKeys Key);
};

// --- cReelblogImpressumMenu -------------------------------------------------------

class cReelblogImpressumMenu: public cMenuText {
public:
  cReelblogImpressumMenu(void);
};

// --- cReelblogItemsMenu -------------------------------------------------------

class cReelblogItemsMenu: public cOsdMenu {
private:
  eOSState ShowDetails(void);
public:
  cReelblogItemsMenu(void);
  virtual eOSState ProcessKey(eKeys Key);
};

#endif // __REELBLOG_MENU_H
