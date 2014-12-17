#ifndef __XINEPLUGIN_DVDSWITCH_H
#define __XINEPLUGIN_DVDSWITCH_H

#include <vdr/thread.h>
#include "helpers.h"
#include "setup.h"


class cXinePluginThread : public cThread
{
  private:
    char *Image;
  protected:
    virtual void Action(void);
  public:
    cXinePluginThread(char *image = NULL);
    ~cXinePluginThread(void);
};

class cXinePlugin
{
  private:
    static cXinePluginThread *thread;
  public:
    static void Start(char *image = NULL);
    static void Exit(void);
    static void CheckMount();
    static void Init(void)
    {
       CheckMount();
    }
    static void Finish(void)
    {
      MYDEBUG(" ---------------- %s ----------------------------- \n", __PRETTY_FUNCTION__);
      printf (" ---------------- %s ----------------------------- \n", __PRETTY_FUNCTION__);
      Exit();
      CheckMount();
      //KillLink();
    }
};

#endif // __XINEPLUGIN_DVDSWITCH_H
