/*
 * cdda_handler.h:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef _MEDIA_CDDA_HANDLER_H
#define _MEDIA_CDDA_HANDLER_H

#include <vdr/i18n.h>
#include <vdr/osdbase.h>

#include "mediahandlerobject.h"
#include "media_cdda/cdda_disc.h"

class cMediaCddaHandler : public cMediaHandlerObject {
  private:
  	cMediaCddaDisc *disc;
  public:
	cMediaCddaHandler();
	~cMediaCddaHandler();

	virtual const char *Description(void);
	virtual const char *MainMenuEntry(void);
	virtual cOsdObject *MainMenuAction(void);

	virtual bool Activate(bool On);
};

#endif
