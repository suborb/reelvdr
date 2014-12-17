
#include "ripitstatus.h"


cRipitStatus::cRipitStatus(string filename, string desc)
{
    logFileName = filename;
    description = desc;
    old_time = 0;
    ripCount = encodeCount = 0;
    totalTracks = 0;
}

void cRipitStatus::Action()
{
    //cout<<"****************************************Ripit status thread started*************************"<<endl;
    startTime = time(NULL);
    while (Running())
    {
        percentage_done = StatusRip();
        NotifyBgProcess(tr("Reading CD"), description, startTime, percentage_done);
        if (percentage_done > 100)
            break;
        cCondWait::SleepMs(500);
    }
}

int cRipitStatus::StatusRip()
{

    if (stat(logFileName.c_str(), &fstats) == 0 && fstats.st_mtime != old_time)
    {                           // only re-read file if it has changed
        old_time = fstats.st_mtime;
        std::ifstream logfile(logFileName.c_str());
        std::string line;
        int tmp;
        stringstream oss(stringstream::in | stringstream::out);
        ripCount = encodeCount = totalTracks = 0;

        while (logfile.good())
        {                       // read logfile
            getline(logfile, line);

            if (line.find("Tracks") == 0)
            {                   // Tracks 1 2 3 4 5 6 7 8 will be ripped
                oss << line.substr(7);
                while (oss.good())
                    oss >> tmp;
                totalTracks = tmp;
                oss.clear();    // clear error flags on oss
            }
            else if (line.find("Ripper : ") == 0)   // Ripper : 01 ...
                ripCount++;
            else if (line.find("Encoder: finished -> ") == 0)
                encodeCount++;
            else if (line.find("!! DONE") == 0)
            {
                description = tr("Completed");
                return 130;
            }
            else if (line == tr("PROCESS MANUALLY ABORTED"))
            {
                description = tr("Aborted");
                return 110;
            }
        }
        if (ripCount == totalTracks)
            description = tr("Encoding tracks");
        else
            description = tr("Ripping and Encoding tracks");
        logfile.close();
    }
    if (totalTracks == 0)
        return 0;
    return (int)((ripCount + encodeCount) * 100.0 / (totalTracks * 2));
}

void cRipitStatus::Abort()
{
    Cancel(5);
    NotifyBgProcess(tr("Reading CD"), description, startTime, 110);
}
