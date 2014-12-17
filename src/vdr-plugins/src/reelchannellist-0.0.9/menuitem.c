#include "menuitem.h"
#include "tools.h"

#include <vdr/tools.h>
#include <vdr/sources.h>

/** Sort mode for channels */
eSortModes ChannelSortMode = SortByName;

/** **************** cOsdChannelItem *******************************************
*** Osd Item that shows channel and bouquet names
*** Channel:
      channel-number  name    current-event-progressbar    current-event-title
    channel-number is omitted if showAsFav=true
***
*** Bouquet:
        shown with a folder icon preceeding its name

*** Bouquet and channel can be 'marked' with a tick, used when moving them
*******************************************************************************/

cOsdChannelItem::cOsdChannelItem(cChannel * ch, bool fav, bool mark, bool forbidden) :
    channel(ch),
    event(NULL),
    showAsFav(fav),
    isMarked(mark),
    isForbidden(forbidden)
{
    Set();
}

cOsdChannelItem::~cOsdChannelItem()
{

}

int cOsdChannelItem::Compare(const cListObject &ListObject) const
{
    const cOsdChannelItem *p = dynamic_cast<const cOsdChannelItem*> (&ListObject);
    if (!p) return 0;

    int r = -1;

    if (!r || ChannelSortMode == SortByProvider)
        r = strcasecmp(Channel()->Provider(), p->Channel()->Provider());

    if (!r || ChannelSortMode == SortByName)
        r = strcasecmp(Channel()->Name(), p->Channel()->Name());

    if (!r || ChannelSortMode == SortByNumber)
        r = Channel()->Number() - p->Channel()->Number();

    return r;
}

void cOsdChannelItem::Set()
{
    cString text;

    // must call PresentEvent() so that event variable is set/updated
    const cEvent* dummy  = PresentEvent();

    // do not pass NULL pointer instead use empty string ""
    cString pgBar = ProgressbarString();
    const char* pgBarStr = *pgBar?*pgBar:"";
    const char* title    = (event&&event->Title())?event->Title():"";


    if (channel->GroupSep()) { // bouquet name

        if (showAsFav || isMarked) {
            char icon = char(130); // folder
            if (isMarked) icon = char(135);
            if (isForbidden) icon = char(146);

            text = cString::sprintf("%c%s%s", icon,
                                    showAsFav?" .. ":" ", channel->Name());
        }
        else
            text = cString::sprintf("%s", channel->Name());

        // make bouquet names selectable if favourites
        SetSelectable(showAsFav);
    }
    else { // normal channel
        if (showAsFav) {// no channel number
            if (isMarked)
                text = cString::sprintf("%c %s\t%s\t%s",
                                        char(135),
                                        channel->Name(),
                                        pgBarStr,   // progress bar rep. time elapsed in current event
                                        title);    // current event title
            else
                text = cString::sprintf("%s\t%s\t%s",
                                        channel->Name(),
                                        pgBarStr,   // progress bar rep. time elapsed in current event
                                        title);    // current event title
        }
        else {
            if (isMarked)
                text = cString::sprintf(">\t%s\t%s\t%s",
                                        channel->Name(),
                                        pgBarStr,   // progress bar rep. time elapsed in current event
                                        title);    // current event title
        else
            text = cString::sprintf("%03d\t%s\t%s\t%s",
                                    channel->Number(),
                                    channel->Name(),
                                    pgBarStr,   // progress bar rep. time elapsed in current event
                                    title);    // current event title
        }
    }

    SetSelectable(!isForbidden);
    SetText(*text);
   // printf("Setting: '%s'\n", *text);
}

cChannel* cOsdChannelItem::Channel()
{
    return channel;
}

const cChannel* cOsdChannelItem::Channel() const
{
    return channel;
}

const cEvent* cOsdChannelItem::PresentEvent()
{
    event = GetPresentEvent(channel);
    return event;
} // GetPresentEvent()


cString cOsdChannelItem::ProgressbarString()
{
    cString progressStr;

    if (event && event->Duration() /* no duration no progress! */)
    {
        char szProgress[9];
        int frac = 0;
#ifdef SLOW_FPMATH
        frac =
                (int)roundf((float)(time(NULL) - event->StartTime()) /
                            (float)(event->Duration()) * 8.0);
        frac = min(8, max(0, frac));
#else
        frac =
                min(8,
                    max(0,
                        (int)((time(NULL) -
                               event->StartTime()) * 8) / event->Duration()));
#endif
        for (int i = 0; i < frac; i++)
            szProgress[i] = '|';
        szProgress[frac] = 0;
        //sprintf(szProgressPart, "[%-8s]\t", szProgress);

        progressStr = cString::sprintf("[%-8s]", szProgress);
    }

    return progressStr;
}


/** **************** cOsdSourceItem ********************************************
** Osd Item that stores source as int and displays source name as a string
**
** actionType indicates the suffix to the source name
**            (new channels/Providers/channels)
**            and also indicates what type of filter to be added to filter list
*******************************************************************************/

cOsdSourceItem::cOsdSourceItem(int src, eItemActionType action) :
    source(src),
    actionType(action)
{
    Set();
}

cOsdSourceItem::~cOsdSourceItem()
{
}


void cOsdSourceItem::Set()
{
    cString text;
    const cSource *src = Sources.Get(source);

    // show action type after source name/info
    cString actionStr;

    switch (actionType) {
    case eActionNewChannels: actionStr = tr("New channels"); break;
    case eActionProviders:   actionStr = tr("Providers");            break;
    case eActionChannels:    actionStr = tr("Channels");             break;
    default: break;
    }

    if(actionType == eActionCurrentTransponder)
    {
        text = cString::sprintf("%c %s", (char)130, tr(CURRENT_TRANSPONDER));
    } else if (src)
    {
        if (src->Description())
            text = cString::sprintf("%c %s - %s  %s", (char)130, *cSource::ToString(source),
                                    src->Description(), *actionStr);
        else
            text = cString::sprintf("%c %s  %s", (char)130, *cSource::ToString(source), *actionStr);
    } else
        text = cString::sprintf("%c %s", (char)130, tr("Unknown source"));

    SetText(*text);
    printf("Source %p: id:=%d '%s'\n",src, source, *text);
}



/** ****************** cOsdProviderItem  ***************************************
***    Show Provider name and an icon before it (folder icon)
*******************************************************************************/

cOsdProviderItem::cOsdProviderItem(const char*name): providerName(name)
{
    Set();
}

// show provider name with icon preceeding it
void cOsdProviderItem::Set()
{
    // provider name with icon
    cString text = cString::sprintf("%c\t%s", (char)130, isempty(*providerName)?tr("Unknown Provider"):*providerName);

    SetText(text);
}
