
#ifndef __MMFILES_THREAD_H
#define __MMFILES_THREAD_H

#include <vdr/thread.h>
#include "scanDir.h"

class cScanDirThread : public cThread
{
    public:
        cScanDirThread();
        void Action();
};

#endif
