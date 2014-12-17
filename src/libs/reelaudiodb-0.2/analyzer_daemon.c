#include "analyzer.h"
#include "createdatabaseinstance.h"
#include "databasereader.h"
#include "databasewriter.h"
#include "databasecleaner.h"
#include "tools.h"
#include <iostream>
#include <getopt.h>
#include <stdlib.h>

#define VERSION "0.1"

int main(int argc, char* argv[])
{
    int daemon_flag       = 0;
    int run_once_flag     = 0;
    int reanalyze_flag    = 0;
    int verbose_flag      = 0;
    int version_flag      = 0;

    static struct option long_options[] =
    {
        {"daemon",    no_argument,        &daemon_flag,            1          },
        {"run-once",  no_argument,        &run_once_flag,          1          },
        {"reanalyze", no_argument,        &reanalyze_flag,         1          },
        {"verbose",   no_argument,        &verbose_flag,           1          },
        {"version",   no_argument,        &version_flag,           1          },
        {"database",  required_argument,       0,                 'd'         },
        { NULL,       0,                     NULL,                 0          }
    };


    int c = -1;
    int option_index = 0;

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

    if (version_flag)
    {
        std::cout << VERSION << std::endl;
        return 0;
    }
    if (dbPath.empty())
    {
        std::cerr << "Usage: --database <path/name> [--daemon] [--reanalyze] [--run-once] [--verbose]  [--version]"
                  << std::endl;
        return 0;
    }

    if (daemon_flag)
    {
        std::cout << "daemon flag set. forking ..." << std::endl;
        pid_t pid = fork();

        if (pid < 0)               /* fork failed */
        {
            std::cerr << "fork failed. exiting" << std::endl;
            exit(3);
        }
        else if (pid == 0)         /* child process */
        {

        }
        else                       /* parent process */
        {
            exit(0);
        }
    }

    cDatabase *db = CreateDatabaseInstance(dbPath);
    cDatabaseReader reader(db);
    cDatabaseWriter writer(db);

    cAnalyzerManager manager(&reader, &writer, 2);
    manager.SetReanalyze(reanalyze_flag);

    manager.Start();

    // run a thread that updates the 'availability' status
    // of every file in the database once every 10 mins
    cDatabaseCleaner cleaner(&reader, &writer, run_once_flag);
    cleaner.Start();
    
    while(manager.Active())
    {
        cCondWait::SleepMs(1000);
        
        if (verbose_flag)
            std::cout << "Total files analyzed: " << manager.AnalyzedFileCount()
                      << " Active analyzers: " << manager.CountActive()
                      << " Queue size: " << manager.QueueSize()
                      << " Still to update: " << manager.StillToUpdateCount()
                      << std::endl;
        
        // for this test stop manager if no more files are to be analyzed
        if (!manager.StillToUpdateCount() && run_once_flag)
            manager.Stop();
    }

    delete db;
    return 0;
}
