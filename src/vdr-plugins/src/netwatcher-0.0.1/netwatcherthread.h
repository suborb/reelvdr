
#ifndef NETWATCHERTHREAD_H
#define NETWATCHERTHREAD_H

#include <vdr/thread.h>

#define DEFAULT_DEV "eth0"

/** a thread watching the link status and the tuner status */
class cNetWatcherThread:public cThread
{
  private:
#ifdef RBMINI
    eReelboxMode LastReelboxMode;
    void CheckDatabase();
#endif
    bool firstRun;
    int countEthCableNotConnected;     /** > 0 if link status says a cable is connected */
    int countTunersOccupied;           /** > 0 if no free tuner was found */
    bool tunerWarningShown;            /** "true" if the "no free tuner"-warning has already been shown */
    char devname[16];                  /** the name of the network device to monitor */
    int MultiRoomTimeoutCounter_;
    static cNetWatcherThread *_instance;
  public:
    static cNetWatcherThread *instance()
    {
        if (_instance == NULL)
            _instance = new cNetWatcherThread(DEFAULT_DEV);
        return _instance;
    }

  protected:
      virtual void Action(void);        /** the thread's main loop */
  public:
    cNetWatcherThread(const char *devname);
    virtual ~ cNetWatcherThread();
    bool CheckCable();
    void CheckTuner();
    bool CableConnected();             /** returns true if a cable is connected */
    bool TunersOccupied();             /** returns true if all tuners are occupied */
    bool NetceiverFound();             /** returns true if any NetCeiver is found */
    void ResetTunersOccupied();
    void ResetTunerWarningShown();
};

#endif
