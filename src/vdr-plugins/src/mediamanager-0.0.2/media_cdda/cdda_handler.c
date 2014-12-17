/*
 * cdda_handler.c:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "media_cdda/cdda_handler.h"
#include "media_cdda/cdda_menu.h"
#include "mediahandler.h"

//#define DEBUG_D 1
//#define DEBUG_I 1
#include "mediadebug.h"

static const char *NAME    = "Audio CD Player";
static const char *DESCRIPTION    = "Plays digital audio discs";

cMediaCddaHandler::cMediaCddaHandler()
: cMediaHandlerObject()
{
	setSupportedMediaType(mtAudio);
	mediaShouldBeMounted(false);
	disc = NULL;
}

cMediaCddaHandler::~cMediaCddaHandler()
{
DBG_D("cMediaCddaHandler::~cMediaCddaHandler()")
	if(disc)
		delete(disc);
}

const char *cMediaCddaHandler::Description(void)
{
	return tr(DESCRIPTION);
}

const char *cMediaCddaHandler::MainMenuEntry(void)
{
	return tr(NAME);
}

cOsdObject *cMediaCddaHandler::MainMenuAction(void)
{
	return new cMediaCddaMenu(disc);
}

bool cMediaCddaHandler::Activate(bool On)
{
	if(On) {
		disc = new cMediaCddaDisc();
		if(disc->Open() == 0)
			return true;
	} else {
		if(disc) {
			delete(disc);
			disc = NULL;
		}
		return true;
	}
	return false;
}
