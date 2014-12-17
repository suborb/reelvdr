/*
 * vdrarchiv_menu.h:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef _MEDIA_VDRARCHIV_MENU_H
#define _MEDIA_VDRARCHIV_MENU_H

#include <vdr/osdbase.h>
#include "media_vdrarchiv/vdrarchiv_disc.h"

class cMediaVDRArchivMenuItem : public cOsdItem {
  private:
	cMediaVDRArchivRecInfo *info;
	const char *filename;
	const char *title;
	int track;
	bool has_info;
	bool has_summary;
	bool is_dir;
  public:
	cMediaVDRArchivMenuItem(cMediaVDRArchivRec *Recording, int Level);
	~cMediaVDRArchivMenuItem();

	const char *Title(void) { return title; }
	const char *FileName(void) { return filename; }
	int Track(void) { return track; }
	bool HasInfo(void) { return has_info; }
	bool HasSummary(void) { return has_summary; }
	bool IsDirectory(void) { return is_dir; }
	cMediaVDRArchivRecInfo *RecInfo(void) { return info; }
};

class cMediaVDRArchivMenu : public cOsdMenu {
  private:
  	cMediaVDRArchivDisc *disc;
	char *base;
	int level, helpkeys;
	bool lastreplayed_insubdir;
	bool ejectrequest;
	void SetHelpKeys(void);
  public:
	cMediaVDRArchivMenu(cMediaVDRArchivDisc *Disc,
						const char *Base = NULL, int Level = 0);
	~cMediaVDRArchivMenu();
	void Open(void);
	eOSState Play(void);
	eOSState Rewind(void);
	eOSState Info(void);
	void Set(void);
	eOSState ProcessKey(eKeys Key);
};

#endif
