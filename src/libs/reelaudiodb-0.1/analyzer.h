#ifndef ANALYZER_H
#define ANALYZER_H

#include "thread.h"
#include "commondefs.h"
#include "databasereader.h"
#include "databasewriter.h"
#include <queue>

#define METADATA_TABLE "metadata"

/**
 * @brief The cAnalyzer class
 *  extract metadata from/of the given path (file/maybe a dir?)
 *  and writes metadata into a "database"
 */
class cAnalyzer : public cThread
{
private:
    cPath path;
    cDatabaseWriter *writer;
protected:
    bool UpdateDatabase();
    
public:
    cAnalyzer(cDatabaseWriter *w, std::string name="cAnalyzer");
    void SetPath(const cPath&);
    void Action();
};


#define MAX_CRAWLERS  12
class cAnalyzerManager : public cThread
{
    cAnalyzer *analyzers[MAX_CRAWLERS];
    
    /* the read "buffer" between database and analyzers */
    std::queue<std::string> Queue;
    
    /* at most 'num_threads' number of analyzer threads are active at any given time */
    unsigned int num_threads; 
    
    /* count the number of files sent to analyzers */
    unsigned long long analyzedFilesCount;

    /* analyze all files even if they were previously analyzed */
    bool reanalyze;

    cDatabaseReader *reader;
    cDatabaseWriter *writer;
protected:
    bool AllActive() const;
    
public:
    cAnalyzerManager(cDatabaseReader *r, cDatabaseWriter *w, unsigned int num = 2);
    
    /* number of active analyzer thread */
    int CountActive() const;
    
    /* total number of files sent to analyzers */
    unsigned long long AnalyzedFileCount() const;

    void SetReanalyze(bool on);
    bool Reanalyze() const;
    
    /* present Queue size */
    std::queue<std::string>::size_type QueueSize() const;
    
    unsigned long long StillToUpdateCount() const;
    void Action();
    
    void Stop(int waitSec=0);
};

#endif // ANALYZER_H
