/*
 * status.c: Status monitoring
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: status.c 1.8 2005/12/31 15:10:10 kls Exp $
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

void cStatus::MsgChannelSwitch(const cDevice *Device, int ChannelNumber)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->ChannelSwitch(Device, ChannelNumber);
}

void cStatus::MsgRecording(const cDevice *Device, const char *Name, const char *FileName, bool On, int ChannelNumber)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->Recording(Device, Name, FileName, On, ChannelNumber);
}

void cStatus::MsgReplaying(const cControl *Control, const char *Name, const char *FileName, bool On)
{
//  dsyslog("MsgReplaying: Name: %s, FileName: %s, On: %i", Name, FileName, On);

  //TB: Hack: detach the osdtelext-receiver when replaying
  cPlugin *plug = cPluginManager::GetPlugin("osdteletext");
  if (plug) {
      if (On)
        plug->Service("StopReceiver");
      else
        plug->Service("StartReceiver");
  }

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

void cStatus::MsgOsdClear(bool doNotShow)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      sm->OsdClear(doNotShow);
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

bool cStatus::MsgChannelProtected(const cDevice* Device, const cChannel* Channel)     // PIN PATCH
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
      if (sm->ChannelProtected(Device, Channel) == true)
 	 return true;

  return false;
}

bool cStatus::MsgReplayProtected(const cRecording* Recording, const char* Name, 
                                 const char* Base, bool isDirectory)             // PIN PATCH
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
     if (sm->ReplayProtected(Recording, Name, Base, isDirectory) == true)
       return true;

  return false;
}

void cStatus::MsgRecordingFile(const char* FileName)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))   // PIN PATCH
      sm->RecordingFile(FileName);
}

void cStatus::MsgTimerCreation(cTimer* Timer, const cEvent *Event)
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))   // PIN PATCH
     sm->TimerCreation(Timer, Event);
}

bool cStatus::MsgPluginProtected(cPlugin* Plugin)   // PIN PATCH
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
    if (sm->PluginProtected(Plugin) == true)
      return true;

  return false;
}

void cStatus::MsgUserAction(const eKeys key, const cOsdObject* Interact)         // PIN PATCH
{
  for (cStatus *sm = statusMonitors.First(); sm; sm = statusMonitors.Next(sm))
     sm->UserAction(key, Interact);
}

