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
 * epgsearchmysql.h
 *
 ***************************************************************************/

#ifndef EPGSEARCHMYSQL_H
#define EPGSEARCHMYSQL_H

//#include <mysql/mymysql.h> // already included in vdr/vdrmysql.h

#include <vdr/vdrmysql.h>

#include "epgsearchext.h"

struct StringTimerSearch
{
    unsigned int ID;
    char *s;
};

class cTimerSearchMysql : public cVdrMysql
{
    private:
        const char *Table_; // Name of the table in DB

    public:
        cTimerSearchMysql(const char *Username, const char *Password, const char *Database);
        ~cTimerSearchMysql();

        bool InsertTimerSearch(cSearchExt *SearchExt); // Add new element into DB
        bool DeleteTimerSearch(unsigned int ID); // Delete element from DB
        bool UpdateTimerSearch(cSearchExt *SearchExt); // Update element on DB
        bool LoadDB(int *LastEventID); // Load all data from DB
        bool Sync(int *LastEventID); // Get changed data from DB if any, then do action on VDR
        void UpdateDB(); // Update all elements in DB
        bool PutTimerSearch(unsigned int ID); // Add new element in VDR
        bool RemoveTimerSearch(unsigned int ID); // Delete element from VDR
        bool SyncTimerSearch(unsigned int ID); // Update element in VDR
};

#endif
