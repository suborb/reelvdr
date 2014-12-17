#ifndef DATABASEWRITER_H
#define DATABASEWRITER_H

#include "database.h"
#include <deque>
#include <string>


class cDatabaseWriter
{
private:
    cDatabase *db;
public:
    cDatabaseWriter(cDatabase *db_);
    bool Write(const std::string& table, const cFields&, const cRow&, const std::string&where);
    bool Update(const std::string& table, const cFields&, const cRow&, const std::string&where);
};

#endif // DATABASEWRITER_H
