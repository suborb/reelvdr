#ifndef DATABASE_H
#define DATABASE_H
#include <vector>
#include <string>
#include "databaseschema.h"
#include "query.h"


/// class to make a connection and keeps it till it is destroyed
/// provides functions to read/query and write into database
class cDatabase 
{
protected:
    // path to "database"
    std::string path;
    virtual bool Open() = 0;
    virtual bool Close() = 0;
    
public:
    cDatabase(std::string path_, bool noCreate=true);
    virtual ~cDatabase();
    
    // read or query the database
    virtual cResults Query(const std::string& table, const cQuery&q) = 0;
    virtual unsigned int CountHits(const cQuery& q) const = 0;
    
    // add new rows into database
    virtual bool Insert(const std::string& table, const cFields& field, const cRow& row) = 0;
    
    // update fails if an "identical" entry is already present 
    // (or a PRIMARY KEY is duplicated)
    virtual bool Update(const std::string& table, const cFields& field, const cRow& row, const std::string& where) = 0;
    
    // creates a table with schema : DB_column *databaseSchema
    virtual bool CreateTable(const std::string& table) = 0;
    
};


#endif // DATABASE_H
