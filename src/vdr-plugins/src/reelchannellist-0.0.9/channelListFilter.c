#include "channelListFilter.h"
#include "tools.h"
#include <string.h>
#include <vdr/device.h>
#include "favourites.h"
#include <vdr/plugin.h>

/// cChannelFilters ------------------------------------------------------------
cChannelFilters::cChannelFilters(const char* name_): name(name_)
{
}

cString cChannelFilters::Name() const 
{
    return name;
}

bool cChannelFilters::IsAllowed(const cChannel *channel)
{
    cChannelFilter *filter;

    for (filter = First(); filter ; filter = Next(filter))
        if (!filter->IsAllowed(channel))
            return false;

    /* all filters allow channel */
    return true;
}

void cChannelFilters::RemoveLastFilter()
{
    if(Last())
        Del(Last());
}

cString cChannelFilters::FilterNames() const
{
    cString tmp;
    cString result;
    cChannelFilter *filter = First();
    while (filter) {
        if (*result)
            tmp = cString::sprintf("%s/%s", *result, *filter->Name());
        else
            tmp = filter->Name();

        result = tmp;

        filter = Next(filter);
    } // while

    return result;
}

cString cChannelFilters::FilterParamsToString() const
{
    cString tmp;
    cString result;
    cChannelFilter *filter = First();
    while (filter) {
        if (*result)
            tmp = cString::sprintf("%s/%s", *result, *filter->ParamsToString());
        else
            tmp = filter->Name();

        result = tmp;

        filter = Next(filter);
    } // while

    return result;
}

cString cChannelFilters::LastFilterParam() const
{
    cChannelFilter *filter = Last();

    return filter? filter->ParamsToString():"";
}

void cChannelFilters::Save() const
{
    cString str, param;
    cChannelFilter *filter = First();
    int i = 0;

    cPlugin *plugin = cPluginManager::GetPlugin("reelchannellist");

    str = cString::sprintf("%s_count", *name);
    plugin->SetupStore(str, Count());
    while (filter) {
        str = cString::sprintf("%s_%d", *name, i);
        param = cString::sprintf("%s %s", *filter->Name(), *filter->ParamToSave());
        plugin->SetupStore(str, param);

        filter = Next(filter);
        i++;
    }
}

bool cChannelFilters::AddFilterFromString(const char* filterName, const char* param)
{

    if(!filterName) return false;

    cChannelFilter *filter = NULL;

    if(strcmp(filterName, "TextSearch")==0)         filter = new cChannelTextFilter;
    else if (strcmp(filterName, "Source")==0)       filter = new cChannelSourceFilter;
    else if(strcmp(filterName, "Bouquet")==0)       filter = new cChannelBouquetFilter;
    else if(strcmp(filterName, "Provider")==0)      filter = new cChannelProviderFilter;
    else if(strcmp(filterName, "Transponder")==0)   filter = new cChannelTransponderFilter;
    else if(strcmp(filterName, "Radio")==0)         filter = new cChannelRadioFilter;
    else if(strcmp(filterName, "TV")==0)            filter = new cChannelTVFilter;
    // add new filters here

    if(filter) {
        filter->ParseParamString(param);
        Add(filter);
        printf("\033[0;91mAdded filter: %s params: %s, Count:%d\033[0m\n", *filter->Name(), param, Count());
        return true;
    } else
        printf("\033[0;92mfilterName: %s, param: %s NOT added, Count:%d\033[0m\n", filterName, param, Count());

    return false;
}


template <class T>
T* MakeCopy(cChannelFilter *filter)
{
    T *tFilter = dynamic_cast<T *> (filter);
    if (tFilter) {
        T *nFilter = new T;
        *nFilter = *tFilter;
        return nFilter;
    }

    return NULL;
}

cChannelFilters& cChannelFilters::operator=(cChannelFilters const &from)
{
    if (this == &from) {
        printf("not copying myself\n");
        return *this;
    }
    printf("%s\n", __PRETTY_FUNCTION__);

    // delete old contents
    Clear();

    cChannelFilter *filter = from.First();
    cChannelFilter *nFilter = NULL;

    while (filter) {
        if ( (nFilter = MakeCopy<cChannelTextFilter>(filter)) ) {
            Add(nFilter);
        } else if ( (nFilter = MakeCopy<cChannelSourceFilter>(filter)) ) {
            Add(nFilter);
        }else if ( (nFilter = MakeCopy<cChannelBouquetFilter>(filter)) ) {
            Add(nFilter);
        }else if ( (nFilter = MakeCopy<cChannelProviderFilter>(filter)) ) {
            Add(nFilter);
        }else if ( (nFilter = MakeCopy<cChannelTransponderFilter>(filter)) ) {
            Add(nFilter);
        } else if ( (nFilter = MakeCopy<cChannelRadioFilter>(filter)) ) {
            Add(nFilter);
        }else if ( (nFilter = MakeCopy<cChannelTVFilter>(filter)) ) {
            Add(nFilter);
        }

        filter = from.Next(filter);
    }

    return *this;

}

/// cChannelFilter -------------------------------------------------------------
cChannelFilter::cChannelFilter(const char *s) : name (s)
{

}

cString cChannelFilter::Name() const
{
    return name;
}

///  cChannelTextFilter -------------------------------------------------------
cChannelTextFilter::cChannelTextFilter() : cChannelFilter("Text Search")
{
}

cString cChannelTextFilter::ParamsToString() const
{
    return filterByText;
}

void cChannelTextFilter::SetFilterParams(const cString& params)
{
    filterByText = params;
}

bool cChannelTextFilter::IsAllowed(const cChannel* channel)
{
    if (!channel) return false;

    if (strcasestr(channel->Name(), *filterByText) != NULL)
        return true;
    else {
        /* event title contains text? */
        const cEvent* event = GetPresentEvent(channel);
        if (event && event->Title()
                && strcasestr(event->Title(), *filterByText))
            return true;
    }

    return false;
}

cString cChannelTextFilter::ParamToSave() const
{
    return filterByText;
}

bool cChannelTextFilter::ParseParamString(cString param)
{
    if(!*param) return false;
    filterByText = param;
    return true;
}


///  cChannelSourceFilter -------------------------------------------------------
cChannelSourceFilter::cChannelSourceFilter() : cChannelFilter("Source")
{

}

cString cChannelSourceFilter::ParamsToString() const
{
    return cSource::ToString(source);
    return cString::sprintf("%d", source);
}

void cChannelSourceFilter::SetFilterParams(int src)
{
    source = src;
}

bool cChannelSourceFilter::IsAllowed(const cChannel* channel)
{
    if (!channel) return false;

    return source == channel->Source();
}

cString cChannelSourceFilter::ParamToSave() const
{
    return cString::sprintf("%d", source);
}

bool cChannelSourceFilter::ParseParamString(cString param)
{
    if (!*param) return false;
    source = atoi(*param);
    return true;
}


///  cChannelBouquetFilter -------------------------------------------------------
cChannelBouquetFilter::cChannelBouquetFilter(): cChannelFilter ("Bouquet")
{

}

cString cChannelBouquetFilter::ParamsToString() const
{
    return bouquetName;
}

void cChannelBouquetFilter::SetFilterParams(const cString& bouquet,
                                            cChannels* chList,
                                            bool partialMatch)
{
    bouquetName = bouquet;
    channelList = chList;
    subStringMatch = partialMatch;
}

bool cChannelBouquetFilter::IsAllowed(const cChannel* channel)
{
    if (!channel || !channelList) return false;

    const char* grpName = ChannelInBouquet(channel, *channelList);
    //printf("Searching for '%s' in '%s' Null?%d\n", *bouquetName, grpName, grpName == NULL);

    return grpName &&
            (subStringMatch? strstr(grpName, *bouquetName) != NULL
                          : (*bouquetName && strcmp(grpName, *bouquetName) == 0));
}

cString cChannelBouquetFilter::ParamToSave() const
{
    bool isFav = (channelList == &favourites);
    return cString::sprintf("%s %s %s", isFav?"Favourites":"GlobalChannelList",
                            subStringMatch?"partial":"full",
                            *bouquetName);
    //bouquet name last, since it can contain space.
    //Therefore easier to parse when it is last.
}

// parse string such as 'Favourites full Dritte Programme'
bool cChannelBouquetFilter::ParseParamString(cString param)
{
    if (!*param) return false;

#if 0
    char *p;
    const char *delim = " ";
    char *strtok_saveptr;
    char *param_str = strdup(*param);

    // favourites list OR global vdr's channel list
    p = strtok_r(param_str, delim, &strtok_saveptr);
    if (strncmp(p, "Favourites", strlen("Favourites"))==0)
        channelList = &favourites;
    else
        channelList = &Channels;

    // sub string match
    p = strtok_r(NULL, delim, &strtok_saveptr);
    subStringMatch = (strcmp(p,"partial") == 0);

    // bouquet name
    p = strtok_r(NULL, delim, &strtok_saveptr);
    bouquetName = p;

    free(param_str);
    return true;
#else

    char list[32]  = {0};  // holds 'Favourites' or 'GlobalChannellist'
    char match[16] = {0};  // 'Partial' or 'Full'
    char name[64]  = {0};  // name of bouquet with spaces

    int n = sscanf(*param, "%31s %15s %63[^\n]", list, match, name);
    printf("%s:%d param:'%s' list:'%s', match: '%s', name:'%s'\n", __FILE__, __LINE__, *param,
           list, match, name);
    printf("Got %d variables. Strlen list %d, match %d, name %d\n",
           n, strlen(list), strlen(match), strlen(name));

    if (strncmp(list, "Favourites", strlen("Favourites"))==0)
        channelList = &favourites;
    else
        channelList = &Channels;

    if (strncmp(match, "partial", strlen("partial"))==0)
        subStringMatch = true;
    else
        subStringMatch = false;

    bouquetName = name;
    return true;
#endif

}




///  cChannelProviderFilter ----------------------------------------------------
cChannelProviderFilter::cChannelProviderFilter(): cChannelFilter("Provider")
{

}

cString cChannelProviderFilter::ParamsToString() const
{
    return providerName;
}

void cChannelProviderFilter::SetFilterParams(const cString& provider)
{
    providerName = provider;
}

bool cChannelProviderFilter::IsAllowed(const cChannel* channel)
{
    if (!channel) return false;

    return !channel->GroupSep() && strcmp(*providerName, channel->Provider()) == 0;
}

cString cChannelProviderFilter::ParamToSave() const
{
    return providerName;
}

bool cChannelProviderFilter::ParseParamString(cString param)
{
    if (!*param) return false;
    providerName = param;

    return true;
}


///  cChannelTransponderFilter -------------------------------------------------
cChannelTransponderFilter::cChannelTransponderFilter(): cChannelFilter("Transponder")
{

}

cString cChannelTransponderFilter::ParamsToString() const
{
    cChannel *currentChannel = Channels.GetByNumber(cDevice::PrimaryDevice()->CurrentChannel());

    if (currentChannel && currentChannel->Source() == source
            && currentChannel->Frequency() == frequency)
        return tr("Current Transponder");

    return cString::sprintf("%s %d", *cSource::ToString(source), frequency);
    return cString::sprintf("%d %d", source, frequency);
}

void cChannelTransponderFilter::SetFilterParams(int src, int freq)
{
    source    = src;
    frequency = freq;
}

bool cChannelTransponderFilter::IsAllowed(const cChannel *channel)
{
    if (!channel) return false;

    /* Same source (satellite) and same frequency */
    return channel->Source() == source && channel->Frequency() == frequency;
}

cString cChannelTransponderFilter::ParamToSave() const
{
    return cString::sprintf("%d %d", source, frequency);
}

bool cChannelTransponderFilter::ParseParamString(cString param)
{
    if (!*param) return false;
    int ret = sscanf(*param, "%d %d", &source, &frequency);

    return ret == 2;
}



#define IS_RADIO_CHANNEL(c)           (c->Vpid()==0||c->Vpid()==1)
/// cChannelRadioFilter --------------------------------------------------------
cChannelRadioFilter::cChannelRadioFilter(): cChannelFilter("Radio")
{

}

bool cChannelRadioFilter::IsAllowed(const cChannel * channel)
{
    if (!channel) return false;

    return IS_RADIO_CHANNEL(channel);
}

cString cChannelRadioFilter::ParamsToString() const
{
    return "Radio";
}

cString cChannelRadioFilter::ParamToSave() const
{
    return "Radio";
}

bool cChannelRadioFilter::ParseParamString(cString param)
{
    return true; // no param needed
}


/// cChannelTVFilter -----------------------------------------------------------
cChannelTVFilter::cChannelTVFilter(): cChannelFilter("TV")
{

}

bool cChannelTVFilter::IsAllowed(const cChannel * channel)
{
    if (!channel) return false;

    return !IS_RADIO_CHANNEL(channel);
}

cString cChannelTVFilter::ParamsToString() const
{
    return "TV";
}

cString cChannelTVFilter::ParamToSave() const
{
    return "TV";
}

bool cChannelTVFilter::ParseParamString(cString param)
{
    return true; // no param needed
}

