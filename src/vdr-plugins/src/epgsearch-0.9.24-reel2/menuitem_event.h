#ifndef MENUITEM_EVENT_H
#define MENUITEM_EVENT_H

#include <vdr/menuitems.h>

///
/// ---- cMenuEventItem  -----
/// display cEvent object as a single line text item

class cMenuEventItem : public cOsdItem
{
public:
    const cEvent *event;

    cMenuEventItem(const cEvent *e);
    void Set();
};


/// ---- cMenuEventItem  -----
/// display cEvent object as a single line text item

cMenuEventItem::cMenuEventItem(const cEvent *e) : cOsdItem(osUnknown), event(e)
{
    Set();
}

int Day(time_t t) {
    struct tm tm_r;
    struct tm *tm = localtime_r(&t, &tm_r);

    return tm->tm_mday;
}

/// Set text as:
/// day-date | time | icon | Title ~ shorttext
void cMenuEventItem::Set()
{
    if (!event) return;

    time_t startTime = event->StartTime();
    // day-date | time | icon | Title ~ shorttext
    cString buf = cString::sprintf("%s %2d  %s    ", *WeekDayName(startTime),
                                   Day(startTime),
                          *TimeString(startTime));

    // Title ~ shorttext
    cString desc;
    const char* title     = event->Title();
    const char* shorttext = event->ShortText();

    if (title && shorttext && !isempty(title) && !isempty(shorttext))
        desc = cString::sprintf("%s ~ %s", title, shorttext);
    else if (title && !isempty(title))
        desc = title;
    else if (shorttext && !isempty(shorttext))
        desc = shorttext;
    else // empty string
        desc = tr("programme not known");

    buf = cString::sprintf("%s %s", *buf, *desc);

    SetText(buf);
    //printf("%s\n", *buf);
}

#endif // MENUITEM_EVENT_H
