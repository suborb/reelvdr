#ifdef _WIN32
#pragma warning(disable:4786)
#endif
/**
 **	Query.h
 **
 **	Published / author: 2001-02-15 / grymse@alhem.net
 **/

/*
Copyright (C) 2001  Anders Hedstrom

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef _QUERY_H
#define _QUERY_H

#include <string>
#include <map>
#ifdef WIN32
typedef unsigned __int64 uint64_t;
typedef __int64 int64_t;
#else
#include <stdint.h>
#endif


class Query 
{
public:
	Query(Database& dbin);
	Query(Database& dbin,const std::string& sql);
	~Query();

	bool Connected();
	Database& GetDatabase();
	const std::string& GetLastQuery();

	// execute() returns true if query is successful
	bool execute(const std::string& sql);

// methods using db specific api calls

	MYSQL_RES *get_result(const std::string& sql);
	void free_result();
	MYSQL_ROW fetch_row();
	my_ulonglong insert_id();
	long num_rows();
	std::string GetError();
	int GetErrno();

// data retreival methods
	bool is_null(int x);

	// string
	const char *get_string(const std::string& sql);
	const char *getstr(int x);
	const char *getstr();
	// integer
	long get_count(const std::string& sql);
	long getval(int x);
	long getval();
	unsigned long getuval(int x);
	unsigned long getuval();
	int64_t getbigint(int x);
	int64_t getbigint();
	uint64_t getubigint(int x);
	uint64_t getubigint();
	// float
	double get_num(const std::string& sql);
	double getnum(int x);
	double getnum();

//MYSQL_FIELD *	STDCALL mysql_fetch_field(MYSQL_RES *result);
/*
	MYSQL_FIELD *fetch_field();
	const char *fetch_fieldname();
*/
	std::string safestr(const std::string& );
	std::string unsafestr(const std::string& );
	std::string xmlsafestr(const std::string& );

	int64_t a2bigint(const std::string& );
	uint64_t a2ubigint(const std::string& );
	//

protected:
	Query(Database *dbin);
	Query(Database *dbin,const std::string& sql);
private:
	Database& m_db;
	Database::OPENDB *odb;
	MYSQL_RES *res;
	MYSQL_ROW row;
	short rowcount;
	std::string m_tmpstr;
	std::string m_last_query;
};


#endif // _QUERY_H
