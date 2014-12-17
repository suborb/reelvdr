#include "tools.h"
#include <vdr/sources.h>



/** ********** CompareBySourceName(const void *a, const void *b) ***************
***
*** assume params are pointers to int variable that represent sources
*** String compare source 'Description' / Name
*******************************************************************************/
int CompareBySourceName(const void *a, const void *b)
{
    int src_a = *(int *)a;
    int src_b = *(int *)b;

    const char* src_a_name = "";
    const char* src_b_name = "";

    if (Sources.Get(src_a))
        src_a_name = Sources.Get(src_a)->Description();
    if (Sources.Get(src_b))
        src_b_name = Sources.Get(src_b)->Description();

    return strcmp(src_a_name, src_b_name);
}



/** *********************** cMySourceList **************************************
*** cMySourceList should hold list of sources, represented by numbers
*** (see cSource in vdr ), without any duplicates
*******************************************************************************/

cMySourceList::~cMySourceList ()
{
    Clear();
}

int cMySourceList::Find(int code) const
{
    for (int i=0; i< Size(); ++i)
    {
        if (At(i) == code)
            return true;
    } // for

    return -1; // not found
}

void cMySourceList::Clear()
{
    cVector<int>::Clear();
}


/** const char* BouquetName(int chanNumber) ************************************
*** Get bouquet name of a channel
********************************************************************************/

const char* ChannelInBouquet(const cChannel *channel, cChannels& Channels)
{
    // if channel is bouquet name then it does not belong to any bouquet
    // as it is not a "channel"
    if (!channel || channel->GroupSep()) return NULL;

#if 0 // very slow for large channel lists
    // for a channel list with 3632 lines with 76 bouquets,
    // a bouquet search with partialmatch=true took ~12secs, where as the
    // alternative is near instantaneous
    int grpIdx = Channels.GetPrevGroup(channel->Index());
    cChannel *groupCh = Channels.Get(grpIdx);
    return groupCh ? groupCh->Name() : NULL;
#else
    while (channel && !channel->GroupSep()) channel = Channels.Prev(channel);
    return channel? channel->Name():NULL;
#endif
}


const cEvent* GetPresentEvent(const cChannel* channel)
{
    const cEvent* event = NULL;
    if (channel)
    {
        cSchedulesLock schedulesLock;
        const cSchedules *schedules = cSchedules::Schedules(schedulesLock);

        if (schedules)
        {
            const cSchedule* schedule = schedules->GetSchedule(channel->GetChannelID());

            if (schedule)
                event = schedule->GetPresentEvent();

        } // if

    } // if (bouquet)

    return event;
}


char NumpadToChar(unsigned int k, unsigned int multiples)
{
    static char numpad[][5] = {
        " @+", //0
        ".?!", // 1
        "abc", //2
        "def", //3
        "ghi", //4
        "jkl", //5
        "mno", //6
        "pqrs", //7
        "tuv", //8
        "wxyz", //9
    };

    char ret = 0; // error, not found

    if (k>9) return ret; // error, index over shoot

    // maximum multiple for given number 'k', multiples start from 0...
    int maxMultiple = strlen(numpad[k])-1;

#if 0
    if (multiples > maxMultiple) return ret; // error, too many multiples
#else
    multiples %= strlen(numpad[k]);
#endif

    ret = numpad[k][multiples];
    return ret;

}

char NumpadChar(eKeys Key)
{
#define NUMPAD_TIMEOUT 2000 //2 secs
            static cTimeMs timeout(NUMPAD_TIMEOUT);
            static int multiples = 0;
            static int lastNum = -1;

            int pressedNum = Key - k0;

            if (timeout.TimedOut() || lastNum != pressedNum) {
                multiples = 0;
                lastNum = pressedNum;
            }
            else
                ++multiples;

            timeout.Set(NUMPAD_TIMEOUT);

            return NumpadToChar(pressedNum, multiples);
}
