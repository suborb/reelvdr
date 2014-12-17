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
 * vdrmysql.h
 *
 ***************************************************************************/

#ifndef VDRMYSQL_H
#define VDRMYSQL_H

#include <mysql/mysql.h>
#include <vector>

#include "timers.h"

#define MYSQLREELUSER "reeluser"
#define MYSQLREELPWD  "reeluser"
#define MYSQLADMINUSER "root"
#define MYSQLADMINPWD  "root"

enum eEventAction { eActionUnknown, eActionInsert, eActionUpdate, eActionDelete };

struct vdr_event
{
    unsigned int ID;
    eEventAction Action;
};

struct StringTimer {
  unsigned int ID;
  char *s;
};

class cVdrMysql
{
    protected:
        char *Server_;
        char *Username_;
        char *Password_;
        char *Database_;

        bool ConnectDB(MYSQL *Connection);
        void DisconnectDB(MYSQL *Connection);
        void SetLastEventID(int LastEventID, bool force=false);

    public:
        cVdrMysql(const char *Username, const char *Password, const char *Database);
        ~cVdrMysql();

        bool SetServer(const char *Server);
        int GetLastEventID(int *id); /** set the lastEventId "id" - returns 0 if successful, the mysql-error otherwise */
};

// cVdrMysqlAdmin is used for administration of MySQL-Database
class cVdrMysqlAdmin : public cVdrMysql
{
    private:
        bool InitDatabase(bool RecreateDB=false, const char *Database=NULL);
        void GenerateTableFromTemplate(MYSQL *Connection, const char *filename);
    public:
        cVdrMysqlAdmin(const char *Username, const char *Password, const char *Database);
        ~cVdrMysqlAdmin();
        bool CreateAccount();
        bool CreateDatabase(const char *Database=NULL) { return InitDatabase(false, Database); };
        bool ClearDatabase() { return InitDatabase(true); };
        bool CreateTables();
        bool ClearEvents();
        bool TestTables();
        bool TestPermission();
};

// Timer
class cTimersMysql : public cVdrMysql
{
    private:
        const char *Table_;

    public:
        cTimersMysql(const char *Username, const char *Password, const char *Database);
        ~cTimersMysql();

        bool InsertTimer(cTimer *timer);
        bool DeleteTimer(unsigned int ID);
        bool UpdateTimer(cTimer *timer);
        bool LoadDB(std::vector<StringTimer> *StringTimers, int *LastEventID);
        bool Sync(int *LastEventID);
//        void UpdateDB();
        bool PutTimer(unsigned int ID);
        bool RemoveTimer(unsigned int ID);
        bool SyncTimer(unsigned int ID);
        void GetInstantRecordings(std::vector<cTimer*> *InstantRecordings);
};

#endif
