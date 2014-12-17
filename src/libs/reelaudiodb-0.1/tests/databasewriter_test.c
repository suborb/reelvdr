#include "databasewriter.h"
#include "createdatabaseinstance.h"
#include <iostream>
#include <sstream>

std::string itoStr(int n)
{
    std::stringstream ss;
    ss << n;
    return ss.str();
}

int main()
{
    cDatabase *db = CreateDatabaseInstance("/tmp/my_db.sqlite");
    db->CreateTable("table1");
    cDatabaseWriter writer(db);
    
    cFields field;
    cRow row;
    std::string where;
    
    field.push_back("path");
    field.push_back("available");
    
    std::string p;
    for ( int i=0 ; i < 1000; ++i) 
    {
        row.clear();
        p = std::string("path") + itoStr(i);
        row.push_back(p);
        row.push_back(i%3?"0":"1");
        where = std::string("path=\"") + p +std::string("\"");
        
        bool ok = writer.Write("table1", field, row, where);
        if (!ok)
            std::cout << "Write failed" << std::endl;
    }

    delete db;
    return 0;
}

