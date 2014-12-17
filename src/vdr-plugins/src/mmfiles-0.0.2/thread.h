
#ifndef __MMFILES_THREAD_H
#define __MMFILES_THREAD_H

#include <vdr/thread.h>

class cScanDirThread : public cThread
{
    public:
        cScanDirThread();
        void Action();
        void Stop();
};

#endif
