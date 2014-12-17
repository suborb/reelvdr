/*
 * menu.c: The actual menu implementations
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: menu.c 2.32 2011/08/27 11:05:33 kls Exp $
 */

#include "menu.h"
#ifdef USE_WAREAGLEICON
#include "iconpatch.h"
#endif /* WAREAGLEICON */
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "channels.h"
#include "config.h"
#include "cutter.h"
#include "eitscan.h"
#include "i18n.h"
#include "interface.h"
#include "plugin.h"
#include "recording.h"
#include "remote.h"
#include "shutdown.h"
#include "sourceparams.h"
#include "sources.h"
#include "status.h"
#include "themes.h"
#include "timers.h"
#include "transfer.h"
#include "videodir.h"
#ifdef USE_MENUORG
#include "menuorgpatch.h"
#endif /* MENUORG */

#define MAXWAIT4EPGINFO   3 // seconds
#define MODETIMEOUT       2 // seconds
#define DISKSPACECHEK     5 // seconds between disk space checks
#define NEWTIMERLIMIT   120 // seconds until the start time of a new timer created from the Schedule menu,
                            // within which it will go directly into the "Edit timer" menu to allow
                            // further parameter settings
#define DEFERTIMER       60 // seconds by which a timer is deferred in case of problems

#define MAXRECORDCONTROLS (MAXDEVICES * MAXRECEIVERS)
#define MAXINSTANTRECTIME (24 * 60 - 1) // 23:59 hours
#define MAXWAITFORCAMMENU  10 // seconds to wait for the CAM menu to open
#define CAMMENURETYTIMEOUT  3 // seconds after which opening the CAM menu is retried
#define CAMRESPONSETIMEOUT  5 // seconds to wait for a response from a CAM
#define MINFREEDISK       300 // minimum free disk space (in MB) required to start recording
#define NODISKSPACEDELTA  300 // seconds between "Not enough disk space to start recording!" messages

#define CHNUMWIDTH  (numdigits(Channels.MaxNumber()) + 1)

// --- cFreeDiskSpace --------------------------------------------------------

#define MB_PER_MINUTE 25.75 // this is just an estimate!

class cFreeDiskSpace {
private:
  static time_t lastDiskSpaceCheck;
  static int lastFreeMB;
  static cString freeDiskSpaceString;
public:
  static bool HasChanged(bool ForceCheck = false);
  static const char *FreeDiskSpaceString(void) { HasChanged(); return freeDiskSpaceString; }
  };

time_t cFreeDiskSpace::lastDiskSpaceCheck = 0;
int cFreeDiskSpace::lastFreeMB = 0;
cString cFreeDiskSpace::freeDiskSpaceString;

cFreeDiskSpace FreeDiskSpace;

bool cFreeDiskSpace::HasChanged(bool ForceCheck)
{
  if (ForceCheck || time(NULL) - lastDiskSpaceCheck > DISKSPACECHEK) {
     int FreeMB;
     int Percent = VideoDiskSpace(&FreeMB);
     lastDiskSpaceCheck = time(NULL);
     if (ForceCheck || FreeMB != lastFreeMB) {
        int Minutes = int(double(FreeMB) / MB_PER_MINUTE);
        int Hours = Minutes / 60;
        Minutes %= 60;
        freeDiskSpaceString = cString::sprintf("%s %d%%  -  %2d:%02d %s", tr("Disk"), Percent, Hours, Minutes, tr("free"));
        lastFreeMB = FreeMB;
        return true;
        }
     }
  return false;
}

// --- cMenuEditCaItem -------------------------------------------------------

class cMenuEditCaItem : public cMenuEditIntItem {
protected:
  virtual void Set(void);
public:
  cMenuEditCaItem(const char *Name, int *Value);
  eOSState ProcessKey(eKeys Key);
  };

cMenuEditCaItem::cMenuEditCaItem(const char *Name, int *Value)
:cMenuEditIntItem(Name, Value, 0)
{
  Set();
}

void cMenuEditCaItem::Set(void)
{
  if (*value == CA_FTA)
     SetValue(tr("Free To Air"));
  else if (*value >= CA_ENCRYPTED_MIN)
     SetValue(tr("encrypted"));
  else
     cMenuEditIntItem::Set();
}

eOSState cMenuEditCaItem::ProcessKey(eKeys Key)
{
  eOSState state = cMenuEditItem::ProcessKey(Key);

  if (state == osUnknown) {
     if (NORMALKEY(Key) == kLeft && *value >= CA_ENCRYPTED_MIN)
        *value = CA_FTA;
     else
        return cMenuEditIntItem::ProcessKey(Key);
     Set();
     state = osContinue;
     }
  return state;
}

// --- cMenuEditSrcItem ------------------------------------------------------

#ifndef REELVDR
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

cMenuEditSrcItem::cMenuEditSrcItem(const char *Name, int *Value)
:cMenuEditIntItem(Name, Value, 0)
{
  source = Sources.Get(*Value);
  Set();
}

void cMenuEditSrcItem::Set(void)
{
  if (source)
     SetValue(cString::sprintf("%s - %s", *cSource::ToString(source->Code()), source->Description()));
  else
     cMenuEditIntItem::Set();
}

eOSState cMenuEditSrcItem::ProcessKey(eKeys Key)
{
  eOSState state = cMenuEditItem::ProcessKey(Key);

  if (state == osUnknown) {
     bool IsRepeat = Key & k_Repeat;
     Key = NORMALKEY(Key);
     if (Key == kLeft) { // TODO might want to increase the delta if repeated quickly?
        if (source) {
           if (source->Prev())
              source = (cSource *)source->Prev();
           else if (!IsRepeat)
              source = Sources.Last();
           *value = source->Code();
           }
        }
     else if (Key == kRight) {
        if (source) {
           if (source->Next())
              source = (cSource *)source->Next();
           else if (!IsRepeat)
              source = Sources.First();
           }
        else
           source = Sources.First();
        if (source)
           *value = source->Code();
        }
     else
        return state; // we don't call cMenuEditIntItem::ProcessKey(Key) here since we don't accept numerical input
     Set();
     state = osContinue;
     }
  return state;
}

// --- cMenuEditChannel ------------------------------------------------------

class cMenuEditChannel : public cOsdMenu {
private:
  cChannel *channel;
  cChannel data;
  cSourceParam *sourceParam;
  char name[256];
  void Setup(void);
public:
  cMenuEditChannel(cChannel *Channel, bool New = false);
  virtual eOSState ProcessKey(eKeys Key);
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind() { return "MenuEditChannel"; }
#endif /* GRAPHTFT */
  };

cMenuEditChannel::cMenuEditChannel(cChannel *Channel, bool New)
:cOsdMenu(tr("Edit channel"), 16)
{
  channel = Channel;
  sourceParam = NULL;
  *name = 0;
  if (channel) {
     data = *channel;
     strn0cpy(name, data.name, sizeof(name));
     if (New) {
        channel = NULL;
        data.nid = 0;
        data.tid = 0;
        data.rid = 0;
        }
     }
  Setup();
}

void cMenuEditChannel::Setup(void)
{
  int current = Current();

  Clear();

  // Parameters for all types of sources:
  Add(new cMenuEditStrItem( tr("Name"),          name, sizeof(name)));
  Add(new cMenuEditSrcItem( tr("Source"),       &data.source));
  Add(new cMenuEditIntItem( tr("Frequency"),    &data.frequency));
  Add(new cMenuEditIntItem( tr("Vpid"),         &data.vpid,  0, 0x1FFF));
  Add(new cMenuEditIntItem( tr("Ppid"),         &data.ppid,  0, 0x1FFF));
  Add(new cMenuEditIntItem( tr("Apid1"),        &data.apids[0], 0, 0x1FFF));
  Add(new cMenuEditIntItem( tr("Apid2"),        &data.apids[1], 0, 0x1FFF));
  Add(new cMenuEditIntItem( tr("Dpid1"),        &data.dpids[0], 0, 0x1FFF));
  Add(new cMenuEditIntItem( tr("Dpid2"),        &data.dpids[1], 0, 0x1FFF));
  Add(new cMenuEditIntItem( tr("Spid1"),        &data.spids[0], 0, 0x1FFF));
  Add(new cMenuEditIntItem( tr("Spid2"),        &data.spids[1], 0, 0x1FFF));
  Add(new cMenuEditIntItem( tr("Tpid"),         &data.tpid,  0, 0x1FFF));
  Add(new cMenuEditCaItem(  tr("CA"),           &data.caids[0]));
  Add(new cMenuEditIntItem( tr("Sid"),          &data.sid, 1, 0xFFFF));
  /* XXX not yet used
  Add(new cMenuEditIntItem( tr("Nid"),          &data.nid, 0));
  Add(new cMenuEditIntItem( tr("Tid"),          &data.tid, 0));
  Add(new cMenuEditIntItem( tr("Rid"),          &data.rid, 0));
  XXX*/
#ifdef USE_CHANNELBIND
  Add(new cMenuEditIntItem( tr("Rid"),          &data.rid, 0)); // channel binding patch
#endif /* CHANNELBIND */
  // Parameters for specific types of sources:
  sourceParam = SourceParams.Get(**cSource::ToString(data.source));
  if (sourceParam) {
     sourceParam->SetData(&data);
     cOsdItem *Item;
     while ((Item = sourceParam->GetOsdItem()) != NULL)
           Add(Item);
     }

  SetCurrent(Get(current));
  Display();
}

eOSState cMenuEditChannel::ProcessKey(eKeys Key)
{
  int oldSource = data.source;
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     if (Key == kOk) {
        if (sourceParam)
           sourceParam->GetData(&data);
        if (Channels.HasUniqueChannelID(&data, channel)) {
           data.name = strcpyrealloc(data.name, name);
           if (channel) {
              *channel = data;
              isyslog("edited channel %d %s", channel->Number(), *data.ToText());
              state = osBack;
              }
           else {
              channel = new cChannel;
              *channel = data;
              Channels.Add(channel);
              Channels.ReNumber();
              isyslog("added channel %d %s", channel->Number(), *data.ToText());
              state = osUser1;
              }
           Channels.SetModified(true);
           }
        else {
           Skins.Message(mtError, tr("Channel settings are not unique!"));
           state = osContinue;
           }
        }
     }
  if (Key != kNone && (data.source & cSource::st_Mask) != (oldSource & cSource::st_Mask)) {
     if (sourceParam)
        sourceParam->GetData(&data);
     Setup();
     }
  return state;
}

// --- cMenuChannelItem ------------------------------------------------------

class cMenuChannelItem : public cOsdItem {
public:
  enum eChannelSortMode { csmNumber, csmName, csmProvider };
private:
  static eChannelSortMode sortMode;
  cChannel *channel;
public:
  cMenuChannelItem(cChannel *Channel);
  static void SetSortMode(eChannelSortMode SortMode) { sortMode = SortMode; }
  static void IncSortMode(void) { sortMode = eChannelSortMode((sortMode == csmProvider) ? csmNumber : sortMode + 1); }
  static eChannelSortMode SortMode(void) { return sortMode; }
  virtual int Compare(const cListObject &ListObject) const;
  virtual void Set(void);
  cChannel *Channel(void) { return channel; }
  };

cMenuChannelItem::eChannelSortMode cMenuChannelItem::sortMode = csmNumber;

cMenuChannelItem::cMenuChannelItem(cChannel *Channel)
{
  channel = Channel;
  if (channel->GroupSep())
     SetSelectable(false);
  Set();
}

int cMenuChannelItem::Compare(const cListObject &ListObject) const
{
  cMenuChannelItem *p = (cMenuChannelItem *)&ListObject;
  int r = -1;
  if (sortMode == csmProvider)
     r = strcoll(channel->Provider(), p->channel->Provider());
  if (sortMode == csmName || r == 0)
     r = strcoll(channel->Name(), p->channel->Name());
  if (sortMode == csmNumber || r == 0)
     r = channel->Number() - p->channel->Number();
  return r;
}

void cMenuChannelItem::Set(void)
{
  cString buffer;
  if (!channel->GroupSep()) {
     if (sortMode == csmProvider)
        buffer = cString::sprintf("%d\t%s - %s", channel->Number(), channel->Provider(), channel->Name());
#ifdef USE_WAREAGLEICON
     else if (Setup.WarEagleIcons) {
        if (channel->Vpid() == 1 || channel->Vpid() == 0)
           buffer = cString::sprintf("%d\t%s %-30s", channel->Number(), IsLangUtf8() ? ICON_RADIO_UTF8 : ICON_RADIO, channel->Name());
        else if (channel->Ca() == 0)
           buffer = cString::sprintf("%d\t%s %-30s", channel->Number(), IsLangUtf8() ? ICON_TV_UTF8 : ICON_TV, channel->Name());
        else
           buffer = cString::sprintf("%d\t%s %-30s", channel->Number(), IsLangUtf8() ? ICON_TV_CRYPTED_UTF8 : ICON_TV_CRYPTED, channel->Name());
        }
#endif /* WAREAGLEICON */
     else
        buffer = cString::sprintf("%d\t%s", channel->Number(), channel->Name());
     }
  else
     buffer = cString::sprintf("---\t%s ----------------------------------------------------------------", channel->Name());
  SetText(buffer);
}

// --- cMenuChannels ---------------------------------------------------------

#define CHANNELNUMBERTIMEOUT 1000 //ms

class cMenuChannels : public cOsdMenu {
private:
  int number;
  cTimeMs numberTimer;
  void Setup(void);
  cChannel *GetChannel(int Index);
  void Propagate(void);
protected:
  eOSState Number(eKeys Key);
  eOSState Switch(void);
  eOSState Edit(void);
  eOSState New(void);
  eOSState Delete(void);
  virtual void Move(int From, int To);
public:
  cMenuChannels(void);
  ~cMenuChannels();
  virtual eOSState ProcessKey(eKeys Key);
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind() { return "MenuChannels"; }
#endif /* GRAPHTFT */
  };

cMenuChannels::cMenuChannels(void)
:cOsdMenu(tr("Channels"), CHNUMWIDTH)
{
  number = 0;
  Setup();
  Channels.IncBeingEdited();
}

cMenuChannels::~cMenuChannels()
{
  Channels.DecBeingEdited();
}

void cMenuChannels::Setup(void)
{
  cChannel *currentChannel = GetChannel(Current());
  if (!currentChannel)
     currentChannel = Channels.GetByNumber(cDevice::CurrentChannel());
  cMenuChannelItem *currentItem = NULL;
  Clear();
  for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel)) {
      if (!channel->GroupSep()
          || (cMenuChannelItem::SortMode() == cMenuChannelItem::csmNumber && *channel->Name())) {
         cMenuChannelItem *item = new cMenuChannelItem(channel);
         Add(item);
         if (channel == currentChannel)
            currentItem = item;
         }
      }
  if (cMenuChannelItem::SortMode() != cMenuChannelItem::csmNumber)
     Sort();
  SetCurrent(currentItem);
  SetHelp(tr("Button$Edit"), tr("Button$New"), tr("Button$Delete"), tr("Button$Mark"));
  Display();
}

cChannel *cMenuChannels::GetChannel(int Index)
{
  cMenuChannelItem *p = (cMenuChannelItem *)Get(Index);
  return p ? (cChannel *)p->Channel() : NULL;
}

void cMenuChannels::Propagate(void)
{
  Channels.ReNumber();
  for (cMenuChannelItem *ci = (cMenuChannelItem *)First(); ci; ci = (cMenuChannelItem *)ci->Next())
      ci->Set();
  Display();
  Channels.SetModified(true);
}

eOSState cMenuChannels::Number(eKeys Key)
{
  if (HasSubMenu())
     return osContinue;
  if (numberTimer.TimedOut())
     number = 0;
  if (!number && Key == k0) {
     cMenuChannelItem::IncSortMode();
     Setup();
     }
  else {
     number = number * 10 + Key - k0;
     for (cMenuChannelItem *ci = (cMenuChannelItem *)First(); ci; ci = (cMenuChannelItem *)ci->Next()) {
         if (!ci->Channel()->GroupSep() && ci->Channel()->Number() == number) {
            SetCurrent(ci);
            Display();
            break;
            }
         }
     numberTimer.Set(CHANNELNUMBERTIMEOUT);
     }
  return osContinue;
}

eOSState cMenuChannels::Switch(void)
{
  if (HasSubMenu())
     return osContinue;
  cChannel *ch = GetChannel(Current());
  if (ch)
     return cDevice::PrimaryDevice()->SwitchChannel(ch, true) ? osEnd : osContinue;
  return osEnd;
}

eOSState cMenuChannels::Edit(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cChannel *ch = GetChannel(Current());
  if (ch)
     return AddSubMenu(new cMenuEditChannel(ch));
  return osContinue;
}

eOSState cMenuChannels::New(void)
{
  if (HasSubMenu())
     return osContinue;
  return AddSubMenu(new cMenuEditChannel(GetChannel(Current()), true));
}

eOSState cMenuChannels::Delete(void)
{
  if (!HasSubMenu() && Count() > 0) {
     int CurrentChannelNr = cDevice::CurrentChannel();
     cChannel *CurrentChannel = Channels.GetByNumber(CurrentChannelNr);
     int Index = Current();
     cChannel *channel = GetChannel(Current());
     int DeletedChannel = channel->Number();
     // Check if there is a timer using this channel:
     if (channel->HasTimer()) {
        Skins.Message(mtError, tr("Channel is being used by a timer!"));
        return osContinue;
        }
     if (Interface->Confirm(tr("Delete channel?"))) {
        if (CurrentChannel && channel == CurrentChannel) {
           int n = Channels.GetNextNormal(CurrentChannel->Index());
           if (n < 0)
              n = Channels.GetPrevNormal(CurrentChannel->Index());
           CurrentChannel = Channels.Get(n);
           CurrentChannelNr = 0; // triggers channel switch below
           }
        Channels.Del(channel);
        cOsdMenu::Del(Index);
        Propagate();
        Channels.SetModified(true);
        isyslog("channel %d deleted", DeletedChannel);
        if (CurrentChannel && CurrentChannel->Number() != CurrentChannelNr) {
           if (!cDevice::PrimaryDevice()->Replaying() || cDevice::PrimaryDevice()->Transferring())
              Channels.SwitchTo(CurrentChannel->Number());
           else
              cDevice::SetCurrentChannel(CurrentChannel);
           }
        }
     }
  return osContinue;
}

void cMenuChannels::Move(int From, int To)
{
  int CurrentChannelNr = cDevice::CurrentChannel();
  cChannel *CurrentChannel = Channels.GetByNumber(CurrentChannelNr);
  cChannel *FromChannel = GetChannel(From);
  cChannel *ToChannel = GetChannel(To);
  if (FromChannel && ToChannel) {
     int FromNumber = FromChannel->Number();
     int ToNumber = ToChannel->Number();
     Channels.Move(FromChannel, ToChannel);
     cOsdMenu::Move(From, To);
     Propagate();
     Channels.SetModified(true);
     isyslog("channel %d moved to %d", FromNumber, ToNumber);
     if (CurrentChannel && CurrentChannel->Number() != CurrentChannelNr) {
        if (!cDevice::PrimaryDevice()->Replaying() || cDevice::PrimaryDevice()->Transferring())
           Channels.SwitchTo(CurrentChannel->Number());
        else
           cDevice::SetCurrentChannel(CurrentChannel);
        }
     }
}

eOSState cMenuChannels::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  switch (state) {
    case osUser1: {
         cChannel *channel = Channels.Last();
         if (channel) {
            Add(new cMenuChannelItem(channel), true);
            return CloseSubMenu();
            }
         }
         break;
    default:
         if (state == osUnknown) {
            switch (Key) {
              case k0 ... k9:
                            return Number(Key);
              case kOk:     return Switch();
              case kRed:    return Edit();
              case kGreen:  return New();
              case kYellow: return Delete();
              case kBlue:   if (!HasSubMenu())
                               Mark();
                            break;
              default: break;
              }
            }
    }
  return state;
}

// --- cMenuText -------------------------------------------------------------

cMenuText::cMenuText(const char *Title, const char *Text, eDvbFont Font)
:cOsdMenu(Title)
{
  text = NULL;
  font = Font;
  SetText(Text);
}

cMenuText::~cMenuText()
{
  free(text);
}

void cMenuText::SetText(const char *Text)
{
  free(text);
  text = Text ? strdup(Text) : NULL;
}

void cMenuText::Display(void)
{
  cOsdMenu::Display();
  DisplayMenu()->SetText(text, font == fontFix); //XXX define control character in text to choose the font???
  if (text)
     cStatus::MsgOsdTextItem(text);
}

eOSState cMenuText::ProcessKey(eKeys Key)
{
  switch (int(Key)) {
    case kUp|k_Repeat:
    case kUp:
    case kDown|k_Repeat:
    case kDown:
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight:
                  DisplayMenu()->Scroll(NORMALKEY(Key) == kUp || NORMALKEY(Key) == kLeft, NORMALKEY(Key) == kLeft || NORMALKEY(Key) == kRight);
                  cStatus::MsgOsdTextItem(NULL, NORMALKEY(Key) == kUp || NORMALKEY(Key) == kLeft);
                  return osContinue;
    default: break;
    }

  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk: return osBack;
#ifdef REELVDR
       default:  state = osUnknown;
#else
       default:  state = osContinue;
#endif
       }
     }
  return state;
}

// --- cMenuFolderItem -------------------------------------------------------

class cMenuFolderItem : public cOsdItem {
private:
  cNestedItem *folder;
public:
  cMenuFolderItem(cNestedItem *Folder);
  cNestedItem *Folder(void) { return folder; }
  };

cMenuFolderItem::cMenuFolderItem(cNestedItem *Folder)
:cOsdItem(Folder->Text())
{
  folder = Folder;
  if (folder->SubItems())
     SetText(cString::sprintf("%s...", folder->Text()));
}

// --- cMenuEditFolder -------------------------------------------------------

class cMenuEditFolder : public cOsdMenu {
private:
  cList<cNestedItem> *list;
  cNestedItem *folder;
  char name[PATH_MAX];
  int subFolder;
  eOSState Confirm(void);
public:
  cMenuEditFolder(const char *Dir, cList<cNestedItem> *List, cNestedItem *Folder = NULL);
  cString GetFolder(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuEditFolder::cMenuEditFolder(const char *Dir, cList<cNestedItem> *List, cNestedItem *Folder)
:cOsdMenu(Folder ? tr("Edit folder") : tr("New folder"), 12)
{
  list = List;
  folder = Folder;
  if (folder) {
     strn0cpy(name, folder->Text(), sizeof(name));
     subFolder = folder->SubItems() != NULL;
     }
  else {
     *name = 0;
     subFolder = 0;
     cRemote::Put(kRight, true); // go right into string editing mode
     }
  if (!isempty(Dir)) {
     cOsdItem *DirItem = new cOsdItem(Dir);
     DirItem->SetSelectable(false);
     Add(DirItem);
     }
  Add(new cMenuEditStrItem( tr("Name"), name, sizeof(name)));
  Add(new cMenuEditBoolItem(tr("Sub folder"), &subFolder));
}

cString cMenuEditFolder::GetFolder(void)
{
  return folder ? folder->Text() : "";
}

eOSState cMenuEditFolder::Confirm(void)
{
  if (!folder || strcmp(folder->Text(), name) != 0) {
     // each name may occur only once in a folder list
     for (cNestedItem *Folder = list->First(); Folder; Folder = list->Next(Folder)) {
         if (strcmp(Folder->Text(), name) == 0) {
            Skins.Message(mtError, tr("Folder name already exists!"));
            return osContinue;
            }
         }
     char *p = strpbrk(name, "\\{}#~"); // FOLDERDELIMCHAR
     if (p) {
        Skins.Message(mtError, cString::sprintf(tr("Folder name must not contain '%c'!"), *p));
        return osContinue;
        }
     }
  if (folder) {
     folder->SetText(name);
     folder->SetSubItems(subFolder);
     }
  else
     list->Add(folder = new cNestedItem(name, subFolder));
  return osEnd;
}

eOSState cMenuEditFolder::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk:     return Confirm();
       case kRed:
       case kGreen:
       case kYellow:
       case kBlue:   return osContinue;
       default: break;
       }
     }
  return state;
}

// --- cMenuFolder -----------------------------------------------------------

cMenuFolder::cMenuFolder(const char *Title, cNestedItemList *NestedItemList, const char *Path)
:cOsdMenu(Title)
{
  list = nestedItemList = NestedItemList;
  firstFolder = NULL;
  editing = false;
  Set();
  SetHelpKeys();
  DescendPath(Path);
}

cMenuFolder::cMenuFolder(const char *Title, cList<cNestedItem> *List, cNestedItemList *NestedItemList, const char *Dir, const char *Path)
:cOsdMenu(Title)
{
  list = List;
  nestedItemList = NestedItemList;
  dir = Dir;
  firstFolder = NULL;
  editing = false;
  Set();
  SetHelpKeys();
  DescendPath(Path);
}

void cMenuFolder::SetHelpKeys(void)
{
  SetHelp(firstFolder ? tr("Button$Select") : NULL, tr("Button$New"), firstFolder ? tr("Button$Delete") : NULL, firstFolder ? tr("Button$Edit") : NULL);
}

void cMenuFolder::Set(const char *CurrentFolder)
{
  firstFolder = NULL;
  Clear();
  if (!isempty(dir)) {
     cOsdItem *DirItem = new cOsdItem(dir);
     DirItem->SetSelectable(false);
     Add(DirItem);
     }
  list->Sort();
  for (cNestedItem *Folder = list->First(); Folder; Folder = list->Next(Folder)) {
      cOsdItem *FolderItem = new cMenuFolderItem(Folder);
      Add(FolderItem, CurrentFolder ? strcmp(Folder->Text(), CurrentFolder) == 0 : false);
      if (!firstFolder)
         firstFolder = FolderItem;
      }
}

void cMenuFolder::DescendPath(const char *Path)
{
  if (Path) {
     const char *p = strchr(Path, FOLDERDELIMCHAR);
     if (p) {
        for (cMenuFolderItem *Folder = (cMenuFolderItem *)firstFolder; Folder; Folder = (cMenuFolderItem *)Next(Folder)) {
            if (strncmp(Folder->Folder()->Text(), Path, p - Path) == 0) {
               SetCurrent(Folder);
               if (Folder->Folder()->SubItems())
                  AddSubMenu(new cMenuFolder(Title(), Folder->Folder()->SubItems(), nestedItemList, !isempty(dir) ? *cString::sprintf("%s%c%s", *dir, FOLDERDELIMCHAR, Folder->Folder()->Text()) : Folder->Folder()->Text(), p + 1));
               break;
               }
            }
        }
    }
}

eOSState cMenuFolder::Select(void)
{
  if (firstFolder) {
     cMenuFolderItem *Folder = (cMenuFolderItem *)Get(Current());
     if (Folder) {
        if (Folder->Folder()->SubItems())
           return AddSubMenu(new cMenuFolder(Title(), Folder->Folder()->SubItems(), nestedItemList, !isempty(dir) ? *cString::sprintf("%s%c%s", *dir, FOLDERDELIMCHAR, Folder->Folder()->Text()) : Folder->Folder()->Text()));
        else
           return osEnd;
        }
     }
  return osContinue;
}

eOSState cMenuFolder::New(void)
{
  editing = true;
  return AddSubMenu(new cMenuEditFolder(dir, list));
}

eOSState cMenuFolder::Delete(void)
{
  if (!HasSubMenu() && firstFolder) {
     cMenuFolderItem *Folder = (cMenuFolderItem *)Get(Current());
     if (Folder && Interface->Confirm(Folder->Folder()->SubItems() ? tr("Delete folder and all sub folders?") : tr("Delete folder?"))) {
        list->Del(Folder->Folder());
        Del(Folder->Index());
        firstFolder = Get(isempty(dir) ? 0 : 1);
        Display();
        SetHelpKeys();
        nestedItemList->Save();
        }
     }
  return osContinue;
}

eOSState cMenuFolder::Edit(void)
{
  if (!HasSubMenu() && firstFolder) {
     cMenuFolderItem *Folder = (cMenuFolderItem *)Get(Current());
     if (Folder) {
        editing = true;
        return AddSubMenu(new cMenuEditFolder(dir, list, Folder->Folder()));
        }
     }
  return osContinue;
}

eOSState cMenuFolder::SetFolder(void)
{
  cMenuEditFolder *mef = (cMenuEditFolder *)SubMenu();
  if (mef) {
     Set(mef->GetFolder());
     SetHelpKeys();
     Display();
     nestedItemList->Save();
     }
  return CloseSubMenu();
}

cString cMenuFolder::GetFolder(void)
{
  if (firstFolder) {
     cMenuFolderItem *Folder = (cMenuFolderItem *)Get(Current());
     if (Folder) {
        cMenuFolder *mf = (cMenuFolder *)SubMenu();
        if (mf)
           return cString::sprintf("%s%c%s", Folder->Folder()->Text(), FOLDERDELIMCHAR, *mf->GetFolder());
        return Folder->Folder()->Text();
        }
     }
  return "";
}

eOSState cMenuFolder::ProcessKey(eKeys Key)
{
  if (!HasSubMenu())
     editing = false;
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk:
       case kRed:    return Select();
       case kGreen:  return New();
       case kYellow: return Delete();
       case kBlue:   return Edit();
       default:      state = osContinue;
       }
     }
  else if (state == osEnd && HasSubMenu() && editing)
     state = SetFolder();
  return state;
}

// --- cMenuEditTimer --------------------------------------------------------

cMenuEditTimer::cMenuEditTimer(cTimer *Timer, bool New)
:cOsdMenu(tr("Edit timer"), 12)
{
  file = NULL;
  firstday = NULL;
  timer = Timer;
  addIfConfirmed = New;
  if (timer) {
     data = *timer;
     if (New)
        data.SetFlags(tfActive);
     channel = data.Channel()->Number();
     Add(new cMenuEditBitItem( tr("Active"),       &data.flags, tfActive));
     Add(new cMenuEditChanItem(tr("Channel"),      &channel));
     Add(new cMenuEditDateItem(tr("Day"),          &data.day, &data.weekdays));
     Add(new cMenuEditTimeItem(tr("Start"),        &data.start));
     Add(new cMenuEditTimeItem(tr("Stop"),         &data.stop));
     Add(new cMenuEditBitItem( tr("VPS"),          &data.flags, tfVps));
     Add(new cMenuEditIntItem( tr("Priority"),     &data.priority, 0, MAXPRIORITY));
     Add(new cMenuEditIntItem( tr("Lifetime"),     &data.lifetime, 0, MAXLIFETIME));
#ifdef USE_PINPLUGIN
     if (cOsd::pinValid || !data.fskProtection) Add(new cMenuEditBoolItem(tr("Childlock"),&data.fskProtection));
     else {
        char* buf = 0;
        if (asprintf(&buf, "%s\t%s", tr("Childlock"), data.fskProtection ? tr("yes") : tr("no")) > 0) {
           Add(new cOsdItem(buf));
           free(buf);
           }
        }

#endif /* PINPLUGIN */
     Add(file = new cMenuEditStrItem( tr("File"),   data.file, sizeof(data.file)));
     SetFirstDayItem();
     }
  SetHelpKeys();
  Timers.IncBeingEdited();
}

cMenuEditTimer::~cMenuEditTimer()
{
  if (timer && addIfConfirmed)
     delete timer; // apparently it wasn't confirmed
  Timers.DecBeingEdited();
}

void cMenuEditTimer::SetHelpKeys(void)
{
  SetHelp(tr("Button$Folder"));
}

void cMenuEditTimer::SetFirstDayItem(void)
{
  if (!firstday && !data.IsSingleEvent()) {
     Add(firstday = new cMenuEditDateItem(tr("First day"), &data.day));
     Display();
     }
  else if (firstday && data.IsSingleEvent()) {
     Del(firstday->Index());
     firstday = NULL;
     Display();
     }
}

eOSState cMenuEditTimer::SetFolder(void)
{
  cMenuFolder *mf = (cMenuFolder *)SubMenu();
  if (mf) {
     cString Folder = mf->GetFolder();
     char *p = strrchr(data.file, FOLDERDELIMCHAR);
     if (p)
        p++;
     else
        p = data.file;
     if (!isempty(*Folder))
        strn0cpy(data.file, cString::sprintf("%s%c%s", *Folder, FOLDERDELIMCHAR, p), sizeof(data.file));
     else if (p != data.file)
        memmove(data.file, p, strlen(p) + 1);
     SetCurrent(file);
     Display();
     }
  return CloseSubMenu();
}

eOSState cMenuEditTimer::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk:     {
                       cChannel *ch = Channels.GetByNumber(channel);
                       if (ch)
                          data.channel = ch;
                       else {
                          Skins.Message(mtError, tr("*** Invalid Channel ***"));
                          break;
                          }
                       if (!*data.file)
                          strcpy(data.file, data.Channel()->ShortName(true));
                       if (timer) {
                          if (memcmp(timer, &data, sizeof(data)) != 0)
                             *timer = data;
                          if (addIfConfirmed)
                             Timers.Add(timer);
                          timer->SetEventFromSchedule();
                          timer->Matches();
                          Timers.SetModified();
                          isyslog("timer %s %s (%s)", *timer->ToDescr(), addIfConfirmed ? "added" : "modified", timer->HasFlags(tfActive) ? "active" : "inactive");
                          addIfConfirmed = false;
                          }
                     }
                     return osBack;
       case kRed:    return AddSubMenu(new cMenuFolder(tr("Select folder"), &Folders, data.file));
       case kGreen:
       case kYellow:
       case kBlue:   return osContinue;
       default: break;
       }
     }
  else if (state == osEnd && HasSubMenu())
     state = SetFolder();
  if (Key != kNone)
     SetFirstDayItem();
  return state;
}

// --- cMenuTimerItem --------------------------------------------------------

class cMenuTimerItem : public cOsdItem {
private:
  cTimer *timer;
#ifdef USE_TIMERINFO
  char diskStatus;
#endif /* TIMERINFO */
public:
  cMenuTimerItem(cTimer *Timer);
#ifdef USE_TIMERINFO
  void SetDiskStatus(char DiskStatus);
#endif /* TIMERINFO */
  virtual int Compare(const cListObject &ListObject) const;
  virtual void Set(void);
  cTimer *Timer(void) { return timer; }
  };

cMenuTimerItem::cMenuTimerItem(cTimer *Timer)
{
  timer = Timer;
#ifdef USE_TIMERINFO
  diskStatus = ' ';
#endif /* TIMERINFO */
  Set();
}

int cMenuTimerItem::Compare(const cListObject &ListObject) const
{
  return timer->Compare(*((cMenuTimerItem *)&ListObject)->timer);
}

void cMenuTimerItem::Set(void)
{
  cString day, name("");
  if (timer->WeekDays())
     day = timer->PrintDay(0, timer->WeekDays(), false);
  else if (timer->Day() - time(NULL) < 28 * SECSINDAY) {
     day = itoa(timer->GetMDay(timer->Day()));
     name = WeekDayName(timer->Day());
     }
  else {
     struct tm tm_r;
     time_t Day = timer->Day();
     localtime_r(&Day, &tm_r);
     char buffer[16];
#if REELVDR
     strftime(buffer, sizeof(buffer), "%d.%m", &tm_r); // column is wide enough only for 5 chars, avoid full date as below
#else
     strftime(buffer, sizeof(buffer), "%Y%m%d", &tm_r);
#endif /* REELVDR */
     day = buffer;
     }
  const char *File = Setup.FoldersInTimerMenu ? NULL : strrchr(timer->File(), FOLDERDELIMCHAR);

#if REELVDR && APIVERSNUM >= 10718
  /* do not show folder name */
  if (Setup.FoldersInTimerMenu)
      File = strrchr(timer->File(), FOLDERDELIMCHAR);
  // else, already the 'File' is without foldernames
#endif /* REELVDR && APIVERSNUM >= 10718 */

  if (File && strcmp(File + 1, TIMERMACRO_TITLE) && strcmp(File + 1, TIMERMACRO_EPISODE))
     File++;
  else
     File = timer->File();

 #if REELVDR && APIVERSNUM >= 10718
  // show Event title, if available from Event
  if (timer->Event() && timer->Event()->Title())
      File = timer->Event()->Title();
#endif

#ifdef USE_WAREAGLEICON
#ifdef USE_TIMERINFO
  cCharSetConv csc("ISO-8859-1", cCharSetConv::SystemCharacterTable());
  char diskStatusString[2] = { diskStatus, 0 };
  SetText(cString::sprintf("%s%s\t%d\t%s%s%s\t%02d:%02d\t%02d:%02d\t%s",
                    csc.Convert(diskStatusString),
#else
  SetText(cString::sprintf("%s\t%d\t%s%s%s\t%02d:%02d\t%02d:%02d\t%s",
#endif /* TIMERINFO */
#else
#ifdef USE_TIMERINFO
  cCharSetConv csc("ISO-8859-1", cCharSetConv::SystemCharacterTable());
  char diskStatusString[2] = { diskStatus, 0 };
  SetText(cString::sprintf("%s%c\t%d\t%s%s%s\t%02d:%02d\t%02d:%02d\t%s",
                    csc.Convert(diskStatusString),
#else
#ifdef REELVDR
#if APIVERSNUM >= 10718
  /* do not show end time as it is shown in "Side Note"*/
  SetText(cString::sprintf("%c\t%s\t%s%s%s\t%02d:%02d\t%s",
#else
   SetText(cString::sprintf("%c\t%s\t%s%s%s\t%02d:%02d - %02d:%02d\t%s",
#endif /* APIVERSNUM >= 10718 */
#else
  SetText(cString::sprintf("%c\t%d\t%s%s%s\t%02d:%02d\t%02d:%02d\t%s",
#endif /* REELVDR */
#endif /* TIMERINFO */
#endif /* WAREAGLEICON */
#ifdef USE_WAREAGLEICON
                    !(timer->HasFlags(tfActive)) ? " " : timer->FirstDay() ? Setup.WarEagleIcons ? IsLangUtf8() ? ICON_ARROW_UTF8 : ICON_ARROW : "!" : timer->Recording() ? Setup.WarEagleIcons ? IsLangUtf8() ? ICON_REC_UTF8 : ICON_REC : "#" : Setup.WarEagleIcons ? IsLangUtf8() ? ICON_CLOCK_UTF8 : ICON_CLOCK : ">",

#else
                    !(timer->HasFlags(tfActive)) ? ' ' : timer->FirstDay() ? '!' : timer->Recording() ? '#' : '>',
#endif /* WAREAGLEICON */
#ifdef REELVDR
		    timer->Channel()->Name(),
#else
                    timer->Channel()->Number(),
#endif
                    *name,
                    *name && **name ? " " : "",
                    *day,
                    timer->Start() / 100,
                    timer->Start() % 100,
#if !(REELVDR && APIVERSNUM >= 10718) /*do not show shop time in reelvdr >= 10718*/
                    timer->Stop() / 100,
                    timer->Stop() % 100,
#endif /*!REELVDR || APIVERSNUM < 10718 */
                    File));
}

#ifdef USE_TIMERINFO
void cMenuTimerItem::SetDiskStatus(char DiskStatus)
{
  diskStatus = DiskStatus;
  Set();
}

// --- cTimerEntry -----------------------------------------------------------

class cTimerEntry : public cListObject {
private:
  cMenuTimerItem *item;
  const cTimer *timer;
  time_t start;
public:
  cTimerEntry(cMenuTimerItem *item) : item(item), timer(item->Timer()), start(timer->StartTime()) {}
  cTimerEntry(const cTimer *timer, time_t start) : item(NULL), timer(timer), start(start) {}
  virtual int Compare(const cListObject &ListObject) const;
  bool active(void) const { return timer->HasFlags(tfActive); }
  time_t startTime(void) const { return start; }
  int priority(void) const { return timer->Priority(); }
  int duration(void) const;
  bool repTimer(void) const { return !timer->IsSingleEvent(); }
  bool isDummy(void) const { return item == NULL; }
  const cTimer *Timer(void) const { return timer; }
  void SetDiskStatus(char DiskStatus);
  };

int cTimerEntry::Compare(const cListObject &ListObject) const
{
  cTimerEntry *entry = (cTimerEntry *)&ListObject;
  int r = startTime() - entry->startTime();
  if (r == 0)
     r = entry->priority() - priority();
  return r;
}

int cTimerEntry::duration(void) const
{
  int dur = (timer->Stop()  / 100 * 60 + timer->Stop()  % 100) -
            (timer->Start() / 100 * 60 + timer->Start() % 100);
  if (dur < 0)
     dur += 24 * 60;
  return dur;
}

void cTimerEntry::SetDiskStatus(char DiskStatus)
{
  if (item)
     item->SetDiskStatus(DiskStatus);
}
#endif /* TIMERINFO */
// --- cMenuTimers -----------------------------------------------------------

class cMenuTimers : public cOsdMenu {
private:
  int helpKeys;
  eOSState Edit(void);
  eOSState New(void);
  eOSState Delete(void);
  eOSState OnOff(void);
  eOSState Info(void);
  cTimer *CurrentTimer(void);
  void SetHelpKeys(void);
#ifdef USE_TIMERINFO
  void ActualiseDiskStatus(void);
  bool actualiseDiskStatus;
#endif /* TIMERINFO */
public:
  cMenuTimers(void);
  virtual ~cMenuTimers();
#ifdef USE_TIMERINFO
  virtual void Display(void);
#endif /* TIMERINFO */
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuTimers::cMenuTimers(void)
#ifdef USE_TIMERINFO
:cOsdMenu(tr("Timers"), 3, CHNUMWIDTH, 10, 6, 6)
#else
:cOsdMenu(tr("Timers"), 2, CHNUMWIDTH, 10, 6, 6)
#endif /* TIMERINFO */
{
  helpKeys = -1;
  for (cTimer *timer = Timers.First(); timer; timer = Timers.Next(timer)) {
      timer->SetEventFromSchedule(); // make sure the event is current
      Add(new cMenuTimerItem(timer));
      }
  Sort();
  SetCurrent(First());
  SetHelpKeys();
  Timers.IncBeingEdited();
#ifdef USE_TIMERINFO
  actualiseDiskStatus = true;
#endif /* TIMERINFO */
}

cMenuTimers::~cMenuTimers()
{
  Timers.DecBeingEdited();
}

cTimer *cMenuTimers::CurrentTimer(void)
{
  cMenuTimerItem *item = (cMenuTimerItem *)Get(Current());
  return item ? item->Timer() : NULL;
}

void cMenuTimers::SetHelpKeys(void)
{
  int NewHelpKeys = 0;
  cTimer *timer = CurrentTimer();
  if (timer) {
     if (timer->Event())
        NewHelpKeys = 2;
     else
        NewHelpKeys = 1;
     }
  if (NewHelpKeys != helpKeys) {
     helpKeys = NewHelpKeys;
     SetHelp(helpKeys > 0 ? tr("Button$On/Off") : NULL, tr("Button$New"), helpKeys > 0 ? tr("Button$Delete") : NULL, helpKeys == 2 ? tr("Button$Info") : NULL);
     }
}

eOSState cMenuTimers::OnOff(void)
{
  if (HasSubMenu())
     return osContinue;
  cTimer *timer = CurrentTimer();
  if (timer) {
     timer->OnOff();
     timer->SetEventFromSchedule();
     RefreshCurrent();
#ifdef USE_TIMERINFO
     Display();
#else
     DisplayCurrent(true);
#endif /* TIMERINFO */
     if (timer->FirstDay())
        isyslog("timer %s first day set to %s", *timer->ToDescr(), *timer->PrintFirstDay());
     else
        isyslog("timer %s %sactivated", *timer->ToDescr(), timer->HasFlags(tfActive) ? "" : "de");
     Timers.SetModified();
     }
  return osContinue;
}

eOSState cMenuTimers::Edit(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  isyslog("editing timer %s", *CurrentTimer()->ToDescr());
  return AddSubMenu(new cMenuEditTimer(CurrentTimer()));
}

eOSState cMenuTimers::New(void)
{
  if (HasSubMenu())
     return osContinue;
  return AddSubMenu(new cMenuEditTimer(new cTimer, true));
}

eOSState cMenuTimers::Delete(void)
{
  // Check if this timer is active:
  cTimer *ti = CurrentTimer();
  if (ti) {
     if (Interface->Confirm(tr("Delete timer?"))) {
        if (ti->Recording()) {
           if (Interface->Confirm(tr("Timer still recording - really delete?"))) {
              ti->Skip();
              cRecordControls::Process(time(NULL));
              }
           else
              return osContinue;
           }
        isyslog("deleting timer %s", *ti->ToDescr());
        Timers.Del(ti);
        cOsdMenu::Del(Current());
        Timers.SetModified();
        Display();
        }
     }
  return osContinue;
}

eOSState cMenuTimers::Info(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cTimer *ti = CurrentTimer();
  if (ti && ti->Event())
     return AddSubMenu(new cMenuEvent(ti->Event()));
  return osContinue;
}

#ifdef USE_TIMERINFO
void cMenuTimers::ActualiseDiskStatus(void)
{
  if (!actualiseDiskStatus || !Count())
     return;

  // compute free disk space
  int freeMB, freeMinutes, runshortMinutes;
  VideoDiskSpace(&freeMB);
  freeMinutes = int(double(freeMB) * 1.1 / MB_PER_MINUTE); // overestimate by 10 percent
  runshortMinutes = freeMinutes / 5; // 20 Percent

  // fill entries list
  cTimerEntry *entry;
  cList<cTimerEntry> entries;
  for (cOsdItem *item = First(); item; item = Next(item))
     entries.Add(new cTimerEntry((cMenuTimerItem *)item));

  // search last start time
  time_t last = 0;
  for (entry = entries.First(); entry; entry = entries.Next(entry))
     last = max(entry->startTime(), last);

  // add entries for repeating timers
  for (entry = entries.First(); entry; entry = entries.Next(entry))
     if (entry->repTimer() && !entry->isDummy())
        for (time_t start = cTimer::IncDay(entry->startTime(), 1);
             start <= last;
             start = cTimer::IncDay(start, 1))
           if (entry->Timer()->DayMatches(start))
              entries.Add(new cTimerEntry(entry->Timer(), start));

  // set the disk-status
  entries.Sort();
  for (entry = entries.First(); entry; entry = entries.Next(entry)) {
     char status = ' ';
     if (entry->active()) {
        freeMinutes -= entry->duration();
        status = freeMinutes > runshortMinutes ? '+' : freeMinutes > 0 ? 177 /* +/- */ : '-';
        }
     entry->SetDiskStatus(status);
#ifdef DEBUG_TIMER_INFO
     dsyslog("timer-info: %c | %d | %s | %s | %3d | %+5d -> %+5d",
             status,
             entry->startTime(),
             entry->active() ? "aktiv " : "n.akt.",
             entry->repTimer() ? entry->isDummy() ? "  dummy  " : "mehrmalig" : "einmalig ",
             entry->duration(),
             entry->active() ? freeMinutes + entry->duration() : freeMinutes,
             freeMinutes);
#endif
     }

  actualiseDiskStatus = false;
}

void cMenuTimers::Display(void)
{
  ActualiseDiskStatus();
  cOsdMenu::Display();
}
#endif /* TIMERINFO */
eOSState cMenuTimers::ProcessKey(eKeys Key)
{
  int TimerNumber = HasSubMenu() ? Count() : -1;
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk:     return Edit();
#ifdef USE_TIMERINFO
       case kRed:    actualiseDiskStatus = true;
                     state = OnOff(); break; // must go through SetHelpKeys()!
#else
       case kRed:    state = OnOff(); break; // must go through SetHelpKeys()!
#endif /* TIMERINFO */
       case kGreen:  return New();
#ifdef USE_TIMERINFO
       case kYellow: actualiseDiskStatus = true;
                     state = Delete(); break;
#else
       case kYellow: state = Delete(); break;
#endif /* TIMERINFO */
       case kInfo:
       case kBlue:   return Info();
                     break;
       default: break;
       }
     }
#ifdef USE_TIMERINFO
  if (TimerNumber >= 0 && !HasSubMenu()) {
     if (Timers.Get(TimerNumber)) // a newly created timer was confirmed with Ok
        Add(new cMenuTimerItem(Timers.Get(TimerNumber)), true);
     Sort();
     actualiseDiskStatus = true;
#else
  if (TimerNumber >= 0 && !HasSubMenu() && Timers.Get(TimerNumber)) {
     // a newly created timer was confirmed with Ok
     Add(new cMenuTimerItem(Timers.Get(TimerNumber)), true);
#endif /* TIMERINFO */
     Display();
     }
  if (Key != kNone)
     SetHelpKeys();
  return state;
}

// --- cMenuEvent ------------------------------------------------------------

cMenuEvent::cMenuEvent(const cEvent *Event, bool CanSwitch, bool Buttons)
:cOsdMenu(tr("Event"))
{
  event = Event;
  if (event) {
     cChannel *channel = Channels.GetByChannelID(event->ChannelID(), true);
     if (channel) {
        std::string title = tr("Description:");
        title += " ";
        title += channel->Name();
        //SetTitle(channel->Name());
        SetTitle(title.c_str());
        int TimerMatch = tmNone;
        Timers.GetMatch(event, &TimerMatch);
        if (Buttons)
           SetHelp(TimerMatch == tmFull ? tr("Button$Timer") : tr("Button$Record"), NULL, NULL, CanSwitch ? tr("Button$Switch") : NULL);
        }
     }
}

void cMenuEvent::Display(void)
{
  cOsdMenu::Display();
  DisplayMenu()->SetEvent(event);
#ifdef USE_GRAPHTFT
  cStatus::MsgOsdSetEvent(event);
#endif /* GRAPHTFT */
  if (event->Description())
     cStatus::MsgOsdTextItem(event->Description());
}

eOSState cMenuEvent::ProcessKey(eKeys Key)
{
  switch (int(Key)) {
    case kUp|k_Repeat:
    case kUp:
    case kDown|k_Repeat:
    case kDown:
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight:
                  DisplayMenu()->Scroll(NORMALKEY(Key) == kUp || NORMALKEY(Key) == kLeft, NORMALKEY(Key) == kLeft || NORMALKEY(Key) == kRight);
                  cStatus::MsgOsdTextItem(NULL, NORMALKEY(Key) == kUp || NORMALKEY(Key) == kLeft);
                  return osContinue;
    case kInfo:   return osBack;
    default: break;
    }

  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kGreen:
       case kYellow: return osContinue;
       case kOk:     return osBack;
       default: break;
       }
     }
  return state;
}

// --- cMenuScheduleItem -----------------------------------------------------

class cMenuScheduleItem : public cOsdItem {
public:
  enum eScheduleSortMode { ssmAllThis, ssmThisThis, ssmThisAll, ssmAllAll }; // "which event(s) on which channel(s)"
private:
  static eScheduleSortMode sortMode;
public:
  const cEvent *event;
  const cChannel *channel;
  bool withDate;
  int timerMatch;
#ifdef USE_LIEMIEXT
  bool withBar;
  cMenuScheduleItem(const cEvent *Event, cChannel *Channel = NULL, bool WithDate = false, bool WithBar = false);
#else
  cMenuScheduleItem(const cEvent *Event, cChannel *Channel = NULL, bool WithDate = false);
#endif /* LIEMIEXT */
  static void SetSortMode(eScheduleSortMode SortMode) { sortMode = SortMode; }
  static void IncSortMode(void) { sortMode = eScheduleSortMode((sortMode == ssmAllAll) ? ssmAllThis : sortMode + 1); }
  static eScheduleSortMode SortMode(void) { return sortMode; }
  virtual int Compare(const cListObject &ListObject) const;
  bool Update(bool Force = false);
  };

cMenuScheduleItem::eScheduleSortMode cMenuScheduleItem::sortMode = ssmAllThis;

#ifdef USE_LIEMIEXT
cMenuScheduleItem::cMenuScheduleItem(const cEvent *Event, cChannel *Channel, bool WithDate, bool WithBar)
#else
cMenuScheduleItem::cMenuScheduleItem(const cEvent *Event, cChannel *Channel, bool WithDate)
#endif /* LIEMIEXT */
{
  event = Event;
  channel = Channel;
  withDate = WithDate;
  timerMatch = tmNone;
#ifdef USE_LIEMIEXT
  withBar = WithBar;
#endif /* LIEMIEXT */
  Update(true);
}

int cMenuScheduleItem::Compare(const cListObject &ListObject) const
{
  cMenuScheduleItem *p = (cMenuScheduleItem *)&ListObject;
  int r = -1;
  if (sortMode != ssmAllThis)
     r = strcoll(event->Title(), p->event->Title());
  if (sortMode == ssmAllThis || r == 0)
     r = event->StartTime() - p->event->StartTime();
  return r;
}

#ifdef USE_WAREAGLEICON
static const char *TimerMatchChars[9] =
{
  " ", "t", "T",
  ICON_BLANK, ICON_CLOCK_UH, ICON_CLOCK,
  ICON_BLANK_UTF8, ICON_CLOCK_UH_UTF8, ICON_CLOCK_UTF8
};
#else
static const char *TimerMatchChars = " tT";
#endif /* WAREAGLEICON */

#ifdef USE_LIEMIEXT
static const char * const ProgressBar[7] =
{
  "[      ]",
  "[|     ]",
  "[||    ]",
  "[|||   ]",
  "[||||  ]",
  "[||||| ]",
  "[||||||]"
};

#endif /* LIEMIEXT */

bool cMenuScheduleItem::Update(bool Force)
{
  bool result = false;
  int OldTimerMatch = timerMatch;
  Timers.GetMatch(event, &timerMatch);
  if (Force || timerMatch != OldTimerMatch) {
     cString buffer;
#ifdef USE_WAREAGLEICON
     const char *t = Setup.WarEagleIcons ? IsLangUtf8() ? TimerMatchChars[timerMatch+6] : TimerMatchChars[timerMatch+3] : TimerMatchChars[timerMatch];
     const char *v = event->Vps() && (event->Vps() - event->StartTime()) ? Setup.WarEagleIcons ? IsLangUtf8() ? ICON_VPS_UTF8 : ICON_VPS : "V" : " ";
     const char *r = event->SeenWithin(30) && event->IsRunning() ? Setup.WarEagleIcons ? IsLangUtf8() ? ICON_RUNNING_UTF8 : ICON_RUNNING : "*" : " ";
#else
     char t = TimerMatchChars[timerMatch];
     char v = event->Vps() && (event->Vps() - event->StartTime()) ? 'V' : ' ';
     char r = event->SeenWithin(30) && event->IsRunning() ? '*' : ' ';
#endif /* WAREAGLEICON */
     const char *csn = channel ? channel->ShortName(true) : NULL;
     cString eds = event->GetDateString();
     if (channel && withDate)
#ifdef USE_WAREAGLEICON
        buffer = cString::sprintf("%d\t%.*s\t%.*s\t%s\t%s%s%s\t%s", channel->Number(), Utf8SymChars(csn, 6), csn, Utf8SymChars(eds, 6), *eds, *event->GetTimeString(), t, v, r, event->Title());
#else
        buffer = cString::sprintf("%d\t%.*s\t%.*s\t%s\t%c%c%c\t%s", channel->Number(), Utf8SymChars(csn, 6), csn, Utf8SymChars(eds, 6), *eds, *event->GetTimeString(), t, v, r, event->Title());
#endif /* WAREAGLEICON */
     else if (channel)
#ifdef USE_LIEMIEXT
        if (Setup.ShowProgressBar && withBar) {
           int progress = (int)roundf( (float)(time(NULL) - event->StartTime()) / (float)(event->Duration()) * 6.0 );
           if (progress < 0) progress = 0;
           else if (progress > 6) progress = 6;
#ifdef USE_WAREAGLEICON
           buffer = cString::sprintf("%d\t%.*s\t%s\t%s\t%s%s%s\t%s", channel->Number(), Utf8SymChars(csn, 6), csn, *event->GetTimeString(), ProgressBar[progress], t, v, r, event->Title());
#else
           buffer = cString::sprintf("%d\t%.*s\t%s\t%s\t%c%c%c\t%s", channel->Number(), Utf8SymChars(csn, 6), csn, *event->GetTimeString(), ProgressBar[progress], t, v, r, event->Title());
#endif /* WAREAGLEICON */
           }
        else
#ifdef USE_WAREAGLEICON
        buffer = cString::sprintf("%d\t%.*s\t%s\t%s%s%s\t%s", channel->Number(), Utf8SymChars(csn, 6), csn, *event->GetTimeString(), t, v, r, event->Title());
#else
        buffer = cString::sprintf("%d\t%.*s\t%s\t%c%c%c\t%s", channel->Number(), Utf8SymChars(csn, 6), csn, *event->GetTimeString(), t, v, r, event->Title());
#endif /* WAREAGLEICON */
#else
#ifdef USE_WAREAGLEICON
        buffer = cString::sprintf("%d\t%.*s\t%s\t%s%s%s\t%s", channel->Number(), Utf8SymChars(csn, 6), csn, *event->GetTimeString(), t, v, r, event->Title());
#else
        buffer = cString::sprintf("%d\t%.*s\t%s\t%c%c%c\t%s", channel->Number(), Utf8SymChars(csn, 6), csn, *event->GetTimeString(), t, v, r, event->Title());
#endif /* WAREAGLEICON */
#endif /* LIEMIEXT */
     else
#ifdef USE_WAREAGLEICON
        buffer = cString::sprintf("%.*s\t%s\t%s%s%s\t%s", Utf8SymChars(eds, 6), *eds, *event->GetTimeString(), t, v, r, event->Title());
#else
        buffer = cString::sprintf("%.*s\t%s\t%c%c%c\t%s", Utf8SymChars(eds, 6), *eds, *event->GetTimeString(), t, v, r, event->Title());
#endif /* WAREAGLEICON */
     SetText(buffer);
     result = true;
     }
  return result;
}

// --- cMenuWhatsOn ----------------------------------------------------------

class cMenuWhatsOn : public cOsdMenu {
private:
  bool now;
  int helpKeys;
  int timerState;
  eOSState Record(void);
  eOSState Switch(void);
  static int currentChannel;
  static const cEvent *scheduleEvent;
  bool Update(void);
  void SetHelpKeys(void);
public:
  cMenuWhatsOn(const cSchedules *Schedules, bool Now, int CurrentChannelNr);
  static int CurrentChannel(void) { return currentChannel; }
  static void SetCurrentChannel(int ChannelNr) { currentChannel = ChannelNr; }
  static const cEvent *ScheduleEvent(void);
  virtual eOSState ProcessKey(eKeys Key);
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind() { return now ? "MenuWhatsOnNow" : "MenuWhatsOnNext"; }
  virtual void Display(void);
#endif /* GRAPHTFT */
  };

int cMenuWhatsOn::currentChannel = 0;
const cEvent *cMenuWhatsOn::scheduleEvent = NULL;

cMenuWhatsOn::cMenuWhatsOn(const cSchedules *Schedules, bool Now, int CurrentChannelNr)
#ifdef USE_LIEMIEXT
:cOsdMenu(Now ? tr("What's on now?") : tr("What's on next?"), CHNUMWIDTH, 7, 6, 4, 4)
#else
:cOsdMenu(Now ? tr("What's on now?") : tr("What's on next?"), CHNUMWIDTH, 7, 6, 4)
#endif /* LIEMIEXT */
{
  now = Now;
  helpKeys = -1;
  timerState = 0;
  Timers.Modified(timerState);
  for (cChannel *Channel = Channels.First(); Channel; Channel = Channels.Next(Channel)) {
      if (!Channel->GroupSep()) {
         const cSchedule *Schedule = Schedules->GetSchedule(Channel);
         if (Schedule) {
            const cEvent *Event = Now ? Schedule->GetPresentEvent() : Schedule->GetFollowingEvent();
            if (Event)
#ifdef USE_LIEMIEXT
               Add(new cMenuScheduleItem(Event, Channel, false, Now), Channel->Number() == CurrentChannelNr);
#else
               Add(new cMenuScheduleItem(Event, Channel), Channel->Number() == CurrentChannelNr);
#endif /* LIEMIEXT */
            }
         }
      }
  currentChannel = CurrentChannelNr;
  Display();
  SetHelpKeys();
}

#ifdef USE_GRAPHTFT
void cMenuWhatsOn::Display(void)
{
   cOsdMenu::Display();

   if (Count() > 0) {
      int ni = 0;
      for (cOsdItem *item = First(); item; item = Next(item)) {
         cStatus::MsgOsdEventItem(((cMenuScheduleItem*)item)->event, item->Text(), ni++, Count());
      }
   }
}
#endif /* GRAPHTFT */

bool cMenuWhatsOn::Update(void)
{
  bool result = false;
  if (Timers.Modified(timerState)) {
     for (cOsdItem *item = First(); item; item = Next(item)) {
         if (((cMenuScheduleItem *)item)->Update())
            result = true;
         }
     }
  return result;
}

void cMenuWhatsOn::SetHelpKeys(void)
{
  cMenuScheduleItem *item = (cMenuScheduleItem *)Get(Current());
  int NewHelpKeys = 0;
  if (item) {
     if (item->timerMatch == tmFull)
        NewHelpKeys = 2;
     else
        NewHelpKeys = 1;
     }
  if (NewHelpKeys != helpKeys) {
     const char *Red[] = { NULL, tr("Button$Record"), tr("Button$Timer") };
     SetHelp(Red[NewHelpKeys], now ? tr("Button$Next") : tr("Button$Now"), tr("Button$Schedule"), tr("Button$Switch"));
     helpKeys = NewHelpKeys;
     }
}

const cEvent *cMenuWhatsOn::ScheduleEvent(void)
{
  const cEvent *ei = scheduleEvent;
  scheduleEvent = NULL;
  return ei;
}

eOSState cMenuWhatsOn::Switch(void)
{
  cMenuScheduleItem *item = (cMenuScheduleItem *)Get(Current());
  if (item) {
     cChannel *channel = Channels.GetByChannelID(item->event->ChannelID(), true);
     if (channel && cDevice::PrimaryDevice()->SwitchChannel(channel, true))
        return osEnd;
     }
  Skins.Message(mtError, tr("Can't switch channel!"));
  return osContinue;
}

eOSState cMenuWhatsOn::Record(void)
{
  cMenuScheduleItem *item = (cMenuScheduleItem *)Get(Current());
  if (item) {
     if (item->timerMatch == tmFull) {
        int tm = tmNone;
        cTimer *timer = Timers.GetMatch(item->event, &tm);
        if (timer)
           return AddSubMenu(new cMenuEditTimer(timer));
        }
     cTimer *timer = new cTimer(item->event);
     cTimer *t = Timers.GetTimer(timer);
     if (t) {
        delete timer;
        timer = t;
        return AddSubMenu(new cMenuEditTimer(timer));
        }
     else {
        Timers.Add(timer);
        Timers.SetModified();
        isyslog("timer %s added (active)", *timer->ToDescr());
        if (timer->Matches(0, false, NEWTIMERLIMIT))
           return AddSubMenu(new cMenuEditTimer(timer));
        if (HasSubMenu())
           CloseSubMenu();
        if (Update())
           Display();
        SetHelpKeys();
        }
     }
  return osContinue;
}

eOSState cMenuWhatsOn::ProcessKey(eKeys Key)
{
  bool HadSubMenu = HasSubMenu();
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kRecord:
       case kRed:    return Record();
       case kYellow: state = osBack;
                     // continue with kGreen
       case kGreen:  {
                       cMenuScheduleItem *mi = (cMenuScheduleItem *)Get(Current());
                       if (mi) {
                          scheduleEvent = mi->event;
                          currentChannel = mi->channel->Number();
                          }
                     }
                     break;
       case kBlue:   return Switch();
       case kInfo:
       case kOk:     if (Count())
                        return AddSubMenu(new cMenuEvent(((cMenuScheduleItem *)Get(Current()))->event, true, true));
                     break;
       default:      break;
       }
     }
  else if (!HasSubMenu()) {
     if (HadSubMenu && Update())
        Display();
     if (Key != kNone)
        SetHelpKeys();
     }
  return state;
}

// --- cMenuSchedule ---------------------------------------------------------

class cMenuSchedule : public cOsdMenu {
private:
  cSchedulesLock schedulesLock;
  const cSchedules *schedules;
  bool now, next;
  int otherChannel;
  int helpKeys;
  int timerState;
  eOSState Number(void);
  eOSState Record(void);
  eOSState Switch(void);
  void PrepareScheduleAllThis(const cEvent *Event, const cChannel *Channel);
  void PrepareScheduleThisThis(const cEvent *Event, const cChannel *Channel);
  void PrepareScheduleThisAll(const cEvent *Event, const cChannel *Channel);
  void PrepareScheduleAllAll(const cEvent *Event, const cChannel *Channel);
  bool Update(void);
  void SetHelpKeys(void);
public:
  cMenuSchedule(void);
  virtual ~cMenuSchedule();
  virtual eOSState ProcessKey(eKeys Key);
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind() { return "MenuSchedule"; }
  virtual void Display(void);
#endif /* GRAPHTFT */
  };

cMenuSchedule::cMenuSchedule(void)
:cOsdMenu("")
{
  now = next = false;
  otherChannel = 0;
  helpKeys = -1;
  timerState = 0;
  Timers.Modified(timerState);
  cMenuScheduleItem::SetSortMode(cMenuScheduleItem::ssmAllThis);
  cChannel *channel = Channels.GetByNumber(cDevice::CurrentChannel());
  if (channel) {
     cMenuWhatsOn::SetCurrentChannel(channel->Number());
     schedules = cSchedules::Schedules(schedulesLock);
     PrepareScheduleAllThis(NULL, channel);
     SetHelpKeys();
     }
}

cMenuSchedule::~cMenuSchedule()
{
  cMenuWhatsOn::ScheduleEvent(); // makes sure any posted data is cleared
}

#ifdef USE_GRAPHTFT
void cMenuSchedule::Display(void)
{
   cOsdMenu::Display();

   if (Count() > 0) {
      int ni = 0;
      for (cOsdItem *item = First(); item; item = Next(item)) {
         cStatus::MsgOsdEventItem(((cMenuScheduleItem*)item)->event, item->Text(), ni++, Count());
      }
   }
}
#endif /* GRAPHTFT */

void cMenuSchedule::PrepareScheduleAllThis(const cEvent *Event, const cChannel *Channel)
{
  Clear();
  SetCols(7, 6, 4);
  SetTitle(cString::sprintf(tr("Schedule - %s"), Channel->Name()));
  if (schedules && Channel) {
     const cSchedule *Schedule = schedules->GetSchedule(Channel);
     if (Schedule) {
        const cEvent *PresentEvent = Event ? Event : Schedule->GetPresentEvent();
        time_t now = time(NULL) - Setup.EPGLinger * 60;
        for (const cEvent *ev = Schedule->Events()->First(); ev; ev = Schedule->Events()->Next(ev)) {
            if (ev->EndTime() > now || ev == PresentEvent)
               Add(new cMenuScheduleItem(ev), ev == PresentEvent);
            }
        }
     }
}

void cMenuSchedule::PrepareScheduleThisThis(const cEvent *Event, const cChannel *Channel)
{
  Clear();
  SetCols(7, 6, 4);
  SetTitle(cString::sprintf(tr("This event - %s"), Channel->Name()));
  if (schedules && Channel && Event) {
     const cSchedule *Schedule = schedules->GetSchedule(Channel);
     if (Schedule) {
        time_t now = time(NULL) - Setup.EPGLinger * 60;
        for (const cEvent *ev = Schedule->Events()->First(); ev; ev = Schedule->Events()->Next(ev)) {
            if ((ev->EndTime() > now || ev == Event) && !strcmp(ev->Title(), Event->Title()))
               Add(new cMenuScheduleItem(ev), ev == Event);
            }
        }
     }
}

void cMenuSchedule::PrepareScheduleThisAll(const cEvent *Event, const cChannel *Channel)
{
  Clear();
  SetCols(CHNUMWIDTH, 7, 7, 6, 4);
  SetTitle(tr("This event - all channels"));
  if (schedules && Event) {
     for (cChannel *ch = Channels.First(); ch; ch = Channels.Next(ch)) {
         const cSchedule *Schedule = schedules->GetSchedule(ch);
         if (Schedule) {
            time_t now = time(NULL) - Setup.EPGLinger * 60;
            for (const cEvent *ev = Schedule->Events()->First(); ev; ev = Schedule->Events()->Next(ev)) {
                if ((ev->EndTime() > now || ev == Event) && !strcmp(ev->Title(), Event->Title()))
                   Add(new cMenuScheduleItem(ev, ch, true), ev == Event && ch == Channel);
                }
            }
         }
     }
}

void cMenuSchedule::PrepareScheduleAllAll(const cEvent *Event, const cChannel *Channel)
{
  Clear();
  SetCols(CHNUMWIDTH, 7, 7, 6, 4);
  SetTitle(tr("All events - all channels"));
  if (schedules) {
     for (cChannel *ch = Channels.First(); ch; ch = Channels.Next(ch)) {
         const cSchedule *Schedule = schedules->GetSchedule(ch);
         if (Schedule) {
            time_t now = time(NULL) - Setup.EPGLinger * 60;
            for (const cEvent *ev = Schedule->Events()->First(); ev; ev = Schedule->Events()->Next(ev)) {
                if (ev->EndTime() > now || ev == Event)
                   Add(new cMenuScheduleItem(ev, ch, true), ev == Event && ch == Channel);
                }
            }
         }
     }
}

bool cMenuSchedule::Update(void)
{
  bool result = false;
  if (Timers.Modified(timerState)) {
     for (cOsdItem *item = First(); item; item = Next(item)) {
         if (((cMenuScheduleItem *)item)->Update())
            result = true;
         }
     }
  return result;
}

void cMenuSchedule::SetHelpKeys(void)
{
  cMenuScheduleItem *item = (cMenuScheduleItem *)Get(Current());
  int NewHelpKeys = 0;
  if (item) {
     if (item->timerMatch == tmFull)
        NewHelpKeys = 2;
     else
        NewHelpKeys = 1;
     }
  if (NewHelpKeys != helpKeys) {
     const char *Red[] = { NULL, tr("Button$Record"), tr("Button$Timer") };
     SetHelp(Red[NewHelpKeys], tr("Button$Now"), tr("Button$Next"));
     helpKeys = NewHelpKeys;
     }
}

eOSState cMenuSchedule::Number(void)
{
  cMenuScheduleItem::IncSortMode();
  cMenuScheduleItem *CurrentItem = (cMenuScheduleItem *)Get(Current());
  const cChannel *Channel = NULL;
  const cEvent *Event = NULL;
  if (CurrentItem) {
     Event = CurrentItem->event;
     Channel = Channels.GetByChannelID(Event->ChannelID(), true);
     }
  else
     Channel = Channels.GetByNumber(cDevice::CurrentChannel());
  switch (cMenuScheduleItem::SortMode()) {
    case cMenuScheduleItem::ssmAllThis:  PrepareScheduleAllThis(Event, Channel); break;
    case cMenuScheduleItem::ssmThisThis: PrepareScheduleThisThis(Event, Channel); break;
    case cMenuScheduleItem::ssmThisAll:  PrepareScheduleThisAll(Event, Channel); break;
    case cMenuScheduleItem::ssmAllAll:   PrepareScheduleAllAll(Event, Channel); break;
    default: esyslog("ERROR: unknown SortMode %d (%s %d)", cMenuScheduleItem::SortMode(), __FUNCTION__, __LINE__);
    }
  CurrentItem = (cMenuScheduleItem *)Get(Current());
  Sort();
  SetCurrent(CurrentItem);
  Display();
  return osContinue;
}

eOSState cMenuSchedule::Record(void)
{
  cMenuScheduleItem *item = (cMenuScheduleItem *)Get(Current());
  if (item) {
     if (item->timerMatch == tmFull) {
        int tm = tmNone;
        cTimer *timer = Timers.GetMatch(item->event, &tm);
        if (timer)
           return AddSubMenu(new cMenuEditTimer(timer));
        }
     cTimer *timer = new cTimer(item->event);
     cTimer *t = Timers.GetTimer(timer);
     if (t) {
        delete timer;
        timer = t;
        return AddSubMenu(new cMenuEditTimer(timer));
        }
     else {
        Timers.Add(timer);
        Timers.SetModified();
        isyslog("timer %s added (active)", *timer->ToDescr());
        if (timer->Matches(0, false, NEWTIMERLIMIT))
           return AddSubMenu(new cMenuEditTimer(timer));
        if (HasSubMenu())
           CloseSubMenu();
        if (Update())
           Display();
        SetHelpKeys();
        }
     }
  return osContinue;
}

eOSState cMenuSchedule::Switch(void)
{
  if (otherChannel) {
     if (Channels.SwitchTo(otherChannel))
        return osEnd;
     }
  Skins.Message(mtError, tr("Can't switch channel!"));
  return osContinue;
}

eOSState cMenuSchedule::ProcessKey(eKeys Key)
{
  bool HadSubMenu = HasSubMenu();
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case k0:      return Number();
       case kRecord:
       case kRed:    return Record();
       case kGreen:  if (schedules) {
                        if (!now && !next) {
                           int ChannelNr = 0;
                           if (Count()) {
                              cChannel *channel = Channels.GetByChannelID(((cMenuScheduleItem *)Get(Current()))->event->ChannelID(), true);
                              if (channel)
                                 ChannelNr = channel->Number();
                              }
                           now = true;
                           return AddSubMenu(new cMenuWhatsOn(schedules, true, ChannelNr));
                           }
                        now = !now;
                        next = !next;
                        return AddSubMenu(new cMenuWhatsOn(schedules, now, cMenuWhatsOn::CurrentChannel()));
                        }
       case kYellow: if (schedules)
                        return AddSubMenu(new cMenuWhatsOn(schedules, false, cMenuWhatsOn::CurrentChannel()));
                     break;
       case kBlue:   if (Count() && otherChannel)
                        return Switch();
                     break;
       case kInfo:
       case kOk:     if (Count())
                        return AddSubMenu(new cMenuEvent(((cMenuScheduleItem *)Get(Current()))->event, otherChannel, true));
                     break;
       default:      break;
       }
     }
  else if (!HasSubMenu()) {
     now = next = false;
     const cEvent *ei = cMenuWhatsOn::ScheduleEvent();
     if (ei) {
        cChannel *channel = Channels.GetByChannelID(ei->ChannelID(), true);
        if (channel) {
           cMenuScheduleItem::SetSortMode(cMenuScheduleItem::ssmAllThis);
           PrepareScheduleAllThis(NULL, channel);
           if (channel->Number() != cDevice::CurrentChannel()) {
              otherChannel = channel->Number();
              SetHelp(Count() ? tr("Button$Record") : NULL, tr("Button$Now"), tr("Button$Next"), tr("Button$Switch"));
              }
           Display();
           }
        }
     else if (HadSubMenu && Update())
        Display();
     if (Key != kNone)
        SetHelpKeys();
     }
  return state;
}

// --- cMenuCommands ---------------------------------------------------------

cMenuCommands::cMenuCommands(const char *Title, cList<cNestedItem> *Commands, const char *Parameters)
:cOsdMenu(Title)
{
  result = NULL;
  SetHasHotkeys();
  commands = Commands;
  parameters = Parameters;
  for (cNestedItem *Command = commands->First(); Command; Command = commands->Next(Command)) {
      const char *s = Command->Text();
      if (Command->SubItems())
         Add(new cOsdItem(hk(cString::sprintf("%s...", s))));
      else if (Parse(s))
         Add(new cOsdItem(hk(title)));
      }
}

cMenuCommands::~cMenuCommands()
{
  free(result);
}

bool cMenuCommands::Parse(const char *s)
{
  const char *p = strchr(s, ':');
  if (p) {
     int l = p - s;
     if (l > 0) {
        char t[l + 1];
        stripspace(strn0cpy(t, s, l + 1));
        l = strlen(t);
        if (l > 1 && t[l - 1] == '?') {
           t[l - 1] = 0;
           confirm = true;
           }
        else
           confirm = false;
        title = t;
        command = skipspace(p + 1);
        return true;
        }
     }
  return false;
}

eOSState cMenuCommands::Execute(void)
{
  cNestedItem *Command = commands->Get(Current());
  if (Command) {
     if (Command->SubItems())
        return AddSubMenu(new cMenuCommands(Title(), Command->SubItems(), parameters));
     if (Parse(Command->Text())) {
        if (!confirm || Interface->Confirm(cString::sprintf("%s?", *title))) {
           Skins.Message(mtStatus, cString::sprintf("%s...", *title));
           free(result);
           result = NULL;
           cString cmdbuf;
           if (!isempty(parameters))
              cmdbuf = cString::sprintf("%s %s", *command, *parameters);
           const char *cmd = *cmdbuf ? *cmdbuf : *command;
           dsyslog("executing command '%s'", cmd);
           cPipe p;
           if (p.Open(cmd, "r")) {
              int l = 0;
              int c;
              while ((c = fgetc(p)) != EOF) {
                    if (l % 20 == 0) {
                       if (char *NewBuffer = (char *)realloc(result, l + 21))
                          result = NewBuffer;
                       else {
                          esyslog("ERROR: out of memory");
                          break;
                          }
                       }
                    result[l++] = char(c);
                    }
              if (result)
                 result[l] = 0;
              p.Close();
              }
           else
              esyslog("ERROR: can't open pipe for command '%s'", cmd);
           Skins.Message(mtStatus, NULL);
           if (result)
              return AddSubMenu(new cMenuText(title, result, fontFix));
           return osEnd;
           }
        }
     }
  return osContinue;
}

eOSState cMenuCommands::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kRed:
       case kGreen:
       case kYellow:
       case kBlue:   return osContinue;
       case kOk:     return Execute();
       default:      break;
       }
     }
  return state;
}

// --- cMenuCam --------------------------------------------------------------

class cMenuCam : public cOsdMenu {
private:
  cCamSlot *camSlot;
  cCiMenu *ciMenu;
  cCiEnquiry *ciEnquiry;
  char *input;
  int offset;
  time_t lastCamExchange;
  void GenerateTitle(const char *s = NULL);
  void QueryCam(void);
  void AddMultiLineItem(const char *s);
  void Set(void);
  eOSState Select(void);
public:
  cMenuCam(cCamSlot *CamSlot);
  virtual ~cMenuCam();
  virtual eOSState ProcessKey(eKeys Key);
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind() { return "MenuCam"; }
#endif /* GRAPHTFT */
  };

cMenuCam::cMenuCam(cCamSlot *CamSlot)
:cOsdMenu("", 1) // tab necessary for enquiry!
{
  camSlot = CamSlot;
  ciMenu = NULL;
  ciEnquiry = NULL;
  input = NULL;
  offset = 0;
  lastCamExchange = time(NULL);
  SetNeedsFastResponse(true);
  QueryCam();
}

cMenuCam::~cMenuCam()
{
  if (ciMenu)
     ciMenu->Abort();
  delete ciMenu;
  if (ciEnquiry)
     ciEnquiry->Abort();
  delete ciEnquiry;
  free(input);
}

void cMenuCam::GenerateTitle(const char *s)
{
  SetTitle(cString::sprintf("CAM %d - %s", camSlot->SlotNumber(), (s && *s) ? s : camSlot->GetCamName()));
}

void cMenuCam::QueryCam(void)
{
  delete ciMenu;
  ciMenu = NULL;
  delete ciEnquiry;
  ciEnquiry = NULL;
  if (camSlot->HasUserIO()) {
     ciMenu = camSlot->GetMenu();
     ciEnquiry = camSlot->GetEnquiry();
     }
  Set();
}

void cMenuCam::Set(void)
{
  if (ciMenu) {
     Clear();
     free(input);
     input = NULL;
     dsyslog("CAM %d: Menu ------------------", camSlot->SlotNumber());
     offset = 0;
     SetHasHotkeys(ciMenu->Selectable());
     GenerateTitle(ciMenu->TitleText());
     dsyslog("CAM %d: '%s'", camSlot->SlotNumber(), ciMenu->TitleText());
     if (*ciMenu->SubTitleText()) {
        dsyslog("CAM %d: '%s'", camSlot->SlotNumber(), ciMenu->SubTitleText());
        AddMultiLineItem(ciMenu->SubTitleText());
        offset = Count();
        }
     for (int i = 0; i < ciMenu->NumEntries(); i++) {
         Add(new cOsdItem(hk(ciMenu->Entry(i)), osUnknown, ciMenu->Selectable()));
         dsyslog("CAM %d: '%s'", camSlot->SlotNumber(), ciMenu->Entry(i));
         }
     if (*ciMenu->BottomText()) {
        AddMultiLineItem(ciMenu->BottomText());
        dsyslog("CAM %d: '%s'", camSlot->SlotNumber(), ciMenu->BottomText());
        }
     cRemote::TriggerLastActivity();
     }
  else if (ciEnquiry) {
     Clear();
     int Length = ciEnquiry->ExpectedLength();
     free(input);
     input = MALLOC(char, Length + 1);
     *input = 0;
     GenerateTitle();
     Add(new cOsdItem(ciEnquiry->Text(), osUnknown, false));
     Add(new cOsdItem("", osUnknown, false));
     Add(new cMenuEditNumItem("", input, Length, ciEnquiry->Blind()));
     }
  Display();
}

void cMenuCam::AddMultiLineItem(const char *s)
{
  while (s && *s) {
        const char *p = strchr(s, '\n');
        int l = p ? p - s : strlen(s);
        cOsdItem *item = new cOsdItem;
        item->SetSelectable(false);
        item->SetText(strndup(s, l), false);
        Add(item);
        s = p ? p + 1 : p;
        }
}

eOSState cMenuCam::Select(void)
{
  if (ciMenu) {
     if (ciMenu->Selectable()) {
        ciMenu->Select(Current() - offset);
        dsyslog("CAM %d: select %d", camSlot->SlotNumber(), Current() - offset);
        }
     else
        ciMenu->Cancel();
     }
  else if (ciEnquiry) {
     if (ciEnquiry->ExpectedLength() < 0xFF && int(strlen(input)) != ciEnquiry->ExpectedLength()) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), tr("Please enter %d digits!"), ciEnquiry->ExpectedLength());
        Skins.Message(mtError, buffer);
        return osContinue;
        }
     ciEnquiry->Reply(input);
     dsyslog("CAM %d: entered '%s'", camSlot->SlotNumber(), ciEnquiry->Blind() ? "****" : input);
     }
  QueryCam();
  return osContinue;
}

eOSState cMenuCam::ProcessKey(eKeys Key)
{
  if (!camSlot->HasMMI())
     return osBack;

  eOSState state = cOsdMenu::ProcessKey(Key);

  if (ciMenu || ciEnquiry) {
     lastCamExchange = time(NULL);
     if (state == osUnknown) {
        switch (Key) {
          case kOk: return Select();
          default: break;
          }
        }
     else if (state == osBack) {
        if (ciMenu)
           ciMenu->Cancel();
        if (ciEnquiry)
           ciEnquiry->Cancel();
        QueryCam();
        return osContinue;
        }
     if (ciMenu && ciMenu->HasUpdate()) {
        QueryCam();
        return osContinue;
        }
     }
  else if (time(NULL) - lastCamExchange < CAMRESPONSETIMEOUT)
     QueryCam();
  else {
     Skins.Message(mtError, tr("CAM not responding!"));
     return osBack;
     }
  return state;
}

// --- CamControl ------------------------------------------------------------

cOsdObject *CamControl(void)
{
  for (cCamSlot *CamSlot = CamSlots.First(); CamSlot; CamSlot = CamSlots.Next(CamSlot)) {
      if (CamSlot->HasUserIO())
         return new cMenuCam(CamSlot);
      }
  return NULL;
}

// --- cMenuRecording --------------------------------------------------------

class cMenuRecording : public cOsdMenu {
private:
  const cRecording *recording;
  bool withButtons;
public:
  cMenuRecording(const cRecording *Recording, bool WithButtons = false);
  virtual void Display(void);
  virtual eOSState ProcessKey(eKeys Key);
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind() { return "MenuRecording"; }
#endif /* GRAPHTFT */
};

cMenuRecording::cMenuRecording(const cRecording *Recording, bool WithButtons)
:cOsdMenu(tr("Recording info"))
{
  recording = Recording;
  withButtons = WithButtons;
  if (withButtons)
     SetHelp(tr("Button$Play"), tr("Button$Rewind"));
  //Display();
}

void cMenuRecording::Display(void)
{
  cOsdMenu::Display();
  DisplayMenu()->SetRecording(recording);
#ifdef USE_GRAPHTFT
  cStatus::MsgOsdSetRecording(recording);
#endif /* GRAPHTFT */
  if (recording->Info()->Description())
     cStatus::MsgOsdTextItem(recording->Info()->Description());
}

eOSState cMenuRecording::ProcessKey(eKeys Key)
{
  switch (int(Key)) {
    case kUp|k_Repeat:
    case kUp:
    case kDown|k_Repeat:
    case kDown:
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight:
                  DisplayMenu()->Scroll(NORMALKEY(Key) == kUp || NORMALKEY(Key) == kLeft, NORMALKEY(Key) == kLeft || NORMALKEY(Key) == kRight);
                  cStatus::MsgOsdTextItem(NULL, NORMALKEY(Key) == kUp || NORMALKEY(Key) == kLeft);
                  return osContinue;
    case kInfo:   return osBack;
    default: break;
    }

  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kRed:    if (withButtons)
                        Key = kOk; // will play the recording, even if recording commands are defined
       case kGreen:  if (!withButtons)
                        break;
                     cRemote::Put(Key, true);
                     // continue with osBack to close the info menu and process the key
       case kOk:     return osBack;
       default: break;
       }
     }
  return state;
}

// --- cMenuRecordingItem ----------------------------------------------------

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

cMenuRecordingItem::cMenuRecordingItem(cRecording *Recording, int Level)
{
  fileName = strdup(Recording->FileName());
  name = NULL;
  totalEntries = newEntries = 0;
#ifdef USE_LIEMIEXT
  SetText(Recording->Title('\t', true, Level, false));
#else
  SetText(Recording->Title('\t', true, Level));
#endif /* LIEMIEXT */
  if (*Text() == '\t')
     name = strdup(Text() + 2); // 'Text() + 2' to skip the two '\t'
}

cMenuRecordingItem::~cMenuRecordingItem()
{
  free(fileName);
  free(name);
}

void cMenuRecordingItem::IncrementCounter(bool New)
{
  totalEntries++;
  if (New)
     newEntries++;
#ifdef USE_LIEMIEXT
  switch (Setup.ShowRecTime + Setup.ShowRecDate + Setup.ShowRecLength) {
     case 0:
          SetText(cString::sprintf("%s", name));
          break;
     case 1:
          SetText(cString::sprintf("%d\t%s", totalEntries, name));
          break;
     case 2:
     default:
          SetText(cString::sprintf("%d\t%d\t%s", totalEntries, newEntries, name));
          break;
     case 3:
          SetText(cString::sprintf("%d\t%d\t\t%s", totalEntries, newEntries, name));
          break;
     }
}

// --- cMenuRenameRecording --------------------------------------------------

class cMenuRenameRecording : public cOsdMenu {
private:
  char name[MaxFileName];
  cMenuEditStrItem *file;
  cOsdItem *marksItem, *resumeItem;
  bool isResume, isMarks;
  cRecording *recording;
  void SetHelpKeys(void);
  eOSState SetFolder(void);
public:
  cMenuRenameRecording(cRecording *Recording);
  virtual eOSState ProcessKey(eKeys Key);
};

cMenuRenameRecording::cMenuRenameRecording(cRecording *Recording)
:cOsdMenu(tr("Rename recording"), 12)
{
  cMarks marks;

  file = NULL;
  recording = Recording;

  if (recording) {
     Utf8Strn0Cpy(name, recording->Name(), sizeof(name));
     Add(file = new cMenuEditStrItem(tr("File"), name, sizeof(name)));

     Add(new cOsdItem("", osUnknown, false));

     Add(new cOsdItem(cString::sprintf("%s:\t%s", tr("Date"), *DayDateTime(recording->Start())), osUnknown, false));

     cChannel *channel = Channels.GetByChannelID(((cRecordingInfo *)recording->Info())->ChannelID());
     if (channel)
        Add(new cOsdItem(cString::sprintf("%s:\t%s", tr("Channel"), *ChannelString(channel, 0)), osUnknown, false));

     int recLen = cIndexFile::GetLength(recording->FileName(), recording->IsPesRecording());
     if (recLen >= 0)
        Add(new cOsdItem(cString::sprintf("%s:\t%s", tr("Length"), *IndexToHMSF(recLen, false, recording->FramesPerSecond())), osUnknown, false));
     else
        recLen = 0;

     int dirSize = DirSizeMB(recording->FileName());
     double seconds = recLen / recording->FramesPerSecond();
     cString bitRate = seconds ? cString::sprintf(" (%.2f MBit/s)", 8.0 * dirSize / seconds) : cString("");
     Add(new cOsdItem(cString::sprintf("%s:\t%s", tr("Format"), recording->IsPesRecording() ? tr("PES") : tr("TS")), osUnknown, false));
     Add(new cOsdItem((dirSize > 9999) ? cString::sprintf("%s:\t%.2f GB%s", tr("Size"), dirSize / 1024.0, *bitRate) : cString::sprintf("%s:\t%d MB%s", tr("Size"), dirSize, *bitRate), osUnknown, false));

     Add(new cOsdItem("", osUnknown, false));

     isMarks = marks.Load(recording->FileName(), recording->FramesPerSecond(), recording->IsPesRecording()) && marks.Count();
     marksItem = new cOsdItem(tr("Delete marks information?"), osUser1, isMarks);
     Add(marksItem);

     cResumeFile ResumeFile(recording->FileName(), recording->IsPesRecording());
     isResume = (ResumeFile.Read() != -1);
     resumeItem = new cOsdItem(tr("Delete resume information?"), osUser2, isResume);
     Add(resumeItem);
     }

  SetHelpKeys();
}

void cMenuRenameRecording::SetHelpKeys(void)
{
  SetHelp(tr("Button$Folder"));
}

eOSState cMenuRenameRecording::SetFolder(void)
{
  cMenuFolder *mf = (cMenuFolder *)SubMenu();
  if (mf) {
     cString Folder = mf->GetFolder();
     char *p = strrchr(name, FOLDERDELIMCHAR);
     if (p)
        p++;
     else
        p = name;
     if (!isempty(*Folder))
        strn0cpy(name, cString::sprintf("%s%c%s", *Folder, FOLDERDELIMCHAR, p), sizeof(name));
     else if (p != name)
        memmove(name, p, strlen(p) + 1);
     SetCurrent(file);
     Display();
     }
  return CloseSubMenu();
}

eOSState cMenuRenameRecording::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk:
            if (recording->Rename(name)) {
               Recordings.ChangeState();
               Recordings.TouchUpdate();
               return osRecordings;
               }
            else
               Skins.Message(mtError, tr("Error while accessing recording!"));
            break;
       case kRed:
            return AddSubMenu(new cMenuFolder(tr("Select folder"), &Folders, name));
            break;
       default:
            break;
       }
     if (Key != kNone)
        SetHelpKeys();
     return osContinue;
     }
  else if (state == osEnd && HasSubMenu())
     state = SetFolder();
  else if (state == osUser1) {
     if (isMarks && Interface->Confirm(tr("Delete marks information?"))) {
        cMarks marks;
        marks.Load(recording->FileName(), recording->FramesPerSecond(), recording->IsPesRecording());
        cMark *mark = marks.First();
        while (mark) {
          cMark *nextmark = marks.Next(mark);
          marks.Del(mark);
          mark = nextmark;
          }
        marks.Save();
        isMarks = false;
        marksItem->SetSelectable(isMarks);
        SetCurrent(First());
        Display();
        }
     return osContinue;
     }
  else if (state == osUser2) {
     if (isResume && Interface->Confirm(tr("Delete resume information?"))) {
        cResumeFile ResumeFile(recording->FileName(), recording->IsPesRecording());
        ResumeFile.Delete();
        isResume = false;
        resumeItem->SetSelectable(isResume);
        SetCurrent(First());
        Display();
        }
     return osContinue;
     }

  return state;
#else
  SetText(cString::sprintf("%d\t\t%d\t%s", totalEntries, newEntries, name));
#endif /* LIEMIEXT */
}

// --- cMenuRecordings -------------------------------------------------------

cMenuRecordings::cMenuRecordings(const char *Base, int Level, bool OpenSubMenus)
#ifdef USE_LIEMIEXT
:cOsdMenu(Base ? Base : tr("Recordings"), 9, 7, 7)
#else
:cOsdMenu(Base ? Base : tr("Recordings"), 9, 6, 6)
#endif /* LIEMIEXT */
{
  base = Base ? strdup(Base) : NULL;
  level = Setup.RecordingDirs ? Level : -1;
  Recordings.StateChanged(recordingsState); // just to get the current state
  helpKeys = -1;
  Display(); // this keeps the higher level menus from showing up briefly when pressing 'Back' during replay
  Set();
  SetFreeDiskDisplay(true);
  if (Current() < 0)
     SetCurrent(First());
  else if (OpenSubMenus && cReplayControl::LastReplayed() && Open(true))
     return;
  Display();
  SetHelpKeys();
}

cMenuRecordings::~cMenuRecordings()
{
  helpKeys = -1;
  free(base);
}

bool cMenuRecordings::SetFreeDiskDisplay(bool Force)
{
  if (FreeDiskSpace.HasChanged(Force)) {
     //XXX -> skin function!!!
     SetTitle(cString::sprintf("%s  -  %s", base ? base : tr("Recordings"), FreeDiskSpace.FreeDiskSpaceString()));
     return true;
     }
  return false;
}

void cMenuRecordings::SetHelpKeys(void)
{
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  int NewHelpKeys = 0;
  if (ri) {
     if (ri->IsDirectory())
        NewHelpKeys = 1;
     else {
        NewHelpKeys = 2;
        cRecording *recording = GetRecording(ri);
        if (recording && recording->Info()->Title())
           NewHelpKeys = 3;
        }
     }
  if (NewHelpKeys != helpKeys) {
     switch (NewHelpKeys) {
       case 0: SetHelp(NULL); break;
       case 1: SetHelp(tr("Button$Open")); break;
       case 2:
       case 3: SetHelp(RecordingCommands.Count() ? tr("Commands") : tr("Button$Play"), tr("Button$Rewind"), tr("Button$Delete"), NewHelpKeys == 3 ? tr("Button$Info") : NULL);
       default: ;
       }
     helpKeys = NewHelpKeys;
     }
}

void cMenuRecordings::Set(bool Refresh)
{
  const char *CurrentRecording = cReplayControl::LastReplayed();
  cMenuRecordingItem *LastItem = NULL;
  char *LastItemText = NULL;
  cThreadLock RecordingsLock(&Recordings);
  if (Refresh) {
     cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
     if (ri) {
        cRecording *Recording = Recordings.GetByName(ri->FileName());
        if (Recording)
           CurrentRecording = Recording->FileName();
        }
     }
  Clear();
  Recordings.Sort();
  for (cRecording *recording = Recordings.First(); recording; recording = Recordings.Next(recording)) {
      if (!base || (strstr(recording->Name(), base) == recording->Name() && recording->Name()[strlen(base)] == FOLDERDELIMCHAR)) {
         cMenuRecordingItem *Item = new cMenuRecordingItem(recording, level);
#ifdef USE_PINPLUGIN
         if ((*Item->Text()
            && (!Item->IsDirectory() || (!LastItem || !LastItem->IsDirectory()
                || strcmp(Item->Text(), LastItemText) != 0)))
            && (!cStatus::MsgReplayProtected(GetRecording(Item), Item->Name(), base,
                                             Item->IsDirectory(), true))) {
#else
         if (*Item->Text() && (!Item->IsDirectory() || (!LastItem || !LastItem->IsDirectory() || strcmp(Item->Text(), LastItemText) != 0))) {
#endif /* PINPLUGIN */
            Add(Item);
            LastItem = Item;
            free(LastItemText);
            LastItemText = strdup(LastItem->Text()); // must use a copy because of the counters!
            }
         else
            delete Item;
         if (LastItem) {
            if (CurrentRecording && strcmp(CurrentRecording, recording->FileName()) == 0)
               SetCurrent(LastItem);
            if (LastItem->IsDirectory())
               LastItem->IncrementCounter(recording->IsNew());
            }
         }
      }
  free(LastItemText);
  Refresh |= SetFreeDiskDisplay(Refresh);
  if (Refresh)
     Display();
}

cRecording *cMenuRecordings::GetRecording(cMenuRecordingItem *Item)
{
  cRecording *recording = Recordings.GetByName(Item->FileName());
  if (!recording)
     Skins.Message(mtError, tr("Error while accessing recording!"));
  return recording;
}

bool cMenuRecordings::Open(bool OpenSubMenus)
{
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && ri->IsDirectory()) {
     const char *t = ri->Name();
     cString buffer;
     if (base) {
        buffer = cString::sprintf("%s~%s", base, t);
        t = buffer;
        }
     AddSubMenu(new cMenuRecordings(t, level + 1, OpenSubMenus));
     return true;
     }
  return false;
}

eOSState cMenuRecordings::Play(void)
{
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri) {
#ifdef USE_PINPLUGIN
     if (cStatus::MsgReplayProtected(GetRecording(ri), ri->Name(), base,
                                     ri->IsDirectory()) == true)
        return osContinue;
#endif /* PINPLUGIN */
     if (ri->IsDirectory())
        Open();
     else {
        cRecording *recording = GetRecording(ri);
        if (recording) {
           cReplayControl::SetRecording(recording->FileName(), recording->Title());
           return osReplay;
           }
        }
     }
  return osContinue;
}

eOSState cMenuRecordings::Rewind(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && !ri->IsDirectory()) {
     cRecording *recording = GetRecording(ri);
     if (recording) {
        cDevice::PrimaryDevice()->StopReplay(); // must do this first to be able to rewind the currently replayed recording
        cResumeFile ResumeFile(ri->FileName(), recording->IsPesRecording());
        ResumeFile.Delete();
        return Play();
        }
     }
  return osContinue;
}

eOSState cMenuRecordings::Delete(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && !ri->IsDirectory()) {
     if (Interface->Confirm(tr("Delete recording?"))) {
        cRecordControl *rc = cRecordControls::GetRecordControl(ri->FileName());
        if (rc) {
           if (Interface->Confirm(tr("Timer still recording - really delete?"))) {
              cTimer *timer = rc->Timer();
              if (timer) {
                 timer->Skip();
                 cRecordControls::Process(time(NULL));
                 if (timer->IsSingleEvent()) {
                    isyslog("deleting timer %s", *timer->ToDescr());
                    Timers.Del(timer);
                    }
                 Timers.SetModified();
                 }
              }
           else
              return osContinue;
           }
        cRecording *recording = GetRecording(ri);
        if (recording) {
           if (cReplayControl::NowReplaying() && strcmp(cReplayControl::NowReplaying(), ri->FileName()) == 0)
              cControl::Shutdown();
           if (recording->Delete()) {
              cReplayControl::ClearLastReplayed(ri->FileName());
              Recordings.DelByName(ri->FileName());
              cOsdMenu::Del(Current());
              SetHelpKeys();
              SetFreeDiskDisplay(true);
              Display();
              if (!Count())
                 return osBack;
              }
           else
              Skins.Message(mtError, tr("Error while deleting recording!"));
           }
        }
     }
  return osContinue;
}

eOSState cMenuRecordings::Info(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && !ri->IsDirectory()) {
     cRecording *recording = GetRecording(ri);
     if (recording && recording->Info()->Title())
        return AddSubMenu(new cMenuRecording(recording, true));
     }
  return osContinue;
}

eOSState cMenuRecordings::Commands(eKeys Key)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && !ri->IsDirectory()) {
     cRecording *recording = GetRecording(ri);
     if (recording) {
        cMenuCommands *menu;
        eOSState state = AddSubMenu(menu = new cMenuCommands(tr("Recording commands"), &RecordingCommands, cString::sprintf("\"%s\"", *strescape(recording->FileName(), "\\\"$"))));
        if (Key != kNone)
           state = menu->ProcessKey(Key);
        return state;
        }
     }
  return osContinue;
}

#ifdef USE_LIEMIEXT
eOSState cMenuRecordings::Rename(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && !ri->IsDirectory()) {
     cRecording *recording = GetRecording(ri);
     if (recording)
        return AddSubMenu(new cMenuRenameRecording(recording));
     }
  return osContinue;
}
#endif /* LIEMIEXT */

eOSState cMenuRecordings::ProcessKey(eKeys Key)
{
  bool HadSubMenu = HasSubMenu();
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kPlay:
       case kOk:     return Play();
       case kRed:    return (helpKeys > 1 && RecordingCommands.Count()) ? Commands() : Play();
       case kGreen:  return Rewind();
       case kYellow: return Delete();
       case kInfo:
       case kBlue:   return Info();
#ifdef USE_LIEMIEXT
       case k0:      DirOrderState = !DirOrderState;
                     Set(true);
                     return osContinue;
       case k8:      return Rename();
       case k9:
       case k1...k7: return Commands(Key);
#else
       case k1...k9: return Commands(Key);
#endif /* LIEMIEXT */
       case kNone:   if (Recordings.StateChanged(recordingsState))
                        Set(true);
                     break;
       default: break;
       }
     }
  if (Key == kYellow && HadSubMenu && !HasSubMenu()) {
     // the last recording in a subdirectory was deleted, so let's go back up
     cOsdMenu::Del(Current());
     if (!Count())
        return osBack;
     Display();
     }
  if (!HasSubMenu()) {
     if (HadSubMenu)
        SetFreeDiskDisplay();
     if (Key != kNone)
        SetHelpKeys();
     }
  return state;
}

// --- cMenuSetupBase --------------------------------------------------------

class cMenuSetupBase : public cMenuSetupPage {
protected:
  cSetup data;
  virtual void Store(void);
public:
  cMenuSetupBase(void);
  };

cMenuSetupBase::cMenuSetupBase(void)
{
  data = Setup;
}

void cMenuSetupBase::Store(void)
{
  Setup = data;
  cOsdProvider::UpdateOsdSize(true);
  Setup.Save();
}

// --- cMenuSetupOSD ---------------------------------------------------------

class cMenuSetupOSD : public cMenuSetupBase {
private:
  const char *useSmallFontTexts[3];
  int osdLanguageIndex;
  int numSkins;
  int originalSkinIndex;
  int skinIndex;
  const char **skinDescriptions;
  cThemes themes;
  int originalThemeIndex;
  int themeIndex;
  cStringList fontOsdNames, fontSmlNames, fontFixNames;
  int fontOsdIndex, fontSmlIndex, fontFixIndex;
  virtual void Set(void);
public:
  cMenuSetupOSD(void);
  virtual ~cMenuSetupOSD();
  virtual eOSState ProcessKey(eKeys Key);
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind() { return "MenuSetupOsd"; }
#endif /* GRAPHTFT */
  };

cMenuSetupOSD::cMenuSetupOSD(void)
{
  osdLanguageIndex = I18nCurrentLanguage();
  numSkins = Skins.Count();
  skinIndex = originalSkinIndex = Skins.Current()->Index();
  skinDescriptions = new const char*[numSkins];
  themes.Load(Skins.Current()->Name());
  themeIndex = originalThemeIndex = Skins.Current()->Theme() ? themes.GetThemeIndex(Skins.Current()->Theme()->Description()) : 0;
  cFont::GetAvailableFontNames(&fontOsdNames);
  cFont::GetAvailableFontNames(&fontSmlNames);
  cFont::GetAvailableFontNames(&fontFixNames, true);
  fontOsdNames.Insert(strdup(DefaultFontOsd));
  fontSmlNames.Insert(strdup(DefaultFontSml));
  fontFixNames.Insert(strdup(DefaultFontFix));
  fontOsdIndex = max(0, fontOsdNames.Find(Setup.FontOsd));
  fontSmlIndex = max(0, fontSmlNames.Find(Setup.FontSml));
  fontFixIndex = max(0, fontFixNames.Find(Setup.FontFix));
  Set();
}

cMenuSetupOSD::~cMenuSetupOSD()
{
  delete[] skinDescriptions;
}

void cMenuSetupOSD::Set(void)
{
  int current = Current();
  for (cSkin *Skin = Skins.First(); Skin; Skin = Skins.Next(Skin))
      skinDescriptions[Skin->Index()] = Skin->Description();
  useSmallFontTexts[0] = tr("never");
  useSmallFontTexts[1] = tr("skin dependent");
  useSmallFontTexts[2] = tr("always");
  Clear();
  SetSection(tr("OSD"));
  Add(new cMenuEditStraItem(tr("Setup.OSD$Language"),               &osdLanguageIndex, I18nNumLanguagesWithLocale(), &I18nLanguages()->At(0)));
  Add(new cMenuEditStraItem(tr("Setup.OSD$Skin"),                   &skinIndex, numSkins, skinDescriptions));
  if (themes.NumThemes())
  Add(new cMenuEditStraItem(tr("Setup.OSD$Theme"),                  &themeIndex, themes.NumThemes(), themes.Descriptions()));
#ifdef USE_WAREAGLEICON
  Add(new cMenuEditBoolItem(tr("Setup.OSD$WarEagle icons"),         &data.WarEagleIcons));
#endif /* WAREAGLEICON */
  Add(new cMenuEditPrcItem( tr("Setup.OSD$Left (%)"),               &data.OSDLeftP, 0.0, 0.5));
  Add(new cMenuEditPrcItem( tr("Setup.OSD$Top (%)"),                &data.OSDTopP, 0.0, 0.5));
  Add(new cMenuEditPrcItem( tr("Setup.OSD$Width (%)"),              &data.OSDWidthP, 0.5, 1.0));
  Add(new cMenuEditPrcItem( tr("Setup.OSD$Height (%)"),             &data.OSDHeightP, 0.5, 1.0));
  Add(new cMenuEditIntItem( tr("Setup.OSD$Message time (s)"),       &data.OSDMessageTime, 1, 60));
  Add(new cMenuEditStraItem(tr("Setup.OSD$Use small font"),         &data.UseSmallFont, 3, useSmallFontTexts));
  Add(new cMenuEditBoolItem(tr("Setup.OSD$Anti-alias"),             &data.AntiAlias));
  Add(new cMenuEditStraItem(tr("Setup.OSD$Default font"),           &fontOsdIndex, fontOsdNames.Size(), &fontOsdNames[0]));
  Add(new cMenuEditStraItem(tr("Setup.OSD$Small font"),             &fontSmlIndex, fontSmlNames.Size(), &fontSmlNames[0]));
  Add(new cMenuEditStraItem(tr("Setup.OSD$Fixed font"),             &fontFixIndex, fontFixNames.Size(), &fontFixNames[0]));
  Add(new cMenuEditPrcItem( tr("Setup.OSD$Default font size (%)"),  &data.FontOsdSizeP, 0.01, 0.1, 1));
  Add(new cMenuEditPrcItem( tr("Setup.OSD$Small font size (%)"),    &data.FontSmlSizeP, 0.01, 0.1, 1));
  Add(new cMenuEditPrcItem( tr("Setup.OSD$Fixed font size (%)"),    &data.FontFixSizeP, 0.01, 0.1, 1));
  Add(new cMenuEditBoolItem(tr("Setup.OSD$Channel info position"),  &data.ChannelInfoPos, tr("bottom"), tr("top")));
  Add(new cMenuEditIntItem( tr("Setup.OSD$Channel info time (s)"),  &data.ChannelInfoTime, 1, 60));
  Add(new cMenuEditBoolItem(tr("Setup.OSD$Info on channel switch"), &data.ShowInfoOnChSwitch));
  Add(new cMenuEditBoolItem(tr("Setup.OSD$Timeout requested channel info"), &data.TimeoutRequChInfo));
  Add(new cMenuEditBoolItem(tr("Setup.OSD$Scroll pages"),           &data.MenuScrollPage));
  Add(new cMenuEditBoolItem(tr("Setup.OSD$Scroll wraps"),           &data.MenuScrollWrap));
  Add(new cMenuEditBoolItem(tr("Setup.OSD$Menu key closes"),        &data.MenuKeyCloses));
  Add(new cMenuEditBoolItem(tr("Setup.OSD$Recording directories"),  &data.RecordingDirs));
  Add(new cMenuEditBoolItem(tr("Setup.OSD$Folders in timer menu"),  &data.FoldersInTimerMenu));
  Add(new cMenuEditBoolItem(tr("Setup.OSD$Number keys for characters"), &data.NumberKeysForChars));
#ifdef USE_LIEMIEXT
  Add(new cMenuEditBoolItem(tr("Setup.OSD$Main menu command position"), &data.MenuCmdPosition, tr("bottom"), tr("top")));
#endif /* LIEMIEXT */
#ifdef USE_VALIDINPUT
  Add(new cMenuEditBoolItem(tr("Setup.OSD$Show valid input"),       &data.ShowValidInput));
#endif /* VALIDINPUT */
  SetCurrent(Get(current));
  Display();
}

eOSState cMenuSetupOSD::ProcessKey(eKeys Key)
{
  bool ModifiedAppearance = false;

  if (Key == kOk) {
     I18nSetLocale(data.OSDLanguage);
     if (skinIndex != originalSkinIndex) {
        cSkin *Skin = Skins.Get(skinIndex);
        if (Skin) {
           Utf8Strn0Cpy(data.OSDSkin, Skin->Name(), sizeof(data.OSDSkin));
           Skins.SetCurrent(Skin->Name());
           ModifiedAppearance = true;
           }
        }
     if (themes.NumThemes() && Skins.Current()->Theme()) {
        Skins.Current()->Theme()->Load(themes.FileName(themeIndex));
        Utf8Strn0Cpy(data.OSDTheme, themes.Name(themeIndex), sizeof(data.OSDTheme));
        ModifiedAppearance |= themeIndex != originalThemeIndex;
        }
     if (!(DoubleEqual(data.OSDLeftP, Setup.OSDLeftP) && DoubleEqual(data.OSDTopP, Setup.OSDTopP) && DoubleEqual(data.OSDWidthP, Setup.OSDWidthP) && DoubleEqual(data.OSDHeightP, Setup.OSDHeightP)))
        ModifiedAppearance = true;
     if (data.UseSmallFont != Setup.UseSmallFont || data.AntiAlias != Setup.AntiAlias)
        ModifiedAppearance = true;
     Utf8Strn0Cpy(data.FontOsd, fontOsdNames[fontOsdIndex], sizeof(data.FontOsd));
     Utf8Strn0Cpy(data.FontSml, fontSmlNames[fontSmlIndex], sizeof(data.FontSml));
     Utf8Strn0Cpy(data.FontFix, fontFixNames[fontFixIndex], sizeof(data.FontFix));
     if (strcmp(data.FontOsd, Setup.FontOsd) || !DoubleEqual(data.FontOsdSizeP, Setup.FontOsdSizeP))
        ModifiedAppearance = true;
     if (strcmp(data.FontSml, Setup.FontSml) || !DoubleEqual(data.FontSmlSizeP, Setup.FontSmlSizeP))
        ModifiedAppearance = true;
     if (strcmp(data.FontFix, Setup.FontFix) || !DoubleEqual(data.FontFixSizeP, Setup.FontFixSizeP))
        ModifiedAppearance = true;
     }

  int oldSkinIndex = skinIndex;
  int oldOsdLanguageIndex = osdLanguageIndex;
  eOSState state = cMenuSetupBase::ProcessKey(Key);

  if (ModifiedAppearance) {
     cOsdProvider::UpdateOsdSize(true);
     SetDisplayMenu();
     }

  if (osdLanguageIndex != oldOsdLanguageIndex || skinIndex != oldSkinIndex) {
     strn0cpy(data.OSDLanguage, I18nLocale(osdLanguageIndex), sizeof(data.OSDLanguage));
     int OriginalOSDLanguage = I18nCurrentLanguage();
     I18nSetLanguage(osdLanguageIndex);

     cSkin *Skin = Skins.Get(skinIndex);
     if (Skin) {
        char *d = themes.NumThemes() ? strdup(themes.Descriptions()[themeIndex]) : NULL;
        themes.Load(Skin->Name());
        if (skinIndex != oldSkinIndex)
           themeIndex = d ? themes.GetThemeIndex(d) : 0;
        free(d);
        }

     Set();
     I18nSetLanguage(OriginalOSDLanguage);
     }
  return state;
}

// --- cMenuSetupEPG ---------------------------------------------------------

class cMenuSetupEPG : public cMenuSetupBase {
private:
#ifdef USE_NOEPG
  const char *noEPGModes[2];
#endif /* NOEPG */
  int originalNumLanguages;
  int numLanguages;
  void Setup(void);
public:
  cMenuSetupEPG(void);
  virtual eOSState ProcessKey(eKeys Key);
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind() { return "MenuSetupEpg"; }
#endif /* GRAPHTFT */
  };

cMenuSetupEPG::cMenuSetupEPG(void)
{
  for (numLanguages = 0; numLanguages < I18nLanguages()->Size() && data.EPGLanguages[numLanguages] >= 0; numLanguages++)
      ;
  originalNumLanguages = numLanguages;
  SetSection(tr("EPG"));
  SetHelp(tr("Button$Scan"));
  Setup();
}

void cMenuSetupEPG::Setup(void)
{
  int current = Current();

#ifdef USE_NOEPG
  noEPGModes[0] = tr("blacklist");
  noEPGModes[1] = tr("whitelist");
#endif /* NOEPG */

  Clear();

  Add(new cMenuEditIntItem( tr("Setup.EPG$EPG scan timeout (h)"),      &data.EPGScanTimeout));
  Add(new cMenuEditIntItem( tr("Setup.EPG$EPG bugfix level"),          &data.EPGBugfixLevel, 0, MAXEPGBUGFIXLEVEL));
  Add(new cMenuEditIntItem( tr("Setup.EPG$EPG linger time (min)"),     &data.EPGLinger, 0));
#ifdef USE_LIEMIEXT
  Add(new cMenuEditBoolItem(tr("Setup.EPG$Show progress bar"),         &data.ShowProgressBar));
#endif /* LIEMIEXT */
  Add(new cMenuEditBoolItem(tr("Setup.EPG$Set system time"),           &data.SetSystemTime));
  if (data.SetSystemTime)
     Add(new cMenuEditTranItem(tr("Setup.EPG$Use time from transponder"), &data.TimeTransponder, &data.TimeSource));
  // TRANSLATORS: note the plural!
  Add(new cMenuEditIntItem( tr("Setup.EPG$Preferred languages"),       &numLanguages, 0, I18nLanguages()->Size()));
  for (int i = 0; i < numLanguages; i++)
      // TRANSLATORS: note the singular!
      Add(new cMenuEditStraItem(tr("Setup.EPG$Preferred language"),    &data.EPGLanguages[i], I18nLanguages()->Size(), &I18nLanguages()->At(0)));
#ifdef USE_DDEPGENTRY
  Add(new cMenuEditIntItem(tr("Setup.EPG$Period for double EPG search(min)"), &data.DoubleEpgTimeDelta));
  Add(new cMenuEditBoolItem(tr("Setup.EPG$extern double Epg entry"),   &data.DoubleEpgAction, "adjust", "delete"));
  Add(new cMenuEditBoolItem(tr("Setup.EPG$Mix intern and extern EPG"), &data.MixEpgAction));
  Add(new cMenuEditBoolItem(tr("Setup.EPG$Disable running VPS event"), &data.DisableVPS));
#endif /* DDEPGENTRY */
#ifdef USE_NOEPG
  Add(new cMenuEditStraItem(tr("Setup.EPG$Mode of noEPG-Patch"),       &data.noEPGMode, 2, noEPGModes));
#endif /* NOEPG */

  SetCurrent(Get(current));
  Display();
}

eOSState cMenuSetupEPG::ProcessKey(eKeys Key)
{
  if (Key == kOk) {
     bool Modified = numLanguages != originalNumLanguages;
     if (!Modified) {
        for (int i = 0; i < numLanguages; i++) {
            if (data.EPGLanguages[i] != ::Setup.EPGLanguages[i]) {
               Modified = true;
               break;
               }
            }
        }
     if (Modified)
        cSchedules::ResetVersions();
     }

  int oldnumLanguages = numLanguages;
  int oldSetSystemTime = data.SetSystemTime;

  eOSState state = cMenuSetupBase::ProcessKey(Key);
  if (Key != kNone) {
     if (numLanguages != oldnumLanguages || data.SetSystemTime != oldSetSystemTime) {
        for (int i = oldnumLanguages; i < numLanguages; i++) {
            data.EPGLanguages[i] = 0;
            for (int l = 0; l < I18nLanguages()->Size(); l++) {
                int k;
                for (k = 0; k < oldnumLanguages; k++) {
                    if (data.EPGLanguages[k] == l)
                       break;
                    }
                if (k >= oldnumLanguages) {
                   data.EPGLanguages[i] = l;
                   break;
                   }
                }
            }
        data.EPGLanguages[numLanguages] = -1;
        Setup();
        }
     if (Key == kRed) {
        EITScanner.ForceScan();
        return osEnd;
        }
     }
  return state;
}

// --- cMenuSetupDVB ---------------------------------------------------------

class cMenuSetupDVB : public cMenuSetupBase {
private:
  int originalNumAudioLanguages;
  int numAudioLanguages;
  int originalNumSubtitleLanguages;
  int numSubtitleLanguages;
  void Setup(void);
  const char *videoDisplayFormatTexts[3];
  const char *updateChannelsTexts[6];
public:
  cMenuSetupDVB(void);
  virtual eOSState ProcessKey(eKeys Key);
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind() { return "MenuSetupDvb"; }
#endif /* GRAPHTFT */
  };

cMenuSetupDVB::cMenuSetupDVB(void)
{
  for (numAudioLanguages = 0; numAudioLanguages < I18nLanguages()->Size() && data.AudioLanguages[numAudioLanguages] >= 0; numAudioLanguages++)
      ;
  for (numSubtitleLanguages = 0; numSubtitleLanguages < I18nLanguages()->Size() && data.SubtitleLanguages[numSubtitleLanguages] >= 0; numSubtitleLanguages++)
      ;
  originalNumAudioLanguages = numAudioLanguages;
  originalNumSubtitleLanguages = numSubtitleLanguages;
  videoDisplayFormatTexts[0] = tr("pan&scan");
  videoDisplayFormatTexts[1] = tr("letterbox");
  videoDisplayFormatTexts[2] = tr("center cut out");
  updateChannelsTexts[0] = tr("no");
  updateChannelsTexts[1] = tr("names only");
  updateChannelsTexts[2] = tr("PIDs only");
  updateChannelsTexts[3] = tr("names and PIDs");
  updateChannelsTexts[4] = tr("add new channels");
  updateChannelsTexts[5] = tr("add new transponders");

  SetSection(tr("DVB"));
  SetHelp(NULL, tr("Button$Audio"), tr("Button$Subtitles"), NULL);
  Setup();
}

void cMenuSetupDVB::Setup(void)
{
  int current = Current();

  Clear();

#ifdef USE_CHANNELPROVIDE
  Add(new cMenuEditBoolItem(tr("Setup.DVB$Use DVB receivers"),      &data.LocalChannelProvide));
#endif /* CHANNELPROVIDE */
  Add(new cMenuEditIntItem( tr("Setup.DVB$Primary DVB interface"), &data.PrimaryDVB, 1, cDevice::NumDevices()));
  Add(new cMenuEditBoolItem(tr("Setup.DVB$Video format"),          &data.VideoFormat, "4:3", "16:9"));
  if (data.VideoFormat == 0)
     Add(new cMenuEditStraItem(tr("Setup.DVB$Video display format"), &data.VideoDisplayFormat, 3, videoDisplayFormatTexts));
  Add(new cMenuEditBoolItem(tr("Setup.DVB$Use Dolby Digital"),     &data.UseDolbyDigital));
  Add(new cMenuEditStraItem(tr("Setup.DVB$Update channels"),       &data.UpdateChannels, 6, updateChannelsTexts));
#ifdef USE_CHANNELBIND
  Add(new cMenuEditBoolItem(tr("Setup.DVB$channel binding by Rid"),&data.ChannelBindingByRid));
#endif /* CHANNELBIND */
  Add(new cMenuEditIntItem( tr("Setup.DVB$Audio languages"),       &numAudioLanguages, 0, I18nLanguages()->Size()));
  for (int i = 0; i < numAudioLanguages; i++)
      Add(new cMenuEditStraItem(tr("Setup.DVB$Audio language"),    &data.AudioLanguages[i], I18nLanguages()->Size(), &I18nLanguages()->At(0)));
  Add(new cMenuEditBoolItem(tr("Setup.DVB$Display subtitles"),     &data.DisplaySubtitles));
  if (data.DisplaySubtitles) {
     Add(new cMenuEditIntItem( tr("Setup.DVB$Subtitle languages"),    &numSubtitleLanguages, 0, I18nLanguages()->Size()));
     for (int i = 0; i < numSubtitleLanguages; i++)
         Add(new cMenuEditStraItem(tr("Setup.DVB$Subtitle language"), &data.SubtitleLanguages[i], I18nLanguages()->Size(), &I18nLanguages()->At(0)));
     Add(new cMenuEditIntItem( tr("Setup.DVB$Subtitle offset"),                  &data.SubtitleOffset,      -100, 100));
     Add(new cMenuEditIntItem( tr("Setup.DVB$Subtitle foreground transparency"), &data.SubtitleFgTransparency, 0, 9));
     Add(new cMenuEditIntItem( tr("Setup.DVB$Subtitle background transparency"), &data.SubtitleBgTransparency, 0, 10));
     }
#ifdef USE_TTXTSUBS
  Add(new cMenuEditBoolItem(tr("Setup.DVB$Enable teletext support"), &data.SupportTeletext));
#endif /* TTXTSUBS */

  SetCurrent(Get(current));
  Display();
}

eOSState cMenuSetupDVB::ProcessKey(eKeys Key)
{
  int oldPrimaryDVB = ::Setup.PrimaryDVB;
  int oldVideoDisplayFormat = ::Setup.VideoDisplayFormat;
  bool oldVideoFormat = ::Setup.VideoFormat;
  bool newVideoFormat = data.VideoFormat;
  bool oldDisplaySubtitles = ::Setup.DisplaySubtitles;
  bool newDisplaySubtitles = data.DisplaySubtitles;
  int oldnumAudioLanguages = numAudioLanguages;
  int oldnumSubtitleLanguages = numSubtitleLanguages;
  eOSState state = cMenuSetupBase::ProcessKey(Key);

  if (Key != kNone) {
     switch (Key) {
       case kGreen:  cRemote::Put(kAudio, true);
                     state = osEnd;
                     break;
       case kYellow: cRemote::Put(kSubtitles, true);
                     state = osEnd;
                     break;
       default: {
            bool DoSetup = data.VideoFormat != newVideoFormat;
            DoSetup |= data.DisplaySubtitles != newDisplaySubtitles;
            if (numAudioLanguages != oldnumAudioLanguages) {
               for (int i = oldnumAudioLanguages; i < numAudioLanguages; i++) {
                   data.AudioLanguages[i] = 0;
                   for (int l = 0; l < I18nLanguages()->Size(); l++) {
                       int k;
                       for (k = 0; k < oldnumAudioLanguages; k++) {
                           if (data.AudioLanguages[k] == l)
                              break;
                           }
                       if (k >= oldnumAudioLanguages) {
                          data.AudioLanguages[i] = l;
                          break;
                          }
                       }
                   }
               data.AudioLanguages[numAudioLanguages] = -1;
               DoSetup = true;
               }
            if (numSubtitleLanguages != oldnumSubtitleLanguages) {
               for (int i = oldnumSubtitleLanguages; i < numSubtitleLanguages; i++) {
                   data.SubtitleLanguages[i] = 0;
                   for (int l = 0; l < I18nLanguages()->Size(); l++) {
                       int k;
                       for (k = 0; k < oldnumSubtitleLanguages; k++) {
                           if (data.SubtitleLanguages[k] == l)
                              break;
                           }
                       if (k >= oldnumSubtitleLanguages) {
                          data.SubtitleLanguages[i] = l;
                          break;
                          }
                       }
                   }
               data.SubtitleLanguages[numSubtitleLanguages] = -1;
               DoSetup = true;
               }
            if (DoSetup)
               Setup();
            }
       }
     }
  if (state == osBack && Key == kOk) {
     if (::Setup.PrimaryDVB != oldPrimaryDVB)
        state = osSwitchDvb;
     if (::Setup.VideoDisplayFormat != oldVideoDisplayFormat)
        cDevice::PrimaryDevice()->SetVideoDisplayFormat(eVideoDisplayFormat(::Setup.VideoDisplayFormat));
     if (::Setup.VideoFormat != oldVideoFormat)
        cDevice::PrimaryDevice()->SetVideoFormat(::Setup.VideoFormat);
     if (::Setup.DisplaySubtitles != oldDisplaySubtitles)
        cDevice::PrimaryDevice()->EnsureSubtitleTrack();
     cDvbSubtitleConverter::SetupChanged();
     }
  return state;
}

// --- cMenuSetupLNB ---------------------------------------------------------

class cMenuSetupLNB : public cMenuSetupBase {
private:
  void Setup(void);
public:
  cMenuSetupLNB(void);
  virtual eOSState ProcessKey(eKeys Key);
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind() { return "MenuSetupLnb"; }
#endif /* GRAPHTFT */
  };

cMenuSetupLNB::cMenuSetupLNB(void)
{
  SetSection(tr("LNB"));
  Setup();
}

void cMenuSetupLNB::Setup(void)
{
  int current = Current();

  Clear();

#ifdef USE_LNBSHARE
  int numSatDevices = 0;
  for (int i = 0; i < cDevice::NumDevices(); i++) {
     	if (cDevice::GetDevice(i)->ProvidesSource(cSource::stSat)) numSatDevices++;
  }
  if (numSatDevices > 1) {
  	 char tmp[40];
     for (int i = 1; i <= cDevice::NumDevices(); i++) {
     	if (cDevice::GetDevice(i - 1)->ProvidesSource(cSource::stSat)) {
        	snprintf( tmp, 40, tr("Setup.LNB$DVB device %d uses LNB No."), i);
        	Add(new cMenuEditIntItem( tmp, &data.CardUsesLnbNr[i - 1], 1, numSatDevices ));
        }
     }
   }
  Add(new cMenuEditBoolItem(tr("Setup.LNB$Log LNB usage"), &data.VerboseLNBlog));
#endif /* LNBSHARE */

  Add(new cMenuEditBoolItem(tr("Setup.LNB$Use DiSEqC"),               &data.DiSEqC));
  if (!data.DiSEqC) {
     Add(new cMenuEditIntItem( tr("Setup.LNB$SLOF (MHz)"),               &data.LnbSLOF));
     Add(new cMenuEditIntItem( tr("Setup.LNB$Low LNB frequency (MHz)"),  &data.LnbFrequLo));
     Add(new cMenuEditIntItem( tr("Setup.LNB$High LNB frequency (MHz)"), &data.LnbFrequHi));
     }

  SetCurrent(Get(current));
  Display();
}

eOSState cMenuSetupLNB::ProcessKey(eKeys Key)
{
  int oldDiSEqC = data.DiSEqC;
  eOSState state = cMenuSetupBase::ProcessKey(Key);

#ifdef USE_LNBSHARE
  if (Key == kOk) cDevice::SetLnbNr();
#endif /* LNBSHARE */

  if (Key != kNone && data.DiSEqC != oldDiSEqC)
     Setup();
  return state;
}

// --- cMenuSetupCAM ---------------------------------------------------------

class cMenuSetupCAMItem : public cOsdItem {
private:
  cCamSlot *camSlot;
public:
  cMenuSetupCAMItem(cCamSlot *CamSlot);
  cCamSlot *CamSlot(void) { return camSlot; }
  bool Changed(void);
  };

cMenuSetupCAMItem::cMenuSetupCAMItem(cCamSlot *CamSlot)
{
  camSlot = CamSlot;
  SetText("");
  Changed();
}

bool cMenuSetupCAMItem::Changed(void)
{
  char buffer[32];
  const char *CamName = camSlot->GetCamName();
  if (!CamName) {
     switch (camSlot->ModuleStatus()) {
       case msReset:   CamName = tr("CAM reset"); break;
       case msPresent: CamName = tr("CAM present"); break;
       case msReady:   CamName = tr("CAM ready"); break;
       default:        CamName = "-"; break;
       }
     }
  snprintf(buffer, sizeof(buffer), " %d %s", camSlot->SlotNumber(), CamName);
  if (strcmp(buffer, Text()) != 0) {
     SetText(buffer);
     return true;
     }
  return false;
}

class cMenuSetupCAM : public cMenuSetupBase {
private:
  eOSState Menu(void);
  eOSState Reset(void);
public:
  cMenuSetupCAM(void);
  virtual eOSState ProcessKey(eKeys Key);
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind() { return "MenuSetupCam"; }
#endif /* GRAPHTFT */
  };

cMenuSetupCAM::cMenuSetupCAM(void)
{
  SetSection(tr("CAM"));
  SetCols(15);
  SetHasHotkeys();
  for (cCamSlot *CamSlot = CamSlots.First(); CamSlot; CamSlot = CamSlots.Next(CamSlot))
      Add(new cMenuSetupCAMItem(CamSlot));
  SetHelp(tr("Button$Menu"), tr("Button$Reset"));
}

eOSState cMenuSetupCAM::Menu(void)
{
  cMenuSetupCAMItem *item = (cMenuSetupCAMItem *)Get(Current());
  if (item) {
     if (item->CamSlot()->EnterMenu()) {
        Skins.Message(mtStatus, tr("Opening CAM menu..."));
        time_t t0 = time(NULL);
        time_t t1 = t0;
        while (time(NULL) - t0 <= MAXWAITFORCAMMENU) {
              if (item->CamSlot()->HasUserIO())
                 break;
              if (time(NULL) - t1 >= CAMMENURETYTIMEOUT) {
                 dsyslog("CAM %d: retrying to enter CAM menu...", item->CamSlot()->SlotNumber());
                 item->CamSlot()->EnterMenu();
                 t1 = time(NULL);
                 }
              cCondWait::SleepMs(100);
              }
        Skins.Message(mtStatus, NULL);
        if (item->CamSlot()->HasUserIO())
           return AddSubMenu(new cMenuCam(item->CamSlot()));
        }
     Skins.Message(mtError, tr("Can't open CAM menu!"));
     }
  return osContinue;
}

eOSState cMenuSetupCAM::Reset(void)
{
  cMenuSetupCAMItem *item = (cMenuSetupCAMItem *)Get(Current());
  if (item) {
     if (!item->CamSlot()->Device() || Interface->Confirm(tr("CAM is in use - really reset?"))) {
        if (!item->CamSlot()->Reset())
           Skins.Message(mtError, tr("Can't reset CAM!"));
        }
     }
  return osContinue;
}

eOSState cMenuSetupCAM::ProcessKey(eKeys Key)
{
  eOSState state = HasSubMenu() ? cMenuSetupBase::ProcessKey(Key) : cOsdMenu::ProcessKey(Key);

  if (!HasSubMenu()) {
     switch (Key) {
       case kOk:
       case kRed:    return Menu();
       case kGreen:  state = Reset(); break;
       default: break;
       }
     for (cMenuSetupCAMItem *ci = (cMenuSetupCAMItem *)First(); ci; ci = (cMenuSetupCAMItem *)ci->Next()) {
         if (ci->Changed())
            DisplayItem(ci);
         }
     }
  return state;
}

// --- cMenuSetupRecord ------------------------------------------------------

class cMenuSetupRecord : public cMenuSetupBase {
private:
  const char *pauseKeyHandlingTexts[3];
  const char *delTimeshiftRecTexts[3];
#ifdef USE_DVLVIDPREFER
  void Set(void);
  int tmpNVidPrefer,
      tmpUseVidPrefer;
#endif /* DVLVIDPREFER */
public:
  cMenuSetupRecord(void);
#ifdef USE_DVLVIDPREFER
  eOSState ProcessKey(eKeys key);
#endif /* DVLVIDPREFER */
  };

cMenuSetupRecord::cMenuSetupRecord(void)
{
#ifdef USE_DVLVIDPREFER
  Set();
}

eOSState cMenuSetupRecord::ProcessKey(eKeys key)
{
  eOSState s = cMenuSetupBase::ProcessKey(key);;

  if (key != kNone) {
    if (tmpNVidPrefer != data.nVidPrefer || tmpUseVidPrefer != data.UseVidPrefer) {
      int cur = Current();

      tmpNVidPrefer = data.nVidPrefer;
      tmpUseVidPrefer = data.UseVidPrefer;

      Clear();
      Set();
      SetCurrent(Get(cur));
      Display();
      cMenuSetupBase::ProcessKey(kNone);
      return osContinue;
      }
   }
   return s;
}

void cMenuSetupRecord::Set(void)
{
#endif /* DVLVIDPREFER */
  pauseKeyHandlingTexts[0] = tr("do not pause live video");
  pauseKeyHandlingTexts[1] = tr("confirm pause live video");
  pauseKeyHandlingTexts[2] = tr("pause live video");
  delTimeshiftRecTexts[0] = tr("no");
  delTimeshiftRecTexts[1] = tr("confirm");
  delTimeshiftRecTexts[2] = tr("yes");
  SetSection(tr("Recording"));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Margin at start (min)"),     &data.MarginStart));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Margin at stop (min)"),      &data.MarginStop));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Primary limit"),             &data.PrimaryLimit, 0, MAXPRIORITY));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Default priority"),          &data.DefaultPriority, 0, MAXPRIORITY));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Default Lifetime (d)"),      &data.DefaultLifetime, 0, MAXLIFETIME));
  Add(new cMenuEditStraItem(tr("Setup.Recording$Pause key handling"),        &data.PauseKeyHandling, 3, pauseKeyHandlingTexts));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Pause priority"),            &data.PausePriority, 0, MAXPRIORITY));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Pause lifetime (d)"),        &data.PauseLifetime, 0, MAXLIFETIME));
#ifdef USE_DVLVIDPREFER
  tmpNVidPrefer = data.nVidPrefer;
  tmpUseVidPrefer = data.UseVidPrefer;

  Add(new cMenuEditBoolItem(tr("Setup.Recording$Video directory policy"),    &data.UseVidPrefer));
  if (data.UseVidPrefer != 0) {
     char tmp[ 64 ];
     Add(new cMenuEditIntItem(tr("Setup.Recording$Number of video directories"), &data.nVidPrefer, 1, DVLVIDPREFER_MAX));
     for (int zz = 0; zz < data.nVidPrefer; zz++) {
         sprintf(tmp, tr("Setup.Recording$Video %d priority"), zz);
         Add(new cMenuEditIntItem(tmp, &data.VidPreferPrio[ zz ], 0, 99));
         sprintf(tmp, tr("Setup.Recording$Video %d min. free MB"), zz);
         Add(new cMenuEditIntItem(tmp, &data.VidPreferSize[ zz ], -1, 99999));
         }
     }
#endif /* DVLVIDPREFER */
  Add(new cMenuEditBoolItem(tr("Setup.Recording$Use episode name"),          &data.UseSubtitle));
  Add(new cMenuEditBoolItem(tr("Setup.Recording$Use VPS"),                   &data.UseVps));
  Add(new cMenuEditIntItem( tr("Setup.Recording$VPS margin (s)"),            &data.VpsMargin, 0));
  Add(new cMenuEditBoolItem(tr("Setup.Recording$Mark instant recording"),    &data.MarkInstantRecord));
  Add(new cMenuEditStrItem( tr("Setup.Recording$Name instant recording"),     data.NameInstantRecord, sizeof(data.NameInstantRecord)));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Instant rec. time (min)"),   &data.InstantRecordTime, 1, MAXINSTANTRECTIME));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Max. video file size (MB)"), &data.MaxVideoFileSize, MINVIDEOFILESIZE, MAXVIDEOFILESIZETS));
#ifdef USE_HARDLINKCUTTER
  Add(new cMenuEditIntItem( tr("Setup.Recording$Max. recording size (GB)"),  &data.MaxRecordingSize, MINRECORDINGSIZE, MAXRECORDINGSIZE));
  Add(new cMenuEditBoolItem(tr("Setup.Recording$Hard Link Cutter"),          &data.HardLinkCutter));
#endif /* HARDLINKCUTTER */
  Add(new cMenuEditBoolItem(tr("Setup.Recording$Split edited files"),        &data.SplitEditedFiles));
#ifdef USE_LIEMIEXT
  Add(new cMenuEditBoolItem(tr("Setup.Recording$Show date"),                 &data.ShowRecDate));
  Add(new cMenuEditBoolItem(tr("Setup.Recording$Show time"),                 &data.ShowRecTime));
  Add(new cMenuEditBoolItem(tr("Setup.Recording$Show length"),               &data.ShowRecLength));
#endif /* LIEMIEXT */
  Add(new cMenuEditStraItem(tr("Setup.Recording$Delete timeshift recording"),&data.DelTimeshiftRec, 3, delTimeshiftRecTexts));
}

// --- cMenuSetupReplay ------------------------------------------------------

class cMenuSetupReplay : public cMenuSetupBase {
protected:
  virtual void Store(void);
public:
  cMenuSetupReplay(void);
  };

cMenuSetupReplay::cMenuSetupReplay(void)
{
  SetSection(tr("Replay"));
  Add(new cMenuEditBoolItem(tr("Setup.Replay$Multi speed mode"), &data.MultiSpeedMode));
  Add(new cMenuEditBoolItem(tr("Setup.Replay$Show replay mode"), &data.ShowReplayMode));
  Add(new cMenuEditIntItem(tr("Setup.Replay$Resume ID"), &data.ResumeID, 0, 99));
#ifdef USE_JUMPINGSECONDS
  Add(new cMenuEditIntItem( tr("Setup.Recording$Jump Seconds"),              &data.JumpSeconds));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Jump Seconds Slow"),         &data.JumpSecondsSlow));
  Add(new cMenuEditIntItem( tr("Setup.Recording$Jump Seconds (Repeat)"),     &data.JumpSecondsRepeat));
#endif /* JUMPINGSECONDS */
#if REELVDR
  Add(new cMenuEditIntItem(tr("Setup.Replay$Jump width for green/yellow (min)"), &data.JumpWidth, 0, 10, tr("intelligent")));
#endif
#ifdef USE_JUMPPLAY
  Add(new cMenuEditBoolItem(tr("Setup.Replay$Jump&Play"), &data.JumpPlay));
  Add(new cMenuEditBoolItem(tr("Setup.Replay$Play&Jump"), &data.PlayJump));
  Add(new cMenuEditBoolItem(tr("Setup.Replay$Pause at last mark"), &data.PauseLastMark));
  Add(new cMenuEditBoolItem(tr("Setup.Replay$Reload marks"), &data.ReloadMarks));
#endif /* JUMPPLAY */
}

void cMenuSetupReplay::Store(void)
{
  if (Setup.ResumeID != data.ResumeID)
     Recordings.ResetResume();
  cMenuSetupBase::Store();
}

// --- cMenuSetupMisc --------------------------------------------------------

class cMenuSetupMisc : public cMenuSetupBase {
#ifdef USE_VOLCTRL
private:
  const char *lrChannelGroupsTexts[3];
  const char *lrForwardRewindTexts[3];
  void Setup(void);
#endif /* VOLCTRL */
public:
  cMenuSetupMisc(void);
#ifdef USE_VOLCTRL
  virtual eOSState ProcessKey(eKeys Key);
#endif /* VOLCTRL */
  };

cMenuSetupMisc::cMenuSetupMisc(void)
{
#ifdef USE_VOLCTRL
  lrChannelGroupsTexts[0] = tr("no");
  lrChannelGroupsTexts[1] = tr("Setup.Miscellaneous$only in channelinfo");
  lrChannelGroupsTexts[2] = tr("yes");
  lrForwardRewindTexts[0] = tr("no");
  lrForwardRewindTexts[1] = tr("Setup.Miscellaneous$only in progress display");
  lrForwardRewindTexts[2] = tr("yes");
#endif /* VOLCTRL */
  SetSection(tr("Miscellaneous"));
#ifdef USE_VOLCTRL
  Setup();
}

eOSState cMenuSetupMisc::ProcessKey(eKeys Key)
{
  int newLRVolumeControl = data.LRVolumeControl;
  eOSState state = cMenuSetupBase::ProcessKey(Key);
  if (Key != kNone && data.LRVolumeControl != newLRVolumeControl)
     Setup();
  return state;
}

void cMenuSetupMisc::Setup(void)
{
  int current = Current();
  Clear();
#endif /* VOLCTRL */
  Add(new cMenuEditIntItem( tr("Setup.Miscellaneous$Min. event timeout (min)"),   &data.MinEventTimeout));
  Add(new cMenuEditIntItem( tr("Setup.Miscellaneous$Min. user inactivity (min)"), &data.MinUserInactivity));
  Add(new cMenuEditIntItem( tr("Setup.Miscellaneous$SVDRP timeout (s)"),          &data.SVDRPTimeout));
  Add(new cMenuEditIntItem( tr("Setup.Miscellaneous$Zap timeout (s)"),            &data.ZapTimeout));
  Add(new cMenuEditIntItem( tr("Setup.Miscellaneous$Channel entry timeout (ms)"), &data.ChannelEntryTimeout, 0));
  Add(new cMenuEditChanItem(tr("Setup.Miscellaneous$Initial channel"),            &data.InitialChannel, tr("Setup.Miscellaneous$as before")));
  Add(new cMenuEditIntItem( tr("Setup.Miscellaneous$Initial volume"),             &data.InitialVolume, -1, 255, tr("Setup.Miscellaneous$as before")));
  Add(new cMenuEditBoolItem(tr("Setup.Miscellaneous$Channels wrap"),              &data.ChannelsWrap));
  Add(new cMenuEditBoolItem(tr("Setup.Miscellaneous$Emergency exit"),             &data.EmergencyExit));
#ifdef USE_LIRCSETTINGS
  Add(new cMenuEditIntItem( tr("Setup.Miscellaneous$Lirc repeat delay"),          &data.LircRepeatDelay, 0, 1000));
  Add(new cMenuEditIntItem( tr("Setup.Miscellaneous$Lirc repeat freq"),           &data.LircRepeatFreq, 0, 1000));
  Add(new cMenuEditIntItem( tr("Setup.Miscellaneous$Lirc repeat timeout"),        &data.LircRepeatTimeout, 0, 5000));
#endif /* LIRCSETTINGS */
#ifdef USE_VOLCTRL
  Add(new cMenuEditBoolItem(tr("Setup.Miscellaneous$Volume ctrl with left/right"),     &data.LRVolumeControl));
  if (data.LRVolumeControl) {
     Add(new cMenuEditStraItem(tr("Setup.Miscellaneous$Channelgroups with left/right"),   &data.LRChannelGroups, 3, lrChannelGroupsTexts));
     Add(new cMenuEditStraItem(tr("Setup.Miscellaneous$Search fwd/back with left/right"), &data.LRForwardRewind, 3, lrForwardRewindTexts));
     }
  SetCurrent(Get(current));
  Display();
#endif /* VOLCTRL */
}

// --- cMenuSetupPluginItem --------------------------------------------------

#ifndef REELVDR
class cMenuSetupPluginItem : public cOsdItem {
private:
  int pluginIndex;
public:
  cMenuSetupPluginItem(const char *Name, int Index);
  int PluginIndex(void) { return pluginIndex; }
  };
#endif

cMenuSetupPluginItem::cMenuSetupPluginItem(const char *Name, int Index)
:cOsdItem(Name)
{
  pluginIndex = Index;
}

// --- cMenuSetupPlugins -----------------------------------------------------

class cMenuSetupPlugins : public cMenuSetupBase {
public:
  cMenuSetupPlugins(void);
  virtual eOSState ProcessKey(eKeys Key);
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind() { return "MenuSetupPlugins"; }
#endif /* GRAPHTFT */
  };

cMenuSetupPlugins::cMenuSetupPlugins(void)
{
  SetSection(tr("Plugins"));
  SetHasHotkeys();
  for (int i = 0; ; i++) {
      cPlugin *p = cPluginManager::GetPlugin(i);
      if (p)
         Add(new cMenuSetupPluginItem(hk(cString::sprintf("%s (%s) - %s", p->Name(), p->Version(), p->Description())), i));
      else
         break;
      }
}

eOSState cMenuSetupPlugins::ProcessKey(eKeys Key)
{
  eOSState state = HasSubMenu() ? cMenuSetupBase::ProcessKey(Key) : cOsdMenu::ProcessKey(Key);

  if (Key == kOk) {
     if (state == osUnknown) {
        cMenuSetupPluginItem *item = (cMenuSetupPluginItem *)Get(Current());
        if (item) {
           cPlugin *p = cPluginManager::GetPlugin(item->PluginIndex());
           if (p) {
              cMenuSetupPage *menu = p->SetupMenu();
              if (menu) {
                 menu->SetPlugin(p);
                 return AddSubMenu(menu);
                 }
              Skins.Message(mtInfo, tr("This plugin has no setup parameters!"));
              }
           }
        }
     else if (state == osContinue)
        Store();
     }
  return state;
}

// --- cMenuSetup ------------------------------------------------------------

class cMenuSetup : public cOsdMenu {
private:
  virtual void Set(void);
  eOSState Restart(void);
public:
  cMenuSetup(void);
  virtual eOSState ProcessKey(eKeys Key);
#ifdef USE_GRAPHTFT
  virtual const char* MenuKind() { return "MenuSetup"; }
#endif /* GRAPHTFT */
  };

cMenuSetup::cMenuSetup(void)
:cOsdMenu("")
{
  Set();
}

void cMenuSetup::Set(void)
{
  Clear();
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%s - VDR %s", tr("Setup"), VDRVERSION);
  SetTitle(buffer);
  SetHasHotkeys();
  Add(new cOsdItem(hk(tr("OSD")),           osUser1));
  Add(new cOsdItem(hk(tr("EPG")),           osUser2));
  Add(new cOsdItem(hk(tr("DVB")),           osUser3));
  Add(new cOsdItem(hk(tr("LNB")),           osUser4));
  Add(new cOsdItem(hk(tr("CAM")),           osUser5));
  Add(new cOsdItem(hk(tr("Recording")),     osUser6));
  Add(new cOsdItem(hk(tr("Replay")),        osUser7));
  Add(new cOsdItem(hk(tr("Miscellaneous")), osUser8));
  if (cPluginManager::HasPlugins())
  Add(new cOsdItem(hk(tr("Plugins")),       osUser9));
  Add(new cOsdItem(hk(tr("Restart")),       osUser10));
}

eOSState cMenuSetup::Restart(void)
{
  if (Interface->Confirm(tr("Really restart?")) && ShutdownHandler.ConfirmRestart(true)) {
     ShutdownHandler.Exit(1);
     return osEnd;
     }
  return osContinue;
}

eOSState cMenuSetup::ProcessKey(eKeys Key)
{
  int osdLanguage = I18nCurrentLanguage();
  eOSState state = cOsdMenu::ProcessKey(Key);

  switch (state) {
    case osUser1: return AddSubMenu(new cMenuSetupOSD);
    case osUser2: return AddSubMenu(new cMenuSetupEPG);
    case osUser3: return AddSubMenu(new cMenuSetupDVB);
    case osUser4: return AddSubMenu(new cMenuSetupLNB);
    case osUser5: return AddSubMenu(new cMenuSetupCAM);
    case osUser6: return AddSubMenu(new cMenuSetupRecord);
    case osUser7: return AddSubMenu(new cMenuSetupReplay);
    case osUser8: return AddSubMenu(new cMenuSetupMisc);
    case osUser9: return AddSubMenu(new cMenuSetupPlugins);
    case osUser10: return Restart();
    default: ;
    }
  if (I18nCurrentLanguage() != osdLanguage) {
     Set();
     if (!HasSubMenu())
        Display();
     }
  return state;
}

// --- cMenuPluginItem -------------------------------------------------------

class cMenuPluginItem : public cOsdItem {
private:
  int pluginIndex;
public:
  cMenuPluginItem(const char *Name, int Index);
  int PluginIndex(void) { return pluginIndex; }
  };

cMenuPluginItem::cMenuPluginItem(const char *Name, int Index)
:cOsdItem(Name, osPlugin)
{
  pluginIndex = Index;
}

// --- cMenuMain -------------------------------------------------------------

// TRANSLATORS: note the leading and trailing blanks!
#define STOP_RECORDING trNOOP(" Stop recording ")

cOsdObject *cMenuMain::pluginOsdObject = NULL;

cMenuMain::cMenuMain(eOSState State)
:cOsdMenu("")
{
#ifdef USE_SETUP
  // Load Menu Configuration
  cString menuXML = cString::sprintf("%s/setup/vdr-menu.%s.xml", cPlugin::ConfigDirectory(), Setup.OSDLanguage);
  if (access(menuXML, 04) == -1)
     menuXML = cString::sprintf("%s/setup/vdr-menu.xml", cPlugin::ConfigDirectory());
  subMenu.LoadXml(menuXML);
  nrDynamicMenuEntries = 0;
#endif /* SETUP */

  replaying = false;
  stopReplayItem = NULL;
  cancelEditingItem = NULL;
  stopRecordingItem = NULL;
  recordControlsState = 0;

#ifdef USE_MENUORG
  MenuOrgPatch::EnterRootMenu();
#endif /* MENUORG */
  Set();

  // Initial submenus:
#ifdef USE_MAINMENUHOOKS
  cOsdMenu *menu = NULL;
#endif /* MAINMENUHOOKS */

  switch (State) {
#ifdef USE_MAINMENUHOOKS
    case osSchedule:
        if (!cPluginManager::CallFirstService("MainMenuHooksPatch-v1.0::osSchedule", &menu))
            menu = new cMenuSchedule;
        break;
    case osChannels:
        if (!cPluginManager::CallFirstService("MainMenuHooksPatch-v1.0::osChannels", &menu))
            menu = new cMenuChannels;
        break;
    case osTimers:
        if (!cPluginManager::CallFirstService("MainMenuHooksPatch-v1.0::osTimers", &menu))
            menu = new cMenuTimers;
        break;
    case osRecordings:
        if (!cPluginManager::CallFirstService("MainMenuHooksPatch-v1.0::osRecordings", &menu))
            menu = new cMenuRecordings(NULL, 0, true);
        break;
    case osSetup:      menu = new cMenuSetup; break;
    case osCommands:   menu = new cMenuCommands(tr("Commands"), &Commands); break;
#else
    case osSchedule:   AddSubMenu(new cMenuSchedule); break;
    case osChannels:   AddSubMenu(new cMenuChannels); break;
    case osTimers:     AddSubMenu(new cMenuTimers); break;
    case osRecordings: AddSubMenu(new cMenuRecordings(NULL, 0, true)); break;
    case osSetup:      AddSubMenu(new cMenuSetup); break;
    case osCommands:   AddSubMenu(new cMenuCommands(tr("Commands"), &Commands)); break;
#endif /* MAINMENUHOOKS */
#ifdef REELVDR
    //TODO: implement cMenuActiveEvent from old vdr, but is obsolete
    //case osActiveEvent: AddSubMenu(new cMenuActiveEvent); break;
#endif
    default: break;
    }
#ifdef USE_MAINMENUHOOKS
  if (menu)
      AddSubMenu(menu);
#endif /* MAINMENUHOOKS */
}

cOsdObject *cMenuMain::PluginOsdObject(void)
{
  cOsdObject *o = pluginOsdObject;
  pluginOsdObject = NULL;
  return o;
}

#ifdef USE_SETUP
void cMenuMain::Set(int current)
#else
void cMenuMain::Set(void)
#endif /* SETUP */
{
  Clear();
  SetTitle("VDR");
  SetHasHotkeys();

#ifdef USE_MENUORG
  if (MenuOrgPatch::IsCustomMenuAvailable()) {
     MenuItemDefinitions* menuItems = MenuOrgPatch::MainMenuItems();
     for (MenuItemDefinitions::iterator i = menuItems->begin(); i != menuItems->end(); i++) {
        cOsdItem* osdItem = NULL;
        if ((*i)->IsCustomOsdItem()) {
           osdItem = (*i)->CustomOsdItem();
           if (osdItem &&  !(*i)->IsSeparatorItem())
              osdItem->SetText(hk(osdItem->Text()));
           }
        else if ((*i)->IsPluginItem()) {
           const char *item = (*i)->PluginMenuEntry();
           if (item)
              osdItem = new cMenuPluginItem(hk(item), (*i)->PluginIndex());
           }
        if (osdItem) {
           Add(osdItem);
           if ((*i)->IsSelected())
              SetCurrent(osdItem);
           }
        }
     }
  else {
#endif /* MENUORG */
#ifdef USE_SETUP
  stopReplayItem = NULL;
  cancelEditingItem = NULL;
  stopRecordingItem = NULL;

  // remember initial dynamic MenuEntries added
  nrDynamicMenuEntries = Count();
  for (cSubMenuNode *node = subMenu.GetMenuTree()->First(); node; node = subMenu.GetMenuTree()->Next(node)) {
      cSubMenuNode::Type type = node->GetType();
      if (type==cSubMenuNode::PLUGIN) {
         const char *item = node->GetPluginMainMenuEntry();
#ifdef USE_PINPLUGIN
         if (item && !cStatus::MsgPluginProtected(cPluginManager::GetPlugin(node->GetPluginIndex()), true))
#else
         if (item)
#endif /* PINPLUGIN */
            Add(new cMenuPluginItem(hk(item), node->GetPluginIndex()));
         }
      else if (type==cSubMenuNode::MENU) {
         cString item = cString::sprintf("%s%s", node->GetName(), *subMenu.GetMenuSuffix());
#ifdef USE_PINPLUGIN
         if (!cStatus::MsgMenuItemProtected(item, true))
            Add(new cOsdItem(hk(item), osUnknown, node));
#else
            Add(new cOsdItem(hk(item)));
#endif /* PINPLUGIN */
         }
      else if ((type==cSubMenuNode::COMMAND) || (type==cSubMenuNode::THREAD)) {
#ifdef USE_PINPLUGIN
         if (!cStatus::MsgMenuItemProtected(node->GetName(), true))
            Add(new cOsdItem(hk(node->GetName()), osUnknown, node));
#else
            Add(new cOsdItem(hk(node->GetName())));
#endif /* PINPLUGIN */
         }
      else if (type==cSubMenuNode::SYSTEM) {
         const char *item = node->GetName();
#ifdef USE_PINPLUGIN
         if (cStatus::MsgMenuItemProtected(item, true))
            ; // nothing to do ;)
         else
#endif /* PINPLUGIN */
         if (strcmp(item, "Schedule") == 0)
            Add(new cOsdItem(hk(tr("Schedule")), osSchedule));
         else if (strcmp(item, "Channels") == 0)
            Add(new cOsdItem(hk(tr("Channels")), osChannels));
         else if (strcmp(item, "Timers") == 0)
            Add(new cOsdItem(hk(tr("Timers")), osTimers));
         else if (strcmp(item, "Recordings") == 0)
            Add(new cOsdItem(hk(tr("Recordings")), osRecordings));
         else if (strcmp(item, "Setup") == 0) {
            cString itemSetup = cString::sprintf("%s%s", tr("Setup"), *subMenu.GetMenuSuffix());
            Add(new cOsdItem(hk(itemSetup), osSetup));
            }
         else if (strcmp(item, "Commands") == 0 && Commands.Count() > 0) {
            cString itemCommands = cString::sprintf("%s%s", tr("Commands"), *subMenu.GetMenuSuffix());
            Add(new cOsdItem(hk(itemCommands), osCommands));
            }
         }
     }
  if (current >=0 && current<Count()) {
     SetCurrent(Get(current));
     }

#else /* NO SETUP */

  // Basic menu items:

#ifdef USE_PINPLUGIN
  if (!cStatus::MsgMenuItemProtected("Schedule", true))   Add(new cOsdItem(hk(tr("Schedule")),   osSchedule));
  if (!cStatus::MsgMenuItemProtected("Channels", true))   Add(new cOsdItem(hk(tr("Channels")),   osChannels));
  if (!cStatus::MsgMenuItemProtected("Timers", true))     Add(new cOsdItem(hk(tr("Timers")),     osTimers));
  if (!cStatus::MsgMenuItemProtected("Recordings", true)) Add(new cOsdItem(hk(tr("Recordings")), osRecordings));
#else
  Add(new cOsdItem(hk(tr("Schedule")),   osSchedule));
  Add(new cOsdItem(hk(tr("Channels")),   osChannels));
  Add(new cOsdItem(hk(tr("Timers")),     osTimers));
  Add(new cOsdItem(hk(tr("Recordings")), osRecordings));
#endif /* PINPLUGIN */

  // Plugins:

  for (int i = 0; ; i++) {
      cPlugin *p = cPluginManager::GetPlugin(i);
      if (p) {
#ifdef USE_PINPLUGIN
         if (!cStatus::MsgPluginProtected(p, true)) {
#endif /* PINPLUGIN */
         const char *item = p->MainMenuEntry();
         if (item)
            Add(new cMenuPluginItem(hk(item), i));
         }
#ifdef USE_PINPLUGIN
         }
#endif /* PINPLUGIN */
      else
         break;
      }

  // More basic menu items:

#ifdef USE_PINPLUGIN
  if (!cStatus::MsgMenuItemProtected("Setup", true)) Add(new cOsdItem(hk(tr("Setup")), osSetup));
#else
  Add(new cOsdItem(hk(tr("Setup")),      osSetup));
#endif /* PINPLUGIN */
  if (Commands.Count())
#ifdef USE_PINPLUGIN
     if (!cStatus::MsgMenuItemProtected("Commands", true))
#endif /* PINPLUGIN */
     Add(new cOsdItem(hk(tr("Commands")),  osCommands));
#endif /* SETUP */

#ifdef USE_MENUORG
  }
#endif /* MENUORG */

  Update(true);

  Display();
}

bool cMenuMain::Update(bool Force)
{
  bool result = false;

#ifdef USE_SETUP
  cOsdItem *fMenu = NULL;
  if (Force && subMenu.isTopMenu()) {
     fMenu = First();
     nrDynamicMenuEntries = 0;
     }

  if (subMenu.isTopMenu()) {
#endif /* SETUP */
  // Title with disk usage:
  if (FreeDiskSpace.HasChanged(Force)) {
     //XXX -> skin function!!!
     SetTitle(cString::sprintf("%s  -  %s", tr("VDR"), FreeDiskSpace.FreeDiskSpaceString()));
     result = true;
     }
#ifdef USE_SETUP
     }
  else {
     SetTitle(cString::sprintf("%s  -  %s", tr("VDR"), subMenu.GetParentMenuTitel()));
     result = true;
     }
#endif /* SETUP */

  bool NewReplaying = cControl::Control() != NULL;
  if (Force || NewReplaying != replaying) {
     replaying = NewReplaying;
     // Replay control:
     if (replaying && !stopReplayItem)
        // TRANSLATORS: note the leading blank!
#ifdef USE_LIEMIEXT
        if (Setup.MenuCmdPosition) Ins(stopReplayItem = new cOsdItem(tr(" Stop replaying"), osStopReplay)); else
#endif /* LIEMIEXT */
        Add(stopReplayItem = new cOsdItem(tr(" Stop replaying"), osStopReplay));
     else if (stopReplayItem && !replaying) {
        Del(stopReplayItem->Index());
        stopReplayItem = NULL;
        }
     // Color buttons:
     SetHelp(!replaying ? tr("Button$Record") : NULL, tr("Button$Audio"), replaying ? NULL : tr("Button$Pause"), replaying ? tr("Button$Stop") : cReplayControl::LastReplayed() ? tr("Button$Resume") : NULL);
     result = true;
     }

  // Editing control:
  bool CutterActive = cCutter::Active();
  if (CutterActive && !cancelEditingItem) {
     // TRANSLATORS: note the leading blank!
#ifdef USE_LIEMIEXT
     if (Setup.MenuCmdPosition) Ins(cancelEditingItem = new cOsdItem(tr(" Cancel editing"), osCancelEdit)); else
#endif /* LIEMIEXT */
     Add(cancelEditingItem = new cOsdItem(tr(" Cancel editing"), osCancelEdit));
     result = true;
     }
  else if (cancelEditingItem && !CutterActive) {
     Del(cancelEditingItem->Index());
     cancelEditingItem = NULL;
     result = true;
     }

  // Record control:
  if (cRecordControls::StateChanged(recordControlsState)) {
     while (stopRecordingItem) {
           cOsdItem *it = Next(stopRecordingItem);
           Del(stopRecordingItem->Index());
           stopRecordingItem = it;
           }
     const char *s = NULL;
     while ((s = cRecordControls::GetInstantId(s)) != NULL) {
           cOsdItem *item = new cOsdItem(osStopRecord);
           item->SetText(cString::sprintf("%s%s", tr(STOP_RECORDING), s));
#ifdef USE_LIEMIEXT
           if (Setup.MenuCmdPosition) Ins(item); else
#endif /* LIEMIEXT */
           Add(item);
           if (!stopRecordingItem)
              stopRecordingItem = item;
           }
     result = true;
     }

#ifdef USE_SETUP
  // adjust nrDynamicMenuEntries
  if (fMenu != NULL)
     nrDynamicMenuEntries = fMenu->Index();
#endif /* SETUP */

  return result;
}

eOSState cMenuMain::ProcessKey(eKeys Key)
{
  bool HadSubMenu = HasSubMenu();
  int osdLanguage = I18nCurrentLanguage();
  eOSState state = cOsdMenu::ProcessKey(Key);
  HadSubMenu |= HasSubMenu();

#ifdef USE_MAINMENUHOOKS
  cOsdMenu *menu = NULL;
#endif /* MAINMENUHOOKS */
#ifdef USE_PINPLUGIN
  cOsdItem* item = Get(Current());

  if (item && item->Text() && state != osContinue && state != osUnknown && state != osBack)
     if (cStatus::MsgMenuItemProtected(item->Text()))
        return osContinue;
#endif /* PINPLUGIN */

  switch (state) {
#ifdef USE_MAINMENUHOOKS
    case osSchedule:
        if (!cPluginManager::CallFirstService("MainMenuHooksPatch-v1.0::osSchedule", &menu))
            menu = new cMenuSchedule;
        else
            state = osContinue;
        break;
    case osChannels:
        if (!cPluginManager::CallFirstService("MainMenuHooksPatch-v1.0::osChannels", &menu))
            menu = new cMenuChannels;
        else
            state = osContinue;
        break;
    case osTimers:
        if (!cPluginManager::CallFirstService("MainMenuHooksPatch-v1.0::osTimers", &menu))
            menu = new cMenuTimers;
        else
            state = osContinue;
        break;
    case osRecordings:
        if (!cPluginManager::CallFirstService("MainMenuHooksPatch-v1.0::osRecordings", &menu))
            menu = new cMenuRecordings;
        else
            state = osContinue;
        break;
    case osSetup:      menu = new cMenuSetup; break;
    case osCommands:   menu = new cMenuCommands(tr("Commands"), &Commands); break;
#else
    case osSchedule:   return AddSubMenu(new cMenuSchedule);
    case osChannels:   return AddSubMenu(new cMenuChannels);
    case osTimers:     return AddSubMenu(new cMenuTimers);
    case osRecordings: return AddSubMenu(new cMenuRecordings);
    case osSetup:      return AddSubMenu(new cMenuSetup);
    case osCommands:   return AddSubMenu(new cMenuCommands(tr("Commands"), &Commands));
#endif /* MAINMENUHOOKS */
    case osStopRecord: if (Interface->Confirm(tr("Stop recording?"))) {
                          cOsdItem *item = Get(Current());
                          if (item) {
                             cRecordControls::Stop(item->Text() + strlen(tr(STOP_RECORDING)));
                             return osEnd;
                             }
                          }
                       break;
    case osCancelEdit: if (Interface->Confirm(tr("Cancel editing?"))) {
                          cCutter::Stop();
                          return osEnd;
                          }
                       break;
    case osPlugin:     {
                         cMenuPluginItem *item = (cMenuPluginItem *)Get(Current());
                         if (item) {
                            cPlugin *p = cPluginManager::GetPlugin(item->PluginIndex());
                            if (p) {
#ifdef USE_PINPLUGIN
                               if (!cStatus::MsgPluginProtected(p)) {
#endif /* PINPLUGIN */
                               cOsdObject *menu = p->MainMenuAction();
                               if (menu) {
                                  if (menu->IsMenu())
                                     return AddSubMenu((cOsdMenu *)menu);
                                  else {
                                     pluginOsdObject = menu;
                                     return osPlugin;
                                     }
                                  }
#ifdef USE_PINPLUGIN
                               }
#endif /* PINPLUGIN */
                            }
                         }
                         state = osEnd;
                       }
                       break;
#ifdef USE_MENUORG
    case osBack:       {
                          if (MenuOrgPatch::IsCustomMenuAvailable())
                          {
                              bool leavingMenuSucceeded = MenuOrgPatch::LeaveSubMenu();
                              Set();
                              stopReplayItem = NULL;
                              cancelEditingItem = NULL;
                              stopRecordingItem = NULL;
                              recordControlsState = 0;
                              Update(true);
                              Display();
                              if (leavingMenuSucceeded)
                                 return osContinue;
                              else
                                 return osEnd;
                          }
                       }
                       break;
    case osUser1:      {
                          if (MenuOrgPatch::IsCustomMenuAvailable()) {
                             MenuOrgPatch::EnterSubMenu(Get(Current()));
                             Set();
                             return osContinue;
                          }
                       }
                       break;
    case osUser2:      {
                          if (MenuOrgPatch::IsCustomMenuAvailable()) {
                             cOsdMenu* osdMenu = MenuOrgPatch::Execute(Get(Current()));
                             if (osdMenu)
                                return AddSubMenu(osdMenu);
                             return osEnd;
                          }
                       }
                       break;
#endif /* MENUORG */
#ifdef USE_SETUP
    case osBack:       {
                         int newCurrent = 0;
                         if (subMenu.Up(&newCurrent)) {
                            Set(newCurrent);
                            return osContinue;
                            }
                         else
                            return osEnd;
                       }
                       break;
#endif /* SETUP */
    default: switch (Key) {
               case kRecord:
               case kRed:    if (!HadSubMenu)
                                state = replaying ? osContinue : osRecord;
                             break;
               case kGreen:  if (!HadSubMenu) {
                                cRemote::Put(kAudio, true);
                                state = osEnd;
                                }
                             break;
               case kYellow: if (!HadSubMenu)
                                state = replaying ? osContinue : osPause;
                             break;
               case kBlue:   if (!HadSubMenu)
                                state = replaying ? osStopReplay : cReplayControl::LastReplayed() ? osReplay : osContinue;
                             break;
#ifdef USE_SETUP
               case kOk:     if (state == osUnknown) {
                                cString buffer;
                                int index = Current()-nrDynamicMenuEntries;
                                cSubMenuNode *node = subMenu.GetNode(index);

                                if (node != NULL) {
                                   if (node->GetType() == cSubMenuNode::MENU) {
#ifdef USE_PINPLUGIN
                                      subMenu.Down(node, Current());
#else
                                      subMenu.Down(index);
#endif /* PINPLUGIN */
                                      }
                                   else if (node->GetType() == cSubMenuNode::COMMAND) {
                                      bool confirmed = true;
                                      if (node->CommandConfirm()) {
                                         buffer = cString::sprintf("%s?", node->GetName());
                                         confirmed = Interface->Confirm(buffer);
                                         }
                                      if (confirmed) {
                                         const char *Result = subMenu.ExecuteCommand(node->GetCommand());
                                         if (Result)
                                            return AddSubMenu(new cMenuText(node->GetName(), Result, fontFix));
                                         return osEnd;
                                         }
                                      }
                                   else if (node->GetType() == cSubMenuNode::THREAD) {
                                      bool confirmed = true;
                                      if (node->CommandConfirm()) {
                                         buffer = cString::sprintf("%s?", node->GetName());
                                         confirmed = Interface->Confirm(buffer);
                                         }
                                      if (confirmed) {
                                         buffer = cString::sprintf("%s", node->GetCommand());
                                         cExecCmdThread *execcmd = new cExecCmdThread(node->GetCommand());
                                         if (execcmd->Start())
                                            dsyslog("executing command '%s'", *buffer);
                                         else
                                            esyslog("ERROR: can't execute command '%s'", *buffer);
                                         return osEnd;
                                         }
                                      }
                                   }

                                Set();
                                return osContinue;
                                }
                             break;
#endif /* SETUP */
               default:      break;
               }
    }
#ifdef USE_MAINMENUHOOKS
  if (menu)
      return AddSubMenu(menu);
#endif /* MAINMENUHOOKS */
  if (!HasSubMenu() && Update(HadSubMenu))
     Display();
  if (Key != kNone) {
     if (I18nCurrentLanguage() != osdLanguage) {
        Set();
        if (!HasSubMenu())
           Display();
        }
     }
  return state;
}

// --- SetTrackDescriptions --------------------------------------------------

static void SetTrackDescriptions(int LiveChannel)
{
  cDevice::PrimaryDevice()->ClrAvailableTracks(true);
  const cComponents *Components = NULL;
  cSchedulesLock SchedulesLock;
  if (LiveChannel) {
     cChannel *Channel = Channels.GetByNumber(LiveChannel);
     if (Channel) {
        const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);
        if (Schedules) {
           const cSchedule *Schedule = Schedules->GetSchedule(Channel);
           if (Schedule) {
              const cEvent *Present = Schedule->GetPresentEvent();
              if (Present)
                 Components = Present->Components();
              }
           }
        }
     }
  else if (cReplayControl::NowReplaying()) {
     cThreadLock RecordingsLock(&Recordings);
     cRecording *Recording = Recordings.GetByName(cReplayControl::NowReplaying());
     if (Recording)
        Components = Recording->Info()->Components();
     }
  if (Components) {
     int indexAudio = 0;
     int indexDolby = 0;
     int indexSubtitle = 0;
     for (int i = 0; i < Components->NumComponents(); i++) {
         const tComponent *p = Components->Component(i);
         switch (p->stream) {
           case 2: if (p->type == 0x05)
                      cDevice::PrimaryDevice()->SetAvailableTrack(ttDolby, indexDolby++, 0, LiveChannel ? NULL : p->language, p->description);
                   else
                      cDevice::PrimaryDevice()->SetAvailableTrack(ttAudio, indexAudio++, 0, LiveChannel ? NULL : p->language, p->description);
                   break;
           case 3: cDevice::PrimaryDevice()->SetAvailableTrack(ttSubtitle, indexSubtitle++, 0, LiveChannel ? NULL : p->language, p->description);
                   break;
           case 4: cDevice::PrimaryDevice()->SetAvailableTrack(ttDolby, indexDolby++, 0, LiveChannel ? NULL : p->language, p->description);
                   break;
           default: ;
           }
         }
     }
}

// --- cDisplayChannel -------------------------------------------------------

cDisplayChannel *cDisplayChannel::currentDisplayChannel = NULL;

cDisplayChannel::cDisplayChannel(int Number, bool Switched)
:cOsdObject(true)
{
#if REELVDR
  notNumberKey = true;
#endif
  currentDisplayChannel = this;
  group = -1;
  withInfo = !Switched || Setup.ShowInfoOnChSwitch;
  displayChannel = Skins.Current()->DisplayChannel(withInfo);
  number = 0;
  timeout = Switched || Setup.TimeoutRequChInfo;
  channel = Channels.GetByNumber(Number);
  lastPresent = lastFollowing = NULL;
  if (channel) {
     DisplayChannel();
     DisplayInfo();
     displayChannel->Flush();
     }
  lastTime.Set();
}

cDisplayChannel::cDisplayChannel(eKeys FirstKey)
:cOsdObject(true)
{
#if REELVDR
  notNumberKey = true;
#endif
  currentDisplayChannel = this;
  group = -1;
  number = 0;
  timeout = true;
  lastPresent = lastFollowing = NULL;
  lastTime.Set();
  withInfo = Setup.ShowInfoOnChSwitch;
  displayChannel = Skins.Current()->DisplayChannel(withInfo);
  channel = Channels.GetByNumber(cDevice::CurrentChannel());
  ProcessKey(FirstKey);
}

cDisplayChannel::~cDisplayChannel()
{
  delete displayChannel;
  cStatus::MsgOsdClear();
  currentDisplayChannel = NULL;
}

void cDisplayChannel::DisplayChannel(void)
{
  displayChannel->SetChannel(channel, number);
  cStatus::MsgOsdChannel(ChannelString(channel, number));
  lastPresent = lastFollowing = NULL;
}

void cDisplayChannel::DisplayInfo(void)
{
  if (withInfo && channel) {
     cSchedulesLock SchedulesLock;
     const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);
     if (Schedules) {
        const cSchedule *Schedule = Schedules->GetSchedule(channel);
        if (Schedule) {
           const cEvent *Present = Schedule->GetPresentEvent();
           const cEvent *Following = Schedule->GetFollowingEvent();
           if (Present != lastPresent || Following != lastFollowing) {
              SetTrackDescriptions(channel->Number());
              displayChannel->SetEvents(Present, Following);
              cStatus::MsgOsdProgramme(Present ? Present->StartTime() : 0, Present ? Present->Title() : NULL, Present ? Present->ShortText() : NULL, Following ? Following->StartTime() : 0, Following ? Following->Title() : NULL, Following ? Following->ShortText() : NULL);
              lastPresent = Present;
              lastFollowing = Following;
              }
           }
        }
     }
}

void cDisplayChannel::Refresh(void)
{
  DisplayChannel();
  displayChannel->SetEvents(NULL, NULL);
}

#if REELVDR
cChannel* NextChannelFromPlugin(cChannel* Channel, int Direction)
{
    bool gotNextChannel = false;
    cPlugin *plugin = cPluginManager::GetPlugin("reelchannellist");
    if (plugin) {
#if 0 /*send vdr's channel number to plugin*/
        struct Data{ int currChannel; int dir; int resultChannel;} data;
        data.currChannel = Channel->Number();
        data.dir = Direction;

        if(plugin->Service("Next channel in direction:number", &data)) {
            printf("Changing to channel number : %d\n", data.resultChannel);
            cChannel *nChannel = Channels.GetByNumber(data.resultChannel);
            if (nChannel) {
                gotNextChannel = true;
                Channel = nChannel;
            } else
                printf("got channel number invalid\n");
        } else
            printf("plugin service erorr\n");
#else /*send vdr's channel pointer to plugin*/
        struct Data { cChannel* channel; int dir; cChannel* result;} data;
        data.dir = Direction; data.channel = Channel; data.result = NULL;

        if(plugin->Service("Next channel in direction:pointer", &data)) {
            if (data.result) {
                gotNextChannel = true;
                Channel = data.result;
            } else
                printf("plugin returned NULL\n");
        } else
            printf("plugin service error\n");
#endif
    } else
        printf("no such plugin\n");

    return gotNextChannel ? Channel : NULL;

} // NextChannelFromPlugin()

#endif

cChannel *cDisplayChannel::NextAvailableChannel(cChannel *Channel, int Direction)
{
#if REELVDR
    // make sure loop does not run more than number of available channels
    // at times when no device is available, this loop never ends. Issue #652
    int TryCount = Channels.Count() + 1;
#endif
  if (Direction) {
     while (Channel) {
#if REELVDR
         if (TryCount < 0 ) {
             esyslog("Quiting %s. Tried  more times than there are channels. Channels.Count == %d",
                     __PRETTY_FUNCTION__, Channels.Count());
             //printf("\033[0;91m !!!!! Quit finding next channel. Tried  more times than there are channels. Channels.Count == %d\033[0m\”\n", Channels.Count());
             break;
         }
         else TryCount --;

         cChannel *ch = NULL;
         if(Setup.UseZonedChannelList)
             ch = NextChannelFromPlugin(Channel, Direction);
         else
                 printf("Not using ZONED channel list\n");
         if(ch)
             Channel = ch;
         else
             Channel = Direction > 0 ? Channels.Next(Channel) : Channels.Prev(Channel);
#else
         Channel = Direction > 0 ? Channels.Next(Channel) : Channels.Prev(Channel);
#endif /* REELVDR */

           if (!Channel && Setup.ChannelsWrap)
              Channel = Direction > 0 ? Channels.First() : Channels.Last();
#ifdef USE_PINPLUGIN
        if (cStatus::MsgChannelProtected(0, Channel) == false)
#endif /* PINPLUGIN */
           if (Channel && !Channel->GroupSep() && cDevice::GetDevice(Channel, 0, true))
              return Channel;
           }
     }
  return NULL;
}

eOSState cDisplayChannel::ProcessKey(eKeys Key)
{
  cChannel *NewChannel = NULL;

  if (Key != kNone)
     lastTime.Set();
  switch (int(Key)) {
    case k0:
         if (number == 0) {
            // keep the "Toggle channels" function working
            cRemote::Put(Key);
            return osEnd;
            }
    case k1 ... k9:
#if REELVDR
      notNumberKey = false;
#endif

         group = -1;
         if (number >= 0) {
            if (number > Channels.MaxNumber())
               number = Key - k0;
            else
               number = number * 10 + Key - k0;
            channel = Channels.GetByNumber(number);
            Refresh();
            withInfo = false;
            // Lets see if there can be any useful further input:
            int n = channel ? number * 10 : 0;
            int m = 10;
            cChannel *ch = channel;
            while (ch && (ch = Channels.Next(ch)) != NULL) {
                  if (!ch->GroupSep()) {
                     if (n <= ch->Number() && ch->Number() < n + m) {
                        n = 0;
                        break;
                        }
                     if (ch->Number() > n) {
                        n *= 10;
                        m *= 10;
                        }
                     }
                  }
            if (n > 0) {
               // This channel is the only one that fits the input, so let's take it right away:
               NewChannel = channel;
               withInfo = true;
               number = 0;
               Refresh();
               }
            }
         break;
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight:
#ifdef USE_VOLCTRL
         if (Setup.LRVolumeControl && !Setup.LRChannelGroups) {
            cRemote::Put(NORMALKEY(Key) == kLeft ? kVolDn : kVolUp, true);
            break;
            }
         // else fall through
#endif /* VOLCTRL */
    case kNext|k_Repeat:
    case kNext:
    case kPrev|k_Repeat:
    case kPrev:
         withInfo = false;
         number = 0;
         if (group < 0) {
            cChannel *channel = Channels.GetByNumber(cDevice::CurrentChannel());
            if (channel)
               group = channel->Index();
            }
         if (group >= 0) {
            int SaveGroup = group;
            if (NORMALKEY(Key) == kRight || NORMALKEY(Key) == kNext)
               group = Channels.GetNextGroup(group) ;
            else
               group = Channels.GetPrevGroup(group < 1 ? 1 : group);
            if (group < 0)
               group = SaveGroup;
            channel = Channels.Get(group);
            if (channel) {
               Refresh();
               if (!channel->GroupSep())
                  group = -1;
               }
            }
         break;
    case kUp|k_Repeat:
    case kUp:
    case kDown|k_Repeat:
    case kDown:
    case kChanUp|k_Repeat:
    case kChanUp:
    case kChanDn|k_Repeat:
    case kChanDn: {
         eKeys k = NORMALKEY(Key);
         cChannel *ch = NextAvailableChannel(channel, (k == kUp || k == kChanUp) ? 1 : -1);
         if (ch)
            channel = ch;
         else if (channel && channel->Number() != cDevice::CurrentChannel())
            Key = k; // immediately switches channel when hitting the beginning/end of the channel list with k_Repeat
         }
         // no break here
    case kUp|k_Release:
    case kDown|k_Release:
    case kChanUp|k_Release:
    case kChanDn|k_Release:
    case kNext|k_Release:
    case kPrev|k_Release:
         if (!(Key & k_Repeat) && channel && channel->Number() != cDevice::CurrentChannel())
            NewChannel = channel;
         withInfo = true;
         group = -1;
         number = 0;
         Refresh();
         break;
    case kNone:
         if (number && Setup.ChannelEntryTimeout && int(lastTime.Elapsed()) > Setup.ChannelEntryTimeout) {
            channel = Channels.GetByNumber(number);
            if (channel)
               NewChannel = channel;
            withInfo = true;
            number = 0;
            Refresh();
            lastTime.Set();
            }
         break;
    //TODO
    //XXX case kGreen:  return osEventNow;
    //XXX case kYellow: return osEventNext;
    case kOk:
         if (group >= 0) {
            channel = Channels.Get(Channels.GetNextNormal(group));
            if (channel)
               NewChannel = channel;
            withInfo = true;
            group = -1;
            Refresh();
            }
         else if (number > 0) {
            channel = Channels.GetByNumber(number);
            if (channel)
               NewChannel = channel;
            withInfo = true;
            number = 0;
            Refresh();
            }
         else
            return osEnd;
         break;
#ifdef REELVDR
    case kInfo: return osEnd; break;
#endif
    default:
         if ((Key & (k_Repeat | k_Release)) == 0) {
            cRemote::Put(Key);
            return osEnd;
            }
    };
  if (!timeout || lastTime.Elapsed() < (uint64_t)(Setup.ChannelInfoTime * 1000)) {
     if (Key == kNone && !number && group < 0 && !NewChannel && channel && channel->Number() != cDevice::CurrentChannel()) {
        // makes sure a channel switch through the SVDRP CHAN command is displayed
        channel = Channels.GetByNumber(cDevice::CurrentChannel());
        Refresh();
        lastTime.Set();
        }
     DisplayInfo();
     displayChannel->Flush();
     if (NewChannel) {
        SetTrackDescriptions(NewChannel->Number()); // to make them immediately visible in the channel display
        Channels.SwitchTo(NewChannel->Number());
        SetTrackDescriptions(NewChannel->Number()); // switching the channel has cleared them
        channel = NewChannel;
#if REELVDR /* if number key was used, here after use vdr's channel list to zap*/
        if (!notNumberKey) {
            Setup.UseZonedChannelList = false;
            notNumberKey = true;
        }
#endif /*REELVDR*/
        }
     return osContinue;
     }
  return osEnd;
}

// --- cDisplayVolume --------------------------------------------------------

#define VOLUMETIMEOUT 1000 //ms
#define MUTETIMEOUT   5000 //ms

cDisplayVolume *cDisplayVolume::currentDisplayVolume = NULL;

cDisplayVolume::cDisplayVolume(void)
:cOsdObject(true)
{
  currentDisplayVolume = this;
  timeout.Set(cDevice::PrimaryDevice()->IsMute() ? MUTETIMEOUT : VOLUMETIMEOUT);
  displayVolume = Skins.Current()->DisplayVolume();
  Show();
}

cDisplayVolume::~cDisplayVolume()
{
  delete displayVolume;
  currentDisplayVolume = NULL;
}

void cDisplayVolume::Show(void)
{
  displayVolume->SetVolume(cDevice::CurrentVolume(), MAXVOLUME, cDevice::PrimaryDevice()->IsMute());
}

cDisplayVolume *cDisplayVolume::Create(void)
{
  if (!currentDisplayVolume)
     new cDisplayVolume;
  return currentDisplayVolume;
}

void cDisplayVolume::Process(eKeys Key)
{
  if (currentDisplayVolume)
     currentDisplayVolume->ProcessKey(Key);
}

eOSState cDisplayVolume::ProcessKey(eKeys Key)
{
  switch (int(Key)) {
#ifdef USE_VOLCTRL
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight:
         if (Setup.LRVolumeControl) {
            cRemote::Put(NORMALKEY(Key) == kLeft ? kVolDn : kVolUp, true);
            break;
            }
         // else fall through
#endif /* VOLCTRL */
    case kVolUp|k_Repeat:
    case kVolUp:
    case kVolDn|k_Repeat:
    case kVolDn:
         Show();
         timeout.Set(VOLUMETIMEOUT);
         break;
    case kMute:
         if (cDevice::PrimaryDevice()->IsMute()) {
            Show();
            timeout.Set(MUTETIMEOUT);
            }
         else
            timeout.Set();
         break;
    case kNone: break;
    default: if ((Key & k_Release) == 0) {
                cRemote::Put(Key);
                return osEnd;
                }
    }
  return timeout.TimedOut() ? osEnd : osContinue;
}

// --- cDisplayTracks --------------------------------------------------------

#define TRACKTIMEOUT 5000 //ms

cDisplayTracks *cDisplayTracks::currentDisplayTracks = NULL;

cDisplayTracks::cDisplayTracks(void)
:cOsdObject(true)
{
  cDevice::PrimaryDevice()->EnsureAudioTrack();
  SetTrackDescriptions(!cDevice::PrimaryDevice()->Replaying() || cDevice::PrimaryDevice()->Transferring() ? cDevice::CurrentChannel() : 0);
  currentDisplayTracks = this;
  numTracks = track = 0;
  audioChannel = cDevice::PrimaryDevice()->GetAudioChannel();
  eTrackType CurrentAudioTrack = cDevice::PrimaryDevice()->GetCurrentAudioTrack();
  for (int i = ttAudioFirst; i <= ttDolbyLast; i++) {
      const tTrackId *TrackId = cDevice::PrimaryDevice()->GetTrack(eTrackType(i));
      if (TrackId && TrackId->id) {
         types[numTracks] = eTrackType(i);
         descriptions[numTracks] = strdup(*TrackId->description ? TrackId->description : *TrackId->language ? TrackId->language : *itoa(i));
         if (i == CurrentAudioTrack)
            track = numTracks;
         numTracks++;
         }
      }
  descriptions[numTracks] = NULL;
  timeout.Set(TRACKTIMEOUT);
  displayTracks = Skins.Current()->DisplayTracks(tr("Button$Audio"), numTracks, descriptions);
  Show();
}

cDisplayTracks::~cDisplayTracks()
{
  delete displayTracks;
  currentDisplayTracks = NULL;
  for (int i = 0; i < numTracks; i++)
      free(descriptions[i]);
  cStatus::MsgOsdClear();
}

void cDisplayTracks::Show(void)
{
  int ac = IS_AUDIO_TRACK(types[track]) ? audioChannel : -1;
  displayTracks->SetTrack(track, descriptions);
  displayTracks->SetAudioChannel(ac);
  displayTracks->Flush();
  cStatus::MsgSetAudioTrack(track, descriptions);
  cStatus::MsgSetAudioChannel(ac);
}

cDisplayTracks *cDisplayTracks::Create(void)
{
  if (cDevice::PrimaryDevice()->NumAudioTracks() > 0) {
     if (!currentDisplayTracks)
        new cDisplayTracks;
     return currentDisplayTracks;
     }
  Skins.Message(mtWarning, tr("No audio available!"));
  return NULL;
}

void cDisplayTracks::Process(eKeys Key)
{
  if (currentDisplayTracks)
     currentDisplayTracks->ProcessKey(Key);
}

eOSState cDisplayTracks::ProcessKey(eKeys Key)
{
  int oldTrack = track;
  int oldAudioChannel = audioChannel;
  switch (int(Key)) {
    case kUp|k_Repeat:
    case kUp:
    case kDown|k_Repeat:
    case kDown:
         if (NORMALKEY(Key) == kUp && track > 0)
            track--;
         else if (NORMALKEY(Key) == kDown && track < numTracks - 1)
            track++;
         timeout.Set(TRACKTIMEOUT);
         break;
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight: if (IS_AUDIO_TRACK(types[track])) {
                    static int ac[] = { 1, 0, 2 };
                    audioChannel = ac[cDevice::PrimaryDevice()->GetAudioChannel()];
                    if (NORMALKEY(Key) == kLeft && audioChannel > 0)
                       audioChannel--;
                    else if (NORMALKEY(Key) == kRight && audioChannel < 2)
                       audioChannel++;
                    audioChannel = ac[audioChannel];
                    timeout.Set(TRACKTIMEOUT);
                    }
         break;
    case kAudio|k_Repeat:
    case kAudio:
         if (++track >= numTracks)
            track = 0;
         timeout.Set(TRACKTIMEOUT);
         break;
    case kOk:
         if (types[track] != cDevice::PrimaryDevice()->GetCurrentAudioTrack())
            oldTrack = -1; // make sure we explicitly switch to that track
         timeout.Set();
         break;
    case kNone: break;
    default: if ((Key & k_Release) == 0)
                return osEnd;
    }
  if (track != oldTrack || audioChannel != oldAudioChannel)
     Show();
  if (track != oldTrack) {
     cDevice::PrimaryDevice()->SetCurrentAudioTrack(types[track]);
     Setup.CurrentDolby = IS_DOLBY_TRACK(types[track]);
     }
  if (audioChannel != oldAudioChannel)
     cDevice::PrimaryDevice()->SetAudioChannel(audioChannel);
  return timeout.TimedOut() ? osEnd : osContinue;
}

// --- cDisplaySubtitleTracks ------------------------------------------------

cDisplaySubtitleTracks *cDisplaySubtitleTracks::currentDisplayTracks = NULL;

cDisplaySubtitleTracks::cDisplaySubtitleTracks(void)
:cOsdObject(true)
{
  SetTrackDescriptions(!cDevice::PrimaryDevice()->Replaying() || cDevice::PrimaryDevice()->Transferring() ? cDevice::CurrentChannel() : 0);
  currentDisplayTracks = this;
  numTracks = track = 0;
  types[numTracks] = ttNone;
  descriptions[numTracks] = strdup(tr("No subtitles"));
  numTracks++;
  eTrackType CurrentSubtitleTrack = cDevice::PrimaryDevice()->GetCurrentSubtitleTrack();
  for (int i = ttSubtitleFirst; i <= ttSubtitleLast; i++) {
      const tTrackId *TrackId = cDevice::PrimaryDevice()->GetTrack(eTrackType(i));
      if (TrackId && TrackId->id) {
         types[numTracks] = eTrackType(i);
         descriptions[numTracks] = strdup(*TrackId->description ? TrackId->description : *TrackId->language ? TrackId->language : *itoa(i));
         if (i == CurrentSubtitleTrack)
            track = numTracks;
         numTracks++;
         }
      }
  descriptions[numTracks] = NULL;
  timeout.Set(TRACKTIMEOUT);
  displayTracks = Skins.Current()->DisplayTracks(tr("Button$Subtitles"), numTracks, descriptions);
  Show();
}

cDisplaySubtitleTracks::~cDisplaySubtitleTracks()
{
  delete displayTracks;
  currentDisplayTracks = NULL;
  for (int i = 0; i < numTracks; i++)
      free(descriptions[i]);
  cStatus::MsgOsdClear();
}

void cDisplaySubtitleTracks::Show(void)
{
  displayTracks->SetTrack(track, descriptions);
  displayTracks->Flush();
  cStatus::MsgSetSubtitleTrack(track, descriptions);
}

cDisplaySubtitleTracks *cDisplaySubtitleTracks::Create(void)
{
  if (cDevice::PrimaryDevice()->NumSubtitleTracks() > 0) {
     if (!currentDisplayTracks)
        new cDisplaySubtitleTracks;
     return currentDisplayTracks;
     }
  Skins.Message(mtWarning, tr("No subtitles available!"));
  return NULL;
}

void cDisplaySubtitleTracks::Process(eKeys Key)
{
  if (currentDisplayTracks)
     currentDisplayTracks->ProcessKey(Key);
}

eOSState cDisplaySubtitleTracks::ProcessKey(eKeys Key)
{
  int oldTrack = track;
  switch (int(Key)) {
    case kUp|k_Repeat:
    case kUp:
    case kDown|k_Repeat:
    case kDown:
         if (NORMALKEY(Key) == kUp && track > 0)
            track--;
         else if (NORMALKEY(Key) == kDown && track < numTracks - 1)
            track++;
         timeout.Set(TRACKTIMEOUT);
         break;
    case kSubtitles|k_Repeat:
    case kSubtitles:
         if (++track >= numTracks)
            track = 0;
         timeout.Set(TRACKTIMEOUT);
         break;
    case kOk:
         if (types[track] != cDevice::PrimaryDevice()->GetCurrentSubtitleTrack())
            oldTrack = -1; // make sure we explicitly switch to that track
         timeout.Set();
         break;
    case kNone: break;
    default: if ((Key & k_Release) == 0)
                return osEnd;
    }
  if (track != oldTrack) {
     Show();
     cDevice::PrimaryDevice()->SetCurrentSubtitleTrack(types[track], true);
     }
  return timeout.TimedOut() ? osEnd : osContinue;
}

// --- cRecordControl --------------------------------------------------------

#ifdef USE_ALTERNATECHANNEL
cRecordControl::cRecordControl(cDevice *Device, cTimer *Timer, bool Pause, cChannel *Channel)
#else
cRecordControl::cRecordControl(cDevice *Device, cTimer *Timer, bool Pause)
#endif /* ALTERNATECHANNEL */
{
#ifdef USE_DVLRECSCRIPTADDON
  const cChannel *recChan = NULL;
  char *chanName = NULL;
#endif /* DVLRECSCRIPTADDON */
  // We're going to manipulate an event here, so we need to prevent
  // others from modifying any EPG data:
  cSchedulesLock SchedulesLock;
  cSchedules::Schedules(SchedulesLock);

  event = NULL;
  fileName = NULL;
  recorder = NULL;
  device = Device;
  if (!device) device = cDevice::PrimaryDevice();//XXX
  timer = Timer;
  if (!timer) {
     timer = new cTimer(true, Pause);
     Timers.Add(timer);
     Timers.SetModified();
     instantId = cString::sprintf(cDevice::NumDevices() > 1 ? "%s - %d" : "%s", timer->Channel()->Name(), device->CardIndex() + 1);
     }
  timer->SetPending(true);
  timer->SetRecording(true);
  event = timer->Event();

  if (event || GetEvent())
     dsyslog("Title: '%s' Subtitle: '%s'", event->Title(), event->ShortText());
  cRecording Recording(timer, event);
  fileName = strdup(Recording.FileName());

  // crude attempt to avoid duplicate recordings:
  if (cRecordControls::GetRecordControl(fileName)) {
     isyslog("already recording: '%s'", fileName);
     if (Timer) {
        timer->SetPending(false);
        timer->SetRecording(false);
        timer->OnOff();
        }
     else {
        Timers.Del(timer);
        Timers.SetModified();
        if (!cReplayControl::LastReplayed()) // an instant recording, maybe from cRecordControls::PauseLiveVideo()
           cReplayControl::SetRecording(fileName, Recording.Name());
        }
     timer = NULL;
     return;
     }

#ifdef USE_DVLRECSCRIPTADDON
  if (timer)
     if ((recChan = timer->Channel()) != NULL)
        chanName = strdup(recChan->Name());
  if (chanName != NULL) {
     cRecordingUserCommand::InvokeCommand(RUC_BEFORERECORDING, fileName, chanName);
     free(chanName);
     }
  else
#endif /* DVLRECSCRIPTADDON */
  cRecordingUserCommand::InvokeCommand(RUC_BEFORERECORDING, fileName);
  isyslog("record %s", fileName);
  if (MakeDirs(fileName, true)) {
#ifdef USE_ALTERNATECHANNEL
     const cChannel *ch = Channel ? Channel : timer->Channel();
     if (ch)
#ifdef USE_LIVEBUFFER
        recorder = new cBufferRecorder(fileName, ch, timer->Priority(), cRecordControls::GetLiveBuffer(timer));
#else
        recorder = new cRecorder(fileName, ch, timer->Priority());
#endif
     if (ch && device->AttachReceiver(recorder)) {
#else
     const cChannel *ch = timer->Channel();
#ifdef USE_LIVEBUFFER
     recorder = new cBufferRecorder(fileName, ch, timer->Priority(), cRecordControls::GetLiveBuffer(timer));
#else
     recorder = new cRecorder(fileName, ch, timer->Priority());
#endif
     if (device->AttachReceiver(recorder)) {
#endif /* ALTERNATECHANNEL */
#ifdef REELVDR
        //TODO: set HD info
        // if (recorder->GetRemux()->SFmode()==SF_H264)
        //     Recording.SetIsHD(true);
#endif
        Recording.WriteInfo();
        cStatus::MsgRecording(device, Recording.Name(), Recording.FileName(), true);
        if (!Timer && !cReplayControl::LastReplayed()) // an instant recording, maybe from cRecordControls::PauseLiveVideo()
           cReplayControl::SetRecording(fileName, Recording.Name());
        Recordings.AddByName(fileName);
        return;
        }
     else
#ifdef USE_ALTERNATECHANNEL
        if (ch)
        DELETENULL(recorder);
#else
        DELETENULL(recorder);
#endif /* ALTERNATECHANNEL */
     }
  else
     timer->SetDeferred(DEFERTIMER);
  if (!Timer) {
     Timers.Del(timer);
     Timers.SetModified();
     timer = NULL;
     }
}

cRecordControl::~cRecordControl()
{
  Stop();
  free(fileName);
}

#define INSTANT_REC_EPG_LOOKAHEAD 300 // seconds to look into the EPG data for an instant recording

bool cRecordControl::GetEvent(void)
{
  const cChannel *channel = timer->Channel();
  time_t Time = timer->HasFlags(tfInstant) ? timer->StartTime() + INSTANT_REC_EPG_LOOKAHEAD : timer->StartTime() + (timer->StopTime() - timer->StartTime()) / 2;
  for (int seconds = 0; seconds <= MAXWAIT4EPGINFO; seconds++) {
      {
        cSchedulesLock SchedulesLock;
        const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);
        if (Schedules) {
           const cSchedule *Schedule = Schedules->GetSchedule(channel);
           if (Schedule) {
              event = Schedule->GetEventAround(Time);
              if (event) {
                 if (seconds > 0)
                    dsyslog("got EPG info after %d seconds", seconds);
                 return true;
                 }
              }
           }
      }
      if (seconds == 0)
         dsyslog("waiting for EPG info...");
      sleep(1);
      }
  dsyslog("no EPG info available");
  return false;
}

void cRecordControl::Stop(bool ExecuteUserCommand)
{
  if (timer) {
#ifdef USE_DVLRECSCRIPTADDON
     char *chanName = NULL;
     const cChannel *recChan = NULL;

     recChan = timer -> Channel();
     if (recChan != NULL)
        chanName = strdup(recChan -> Name());
#endif /* DVLRECSCRIPTADDON */
     DELETENULL(recorder);
     timer->SetRecording(false);
     timer = NULL;
     cStatus::MsgRecording(device, NULL, fileName, false);
     if (ExecuteUserCommand)
#ifdef USE_DVLRECSCRIPTADDON
        cRecordingUserCommand::InvokeCommand(RUC_AFTERRECORDING, fileName, chanName);
     if (chanName != NULL)
        free(chanName);
#else
        cRecordingUserCommand::InvokeCommand(RUC_AFTERRECORDING, fileName);
#endif /* DVLRECSCRIPTADDON */
     }
}

bool cRecordControl::Process(time_t t)
{
  if (!recorder || !recorder->IsAttached() || !timer || !timer->Matches(t)) {
     if (timer)
        timer->SetPending(false);
     return false;
     }
  AssertFreeDiskSpace(timer->Priority());
  return true;
}

// --- cRecordControls -------------------------------------------------------

cRecordControl *cRecordControls::RecordControls[MAXRECORDCONTROLS] = { NULL };
int cRecordControls::state = 0;

#ifdef USE_LIVEBUFFER
cLiveRecorder *cRecordControls::liveRecorder = NULL;
#endif /*USE_LIVEBUFFER*/

bool cRecordControls::Start(cTimer *Timer, bool Pause)
{
  static time_t LastNoDiskSpaceMessage = 0;
  int FreeMB = 0;
  if (Timer) {
     AssertFreeDiskSpace(Timer->Priority(), !Timer->Pending());
     Timer->SetPending(true);
     }
  VideoDiskSpace(&FreeMB);
#if REELVDR
  if(Setup.ReelboxModeTemp != eModeClient) // Don't check disk usage on Clients
  {
#endif
  if (FreeMB < MINFREEDISK) {
     if (!Timer || time(NULL) - LastNoDiskSpaceMessage > NODISKSPACEDELTA) {
        isyslog("not enough disk space to start recording%s%s", Timer ? " timer " : "", Timer ? *Timer->ToDescr() : "");
        Skins.Message(mtWarning, tr("Not enough disk space to start recording!"));
        LastNoDiskSpaceMessage = time(NULL);
        }
     return false;
     }
  LastNoDiskSpaceMessage = 0;

  ChangeState();
#if REELVDR
  }
#endif
  int ch = Timer ? Timer->Channel()->Number() : cDevice::CurrentChannel();
  cChannel *channel = Channels.GetByNumber(ch);

  if (channel) {
     int Priority = Timer ? Timer->Priority() : Pause ? Setup.PausePriority : Setup.DefaultPriority;
     cDevice *device = cDevice::GetDevice(channel, Priority, false);

#ifdef USE_ALTERNATECHANNEL
     if (!device && channel->AlternativeChannelID().Valid()) {// check for alternatives
        dsyslog("prepare to use alternative channel for channel %d", channel->Number());
        channel = Channels.GetByChannelID(channel->AlternativeChannelID());
        device = cDevice::GetDevice(channel, Priority, false);
        if (device)
            dsyslog("use of alternative channel %d successfully initiated", channel->Number());
        }
#endif /* ALTERNATECHANNEL */

     if (device) {
        dsyslog("switching device %d to channel %d", device->DeviceNumber() + 1, channel->Number());
#ifdef USE_LNBSHARE
        cDevice *tmpDevice;
        while ((tmpDevice = device->GetBadDevice(channel))) {
          if (tmpDevice->Replaying() == false) {
            Stop(tmpDevice);
            if (tmpDevice->CardIndex() == tmpDevice->ActualDevice()->CardIndex())
              tmpDevice->SwitchChannelForced(channel, true);
            else
              tmpDevice->SwitchChannelForced(channel, false);
          } else
                 tmpDevice->SwitchChannelForced(channel, false);
           }
#endif /* LNBSHARE */
        if (!device->SwitchChannel(channel, false)) {
           esyslog("ERROR: Cannot Switch to Channel number %d in device %d. Requesting Emergency Exit.",
                     channel->Number(), device->DeviceNumber()+1);
           ShutdownHandler.RequestEmergencyExit();
           return false;
           }
        if (!Timer || Timer->Matches()) {
#ifdef USEMYSQL
            // On Clients, just create new timer since AVG will record
            if(Setup.ReelboxModeTemp == eModeClient)
            {
                if (!Timer)
                {
                    std::vector<cTimer*> InstantRecordings;
                    Timers.GetInstantRecordings(&InstantRecordings);
                    bool InstantRecordingExists = false;

                    if(InstantRecordings.size())
                    {
                        unsigned int i = 0;
                        while(!InstantRecordingExists && i < InstantRecordings.size())
                        {
                            if(InstantRecordings.at(i)->Channel() == channel)
                                InstantRecordingExists = true;
                            ++i;
                        }
                    }

                    // Don't record if there is already a running recording
                    if(InstantRecordingExists)
                    {
                        isyslog("an instant recording is already running '%s'", channel->Name());
                        Skins.Message(mtWarning, tr("An instant recording is already active!"));
                        Timer = NULL;
                    }
                    else
                        Timer = new cTimer(true, Pause);

                    if(Timer)
                    {
                        if(!Timers.Add(Timer))
                        {
                            Skins.Message(mtError, tr("Could not add timer"));
                            delete Timer;
                            return false;
                        }
                        Timers.SetModified();
                        return true;
                    }
                }
            }
            else
#endif
           for (int i = 0; i < MAXRECORDCONTROLS; i++) {
               if (!RecordControls[i]) {
#ifdef USE_ALTERNATECHANNEL
                  RecordControls[i] = new cRecordControl(device, Timer, Pause, channel);
#else
                  RecordControls[i] = new cRecordControl(device, Timer, Pause);
#endif /* ALTERNATECHANNEL */
#ifdef USE_PINPLUGIN
                  cStatus::MsgRecordingFile(RecordControls[i]->FileName());
#endif /* PINPLUGIN */
                  return RecordControls[i]->Process(time(NULL));
                  }
               }
           }
        }
     else if (!Timer || (Timer->Priority() >= Setup.PrimaryLimit && !Timer->Pending())) {
        isyslog("no free DVB device to record channel %d!", ch);
        Skins.Message(mtError, tr("No free DVB device to record!"));
        }
     }
  else
     esyslog("ERROR: channel %d not defined!", ch);
  return false;
}

void cRecordControls::Stop(const char *InstantId)
{
  ChangeState();
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
      if (RecordControls[i]) {
         const char *id = RecordControls[i]->InstantId();
         if (id && strcmp(id, InstantId) == 0) {
            cTimer *timer = RecordControls[i]->Timer();
            RecordControls[i]->Stop();
            if (timer) {
               isyslog("deleting timer %s", *timer->ToDescr());
               Timers.Del(timer);
               Timers.SetModified();
               }
            break;
            }
         }
      }
}

#ifdef USE_LNBSHARE
void cRecordControls::Stop(cDevice *Device)
{
  ChangeState();
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
      if (RecordControls[i]) {
         if (RecordControls[i]->Device() == Device) {
            isyslog("stopping recording on DVB device %d due to higher priority", Device->CardIndex() + 1);
            RecordControls[i]->Stop();
            }
         }
      }
}
#endif /* LNBSHARE */

#ifdef USE_LIVEBUFFER
bool cRecordControls::StartLiveBuffer(eKeys Key) {
  if(Setup.LiveBuffer && liveRecorder) { //TODO: Use Setup.PauseKeyHandling == 3 instead of Setup.LiveBuffer
     int pos = liveRecorder->LastIFrame();
isyslog("Enter timeshift at %d / %d", pos, liveRecorder->LastFrame());
     liveRecorder->SetResume(pos?pos:liveRecorder->LastFrame());
     cReplayControl::SetRecording(cLiveRecorder::FileName(), tr("Timeshift mode"));
     cReplayControl *rc = new cReplayControl;
     cControl::Launch(rc);
     cControl::Attach();
     if(kPause==Key) rc->Goto(pos, true); // Force display of current frame
     else rc->ProcessKey(Key);
     //rc->Show(); // show progressbar at the start of livebuffer
     rc->ShowTimed(3); //RC - timeout PB
     return true;
  } // if
  return false;
} // cRecordControls::StartLiveBuffer
#endif /*USE_LIVEBUFFER*/

bool cRecordControls::PauseLiveVideo(void)
{
#ifdef USE_LIVEBUFFER
  if(StartLiveBuffer(kPause)) //TODO: Use Setup.PauseKeyHandling == 3 instead of Setup.LiveBuffer
     return true;
#endif /*USE_LIVEBUFFER*/
  Skins.Message(mtStatus, tr("Pausing live video..."));
  cReplayControl::SetRecording(NULL, NULL); // make sure the new cRecordControl will set cReplayControl::LastReplayed()
  if (Start(NULL, true)) {
     sleep(2); // allow recorded file to fill up enough to start replaying
     cReplayControl *rc = new cReplayControl;
     cControl::Launch(rc);
     cControl::Attach();
     sleep(1); // allow device to replay some frames, so we have a picture
     Skins.Message(mtStatus, NULL);
     rc->ProcessKey(kPause); // pause, allowing replay mode display
     return true;
     }
  Skins.Message(mtStatus, NULL);
  return false;
}

#ifdef USE_LIVEBUFFER
void cRecordControls::SetLiveChannel(cDevice *Device, const cChannel *Channel) {
	if(liveRecorder) {
		if(Channel && Device && (liveRecorder->ChannelID()==Channel->GetChannelID()))
			Device->AttachReceiver(liveRecorder);
		else
			DELETENULL(liveRecorder);
	} // if
	if(Device && Channel) cControl::Launch(new cTransferControl(Device, Channel));
	if(Setup.LiveBuffer && Channel && Device && !liveRecorder) { //TODO: Use Setup.PauseKeyHandling == 3 instead of Setup.LiveBuffer
		if (cLiveRecorder::Prepare()) {
			liveRecorder = new cLiveRecorder(Channel);
			if(!Device->AttachReceiver(liveRecorder))
				DELETENULL(liveRecorder);
		} // if
	} // if
} // cRecordControls::SetLiveChannel

bool cRecordControls::CanSetLiveChannel(const cChannel *Channel) {
	if(liveRecorder && Channel && (liveRecorder->ChannelID()==Channel->GetChannelID())) return true;
	return !IsWritingBuffer();
} // cRecordControls::CanSetLiveChannel

bool cRecordControls::IsWritingBuffer() {
	return liveRecorder ? liveRecorder->IsWritingBuffer() : false;
} // cRecordControls::IsWritingBuffer

void cRecordControls::CancelWritingBuffer() {
	if(liveRecorder && liveRecorder->IsWritingBuffer()) {
		liveRecorder->CancelWritingBuffer();
		sleep(1); // allow recorder to really stop
	} // if
} // cRecordControls::CancelWritingBuffer

cIndex *cRecordControls::GetLiveBuffer(cTimer *Timer) {
	if(!liveRecorder || !Timer || !Timer->Channel()) return NULL;
	if(!(liveRecorder->ChannelID() == Timer->Channel()->GetChannelID())) return NULL;
	if(!liveRecorder->SetBufferStart(Timer->StartTime())) return NULL;
	return liveRecorder->GetIndex();
} // cRecordControls::GetLiveBuffer

cIndex *cRecordControls::GetLiveIndex(const char *FileName) {
	if(!FileName || strcmp(cLiveRecorder::FileName(), FileName)) return NULL;
	return liveRecorder ? liveRecorder->GetIndex() : NULL;
} // cRecordControls::GetLiveIndex

double cRecordControls::GetLiveFramesPerSecond() {
	return liveRecorder ? liveRecorder->GetFramesPerSecond() : DEFAULTFRAMESPERSECOND;
} // cRecordControls::GetLiveFramesPerSecond

#endif /* USE_LIVEBUFFER */

#ifdef REELVDR
const char *cRecordControls::GetInstantId(const char *LastInstantId, bool LIFO)
{
  for (int i = LIFO?MAXRECORDCONTROLS-1:0; LIFO? i>=0:i < MAXRECORDCONTROLS; LIFO?--i:i++) {
#else
const char *cRecordControls::GetInstantId(const char *LastInstantId)
{
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
#endif /* REELVDR */
      if (RecordControls[i]) {
         if (!LastInstantId && RecordControls[i]->InstantId())
            return RecordControls[i]->InstantId();
         if (LastInstantId && LastInstantId == RecordControls[i]->InstantId())
            LastInstantId = NULL;
         }
      }
  return NULL;
}

cRecordControl *cRecordControls::GetRecordControl(const char *FileName)
{
  if (FileName) {
     for (int i = 0; i < MAXRECORDCONTROLS; i++) {
         if (RecordControls[i] && strcmp(RecordControls[i]->FileName(), FileName) == 0)
            return RecordControls[i];
         }
     }
  return NULL;
}

void cRecordControls::Process(time_t t)
{
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
      if (RecordControls[i]) {
         if (!RecordControls[i]->Process(t)) {
            DELETENULL(RecordControls[i]);
            ChangeState();
            }
         }
      }
}

#ifdef USE_MCLI
void cRecordControls::ChannelDataModified(cChannel *Channel, int chmod)
{
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
      if (RecordControls[i]) {
         if (RecordControls[i]->Timer() && RecordControls[i]->Timer()->Channel() == Channel) {
            if (RecordControls[i]->Device()->ProvidesTransponder(Channel) &&                // avoids retune on devices that don't really access the transponder
                ((CHANNELMOD_CA != chmod) || !((Channel->Ca() >= CA_MCLI_MIN) && (Channel->Ca() <= CA_MCLI_MAX) && cDevice::ActualDevice()->HasInternalCam()))) { // avoids retune if only ca has changed and the device has an internal cam
               isyslog("stopping recording due to%s%s%s%s%s%s modification of channel %d%s",
                       (chmod & CHANNELMOD_NAME  )?" NAME"  :"",
                       (chmod & CHANNELMOD_PIDS  )?" PIDS"  :"",
                       (chmod & CHANNELMOD_ID    )?" ID"    :"",
                       (chmod & CHANNELMOD_CA    )?" CA"    :"",
                       (chmod & CHANNELMOD_TRANSP)?" TRANSP":"",
                       (chmod & CHANNELMOD_LANGS )?" LANGS" :"",
                       Channel->Number(),
                       ((Channel->Ca() >= CA_MCLI_MIN) && (Channel->Ca() <= CA_MCLI_MAX) && cDevice::ActualDevice()->HasInternalCam())?" (InternalCam)":"");
               RecordControls[i]->Stop();
               // This will restart the recording, maybe even from a different
               // device in case conditional access has changed.
               ChangeState();
               }
            }
         }
      }
}
#else
void cRecordControls::ChannelDataModified(cChannel *Channel)
{
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
      if (RecordControls[i]) {
         if (RecordControls[i]->Timer() && RecordControls[i]->Timer()->Channel() == Channel) {
            if (RecordControls[i]->Device()->ProvidesTransponder(Channel)) { // avoids retune on devices that don't really access the transponder
               isyslog("stopping recording due to modification of channel %d", Channel->Number());
               RecordControls[i]->Stop();
               // This will restart the recording, maybe even from a different
               // device in case conditional access has changed.
               ChangeState();
               }
            }
         }
      }
}
#endif /*USE_MCLI*/

bool cRecordControls::Active(void)
{
  for (int i = 0; i < MAXRECORDCONTROLS; i++) {
      if (RecordControls[i])
         return true;
      }
  return false;
}

void cRecordControls::Shutdown(void)
{
  for (int i = 0; i < MAXRECORDCONTROLS; i++)
      DELETENULL(RecordControls[i]);
  ChangeState();
}

bool cRecordControls::StateChanged(int &State)
{
  int NewState = state;
  bool Result = State != NewState;
  State = state;
  return Result;
}

// --- cReplayControl --------------------------------------------------------

#ifdef USE_LIEMIEXT
#define REPLAYCONTROLSKIPLIMIT   9    // s
#define REPLAYCONTROLSKIPSECONDS 90   // s
#define REPLAYCONTROLSKIPTIMEOUT 5000 // ms
#endif /* LIEMIEXT */

cReplayControl *cReplayControl::currentReplayControl = NULL;
char *cReplayControl::fileName = NULL;
char *cReplayControl::title = NULL;
#if REELVDR
// the above fileName, title variables can hold timeshift buffer so
// hold the "real" last replayed recording title and filename in the
// following variables
cString cReplayControl::lastReplayedFileName = NULL;
cString cReplayControl::lastReplayedTitle = NULL;
#endif

cReplayControl::cReplayControl(void)
#ifdef USE_JUMPPLAY
:cDvbPlayerControl(fileName), marks(fileName)
#else
:cDvbPlayerControl(fileName)
#endif /* JUMPPLAY */
{
  currentReplayControl = this;
  displayReplay = NULL;
  visible = modeOnly = shown = displayFrames = false;
  lastCurrent = lastTotal = -1;
  lastPlay = lastForward = false;
  lastSpeed = -2; // an invalid value
#ifdef USE_LIEMIEXT
  lastSkipKey = kNone;
  lastSkipSeconds = REPLAYCONTROLSKIPSECONDS;
  lastSkipTimeout.Set(0);
#endif /* LIEMIEXT */
  timeoutShow = 0;
  timeSearchActive = false;
  cRecording Recording(fileName);
#ifdef REELVDR
  cStatus::MsgReplaying(this, Recording.Name() ? Recording.Name() : title, Recording.FileName() ? Recording.FileName() : fileName, true);
#else
  cStatus::MsgReplaying(this, Recording.Name(), Recording.FileName(), true);
#endif
#ifndef USE_JUMPPLAY
  marks.Load(fileName, Recording.FramesPerSecond(), Recording.IsPesRecording());
#endif /* JUMPPLAY */
#ifdef REELVDR
  jumpWidth = (Setup.JumpWidth * 60);
  // Initializing variables for intelligent Positioning
  m_LastKeyPressed = 0;
  m_LastKey = kNone;
  m_ActualStep = m_FirstStep = (2*60);
#endif
  SetTrackDescriptions(false);
}

cReplayControl::~cReplayControl()
{
  Hide();
#ifndef REELVDR
  cStatus::MsgReplaying(this, NULL, fileName, false);
#endif /*REELVDR*/
  Stop();
  if (currentReplayControl == this)
     currentReplayControl = NULL;
}

void cReplayControl::Stop(void)
{
#ifdef REELVDR
  cStatus::MsgReplaying(this, NULL, fileName, false);
#endif /*REELVDR*/
  if (Setup.DelTimeshiftRec && fileName) {
     cRecordControl* rc = cRecordControls::GetRecordControl(fileName);
     if (rc && rc->InstantId()) {
        if (Active()) {
           if (Setup.DelTimeshiftRec == 2 || Interface->Confirm(tr("Delete timeshift recording?"))) {
              cTimer *timer = rc->Timer();
              rc->Stop(false); // don't execute user command
              if (timer) {
                 isyslog("deleting timer %s", *timer->ToDescr());
                 Timers.Del(timer);
                 Timers.SetModified();
                 }
              cDvbPlayerControl::Stop();
              cRecording *recording = Recordings.GetByName(fileName);;
              if (recording) {
                 if (recording->Delete()) {
                    Recordings.DelByName(fileName);
                    ClearLastReplayed(fileName);
                    }
                 else
                    Skins.Message(mtError, tr("Error while deleting recording!"));
                 }
              return;
              }
           }
        }
     }
  cDvbPlayerControl::Stop();
}

void cReplayControl::SetRecording(const char *FileName, const char *Title)
{
  free(fileName);
  free(title);
  fileName = FileName ? strdup(FileName) : NULL;
  title = Title ? strdup(Title) : NULL;
#if REELVDR
  // check if the "recording" is not timeshift buffer being replayed
  if ( !(Title && strstr(Title, tr("Timeshift mode"))) )
  {
      lastReplayedFileName = fileName;
      lastReplayedTitle    = title;
  }
#endif
}

const char *cReplayControl::NowReplaying(void)
{
  return currentReplayControl ? fileName : NULL;
}

#ifdef REELVDR
const char *cReplayControl::NowReplayingTitle(void)
{
  return currentReplayControl ? title : NULL;
}

// returns the title of the last replayed "real" recording
const char* cReplayControl::LastReplayedTitle(void)
{
    return lastReplayedTitle;
}

#endif /*REELVDR*/

const char *cReplayControl::LastReplayed(void)
{
#if REELVDR
    // the filename of the last replayed "real" recording
    return lastReplayedFileName;
#else
  return fileName;
#endif
}

void cReplayControl::ClearLastReplayed(const char *FileName)
{
  if (fileName && FileName && strcmp(fileName, FileName) == 0) {
     free(fileName);
     fileName = NULL;
     }
}

void cReplayControl::ShowTimed(int Seconds)
{
  if (modeOnly)
     Hide();
  if (!visible) {
     shown = ShowProgress(true);
     timeoutShow = (shown && Seconds > 0) ? time(NULL) + Seconds : 0;
     }
}

void cReplayControl::Show(void)
{
  ShowTimed();
}

void cReplayControl::Hide(void)
{
  if (visible) {
     delete displayReplay;
     displayReplay = NULL;
     SetNeedsFastResponse(false);
     visible = false;
     modeOnly = false;
     lastPlay = lastForward = false;
     lastSpeed = -2; // an invalid value
     timeSearchActive = false;
     }
}

void cReplayControl::ShowMode(void)
{
  if (visible || (Setup.ShowReplayMode && !cOsd::IsOpen())) {
     bool Play, Forward;
     int Speed;
     if (GetReplayMode(Play, Forward, Speed) && (!visible || Play != lastPlay || Forward != lastForward || Speed != lastSpeed)) {
        bool NormalPlay = (Play && Speed == -1);
#if 0 //def REELVDR
        bool Paused = (!Play && Speed == -1);
#endif /*REELVDR*/

        if (!visible) {
           if (NormalPlay)
              return; // no need to do indicate ">" unless there was a different mode displayed before
           visible = modeOnly = true;

#if 0//def REELVDR
           // if newly paused show full replay osd; ie modeOnly = false
           if (Paused)  {
               modeOnly = (lastPlay == Play);
           }
#endif /*REELVDR*/
           displayReplay = Skins.Current()->DisplayReplay(modeOnly);
           }

#if 0//def REELVDR
        // osd times out when replaying normally OR when paused and full osd is shown
        if (!timeoutShow && (NormalPlay|| (!modeOnly && Paused)))
#else
        if (modeOnly && !timeoutShow && NormalPlay)
#endif
           timeoutShow = time(NULL) + MODETIMEOUT;

        displayReplay->SetMode(Play, Forward, Speed);
        lastPlay = Play;
        lastForward = Forward;
        lastSpeed = Speed;
        }
     }
}

bool cReplayControl::ShowProgress(bool Initial)
{
  int Current, Total;

  if (GetIndex(Current, Total) && Total > 0) {
#ifdef USE_LIVEBUFFER
     int first=0;
     cIndex *idx = cRecordControls::GetLiveIndex(fileName);
     if(idx) first = idx->First(); // Normalize displayed values
     Current -= first;
     if(Current < 0) Current = 0;
     Total   -= first;
     if(Total < 0) Total = 0;
     time_t now = time(NULL);
     static time_t last_sched_check = 0;
     if(displayReplay && idx && (last_sched_check != now)) {
        last_sched_check = now; // Only check every second
        cSchedulesLock SchedulesLock;
        const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);
        if (Schedules) {
           const char *display_title = NULL;// = title;
           const cSchedule *Schedule = Schedules->GetSchedule(Channels.GetByNumber(cDevice::CurrentChannel()));
           if (Schedule) {
              time_t Time = now - round(((double)Total - Current) / FramesPerSecond());
              const cEvent *event = Schedule->GetEventAround(Time);
              if (event) display_title = event->Title();
           } // if

           // no event title; show channel name
           if (!display_title) {
               cChannel *channel = Channels.GetByNumber(cDevice::CurrentChannel());
               display_title = channel->Name();
           }

           // set title as "Timeshift mode: <event title> "
           // OR "Timeshift mode: <channel name>"
           // if neither is possible leave title as such
           if (display_title)
               displayReplay->SetTitle(cString::sprintf("%s: %s",
                                                        tr("Timeshift mode"),
                                                        display_title));
        } // if
     } // if
#endif /*USE_LIVEBUFFER*/
     if (!visible) {
        displayReplay = Skins.Current()->DisplayReplay(modeOnly);
        displayReplay->SetMarks(&marks);
        SetNeedsFastResponse(true);
        visible = true;
        }
     if (Initial) {
        if (title)
           displayReplay->SetTitle(title);
#ifdef REELVDR
#ifdef USE_LIVEBUFFER
        if (strcmp(cLiveRecorder::FileName(), fileName) == 0)
            displayReplay->SetHelp(tr("Key$Record"), tr("Back"), tr("Forward"), tr("End")); // Timeshift on
        else
#endif
            displayReplay->SetHelp(tr("Jump"), tr("Back"), tr("Forward"), tr("End"));
#endif
        lastCurrent = lastTotal = -1;
        }
     if (Total != lastTotal) {
        displayReplay->SetTotal(IndexToHMSF(Total, false, FramesPerSecond()));
        if (!Initial)
           displayReplay->Flush();
        }
     if (Current != lastCurrent || Total != lastTotal) {
        displayReplay->SetProgress(Current, Total);
        if (!Initial)
           displayReplay->Flush();
        displayReplay->SetCurrent(IndexToHMSF(Current, displayFrames, FramesPerSecond()));
        displayReplay->Flush();
        lastCurrent = Current;
        }
     lastTotal = Total;
     ShowMode();
     return true;
     }
  return false;
}

void cReplayControl::TimeSearchDisplay(void)
{
  char buf[64];
  // TRANSLATORS: note the trailing blank!
  strcpy(buf, tr("Jump: "));
  int len = strlen(buf);
  char h10 = '0' + (timeSearchTime >> 24);
  char h1  = '0' + ((timeSearchTime & 0x00FF0000) >> 16);
  char m10 = '0' + ((timeSearchTime & 0x0000FF00) >> 8);
  char m1  = '0' + (timeSearchTime & 0x000000FF);
  char ch10 = timeSearchPos > 3 ? h10 : '-';
  char ch1  = timeSearchPos > 2 ? h1  : '-';
  char cm10 = timeSearchPos > 1 ? m10 : '-';
  char cm1  = timeSearchPos > 0 ? m1  : '-';
  sprintf(buf + len, "%c%c:%c%c", ch10, ch1, cm10, cm1);
  displayReplay->SetJump(buf);
}

void cReplayControl::TimeSearchProcess(eKeys Key)
{
#define STAY_SECONDS_OFF_END 10
  int Seconds = (timeSearchTime >> 24) * 36000 + ((timeSearchTime & 0x00FF0000) >> 16) * 3600 + ((timeSearchTime & 0x0000FF00) >> 8) * 600 + (timeSearchTime & 0x000000FF) * 60;
  int Current = int(round(lastCurrent / FramesPerSecond()));
  int Total = int(round(lastTotal / FramesPerSecond()));
  switch (Key) {
    case k0 ... k9:
         if (timeSearchPos < 4) {
            timeSearchTime <<= 8;
            timeSearchTime |= Key - k0;
            timeSearchPos++;
            TimeSearchDisplay();
            }
         break;
    case kFastRew:
    case kLeft:
    case kFastFwd:
    case kRight: {
         int dir = ((Key == kRight || Key == kFastFwd) ? 1 : -1);
         if (dir > 0)
            Seconds = min(Total - Current - STAY_SECONDS_OFF_END, Seconds);
         SkipSeconds(Seconds * dir);
         timeSearchActive = false;
         }
         break;
    case kPlay:
    case kUp:
    case kPause:
    case kDown:
    case kOk:
         Seconds = min(Total - STAY_SECONDS_OFF_END, Seconds);
#if REELVDR
         {
            bool Play, Forward;
            int Speed;
            bool Still=(Key == kDown || Key == kPause);
            if (GetReplayMode(Play, Forward, Speed) && !Play) Still=true;
            Goto(SecondsToFrames(Seconds, FramesPerSecond()), Still);
         }
#else
         Goto(SecondsToFrames(Seconds, FramesPerSecond()), Key == kDown || Key == kPause || Key == kOk);
#endif
         timeSearchActive = false;
         break;
    default:
         if (!(Key & k_Flags)) // ignore repeat/release keys
            timeSearchActive = false;
         break;
    }

  if (!timeSearchActive) {
     if (timeSearchHide)
        Hide();
     else
        displayReplay->SetJump(NULL);
     ShowMode();
     }
}

void cReplayControl::TimeSearch(void)
{
#ifdef USE_LIVEBUFFER
  if(cRecordControls::GetLiveIndex(fileName)) return;
#endif /*USE_LIVEBUFFER*/
  timeSearchTime = timeSearchPos = 0;
  timeSearchHide = false;
  if (modeOnly)
     Hide();
  if (!visible) {
     Show();
     if (visible)
        timeSearchHide = true;
     else
        return;
     }
  timeoutShow = 0;
  TimeSearchDisplay();
  timeSearchActive = true;
}

void cReplayControl::MarkToggle(void)
{
#ifdef USE_LIVEBUFFER
  if(cRecordControls::GetLiveIndex(fileName)) return;
#endif /*USE_LIVEBUFFER*/
  int Current, Total;
  if (GetIndex(Current, Total, true)) {
     cMark *m = marks.Get(Current);
     lastCurrent = -1; // triggers redisplay
     if (m)
        marks.Del(m);
     else {
        marks.Add(Current);
        ShowTimed(2);
        bool Play, Forward;
        int Speed;
#ifdef USE_JUMPPLAY
        if (GetReplayMode(Play, Forward, Speed) && !Play) {
#else
        if (GetReplayMode(Play, Forward, Speed) && !Play)
#endif /* JUMPPLAY */
           Goto(Current, true);
#ifdef USE_JUMPPLAY
           displayFrames = true;
           }
#endif /* JUMPPLAY */
        }
     marks.Save();
     }
}

void cReplayControl::MarkJump(bool Forward)
{
#ifdef USE_LIVEBUFFER
  if(cRecordControls::GetLiveIndex(fileName)) return;
#endif /*USE_LIVEBUFFER*/
  if (marks.Count()) {
     int Current, Total;
     if (GetIndex(Current, Total)) {
        cMark *m = Forward ? marks.GetNext(Current) : marks.GetPrev(Current);
        if (m) {
#ifdef USE_JUMPPLAY
           bool Play2, Forward2;
           int Speed;
           if (Setup.JumpPlay && GetReplayMode(Play2, Forward2, Speed) &&
               Play2 && Forward && m->Position() < Total - SecondsToFrames(3, FramesPerSecond())) {
              Goto(m->Position());
              Play();
              }
           else {
              //Goto(m->position, true);
              Goto(m->Position(), true);
              displayFrames = true;
              }
#else
           Goto(m->Position(), true);
           displayFrames = true;
#endif /* JUMPPLAY */
           }
        }
     }
}

void cReplayControl::MarkMove(bool Forward)
{
#ifdef USE_LIVEBUFFER
  if(cRecordControls::GetLiveIndex(fileName)) return;
#endif /*USE_LIVEBUFFER*/
  int Current, Total;
  if (GetIndex(Current, Total)) {
     cMark *m = marks.Get(Current);
     if (m) {
        displayFrames = true;
        int p = SkipFrames(Forward ? 1 : -1);
        cMark *m2;
        if (Forward) {
           if ((m2 = marks.Next(m)) != NULL && m2->Position() <= p)
              return;
           }
        else {
           if ((m2 = marks.Prev(m)) != NULL && m2->Position() >= p)
              return;
           }
        m->SetPosition(p);
        Goto(m->Position(), true);
        marks.Save();
        }
     }
}

void cReplayControl::EditCut(void)
{
#ifdef USE_LIVEBUFFER
  if(cRecordControls::GetLiveIndex(fileName)) return;
#endif /*USE_LIVEBUFFER*/
  if (fileName) {
     Hide();
     if (!cCutter::Active()) {
        if (!marks.Count())
           Skins.Message(mtError, tr("No editing marks defined!"));
        else if (!cCutter::Start(fileName))
           Skins.Message(mtError, tr("Can't start editing process!"));
        else
           Skins.Message(mtInfo, tr("Editing process started"));
        }
     else
        Skins.Message(mtError, tr("Editing process already active!"));
     ShowMode();
     }
}

void cReplayControl::EditTest(void)
{
#ifdef USE_LIVEBUFFER
  if(cRecordControls::GetLiveIndex(fileName)) return;
#endif /*USE_LIVEBUFFER*/
  int Current, Total;
  if (GetIndex(Current, Total)) {
     cMark *m = marks.Get(Current);
     if (!m)
        m = marks.GetNext(Current);
     if (m) {
#ifdef USE_JUMPPLAY
        if ((m->Index() & 0x01) != 0 && !Setup.PlayJump)
#else
        if ((m->Index() & 0x01) != 0)
#endif /* JUMPPLAY */
           m = marks.Next(m);
        if (m) {
           Goto(m->Position() - SecondsToFrames(3, FramesPerSecond()));
           Play();
           }
        }
     }
}

cOsdObject *cReplayControl::GetInfo(void)
{
  cRecording *Recording = Recordings.GetByName(cReplayControl::LastReplayed());
  if (Recording)
     return new cMenuRecording(Recording, false);
  return NULL;
}

#if REELVDR
void cReplayControl::Jump(eKeys xkey)
{
  // Reset step width after 30 seconds
  if (time(NULL) - m_LastKeyPressed > 30)
  {
      m_ActualStep = m_FirstStep;
  }
  else
  {
     // direction has changed -> reduce step width
      if (xkey != m_LastKey)
      {
          // min 4 seconds step remaining
          if (m_ActualStep > 4)
              m_ActualStep /= 4;
      }
  }

  // Now jump the computed step width.
  if (xkey==kYellow)
      SkipSeconds(m_ActualStep);
  else
      SkipSeconds(-1 * m_ActualStep);

  m_LastKeyPressed = time(NULL);
  m_LastKey = xkey;
}
#endif

eOSState cReplayControl::ProcessKey(eKeys Key)
{
  if (!Active())
     return osEnd;

#if REELVDR
    if (NORMALKEY(Key) == kChanUp || NORMALKEY(Key) == kChanDn) // donot switch channels when replaying a recording
        return osContinue;
#endif

  if (Key == kNone)
     marks.Update();
  if (visible) {
#if 0//def REELVDR
      if (Key != kNone /*&& !modeOnly*/ && timeoutShow) {
          printf("timeout reset +%d\n", MODETIMEOUT);
          timeoutShow = time(NULL) + MODETIMEOUT;
      }
#endif /*REELVDR*/
     if (timeoutShow && time(NULL) > timeoutShow) {
#if 0//def REELVDR
         printf("timed out \n");
#endif /*REELVDR*/
        Hide();
        ShowMode();
        timeoutShow = 0;
        }
     else if (modeOnly)
        ShowMode();
     else
        shown = ShowProgress(!shown) || shown;
     }
  bool DisplayedFrames = displayFrames;
  displayFrames = false;
  if (timeSearchActive && Key != kNone) {
     TimeSearchProcess(Key);
     return osContinue;
     }
  bool DoShowMode = true;
#if 0 //REELVDR
  bool DoShowFullOSD = false;
#endif

#ifdef USE_VOLCTRL
  if (Setup.LRVolumeControl && (!Setup.LRForwardRewind || (Setup.LRForwardRewind == 1 && !visible))) {
     switch (int(Key)) {
       // Left/Right volume control
       case kLeft|k_Repeat:
       case kLeft:
       case kRight|k_Repeat:
       case kRight:
            cRemote::Put(NORMALKEY(Key) == kLeft ? kVolDn : kVolUp, true);
            return osContinue;
            break;
       default:
            break;
       }
     }
#endif /* VOLCTRL */
#ifdef USE_LIVEBUFFER
  if (cRecordControls::GetLiveIndex(fileName) && (Key >= k0) && (Key <= k9))
      return osSwitchChannel;
#endif /*USE_LIVEBUFFER*/
  switch (int(Key)) {
    // Positioning:
#ifdef USE_LIVEBUFFER
    case kUp:      if(cRecordControls::GetLiveIndex(fileName)) {
                      cDevice::SwitchChannel(1);
                      return osEnd;
                   } // if
                   // NO break
#if REELVDR
                   else
                        MarkJump(true); break;
#endif /*REELVDR*/
    case kPlay:
#if 0 //REELVDR
  {

      bool playing, forwarding;
      int speed;
      // show full osd only if going from Paused to Play
      // therefore check if paused
      if (GetReplayMode(playing, forwarding, speed) && !playing)
          DoShowFullOSD = true;
  }
#endif /* REELVDR */
        timeoutShow = 3;  //RC TODO: does not timeout within 3sec but closes immediately.
                          //         Same when using ShowTimed(3). Bug?
        Play(); break;
    case kDown:    if(cRecordControls::GetLiveIndex(fileName)) {
                      cDevice::SwitchChannel(-1);
                      return osEnd;
                   } // if
                   // NO break
#if REELVDR
                   else
                        MarkJump(false); break;
#endif /*REELVDR*/
    case kPause:
        Pause();
        timeoutShow = 3;  //RC
#if 0//REELVDR
      DoShowFullOSD = true;
#endif /* REELVDR */
        break;
#else
    case kPlay:
    case kUp:      Play(); break;
    case kPause:
    case kDown:    Pause(); break;
#endif /*USE_LIVEBUFFER*/
    case kFastRew|k_Release:
    case kLeft|k_Release:
                   if (Setup.MultiSpeedMode) break;
    case kFastRew:
#if REELVDR
      Backward();
      Show();
      break;
    case kLeft:
    case kLeft|k_Repeat:
                   SkipSeconds( -5); break;
#else
    case kLeft:    Backward(); break;
#endif
    case kFastFwd|k_Release:
    case kRight|k_Release:
                   if (Setup.MultiSpeedMode) break;
    case kFastFwd:
#if REELVDR
      Forward(); //BS
      Show();
      break;
    case kRight:   bool playing, forwarding;
                   int speed;
                   if (GetReplayMode(playing, forwarding, speed) && !playing) {
                       Play();
                       break;
                   } // else: fall through
    case kRight|k_Repeat:
                   SkipSeconds( 5); break;
#else
    case kRight:   Forward(); break;
#endif
#ifdef USE_JUMPINGSECONDS
    case kGreen|k_Repeat:
                   SkipSeconds(-(Setup.JumpSecondsRepeat)); break;
    case kGreen:   SkipSeconds(-(Setup.JumpSeconds)); break;
    case k1|k_Repeat:
    case k1:       SkipSeconds(-Setup.JumpSecondsSlow); break;
    case k3|k_Repeat:
    case k3:       SkipSeconds( Setup.JumpSecondsSlow); break;
    case kYellow|k_Repeat:
                   SkipSeconds(Setup.JumpSecondsRepeat); break;
    case kYellow:  SkipSeconds(Setup.JumpSeconds); break;
#elif defined REELVDR
    case kGreen|k_Repeat:
    case kGreen:   if(cRecordControls::GetLiveIndex(fileName) && !(visible && !modeOnly)) return osUnknown;
                   Setup.JumpWidth ? SkipSeconds(-jumpWidth) : Jump(Key) ; break; // intelligent jump
                   //SkipSeconds(-jumpWidth); break;
    case kYellow|k_Repeat:
    case kYellow:  if(cRecordControls::GetLiveIndex(fileName) && !(visible && !modeOnly)) return osUnknown;
                   Setup.JumpWidth ? SkipSeconds(jumpWidth) : Jump(Key) ; break; // intelligent jump
                   //SkipSeconds(jumpWidth); break;
#else
    case kGreen|k_Repeat:
    case kGreen:   SkipSeconds(-60); break;
    case kYellow|k_Repeat:
    case kYellow:  SkipSeconds( 60); break;
#endif /* JUMPINGSECONDS */
#ifdef USE_LIEMIEXT
#ifndef USE_JUMPINGSECONDS
    case k1|k_Repeat:
    case k1:       SkipSeconds(-20); break;
    case k3|k_Repeat:
    case k3:       SkipSeconds( 20); break;
#endif /* JUMPINGSECONDS */
    case kPrev|k_Repeat:
    case kPrev:    if (lastSkipTimeout.TimedOut()) {
                      lastSkipSeconds = REPLAYCONTROLSKIPSECONDS;
                      lastSkipKey = kPrev;
                   }
                   else if (RAWKEY(lastSkipKey) != kPrev && lastSkipSeconds > (2 * REPLAYCONTROLSKIPLIMIT)) {
                      lastSkipSeconds /= 2;
                      lastSkipKey = kNone;
                   }
                   lastSkipTimeout.Set(REPLAYCONTROLSKIPTIMEOUT);
                   SkipSeconds(-lastSkipSeconds); break;
    case kNext|k_Repeat:
    case kNext:    if (lastSkipTimeout.TimedOut()) {
                      lastSkipSeconds = REPLAYCONTROLSKIPSECONDS;
                      lastSkipKey = kNext;
                   }
                   else if (RAWKEY(lastSkipKey) != kNext && lastSkipSeconds > (2 * REPLAYCONTROLSKIPLIMIT)) {
                      lastSkipSeconds /= 2;
                      lastSkipKey = kNone;
                   }
                   lastSkipTimeout.Set(REPLAYCONTROLSKIPTIMEOUT);
                   SkipSeconds(lastSkipSeconds); break;
#endif /* LIEMIEXT */
#ifdef USE_LIVEBUFFER
    case kRed:     if(cRecordControls::GetLiveIndex(fileName)) {

                       if (!(visible && !modeOnly)) return osUnknown;
                       else {} // fall through to case kRecord
                               // since Timeshift ON and replay OSD is shown
                       } // if
                       else { //timeshift off
                           TimeSearch();
                           break;
                       } // else
                       // No break
    case kRecord:  if(cRecordControls::GetLiveIndex(fileName)) {
                      int frames = 0;
                      int Current, Total;
                      if(GetIndex(Current, Total))
                         frames = Total-Current;
                      cTimer *timer = new cTimer(true, false, Channels.GetByNumber(cDevice::CurrentChannel()), frames / FramesPerSecond());
                      Timers.Add(timer);
                      Timers.SetModified();
                      if (cRecordControls::Start(timer))
                         Skins.Message(mtInfo, tr("Recording started"));
                      else
                         Timers.Del(timer);
                   } // if
                   break;
    case kBlue:    if(cRecordControls::GetLiveIndex(fileName))
                       if(!(visible && !modeOnly))
                           return osUnknown;
                   //NO break
    case kStop:    Hide();
                   Stop();
                   return osEnd;
#else
    case kRed:     TimeSearch(); break;
    case kStop:
    case kBlue:    Hide();
                   Stop();
                   return osEnd;
#endif /*USE_LIVEBUFFER*/

    default: {
      DoShowMode = false;
      switch (int(Key)) {
        // Editing:
        case kMarkToggle:      MarkToggle(); break;
#ifndef USE_LIEMIEXT
        case kPrev|k_Repeat:
        case kPrev:
#endif /* LIEMIEXT */
#if REELVDR && ! USE_LIVEBUFFER
        case kDown:
#endif
        case kMarkJumpBack|k_Repeat:
        case kMarkJumpBack:    MarkJump(false); break;
#ifndef USE_LIEMIEXT
        case kNext|k_Repeat:
        case kNext:
#endif /* LIEMIEXT */
#if REELVDR && ! USE_LIVEBUFFER
        case kUp:
#endif
        case kMarkJumpForward|k_Repeat:
        case kMarkJumpForward: MarkJump(true); break;
        case kMarkMoveBack|k_Repeat:
        case kMarkMoveBack:    MarkMove(false); break;
        case kMarkMoveForward|k_Repeat:
        case kMarkMoveForward: MarkMove(true); break;
        case kEditCut:         EditCut(); break;
        case kEditTest:        EditTest(); break;
        default: {
          displayFrames = DisplayedFrames;
          switch (Key) {
            // Menu control:
            case kOk:      if (visible && !modeOnly) {
                              Hide();
                              DoShowMode = true;
                              }
                           else
                              Show();
                           break;
            case kBack:
#ifdef REELVDR
                           if (visible && !modeOnly) {
                              Hide();
                              DoShowMode = true;
                              break;
                           }
#endif /*REELVDR*/
#ifdef USE_LIVEBUFFER
                           if(cRecordControls::GetLiveIndex(fileName)) {
                              Hide();
                              Stop();
                              return osEnd;
                           } // if
#endif /*USE_LIVEBUFFER*/
                           if (Setup.DelTimeshiftRec) {
                              cRecordControl* rc = cRecordControls::GetRecordControl(fileName);
                              return rc && rc->InstantId() ? osEnd : osRecordings;
                              }
                           return osRecordings;
            default:       return osUnknown;
            }
          }
        }
      }
    }

#if 0//REELVDR

// if full osd already ON then just change mode (if DoShowMode is off)
// if full osd is not shown then show full
  if (DoShowFullOSD) {
    // already full osd on?
      if (visible && !modeOnly) {
          printf("Full osd on\n");
          if (!DoShowMode) /* if DoShowMode==true, then mode shown below */
              ShowMode();
      } else {
          printf("Full osd NOT on\n");
          Hide();
          ShowTimed(MODETIMEOUT);
      } // else
  } // DoShowFullOSD

#endif /* REELVDR */


  if (DoShowMode)
     ShowMode();

  return osContinue;
}

