#ifndef RECORDINGS_REPLAY_CONTROL_H
#define RECORDINGS_REPLAY_CONTROL_H

#include <vdr/menu.h>

// captures kBack and calls mmfiles plugin when exiting
class cVdrRecDirReplayControl : public cReplayControl
{
    public:
        ~cVdrRecDirReplayControl();
        eOSState ProcessKey(eKeys key);
};

#endif
