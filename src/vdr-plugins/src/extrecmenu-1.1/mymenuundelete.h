#ifndef  MENUNDELETE_H
#define  MENUNDELETE_H

//#include "mymenurecordings.h"
//#include <vdr/menu.h>
#include <vdr/osdbase.h>
#include <vdr/menuitems.h>



// --- cMenuRecordingItem  -------------------------------------------------------
class cMenuRecordingItem : public cOsdItem {
private:
  char *fileName;
  char *name;
  int totalEntries, newEntries;

public:
  cMenuRecordingItem(cRecording *Recording, int Level);
  ~cMenuRecordingItem();
  void IncrementCounter(bool New);
  const char *Name(void) { return name; }
  const char *FileName(void) { return fileName; }
  bool IsDirectory(void) { return name != NULL; }
  };


// --- cMenuUndelete  -------------------------------------------------------
class cMenuUndelete : public cOsdMenu
{
 private:
  cLockFile *lockFile;
  char *base;
  int level;
  int recordingsState;
  int helpKeys;

  //void Set(bool Refresh=false, char *Current=NULL);
  void Set(bool Refresh=false);
  bool Open(bool OpenSubmenu=false);

  void SetHelpKeys();
  void SetFreeSpaceTitle();

  cRecording *GetRecording(cMenuRecordingItem *Item);
  eOSState Play();
  eOSState Info();
  eOSState Remove();
  eOSState Undelete();
  eOSState SetTitle();
  //eOSState Details();
 public:
 //cMenuUndelete(const char *Base=NULL, const char *Parent=NULL,int Level=0);
 cMenuUndelete(const char *Base = NULL, int Level = 0, bool OpenSubMenus = false);
  ~cMenuUndelete();
  virtual eOSState ProcessKey(eKeys Key);
};


// --- cMenuUndeleteInfo  -------------------------------------------------------
class myMenuUndeleteInfo:public cOsdMenu
{
 private:
  const cRecording *recording;
 public:
  myMenuUndeleteInfo(const cRecording *Recording);
  virtual void Display(void);
  virtual eOSState ProcessKey(eKeys Key);
};


// --- cMenuRecording --------------------------------------------------------

class cMenuRecording : public cOsdMenu {
private:
  const cRecording *recording;
  bool withButtons;
public:
  cMenuRecording(const cRecording *Recording, bool WithButtons = false);
  virtual void Display(void);
  virtual eOSState ProcessKey(eKeys Key);
};


#endif //  MENUNDELETE_H

