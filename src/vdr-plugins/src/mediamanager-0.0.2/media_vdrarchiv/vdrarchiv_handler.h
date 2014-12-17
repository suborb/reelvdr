/*
 * vdrarchiv_handler.h:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef _MEDIA_VDRARCHIV_HANDLER_H
#define _MEDIA_VDRARCHIV_HANDLER_H

#include <vdr/i18n.h>

#include "mediahandlerobject.h"
#include "media_vdrarchiv/vdrarchiv_disc.h"

class cMediaVDRArchivHandler : public cMediaHandlerObject {
  private:
	cMediaVDRArchivDisc *disc;
  public:
	cMediaVDRArchivHandler();
	~cMediaVDRArchivHandler();

	virtual const char *Description(void);
	virtual const char *MainMenuEntry(void);
	virtual cOsdObject *MainMenuAction(void);

	virtual bool Activate(bool On);
};

#endif
