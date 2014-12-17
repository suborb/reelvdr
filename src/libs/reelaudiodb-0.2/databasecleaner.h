#ifndef DATABASECLEANER_H
#define DATABASECLEANER_H
#include "thread.h"
#include "databasereader.h"
#include "databasewriter.h"

/**
 * @brief The cDatabaseCleaner class
 *       "removes" stale entry from database
 *
 * "removes" ==> updates 'available' and 'needs_metadata_update' in database
 *
 *  Updates the availability status of files in database.
 */
class cDatabaseCleaner : public cThread
{
private:
    cDatabaseReader *reader;
    cDatabaseWriter *writer;
    bool runOnce; // stop when no more files to update
public:
    cDatabaseCleaner(cDatabaseReader *r, cDatabaseWriter *w, bool once=false);
    ~cDatabaseCleaner();

    void Stop();
    void Action();
};

#endif // DATABASECLEANER_H
