/*
 * streamdev.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: streamdev-client.c,v 1.6 2008/04/08 14:18:15 schmirl Exp $
 */

#include "streamdev-client.h"
#include "client/device.h"
#include "client/setup.h"

const char *cPluginStreamdevClient::DESCRIPTION = trNOOP("VTP Streaming Client");
const char *cPluginStreamdevClient::MAINMENUENTRY = trNOOP("Streaming Client");

cPluginStreamdevClient::cPluginStreamdevClient(void) {
}

cPluginStreamdevClient::~cPluginStreamdevClient() {
}

const char *cPluginStreamdevClient::Description(void) {
	return tr(DESCRIPTION);
}

bool cPluginStreamdevClient::Start(void) {
	I18nRegister(PLUGIN_NAME_I18N);
	cStreamdevDevice::Init();
	return true;
}

void cPluginStreamdevClient::Housekeeping(void) {
	if (StreamdevClientSetup.StartClient && StreamdevClientSetup.SyncEPG)
		ClientSocket.SynchronizeEPG();
}

const char *cPluginStreamdevClient::MainMenuEntry(void) {
	//return NULL;
	//return StreamdevClientSetup.StartClient ? tr("Streaming Control") : NULL;
	return MAINMENUENTRY;
}

cOsdObject *cPluginStreamdevClient::MainMenuAction(void) {
	if (ClientSocket.SuspendServer())
		Skins.Message(mtInfo, tr("Server is suspended"));
	else
		Skins.Message(mtError, tr("Couldn't suspend Server!"));
	return NULL;
}

cMenuSetupPage *cPluginStreamdevClient::SetupMenu(void) {
  return new cStreamdevClientMenuSetupPage;
}

bool cPluginStreamdevClient::SetupParse(const char *Name, const char *Value) {
  return StreamdevClientSetup.SetupParse(Name, Value);
}

bool cPluginStreamdevClient::Service(const char *Id, void *Data)
{
    if (strcmp(Id, "Streamdev Client Reinit") == 0)
    {
        cStreamdevDevice::ReInit();
        return true;
    }
    return false;
}

VDRPLUGINCREATOR(cPluginStreamdevClient); // Don't touch this!
