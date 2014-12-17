#ifndef SQLITEDATABASE_H
#define SQLITEDATABASE_H
#include "database.h"
#include <sqlite3.h>

class cSqliteDatabase : public cDatabase
{
private:
    std::string MakeSqlQuery(std::string cmd, const cQuery& q);
    
protected:
    sqlite3 *db;
    virtual bool Open();
    virtual bool Close();
    
    bool ExecuteSQLQuery(const std::string&, cResults *result = NULL) const;
public:
    cSqliteDatabase(std::string path_, bool noCreate=true);
    virtual ~cSqliteDatabase() { Close();}

    // read or query the database
    virtual cResults Query(const std::string& table, const cQuery&q);
    virtual unsigned int CountHits(const cQuery& q) const { return 0;}
    
    // add new rows into database
    virtual bool Insert(const std::string& table, const cFields& field, const cRow& row);
    
    // update fails if an "identical" entry is already present 
    // (or a PRIMARY KEY is duplicated)
    virtual bool Update(const std::string& table, const cFields& field, const cRow& row, const std::string & where);
    
    // creates a table with schema : DB_column *databaseSchema
    virtual bool CreateTable(const std::string& table);
};


#endif // SQLITEDATABASE_H
