#ifndef NETWATCHERMENU_H
#define NETWATCHERMENU_H

#include <vdr/osdbase.h>

enum warningType_t
{ eWarnNoNetwork, eWarnNoTuner };

class cNetWatcherMenu:public cOsdMenu
{
  private:
    warningType_t warningType;
  public:
    cNetWatcherMenu();
    virtual ~ cNetWatcherMenu();
    virtual void Display(void);
    virtual eOSState ProcessKey(eKeys Key);
};


#endif
