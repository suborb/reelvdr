#include "RecordingsReplayControl.h"
#include <vdr/remote.h>

cVdrRecDirReplayControl::~cVdrRecDirReplayControl()
{
    printf("%s\n", __PRETTY_FUNCTION__);
    cRemote::CallPlugin("mmfiles");
}

eOSState cVdrRecDirReplayControl::ProcessKey(eKeys key)
{
    if (key == kBack) // donot send to cReplayControl, it calls extrecmenu
    {
        cRemote::CallPlugin("mmfiles");
        return osEnd;
    }

    return cReplayControl::ProcessKey(key);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
