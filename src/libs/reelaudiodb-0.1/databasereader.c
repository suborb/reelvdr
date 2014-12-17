#include "databasereader.h"

cDatabaseReader::cDatabaseReader(cDatabase *db_) : db(db_)
{
}

cResults cDatabaseReader::Query(const std::string& table, const cQuery &q) const
{
    return db->Query(table, q);
}
