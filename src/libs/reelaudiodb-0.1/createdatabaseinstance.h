#ifndef CREATEDATABASEINSTANCE_H
#define CREATEDATABASEINSTANCE_H

#include "database.h"
#include <string>


/*
** Make it easier to change database backend/type
*/

enum DatabaseType { eDefaultDB, eMysqlDB, eSqliteDB };

/**
 * @brief CreateDatabaseInstance : returns a pointer to a newly created cDatabase
 *   calling function owns the pointer and has to delete it after its use.
 * @param noCreate : if true then don't create database if there is't one
 * @param type : database type
 * @return pointer to cDatabase
 */
cDatabase* CreateDatabaseInstance(std::string path,
                                  bool noCreate = true,
                                  DatabaseType type = eDefaultDB);

#endif // CREATEDATABASEINSTANCE_H
