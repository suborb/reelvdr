#include "createdatabaseinstance.h"
#include "databasereader.h"
#include <iostream>


int main()
{
    cDatabase *db = CreateDatabaseInstance("my_db");
    cDatabaseReader reader(db);
    
    cQuery q;
    q.AddField("path");
    q.AddField("available");
    q.AddWhereClause("path like '%10%'");
    q.AddWhereClause("path like '%2%'");
    
    cResults r = reader.Query("table1", q);
    
    std::vector<std::string>::const_iterator f = r.fields.begin();
    for ( ; f != r.fields.end(); ++f)
        std::cout << *f << "|" ;
    std::cout << std::endl;
    
    std::vector<cRow>::const_iterator row =  r.results.begin();
    cRow::const_iterator r1;
    for ( ; row != r.results.end(); ++row)
    {
        r1 = (*row).begin();
        for ( ; r1 != (*row).end(); ++r1)
            std::cout << *r1 << " ";
        
        std::cout << std::endl;
    }

    if (!r.errorMesg.empty())
        std::cout << r.sql_errorno << " SQL Error: " << r.errorMesg << std::endl;
    
    delete db;
    return 0;
}
