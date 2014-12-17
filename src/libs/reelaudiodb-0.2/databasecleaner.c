#include "databasecleaner.h"
#include "tools.h"
#include <iostream>

cDatabaseCleaner::cDatabaseCleaner(cDatabaseReader *r, cDatabaseWriter *w, bool once)
    : reader(r), writer(w), runOnce(once)
{

}

cDatabaseCleaner::~cDatabaseCleaner()
{
    Stop();
}

void cDatabaseCleaner::Stop()
{
    Cancel(1);
}

/* check for unavailable file, every 10 minutes or so
 * TODO must be triggerable
 */
void cDatabaseCleaner::Action()
{

    unsigned int offset = 0;

    while (Running())
    {
        cQuery query;
        query.AddField("path");
        query.AddField("available");
        query.AddField("metadata_needs_update");

        std::string where = std::string("path is not NULL LIMIT 1000 OFFSET ") + std::string(*itoa(offset));
        query.AddWhereClause(where);

        cResults results;
        bool res = reader->Query(results, "metadata", query);
        unsigned int res_len = results.results.size();

        offset += res_len;

        unsigned int i = 0;
        bool available = false;
        bool wasAvailable = false;

        for (i=0; i < res_len && Running(); ++i)
        {
            cRow &row = results.results[i];
            if (row.size())
            {
                available    = (access(row.at(0).c_str(), R_OK) == 0);
                wasAvailable = (row.at(1) != "0");

                // file availability changed
                if (available != wasAvailable)
                {
#if 0
                    std::cerr << "'" << row.at(0) << "' available?"<< available
                              << " row.at(1)='" << row.at(1)
                              << "' wasAvailable?"<< wasAvailable << std::endl;
#endif

                    cFields field;
                    cRow r;
                    std::string w = "path=\'" + row.at(0) + "\'";

                    field.push_back("available");
                    r.push_back(available ?"1":"0");

                    writer->Update("metadata", field, r, w);
                }
            }
        } // for results

        cCondWait::SleepMs(10);

        // no more files' status to update
        if (res_len == 0)
        {
            // run once ==> stop when no more files to update
            if (runOnce)
                break;

            offset = 0;

            // sleep for a while, before starting again
            i = 0;
            while (i++ < 100*60 && Running())
            {
                cCondWait::SleepMs(100);
            } // while
        } // if

    } // while (Running())
}
