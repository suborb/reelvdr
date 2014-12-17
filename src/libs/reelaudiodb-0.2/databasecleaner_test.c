#include "databasecleaner.h"
#include "createdatabaseinstance.h"
#include <iostream>
#include <getopt.h>

void PrintUsage(int argc, char* argv[])
{
    std::cerr << "Usage:\n\t" << argv[0] << " --database <dbname>" << std::endl;
}

int main(int argc, char* argv[])
{
    static struct option long_options[] =
    {
        {"database",  required_argument,       0,                 'd'         },
        { NULL,       0,                     NULL,                 0          }
    };


    int c = -1;
    int option_index = 0;

    std::string dirTree;
    std::string dbPath;
    
    while ( 1 )
    {
        c = getopt_long(argc, argv, "d", long_options, &option_index);

        /* no more options */
        if (c == -1)
            break;

        switch ( c )
        {
        case 0:
            break;

        case 'd':
            dbPath = optarg;
            break;

        default:
            break;
        } // switch
    } // while
    
    if (dbPath.empty())
    {
        PrintUsage(argc, argv);
        return 1;
    }
    
    cDatabase *db = CreateDatabaseInstance(dbPath);
    cDatabaseReader reader(db);
    cDatabaseWriter writer(db);

    cDatabaseCleaner cleaner(&reader, &writer, true);
    cleaner.Start();

    while (cleaner.Active())
        cCondWait::SleepMs(500);

    return 0;
}
