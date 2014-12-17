/*
 * vdrarchiv_handler.c:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include "mediahandler.h"

#include "media_vdrarchiv/vdrarchiv_handler.h"
#include "media_vdrarchiv/vdrarchiv_menu.h"

//#define DEBUG_I 1
//#define DEBUG_D 1
#include "mediadebug.h"

static const char *NAME    = "VDR-Archives";
static const char *DESCRIPTION    = "Plays VDR recordings stored on DVD";

cMediaVDRArchivHandler::cMediaVDRArchivHandler()
: cMediaHandlerObject()
{
	// set the mediatype supported by this handler
	setSupportedMediaType(mtVDRdata);

	// set if the mediatype supported by this handler should be mounted
	mediaShouldBeMounted(true);

	disc = NULL;
}

cMediaVDRArchivHandler::~cMediaVDRArchivHandler()
{
	if(disc)
		delete(disc);
}

const char *cMediaVDRArchivHandler::Description(void)
{
	return tr(DESCRIPTION);
}

const char *cMediaVDRArchivHandler::MainMenuEntry(void)
{
	return tr(NAME);
}

cOsdObject *cMediaVDRArchivHandler::MainMenuAction(void)
{
	return new cMediaVDRArchivMenu(disc);
}

bool cMediaVDRArchivHandler::Activate(bool On)
{
	if(On) {
		disc = new cMediaVDRArchivDisc();
		if(disc->ScanDisc()) {
			return true;
		}
	} else {
		if(disc) {
			delete(disc);
			disc  = NULL;
		}
		return true;
	}
	return false;
}
