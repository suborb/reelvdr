#include "analyzer.h"
#include "createdatabaseinstance.h"
#include "databasereader.h"
#include "databasewriter.h"
#include "tools.h"
#include <iostream>

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage:\n\t" << argv[0] << "  path_to_database" << std::endl;
        return 1;
    }

    cDatabase *db = CreateDatabaseInstance(argv[1]);
    cDatabaseReader reader(db);
    cDatabaseWriter writer(db);
    
    cAnalyzerManager manager(&reader, &writer, 2);
    manager.Start();
    
    while(manager.Active())
    {
        cCondWait::SleepMs(1000);
        
        std::cout << "Total files analyzed: " << manager.AnalyzedFileCount() 
                  << " Active analyzers: " << manager.CountActive() 
                  << " Queue size: " << manager.QueueSize() 
                  << " Still to update: " << manager.StillToUpdateCount() 
                  << std::endl;
        
        // for this test stop manager if no more files are to be analyzed
        if (!manager.StillToUpdateCount())
            manager.Stop();
    }

    delete db;
    return 0;
}
