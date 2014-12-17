#include "status.h"
#include "vdrcd.h"

cVdrcdStatus::cVdrcdStatus(void) {
	m_Title = NULL;
	m_Current = NULL;
}

cVdrcdStatus::~cVdrcdStatus() {
}

void cVdrcdStatus::OsdTitle(const char *Title) {
	m_Title = Title;
}

void cVdrcdStatus::OsdCurrentItem(const char *Text) {
	m_Current = Text;
}

void cVdrcdStatus::OsdClear(void) {
#if VDRVERSNUM >= 10311
	if (m_NumClears && --m_NumClears == 0)
		Recordings.Load();
#endif
}

