#include "thread.h"
#include "scanDir.h"
#include "inotify.h"
#include "cache.h"
#include "mmfiles.h"

cScanDirThread::cScanDirThread(): cThread("Scanning Dir for media files")
{
}

void cScanDirThread::Action()
{
    printf("Start Thread: %s\n", __PRETTY_FUNCTION__);
    cScanDir scan;
    //scan.ScanDirTree("/media/hd/recordings");
    //scan.ScanDirTree("/media/hd/video");

    Cache.ClearCache();
    // load dirs from cDirList "global variable"
    unsigned int i = 0, len = cDirList::Instance().ListSize();
    for ( ; i<len; ++i)
        scan.ScanDirTree(cDirList::Instance().PathAt(i));

    cDirList::Instance().ResetChanged();

    cInotify inotify;

    for (i=0 ; i<len; ++i)
        inotify.WatchDir(cDirList::Instance().PathAt(i));
    //inotify.WatchDir("/media/hd/recordings");
    //inotify.WatchDir("/media/hd/video");

    printf("\n\n\n\033[0;91m Done adding Watches!\n\n\033[0m");
    while(Running())
        {
            // handle Inotify Events
            inotify.InotifyEvents();
            cCondWait::SleepMs(100);
        }
}

void cScanDirThread::Stop()
{
    Cancel(1);
}
