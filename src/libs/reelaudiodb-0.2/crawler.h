#ifndef CRAWLER_H
#define CRAWLER_H

#include "thread.h"
#include "commondefs.h"
#include "databasereader.h"
#include "databasewriter.h"
#include <queue>

#define METADATA_TABLE "metadata"



class cCrawlerManager;

/**
 * @brief The cDirCrawler class
 *  traverse ONE given directory for its contents
 *  add subdirectories to DirQ and files to DB
 */
class cDirCrawler : public cThread
{
    cDirectoryQueue *dirQ;
    cPath pwd; // present working directory
    cDatabaseWriter *writer;
protected:
    bool AddPathToDatabase(const cPath& );
    friend class cCrawlManager;

public:
    cDirCrawler(cDatabaseWriter *w, cDirectoryQueue *queue, std::string name="cDirCrawler");
    void SetDirectory(const cPath& path);
    void Action();
};


#define MAX_CRAWLERS  12
class cCrawlManager : public cThread
{
    cDirCrawler *crawlers[MAX_CRAWLERS];
    cDirectoryQueue dirQueue;
    unsigned long long crawledDirsCount;
    cDatabaseReader *reader;
    cDatabaseWriter *writer;
    unsigned int num_threads;
public:
    cCrawlManager(cDatabaseReader* r, cDatabaseWriter* w, unsigned int num = 2);
    ~cCrawlManager();

    int CountActive() const;
    unsigned long long CrawledDirsCount() const;
    void Action();
    bool ScanDir(const std::string& dir);
    void Stop();
};

#endif /*CRAWLER_H*/
