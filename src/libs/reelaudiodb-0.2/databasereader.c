#include "databasereader.h"

cDatabaseReader::cDatabaseReader(cDatabase *db_) : db(db_)
{
}

bool cDatabaseReader::Query(cResults &results, const std::string& table, const cQuery &q) const
{
    return db->Query(results, table, q);
}
