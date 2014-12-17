
#define  __STL_CONFIG_H

#include <algorithm>
#include <stdio.h>
#include <vdr/tools.h>
#include <vdr/menuitems.h>
#include <vdr/skins.h>

#include "radioStations.h"

#define USER_SERVER_LIST "/etc/vdr/plugins/shoutcast_server.list"

cServerInfo::cServerInfo(const string name, const string url)
{
    Name = name;
    Url = url;
}

cServerInfo::cServerInfo()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
cRadioStation *cRadioStation::instance_ = NULL;

cRadioStation::cRadioStation()
{
}

cRadioStation::~cRadioStation()
{
    stationList_.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
cRadioStation *cRadioStation::Instance()
{
    if (!instance_)
    {
        instance_ = new cRadioStation;
        instance_->ReadStationList();
        instance_->SetDirty();
    }

    return instance_;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
vector < cServerInfo > cRadioStation::StationList() const
{
    return stationList_;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool cRadioStation::IsUniqueUrl(string url) const
{
    unsigned int i = 0;
    for (; i < stationList_.size(); ++i)
        if (stationList_.at(i).Url == url)
            return false;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool cRadioStation::IsUniqueName(string name) const
{
    unsigned int i = 0;
    for (; i < stationList_.size(); ++i)
        if (stationList_.at(i).Name == name)
            return false;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void cRadioStation::ForgetChanges()
{
    stationList_.clear();
    ReadStationList();
    SetDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool cRadioStation::AddStation(string name, string url)
{
    if (!IsUniqueUrl(url))
    {
        //Skins.Message(mtError, tr("Station Url already added"));
        return false;
    }

    if (!IsUniqueName(name))
    {
        //Skins.Message(mtError, tr("Station name already used"));
        return false;
    }

    stationList_.push_back(cServerInfo(name, url));
    sort(stationList_.begin(), stationList_.end());
    SetDirty();
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool cRadioStation::DelStation(string name, string url)
{
    unsigned int i = 0;

    for (; i < stationList_.size(); ++i)
        if (stationList_.at(i).Name == name && stationList_.at(i).Url == url)
        {
            stationList_.erase(stationList_.begin() + i);
            SetDirty();
            return true;
        }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool cRadioStation::ReadStationList()
{
    /// read user defined server list

    FILE *fp = fopen(USER_SERVER_LIST, "r");
    if (!fp)
    {
        esyslog("Could not open Radio station list file: %s", USER_SERVER_LIST);
        printf("Could not open Radio station list file: %s\n", USER_SERVER_LIST);
        return false;           // ERROR
    }

    stationList_.clear();
    cServerInfo serverObj;
    char *line;
    cReadLine rl;


    char serverName[128], serverUrl[256];
    char ch;

    while ((line = rl.Read(fp)) != NULL)
    {
        if (strlen(line) > 0)
        {

            sscanf(line, "%[^#]%c %[^\n]", serverName, &ch, serverUrl); // ch removes '#'
            serverName[127] = serverUrl[255] = 0;
            //printf("[%s] (%c) [%s]\n", serverName, ch, serverUrl );

            serverObj.Name = serverName;
            serverObj.Url = serverUrl;
            stationList_.push_back(serverObj);
        }
    }
    fclose(fp);

    //sort ( serverList_.begin(), serverList_.end(), ServerCompare );
    sort(stationList_.begin(), stationList_.end());

    SetDirty();

    //unique ?
    //
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool cRadioStation::IsEmpty() const
{
    return stationList_.empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Deletes Station (by index) from station list 
//
bool cRadioStation::DelStation(unsigned int stationNumber)
{
    if (stationNumber < 0 || stationNumber > stationList_.size())
        return false;

    stationList_.erase(stationList_.begin() + stationNumber);
    SetDirty();
    Save();
    return true;
}

bool cRadioStation::ReplaceStation(int index, const cServerInfo newServerInfo)
{
    if (index > (int)stationList_.size() || index < 0)
        return false;

    // replace
    stationList_[index] = newServerInfo;
    SetDirty();
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Saves the station vector to file
///

bool cRadioStation::Save()
{
    // overwrite the file 'w'
    FILE *fp = fopen(USER_SERVER_LIST, "w+");
    if (!fp)
    {
        printf("\033[0;91m Error in opening for writing %s file \033[0m\n",
               USER_SERVER_LIST);
        return false;           // ERROR
    }

    int res;
    bool flag = true;

    for (unsigned int i = 0; i < stationList_.size(); ++i)
    {
        res =
            fprintf(fp, "%s#%s\n", stationList_.at(i).Name.c_str(),
                    stationList_.at(i).Url.c_str());
        if (res <= 0)
        {
            flag = false;
            printf("\033[0;91m Error in saving \033[0m%s  %s\n",
                   stationList_.at(i).Name.c_str(), stationList_.at(i).Url.c_str());
        }
    }
    fclose(fp);

    return flag;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void cRadioStation::SetDirty()
{
    dirty_ = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void cRadioStation::ResetDirty()
{
    dirty_ = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool cRadioStation::Dirty() const
{
    return dirty_;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
