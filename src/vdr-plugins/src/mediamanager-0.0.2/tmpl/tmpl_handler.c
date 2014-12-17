
#include "media_#handler#/#handler#_handler.h"
#include "media_#handler#/#handler#_menu.h"
#include "mediahandler.h"

//#define DEBUG_D 1
//#define DEBUG_I 1
#include "mediadebug.h"

static const char *NAME    = "#HANDLERNAME#";
static const char *DESCRIPTION    = "";

cMedia#HANDLERNAME#Handler::cMedia#HANDLERNAME#Handler()
: cMediaHandlerObject()
{
	// set the mediatype supported by this handler
	setSupportedMediaType(mtInvalid);

	// set if the mediatype supported by this handler should be mounted
	mediaShouldBeMounted(false);
	disc = NULL;
}

cMedia#HANDLERNAME#Handler::~cMedia#HANDLERNAME#Handler()
{
	if(disc)
		delete(disc);
}

const char *cMedia#HANDLERNAME#Handler::Description(void)
{
	return tr(DESCRIPTION);
}

const char *cMedia#HANDLERNAME#Handler::MainMenuEntry(void)
{
	return tr(NAME);
}

cOsdObject *cMedia#HANDLERNAME#Handler::MainMenuAction(void)
{
	return new cMedia#HANDLERNAME#Menu(disc);
}

bool cMedia#HANDLERNAME#Handler::Activate(bool On)
{
	if(On) {
		/* create any objects and call
			disc = new cMedia#HANDLERNAME#Disc();
			return true if all works well
		*/
	} else {
		/* delete any created objects
		   always return true
		*/
	}
	return false;
}
