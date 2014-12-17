#include "MenuSearchRecordings.h"
#include <vdr/menuitems.h>
#include <vdr/interface.h>
#include "myreplaycontrol.h"
#include "mymenurecordings.h"
#include <time.h>

const char *latestMonthStrs[] = {
    tr("All recordings"),
    tr("last 30 days")
};

const char *videoDefStrs[] = {
    tr("Both SD and HD"),
    tr("SD only"),
    tr("HD Only")
};



SearchParams::SearchParams()
{
    Init();
}

void SearchParams::Init()
{
    name[0] = 0;
    videoDefinition = latestMonth = 0;
}

SearchParams& SearchParams::operator=(const SearchParams& rhs)
{
    strncpy(name, rhs.name, 63); name[63] = 0 ;

    videoDefinition = rhs.videoDefinition;
    latestMonth     = rhs.latestMonth;

    return *this;
}

bool SearchParams::operator==(const SearchParams& rhs)const
{
    return !strcasecmp(name, rhs.name) &&
            videoDefinition == rhs.videoDefinition &&
            latestMonth == rhs.latestMonth;
}

bool SearchParams::operator!=(const SearchParams& rhs) const
{
    return !(*this==rhs);
}


//
// cOsdRecordingItem  ----------------------------------------------------------
//

cOsdRecordingItem::cOsdRecordingItem(cRecording *Recording)
    : recording(Recording)
{
    Set();
}

cRecording* cOsdRecordingItem::Recording()
{
    return recording;
}

void cOsdRecordingItem::Set()
{
    cString name = recording->Name();
    const cRecordingInfo *info = recording->Info();

    if (info && info->Title())
    {
        const char *shortText = info->ShortText();
        const char *title = info->Title();

        if (!shortText)
            name = title;
        else
            name = cString::sprintf("%s ~ %s", title, shortText);
    } // if

    // date of recording
    time_t startTime = recording->Start();
    struct tm tm_r;
    struct tm *t=localtime_r(&startTime, &tm_r);
    char tstr[16];
    strftime(tstr, sizeof(tstr), "%d.%m.%y", t);

    int icon = 0;
    if (recording->IsCompressed())
        icon = 148;
    else if (recording->IsNew() && recording->IsHD())
        icon = 144;
    else if (recording->IsNew())
        icon = 132;
    else if (recording->IsHD())
        icon = 134;

    if(icon)
        name = cString::sprintf("%c %s   %s", (char)icon, tstr, *name);
    else
        name = cString::sprintf("      %s   %s", tstr, *name);
    // 6 space characters in the beginning, to make sure the non-iconed item
    // is aligned with the icon-ed item

    SetText(name);
}


int cOsdRecordingItem::Compare(const cListObject &ListObject) const
{
        cOsdRecordingItem *item = (cOsdRecordingItem*) (&ListObject);
        if (recording->Start() == item->Recording()->Start())
            return 0;

        return recording->Start() < item->Recording()->Start(); // latest recording first
}


//
// cMenuSearchRecordings
//

SearchParams cMenuSearchRecordings::searchParams;
cString cMenuSearchRecordings::lastPlayedFile = "";

cMenuSearchRecordings::cMenuSearchRecordings() : cOsdMenu(tr("Search Recordings"))
{
#if REELVDR && APIVERSNUM >= 10718
    EnableSideNote(true);
#endif

    Set();
}

void cMenuSearchRecordings::Set()
{
    int curr = Current();
    Clear();
    SetCols(20);

    Add(new cMenuEditStrItem(tr("Recording Name"), searchParams.name, 63,
                             NULL, tr("All recordings")));
    Add(new cMenuEditStraItem(tr("Time"), &searchParams.latestMonth, 2,
                             latestMonthStrs));
/*    Add(new cMenuEditBoolItem(tr("Recording only from last 30 days"), &searchParams.latestMonth)); */

    Add(new cMenuEditStraItem(tr("Filter by"), &searchParams.videoDefinition, 3,
                              videoDefStrs));

    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem(tr("Select a recording to play"), osUnknown, false));

    cRecording *rec = Recordings.First();

    int count = 0;
    cOsdRecordingItem *recItem = NULL;
    cOsdItem *lastPlayedItem = NULL;
    for (; rec; rec = Recordings.Next(rec))
    {
        if (IsAllowed(rec))
        {
            count++;
            Add(recItem = new cOsdRecordingItem(rec));
            if (strcmp(rec->FileName(), lastPlayedFile)==0)
            {
                lastPlayedItem = recItem;
                printf(" --------------  last played '%s'\n", *lastPlayedFile);
            }
        } // if
    } // for

    if (count)
    {
        Sort();
        SetCurrent(lastPlayedItem); // must be set only when osd is drawn for the first time
    }
    else {
        // delete the info text "Select a recording to play" because there is no recording
        Del(Count()-1);

        AddFloatingText(tr("No recordings found.\nModify search parameters and try again."), 45);
    }

    // when the menu is redrawn keep the current item
    if (Get(curr))
        SetCurrent(Get(curr));

    Display();
    SetHelpKeys();
}

void  cMenuSearchRecordings::SetHelpKeys()
{
    cRecording *rec = CurrentRecording();

    if (!rec) {
        // still not the correct behaviour
        // help keys are not shown after the user exits the
        // edit mode of cMenuEditStrItem
        cMenuEditStrItem *item = dynamic_cast<cMenuEditStrItem*>(Get(Current()));
        if (item)
            ; // do nothing
        else
            SetHelp(NULL, tr("New Search"));
        return;
    }

    static eKeys info_keys[2] = { kInfo, kNone};
    SetHelp(tr("Delete"), tr("New Search"), NULL, NULL, info_keys);
}

bool cMenuSearchRecordings::PlayRecording(const cRecording * rec) const
{
    if (!rec) return false;

    myMenuRecordings::golastreplayed=true;
    cControl::Shutdown();
#if VDRVERSNUM < 10728
    myReplayControl::SetRecording(rec->FileName(), rec->Title());
#else
    myReplayControl::SetRecording(rec->FileName());
#endif
    cControl::Launch(new myReplayControl());
    lastPlayedFile = rec->FileName();
    return true;
}


cRecording* cMenuSearchRecordings::CurrentRecording() const
{
    cOsdRecordingItem *item = dynamic_cast<cOsdRecordingItem*>(Get(Current()));

    if (item)
        return item->Recording();
    return NULL;
}

void cMenuSearchRecordings::ShowSideNoteInfo()
{
#if REELVDR && APIVERSNUM >= 10718

    if (!HasSubMenu()) {
            const cRecording* rec = CurrentRecording();
            SideNote(rec);
            SideNoteIcons(RecordingsIcons(rec).c_str());
    } // if

#endif /* REELVDR && APIVERSNUM >= 10718 */
}

void cMenuSearchRecordings::Display()
{
    cOsdMenu::Display();
    ShowSideNoteInfo();
}


bool DeleteRecording(cRecording* rec)
{
    if (!rec) return false;

    if (!rec->Delete()) return false;

    cRecordingUserCommand::InvokeCommand("delete",rec->FileName());
    cReplayControl::ClearLastReplayed(rec->FileName());

    Recordings.DelByName(rec->FileName());
    return true;
}

bool cMenuSearchRecordings::UserConfirmedDeleteRecording(cRecording *rec) const
{
    if (!rec) return false;

    bool result = Interface->Confirm(tr("Delete recording?"));
    if (!result) return false;

    // Check if recording is in progress
    cRecordControl *rc = cRecordControls::GetRecordControl(rec->FileName());

    if (rc) {
        result = Interface->Confirm(tr("Recording in progress, stop recording and delete it?"));
        if (!result) return false;

        cTimer *timer=rc->Timer();
        if(timer) {
            timer->Skip();
            cRecordControls::Process(time(NULL));
            if(timer->IsSingleEvent()) {
                isyslog("deleting timer %s",*timer->ToDescr());
                if(!Timers.Del(timer))
                    Skins.Message(mtError, trVDR("Couldn't delete timer"));
            } // if single event
            Timers.SetModified();
        } // if timer
    } // if rc

    return true;
}
eOSState cMenuSearchRecordings::ProcessKey(eKeys Key)
{
    /* if input has changed, act on it and refresh menu */
    SearchParams wasSearchParam = searchParams;

    eOSState state = cOsdMenu::ProcessKey(Key);

    if(HasSubMenu()) return state;

    if (wasSearchParam != searchParams) /* changed */
        Set();

    if (state!=osUnknown && Key != kNone) {
        ShowSideNoteInfo();
        SetHelpKeys();
    }

    if (state == osUnknown) {
        switch (NORMALKEY(Key)) {
        case kOk:
        {
            if(PlayRecording(CurrentRecording()))
                state = osEnd;
            else
                state = osContinue;
        }
            break;

        case kInfo: // show recording Info!
            if(CurrentRecording())
                state = AddSubMenu(new myMenuRecordingInfo(CurrentRecording(), true));
            else
                state = osContinue;

            break;

        case kRed:  // delete recording
        {
            cRecording *rec = CurrentRecording();
            bool result = UserConfirmedDeleteRecording(rec);

            if (result) {
                if(DeleteRecording(rec))
                    Set();
                else
                    Skins.Message(mtError, tr("Unable to delete recording"));
            } // if

            state = osContinue;
        }
            break;

        case kGreen: // reset search/New search
            searchParams.Init(); // reset
            Set();
            state = osContinue;
            break;

        case kYellow:
        case kBlue:
            state = osContinue;
            break;

        default:break;
        }
    }

    return state;
}


bool cMenuSearchRecordings::IsAllowed(const cRecording * rec)
{

    if (!rec) return false;

    bool result = true;

    if (isempty(searchParams.name)) // empty string matches all names
        result = true;

    if (rec->Info() && rec->Info()->Title())
        result = result && strcasestr(rec->Info()->Title(), searchParams.name);
    else
        result = result && strcasestr(rec->Name(), searchParams.name);


    if (searchParams.latestMonth)
    {
        time_t now = time(NULL);
        struct tm tm_r;
        struct tm *now_tm = localtime_r(&now, &tm_r);
        now_tm->tm_mon--;
        if (now_tm->tm_mon<0)
        {
            now_tm->tm_mon = 12;
            now_tm->tm_year--;
        }
        now_tm->tm_hour = now_tm->tm_min = now_tm->tm_sec = 0;
        time_t month_ago = mktime(now_tm);

        result = result && (month_ago < rec->Start());
    }

    if (searchParams.videoDefinition == eHDOnly)
        result = result && rec->IsHD();
    else if (searchParams.videoDefinition == eSDOnly)
        result = result && !rec->IsHD();

    return result;
}
