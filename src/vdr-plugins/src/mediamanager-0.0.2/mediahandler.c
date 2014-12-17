/*
 * mediahandler.c:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vdr/remote.h>
#include <vdr/keys.h>
#include <vdr/osdbase.h>
#include <vdr/plugin.h>
#include <vdr/skins.h>
#include <vdr/tools.h>

#include "mediaconfig.h"
#include "mediahandler.h"
#include "auto-generated/allhandlers.h"
#include "mediamanagerservice.h"
#include "config.h"

#define MYSELF "mediamanager"

//#define DEBUG_D 1
#define DEBUG_I 1
#include "mediadebug.h"

class cDiscFile : public cListObject {
  private:
  	char *file;
  public:
	cDiscFile(const char *Filename);
	~cDiscFile();
	bool HasExtension(const char *Extension);
};

class cDiscFileList : public cList<cDiscFile> {
  private:
	void ScanDisc(const char *Directory);
  public:
	cDiscFileList();
	~cDiscFileList();
	eMediaType FindFiles(const char *Directory);
};

class cMediaExternHandler : public cMediaHandlerObject {
  private:
	char *name;
	char *menuentry;
	char *description;
	cPlugin *plugin;
  public:
	cMediaExternHandler(eMediaType Mt, bool Mount);
	~cMediaExternHandler();
	void SetNames(const char *Name, const char *MenuEntry,
									const char *Description);
	const char *Name(void) { return name; }
	virtual const char *Description(void) { return description; }
	virtual const char *MainMenuEntry(void) { return menuentry; }
	virtual cOsdObject *MainMenuAction(void);

	virtual bool Activate(bool On);
};

cMediaExternHandler::cMediaExternHandler(eMediaType Mt, bool Mount)
: cMediaHandlerObject()
{
	name = NULL;
	menuentry = NULL;
	description = NULL;
	plugin = NULL;
	setIsExternHandler(true);
	setSupportedMediaType(Mt);
	mediaShouldBeMounted(Mount);
}

cMediaExternHandler::~cMediaExternHandler()
{
	plugin = NULL;
	free(name);
	free(menuentry);
	free(description);
}

void cMediaExternHandler::SetNames(const char *Name, const char *MenuEntry,
											const char *Description)
{
	name = strdup(Name);
	menuentry = strdup(MenuEntry);
	description = strdup(Description);
	plugin = cPluginManager::GetPlugin(name);
}

cOsdObject *cMediaExternHandler::MainMenuAction(void)
{
	MediaManager_Mainmenu_v1_0 menu;
	menu.mainmenu = NULL;
	if(plugin && plugin->Service(MEDIA_MANAGER_MAINMENU_ID, &menu)) {
		if(menu.mainmenu == NULL)
			cRemote::CallPlugin(name);
		else
			return menu.mainmenu;
	}
	return NULL;
}

bool cMediaExternHandler::Activate(bool On)
{
	MediaManager_Activate_v1_0 act;
	if(On) {
		act.on = true;
		act.device_file = cMediaConfig::GetConfig().GetDeviceFile();
		act.mount_point = cMediaConfig::GetConfig().GetMountPoint();
		if(plugin && plugin->Service(MEDIA_MANAGER_ACTIVATE_ID, &act))
			return true;
	} else {
		act.on = false;
		if(plugin)
			plugin->Service(MEDIA_MANAGER_ACTIVATE_ID, &act);
		return true;
	}
	return false;
}

class cMediaHandlerMenuItem : public cOsdItem {
  private:
  public:
	cMediaHandlerMenuItem(const char *Text);
	~cMediaHandlerMenuItem();
};

class cMediaHandlerMenu : public cOsdMenu {
  private:
  	cMediaHandler *handler;
  public:
	cMediaHandlerMenu(cMediaHandler *Handler);
	~cMediaHandlerMenu();
	eOSState ProcessKey(eKeys Key);
};

cMediaHandlerMenuItem::cMediaHandlerMenuItem(const char *Text)
{
	SetText(Text);
	SetSelectable(false);
}

cMediaHandlerMenuItem::~cMediaHandlerMenuItem()
{
}

cMediaHandlerMenu::cMediaHandlerMenu(cMediaHandler *Handler)
: cOsdMenu(tr("Media Manager"), 13)
{
	int current = Current();
	handler = Handler;
	char *buf = NULL;
	Clear();

	if(handler->SuspendPolling()) {
		if(handler->IsSuspendedByPlugin())
			asprintf(&buf, "%s %s", tr("Media Manager - deactivated by"), handler->IsSuspendedByPlugin());
		else
			asprintf(&buf, "%s", tr("Media Manager - deactivated"));
	} else {
		asprintf(&buf, "%s", tr("Media Manager - active"));
	}
	SetTitle(buf);
	free(buf);
	buf = NULL;
	Add(new cMediaHandlerMenuItem(tr("Supported Media")));
	Add(new cMediaHandlerMenuItem("\t"));

	asprintf(&buf, "%s:\t ", tr("internal"));
	Add(new cMediaHandlerMenuItem(buf));
	free(buf);

	if(handler->InternHandlers() > 0) {
		for(cMediaHandlerObject *obj = handler->First(); obj; obj = handler->Next(obj)) {
			if(!obj->IsExternHandler()) {
				buf = NULL;			
				asprintf(&buf, "  %s:\t%s", obj->MainMenuEntry(), obj->Description());
				Add(new cMediaHandlerMenuItem(buf));
				free(buf);
			}
		}
	} else 
		Add(new cMediaHandlerMenuItem(tr("none")));

	buf = NULL;
	Add(new cMediaHandlerMenuItem("\t"));
	asprintf(&buf, "%s:\t ", tr("through plugins"));
	Add(new cMediaHandlerMenuItem(buf));
	free(buf);
	if(handler->ExternHandlers() > 0) {
		for(cMediaHandlerObject *obj = handler->First(); obj; obj = handler->Next(obj)) {
			if(obj->IsExternHandler()) {
				cMediaExternHandler *e = (cMediaExternHandler *)obj;
				buf = NULL;			
				asprintf(&buf, "  %s:\t%s", e->Name(), obj->Description());
				Add(new cMediaHandlerMenuItem(buf));
				free(buf);
			}
		}
	} else 
		Add(new cMediaHandlerMenuItem(tr("none")));

	SetCurrent(Get(current));
	Display();
	SetHelp(handler->SuspendPolling() ? tr("Activate") : tr("Deactivate"),
		NULL, " ^ ", tr("Button$Close"));
}

cMediaHandlerMenu::~cMediaHandlerMenu()
{
}

eOSState cMediaHandlerMenu::ProcessKey(eKeys Key)
{
	eOSState state = cOsdMenu::ProcessKey(Key);
	if((Key != kNone) && (state == osUnknown)) {
		switch (Key & ~k_Repeat) {
			case kRed:
				if(handler->SuspendPolling()) {
					handler->SetSuspendPolling(false);
					//SetHelp(tr("Deactivate"), NULL, NULL, tr("Button$Close"));
				} else {
					handler->SetSuspendPolling(true);
					//SetHelp(tr("Activate"),	NULL, NULL, tr("Button$Close"));
				}
				state = osEnd;
				break;
			case kYellow:
				if(handler->open_close_Tray()) {
					state = osEnd;
				} else {
					state = osContinue;
				}
				break;
			case kBlue:
				state = osEnd;
				break;
			default:
				break;
		}
	}

	return state;
}

cMutex cMediaHandler::stMutex;
bool cMediaHandler::active = false;
bool cMediaHandler::replaying = false;
bool cMediaHandler::ejectdisc = false;
bool cMediaHandler::crashed = false;

cMediaHandler::cMediaHandler():cThread("Mediamanager: MediaPoller")
{
	current = NULL;

#include "auto-generated/mediahandler.inc"

	extern_count = 0;
	intern_count = Count();
	suspend_polling = false;
	suspended_by = NULL;
	poller = new cMediaPoller();
}

cMediaHandler::~cMediaHandler()
{
	delete(poller);
}

const char *cMediaHandler::IsSuspendedByPlugin(void)
{
	return suspended_by;
}

bool cMediaHandler::Service(const char *Id, void *Data)
{
	if(Data == NULL)
		return false;

	if(strcmp(Id, MEDIA_MANAGER_REGISTER_ID) == 0) {
		cMutexLock ml(&listMutex);
		struct MediaManager_Register_v1_0 *reg = (MediaManager_Register_v1_0*)Data;
		cMediaExternHandler *n = new cMediaExternHandler(reg->mediatype, reg->shouldmount);
		n->SetNames(reg->PluginName, reg->MainMenuEntry, reg->Description);
		Add(n);
		extern_count++;
DBG_I("[Media Manager]: Register plugin '%s' mediatype %d",reg->PluginName, reg->mediatype)
		return true;
	} else if(strcmp(Id, MEDIA_MANAGER_STATUS_ID) == 0) {
		struct MediaManager_Status_v1_0 *status = (MediaManager_Status_v1_0*)Data;
		if(status->set) {
			if(status->flags & MEDIA_STATUS_REPLAY)
				 SetHandlerReplays(status->isreplaying);
			if(status->flags & MEDIA_STATUS_EJECTDISC)
				 SetHandlerEjectRequest(status->ejectdisc);
			if(status->flags & MEDIA_STATUS_CRASHED)
				 SetHandlerCrashed(status->crashed);
		} else {
			if(status->flags & MEDIA_STATUS_REPLAY)
				 status->isreplaying = HandlerIsReplaying();
			if(status->flags & MEDIA_STATUS_EJECTDISC)
				 status->ejectdisc = HandlerRequestEject();
			if(status->flags & MEDIA_STATUS_ACTIVE)
				 status->active = HandlerIsActive();
			if(status->flags & MEDIA_STATUS_CRASHED)
				 status->crashed = HandlerCrashed();
		}
		return true;
	} else if(strcmp(Id, MEDIA_MANAGER_SUSPEND_ID) == 0) {
		struct MediaManager_Suspend_v1_0 *s = (MediaManager_Suspend_v1_0*)Data;
		if(s->suspend) {
			if(s->PluginName == NULL)
				return false;
			suspended_by = strdup(s->PluginName);
			SetSuspendPolling(true);
			DBG_I("[Media Manager]: Deaktivated by %s", suspended_by)
		} else {
			SetSuspendPolling(false);
			if(suspended_by) free(suspended_by);
			suspended_by = NULL;
			DBG_I("[Media Manager]: Aktivated")
		}
		return true;
	}
	return false;
}

void cMediaHandler::Stop()
{
	Cancel(3);
}

const char *cMediaHandler::MainMenuEntry(void)
{
	if(current && HandlerIsActive())
		return current->MainMenuEntry();

	return NULL;
}

cOsdObject *cMediaHandler::MainMenuAction(void)
{
	if(current && HandlerIsActive())
		return current->MainMenuAction();

	return new cMediaHandlerMenu(this);
}

bool cMediaHandler::FindHandler(eMediaType MediaType)
{
	cMutexLock ml(&listMutex);
	for(cMediaHandlerObject *obj = First(); obj; obj = Next(obj)) {
		if(obj->supportsMedia(MediaType)) {
			cMutexLock ml(&stMutex);
			active = false;
			replaying = false;
			ejectdisc = false;
			crashed = false;
			current = obj;
			return true;
		}
	}
	return false;
}

bool cMediaHandler::ActivateHandler(void)
{
	cMutexLock ml(&stMutex);
	if(current)
		active = current->Activate(true);
	return active;
}

void cMediaHandler::StopHandler(void)
{
	if(current) {
		cMutexLock ml(&stMutex);
		current->Activate(false);
		active = false;
		current = NULL;
	}
}

bool cMediaHandler::shouldMountMedia(void)
{
	if(current)
		return current->shouldMountMedia();

	return false;
}

bool cMediaHandler::HandlerIsActive(void)
{
	cMutexLock ml(&stMutex);
	return active;
}

bool cMediaHandler::HandlerIsReplaying(void)
{
	cMutexLock ml(&stMutex);
	return replaying;
}

void cMediaHandler::SetHandlerReplays(bool On)
{
	cMutexLock ml(&stMutex);
	replaying = On;
}

bool cMediaHandler::HandlerRequestEject(void)
{
	cMutexLock ml(&stMutex);
	return ejectdisc;
}

void cMediaHandler::SetHandlerEjectRequest(bool Eject)
{
	cMutexLock ml(&stMutex);
	ejectdisc = Eject;
}

bool cMediaHandler::HandlerCrashed(void)
{
	cMutexLock ml(&stMutex);
	return crashed;
}

void cMediaHandler::SetHandlerCrashed(bool Crashed)
{
	cMutexLock ml(&stMutex);
	crashed = Crashed;
}

eMediaType cMediaHandler::CheckDiscData(void)
{
	const char *mp = cMediaConfig::GetConfig().GetMountPoint();
	eMediaType ret = mtData;

	if(!mountMedia(true))
		return ret;

	cDiscFileList *l = new cDiscFileList();
	ret = l->FindFiles(mp);
	delete(l);

	mountMedia(false);
	return ret;
}

bool cMediaHandler::mountMedia(bool Mount_it)
{
	char *cmd = NULL;
	const char *mp = cMediaConfig::GetConfig().GetMountPoint();
	const char *realdevice = cMediaConfig::GetConfig().GetRealDeviceFile();

	if(Mount_it) {
		if(poller->isMounted(realdevice))
			return true;
		asprintf(&cmd,"%s \"%s\"",MOUNTCMD, mp);
	} else {
		if(!poller->isMounted(realdevice))
			return true;
		asprintf(&cmd,"%s \"%s\"",UMOUNTCMD, mp);
	}

	int result = SystemExec(cmd);
	free(cmd);
	if(result) {
		DBG_E("[cMediaHandler]: %s failed error: %d",
				 Mount_it ? "mount" : "umount", result/256);
		return false;
	}
	return true;
}

bool cMediaHandler::open_close_Tray(void)
{
	const char *device = cMediaConfig::GetConfig().GetDeviceFile();
	cMutexLock ml(&ejectMutex);
	return poller ? poller->EjectDisc(device) : false;
}

const char *cMediaHandler::MediaTypeToString(eMediaType MediaType)
{
	switch(MediaType) {
		case mtBlank:
			return "Blank disc"; break;
		case mtAudio:
			return "Digital Audio"; break;
		case mtVideoDvd:
			return "Video DVD"; break;
		case mtSvcd:
			return "SVCD"; break;
		case mtVcd:
			return "VCD"; break;
		case mtVDRdata:
			return "VDR archive"; break;
		case mtData:
			return "Data"; break;
		case mtAudioData:
			return "Audio data"; break;
		case mtImageData:
			return "Image data"; break;
		default:
			return "???"; break;
	}
	return "???";
}

void cMediaHandler::Action()
{
	const char *device = cMediaConfig::GetConfig().GetDeviceFile();
	const char *realdevice = cMediaConfig::GetConfig().GetRealDeviceFile();
	bool mediaIsHandled = false;
	bool mounted = false;
	bool eject = false;
	eMediaType last = mtNoDisc;
	// sleep on startup
	cCondWait::SleepMs(2000);
DBG_D("[cMediaHandler]: start poller")
	while(Running()) {
		if(SuspendPolling()) {
			cCondWait::SleepMs(500);
			continue;
		}
		if(!HandlerIsActive()) {
			ejectMutex.Lock();
			mediatype = poller->poll(device, realdevice);
			last = mediatype;
			if(mediatype == mtData)
				mediatype = CheckDiscData();

			if(mediatype > mtNoDisc)
				mediaIsHandled = FindHandler(mediatype);
			else
				mediaIsHandled = false;

			ejectMutex.Unlock();

			if(!mediaIsHandled) {
				if(mediatype > mtNoDisc) {
					char *buf = NULL;
					asprintf(&buf, "%s: %s", tr("Media Manager: Unsupported media"),
											MediaTypeToString(mediatype));
					Skins.QueueMessage(mtInfo, buf, 5);
					DBG_I("MediaManager: unsupported media type")
					free(buf);
					eMediaType mt = last;
					while(Running() && (mt == last) && !SuspendPolling()) {
						ejectMutex.Lock();
						mt = poller->poll(device, realdevice);
						ejectMutex.Unlock();
						cCondWait::SleepMs(2000);
					}
				}
				continue;
			}

			if(mediaIsHandled) {
DBG_D("[cMediaHandler]: Media is handled")
				if(shouldMountMedia()) {
					if(mountMedia(true)) {
						mounted = true;
						if(ActivateHandler())
							cRemote::CallPlugin(MYSELF);
					} else {
						char *buf = NULL;
						asprintf(&buf, "%s %s",
							tr("Media Manager: Failed to mount"), device);
						Skins.QueueMessage(mtError, buf, 5);
						free(buf);
						SetSuspendPolling(true);
						DBG_E("MediaManager: Failed to mount %s", device)
					}
				} else {
					if(ActivateHandler())
						cRemote::CallPlugin(MYSELF);
				}
			}

		} else {
DBG_D("[cMediaHandler]: Entering handler loop")
			while(Running() && HandlerIsActive() && !HandlerCrashed()) {
				if(SuspendPolling()) {
					cCondWait::SleepMs(500);
					continue;
				}
				if(!HandlerIsReplaying()) {
					ejectMutex.Lock();
					eMediaType mt = poller->poll(device, realdevice);
					ejectMutex.Unlock();
					if(mt != last) {
						break;
					}
				}
				if(HandlerRequestEject()) {
					eject = true;
					break;
				}
				cCondWait::SleepMs(2000);
			}
DBG_D("[cMediaHandler]: Leaving handler loop")
			StopHandler();
			if(mounted) {
				mounted = false;
				if(!mountMedia(false)) {
					char *buf = NULL;
					asprintf(&buf, "%s %s",
						tr("Media Manager: Failed to umount"), device);
					Skins.QueueMessage(mtError, buf, 5);
					free(buf);
					SetSuspendPolling(true);
					DBG_E("MediaManager: Failed to umount %s", device)
				}
			}
			if(eject) {
				eject = false;
				if(!poller->EjectDisc(device)) {
					char *buf = NULL;
					asprintf(&buf, "%s %s",
						tr("Media Manager: Failed to eject"), device);
					Skins.QueueMessage(mtError, buf, 5);
					free(buf);
					SetSuspendPolling(true);
					DBG_E("MediaManager: Failed to eject %s", device)
				}
			}
		}
	}
DBG_D("[cMediaHandler]: stop poller")
}

cDiscFile::cDiscFile(const char *Filename)
{
	file = strdup(Filename);
}

cDiscFile::~cDiscFile()
{
	free(file);
}

bool cDiscFile::HasExtension(const char *Extension)
{
	char *e = strrchr(file, '.');
	if(e) {
		if(strcasecmp(e, Extension))
			return false;
		return true;
	}
	return false;
}

cDiscFileList::cDiscFileList(void)
{
}

cDiscFileList::~cDiscFileList()
{
}

eMediaType cDiscFileList::FindFiles(const char *Directory)
{
	int image = 0;
	int audio = 0;
	ScanDisc(Directory);
	for(cDiscFile *f = First(); f; f = Next(f)) {
		if(f) {
			if(f->HasExtension(".mp3"))
				audio++;
			else if(f->HasExtension(".ogg"))
				audio++;
			else if(f->HasExtension(".wav"))
				audio++;
			else if(f->HasExtension(".jpg"))
				image++;
			else if(f->HasExtension(".jpeg"))
				image++;
			else if(f->HasExtension(".png"))
				image++;
			else if(f->HasExtension(".gif"))
				image++;
			else if(f->HasExtension(".tif"))
				image++;
			else if(f->HasExtension(".tiff"))
				image++;
			else if(f->HasExtension(".bmp"))
				image++;
		}
	}
DBG_I("[Media Manager]: found %d audio and %d image files",audio, image)
	if(audio > image)
		return mtAudioData;
	if(image)
		return mtImageData;
	return mtData;
}

#define MAXSCANFILES 19

void cDiscFileList::ScanDisc(const char *Directory)
{
	if(Count() > MAXSCANFILES) return;

	cReadDir dir(Directory);
	struct dirent *e;
	while((e = dir.Next()) != NULL) {
		if(Count() > MAXSCANFILES) break;
		if(strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) {
			char *buf = NULL;
			struct stat st;
			asprintf(&buf, "%s/%s", Directory, e->d_name);
			if(stat(buf, &st) == 0) {
				if(S_ISDIR(st.st_mode)) {
					ScanDisc(buf);
				} else if(S_ISREG(st.st_mode)) {
					Add(new cDiscFile(e->d_name));
				}
			}
			free(buf);
		}
	}
}
