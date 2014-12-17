#ifndef HELPER_H
#define HELPER_H

#include <vdr/thread.h>
#include <linux/dvb/frontend.h>

class cDvbTuner:public cThread
{
  private:
    enum eTunerStatus
    { tsIdle, tsSet, tsTuned, tsLocked };
    bool SendDiseqc;
    int fd_frontend;
    int cardIndex;
    int tuneTimeout;
    int lockTimeout;
    time_t lastTimeoutReport;
    fe_type_t frontendType;
    cCiHandler *ciHandler;
    cChannel channel;
    const char *diseqcCommands;
    eTunerStatus tunerStatus;
    cMutex mutex;
    cCondVar locked;
    cCondVar newSet;
    char *description;
    dvb_diseqc_master_cmd diseqc_cmd;
    bool SetFrontend(void);
    virtual void Action(void);
  public:
      cDvbTuner(int Fd_Frontend, int CardIndex, fe_type_t FrontendType, cCiHandler * CiHandler);
      virtual ~ cDvbTuner();
    bool IsTunedTo(const cChannel * Channel) const;
    void Set(const cChannel * Channel, bool Tune);
    bool SendDiseqcCmd(dvb_diseqc_master_cmd cmd);
    bool Locked(int TimeoutMs = 0);
    bool GetFrontendStatus(fe_status_t & Status, int TimeoutMs = 0);
};

#endif
