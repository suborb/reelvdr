
#include <sys/time.h>

#include "shutdown.h"


//enum eShutdownMode { standby, deepstandby, restart, maintenance };


class cMenuShutdown : public cOsdMenu {
private:
  //int &interrupted_;
  eActiveMode shutdownMode_;
  bool userShutdown_;
  time_t lastActivity_;
  bool shutdown_;
  bool standbyAfterRecording_;
  bool quickShutdown_;
  bool expert;
  bool IsTimerNear();
  eOSState Shutdown(eActiveMode mode);

  class cCounterTimer
  {
  public:
    cCounterTimer()
    : oldtime_(0), timeout_(0)
    {
    }
    bool TimedOut()
    {
        gettimeofday(&time, &tz);
        uint newtime = (time.tv_sec%1000000)*1000 + time.tv_usec/1000;
        if(oldtime_ && newtime - oldtime_ > timeout_)
        {
            return true;
        }
        return false;
    }
    void Start(int timeout)
    {
        gettimeofday(&time, &tz);
        oldtime_ = (time.tv_sec%1000000)*1000 + time.tv_usec/1000;
        timeout_ = timeout;
    }
  private:
    struct timeval time;
    struct timezone tz;
    uint oldtime_;
    uint timeout_;
  }
  timer_;

  struct BgProcessInfo 
  {
      int nProcess;
  };

public:
  //cMenuShutdown(int &Interrupted, eShutdownMode &shutdownMode, bool &userShutdown, time_t& lastActivity);
  cMenuShutdown();
  ~cMenuShutdown();
  void Set();
  /*override*/ eOSState ProcessKey(eKeys Key);
  void CancelShutdown();
  static void CancelShutdownScript(const char *msg = NULL);
};


