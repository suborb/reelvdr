/***************************************************************************
 *   Copyright (C) 2008 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 ***************************************************************************
 *
 * epgsearchmysql.c
 *
 ***************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>

#include "epgsearchmysql.h"
#include "changrp.h"
#include "epgsearchcats.h"
#include "blacklist.h"

//##########################################################################
// class cTimerSearchMysql
//##########################################################################
cTimerSearchMysql::cTimerSearchMysql(const char *Username, const char *Password, const char *Database) : cVdrMysql(Username, Password, Database), Table_("timer_search")
{
}

cTimerSearchMysql::~cTimerSearchMysql()
{
}

bool cTimerSearchMysql::InsertTimerSearch(cSearchExt *SearchExt)
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;101m " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif
    MYSQL *Connection = mysql_init(NULL);
    std::stringstream Query;
    std::string tmp_search;
    char* EscapedSearch = NULL;
    std::stringstream tmp_chanSel;
    std::string tmp_directory;
    std::stringstream tmp_catvalues;
    std::stringstream tmp_blacklists;

    if (Connection == NULL)
        return false;

    if(!ConnectDB(Connection)) {
        free(Connection);
        return false; // Connection failed
    }

    tmp_search = SearchExt->search;
    size_t pos = 0;
    while((pos = tmp_search.find('|')) != std::string::npos)
        tmp_search.replace(pos, 1, "!^pipe^!"); // ugly: replace a pipe with something,
                                                              // that should not happen to be part of a search string
    while((pos = tmp_search.find(':')) != std::string::npos)
        tmp_search.replace(pos, 1, 1, '|');
    // First escape strings
    EscapedSearch = (char*)malloc((tmp_search.length()*2+1)*sizeof(char));
    mysql_real_escape_string(Connection, EscapedSearch, tmp_search.c_str(), tmp_search.length());

    if(SearchExt->useChannel == 1)
    {
        if(SearchExt->channelMin->Number() < SearchExt->channelMax->Number())
            tmp_chanSel << CHANNELSTRING(SearchExt->channelMin) << '|' << CHANNELSTRING(SearchExt->channelMax);
        else
            tmp_chanSel << CHANNELSTRING(SearchExt->channelMin);
    }
    else if(SearchExt->useChannel == 2)
    {
        int channelGroupNr = ChannelGroups.GetIndex(SearchExt->channelGroup);
        if(channelGroupNr == -1)
        {
            LogFile.eSysLog("channel group '%s' does not exist!", SearchExt->channelGroup);
            SearchExt->useChannel = 0;
        }
        else
            tmp_chanSel << SearchExt->channelGroup;
    }

    tmp_directory = SearchExt->directory;
    while((pos = tmp_directory.find('|')) != std::string::npos)
        tmp_directory.replace(pos, 1, "!^pipe^!"); // ugly: replace a pipe with something,
                                                                    // that should not happen to be part of a search string
    while((pos = tmp_directory.find(':')) != std::string::npos)
        tmp_directory.replace(pos, 1, 1, '|');

    if (SearchExt->useExtEPGInfo)
    {
        cSearchExtCat *SearchExtCat = SearchExtCats.First();
        int index = 0;
        while (SearchExtCat) 
        {
            std::string catvalue = SearchExt->catvalues[index];
            while((pos = catvalue.find(':')) != std::string::npos)
                catvalue.replace(pos, 1, "!^colon^!"); // ugly: replace with something, that should not happen to be part ofa category value
            while((pos = catvalue.find('|')) != std::string::npos)
                catvalue.replace(pos, 1, "!^pipe^!"); // ugly: replace with something, that should not happen to be part of a regular expression 

            if (index == 0)
                tmp_catvalues << SearchExtCat->id << '#' << catvalue;
            else
                tmp_catvalues << '|' << SearchExtCat->id << '#' << catvalue;
            SearchExtCat = SearchExtCats.Next(SearchExtCat);	 
            index++;
        }      
    }

    if (SearchExt->blacklistMode == blacklistsSelection && SearchExt->blacklists.Count() > 0)
    {
        cBlacklistObject *blacklistObj = SearchExt->blacklists.First();
        int index = 0;
        while (blacklistObj) 
        {
            if (index == 0)
                tmp_blacklists << blacklistObj->blacklist->ID;
            else
                tmp_blacklists << '|' << blacklistObj->blacklist->ID;
            blacklistObj = SearchExt->blacklists.Next(blacklistObj);	 
            index++;
        }      
    }

    Query << "INSERT INTO " << Table_ << " (id, search, useTime, startTime, stopTime, useChannel, ChannelGroup, useCase, mode, useTitle, useSubtitle, useDescription, useDuration, minDuration, maxDuration, useAsSearchTimer, useDayOfWeek, DayOfWeek, useEpisode, directory, Priority, Lifetime, MarginStart, MarginStop, useVPS, action, useExtEPGInfo, ExtEPGInfo, avoidRepeats, allowedRepeats, compareTitle, compareSubtitle, compareSummary, catvaluesAvoidRepeat, repeatsWithinDays, delAfterDays, recordingsKeep, switchMinsBefore, pauseOnNrRecordings, blacklistMode, blacklists, fuzzyTolerance, useInFavorites, menuTemplate, delMode, delAfterCountRecs, delAfterDaysOfFirstRec, useAsSearchTimerFrom, useAsSearchTimerTil, ignoreMissingEPGCats, unmuteSoundOnSwitch)";
    Query << " VALUES (" << SearchExt->ID, 
    Query << ", '" << EscapedSearch;
    Query << "', " << SearchExt->useTime;
    Query << ", '" << std::setw(4) << std::setfill('0') << SearchExt->startTime;
    Query << "', '" << std::setw(4) << std::setfill('0') << SearchExt->stopTime;
    Query << "', " << SearchExt->useChannel << ", '";
    if (SearchExt->useChannel>0 && SearchExt->useChannel<3)
        Query << tmp_chanSel.str();
    else
        Query << "0";
    Query << "', " << SearchExt->useCase;
    Query << ", " << SearchExt->mode;
    Query << ", " << SearchExt->useTitle;
    Query << ", " << SearchExt->useSubtitle;
    Query << ", " << SearchExt->useDescription;
    Query << ", " << SearchExt->useDuration;
    Query << ", '" << std::setw(4) << std::setfill('0') << SearchExt->minDuration;
    Query << "', '" << std::setw(4) << std::setfill('0') << SearchExt->maxDuration;
    Query << "', " << SearchExt->useAsSearchTimer;
    Query << ", " << SearchExt->useDayOfWeek;
    Query << ", " << SearchExt->DayOfWeek;
    Query << ", " << SearchExt->useEpisode;
    Query << ", '" << tmp_directory;
    Query << "', " << SearchExt->Priority;
    Query << ", " << SearchExt->Lifetime;
    Query << ", " << SearchExt->MarginStart;
    Query << ", " << SearchExt->MarginStop;
    Query << ", " << SearchExt->useVPS;
    Query << ", " << SearchExt->action;
    Query << ", " << SearchExt->useExtEPGInfo << ", '";
    if (SearchExt->useExtEPGInfo)
        Query << tmp_catvalues.str();
    Query << "', " << SearchExt->avoidRepeats;
    Query << ", " << SearchExt->allowedRepeats;
    Query << ", " << SearchExt->compareTitle;
    Query << ", " << SearchExt->compareSubtitle;
    Query << ", " << SearchExt->compareSummary;
    Query << ", " << SearchExt->catvaluesAvoidRepeat;
    Query << ", " << SearchExt->repeatsWithinDays;
    Query << ", " << SearchExt->delAfterDays;
    Query << ", " << SearchExt->recordingsKeep;
    Query << ", " << SearchExt->switchMinsBefore;
    Query << ", " << SearchExt->pauseOnNrRecordings;
    Query << ", " << SearchExt->blacklistMode << ", '"; 
    if (SearchExt->blacklists.Count()>0)
        Query << tmp_blacklists.str();
    Query << "', " << SearchExt->fuzzyTolerance;
    Query << ", " << SearchExt->useInFavorites;
    Query << ", " << SearchExt->menuTemplate;
    Query << ", " << SearchExt->delMode;
    Query << ", " << SearchExt->delAfterCountRecs;
    Query << ", " << SearchExt->delAfterDaysOfFirstRec;
    Query << ", " << SearchExt->useAsSearchTimerFrom;
    Query << ", " << SearchExt->useAsSearchTimerTil;
    Query << ", " << SearchExt->ignoreMissingEPGCats;
    Query << ", " << SearchExt->unmuteSoundOnSwitch;
    Query << ')';
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;93m " << __FILE__ << '(' << __LINE__ << "): \"" << Query.str() << "\" \033[0m" << std::endl;
#endif
    int res = mysql_query(Connection, Query.str().c_str());
    if (res != 0) {
        esyslog("ERROR: Database Error %i\n", res);
        free(EscapedSearch);
        DisconnectDB(Connection);
        return false;
    }

    DisconnectDB(Connection);

    if(EscapedSearch)
        free(EscapedSearch);
    return true;
}

bool cTimerSearchMysql::DeleteTimerSearch(unsigned int ID)
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;101m " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif
    MYSQL *Connection = mysql_init(NULL);
    std::stringstream Query;

    if (Connection == NULL)
        return false;

    if(!ConnectDB(Connection)) {
        free(Connection);
        return false; // Connection failed
    }

    Query << "DELETE FROM " << Table_ << " WHERE id=" << ID;
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;93m \"" << Query.str() << "\" \033[0m" << std::endl;
#endif
    int res = mysql_query(Connection, Query.str().c_str());
    if (res != 0) {
        esyslog("ERROR: Database Error %i\n", res);
        DisconnectDB(Connection);
        return false;
    }

    DisconnectDB(Connection);
    return true;
}

bool cTimerSearchMysql::UpdateTimerSearch(cSearchExt *SearchExt)
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;101m " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif

    MYSQL *Connection = mysql_init(NULL);
    std::stringstream Query;
    std::string tmp_search;
    char* EscapedSearch = NULL;
    std::stringstream tmp_chanSel;
    std::string tmp_directory;
    std::stringstream tmp_catvalues;
    std::stringstream tmp_blacklists;

    if (Connection == NULL)
        return false;

    if(!ConnectDB(Connection)) {
        free(Connection);
        return false; // Connection failed
    }

    tmp_search = SearchExt->search;
    size_t pos = 0;
    while((pos = tmp_search.find('|')) != std::string::npos)
        tmp_search.replace(pos, 1, "!^pipe^!"); // ugly: replace a pipe with something,
                                                              // that should not happen to be part of a search string
    while((pos = tmp_search.find(':')) != std::string::npos)
        tmp_search.replace(pos, 1, 1, '|');
    // First escape strings
    EscapedSearch = (char*)malloc((tmp_search.length()*2+1)*sizeof(char));
    mysql_real_escape_string(Connection, EscapedSearch, tmp_search.c_str(), tmp_search.length());

    if(SearchExt->useChannel == 1)
    {
        if(SearchExt->channelMin->Number() < SearchExt->channelMax->Number())
            tmp_chanSel << CHANNELSTRING(SearchExt->channelMin) << '|' << CHANNELSTRING(SearchExt->channelMax);
        else {
            tmp_chanSel << CHANNELSTRING(SearchExt->channelMin);
        }
    }
    else if(SearchExt->useChannel == 2)
    {
        int channelGroupNr = ChannelGroups.GetIndex(SearchExt->channelGroup);
        if(channelGroupNr == -1)
        {
            LogFile.eSysLog("channel group '%s' does not exist!", SearchExt->channelGroup);
            SearchExt->useChannel = 0;
        }
        else
            tmp_chanSel << SearchExt->channelGroup;
    }

    tmp_directory = SearchExt->directory;
    while((pos = tmp_directory.find('|')) != std::string::npos)
        tmp_directory.replace(pos, 1, "!^pipe^!"); // ugly: replace a pipe with something,
                                                                    // that should not happen to be part of a search string

    while((pos = tmp_directory.find(':')) != std::string::npos)
        tmp_directory.replace(pos, 1, 1, '|');

    if (SearchExt->useExtEPGInfo)
    {
        cSearchExtCat *SearchExtCat = SearchExtCats.First();
        int index = 0;
        while (SearchExtCat) 
        {
            std::string catvalue = SearchExt->catvalues[index];
            while((pos = catvalue.find(':')) != std::string::npos)
                catvalue.replace(pos, 1, "!^colon^!"); // ugly: replace with something, that should not happen to be part ofa category value
            while((pos = catvalue.find('|')) != std::string::npos)
                catvalue.replace(pos, 1, "!^pipe^!"); // ugly: replace with something, that should not happen to be part of a regular expression 

            if (index == 0)
                tmp_catvalues << SearchExtCat->id << '#' << catvalue;
            else
            {
                tmp_catvalues << '|' << SearchExtCat->id << '#' << catvalue;
            }
            SearchExtCat = SearchExtCats.Next(SearchExtCat);	 
            index++;
        }      
    }

    if (SearchExt->blacklistMode == blacklistsSelection && SearchExt->blacklists.Count() > 0)
    {
        cBlacklistObject *blacklistObj = SearchExt->blacklists.First();
        int index = 0;
        while (blacklistObj) 
        {
            if (index == 0)
                tmp_blacklists << blacklistObj->blacklist->ID;
            else
            {
                tmp_blacklists << '|' << blacklistObj->blacklist->ID;
            }
            blacklistObj = SearchExt->blacklists.Next(blacklistObj);	 
            index++;
        }      
    }

    Query << "UPDATE " << Table_ << " SET search='" << EscapedSearch;
    Query << "', useTime=" << SearchExt->useTime;
    Query << ", startTime='" << std::setw(4) << std::setfill('0') << SearchExt->startTime;
    Query << "', stopTime='" << std::setw(4) << std::setfill('0') << SearchExt->stopTime;
    Query << "', useChannel=" << SearchExt->useChannel << ", ChannelGroup='";
    if (SearchExt->useChannel>0 && SearchExt->useChannel<3)
        Query << tmp_chanSel.str();
    else
        Query << "0";
    Query << "', useCase=" << SearchExt->useCase;
    Query << ", mode=" << SearchExt->mode;
    Query << ", useTitle=" << SearchExt->useTitle;
    Query << ", useSubtitle=" << SearchExt->useSubtitle;
    Query << ", useDescription=" << SearchExt->useDescription;
    Query << ", useDuration=" << SearchExt->useDuration;
    Query << ", minDuration='" << std::setw(4) << std::setfill('0') << SearchExt->minDuration;
    Query << "', maxDuration='" << std::setw(4) << std::setfill('0') << SearchExt->maxDuration;
    Query << "', useAsSearchTimer=" << SearchExt->useAsSearchTimer;
    Query << ", useDayOfWeek=" << SearchExt->useDayOfWeek;
    Query << ", DayOfWeek=" << SearchExt->DayOfWeek;
    Query << ", useEpisode=" << SearchExt->useEpisode;
    Query << ", directory='" << tmp_directory;
    Query << "', Priority=" << SearchExt->Priority;
    Query << ", Lifetime=" << SearchExt->Lifetime;
    Query << ", MarginStart=" << SearchExt->MarginStart;
    Query << ", MarginStop=" << SearchExt->MarginStop;
    Query << ", useVPS=" << SearchExt->useVPS;
    Query << ", action=" << SearchExt->action;
    Query << ", useExtEPGInfo=" << SearchExt->useExtEPGInfo << ", ExtEPGInfo='";
    if (SearchExt->useExtEPGInfo)
        Query << tmp_catvalues.str();
    Query << "', avoidRepeats=" << SearchExt->avoidRepeats;
    Query << ", allowedRepeats=" << SearchExt->allowedRepeats;
    Query << ", compareTitle=" << SearchExt->compareTitle;
    Query << ", compareSubtitle=" << SearchExt->compareSubtitle;
    Query << ", compareSummary=" << SearchExt->compareSummary;
    Query << ", catvaluesAvoidRepeat=" << SearchExt->catvaluesAvoidRepeat;
    Query << ", repeatsWithinDays=" << SearchExt->repeatsWithinDays;
    Query << ", delAfterDays=" << SearchExt->delAfterDays;
    Query << ", recordingsKeep=" << SearchExt->recordingsKeep;
    Query << ", switchMinsBefore=" << SearchExt->switchMinsBefore;
    Query << ", pauseOnNrRecordings=" << SearchExt->pauseOnNrRecordings;
    Query << ", blacklistMode=" << SearchExt->blacklistMode << ", blacklists='";
    if (SearchExt->blacklists.Count()>0)
        Query << tmp_blacklists.str(); 
    Query << "', fuzzyTolerance=" << SearchExt->fuzzyTolerance;
    Query << ", useInFavorites=" << SearchExt->useInFavorites;
    Query << ", menuTemplate=" << SearchExt->menuTemplate;
    Query << ", delMode=" << SearchExt->delMode;
    Query << ", delAfterCountRecs=" << SearchExt->delAfterCountRecs;
    Query << ", delAfterDaysOfFirstRec=" << SearchExt->delAfterDaysOfFirstRec;
    Query << ", useAsSearchTimerFrom=" << SearchExt->useAsSearchTimerFrom;
    Query << ", useAsSearchTimerTil=" << SearchExt->useAsSearchTimerTil;
    Query << ", ignoreMissingEPGCats=" << SearchExt->ignoreMissingEPGCats;
    Query << ", unmuteSoundOnSwitch=" << SearchExt->unmuteSoundOnSwitch;
    Query << " WHERE id=" << SearchExt->ID;
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;93m " << __FILE__ << '(' << __LINE__ << "): \"" << Query.str() << "\" \033[0m" << std::endl;
#endif
    int res = mysql_query(Connection, Query.str().c_str());
    if (res != 0) {
        esyslog("ERROR: Database Error %i\n", res);
        DisconnectDB(Connection);
        free(EscapedSearch);
        return false;
    }

    DisconnectDB(Connection);
    if(EscapedSearch)
        free(EscapedSearch);
    return true;
}

bool cTimerSearchMysql::LoadDB(int *LastEventID)
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;41m " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif
    bool retValue = false;
    MYSQL *Connection = mysql_init(NULL);
    MYSQL_RES *Result;
    MYSQL_ROW Row;
    std::stringstream Query;

    if (Connection == NULL)
        return false;

    if(!ConnectDB(Connection)) {
        free(Connection);
        return false; // Connection failed
    }

    Query << "SELECT id, search, useTime, startTime, stopTime, useChannel, ChannelGroup, useCase, mode, useTitle, useSubtitle, useDescription, useDuration, minDuration, maxDuration, useAsSearchTimer, useDayOfWeek, DayOfWeek, useEpisode, directory, Priority, Lifetime, MarginStart, MarginStop, useVPS, action, useExtEPGInfo, ExtEPGInfo, avoidRepeats, allowedRepeats, compareTitle, compareSubtitle, compareSummary, catvaluesAvoidRepeat, repeatsWithinDays, delAfterDays, recordingsKeep, switchMinsBefore, pauseOnNrRecordings, blacklistMode, blacklists, fuzzyTolerance, useInFavorites, menuTemplate, delMode, delAfterCountRecs, delAfterDaysOfFirstRec, useAsSearchTimerFrom, useAsSearchTimerTil, ignoreMissingEPGCats, unmuteSoundOnSwitch FROM " << Table_;
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;93m " << __FILE__ << '(' << __LINE__ << "): \"" << Query.str() << "\" \033[0m" << std::endl;
#endif
    int res = mysql_query(Connection, Query.str().c_str());
    if (res != 0) {
        esyslog("ERROR: Database Error %i\n", res);
        DisconnectDB(Connection);
        return false;
    }

    Result = mysql_store_result(Connection);

    if(Result)
    {
        unsigned int numRes = mysql_num_fields(Result);
        while((Row=mysql_fetch_row(Result)) != NULL)
        {
            std::stringstream strBuff;
            int ID = atoi(Row[0]);
            if(numRes >= 1)
                strBuff << ID << ':' << Row[1];
            for(unsigned int i=2; i<numRes; ++i)
            {
                if(Row[i])
                    strBuff << ':' << Row[i];
                else
                    strBuff << ":(null)";
            }

            cSearchExt *searchExt = new cSearchExt;
            if(searchExt->Parse(strBuff.str().c_str(), ID))
                SearchExts.cList<cSearchExt>::Add(searchExt);
        }
        mysql_free_result(Result);
        retValue = true;
    }
    else
        retValue = false;

    DisconnectDB(Connection);

    res = GetLastEventID(LastEventID);
    if (res != 0) {
        esyslog("ERROR: Database Error %i\n", res);
        return false;
    }

    if(Setup.ReelboxModeTemp==eModeServer) // On Server update Avahi-MySQL-file
        SetLastEventID(*LastEventID);
    return retValue;
}

bool cTimerSearchMysql::Sync(int *LastEventID)
{
    //std::cout << "\033[0;41m " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;

    int CurrentLastEventID=0;
    int res = GetLastEventID(&CurrentLastEventID);
    if (res != 0) {
        esyslog("ERROR: Database Error %i\n", res);
        return false;
    }

    if(*LastEventID == CurrentLastEventID)
        return false; // already up2date

    MYSQL *Connection = mysql_init(NULL);
    MYSQL_RES *Result;
    MYSQL_ROW Row;
    std::stringstream Query;

    if (Connection == NULL)
        return false;

    if(!ConnectDB(Connection)) {
        free(Connection);
        return false; // Connection failed
    }

    Query << "SELECT object_id, action FROM vdr_event WHERE id>" << *LastEventID << " AND table_name='timer_search'";
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;93m " << __FILE__ << '(' << __LINE__ << "): \"" << Query.str() << "\" \033[0m" << std::endl;
#endif
    res = mysql_query(Connection, Query.str().c_str());
    if (res != 0) {
        esyslog("ERROR: Database Error %i\n", res);
        DisconnectDB(Connection);
        return false;
    }

    Result = mysql_store_result(Connection);

    while((Row=mysql_fetch_row(Result)) != NULL)
    {
        unsigned int ID = atoi(Row[0]);
        const char *Action = Row[1];

#ifdef MYSQL_DEBUG
        std::cout << "\033[0;94m ID:" << ID << " Action:" << Action << " \033[0m" << std::endl;
#endif
        if(!strcmp(Action, "insert"))
            PutTimerSearch(ID);
        else if(!strcmp(Action, "delete"))
            RemoveTimerSearch(ID);
        else if(!strcmp(Action, "update"))
            SyncTimerSearch(ID);
    }
    mysql_free_result(Result);
    DisconnectDB(Connection);

    *LastEventID = CurrentLastEventID; // Update LastTimestamp
    if(Setup.ReelboxModeTemp==eModeServer) // On Server update Avahi-MySQL-file
        SetLastEventID(CurrentLastEventID);
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;41m " << __PRETTY_FUNCTION__ << " FINISHED! \033[0m" << std::endl;
#endif

    return true;
}

void cTimerSearchMysql::UpdateDB()
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;41m " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif
    for(cSearchExt *searchExt = SearchExts.First(); searchExt; searchExt = SearchExts.Next(searchExt))
        UpdateTimerSearch(searchExt);
}

bool cTimerSearchMysql::PutTimerSearch(unsigned int ID)
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;104m " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif
    // First search if search already exist. If yes skip it
    if(SearchExts.GetSearchFromID(ID))
        return false;

    MYSQL *Connection = mysql_init(NULL);
    MYSQL_RES *Result;
    MYSQL_ROW Row;
    std::stringstream Query;

    if (Connection == NULL)
        return false;

    if(!ConnectDB(Connection)) {
        free(Connection);
        return false; // Connection failed
    }

    Query << "SELECT id, search, useTime, startTime, stopTime, useChannel, ChannelGroup, useCase, mode, useTitle, useSubtitle, useDescription, useDuration, minDuration, maxDuration, useAsSearchTimer, useDayOfWeek, DayOfWeek, useEpisode, directory, Priority, Lifetime, MarginStart, MarginStop, useVPS, action, useExtEPGInfo, ExtEPGInfo, avoidRepeats, allowedRepeats, compareTitle, compareSubtitle, compareSummary, catvaluesAvoidRepeat, repeatsWithinDays, delAfterDays, recordingsKeep, switchMinsBefore, pauseOnNrRecordings, blacklistMode, blacklists, fuzzyTolerance, useInFavorites, menuTemplate, delMode, delAfterCountRecs, delAfterDaysOfFirstRec, useAsSearchTimerFrom, useAsSearchTimerTil, ignoreMissingEPGCats, unmuteSoundOnSwitch FROM " << Table_ << " WHERE id='" << ID << "'";
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;93m " << __FILE__ << '(' << __LINE__ << "): \"" << Query.str() << "\" \033[0m" << std::endl;
#endif
    int res = mysql_query(Connection, Query.str().c_str());
    if (res != 0) {
        esyslog("ERROR: Database Error %i\n", res);
        DisconnectDB(Connection);
        return false;
    }

    Result = mysql_store_result(Connection);

    if((Row=mysql_fetch_row(Result)) != NULL)
    {
        std::stringstream strBuff;
        unsigned int numRes = mysql_num_fields(Result);
        if(numRes)
            strBuff << Row[0];
        for(unsigned int i=1; i<numRes; ++i)
            strBuff << ':' << Row[i];

        cSearchExt *searchExt = new cSearchExt;
        if(searchExt->Parse(strBuff.str().c_str(), ID))
            SearchExts.cList<cSearchExt>::Add(searchExt);
        else
            delete searchExt;
    }
    else
        return false; // Data not found

    mysql_free_result(Result);
    DisconnectDB(Connection);
    return true;
}

bool cTimerSearchMysql::RemoveTimerSearch(unsigned int ID)
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;104m " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif
    cSearchExt *deletesearchExt = SearchExts.GetSearchFromID(ID);
    if(deletesearchExt)
        SearchExts.cList<cSearchExt>::Del(deletesearchExt);

    return true;
}

bool cTimerSearchMysql::SyncTimerSearch(unsigned int ID)
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;104m " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif
    MYSQL *Connection = mysql_init(NULL);
    MYSQL_RES *Result;
    MYSQL_ROW Row;
    std::stringstream Query;

    if (Connection == NULL)
        return false;

    if(!ConnectDB(Connection)) {
        free(Connection);
        return false; // Connection failed
    }

    Query << "SELECT id, search, useTime, startTime, stopTime, useChannel, ChannelGroup, useCase, mode, useTitle, useSubtitle, useDescription, useDuration, minDuration, maxDuration, useAsSearchTimer, useDayOfWeek, DayOfWeek, useEpisode, directory, Priority, Lifetime, MarginStart, MarginStop, useVPS, action, useExtEPGInfo, ExtEPGInfo, avoidRepeats, allowedRepeats, compareTitle, compareSubtitle, compareSummary, catvaluesAvoidRepeat, repeatsWithinDays, delAfterDays, recordingsKeep, switchMinsBefore, pauseOnNrRecordings, blacklistMode, blacklists, fuzzyTolerance, useInFavorites, menuTemplate, delMode, delAfterCountRecs, delAfterDaysOfFirstRec, useAsSearchTimerFrom, useAsSearchTimerTil, ignoreMissingEPGCats, unmuteSoundOnSwitch FROM " << Table_ << " WHERE id='" << ID << "'";
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;93m " << __FILE__ << '(' << __LINE__ << "): \"" << Query.str() << "\" \033[0m" << std::endl;
#endif
    int res = mysql_query(Connection, Query.str().c_str());
    if (res != 0) {
        esyslog("ERROR: Database Error %i\n", res);
        DisconnectDB(Connection);
        return false;
    }

    Result = mysql_store_result(Connection);

    if((Row=mysql_fetch_row(Result)) != NULL)
    {
        std::stringstream strBuff;
        unsigned int numRes = mysql_num_fields(Result);
        if(numRes)
            strBuff << Row[0];
        for(unsigned int i=1; i<numRes; ++i)
            strBuff << ':' << Row[i];

        cSearchExt *searchExt= SearchExts.GetSearchFromID(ID);
        searchExt->Parse(strBuff.str().c_str(), ID);
    }
    else
        return false; // Data not found

    mysql_free_result(Result);
    DisconnectDB(Connection);
    return true;
}

