#ifndef _OBSERVERTHREAD_H
#define _OBSERVERTHREAD_H

#include "vdr/thread.h"

class ObserverThread : public cThread {
  private:
    bool active;
    bool finished;
    int fd;
    int camState;
  protected:
    virtual void Action(void);
  public:
    ObserverThread(int Fd);
    ~ObserverThread(void);
    static void printState(int state);
    static int powerOn(int fd, int state);
    static int powerOff(int fd, int state);
};

#endif
