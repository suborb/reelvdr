/*
 * menu.h: The actual menu implementations
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: menu.h 1.86 2006/10/20 13:09:57 kls Exp $
 */

#ifndef __MENU_H
#define __MENU_H

//#include "ci.h"
#include "device.h"
#include "epg.h"
#include "osdbase.h"
#include "dvbplayer.h"
#include "menuitems.h"
#include "recorder.h"
#include "skins.h"
#include "submenu.h"
#include "help.h"
#include <vector>
#include <sys/time.h>

// --- cMenuEditCaItem -------
// needed by bouquets plugin

class cMenuEditCaItem : public cMenuEditIntItem {
protected:
  virtual void Set(void);
public:
  cMenuEditCaItem(const char *Name, int *Value, bool EditingBouquet);
  eOSState ProcessKey(eKeys Key);
  };

// --- cMenuEditMapItem -------
// needed by bouquets plugin

class cMenuEditMapItem : public cMenuEditItem {
protected:
  int *value;
  const tChannelParameterMap *map;
  const char *zeroString;
  virtual void Set(void);
public:
  cMenuEditMapItem(const char *Name, int *Value, const tChannelParameterMap *Map, const char *ZeroString = NULL);
  virtual eOSState ProcessKey(eKeys Key);
  };

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

class cMenuHelp : public cOsdMenu {
private:
  char *text;
  cHelpSection *section;
  cHelpPage *helpPage;
  eDvbFont font;
public:
  cMenuHelp(cHelpSection *Section, const char *Title);
  virtual ~cMenuHelp();
  void SetText(const char *Text);
  void SetNextHelp();
  void SetPrevHelp();
  virtual void Display(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

class cMenuEditTimer : public cOsdMenu {
private:
  cTimer *timer;
  cTimer data;
  int channel;
  bool addIfConfirmed;
  cMenuEditDateItem *firstday;
  void SetFirstDayItem(void);
  int tmpprio;
  const char *PriorityTexts[3];
public:
  cMenuEditTimer(cTimer *Timer, bool New = false);
  virtual ~cMenuEditTimer();
  virtual eOSState ProcessKey(eKeys Key);
  };

class cMenuEvent : public cOsdMenu {
private:
  const cEvent *event;
public:
  cMenuEvent(const cEvent *Event, bool CanSwitch = false, bool Buttons = false);
  virtual void Display(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

class cMenuActiveEvent : public cOsdMenu {
private:
  const cEvent *event;
public:
  cMenuActiveEvent();
  virtual void Display(void);
  eOSState Record(void);
  void SetHelpButtons(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

class cMenuMain : public cOsdMenu {
private:
  int    nrDynamicMenuEntries;
  time_t lastDiskSpaceCheck;
  cSubMenu subMenu;
  int lastFreeMB;
  bool replaying;
  cOsdItem *stopReplayItem;
  cOsdItem *cancelEditingItem;
  cOsdItem *stopRecordingItem;
  int recordControlsState;
  static cOsdObject *pluginOsdObject;
  void Set(int current=0);
  bool Update(bool Force = false);
  eOSState DisplayHelp(int Index);
public:
  cMenuMain(eOSState State = osUnknown);
  virtual eOSState ProcessKey(eKeys Key);
  static cOsdObject *PluginOsdObject(void);
  static void SetPluginOsdObject(cOsdObject *PluginOsdObject)
      { pluginOsdObject = PluginOsdObject ; }
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

class cMenuCam : public cOsdMenu {
private:
  cCiMenu *ciMenu;
  bool selected;
  int offset;
  void AddMultiLineItem(const char *s);
  eOSState Select(void);
public:
  cMenuCam(cCiMenu *CiMenu);
  virtual ~cMenuCam();
  virtual eOSState ProcessKey(eKeys Key);
  };

class cMenuCamEnquiry : public cOsdMenu {
private:
  cCiEnquiry *ciEnquiry;
  char *input;
  bool replied;
  eOSState Reply(void);
public:
  cMenuCamEnquiry(cCiEnquiry *CiEnquiry);
  virtual ~cMenuCamEnquiry();
  virtual eOSState ProcessKey(eKeys Key);
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
protected:
  cRecording *GetRecording(cMenuRecordingItem *Item);
public:
  cMenuRecordings(const char *Base = NULL, int Level = 0, bool OpenSubMenus = false);
  ~cMenuRecordings();
  virtual eOSState ProcessKey(eKeys Key);
  };

class cMenuEditSrcItem : public cMenuEditIntItem {
private:
  const cSource *source;
protected:
  virtual void Set(void);
public:
  cMenuEditSrcItem(const char *Name, int *Value);
  eOSState ProcessKey(eKeys Key);
  };

class cRecordControl {
private:
  cDevice *device;
  cTimer *timer;
  cRecorder *recorder;
  const cEvent *event;
  char *instantId;
  char *fileName;
  bool usrActive;
  bool GetEvent(void);
public:
  cRecordControl(cDevice *Device, cTimer *Timer = NULL, bool Pause = false);
  virtual ~cRecordControl();
  bool Process(time_t t);
  cDevice *Device(void) { return device; }
  void Stop(void);
  const char *InstantId(void) { return instantId; }
  const char *FileName(void) { return fileName; }
  cTimer *Timer(void) { return timer; }
  };

class cRecordControls {
  friend class cRecordControl;
private:
  static cRecordControl *RecordControls[];
  static int state;
public:
  static bool Start(cTimer *Timer = NULL, bool Pause = false);
  static void Stop(const char *InstantId);
  static void Stop(cDevice *Device);
  static bool PauseLiveVideo(bool fromLiveBuffer = false);
  static const char *GetInstantId(const char *LastInstantId, bool LIFO = false);
  static cRecordControl *GetRecordControl(const char *FileName);
  static void Process(time_t t);
  static void ChannelDataModified(cChannel *Channel, int chmod = CHANNELMOD_ALL);
  static bool Active(void);
  static void Shutdown(void);
  static void ChangeState(void) { state++; }
  static bool StateChanged(int &State);
  };

class cReplayControl : public cDvbPlayerControl {
private:
  cSkinDisplayReplay *displayReplay;
  cMarksReload marks;
  bool visible, modeOnly, shown, displayFrames;
  int lastCurrent, lastTotal;
  bool lastPlay, lastForward;
  int lastSpeed;
  int jumpWidth;
  int lastJump;
  time_t timeoutShow;
  bool timeSearchActive, timeSearchHide;
  int timeSearchTime, timeSearchPos;
  void TimeSearchDisplay(void);
  void TimeSearchProcess(eKeys Key);
  void TimeSearch(void);
  void ShowTimed(int Seconds = 0);
  static cReplayControl *currentReplayControl;
  static char *fileName;
  static char *title;
  void ShowMode(void);
  bool ShowProgress(bool Initial);
  void MarkToggle(void);
  void MarkJump(bool Forward);
  void MarkMove(bool Forward);
  void EditCut(void);
  void EditTest(void);
  void Jump(eKeys);

  time_t m_LastKeyPressed;
  int m_ActualStep;
  int m_FirstStep;
  eKeys m_LastKey;

public:
  cReplayControl(void);
  virtual ~cReplayControl();
  virtual cOsdObject *GetInfo(void);
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Show(void);
  virtual void Hide(void);
  bool Visible(void) { return visible; }
  static void SetRecording(const char *FileName, const char *Title);
  static const char *NowReplaying(void);
  static const char *NowReplayingTitle(void);
  static const char *LastReplayed(void);
  static void ClearLastReplayed(const char *FileName);
  };

class cMenuChannelItem : public cOsdItem {
public:
  enum eViewMode { mode_edit, mode_view, mode_classic };
  enum eChannelSortMode { csmNumber, csmName, csmProvider };
private:
  static eChannelSortMode sortMode;
  enum eViewMode viewMode;
  cChannel *channel;
  cSchedulesLock schedulesLock;
  const cSchedules *schedules;
  const cEvent *event;
  char szProgressPart[12];
  bool isSet;
  bool isMarked;
public:
  cMenuChannelItem(cChannel *Channel, eViewMode viewMode = mode_view);
  static void SetSortMode(eChannelSortMode SortMode) { sortMode = SortMode; }
  static void IncSortMode(void) { sortMode = eChannelSortMode((sortMode == csmProvider) ? csmNumber : sortMode + 1); }
  static eChannelSortMode SortMode(void) { return sortMode; }
  virtual int Compare(const cListObject &ListObject) const;
  virtual void Set(void);
  bool IsSet(void);
  bool IsMarked(void) { return isMarked; }
  void SetMarked(bool marked) { isMarked = marked; }
  cChannel *Channel(void) { return channel; }
  };

class cMenuBouquets : public cOsdMenu {
public:
  enum eViewMode { mode_view, mode_edit };
private:
  enum eViewMode viewMode;
  int view_;
  bool edit;
  bool favourite;
  bool move;
  bool selectionChanched;  
  int startChannel;
  int number;
  std::vector<int> channelMarked;
  cTimeMs numberTimer;
  void Setup(void);
  void SetChannels(void);
  void SetGroup(int Index);
  cChannel *GetChannel(int Index);
  void Propagate(void);
  void GetFavourite(void);
  eOSState PrevBouquet(void);
  eOSState NextBouquet(void);
  char titleBuf[128];
protected:
  void Options(void);
  eOSState Switch(void);
  eOSState NewChannel(void);
  eOSState EditChannel(void);
  eOSState DeleteChannel(void);
  eOSState ListBouquets(void);
  eOSState Number(eKeys Key);
  void Mark(/*cMenuChannelItem *item = NULL*/);
  virtual void Move(int From, int To);
  void Move(int From, int To, bool doSwitch);
  eOSState MoveMultiple(void);
  eOSState MoveMultipleOrCurrent();
  void UnMark(cMenuChannelItem *p);
public:
  cMenuBouquets(int view, eViewMode viewMode = mode_view);
  ~cMenuBouquets(void);
  void CreateTitle(cChannel *channel);
  void AddFavourite(bool active);
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Display(void);
  };

enum eShutdownMode { standby, deepstandby, restart, quickstandby };

class cMenuShutdown : public cOsdMenu {
private:
  int &interrupted_;
  eShutdownMode &shutdownMode_;
  bool &userShutdown_;
  time_t &lastActivity_;
  bool shutdown_;
  bool standbyAfterRecording_;
  bool quickShutdown_;
  bool expert;
  eOSState Shutdown(eShutdownMode mode);

  class cTimer
  {
  public:
    cTimer()
    : oldtime_(0), timeout_(0)
    {
    }
    bool TimedOut()
    {
        gettimeofday(&time, &tz);
        uint newtime = (time.tv_sec%1000000)*1000 + time.tv_usec/1000;
        if(oldtime_ && newtime - oldtime_ > timeout_)
        {
            return true;
        }
        return false;
    }
    void Start(int timeout)
    {
        gettimeofday(&time, &tz);
        oldtime_ = (time.tv_sec%1000000)*1000 + time.tv_usec/1000;
        timeout_ = timeout;
    }
  private:
    struct timeval time;
    struct timezone tz;
    uint oldtime_;
    uint timeout_;
  }
  timer_;

public:
  cMenuShutdown(int &Interrupted, eShutdownMode &shutdownMode, bool &userShutdown, time_t& lastActivity);
  ~cMenuShutdown();
  void Set();
  /*override*/ eOSState ProcessKey(eKeys Key);
  void CancelShutdown();
  static void CancelShutdownScript(const char *msg = NULL);
  };

bool IsTimerNear();
extern bool kChanOpen;

#endif //__MENU_H
