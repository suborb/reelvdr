#ifndef MENUSEARCHRECORDINGS_H
#define MENUSEARCHRECORDINGS_H

#include <vdr/recording.h>
#include <vdr/osdbase.h>
#include <vdr/tools.h>

enum VideoDefinitionType{ eAll, eSDOnly, eHDOnly};
struct SearchParams
{
    char name[64];
    int videoDefinition; // any/SD/HD
    int latestMonth; // 0: all recordings, 1: recordings of the last 30 days

    SearchParams();
    void Init();

    SearchParams& operator=(const SearchParams& rhs);
    bool operator==(const SearchParams& rhs)const;
    bool operator!=(const SearchParams& rhs)const;

};


bool DeleteRecording(cRecording*);


class cMenuSearchRecordings : public cOsdMenu
{
private:
   static SearchParams searchParams;
   static cString lastPlayedFile;

protected:
    bool IsAllowed(const cRecording*);
    void ShowSideNoteInfo();

public:
    cMenuSearchRecordings();
    eOSState ProcessKey(eKeys Key);
    void Set();
    void SetHelpKeys();

    cRecording* CurrentRecording() const;
    bool PlayRecording(const cRecording*) const;

    void Display();

    // ask user to confirm deletion of recording, and Warn them if the recording is being recorded currently
    bool UserConfirmedDeleteRecording(cRecording*) const;
};



class cOsdRecordingItem : public cOsdItem
{
private:
    cRecording *recording;
public:
    cOsdRecordingItem(cRecording *Recording);
    void Set();
    cRecording * Recording();

    int Compare(const cListObject &ListObject) const;
};

#endif // MENUSEARCHRECORDINGS_H
