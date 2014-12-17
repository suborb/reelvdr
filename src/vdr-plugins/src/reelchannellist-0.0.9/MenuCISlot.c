#include "MenuCISlot.h"
#include "favourites.h"
#include "menu.h"

cOsdMenuCISlot::cOsdMenuCISlot(cString param, bool isFav, cChannel *channel_):
    cOsdMenu(trVDR("Functions: CI-slot assignment")),
    filterParam(param), isFavourites(isFav), channel(channel_)
{
    // init. caids to 0
    for (int i = 0; i < MAXCAIDS+1; ++i) caids[i] = 0;
    InitCaIds();
    Set();
}

void cOsdMenuCISlot::InitCaIds()
{
    int common_ca = CA_FTA;

    if (channel)
        common_ca = channel->Ca();
    else if (isFavourites) {
        // jump to bouquet
        cChannel *ch = IsBouquetInFavourites(*filterParam);
        if (!ch) return;
        ch = favourites.Next(ch); // next channel


        common_ca = ch? ch->Ca():CA_FTA;

        // traverse the bouquet and check if all channels have ca = common_ca
        while (ch && !ch->GroupSep()) {
            if (common_ca != ch->Ca()) return;
            ch = favourites.Next(ch);
        } // while

    } else {

        cChannelProviderFilter filter;
        filter.SetFilterParams(filterParam);
        cChannel *ch = Channels.First();

        bool common_ca_init = false; // common_ca initialized?

        // find first channel that belongs to the provider, to set common_ca
        while (ch) {
            if (globalFilters.IsAllowedChannel(ch) && filter.IsAllowed(ch)) {

                if (!common_ca_init)  {
                    common_ca = ch->Ca();
                    common_ca_init = true;
                } else if (ch->Ca() != common_ca)
                    return; // common_ca initialized and
                            // not equal to channel in provider...
                            // there is no common CA

            } // if

            ch = Channels.Next(ch);
        } // while

    } // else

    // all channels have the same ca
    caids[0] = common_ca;
}

void cOsdMenuCISlot::Set()
{
    Clear();
    SetCols(25);

    cString tmp;

    cString name, param;
    if (channel) {
        name = tr("channel");
        param = channel->Name();
    }
    else  {
        name = isFavourites?tr("bouquet"):tr("provider");
        param = filterParam;
    }

    tmp = cString::sprintf("%s %s: '%s'", tr("Choose CI slot for"),
                           *name,
                           *param);


    AddFloatingText(*tmp, 45);

    //blank line
    Add(new cOsdItem("", osUnknown, false));

    tmp = cString::sprintf("%s %s", tr("CI-slot for"), *name);

    Add(new cMenuEditMyCaItem(*tmp, &caids[0], channel==NULL));

    Display();
}

/* Set CaId in vdr's list for all channels with provider 'provider'*/
void SetCaProvider(int *caids, const cString& provider)
{
    cChannelProviderFilter filter;
    filter.SetFilterParams(provider);

    cChannel *ch = Channels.First();
    for (; ch; ch = Channels.Next(ch)) {
        if (filter.IsAllowed(ch)  && globalFilters.IsAllowedChannel(ch)
                && !ch->GroupSep()) {
#if REELVDR
            ch->ForceCaIds(caids);
#else
            ch->SetCaIds(caids);
#endif
            isyslog("setting caid of channel %s with provider %s to %i",
                    ch->Name(), *provider, caids[0]);
        } // if
    } // for
}

void SetCaFavourites(int *caids, const cString& folder)
{
    cChannel *ch = IsBouquetInFavourites(*folder);
    cChannel *vdrCh = NULL;
    // skip the bouquet
    if (ch) ch = favourites.Next(ch);

    while(ch && !ch->GroupSep()) {
        vdrCh = Channels.GetByChannelID(ch->GetChannelID());
#if REELVDR
        ch->ForceCaIds(caids);
        if (vdrCh) vdrCh->ForceCaIds(caids);
#else
        ch->SetCaIds(caids);
        if (vdrCh) vdrCh->SetCaIds(caids);
#endif
        isyslog("setting caid of channel %s in fav folder %s to %i",
                ch->Name(), *folder, caids[0]);
        ch = favourites.Next(ch);
    }

}

void SetCaChannel( int *caids, cChannel *channel, bool isFav=false)
{
    if (!channel) return;

    if (isFav) {
        // change CaIds in vdr's list as well as favourites list
        cChannel *vdrCh = Channels.GetByChannelID(channel->GetChannelID());
#if REELVDR
        channel->ForceCaIds(caids);
        if (vdrCh) vdrCh->ForceCaIds(caids);
#else
        channel->SetCaIds(caids);
        if (vdrCh) vdrCh->SetCaIds(caids);
#endif
    } else {
#if REELVDR
        channel->ForceCaIds(caids);
#else
        channel->SetCaIds(caids);
#endif
    }

}

eOSState cOsdMenuCISlot::ProcessKey(eKeys key)
{
    eOSState state = cOsdMenu::ProcessKey(key);

    if (state == osUnknown)
        switch (key) {

        case kOk:
            printf("caid: %d\n", caids[0]);
            if (channel)
                SetCaChannel(caids, channel, isFavourites);
            else if (isFavourites)
                SetCaFavourites(caids, filterParam);
            else
                SetCaProvider(caids, filterParam);

            state = osBack;
            break;

        default:
            break;
        } //switch

    return state;
}
