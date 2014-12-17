#ifndef __RIPITOSD_H
#define __RIPITOSD_H

#include <string>
#include <vdr/plugin.h>
#include <vdr/status.h>
#include <vdr/thread.h>
#include "setup.h"
#include <vector>
#include <sys/stat.h>

class cRipitOsd:public cOsdMenu
{
  private:
    unsigned int offset;
    struct stat fstats;
    time_t old_mtime;
    bool filechanged;
      std::string ripTitle, encodeTitle, totalTracksStr;
    int ripCount, encodeCount, totalTracks;
    const char *bitrateStrings[10];
    const char *preset[5];
    const char *encoders[5];
  public:
      cRipitOsd();
     ~cRipitOsd();
    virtual void ShowStatus(void);
    //virtual void Action();
    virtual eOSState ProcessKey(eKeys Key);
};

extern cRipitOsd *ripitosd;

#endif //__RIPITOSD_H
