/*
 * status.c: Status monitoring
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: status.c 2.1 2012/03/07 14:17:24 kls Exp $
 */

#include "status.h"

// --- cStatus ---------------------------------------------------------------

cList<cStatus> cStatus::statusMonitors;

cStatus::cStatus(void)
{
  statusMonitors.Add(this);
}

cStatus::~cStatus()
{
  statusMonitors.Del(this, false);
}

void cStatus::MsgTimerChange(const cTimer *Timer, eTimerChange Change)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->TimerChange(Timer, Change);
}

void cStatus::MsgChannelSwitch(const cDevice *Device, int ChannelNumber, bool LiveView)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->ChannelSwitch(Device, ChannelNumber, LiveView);
}

void cStatus::MsgRecording(const cDevice *Device, const char *Name, const char *FileName, bool On)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->Recording(Device, Name, FileName, On);
}

void cStatus::MsgReplaying(const cControl *Control, const char *Name, const char *FileName, bool On)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->Replaying(Control, Name, FileName, On);
}

void cStatus::MsgSetVolume(int Volume, bool Absolute)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->SetVolume(Volume, Absolute);
}

void cStatus::MsgSetAudioTrack(int Index, const char * const *Tracks)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->SetAudioTrack(Index, Tracks);
}

void cStatus::MsgSetAudioChannel(int AudioChannel)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->SetAudioChannel(AudioChannel);
}

void cStatus::MsgSetSubtitleTrack(int Index, const char * const *Tracks)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->SetSubtitleTrack(Index, Tracks);
}

void cStatus::MsgOsdClear(void)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdClear();
}

void cStatus::MsgOsdTitle(const char *Title)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdTitle(Title);
}

void cStatus::MsgOsdStatusMessage(const char *Message)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdStatusMessage(Message);
}

void cStatus::MsgOsdHelpKeys(const char *Red, const char *Green, const char *Yellow, const char *Blue)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdHelpKeys(Red, Green, Yellow, Blue);
}

void cStatus::MsgOsdItem(const char *Text, int Index)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdItem(Text, Index);
}

void cStatus::MsgOsdCurrentItem(const char *Text)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdCurrentItem(Text);
}

void cStatus::MsgOsdTextItem(const char *Text, bool Scroll)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdTextItem(Text, Scroll);
}

void cStatus::MsgOsdChannel(const char *Text)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdChannel(Text);
}

void cStatus::MsgOsdProgramme(time_t PresentTime, const char *PresentTitle, const char *PresentSubtitle, time_t FollowingTime, const char *FollowingTitle, const char *FollowingSubtitle)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdProgramme(PresentTime, PresentTitle, PresentSubtitle, FollowingTime, FollowingTitle, FollowingSubtitle);
}
#ifdef USE_GRAPHTFT

void cStatus::MsgOsdSetEvent(const cEvent* event)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdSetEvent(event);
}

void cStatus::MsgOsdSetRecording(const cRecording* recording)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdSetRecording(recording);
}

void cStatus::MsgOsdMenuDisplay(const char* kind)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdMenuDisplay(kind);
}

void cStatus::MsgOsdMenuDestroy()
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdMenuDestroy();
}
void cStatus::MsgOsdEventItem(const cEvent* Event, const char *Text, int Index, int Count)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
     sm->OsdEventItem(Event, Text, Index, Count);
}
#endif /* GRAPHTFT */
#ifdef USE_PINPLUGIN

bool cStatus::MsgChannelProtected(const cDevice* Device, const cChannel* Channel)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      if (sm->ChannelProtected(Device, Channel) == true)
          return true;

  return false;
}

bool cStatus::MsgReplayProtected(const cRecording* Recording, const char* Name, 
                                 const char* Base, bool isDirectory, int menuView)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
     if (sm->ReplayProtected(Recording, Name, Base, isDirectory, menuView) == true)
         return true;
     return false;
}

void cStatus::MsgRecordingFile(const char* FileName)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->RecordingFile(FileName);
}

void cStatus::MsgTimerCreation(cTimer* Timer, const cEvent *Event)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
     sm->TimerCreation(Timer, Event);
}

bool cStatus::MsgPluginProtected(cPlugin* Plugin, int menuView)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
     if (sm->PluginProtected(Plugin, menuView) == true)
         return true;
     return false;
}

void cStatus::MsgUserAction(const eKeys key, const cOsdObject* Interact)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
     sm->UserAction(key, Interact);
}

bool cStatus::MsgMenuItemProtected(const char* Name, int menuView)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
     if (sm->MenuItemProtected(Name, menuView) == true)
         return true;
     return false;
}
#endif /* PINPLUGIN */
