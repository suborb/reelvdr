/*
 * menu.h: The actual menu implementations
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: menu.h 2.9 2012/05/12 11:48:04 kls Exp $
 */

#ifndef __MENU_H
#define __MENU_H

#include "ci.h"
#include "device.h"
#include "epg.h"
#include "osdbase.h"
#include "dvbplayer.h"
#include "menuitems.h"
#include "recorder.h"
#include "skins.h"
#ifdef USE_SETUP
#include "submenu.h"
#endif /* SETUP */
#ifdef USE_LIVEBUFFER
#include "livebuffer.h"
#endif /*USE_LIVEBUFFER*/


#ifdef REELVDR
class cMenuEditSrcItem : public cMenuEditIntItem {
private:
  const cSource *source;
protected:
  virtual void Set(void);
public:
  cMenuEditSrcItem(const char *Name, int *Value);
  eOSState ProcessKey(eKeys Key);
  };
#endif


class cMenuText : public cOsdMenu {
private:
  char *text;
  eDvbFont font;
public:
  cMenuText(const char *Title, const char *Text, eDvbFont Font = fontOsd);
  virtual ~cMenuText();
  void SetText(const char *Text);
  virtual void Display(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

class cMenuFolder : public cOsdMenu {
private:
  cNestedItemList *nestedItemList;
  cList<cNestedItem> *list;
  cString dir;
  cOsdItem *firstFolder;
  bool editing;
  void SetHelpKeys(void);
  void Set(const char *CurrentFolder = NULL);
  void DescendPath(const char *Path);
  eOSState SetFolder(void);
  eOSState Select(void);
  eOSState New(void);
  eOSState Delete(void);
  eOSState Edit(void);
  cMenuFolder(const char *Title, cList<cNestedItem> *List, cNestedItemList *NestedItemList, const char *Dir, const char *Path = NULL);
public:
  cMenuFolder(const char *Title, cNestedItemList *NestedItemList, const char *Path = NULL);
  cString GetFolder(void);
  virtual eOSState ProcessKey(eKeys Key);
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind() { return "MenuText"; }
#endif /* GRAPHTFT */
  };

class cMenuCommands : public cOsdMenu {
private:
  cList<cNestedItem> *commands;
  cString parameters;
  cString title;
  cString command;
  bool confirm;
  char *result;
  bool Parse(const char *s);
  eOSState Execute(void);
public:
  cMenuCommands(const char *Title, cList<cNestedItem> *Commands, const char *Parameters = NULL);
  virtual ~cMenuCommands();
  virtual eOSState ProcessKey(eKeys Key);
  };

class cMenuEditTimer : public cOsdMenu {
private:
  cTimer *timer;
  cTimer data;
  int channel;
  bool addIfConfirmed;
  cMenuEditStrItem *file;
  cMenuEditDateItem *day;
  cMenuEditDateItem *firstday;
  eOSState SetFolder(void);
  void SetFirstDayItem(void);
  void SetHelpKeys(void);
public:
  cMenuEditTimer(cTimer *Timer, bool New = false);
  virtual ~cMenuEditTimer();
  virtual eOSState ProcessKey(eKeys Key);
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind() { return "MenuTimerEdit"; }
#endif /* GRAPHTFT */
  };

class cMenuEvent : public cOsdMenu {
private:
  const cEvent *event;
public:
  cMenuEvent(const cEvent *Event, bool CanSwitch = false, bool Buttons = false);
  virtual void Display(void);
  virtual eOSState ProcessKey(eKeys Key);
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind() { return "MenuEvent"; }
#endif /* GRAPHTFT */
  };

#ifdef REELVDR
struct MenuMainHook_Data_V1_0 {
  eOSState  Function;    /*IN*/
  cOsdMenu *pResultMenu; /*OUT*/
};
#define CREATE_MENU_MAIN(function,menu) {                              \
  MenuMainHook_Data_V1_0 data;                                         \
  memset(&data, 0, sizeof(data));                                      \
  data.Function = function;                                            \
  if (!cPluginManager::CallFirstService("MenuMainHook-V1.0", &data))   \
    menu = new cMenuMain(function);                                    \
  else                                                                 \
    menu = data.pResultMenu;                                           \
}
#endif /* REELVDR*/

class cMenuMain : public cOsdMenu {
#ifdef REELVDR
protected:
  cMenuMain(const char *Title):cOsdMenu(Title){}; // Allow custom constructor
#else
private:
#endif
#ifdef USE_SETUP
  int    nrDynamicMenuEntries;
#endif /* SETUP */
  bool replaying;
  cOsdItem *stopReplayItem;
  cOsdItem *cancelEditingItem;
  cOsdItem *stopRecordingItem;
  int recordControlsState;
  static cOsdObject *pluginOsdObject;
#ifdef USE_SETUP
  void Set(int current=0);
  bool Update(bool Force = false);
  cSubMenu subMenu;
#else
  void Set(void);
  bool Update(bool Force = false);
#endif /* SETUP */
public:
  cMenuMain(eOSState State = osUnknown);
  virtual eOSState ProcessKey(eKeys Key);
  static cOsdObject *PluginOsdObject(void);
#ifdef REELVDR
  static void SetPluginOsdObject(cOsdObject *PluginOsdObject)
      { pluginOsdObject = PluginOsdObject ; }
#endif /* REELVDR */
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind() { return "MenuMain"; }
#endif /* GRAPHTFT */
  };

class cDisplayChannel : public cOsdObject {
private:
  cSkinDisplayChannel *displayChannel;
  int group;
  bool withInfo;
  cTimeMs lastTime;
  int number;
  bool timeout;
  cChannel *channel;
  const cEvent *lastPresent;
  const cEvent *lastFollowing;
  static cDisplayChannel *currentDisplayChannel;
  void DisplayChannel(void);
  void DisplayInfo(void);
  void Refresh(void);
  cChannel *NextAvailableChannel(cChannel *Channel, int Direction);
#if REELVDR
  bool notNumberKey;
#endif
public:
  cDisplayChannel(int Number, bool Switched);
  cDisplayChannel(eKeys FirstKey);
  virtual ~cDisplayChannel();
  virtual eOSState ProcessKey(eKeys Key);
  static bool IsOpen(void) { return currentDisplayChannel != NULL; }
  };

class cDisplayVolume : public cOsdObject {
private:
  cSkinDisplayVolume *displayVolume;
  cTimeMs timeout;
  static cDisplayVolume *currentDisplayVolume;
  virtual void Show(void);
  cDisplayVolume(void);
public:
  virtual ~cDisplayVolume();
  static cDisplayVolume *Create(void);
  static void Process(eKeys Key);
  eOSState ProcessKey(eKeys Key);
  };

class cDisplayTracks : public cOsdObject {
private:
  cSkinDisplayTracks *displayTracks;
  cTimeMs timeout;
  eTrackType types[ttMaxTrackTypes];
  char *descriptions[ttMaxTrackTypes + 1]; // list is NULL terminated
  int numTracks, track, audioChannel;
  static cDisplayTracks *currentDisplayTracks;
  virtual void Show(void);
  cDisplayTracks(void);
public:
  virtual ~cDisplayTracks();
  static bool IsOpen(void) { return currentDisplayTracks != NULL; }
  static cDisplayTracks *Create(void);
  static void Process(eKeys Key);
  eOSState ProcessKey(eKeys Key);
  };

class cDisplaySubtitleTracks : public cOsdObject {
private:
  cSkinDisplayTracks *displayTracks;
  cTimeMs timeout;
  eTrackType types[ttMaxTrackTypes];
  char *descriptions[ttMaxTrackTypes + 1]; // list is NULL terminated
  int numTracks, track;
  static cDisplaySubtitleTracks *currentDisplayTracks;
  virtual void Show(void);
  cDisplaySubtitleTracks(void);
public:
  virtual ~cDisplaySubtitleTracks();
  static bool IsOpen(void) { return currentDisplayTracks != NULL; }
  static cDisplaySubtitleTracks *Create(void);
  static void Process(eKeys Key);
  eOSState ProcessKey(eKeys Key);
  };

cOsdObject *CamControl(void);

class cMenuRecordingItem;

class cMenuRecordings : public cOsdMenu {
private:
  char *base;
  int level;
  int recordingsState;
  int helpKeys;
  void SetHelpKeys(void);
  void Set(bool Refresh = false);
  bool Open(bool OpenSubMenus = false);
  eOSState Play(void);
  eOSState Rewind(void);
  eOSState Delete(void);
  eOSState Info(void);
  eOSState Commands(eKeys Key = kNone);
#ifdef USE_LIEMIEXT
  eOSState Rename(void);
#endif /* LIEMIEXT */
protected:
  cRecording *GetRecording(cMenuRecordingItem *Item);
public:
  cMenuRecordings(const char *Base = NULL, int Level = 0, bool OpenSubMenus = false);
  ~cMenuRecordings();
  virtual eOSState ProcessKey(eKeys Key);
  };

class cRecordControl {
private:
  cDevice *device;
  cTimer *timer;
  cRecorder *recorder;
  const cEvent *event;
  cString instantId;
  char *fileName;
  bool GetEvent(void);
public:
#ifdef USE_ALTERNATECHANNEL
  cRecordControl(cDevice *Device, cTimer *Timer = NULL, bool Pause = false, cChannel *Channel = NULL);
#else
  cRecordControl(cDevice *Device, cTimer *Timer = NULL, bool Pause = false);
#endif /* ALTERNATECHANNEL */
  virtual ~cRecordControl();
  bool Process(time_t t);
  cDevice *Device(void) { return device; }
  void Stop(bool ExecuteUserCommand = true);
  const char *InstantId(void) { return instantId; }
  const char *FileName(void) { return fileName; }
  cTimer *Timer(void) { return timer; }
  };

class cRecordControls {
private:
  static cRecordControl *RecordControls[];
  static int state;
#ifdef USE_LIVEBUFFER
protected:
  friend class cRecordControl;
  static cLiveRecorder *liveRecorder;
#endif /*USE_LIVEBUFFER*/
public:
  static bool Start(cTimer *Timer = NULL, bool Pause = false);
  static void Stop(const char *InstantId);
#ifdef USE_LNBSHARE
  static void Stop(cDevice *Device);   // LNB Sharing
#endif /* LNBSHARE */
  static bool PauseLiveVideo(void);
#ifdef USE_LIVEBUFFER
  static bool StartLiveBuffer(eKeys Key);
#endif /*USE_LIVEBUFFER*/
#ifdef REELVDR
  static const char *GetInstantId(const char *LastInstantId, bool LIFO=false);
#else
  static const char *GetInstantId(const char *LastInstantId);
#endif
  static cRecordControl *GetRecordControl(const char *FileName);
  static cRecordControl *GetRecordControl(const cTimer *Timer);
         ///< Returns the cRecordControl for the given Timer.
         ///< If there is no cRecordControl for Timer, NULL is returned.
  static void Process(time_t t);
#ifdef USE_MCLI
  static void ChannelDataModified(cChannel *Channel, int chmod = CHANNELMOD_RETUNE);
#else
  static void ChannelDataModified(cChannel *Channel);
#endif /*USE_MCLI*/
  static bool Active(void);
  static void Shutdown(void);
  static void ChangeState(void) { state++; }
  static bool StateChanged(int &State);
#ifdef USE_LIVEBUFFER
  static void SetLiveChannel(cDevice *Device, const cChannel *Channel);
  static bool CanSetLiveChannel(const cChannel *Channel);
  static bool IsWritingBuffer();
  static void CancelWritingBuffer();
  static cIndex *GetLiveBuffer(cTimer *Timer);
  static cIndex *GetLiveIndex(const char *FileName);
  static double GetLiveFramesPerSecond();
#endif /*USE_LIVEBUFFER*/
  };

class cReplayControl : public cDvbPlayerControl {
#ifdef REELVDR
  friend class cRecordControls;
#endif
private:
  cSkinDisplayReplay *displayReplay;
#ifdef USE_JUMPPLAY
  cMarksReload marks;
#else
  cMarks marks;
#endif /* JUMPPLAY */
  bool visible, modeOnly, shown, displayFrames;
  int lastCurrent, lastTotal;
  bool lastPlay, lastForward;
  int lastSpeed;
#ifdef USE_LIEMIEXT
  int lastSkipSeconds;
  eKeys lastSkipKey;
  cTimeMs lastSkipTimeout;
#endif /* LIEMIEXT */
  time_t timeoutShow;
  bool timeSearchActive, timeSearchHide;
  int timeSearchTime, timeSearchPos;
  void TimeSearchDisplay(void);
  void TimeSearchProcess(eKeys Key);
  void TimeSearch(void);
  void ShowTimed(int Seconds = 0);
  static cReplayControl *currentReplayControl;
  static cString fileName;
#if REELVDR
  /* hold the last replayed recording's title and filename
     since livebuffer is also replayed as if it is a recording and
     livebuffer should not clear the last replayed recording */
  static cString lastReplayedFileName;
  static cString lastReplayedTitle;
#endif
  void ShowMode(void);
  bool ShowProgress(bool Initial);
  void MarkToggle(void);
  void MarkJump(bool Forward);
  void MarkMove(bool Forward);
  void EditCut(void);
  void EditTest(void);
#ifdef REELVDR
  int jumpWidth;
  void Jump(eKeys);
  time_t m_LastKeyPressed;
  int m_ActualStep;
  int m_FirstStep;
  eKeys m_LastKey;
#endif
public:
  cReplayControl(bool PauseLive = false);
  virtual ~cReplayControl();
  void Stop(void);
  virtual cOsdObject *GetInfo(void);
  virtual const cRecording *GetRecording(void);
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Show(void);
  virtual void Hide(void);
  bool Visible(void) { return visible; }
  static void SetRecording(const char *FileName);
  static const char *NowReplaying(void);
#ifdef REELVDR
  static const char *NowReplayingTitle(void);

  // returns the title of the last replayed "real" recording
  static const char *LastReplayedTitle(void);
#endif /*REELVDR*/
  static const char *LastReplayed(void);
  static void ClearLastReplayed(const char *FileName);
  };

#ifdef REELVDR
// --- cMenuSetupPluginItem --------------------------------------------------

class cMenuSetupPluginItem : public cOsdItem {
  private:
    int pluginIndex;
  public:
    cMenuSetupPluginItem(const char *Name, int Index);
    int PluginIndex(void) { return pluginIndex; }
};
#endif //REELVDR

#endif //__MENU_H
