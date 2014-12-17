/*
 * mediahandlerobject.h:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef _MEDIA_HANDLER_OBJECT_H
#define _MEDIA_HANDLER_OBJECT_H

#include <vdr/thread.h>
#include <vdr/tools.h>
#include <vdr/osdbase.h>
#include "mediatypes.h"

class cMediaHandlerObject : public cListObject {
  private:
  	eMediaType mt;
	bool mount;
	bool is_extern;
  protected:
	void setSupportedMediaType(eMediaType MediaType) { mt = MediaType; }
	void mediaShouldBeMounted(bool Mount) { mount = Mount; }
	void setIsExternHandler(bool Extern) { is_extern = Extern; }
  public:
	cMediaHandlerObject() { mt = mtInvalid; mount = false; is_extern = false; }
	virtual ~cMediaHandlerObject() {}
	bool supportsMedia(eMediaType MediaType) { return MediaType == mt; }
	bool shouldMountMedia(void) { return mount; }
	bool IsExternHandler(void) { return is_extern; }

	virtual const char *Description(void) = 0;
	virtual const char *MainMenuEntry(void) = 0;
	virtual cOsdObject *MainMenuAction(void) = 0;

	virtual bool Activate(bool On) = 0;
};

#endif
