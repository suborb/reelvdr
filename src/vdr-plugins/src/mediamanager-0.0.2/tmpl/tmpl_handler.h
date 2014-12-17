#ifndef _MEDIA_#HANDLER#_HANDLER_H
#define _MEDIA_#HANDLER#_HANDLER_H

#include <vdr/i18n.h>

#include "mediahandlerobject.h"
#include "media_#handler#/#handler#_disc.h"

class cMedia#HANDLERNAME#Handler : public cMediaHandlerObject {
  private:
	cMedia#HANDLERNAME#Disc *disc;
  public:
	cMedia#HANDLERNAME#Handler();
	~cMedia#HANDLERNAME#Handler();

	virtual const char *Description(void);
	virtual const char *MainMenuEntry(void);
	virtual cOsdObject *MainMenuAction(void);

	virtual bool Activate(bool On);
};

#endif
