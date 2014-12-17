/*
 * cdda_menu.h:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef _MEDIA_CDDA_MENU_H
#define _MEDIA_CDDA_MENU_H

#include <vdr/osdbase.h>

#include "media_cdda/cdda_disc.h"

class cMediaCddaMenu : public cOsdMenu {
  private:
  	cMediaCddaDisc *disc;
	bool ejectrequest;
  public:
	cMediaCddaMenu(cMediaCddaDisc *Disc);
	~cMediaCddaMenu();
	eOSState ProcessKey(eKeys Key);
};

#endif
