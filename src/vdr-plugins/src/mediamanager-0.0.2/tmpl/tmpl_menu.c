#include <vdr/menu.h>
#include <vdr/menuitems.h>

#include "media_#handler#/#handler#_menu.h"
#include "mediahandler.h"

//#define DEBUG_D 1
//#define DEBUG_I 1
#include "mediadebug.h"

cMedia#HANDLERNAME#Menu::cMedia#HANDLERNAME#Menu(cMedia#HANDLERNAME#Disc *Disc)
: cOsdMenu(tr("#HANDLERNAME#"))
{
	ejectrequest = false;
	disc = Disc;
}

cMedia#HANDLERNAME#Menu::~cMedia#HANDLERNAME#Menu()
{
	if(ejectrequest)
		cMediaHandler::SetHandlerEjectRequest(true);
}

eOSState cMedia#HANDLERNAME#Menu::ProcessKey(eKeys Key)
{
	eOSState state = cOsdMenu::ProcessKey(Key);
	
	if(!cMediaHandler::HandlerIsActive())
		return osEnd;

/*
	to eject disc use something like this
	see media_cdda/cdda_player.h
	and media_cdda/cdda_player.c
	case kYellow:
		if(!cMediaHandler::HandlerIsReplaying()) {
			ejectrequest = true;
			state = osEnd;
		}
		break;
*/
	return state;
}
