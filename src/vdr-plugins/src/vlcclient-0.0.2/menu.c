
#include "menu.h"
#include "common.h"
#include "i18n.h"
#include "remote.h"
#include "patchfont.h"
#include "socket.h"
#include "setup.h"
#include "player.h"

#include "vdr/debug.h"

cRemoteRecordings cVlcClientMenu::Recordings;
int cVlcClientMenu::HelpKeys = -1;

cVlcClientMenu::cVlcClientMenu(cRecordingsListThread *listThread, const char *Base, int Level, bool OpenSubMenus): cOsdMenu(Base ? Base : tr("Remote VLC"), 6, 6) {
  m_Base = Base ? m_Base : strdup(VlcClientSetup.StartDir);
  m_Level = Setup.RecordingDirs ? Level : -1;
#if 0
  // patch font
  if(!strcmp(Setup.OSDSkin, "Reel") || Setup.UseSmallFont==2)
   PatchFont(fontSml);
  else
   PatchFont(fontOsd);
#endif
  Clear();
  STATUS(tr("Fetching recordings"));
  FLUSH();
  if (!listThread->Load(this, &Recordings, m_Base))
	  isyslog("VlcClient error: Load-Thread already running.");
}

void cVlcClientMenu::Set()
{
  int current = Current();
  if (Recordings.Count() > 0) {
    cVlcClientMenuRecordingItem *LastItem = NULL;
    char *LastItemText = NULL;
    for (cRemoteRecording *r = Recordings.First(); r; r = Recordings.Next(r)) {
        cVlcClientMenuRecordingItem *Item = new cVlcClientMenuRecordingItem(r, m_Level);
        if (*Item->Text() && (!LastItem || strcmp(Item->Text(), LastItemText) != 0)) {
          Add(Item);
          LastItem = Item;
          free(LastItemText);
          LastItemText = strdup(LastItem->Text());
        } else
          delete Item;

        if (LastItem) {
          if (LastItem->IsDirectory())
            LastItem->IncrementCounter(r->IsNew());
        }
    }
    free(LastItemText);
    if (Current() < 0)
      SetCurrent(First());
    //else if (Open(true)) //TB: ????
    SetCurrent(Get(current));
    Display();
    return;
  } else {
    STATUS(tr("Error fetching recordings"));
    CloseSubMenu();
    FLUSH();
  }

  //STATUS(NULL);
  //FLUSH();

  SetHelpKeys();
}

cVlcClientMenu::~cVlcClientMenu() {
  HelpKeys = -1;
}

void cVlcClientMenu::SetHelpKeys(void) {
  cVlcClientMenuRecordingItem *ri =(cVlcClientMenuRecordingItem*)Get(Current());
  int NewHelpKeys = HelpKeys;
  if (ri) {
    if (ri->IsDirectory())
      NewHelpKeys = 1;
    else {
       NewHelpKeys = 0;
    }
  }
  if (NewHelpKeys != HelpKeys) {
    switch (NewHelpKeys) {
      case 0: SetHelp(tr("Play")); break;
      case 1:
        SetHelp(tr("Open")); break;
      default:
        break;
    }
    HelpKeys = NewHelpKeys;
  }
}

bool cVlcClientMenu::Open(cVlcClientMenuRecordingItem *ri, bool OpenSubMenus) {
  free(m_Base);
  m_Base = strdup(ri->FileName());
  Set();
  return true;
}

eOSState cVlcClientMenu::ProcessKey(eKeys Key) {
       bool HadSubMenu = HasSubMenu();
       eOSState state = cOsdMenu::ProcessKey(Key);

       if (state == osUnknown) {
                switch (Key) {
                        case kOk:
                        case kRed:    return Select();
                 default:      break;
             }
       }

        if (Key == kYellow && HadSubMenu && !HasSubMenu()) {
               cOsdMenu::Del(Current());
                        if (!Count())
                        return osBack;
                Display();
        }

        if (!HasSubMenu() && Key != kNone)
                SetHelpKeys();
        return state;
}


cRemoteRecording *cVlcClientMenu::GetRecording(cVlcClientMenuRecordingItem *Item) {
  DDD("looking for %s\n", Item->FileName());
  cRemoteRecording *recording = Recordings.GetByName(Item->FileName());
  if (!recording)
    esyslog(tr("Error while accessing recording!"));
  return recording;
}

eOSState cVlcClientMenu::Select(void) {
  cVlcClientMenuRecordingItem *ri  = (cVlcClientMenuRecordingItem*)Get(Current());

  if (ri) {
    if (ri->IsDirectory()){
      printf("DDD: open %s\n", ri->FileName());
      Open(ri);
    } else {
      ClientSocket.ClearVLCPlaylist();
      ClientSocket.AddToVLCPlaylist(ri->FileName());
      ClientSocket.SetVLCSoutMode();
      cControl::Launch(new cVlcControl(ri->FileName()));
      printf("DDD: play: %s\n", ri->FileName());
      return osEnd;
    }
  } else {
  //cControl::Launch(new cVlcClientPlayerControl(ri->FileName()));
  return osEnd;
  } //XXX
  return osContinue;
}


// --- cVlcClientMenuRecordingItem --------------------------------------

cVlcClientMenuRecordingItem::cVlcClientMenuRecordingItem(cRemoteRecording *Recording, int Level) {
        m_FileName = strdup(Recording->Name());
        m_Name = NULL;
	m_Directory = Recording->IsDirectory();
	if(m_Directory){
	char *m_Title = NULL;
	asprintf(&m_Title, "%c  %s", 130, Recording->Title('\t', true, Level));
	SetText(m_Title);
        } else
        SetText(Recording->Title('\t', true, Level));
        /*if (*Text() == '\t')
                m_Name = strdup(Text() + 2);*/
}

cVlcClientMenuRecordingItem::~cVlcClientMenuRecordingItem() {
}

void cVlcClientMenuRecordingItem::IncrementCounter(bool New) {
      /*  char *buffer = NULL;
        asprintf(&buffer, "%s", m_Name);
        SetText(buffer, false);*/
}

