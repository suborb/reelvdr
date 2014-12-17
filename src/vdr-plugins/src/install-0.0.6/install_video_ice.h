#ifndef INSTALL_VIDEO_ICE__H
#define INSTALL_VIDEO_ICE__H

#include "installmenu.h"

/* Get the video-setup menu from reelice plugin
   and show it as its submenu
  */
class cInstallVidoutICE:public cInstallSubMenu
{
  private:
    
    cOsdMenu* menu;
    
    void Set();
    bool Save();
    
  public:
      cInstallVidoutICE();
     ~cInstallVidoutICE();
      eOSState ProcessKey(eKeys Key);
};


#endif /*INSTALL_VIDEO_ICE__H*/