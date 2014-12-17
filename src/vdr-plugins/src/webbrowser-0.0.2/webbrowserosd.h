#ifndef __WEBBROWSEROSD_H
#define __WEBBROWSEROSD_H

#include <string>
#include <vdr/plugin.h>
#include <vdr/status.h>
#include "setup.h"
#include <vector>
#include <sys/stat.h>

class cWebbrowserOsd:public cOsdMenu
{
  private:
  public:
    cWebbrowserOsd();
    ~cWebbrowserOsd();
    virtual void Display(void);
    virtual eOSState ProcessKey(eKeys Key);
    void SetFbParams(void);
};

extern cWebbrowserOsd *webbrowserosd;

#endif //__WEBBROWSEROSD_H
