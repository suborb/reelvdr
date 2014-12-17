#include "crawler.h"
#include "createdatabaseinstance.h"
#include "tools.h"
#include <iostream>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/file.h> // flock()

#define VERSION 0.1

extern int verbose_flag;

/**
 *  Crawler writes the directory tree into database.
 *
 *    It writes all the files in the given 'root' directory,
 *     traversing through its subdirectories.
 */



int main(int argc, char* argv[])
{
    int daemon_flag  = 0;
    int version_flag = 0;

    static struct option long_options[] =
    {
        {"daemon",    no_argument,        &daemon_flag,            1          },
        {"verbose",   no_argument,        &verbose_flag,           1          },
        {"version",   no_argument,        &version_flag,           1          },
        {"database",  required_argument,       0,                 'd'         },
        {"sourcedir", required_argument,       0,                 's'         },
        { NULL,       no_argument,            NULL,                0          }
    };

    int c = -1;
    int option_index = 0;

    std::string dbPath;
    std::string source;
    while ( 1 )
    {
        c = getopt_long(argc, argv, "d:s:", long_options, &option_index);

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

        case 's':
            if (optarg)
                source = optarg;
            break;

        default:
            break;
        } // switch
    } // while

    // Version asked for. Show version number and exit.
    if (version_flag)
    {
        std::cout << argv[0] << " version:" << VERSION << std::endl;
        exit(0);
    }

    if (argc < 3)
    {
       std::cout << "Usage: \n\t " << argv[0]
                 << "  --database path_to_database  --source source_dir_of_music_files "
                 << " [--verbose] [--daemon] "
                 << std::endl;
       return 1;
    }


    if (source.empty())
    {
        std::cerr << "No sources specified" << std::endl;
        return 1;
    }

    if (dbPath.empty())
    {
        std::cerr << "No database path specified" << std::endl;
        return 2;
    }

    // run as daemon
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
            freopen("/tmp/crawler_stderr", "w", stderr);
            freopen("/tmp/crawler_stdout", "w", stdout);
        }
        else                       /* parent process */
        {
            _exit(0);
        }
    }


#if LOCK_CRAWLER
    // get exclusive lock over the given source directory
    // if cannot exit...
    int fd = open("/var/lock/crawler", O_CREAT|O_RDONLY, S_IRUSR);
    if (fd < 0)
    {
        // error
        std::cerr << " could not open lock file /var/lock/crawler .Exiting." << std::endl;
        exit(1);
    }

    if (0 != flock(fd, LOCK_EX))
    {
        // lock failed
        std::cerr << "Locking failed" << std::endl;
        exit(1);
    }
#endif

    cDatabase *db = CreateDatabaseInstance(dbPath, false); //TODO dont create
    db->CreateTable("metadata");
    
    cDatabaseReader reader(db);
    cDatabaseWriter writer(db);
    
    cCrawlManager manager(&reader, &writer, 2);
    manager.ScanDir(source);
    manager.Start();


    std::cout << "Scanning \'" << argv[2] << "\' and writing into database " << argv[1] << std::endl;
    while (manager.Active())
    {
        if (verbose_flag)
            std::cout << manager.CountActive() << " crawled dirs"
                      << manager.CrawledDirsCount() << std::endl;

        cCondWait::SleepMs(1000);
    }
    
    delete db;

#if LOCK_CRAWLER
    close(fd);
#endif

    return 0;
}
