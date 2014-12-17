#ifndef __WEBBROWSEROSD_H
#define __WEBBROWSEROSD_H

#include <string>
#include <vdr/plugin.h>
#include <vdr/status.h>
#include <vector>
#include <sys/stat.h>

class cAmarokStarterOsd:public cOsdMenu
{
  private:
  public:
    cAmarokStarterOsd();
    ~cAmarokStarterOsd();
    virtual void Display(void);
    virtual eOSState ProcessKey(eKeys Key);
    void SetFbParams(void);
};

extern cAmarokStarterOsd *amarokstarterosd;

#endif //__WEBBROWSEROSD_H
