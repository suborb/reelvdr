#ifndef RECORDING_INFO_MENU_H
#define RECORDING_INFO_MENU_H

#include <vdr/recording.h>
#include <vdr/osdbase.h>
#include <vdr/status.h>
#include <vdr/remote.h>

class cRecordingInfoMenu : public cOsdMenu 
{
    private:
        const cRecording *recording;
        bool withButtons;
        bool copy_;
    public:
        cRecordingInfoMenu(const cRecording *Recording, bool WithButtons = false, bool cpy=true);
        ~cRecordingInfoMenu();
        virtual void Display(void);
        virtual eOSState ProcessKey(eKeys Key);
};

#endif

