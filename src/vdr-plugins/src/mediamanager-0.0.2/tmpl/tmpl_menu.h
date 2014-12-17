#ifndef _MEDIA_#HANDLER#_MENU_H
#define _MEDIA_#HANDLER#_MENU_H

#include <vdr/osdbase.h>

#include "media_#handler#/#handler#_disc.h"

class cMedia#HANDLERNAME#Menu : public cOsdMenu {
  private:
  	cMedia#HANDLERNAME#Disc *disc;
	bool ejectrequest;
  public:
	cMedia#HANDLERNAME#Menu(cMedia#HANDLERNAME#Disc *Disc);
	~cMedia#HANDLERNAME#Menu();
	eOSState ProcessKey(eKeys Key);
};

#endif
