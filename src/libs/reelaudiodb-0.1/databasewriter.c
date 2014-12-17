#include "databasewriter.h"

cDatabaseWriter::cDatabaseWriter(cDatabase *db_): db(db_)
{
    
}

bool cDatabaseWriter::Write(const std::string& table, const cFields& field, const cRow& row, const std::string& where)
{
    // try to insert fields first, if unsuccessfull try updating
    bool ok = false;
    
    //insert
    ok = db->Insert(table, field, row);
    
    if (!where.empty() && !ok)
        ok = db->Update(table, field, row, where);

    return ok;
}

bool cDatabaseWriter::Update(const std::string &table, const cFields &field, const cRow & row, const std::string &where)
{
    bool ok = false;
    
    if (!where.empty())
        ok = db->Update(table, field, row, where);
    
    return ok;
}

