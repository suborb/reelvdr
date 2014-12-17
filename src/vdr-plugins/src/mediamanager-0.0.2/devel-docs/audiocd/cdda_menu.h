#ifndef _MEDIA_CDDA_MENU_H
#define _MEDIA_CDDA_MENU_H

#include <vdr/osdbase.h>

#include "cdda_disc.h"

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
