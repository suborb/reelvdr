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
 * vdrmysql.c
 *
 ***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "thread.h"
#include "vdrmysql.h"
#include "status.h"
#include "menu.h"

#define MYSQL_TEMPLATE_PATH "/usr/share/reel/mysql/"
#define MYSQL_VDRDB_TEMPLATE "vdr-db.sql"
#define MYSQL_TIMERDB_TEMPLATE "timer-db.sql"
#define MYSQL_TIMERSEARCHDB_TEMPLATE "timer-search.sql"

static int AvahiLastEventID = -1;
static cMutex SetLastEventIDMutex;

char* GetIpAddress()
{
    int sock_fd = socket( AF_INET , SOCK_DGRAM , 0 );
    if( sock_fd != -1 )
    {
        struct ifreq ifr;
        strcpy(ifr.ifr_name, "eth0");
        if(ioctl(sock_fd, SIOCGIFADDR, &ifr) != -1)
        {
            char *IpAddress = inet_ntoa( ((struct sockaddr_in *) (&ifr.ifr_addr))->sin_addr );
            close( sock_fd );
            return IpAddress;
        }
        else
            esyslog("ERROR (%s,%d): Problem with ioctl", __FILE__, __LINE__);
    }
    else
        esyslog("ERROR (%s,%d): Creating Socket failed", __FILE__, __LINE__);
    return NULL;
}

//#########################################################################
// class cVdrMysql
//#########################################################################
cVdrMysql::cVdrMysql(const char *Username, const char *Password, const char *Database)
{
    Server_ = NULL;
    Username_ = strdup(Username);
    Password_ = Password?strdup(Password):NULL;
    Database_ = Database?strdup(Database):NULL;
}

cVdrMysql::~cVdrMysql()
{
    if(Server_)
        free(Server_);
    if(Username_)
        free(Username_);
    if(Password_)
        free(Password_);
    if(Database_)
        free(Database_);
}

bool cVdrMysql::SetServer(const char *Server)
{
    if(Server == NULL)
        return false;
    else
        free(Server_);

    Server_ = strdup(Server);

    // Test
    MYSQL *Connection = mysql_init(NULL);
    if(Connection)
    {
        if(ConnectDB(Connection))
        {
            DisconnectDB(Connection);
            return true; // Server valid
        }
        else
        {
            free(Connection);
            return false; // Connection failed
        }
    }
    return false;
};

bool cVdrMysql::ConnectDB(MYSQL *Connection)
{
    if(!Server_)
        return false; // no Server entered
    mysql_options(Connection, MYSQL_OPT_CONNECT_TIMEOUT, "5");
    if(!mysql_real_connect(Connection, Server_, Username_, Password_, Database_, 0, NULL, 0)) {
        esyslog("Failed to connect to database: Error: %s\n", mysql_error(Connection));
        return false; // Connection failed
    } else
        return true;
}

void cVdrMysql::DisconnectDB(MYSQL *Connection)
{
    mysql_close(Connection);
}

int cVdrMysql::GetLastEventID(int *id)
{
    MYSQL *Connection = mysql_init(NULL);
    MYSQL_RES *Result;
    MYSQL_ROW Row;

    if (Connection == NULL)
        return -1;

    if(!ConnectDB(Connection)) {
        free(Connection);
        return -1; // Connection failed
    }

    int res = mysql_query(Connection, "SELECT MAX(id) FROM vdr_event");
    if(res != 0) { // error
        *id = 0;
        esyslog("%s (%s:%d) mysql error: %u '%s'",
                __PRETTY_FUNCTION__, __FILE__, __LINE__,
                mysql_errno(Connection),
                mysql_error(Connection));
        DisconnectDB(Connection);
        return res;
    }

    Result = mysql_store_result(Connection);

    if(Result)
    {
        if((Row = mysql_fetch_row(Result)) != NULL)
        {
            if(Row[0])
                *id = atoi(Row[0]);
        }
    }

    mysql_free_result(Result);
    DisconnectDB(Connection);

    return 0;
}

void cVdrMysql::SetLastEventID(int LastEventID, bool force)
{
    cMutexLock MutexLock(&SetLastEventIDMutex);
    if(force || (AvahiLastEventID != LastEventID))
    {
        char command[128];
        snprintf(command, 128, "avahi-publish-avg-mysql.sh %i", LastEventID);
        if(SystemExec(command))
        {
            esyslog("Error: couldn't publish avg-mysql.service in avahi");
            AvahiLastEventID = -1;
        }
        else
            AvahiLastEventID = LastEventID;
    }

// This didn't work reliable:
// If this service is updated too often(?), avahi-daemon(?) won't publish it for a while.
// => NetClients thinks AVG is not available => MultiRoom disabled :(
#if 0
    if(AvahiLastEventID != LastEventID)
    {
        std::ofstream file("/etc/avahi/services/reel-avg-mysql.service");
        if(file.is_open())
        {
            file << "<?xml version=\"1.0\" standalone='no'?>" << std::endl;
            file << "<service-group>" << std::endl;
            file << "   <name replace-wildcards=\"yes\">%h</name>" << std::endl;
            file << "   <service protocol=\"ipv4\">" << std::endl;
            file << "      <type>_reelboxMySQL._tcp</type>" << std::endl;
            file << "      <txt-record>lasteventid=" << LastEventID << "</txt-record>" << std::endl;
            file << "   </service>" << std::endl;
            file << "</service-group>" << std::endl;
            file.close();
            AvahiLastEventID = LastEventID;
        } else
            esyslog("Error: couldn't open \"/etc/avahi/services/reel-avg-mysql.service\"");
    }
#endif
}

//#########################################################################
// class cVdrMysqlAdmin
//#########################################################################

cVdrMysqlAdmin::cVdrMysqlAdmin(const char *Username, const char *Password, const char *Database) : cVdrMysql(Username, Password, Database)
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;41m " << __FILE__ << '(' << __LINE__ << "): " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif
}

cVdrMysqlAdmin::~cVdrMysqlAdmin()
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;41m " << __FILE__ << '(' << __LINE__ << "): " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif
}

bool cVdrMysqlAdmin::InitDatabase(bool RecreateDB, const char *NewDatabase)
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;41m " << __FILE__ << '(' << __LINE__ << "): " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif
    std::stringstream Query;
    MYSQL *Connection = mysql_init(NULL);

    if (Connection == NULL)
        return false;

    if(!ConnectDB(Connection)) {
        free(Connection);
        return false;
    }

    if(RecreateDB && cVdrMysql::Database_)
    {
        // Delete database
        Query << "DROP DATABASE IF EXISTS " << cVdrMysql::Database_; 
#ifdef MYSQL_DEBUG
        std::cout << "\033[0;93m " << __FILE__ << '(' << __LINE__ << "): " << Query.str() << " \033[0m" << std::endl;
#endif
        int res = mysql_query(Connection, Query.str().c_str());
        if (res != 0) {
            esyslog("%s(%i) ERROR: Database Error %i\n", __FILE__, __LINE__, res);
            return false;
        }

        Query.str("");
    }

    if(NewDatabase)
    {
        if(cVdrMysql::Database_)
            free(cVdrMysql::Database_);
        cVdrMysql::Database_ = strdup(NewDatabase);
    }

    if(cVdrMysql::Database_)
    {
        // Create database
        Query << "CREATE DATABASE " << cVdrMysql::Database_;
#ifdef MYSQL_DEBUG
        std::cout << "\033[0;93m " << __FILE__ << '(' << __LINE__ << "): " << Query.str() << " \033[0m" << std::endl;
#endif
        int res = mysql_query(Connection, Query.str().c_str());
        if (res != 0) {
            esyslog("%s(%i) ERROR: Database Error %i\n", __FILE__, __LINE__, res);
            esyslog("%s (%s:%d) mysql error: %u '%s'",
                    __PRETTY_FUNCTION__, __FILE__, __LINE__,
                    mysql_errno(Connection),
                    mysql_error(Connection));
            DisconnectDB(Connection);
            return false;
        }

        Query.str("");
    }

    if(!cVdrMysql::Password_)
    {
        // Set password for root
        Query << "SET PASSWORD = PASSWORD('" << MYSQLADMINPWD << "')";
#ifdef MYSQL_DEBUG
        std::cout << "\033[0;93m " << __FILE__ << '(' << __LINE__ << "): " << Query.str() << " \033[0m" << std::endl;
#endif
        int res = mysql_query(Connection, Query.str().c_str());
        if (res != 0) {
            esyslog("%s(%i) ERROR: Database Error %i\n", __FILE__, __LINE__, res);
            return false;
        }

    }

    // Create tables from templates
    GenerateTableFromTemplate(Connection, MYSQL_VDRDB_TEMPLATE);
    GenerateTableFromTemplate(Connection, MYSQL_TIMERDB_TEMPLATE);
    GenerateTableFromTemplate(Connection, MYSQL_TIMERSEARCHDB_TEMPLATE);

    return true;
}

void cVdrMysqlAdmin::GenerateTableFromTemplate(MYSQL *Connection, const char *filename)
{
    std::string strBuff;
    std::stringstream fullfilename;
    std::ifstream file;

    fullfilename << MYSQL_TEMPLATE_PATH << filename;
    file.open(fullfilename.str().c_str());
    if(file.is_open()) {
        while ( file.good() ) {
            getline(file, strBuff);
            if(strBuff.length()) {
                int res = mysql_query(Connection, strBuff.c_str());
                if (res != 0) {
                    esyslog("%s(%i) ERROR: Database Error %i\n", __FILE__, __LINE__, res);
                }

            }
#ifdef MYSQL_DEBUG
            std::cout << "\033[0;93m " << __FILE__ << '(' << __LINE__ << "): " << strBuff <<"\033[0m" << std::endl;
#endif
        }
        file.close();
    }
}

bool cVdrMysqlAdmin::CreateAccount()
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;41m " << __FILE__ << '(' << __LINE__ << "): " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif
    bool retValue = false;
    MYSQL *Connection = mysql_init(NULL);
    if(ConnectDB(Connection))
    {
        int res = mysql_query(Connection, "GRANT ALL PRIVILEGES on vdr.* to 'reeluser'@'%' IDENTIFIED BY 'reeluser'");
        if (res)
            esyslog("%s(%i) ERROR: Grant 'reeluser'@'%%' => Database Error %i\n", __FILE__, __LINE__, res);

        // TODO: Find a way to detect missing grants '%' and 'localhost' seperately and split up this function
        int res2 = mysql_query(Connection, "GRANT ALL PRIVILEGES on vdr.* to 'reeluser'@'localhost' IDENTIFIED BY 'reeluser'");
        if (res2)
            esyslog("%s(%i) ERROR: Grant 'reeluser'@'localhost' => Database Error %i\n", __FILE__, __LINE__, res2);

        // If one of both was successful, then return true since one of both grant may already exists and works
        if((res==0) || (res2==0))
            retValue = true;

        DisconnectDB(Connection);
    }
    else
        free(Connection);
    return retValue;
}

bool cVdrMysqlAdmin::CreateTables()
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;41m " << __FILE__ << '(' << __LINE__ << "): " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif
    bool retValue = true;
    MYSQL *Connection = mysql_init(NULL);
    MYSQL_RES *Result;
    MYSQL_ROW Row;

    if(ConnectDB(Connection))
    {
        std::vector<std::string> Queries;
        bool CreateTable_vdr_event = true;
        bool CreateTable_timer = true;
        bool CreateTable_timer_search = true;

        // First checks, which tables already exist
        if(!mysql_query(Connection, "SHOW TABLES"))
        {
            Result = mysql_store_result(Connection);
            if(Result)
            {
                while((Row=mysql_fetch_row(Result)) != NULL)
                {
                    if(!strcmp(Row[0], "vdr_event"))
                        CreateTable_vdr_event = false;
                    else if(!strcmp(Row[0], "timer"))
                        CreateTable_timer = false;
                    else if(!strcmp(Row[0], "timer_search"))
                        CreateTable_timer_search = false;
                }
                mysql_free_result(Result);
            }
        }
        else
            retValue = false;

        if(CreateTable_vdr_event)
        {
            Queries.push_back("CREATE TABLE vdr_event (id INT NOT NULL AUTO_INCREMENT, timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL, object_id INT NOT NULL, table_name VARCHAR(16) NOT NULL, action VARCHAR(16) NOT NULL, PRIMARY KEY(id))");
        }
        if(CreateTable_timer)
        {
            Queries.push_back("CREATE TABLE timer (id INT NOT NULL AUTO_INCREMENT, flags TINYINT UNSIGNED, channel VARCHAR(128), date VARCHAR(64), start VARCHAR(4), stop VARCHAR(4), priority TINYINT, lifetime TINYINT, file VARCHAR(256), aux VARCHAR(512), ip VARCHAR(16), PRIMARY KEY(id))");
            Queries.push_back("CREATE TRIGGER insert_timer AFTER INSERT ON timer FOR EACH ROW INSERT INTO vdr_event (object_id, table_name, action) VALUES (NEW.id, 'timer', 'insert')");
            Queries.push_back("CREATE TRIGGER update_timer AFTER UPDATE ON timer FOR EACH ROW INSERT INTO vdr_event (object_id, table_name, action) VALUES (OLD.id, 'timer', 'update')");
            Queries.push_back("CREATE TRIGGER delete_timer AFTER DELETE ON timer FOR EACH ROW INSERT INTO vdr_event (object_id, table_name, action) VALUES (OLD.id, 'timer', 'delete')");
        }
        if(CreateTable_timer_search)
        {
            Queries.push_back("CREATE TABLE timer_search (id INT NOT NULL, search VARCHAR(256), useTime TINYINT, startTime VARCHAR(4), stopTime VARCHAR(4), useChannel TINYINT, ChannelGroup VARCHAR(256), useCase TINYINT, mode TINYINT, useTitle TINYINT, useSubtitle TINYINT, useDescription TINYINT, useDuration TINYINT, minDuration VARCHAR(4), maxDuration VARCHAR(4), useAsSearchTimer TINYINT, useDayOfWeek TINYINT, DayOfWeek TINYINT, useEpisode TINYINT, directory VARCHAR(512), Priority TINYINT UNSIGNED, Lifetime TINYINT UNSIGNED, MarginStart SMALLINT, MarginStop SMALLINT, useVPS TINYINT, action TINYINT, useExtEPGInfo TINYINT, ExtEPGInfo VARCHAR(512), avoidRepeats TINYINT, allowedRepeats TINYINT, compareTitle TINYINT, compareSubtitle TINYINT, compareSummary TINYINT, catvaluesAvoidRepeat INT UNSIGNED, repeatsWithinDays TINYINT, delAfterDays TINYINT, recordingsKeep TINYINT, switchMinsBefore TINYINT, pauseOnNrRecordings TINYINT, blacklistMode TINYINT, blacklists VARCHAR(512), fuzzyTolerance TINYINT, useInFavorites TINYINT, menuTemplate TINYINT, delMode TINYINT, delAfterCountRecs SMALLINT, delAfterDaysOfFirstRec SMALLINT, useAsSearchTimerFrom INT, useAsSearchTimerTil INT, ignoreMissingEPGCats TINYINT, unmuteSoundOnSwitch TINYINT, PRIMARY KEY(id))");
            Queries.push_back("CREATE TRIGGER insert_timer_search AFTER INSERT ON timer_search FOR EACH ROW INSERT INTO vdr_event (object_id, table_name, action) VALUES (NEW.id, 'timer_search', 'insert')");
            Queries.push_back("CREATE TRIGGER update_timer_search AFTER UPDATE ON timer_search FOR EACH ROW INSERT INTO vdr_event (object_id, table_name, action) VALUES (OLD.id, 'timer_search', 'update')");
            Queries.push_back("CREATE TRIGGER delete_timer_search AFTER DELETE ON timer_search FOR EACH ROW INSERT INTO vdr_event (object_id, table_name, action) VALUES (OLD.id, 'timer_search', 'delete')");
        }

        unsigned int i = 0;
        while(retValue && (i < Queries.size()))
        {
            //PRINTF("\033[0;93m %s(%i): Query(%i) - %s \033[0m\n", __FILE__, __LINE__, i, Queries.at(i).c_str());
            int res = mysql_query(Connection, Queries.at(i).c_str());
            if(res)
            {
                retValue = false;
                esyslog("%s(%i) ERROR: Database Error: %s\n", __FILE__, __LINE__, Queries.at(i).c_str());
            }
            ++i;
        }
        DisconnectDB(Connection);
    }
    else
    {
        retValue = false;
        free(Connection);
    }
    return retValue;
}

bool cVdrMysqlAdmin::ClearEvents()
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;41m " << __FILE__ << '(' << __LINE__ << "): " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif
    char Query[] = "DELETE FROM vdr_event";
    MYSQL *Connection = mysql_init(NULL);

    if (Connection == NULL)
        return false;

    if(!ConnectDB(Connection)) {
        free(Connection);
        return false;
    }

#ifdef MYSQL_DEBUG
    std::cout << "\033[0;93m " << __FILE__ << '(' << __LINE__ << "): " << Query << " \033[0m" << std::endl;
#endif
    int res = mysql_query(Connection, Query);
    if (res != 0) {
        esyslog("%s(%i) ERROR: Database Error %i\n", __FILE__, __LINE__, res);
        esyslog("%s (%s:%d) mysql error: %u '%s'",
                __PRETTY_FUNCTION__, __FILE__, __LINE__,
                mysql_errno(Connection),
                mysql_error(Connection));
        DisconnectDB(Connection);
        return false;
    }

    SetLastEventID(0, true);

    return true;
}

bool cVdrMysqlAdmin::TestTables() {
    MYSQL *Connection = mysql_init(NULL);
    MYSQL_RES *Result;
    MYSQL_ROW Row;

    if (Connection == NULL)
        return false;

    if(!ConnectDB(Connection)) {
        free(Connection);
        return false; // Connection failed
    }

    static const char *tables[] = { "vdr_event", "timer", "timer_search" };
    for(unsigned int i=0; i < sizeof(tables)/sizeof(const char*); i++) {

#ifdef MYSQL_DEBUG
        std::cout << "\033[0;41m " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif

        std::stringstream Query;
        Query << "DESCRIBE " << tables[i];
#ifdef MYSQL_DEBUG
        std::cout << "\033[0;93m \"" << Query.str() << "\" \033[0m" << std::endl;
#endif
        int res = mysql_query(Connection, Query.str().c_str());
        if (res != 0) {
            esyslog("%s(%i) ERROR: Database Error %i\n", __FILE__, __LINE__, res);
        }

        Result = mysql_store_result(Connection);

        if(Result) {
            while((Row=mysql_fetch_row(Result)) != NULL) {
                unsigned int numRes = mysql_num_fields(Result);
                if(numRes == 0) {
                    esyslog("%s(%i) ERROR: Database Error mysql_num_fields() returned 0", __FILE__, __LINE__);
                    esyslog("%s (%s:%d) mysql error: %u '%s'",
                            __PRETTY_FUNCTION__, __FILE__, __LINE__,
                            mysql_errno(Connection),
                            mysql_error(Connection));
                    DisconnectDB(Connection);
                    return false;
                }
            }
            mysql_free_result(Result);
        } else {
            DisconnectDB(Connection);
            SetLastEventID(-1);
            return false;
        }
    }
    DisconnectDB(Connection);

    return true;
}

bool cVdrMysqlAdmin::TestPermission()
{
    bool retValue = false;
    MYSQL *Connection = mysql_init(NULL);
    MYSQL_RES *Result;
    MYSQL_ROW Row;

    if(Connection == NULL)
        return false;

    if(!ConnectDB(Connection))
    {
        free(Connection);
        return false; // Connection failed
    }

    int res = mysql_query(Connection, "SHOW GRANTS FOR CURRENT_USER()");
    if(res!=0)
        esyslog("%s(%i) ERROR: Database Error %i\n", __FILE__, __LINE__, res);

    Result = mysql_store_result(Connection);
    if(Result)
    {
        while((Row=mysql_fetch_row(Result)) != NULL)
        {
            //PRINTF("\033[0;93m %s(%i): %s \033[0m\n", __FILE__, __LINE__, Row[0]);
            if(strstr(Row[0], "GRANT ALL PRIVILEGES ON `vdr`.* TO"))
                retValue = true; // needed Permission found
        }
        mysql_free_result(Result);
    }

    DisconnectDB(Connection);
    return retValue;    
}

//#########################################################################
// class cTimersMysql
//#########################################################################

cTimersMysql::cTimersMysql(const char *Username, const char *Password, const char *Database) : cVdrMysql(Username, Password, Database), Table_("timer")
{
}

cTimersMysql::~cTimersMysql()
{
}

bool cTimersMysql::InsertTimer(cTimer *timer)
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;101m " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif
    MYSQL *Connection = mysql_init(NULL);
    MYSQL_RES *Result;
    MYSQL_ROW Row;
    std::stringstream Query;
    char *EscapedFile=NULL;
    char *EscapedAux=NULL;

    if(Connection == NULL)
        return false;

    if(!ConnectDB(Connection)) {
        free(Connection);
        return false; // Connection failed
    }

    // Get local IP-Adress
    char* IpAddress = NULL;
#ifdef RBMINI
    IpAddress = GetIpAddress();
#endif

    // First escape strings
    EscapedFile = (char*)malloc((strlen(timer->File())*2+1)*sizeof(char));
    mysql_real_escape_string(Connection, EscapedFile, timer->File(), strlen(timer->File()));
    if(timer->Aux()) {
        EscapedAux = (char*)malloc((strlen(timer->Aux())*2+1)*sizeof(char));
        mysql_real_escape_string(Connection, EscapedAux, timer->Aux(), strlen(timer->Aux()));
    }
    Query << "INSERT INTO " << Table_ << " (flags, channel, date, start, stop, priority, lifetime, file, aux, ip) VALUES ( ";
    Query << timer->Flags();
    Query << ", '" << *timer->Channel()->GetChannelID().ToString(); 
    Query << "', '" << *(timer->PrintDay(timer->Day(), timer->WeekDays(), true));
    Query << "', '" << std::setw(4) << std::setfill('0') << timer->Start(); 
    Query << "', '" << std::setw(4) << std::setfill('0') << timer->Stop();
    Query << "', " << timer->Priority();
    Query << ", " << timer->Lifetime();
    Query << ", '" << EscapedFile << "', '";
    if(EscapedAux)
        Query << EscapedAux;
    if(IpAddress)
        Query << "', '" << IpAddress << "')";
    else
        Query << "', '(null)')";
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;93m \"" << Query.str() << "\" \033[0m" << std::endl;
#endif
    int res =mysql_query(Connection, Query.str().c_str());
    if (res != 0) {
        esyslog("%s(%i) ERROR: Database Error %i (%s)\n", __FILE__, __LINE__, res, Query.str().c_str());
    }
    Query.str(""); // Clear Query-string

    Query << "SELECT id FROM " << Table_ << " WHERE flags=" << timer->Flags();
    Query << " AND channel='" << *timer->Channel()->GetChannelID().ToString();
    Query << "' AND date='" << *(timer->PrintDay(timer->Day(), timer->WeekDays(), true));
    Query << "' AND start='" << std::setw(4) << std::setfill('0') << timer->Start();
    Query << "' AND stop='" << std::setw(4) << std::setfill('0') << timer->Stop();
    Query << "' AND priority=" << timer->Priority(); 
    Query << " AND lifetime=" << timer->Lifetime();
    Query << " AND file='" << EscapedFile << "' AND aux='";
    if(EscapedAux)
        Query << EscapedAux;
    if(IpAddress)
        Query << "' AND ip='" << IpAddress << '\'';
    else
        Query << "' AND ip='(null)'";
    //std::cout << "\033[0;93m \"" << Query.str() << "\" \033[0m" << std::endl;
    res = mysql_query(Connection, Query.str().c_str());
    if (res != 0) {
        esyslog("%s(%i) ERROR: Database Error %i (%s)\n", __FILE__, __LINE__, res, Query.str().c_str());
        esyslog("%s (%s:%d) mysql error: %u '%s'",
                __PRETTY_FUNCTION__, __FILE__, __LINE__,
                mysql_errno(Connection),
                mysql_error(Connection));
        DisconnectDB(Connection);
        return false;
    }

    Result = mysql_store_result(Connection);
    if(Result) {
        if((Row=mysql_fetch_row(Result)) != NULL)
            timer->SetID(atoi(Row[0]));
    } else {
        esyslog("ERROR (%s,%d): %s - \"%s\"", __FILE__, __LINE__, "Could not insert Timer into DB!", timer->File());
    }
                      
    if(EscapedFile)
        free(EscapedFile);
    if(EscapedAux)
        free(EscapedAux);
    mysql_free_result(Result);
    DisconnectDB(Connection);
    return true;
}

bool cTimersMysql::DeleteTimer(unsigned int ID)
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
        esyslog("%s(%i) ERROR: Database Error %i\n", __FILE__, __LINE__, res);
        esyslog("%s (%s:%d) mysql error: %u '%s'",
                __PRETTY_FUNCTION__, __FILE__, __LINE__,
                mysql_errno(Connection),
                mysql_error(Connection));
        DisconnectDB(Connection);
        return false;
    }

    DisconnectDB(Connection);
    return true;
}

bool cTimersMysql::UpdateTimer(cTimer *timer)
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;101m " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif
    MYSQL *Connection = mysql_init(NULL);
    std::stringstream Query;
    char *EscapedFile=NULL;
    char *EscapedAux=NULL;

    if (Connection == NULL)
        return false;

    if(!ConnectDB(Connection)) {
        free(Connection);
        return false; // Connection failed
    }

    // First escape strings
    EscapedFile = (char*)malloc((strlen(timer->File())*2+1)*sizeof(char));
    mysql_real_escape_string(Connection, EscapedFile, timer->File(), strlen(timer->File()));
    if(timer->Aux()) {
        EscapedAux = (char*)malloc((strlen(timer->Aux())*2+1)*sizeof(char));
        mysql_real_escape_string(Connection, EscapedAux, timer->Aux(), strlen(timer->Aux()));
    }

    Query << "UPDATE " << Table_ << " SET flags=" << timer->Flags();
    Query << ", channel='" << *timer->Channel()->GetChannelID().ToString();
    Query << "', date='" << *(timer->PrintDay(timer->Day(), timer->WeekDays(), true));
    Query << "', start='" << std::setw(4) << std::setfill('0') << timer->Start();
    Query << "', stop='" << std::setw(4) << std::setfill('0') << timer->Stop();
    Query << "', priority=" << timer->Priority() << ", lifetime=" << timer->Lifetime(); 
    Query << ", file='" << EscapedFile << "', aux='";
    if(EscapedAux)
        Query << EscapedAux;
    Query << "' WHERE id=" << timer->GetID();
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;93m \"" << Query.str() << "\" \033[0m" << std::endl;
#endif
    int res = mysql_query(Connection, Query.str().c_str());
    if (res != 0) {
        esyslog("%s(%i) ERROR: Database Error %i\n", __FILE__, __LINE__, res);
        esyslog("%s (%s:%d) mysql error: %u '%s'",
                __PRETTY_FUNCTION__, __FILE__, __LINE__,
                mysql_errno(Connection),
                mysql_error(Connection));
        DisconnectDB(Connection);
        return false;
    }

    if(EscapedFile)
        free(EscapedFile);
    if(EscapedAux)
        free(EscapedAux);
    DisconnectDB(Connection);
    return true;
}

bool cTimersMysql::LoadDB(std::vector<StringTimer> *StringTimers, int *LastEventID)
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;41m " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
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

    Query << "SELECT id, flags, channel, date, start, stop, priority, lifetime, file, aux FROM " << Table_;
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;93m \"" << Query.str() << "\" \033[0m" << std::endl;
#endif
    int res = mysql_query(Connection, Query.str().c_str());
    if (res != 0) {
        esyslog("%s(%i) ERROR: Database Error %i\n", __FILE__, __LINE__, res);
        esyslog("%s (%s:%d) mysql error: %u '%s'",
                __PRETTY_FUNCTION__, __FILE__, __LINE__,
                mysql_errno(Connection),
                mysql_error(Connection));
        DisconnectDB(Connection);
        return false;
    }

    Result = mysql_store_result(Connection);

    if(Result)
    {
        while((Row=mysql_fetch_row(Result)) != NULL)
        {
            int id = atoi(Row[0]);
            std::stringstream strBuff;

            unsigned int numRes = mysql_num_fields(Result);

            // replace colons ':' with '|'. Colon is used as field separator, so no colons inside field strings.
            if(numRes >= 1)
                strBuff << strreplace(Row[1],':', '|');
            for(unsigned int i=2; i<numRes; ++i)
                strBuff << ':' << strreplace(Row[i],':', '|');

            StringTimer Timer;
            Timer.ID = id;
            Timer.s = strdup(strBuff.str().c_str());
            StringTimers->push_back(Timer);
        }
        mysql_free_result(Result);
    } else {
        DisconnectDB(Connection);
        SetLastEventID(-1);
        return false;
    }
    DisconnectDB(Connection);

    res = GetLastEventID(LastEventID);
    if (res != 0) {
        esyslog("%s(%i) ERROR: Database Error %i\n", __FILE__, __LINE__, res);
        return false;
    }
        
    if(Setup.ReelboxModeTemp==eModeServer) // On Server update Avahi-MySQL-file
        SetLastEventID(*LastEventID);
    return true;
}

bool cTimersMysql::Sync(int *LastEventID)
{
    //std::cout << "\033[0;41m " << __FILE__ << "(" << __LINE__ << "): " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;

    int CurrentLastEventID=0;
    int res = GetLastEventID(&CurrentLastEventID);
    if (res != 0) {
        esyslog("%s(%i) ERROR: Database Error %i\n", __FILE__, __LINE__, res);
        return false;
    }

    if(*LastEventID == CurrentLastEventID)
        return false; // Already done!

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

    Query << "SELECT object_id, action FROM vdr_event WHERE id>" << *LastEventID << " AND table_name='timer'";
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;93m \"" << Query.str() << "\" \033[0m" << std::endl;
#endif
    res = mysql_query(Connection, Query.str().c_str());
    if (res != 0) {
        esyslog("%s(%i) ERROR: Database Error %i\n", __FILE__, __LINE__, res);
        esyslog("%s (%s:%d) mysql error: %u '%s'",
                __PRETTY_FUNCTION__, __FILE__, __LINE__,
                mysql_errno(Connection),
                mysql_error(Connection));
        DisconnectDB(Connection);
        return false;
    }

    Result = mysql_store_result(Connection);

    if(Result)
    {
        while((Row=mysql_fetch_row(Result)) != NULL)
        {
            unsigned int ID = atoi(Row[0]);
            const char *Action = Row[1];

#ifdef MYSQL_DEBUG
            std::cout << "\033[0;94m ID:" << ID << " Action: " << Action << " \033[0m" << std::endl;
#endif
            if(!strcmp(Action, "insert"))
            {
                if(!Timers.GetTimerByID(ID))
                    PutTimer(ID);
                else
                    esyslog("ERROR: Timer(ID=%i) already exists!\n", ID);
            }
            else if(!strcmp(Action, "delete"))
                RemoveTimer(ID);
            else if(!strcmp(Action, "update"))
                SyncTimer(ID);
        }
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

#if 0
void cTimersMysql::UpdateDB()
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;41m " << __FILE__ << '(' << __LINE__ << "): " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif
    MYSQL *Connection = mysql_init(NULL);
    std::stringstream Query;
    char *EscapedFile=NULL;
    char *EscapedAux=NULL;
    
    if (Connection == NULL)
        return;

    if(!ConnectDB(Connection)) {
        free(Connection);
        return; // Connection failed
    }

    for (cTimer *timer = Timers.First(); timer; timer = Timers.Next(timer)) {
        Query.str(""); // Clear Query-string

        // First escape strings
        EscapedFile = (char*)malloc((strlen(timer->File())*2+1)*sizeof(char));
        mysql_real_escape_string(Connection, EscapedFile, timer->File(), strlen(timer->File()));
        if(timer->Aux()) {
            EscapedAux = (char*)malloc((strlen(timer->Aux())*2+1)*sizeof(char));
            mysql_real_escape_string(Connection, EscapedAux, timer->Aux(), strlen(timer->Aux()));
        }

        Query << "UPDATE " << Table_ << " SET flags=" << timer->Flags();
        Query << ", channel='" << *timer->Channel()->GetChannelID().ToString();
        Query << "', date='" << *(timer->PrintDay(timer->Day(), timer->WeekDays(), true));
        Query << "', start='" << std::setw(4) << std::setfill('0') << timer->Start();
        Query << "', stop='" << std::setw(4) << std::setfill('0') << timer->Stop();
        Query << "', priority=" << timer->Priority();
        Query << ", lifetime=" << timer->Lifetime();
        Query << ", file='" << EscapedFile << "', aux='";
        if(EscapedAux)
            Query << EscapedAux;
        Query << "' WHERE id=" << timer->GetID();
#ifdef MYSQL_DEBUG
        std::cout << "\033[0;93m " << __FILE__ << "(" << __LINE__ << "): \"" << Query.str() << "\" \033[0m" << std::endl;
#endif
        int res = mysql_query(Connection, Query.str().c_str());
        if (res != 0) {
            esyslog("%s(%i) ERROR: Database Error %i\n", __FILE__, __LINE__, res);
        }

        if(EscapedFile)
            free(EscapedFile);
        if(EscapedAux)
            free(EscapedAux);
    }
    DisconnectDB(Connection);
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;41m " << __PRETTY_FUNCTION__ << " FINISHED! \033[0m" << std::endl;
#endif
}
#endif

bool cTimersMysql::PutTimer(unsigned int ID)
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

    Query << "SELECT flags, channel, date, start, stop, priority, lifetime, file, aux FROM " << Table_ << " WHERE id='" << ID << "'";
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;93m \"" << Query.str() << "\" \033[0m" << std::endl;
#endif
    int res = mysql_query(Connection, Query.str().c_str());
    if (res != 0) {
        esyslog("%s(%i) ERROR: Database Error %i\n", __FILE__, __LINE__, res);
        esyslog("%s (%s:%d) mysql error: %u '%s'",
                __PRETTY_FUNCTION__, __FILE__, __LINE__,
                mysql_errno(Connection),
                mysql_error(Connection));
        DisconnectDB(Connection);
        return false;
    }

    Result = mysql_store_result(Connection);

    if(Result)
    {
        if((Row=mysql_fetch_row(Result)) != NULL)
        {
            std::stringstream strBuff;
            unsigned int numRes = mysql_num_fields(Result);
            if(numRes)
                strBuff << Row[0];
            for(unsigned int i=1; i<numRes; ++i)
                strBuff << ':' << Row[i];

            cTimer *timer = new cTimer;
            if(timer->Parse(strBuff.str().c_str(), ID))
            {
                Timers.cConfig<cTimer>::Add(timer);
                cStatus::MsgTimerChange(timer, tcAdd);
            }
            else
                delete timer;
        }
        else {
            esyslog("%s(%i) ERROR: Database Error mysql_fetch_row() returned NULL", __FILE__, __LINE__);
            esyslog("%s (%s:%d) mysql error: %u '%s'",
                    __PRETTY_FUNCTION__, __FILE__, __LINE__,
                    mysql_errno(Connection),
                    mysql_error(Connection));
            DisconnectDB(Connection);
            return false; // Data not found
        }
    }
    else
    {
        mysql_free_result(Result);
        DisconnectDB(Connection);
        return false; // No Result
    }

    mysql_free_result(Result);
    DisconnectDB(Connection);
    return true;
}

bool cTimersMysql::RemoveTimer(unsigned int ID)
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;104m " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif
    cTimer *deletetimer = Timers.GetTimerByID(ID);
    if(deletetimer)
    {
        if(deletetimer->Recording())
        {
            deletetimer->Skip();
            cRecordControls::Process(time(NULL));
        }
#ifdef MYSQL_DEBUG
        std::cout << "\033[0;38m Timer found - Deleting! \033[0m" << std::endl;
#endif
        cStatus::MsgTimerChange(deletetimer, tcDel);
        Timers.cConfig<cTimer>::Del(deletetimer);
    }

    return true;
}

bool cTimersMysql::SyncTimer(unsigned int ID)
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

    Query << "SELECT flags, channel, date, start, stop, priority, lifetime, file, aux FROM " << Table_ << " WHERE id='" << ID << "'";
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;93m \"" << Query.str() << "\" \033[0m" << std::endl;
#endif
    int res = mysql_query(Connection, Query.str().c_str());
    if (res != 0) {
        esyslog("%s(%i) ERROR: Database Error %i\n", __FILE__, __LINE__, res);
        esyslog("%s (%s:%d) mysql error: %u '%s'",
                __PRETTY_FUNCTION__, __FILE__, __LINE__,
                mysql_errno(Connection),
                mysql_error(Connection));
        DisconnectDB(Connection);
        return false;
    }

    Result = mysql_store_result(Connection);

    if(Result)
    {
        if((Row=mysql_fetch_row(Result)) != NULL)
        {
            std::stringstream strBuff;
            unsigned int numRes = mysql_num_fields(Result);
            if(numRes)
                strBuff << Row[0];
            for(unsigned int i=1; i<numRes; ++i)
                strBuff << ':' << Row[i];

            cTimer *timer = Timers.GetTimerByID(ID);
            if (timer)
            {
                /* Donot update DB, as we have just got the changes from it
                 * cTimer::Parse() updates database when it's ID is non-zero
                 * Hence set it's ID = 0 before Parse()
                 */
                timer->SetID(0); 

                if(timer->Parse(strBuff.str().c_str(), ID))
                    Timers.SetModified();
            }
        }
        else {
            esyslog("%s(%i) ERROR: Database Error mysql_fetch_row() returned NULL", __FILE__, __LINE__);
            esyslog("%s (%s:%d) mysql error: %u '%s'",
                    __PRETTY_FUNCTION__, __FILE__, __LINE__,
                    mysql_errno(Connection),
                    mysql_error(Connection));
            DisconnectDB(Connection);
            return false; // Data not found
        }
    }
    else
    {
        mysql_free_result(Result);
        DisconnectDB(Connection);
        return false; // No Result
    }

    mysql_free_result(Result);
    DisconnectDB(Connection);
    return true;
}

void cTimersMysql::GetInstantRecordings(std::vector<cTimer*> *InstantRecordings)
{
#ifdef MYSQL_DEBUG
    std::cout << "\033[0;101m " << __PRETTY_FUNCTION__ << " \033[0m" << std::endl;
#endif
    InstantRecordings->clear();
    char *IpAddress = NULL;
#ifdef RBMINI
    IpAddress = GetIpAddress();
#endif
    if(IpAddress || (Setup.ReelboxMode == eModeServer))
    {
        MYSQL *Connection = mysql_init(NULL);
        MYSQL_RES *Result;
        MYSQL_ROW Row;
        std::stringstream Query;

        if (Connection)
        {
            if(ConnectDB(Connection))
            {
                if(Setup.ReelboxMode == eModeClient) // Client
                    Query << "SELECT id, flags FROM " << Table_ << " WHERE ip='" << IpAddress << '\'';
                else // Server
                    Query << "SELECT id, flags FROM " << Table_ << " WHERE ip='(null)'";
#ifdef MYSQL_DEBUG
                std::cout << "\033[0;93m \"" << Query.str() << "\" \033[0m" << std::endl;
#endif
                int res = mysql_query(Connection, Query.str().c_str());
                if (res == 0)
                {
                    Result = mysql_store_result(Connection);

                    while((Row=mysql_fetch_row(Result)) != NULL)
                    {
                        int flags = atoi(Row[1]);
                        if(flags&tfInstant)
                            InstantRecordings->push_back(Timers.GetTimerByID(atoi(Row[0])));
                    }
                    mysql_free_result(Result);
                }
                else
                    esyslog("%s(%i) ERROR: Database Error %i\n", __FILE__, __LINE__, res);

                DisconnectDB(Connection);
            }
            else
                free(Connection);
        }
    }
}

