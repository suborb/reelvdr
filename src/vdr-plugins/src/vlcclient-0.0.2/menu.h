#ifndef vlcclient_menu_h
#define vlcclient_menu_h

#include <vdr/osd.h>
#include <vdr/osdbase.h>

#include "remote.h"

class cRemoteRecording;
class cRemoteRecordings;
class cRecordingsListThread;

class cVlcClientMenuRecordingItem: public cOsdItem {
private:
	bool  m_Directory;
        char *m_FileName;
        char *m_Name;

public:
        cVlcClientMenuRecordingItem(cRemoteRecording *Recording, int Level);
        virtual ~cVlcClientMenuRecordingItem();

        void IncrementCounter(bool New);
        const char *Name(void) const { return m_Name; }
        const char *FileName(void) const { return m_FileName; }
        bool IsDirectory(void) const { return m_Directory; }
};

class cVlcClientMenu: public cOsdMenu {
private:
        static int               HelpKeys;
        static cRemoteRecordings Recordings;
        char *m_Base;
        int   m_Level;

protected:
        bool Open(cVlcClientMenuRecordingItem *ir, bool OpenSubMenus = false);
        void SetHelpKeys();
        cRemoteRecording *GetRecording(cVlcClientMenuRecordingItem *Item);

        eOSState Select(void);

public:
        cVlcClientMenu(cRecordingsListThread *listThread, const char *Base = NULL, int Level = 0, bool OpenSubMenus = false);
        virtual ~cVlcClientMenu();
	void Set(void);
        virtual eOSState ProcessKey(eKeys Key);
};


#endif
