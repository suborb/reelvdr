#include "recordingInfoMenu.h"


cRecordingInfoMenu::cRecordingInfoMenu(const cRecording *Recording, bool WithButtons, bool cp)
    :cOsdMenu(tr("Recording info"))
{
    copy_ = cp;

    if (copy_)
        recording = new cRecording(Recording->FileName(), Recording->BaseDir());
    else
        recording = Recording;

    withButtons = WithButtons;
    if (withButtons)
        SetHelp(tr("Button$Play"), tr("Button$Rewind"));
}

cRecordingInfoMenu::~cRecordingInfoMenu()
{
    if(copy_)
        delete recording;
}

void cRecordingInfoMenu::Display(void)
{
    cOsdMenu::Display();
    DisplayMenu()->SetRecording(recording);
    if (recording->Info()->Description())
        cStatus::MsgOsdTextItem(recording->Info()->Description());
}

eOSState cRecordingInfoMenu::ProcessKey(eKeys Key)
{
    switch (Key) 
    {
        case kUp|k_Repeat:
        case kUp:
        case kDown|k_Repeat:
        case kDown:
        case kLeft|k_Repeat:
        case kLeft:
        case kRight|k_Repeat:
        case kRight:
            DisplayMenu()->Scroll(NORMALKEY(Key) == kUp || NORMALKEY(Key) == kLeft, NORMALKEY(Key) == kLeft || NORMALKEY(Key) == kRight);
            cStatus::MsgOsdTextItem(NULL, NORMALKEY(Key) == kUp);
            return osContinue;
        default: break;
    }

    eOSState state = cOsdMenu::ProcessKey(Key);

    if (state == osUnknown) 
    {
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

