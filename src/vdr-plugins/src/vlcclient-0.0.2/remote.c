#include <ctype.h>
 
#include "remote.h"
#include "common.h"
#include "socket.h"

cRemoteRecording::cRemoteRecording(const char *Name){
	m_Name = Name;
	m_StartTime = "";
	m_IsNew = false;

	if(!strncmp(Name, "DIR:", 4)){
	  m_IsDir = true;
	  //m_Name = char(130) + '\t' + (const char*)Name+4;
	  m_Name = (const char*)Name+4;
	} else
	  m_IsDir = false;
}

cRemoteRecording::~cRemoteRecording(void) {
}

bool cRemoteRecording::operator==(const cRemoteRecording &Recording) {
        return          m_Index     == Recording.m_Index
                        && m_StartTime == Recording.m_StartTime
                        && m_Name      == Recording.m_Name;
}

const char *cRemoteRecording::Title(char Delimiter, bool NewIndicator, int Level) {
 return m_Name.c_str();

  char New = NewIndicator && IsNew() ? '*' : ' ';
        
  if (m_TitleBuffer != NULL) {
                free(m_TitleBuffer);
                m_TitleBuffer = NULL;
        }
        
  if (Level < 0 || Level == HierarchyLevels()) {
    char *s;
        const char *t;  
    if (Level > 0 && (t = strrchr(m_Name.c_str(), '~')) != NULL)
      t++;
    else
      t = m_Name.c_str();

        asprintf(&m_TitleBuffer, "%s%c%c%s", m_StartTime.c_str(), New, Delimiter, t);
    // let's not display a trailing '~':
    stripspace(m_TitleBuffer);
    s = &m_TitleBuffer[strlen(m_TitleBuffer) - 1];
    if (*s == '~')
      *s = 0;
  } else if (Level < HierarchyLevels()) {
    const char *s = m_Name.c_str();
    const char *p = s;
    while (*++s) {
      if (*s == '~') {
        if (Level--)
          p = s + 1;
        else
          break;
      }
    }
    m_TitleBuffer = MALLOC(char, s - p + 3);
    *m_TitleBuffer = Delimiter;
    *(m_TitleBuffer + 1) = Delimiter;
    strn0cpy(m_TitleBuffer + 2, p, s - p + 1);
  } else
    return "";
  return m_TitleBuffer;
}

int cRemoteRecording::HierarchyLevels(void){
  const char *s = m_Name.c_str();
  int level = 0;
  while (*++s) {
    if (*s == '~') ++level;
  }
  return level;
}

// --- cRemoteRecordings -----------------------------------------------------

bool cRemoteRecordings::Load(const char *BaseDir) {
  Clear();
  return ClientSocket.LoadVLCRecordings(*this, BaseDir);
}

cRemoteRecording *cRemoteRecordings::GetByName(const char *Name) {
        for (cRemoteRecording *r = First(); r; r = Next(r))
                if (strcmp(r->Name(), Name) == 0)
                        return r;
        return NULL;
}

// --- cListRecordingsThread -------------------------------------------------

cRecordingsListThread::cRecordingsListThread(void) :cThread("VLC Recordlisting")
{
	recordings = NULL;
	baseDir = NULL;
}

void cRecordingsListThread::Action(void)
{
	if (recordings->Load(baseDir)) {
		// recordings loaded
		menu->Set();
	}
}

bool cRecordingsListThread::Load(cVlcClientMenu *Menu, cRemoteRecordings *Recordings, const char *BaseDir)
{
	if (!Active()) {
		menu = Menu;
		recordings = Recordings;
		baseDir = BaseDir;
		Start();
		return true;
	}
	return false;
}
