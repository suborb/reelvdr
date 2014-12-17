#ifndef vlcclient_remote_h
#define vlcclient_remote_h

#include <vdr/config.h>
#include <string>
#include "menu.h"

class cVlcClientMenu;

class cRemoteRecording: public cListObject {
private:
        int         m_Index;
        bool        m_IsNew;
	bool	    m_IsDir;
        char       *m_TitleBuffer;
        std::string m_StartTime;
        std::string m_Name;
	std::string m_Title;

public:
        cRemoteRecording(const char *Name);	
        ~cRemoteRecording();

        bool operator==(const cRemoteRecording &Recording);
        bool operator!=(const cRemoteRecording &Recording);

        void ParseInfo(const char *Text);

        int Index(void) const { return m_Index; }
        const char *StartTime(void) const { return m_StartTime.c_str(); }
        bool IsNew(void) const { return m_IsNew; }
        const char *Name(void) const { return m_Name.c_str(); }
        const char *Title(char Delimiter, bool NewIndicator, int Level);
        int HierarchyLevels(void);
	bool IsDirectory(void) { return m_IsDir; }
};

inline bool cRemoteRecording::operator!=(const cRemoteRecording &Recording) {
        return !operator==(Recording);
}

class cRemoteRecordings: public cList<cRemoteRecording> {
public:
        bool Load(const char *BaseDir);
        cRemoteRecording *GetByName(const char *Name);
};

class cRecordingsListThread: public cThread {
private:
	const char *baseDir;
	cRemoteRecordings *recordings;
	cVlcClientMenu *menu;
protected:
	virtual void Action(void);
public:
	cRecordingsListThread(void);
	bool Load(cVlcClientMenu *Menu, cRemoteRecordings *Recordings, const char *BaseDir);
};


#endif

