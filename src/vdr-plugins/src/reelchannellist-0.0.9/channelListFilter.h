#ifndef CHANNELLISTFILTER_H
#define CHANNELLISTFILTER_H

#include <vdr/tools.h>
#include <vdr/channels.h>

/** ************ cChannelFilter ************************************************
*** Parent class of channel filters to be created
*** creates filters for channels in channel list
*******************************************************************************/

class cChannelFilter : public cListObject
{
protected:
    cString name;
public:
    cChannelFilter(const char* s);
    virtual bool IsAllowed(const cChannel*) = 0;
    virtual cString Name() const;
    virtual cString ParamsToString() const = 0;
    virtual cString ParamToSave() const = 0;
    virtual bool ParseParamString(cString param) = 0;
}; // class cChannelFilter



/** ****************** cChannelFilters *****************************************
*** holds all channel filters that are to be applied to a channel when decided
*** whether this channel is to be shown or not.
*******************************************************************************/
class cChannelFilters : public cList<cChannelFilter>
{
private:
    cString name; // is it globalChannel filter list or favourite filter list?
public:
    cChannelFilters(const char* name_);

    cString Name() const; 
    bool IsAllowed(const cChannel* channel);
    void RemoveLastFilter();
    cChannelFilters& operator=(cChannelFilters const &);

    cString FilterNames() const;
    cString FilterParamsToString() const;
    cString LastFilterParam() const;

    void Save() const;
    bool AddFilterFromString(const char* filterName, const char* param);
};



/** ****************************************************************************
*** The following are subclasses of cChannelFilter
*** Each implements a single/specific filter
*** and are to be added to cChannelFilters as and when needed
*******************************************************************************/

/** text filter  **************************************************************/
class cChannelTextFilter : public cChannelFilter
{
private:
    cString filterByText;
public:
    cChannelTextFilter();
    void SetFilterParams(const cString& params);
    bool IsAllowed(const cChannel* channel);
    cString ParamsToString() const;
    virtual cString ParamToSave() const;
    bool ParseParamString(cString param);
};

/** source/Satellite filter  **************************************************/
class cChannelSourceFilter : public cChannelFilter
{
private:
    int source;
public:
    cChannelSourceFilter();
    void SetFilterParams(int src);
    bool IsAllowed(const cChannel* channel);
    cString ParamsToString() const;
    virtual cString ParamToSave() const;
    bool ParseParamString(cString param);
};

/** bouquet name filter  *******************************************************
  bouquet filter:
  looks if bouquetName string is a substring of the bouquet of given channel
*******************************************************************************/
class cChannelBouquetFilter : public cChannelFilter
{
private:
    cString bouquetName;
    cChannels *channelList;
    bool subStringMatch;
public:
    cChannelBouquetFilter();
    void SetFilterParams(const cString& bouquet, cChannels*, bool partialMatch=true );
    bool IsAllowed(const cChannel* channel);
    cString ParamsToString() const;
    virtual cString ParamToSave() const;
    bool ParseParamString(cString);
};

/** provider name filter ******************************************************/
class cChannelProviderFilter : public cChannelFilter
{
private:
    cString providerName;
public:
    cChannelProviderFilter();
    void SetFilterParams(const cString& provider);
    bool IsAllowed(const cChannel* channel);
    cString ParamsToString() const;
    virtual cString ParamToSave() const;
    bool ParseParamString(cString);
};

/** Transponder filter ******************************************************/
class cChannelTransponderFilter : public cChannelFilter
{
private:
    int source; /* source Code as in cSources */
    int frequency;

public:
    cChannelTransponderFilter();
    void SetFilterParams(int src, int freq);
    bool IsAllowed(const cChannel* channel);
    cString ParamsToString() const;
    virtual cString ParamToSave() const;
    bool ParseParamString(cString);
};

/** Radio channels filter **************************************************/
class cChannelRadioFilter : public cChannelFilter
{
public:
    cChannelRadioFilter();
    bool IsAllowed(const cChannel *);
    cString ParamsToString() const;
    virtual cString ParamToSave() const;
    bool ParseParamString(cString);
};


/** TV channels filter *****************************************************/
class cChannelTVFilter : public cChannelFilter
{
public:
    cChannelTVFilter();
    bool IsAllowed(const cChannel *);
    cString ParamsToString() const;
    virtual cString ParamToSave() const;
    bool ParseParamString(cString);
};


#endif // CHANNELLISTFILTER_H
