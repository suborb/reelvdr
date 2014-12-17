#include "mysqldatabase.h"
#include <iostream>

// squash a deque of strings to a comma seperated string
// fields ==> no double or single quotes around strings
static std::string ToANDSeperatedFields(const cFields& vec)
{
    std::string str;
    cFields::const_iterator it = vec.begin();
    
    while (it != vec.end())
    {
        str += *it;
        ++it;
        
        if (it != vec.end())
            str += " AND ";
        else
            break;
    } // while
    
    return str;
}



// squash a deque of strings to a comma seperated string
// fields ==> no double or single quotes around strings
static std::string ToCommaSeperatedFields(const cFields& vec)
{
    std::string str;
    cFields::const_iterator it = vec.begin();
    
    while (it != vec.end())
    {
        str += *it;
        ++it;
        
        if (it != vec.end())
            str += " ,";
        else
            break;
    } // while
    
    return str;
}

// squash a deque of strings to a comma seperated string
// values ==>  double quotes around strings in the result string
static std::string ToCommaSeperatedValues(const cRow &vec)
{
    std::string str;
    cRow::const_iterator it = vec.begin();

    while (it != vec.end())
    {
        str += "\"" + *it + "\"";
        ++it;
        
        if (it != vec.end())
            str += " ,";
        else
            break;
    } // while
    
    return str;
}


cMysqlDatabase::cMysqlDatabase(std::string path_, bool noCreate_)
    : cDatabase(path_, noCreate_), noCreate(noCreate_)
{
    Open();
}

bool cMysqlDatabase::CreateDatabase(const std::string &dbName) const
{
    MYSQL m;
    int ret;
    std::string cmd;

    mysql_init(&m);

    MYSQL* mptr = mysql_real_connect (&m, "127.0.0.1", "root", "root", NULL, 0, NULL, 0);
    if (!mptr) goto breach;

    cmd ="CREATE DATABASE IF NOT EXISTS " + dbName;

    ret = mysql_query(&m , cmd.c_str());
    std::cerr << cmd << " returned " << ret << std::endl;
    if (ret) goto breach;

    // give privileges to 'reeluser'
    cmd = std::string("GRANT ALL PRIVILEGES on ")
            + dbName
            + std::string(".* to 'reeluser'@'%' IDENTIFIED BY 'reeluser'");

    ret = mysql_query(&m , cmd.c_str());
    std::cerr << cmd << " returned " << ret << std::endl;
    if (ret) goto breach;

    mysql_close(&m);
    return true;

breach:
    mysql_close(&m);
    return false;
}

bool cMysqlDatabase::Open()
{
    cMutexLock mutexLock(&mutex);
    mysql = mysql_init(NULL);
    if (!mysql)
        return false;

    mysql_library_init(0, NULL, NULL);

    mysql_options (mysql, MYSQL_OPT_CONNECT_TIMEOUT, "5");

    std::string databaseName;
    std::string::size_type idx = path.find_last_of('/');
    if (idx != std::string::npos)
        databaseName = path.substr(idx+1);
    else
        databaseName = path;

    // create the database
    if (!noCreate && !CreateDatabase(databaseName))
        return false;

    MYSQL *m = mysql_real_connect (mysql, "127.0.0.1", "reeluser", "reeluser", databaseName.c_str(), 0, NULL, 0);
    if (!m)
        return false;

    return true;
}

bool cMysqlDatabase::Close()
{
    mysql_close(mysql);
    return true;
}


// UPDATE <table name> SET column1=value1, column2=value2 WHERE columnx=valueX
bool cMysqlDatabase::Update(const std::string& table, const cFields &field, const cRow &row, const std::string& where)
{
    // number of columns and values mismatch
    if (field.size() != row.size())
        return false;
    
    // fields and rows into a single comma seperate string
    // column1=value1, column2=value2
    cFields::const_iterator f = field.begin();
    cRow::const_iterator r = row.begin();
    std::string set_str;
    while ( r!= row.end() && f!=field.end()) 
    {
        char* esc_str = new char[r->size()*2 + 1];
        mysql_real_escape_string(mysql, esc_str, r->c_str(), r->size());
        set_str += *f + "=\"" + std::string(esc_str) + "\"";  // column="value"
        delete [] esc_str;

        ++r; ++f;
        
        if (r!= row.end() && f!=field.end()) 
            set_str += " ,";
        else 
            break;
    }
    
    // no update clause got
    if (set_str.empty())
        return false;
    
    std::string sqlQuery = "UPDATE " + table 
                           + " SET " + set_str 
                           + (where.empty()?" ":std::string(" WHERE ") + where);

    return ExecuteSQLQuery(sqlQuery);
}


// INSERT INTO <table name> (column1, column2...) VALUES (value1, value2, ...)
bool cMysqlDatabase::Insert(const std::string& table, const cFields &field, const cRow &row)
{
    std::string sqlQuery;
    cFields::const_iterator f = field.begin();
    
    // convert deque of strings into a comma seperated string (str1, str2, str3 ...)
    std::string fields_str = ToCommaSeperatedFields(field);
    std::string values_str = ToCommaSeperatedValues(row);
    
    
    if (fields_str.size()) fields_str = "( " + fields_str + " )";
    if (values_str.size()) values_str = "( " + values_str + " )";
    
    sqlQuery = std::string("INSERT INTO ") + table + " " + fields_str + " VALUES "
            + values_str;
    

    return ExecuteSQLQuery(sqlQuery);
}


bool cMysqlDatabase::ExecuteSQLQuery(const std::string& query, cResults *results)
{
    cMutexLock mutexLock(&mutex);

    int ret = mysql_query (mysql, query.c_str());

    if (ret != 0)
    {
        const char* err = mysql_error(mysql);
        std::cerr << query << " returned " << ret <<  std::endl
                  << "MYSQL error:" << err << std::endl;
        if (results)
        {
            results->errorMesg = err?err:"unknown error"; // why does this result in crash ?
            results->sql_errorno = mysql_errno(mysql);
        }

        return false;
    }

    MYSQL_RES *res = mysql_store_result(mysql);
    if (res)
    {
        MYSQL_ROW row;
        MYSQL_FIELD *fields = mysql_fetch_fields(res);
        unsigned int num_fields = mysql_num_fields(res);
        unsigned int i;

        // get fields names
        results->fields.clear();
        for (i=0; i < num_fields; ++i)
        {
            results->fields.push_back(fields[i].name?fields[i].name:"");
        }

        // get the resulting rows
        while ((row = mysql_fetch_row(res)))
        {
            cRow r;
            for (i=0; i < num_fields; ++i)
            {
                r.push_back(row[i]?row[i]:"");
            }

            results->results.push_back(r);
        }

        mysql_free_result(res);
    }
    else // no result
    {
        // number of fields expected in the result
        unsigned int num_fields = mysql_field_count(mysql);

        if (num_fields != 0) // error!
        {
            const char* err = mysql_error(mysql);
            if (results)
            {
                results->errorMesg = err?err:"unknown error"; // why does this result in crash ?
                results->sql_errorno = mysql_errno(mysql);
            }

            return false;
        }

        unsigned int num_rows_affected = mysql_affected_rows(mysql);
        //std::cerr<< "rows affected: " << num_rows_affected << std::endl;
    }

    return true;
}

// SELECT column1, column2 FROM <table name> WHERE <clauses>
bool cMysqlDatabase::Query(cResults &result, const std::string& table , const cQuery &q)
{
    std::string sqlQuery;

    // error: table name empty
    if (table.empty())
        return false;

    std::string fields_str = ToCommaSeperatedFields(q.Fields());

    // error: no fields specified
    if (fields_str.empty())
        return false;
    
    sqlQuery = "SELECT " + fields_str + " FROM " + table;

    if (!q.WhereClauses().empty())
        sqlQuery += std::string(" WHERE ") + ToANDSeperatedFields(q.WhereClauses());

    if (!q.OptionClause().empty())
        sqlQuery += std::string(" ") + q.OptionClause();

    return ExecuteSQLQuery(sqlQuery, &result);
}


// CREATE TABLE <table name> ( col1 col1_type, col2 col2_type, col3 col3_type ...)
bool cMysqlDatabase::CreateTable(const std::string &table)
{
    if (table.empty())
        return false;

    std::string sqlQuery = std::string ("CREATE TABLE ") + table + " ";
    
    std::string columns;
    int i = 0;
    
    // construct column string from schema
    // "col1 col1_type, col2 col2_type, col3 col3_type ..."
    while (databaseSchema[i].name && databaseSchema[i].type)
    {
        columns += std::string(databaseSchema[i].name) 
                + std::string(" ")
                + std::string(databaseSchema[i].type);
        
        ++i;
        
        // add comma after "col col_type"
        // except if there are no more columns to append
        if (databaseSchema[i].name && databaseSchema[i].type)
            columns += ",";
        
    } // while
    
    // no columns in schema; table creation failed
    if (columns.empty())
        return false;
    
    sqlQuery += std::string ("(") + columns + std::string(")");
    
    // execute the query
    return ExecuteSQLQuery(sqlQuery);
}
