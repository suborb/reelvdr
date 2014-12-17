#ifndef DATABASEREADER_H
#define DATABASEREADER_H
#include "database.h"
#include <vector>
#include <string>


/**
 * @brief The cDatabaseReader class
 *  takes a database and table name
 *  The class can run queries on this table and return results.
 */

class cDatabaseReader
{
    cDatabase *db;
public:
    // a connection to already opened database
    cDatabaseReader(cDatabase *db);
    cResults Query(const std::string &, const cQuery& q) const;
    
};

#endif // DATABASEREADER_H
