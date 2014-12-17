#ifndef MYSQLDATABASE_H
#define MYSQLDATABASE_H
#include "database.h"
#include <mysql/mysql.h>
#include "thread.h"

class cMysqlDatabase : public cDatabase
{
private:
    std::string MakeSqlQuery(std::string cmd, const cQuery& q);
    bool noCreate;
protected:
    MYSQL *mysql;
    virtual bool Open();
    virtual bool Close();
    bool CreateDatabase(const std::string& dbName) const;

    cMutex mutex;
    bool ExecuteSQLQuery(const std::string&, cResults *result = NULL);
public:
    cMysqlDatabase(std::string path_, bool noCreate=true);
    virtual ~cMysqlDatabase() { Close();}

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


#endif // MYSQLDATABASE_H
