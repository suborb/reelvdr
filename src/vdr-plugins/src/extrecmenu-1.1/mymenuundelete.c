/*
 * See the README file for copyright information and how to reach the author.
 */

#include <vdr/interface.h>
#include <vdr/videodir.h>
#include <vdr/status.h>
#include <vdr/plugin.h>
#include <vdr/cutter.h>
#include "extrecmenu.h"
#include "myreplaycontrol.h"
#include "mymenuundelete.h"
#include "mymenusetup.h"
#include "mymenucommands.h"
#include "patchfont.h"
#include "tools.h"



// --- cMenuUndelete -------------------------------------------------------
#define MB_PER_MINUTE 25.75 // this is just an estimate!

cMenuUndelete::cMenuUndelete(const char *Base, int Level, bool OpenSubMenus)
 :cOsdMenu(Base?Base:""),lockFile(NULL) //tr("Undelete Recordings"))
{
    if(Level==0) {
        SetFreeSpaceTitle();
        lockFile = new cLockFile(VideoDirectory);
    }

    level = Level;
    base = Base ? strdup(Base) : NULL;

    DeletedRecordings.StateChanged(recordingsState);
    helpKeys = -1;
    Display(); // this keeps the higher level menus from showing up briefly when pressing 'Back' during replay

    // wenn du fertig bist nochmal die änderungen an den schreiber dieses plugins schicken 
    Set();
    if (Current() < 0)
        SetCurrent(First());
    else if (OpenSubMenus && Open(true))
        return;

    Display();
    SetHelpKeys();
}

cMenuUndelete::~cMenuUndelete()
{
    if (base)
      free(base);

    if (level==0)
    {
        if (Setup.UseSmallFont==2)
            cFont::SetFont(fontSml);
        else
            cFont::SetFont(fontOsd);
    }

    delete lockFile;
}

// create the menu list
void cMenuUndelete::Set(bool Refresh)
{
    SetCols(7,6);
    const char *CurrentRecording = NULL;

    cMenuRecordingItem *LastItem = NULL;
    char *LastItemText = NULL;

    // bitte überprüfen !
    cThreadLock DeleteRecordingsLock(&DeletedRecordings);

    if (Refresh) {
     cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
     if (ri) {
        cRecording *Recording = DeletedRecordings.GetByName(ri->FileName());
        if (Recording) {
           CurrentRecording = Recording->FileName();
           }
        }
     }

    Clear();
     
    // create my own recordings list from VDR's

    for (cRecording *recording = DeletedRecordings.First(); recording; recording = DeletedRecordings.Next(recording)) {
        if (!base || (strstr(recording->Name(), base) == recording->Name() && recording->Name()[strlen(base)] == '~')) {
            cMenuRecordingItem *Item = new cMenuRecordingItem(recording, level);
            if (*Item->Text() && (!LastItem || strcmp(Item->Text(), LastItemText) != 0)) {
                Add(Item);
                LastItem = Item;
                LastItemText = strdup(LastItem->Text()); // must use a copy because of the counters!
            }
            else
                delete Item;
            if (LastItem) {
                if (LastItem->IsDirectory())
                    LastItem->IncrementCounter(recording->IsNew());
            }
        }
    }
    if (LastItemText)
        free(LastItemText);

    if (Count() <=0)
        Add(new cOsdItem(tr("No recordings to undelete!"), osUnknown , true));
    if (Refresh)
        Display();
}

void cMenuUndelete::SetFreeSpaceTitle()
{
    int freemb;
    VideoDiskSpace(&freemb);
    int minutes=int(double(freemb)/MB_PER_MINUTE);
    char buffer[40];
    snprintf(buffer,sizeof(buffer),"%s - %2d:%02dh %s",tr("Undelete Recordings"),minutes/60,minutes%60,tr("free"));
    cOsdMenu::SetTitle(buffer);
}

void cMenuUndelete::SetHelpKeys()
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
  if (!DeletedRecordings.Count())
      NewHelpKeys = 0;
  if (NewHelpKeys != helpKeys) {
     switch (NewHelpKeys) {
       case 0: SetHelp(NULL); break;
       case 1: SetHelp(tr("Button$Open")); break;
       case 2: SetHelp(NULL, tr("Button$Undelete"), tr("Button$Remove")); break;
       case 3: SetHelp(NULL, tr("Button$Undelete"), tr("Button$Remove"), tr("Button$Info"));
       }
     helpKeys = NewHelpKeys;
     }
}


// returns the corresponding recording to an item
cRecording *cMenuUndelete::GetRecording(cMenuRecordingItem *Item)
{
    cRecording *recording=DeletedRecordings.GetByName(Item->FileName());
    if(!recording) {
        cOsdMenu::Del(Current());
        esyslog ("\033[0;44m %s  No Recording %s \033[0m\n", __PRETTY_FUNCTION__, Item->FileName());
    }
    return recording;
}

// opens a subdirectory
bool cMenuUndelete::Open(bool OpenSubMenus)
{
    cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
    if (ri && ri->IsDirectory()) {
        const char *t = ri->Name();
        char *buffer = NULL;
        if (base) {
            asprintf(&buffer, "%s~%s", base, t);
            t = buffer;
        }
        AddSubMenu(new cMenuUndelete(t, level + 1, OpenSubMenus));
        free(buffer);
        return true;
    }
    return false;

}

// delete a recording
eOSState cMenuUndelete::Remove()
{
    if (HasSubMenu() || Count() == 0)
        return osContinue;

    cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
    if (ri && !ri->IsDirectory()) {
        if (Interface->Confirm(tr("Remove recording from disk?"))) {
            Skins.Message(mtInfo, tr("Removing recording ..."));
            cRecording *recording = GetRecording(ri);
            if (recording) {
                if (recording->Remove()) {
                    cReplayControl::ClearLastReplayed(ri->FileName());
                    DeletedRecordings.Del(recording);
                    cOsdMenu::Del(Current());
                    SetHelpKeys();
                    Display();
                    if (!Count()){
                        if (level == 0)
                            Set(true);
                        else  
                            return osBack; // avoid empty directory
                    }
                }
                else
                    Skins.Message(mtError, tr("Error while deleting recording!"));
            }
        }
    }
 
    return osContinue;
}


eOSState cMenuUndelete::Undelete()
{

    if (HasSubMenu() || Count() == 0)
        return osContinue;

    cRecording *newRecording = NULL;
    cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
    if (ri && !ri->IsDirectory()) {
        cRecording *recording = GetRecording(ri);
        cReplayControl::ClearLastReplayed(ri->FileName());

        newRecording = new cRecording(recording->FileName());
        DeletedRecordings.Del(recording); // ???
        //DeletedRecordings.DelByName(ri->FileName()) ; // remove from list
        cOsdMenu::Del(Current()); // remove item from menu

        if (!newRecording->Undelete()) { // change ending  .del to .rec
            Skins.Message(mtError, tr("Sorry, undeleting recording failed"));
         }
        else {
            Recordings.Add(newRecording);
        }

        SetHelpKeys();
        Display();
        if (!Count()){
            if (level == 0)
                Set(true);
            else
                return osBack; // avoid empty directory
        }
        //Set(true);
    }
    return osContinue;
}

// renames a recording

// edit details of a recording

// opens an info screen to a recording

// --- myMenuUndeleteInfo ----------------------------------------------------

myMenuUndeleteInfo::myMenuUndeleteInfo(const cRecording *Recording)
 :cOsdMenu(tr("Recording info"))
{
 recording=Recording;
 SetHelp(NULL,NULL,NULL,tr("Button$Back"));
}

void myMenuUndeleteInfo::Display(void)
{
 cOsdMenu::Display();
 DisplayMenu()->SetRecording(recording);
 cStatus::MsgOsdTextItem(recording->Info()->Description());
}

eOSState myMenuUndeleteInfo::ProcessKey(eKeys Key)
{
 switch(Key)
 {
  case kUp|k_Repeat:
  case kUp:
  case kDown|k_Repeat:
  case kDown:
  case kLeft|k_Repeat:
  case kLeft:
  case kRight|k_Repeat:
  case kRight: DisplayMenu()->Scroll(NORMALKEY(Key)==kUp||NORMALKEY(Key)==kLeft,NORMALKEY(Key)==kLeft||NORMALKEY(Key)==kRight);
               cStatus::MsgOsdTextItem(NULL, NORMALKEY(Key) == kUp);
               return osContinue;
  default: break;
 }

 eOSState state=cOsdMenu::ProcessKey(Key);
 if(state==osUnknown)
 {
  switch(Key)
  {
   case kBlue:
   case kOk: return osBack;
   default: break;
  }
 }
 return state;
}

eOSState cMenuUndelete::Info()
{
    if (HasSubMenu() || Count() == 0)
        return osContinue;
    cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
    if (ri && !ri->IsDirectory()) {
        cRecording *recording = GetRecording(ri);
        if (recording && recording->Info()->Title())
            return AddSubMenu(new myMenuUndeleteInfo(recording));
    }
    return osContinue;
}

eOSState cMenuUndelete::ProcessKey(eKeys Key)
{
  bool hadSubMenu = HasSubMenu();

  eOSState state = cOsdMenu::ProcessKey(Key);
  if ((state == osUnknown) && (DeletedRecordings.Count())) {
     switch (Key) {
       case kOk:     return Play();  // implizit Open
       //case kRed:    return if (helpKeys ==  1) Open(); // ???
       case kRed:    if (helpKeys > 1)  return Play(); break;
       case kGreen:  return Undelete();
       case kYellow: return Remove(); // remove instantly from disk
       case kBlue:   return Info();
       case kNone:   if (DeletedRecordings.StateChanged(recordingsState)) {
                         Set(true);
                         break;
                     }
       default: break;
       }
     }
  if (Key == kGreen && hadSubMenu && !HasSubMenu()) {
     // the last recording in a subdirectory was deleted, so let's go back up
     cOsdMenu::Del(Current());
     SetHelpKeys();
     Set(true);

     }
  if (!HasSubMenu() && Key != kNone)
     SetHelpKeys();

  return state;
}


eOSState cMenuUndelete::Play(void)
{
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri) {
     if (cStatus::MsgReplayProtected(GetRecording(ri), ri->Name(), base,
                                     ri->IsDirectory()) == true)    // PIN PATCH
        return osContinue;                                          // PIN PATCH
     if (ri->IsDirectory())
        Open();
#if 0
     else {
        cRecording *recording = GetRecording(ri);
        if (recording) {
           cReplayControl::SetRecording(recording->FileName(), recording->Title());
           return osReplay;
           }
        }
#endif
  }
  return osContinue;
}


