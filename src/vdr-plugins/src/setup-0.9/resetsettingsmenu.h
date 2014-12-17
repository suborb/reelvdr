
#ifndef RESETSETTINGSMENU_H
#define RESETSETTINGSMENU_H

#include <vdr/osdbase.h>

// ---  Class cMenuSetupResetSettings  --------------------------------

class cMenuSetupResetSettings:public cOsdMenu
{
  private:
  public:
      cMenuSetupResetSettings(void);
    eOSState ProcessKey(eKeys Key);
    void Set(void);
    void ResetSettings(void);
};

#endif

