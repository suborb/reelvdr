/*
 * mediahandler.h:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef _MEDIA_HANDLER_H
#define _MEDIA_HANDLER_H

#include <vdr/thread.h>
#include "mediahandlerobject.h"
#include "mediapoller.h"

class cMediaHandlerMenu;

class cMediaHandler : public cThread, public cList<cMediaHandlerObject> {
	friend class cMediaHandlerMenu;
  private:
	cMediaHandlerObject *current;
	cMediaPoller *poller;
	eMediaType mediatype;
	cMutex listMutex;
	cMutex ejectMutex;
	
	bool suspend_polling;
	char *suspended_by;
	int intern_count;
	int extern_count;

	static cMutex stMutex;
	static bool active;
	static bool replaying;
	static bool ejectdisc;
	static bool crashed;
	
	bool mountMedia(bool Mount_it);
	bool open_close_Tray(void);
	bool FindHandler(eMediaType MediaType);
	bool ActivateHandler(void);
	const char *IsSuspendedByPlugin(void);
	void StopHandler(void);
	bool shouldMountMedia(void);
	eMediaType CheckDiscData(void);
	const char *MediaTypeToString(eMediaType MediaType);
  protected:
	virtual void Action(void);
  public:
	cMediaHandler();
	~cMediaHandler();

	bool Service(const char *Id, void *Data = NULL);
	void Stop();

	const char *MainMenuEntry(void);
	cOsdObject *MainMenuAction(void);

	int InternHandlers(void) { return intern_count; }
	int ExternHandlers(void) { return extern_count; }
	bool SuspendPolling(void) { return suspend_polling; }
	void SetSuspendPolling(bool Suspend) { suspend_polling = Suspend; }

	static bool HandlerIsActive(void);
	static bool HandlerIsReplaying(void);
	static void SetHandlerReplays(bool On);
	static bool HandlerRequestEject(void);
	static void SetHandlerEjectRequest(bool Eject);
	static bool HandlerCrashed(void);
	static void SetHandlerCrashed(bool Crashed);
};

#endif
