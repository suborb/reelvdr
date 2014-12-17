#include "setup.h"

#include <vdr/menuitems.h>

cVdrcdSetup VdrcdSetup;

cVdrcdSetup::cVdrcdSetup(void) {
	AutoStartVCD = true;
	AutoStartMP3 = true;
}

bool cVdrcdSetup::SetupParse(const char *Name, const char *Value) {
	if      (strcmp(Name, "AutoStartVCD") == 0) AutoStartVCD = atoi(Value);
	else if (strcmp(Name, "AutoStartMP3") == 0) AutoStartMP3 = atoi(Value);
	else return false;
	return true;
}

cVdrcdMenuSetupPage::cVdrcdMenuSetupPage(void) {
	m_NewVdrcdSetup = VdrcdSetup;

	Add(new cMenuEditBoolItem(tr("Video-CD Autostart"),
			&m_NewVdrcdSetup.AutoStartVCD));
	Add(new cMenuEditBoolItem(tr("Audio/MP3-CD Autostart"),
			&m_NewVdrcdSetup.AutoStartMP3));
}

cVdrcdMenuSetupPage::~cVdrcdMenuSetupPage() {
}

void cVdrcdMenuSetupPage::Store(void) {
	SetupStore("AutoStartVCD", m_NewVdrcdSetup.AutoStartVCD);
	SetupStore("AutoStartMP3", m_NewVdrcdSetup.AutoStartMP3);

	VdrcdSetup = m_NewVdrcdSetup;
}
