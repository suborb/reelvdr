    
#include "thread.h"
cScanDirThread::cScanDirThread(): cThread("Scanning Dir for media files")
{
}

void cScanDirThread::Action()
{
    printf("Start Thread: %s\n", __PRETTY_FUNCTION__);
    cScanDir scan;
    scan.ScanDirTree("/media/hd/recordings");
    scan.ScanDirTree("/home/reel/av");
}
