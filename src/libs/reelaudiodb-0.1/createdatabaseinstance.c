#include "createdatabaseinstance.h"
#include "mysqldatabase.h"
#include "sqlitedatabase.h"

cDatabase* CreateDatabaseInstance(std::string path, bool noCreate, DatabaseType type)
{
    cDatabase *db = NULL;
    switch (type)
    {
    case eMysqlDB:
        db = new cMysqlDatabase(path, noCreate); break;

    case eSqliteDB:
        db = new cSqliteDatabase(path, noCreate); break;

        /* default type */
    case eDefaultDB:
        db = new cMysqlDatabase(path, noCreate); break;

    default:
        break;
    }

    return db;
}
