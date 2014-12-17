/*
 *  Provides "quick shutdown" of VDR. That is: a "empty" player is running, that has no audio, on video.
 *
 *  Taken from Streamdev plugin
 */
#ifndef VDR_SUSPEND_H
#define VDR_SUSPEND_H

#include "player.h"

class cQuickShutdownLive:public cPlayer, cThread
{
  private:
    bool m_Active;

  protected:
    virtual void Activate(bool On);
    virtual void Action(void);

    void Stop(void);

  public:
      cQuickShutdownLive(void);
      virtual ~ cQuickShutdownLive();

    bool IsActive(void) const
    {
        return m_Active;
    }
};

class cQuickShutdownCtl:public cControl
{
  private:
    cQuickShutdownLive * m_QuickShutdown;
    static bool m_Active;

  public:
      cQuickShutdownCtl(void);
      virtual ~ cQuickShutdownCtl();
    virtual void Hide(void)
    {
    }
    virtual eOSState ProcessKey(eKeys Key);

    static bool IsActive(void)
    {
        return m_Active;
    }
    
 /*  only if true "quickshutdown.sh end" is called.
  *  Reason: while going from quickshutdown to standby, video/audio should not be switched on
  **/
   static bool runScript; /* default true, set to false in cMenuShutdown */
   static int afterQuickShutdown; /* indicate what should be done "after" the Quickshutdown mode: 
	used when vdr goes into QuickShutdown when user chooses to standby/deepstandby when a recording/timer is running*/
};

#endif // VDR_SUSPEND_H
