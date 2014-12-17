#include "sqlitedatabase.h"
#include <iostream>
#include <unistd.h>

// http://www.sqlite.org/faq.html#q14
// single quotes are "escaped" by adding another single quote next to it
std::string EscapeSingleQuotes(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    std::string result;

    for ( ; it != s.end(); ++it)
    {
        result.push_back(*it);

        if (*it =='\'')
            result.push_back('\'');
    } // for

    return result;
}

// squash a deque of strings to a comma seperated string
// fields ==> no double or single quotes around strings
std::string ToANDSeperatedFields(const cFields& vec)
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
std::string ToCommaSeperatedFields(const cFields& vec)
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
std::string ToCommaSeperatedValues(const cRow &vec)
{
    std::string str;
    cRow::const_iterator it = vec.begin();
    
    while (it != vec.end())
    {
        str += "\"" + EscapeSingleQuotes(*it) + "\"";
        ++it;
        
        if (it != vec.end())
            str += " ,";
        else
            break;
    } // while
    
    return str;
}


cSqliteDatabase::cSqliteDatabase(std::string path_, bool noCreate) : cDatabase(path_, noCreate)
{
    Open();
}

bool cSqliteDatabase::Open()
{
    return  SQLITE_OK == sqlite3_open(path.c_str(), &db);
}

bool cSqliteDatabase::Close()
{
    return SQLITE_OK == sqlite3_close(db);
}


int sql_callback(void* p, int argc, char** argv, char** columns)
{
    cResults *result = (cResults*) p;
    if (!p) return SQLITE_ABORT; /* any non-zero value to indicate error */
    
    result->fields.clear();
    cRow row;
    
    // create a cRow from result
    for (int i = 0; i < argc ; ++i) 
    {
        row.push_back(argv[i]?argv[i]:"");
        result->fields.push_back(columns[i]?columns[i]:"");
    }
    
    result->results.push_back(row);
    
    return SQLITE_OK;
}


// UPDATE <table name> SET column1=value1, column2=value2 WHERE columnx=valueX
bool cSqliteDatabase::Update(const std::string& table, const cFields &field, const cRow &row, const std::string& where)
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
        set_str += *f + "=\'" + EscapeSingleQuotes(*r) + "\'";  // column="value"
        
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
bool cSqliteDatabase::Insert(const std::string& table, const cFields &field, const cRow &row)
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


bool cSqliteDatabase::ExecuteSQLQuery(const std::string& query, cResults *results) const
{
    char* errmsg = NULL;
    int ok, trys = 0;

    do
    {
       ok = sqlite3_exec(db, query.c_str(), results?sql_callback:NULL, results, &errmsg);
        usleep(10000);
        trys++;
    }
    while((ok == SQLITE_BUSY || ok == SQLITE_LOCKED) && trys < 10);

    if (ok != SQLITE_OK && results)
    {
        results->errorMesg = errmsg?errmsg:"no error message set";
        results->sql_errorno = ok;
    }

    //std::cerr << sqlQuery << std::endl;
    if (ok != SQLITE_OK)
    {
        std::cerr << ok << " " << errmsg << " '" << query << "'" << std::endl;
    }

    return SQLITE_OK == ok;
}

// SELECT column1, column2 FROM <table name> WHERE <clauses>
bool cSqliteDatabase::Query(cResults &result, const std::string& table , const cQuery &q)
{
    std::string sqlQuery;

    std::string fields_str = ToCommaSeperatedFields(q.Fields());
    
    sqlQuery = "SELECT " + fields_str + " FROM " + table;

    if (!q.WhereClauses().empty())
        sqlQuery += std::string(" WHERE ") + ToANDSeperatedFields(q.WhereClauses());

    if (!q.OptionClause().empty())
        sqlQuery += std::string(" ") + q.OptionClause();


    return ExecuteSQLQuery(sqlQuery, &result);
}


// CREATE TABLE <table name> ( col1 col1_type, col2 col2_type, col3 col3_type ...)
bool cSqliteDatabase::CreateTable(const std::string &table)
{
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
