#ifndef vlcclient_player_h
#define vlcclient_player_h

#include <vdr/player.h>                     
#include <vdr/thread.h>
#include <vdr/remux.h>

class cVlcReader : public cThread {
  private:
    uchar *buffer;
    bool hasData;
    int length;
    int wanted;
    cCondWait newSet;
    int sd;
  protected:
    virtual void Action(void);  
  public:
    cVlcReader();
    ~cVlcReader();
    bool InitVLCDataSocket(void);
    void CloseVLCDataSocket(void);
    bool Active(void) { return cThread::Running(); }
    bool Reading(void) { return buffer; }  
    int Read(uchar *Buffer, int Length);
    bool ParseHeader();
    bool SkipToSyncByte();
};

class cVlcPlayerFeeder : public cThread {
  private:
    cVlcReader *vlcReader;
    cRemux *remux;
  protected:
    virtual void Action(void);
  public:
    cVlcPlayerFeeder(cRemux *Remux);
    virtual ~cVlcPlayerFeeder();
    bool Active(void) { return cThread::Running(); }
    void ResetStream(void);
    void InitStream(void);
};

class cVlcPlayer : public cPlayer, cThread {
  private:
   cRemux *remux;
   cVlcPlayerFeeder *feeder;
   enum ePlayModes { pmPlay, pmPause, pmSlow, pmFast, pmStill };
   ePlayModes playMode;
   bool timeSearchActive, timeSearchHide;
   int timeSearchTime, timeSearchPos;
   uint64_t lastGetIndex;
   void TimeSearchDisplay(void);
  protected:
   virtual void Activate(bool On);
   virtual void Action(void);
  public:
   cVlcPlayer(const char *Name);
   virtual ~cVlcPlayer();
   bool Active(void) { return cThread::Running(); }
   void Stop(void);
   void Pause(void);
   void Play(void);
   void Forward(void);
   void Backward(void);
   void Goto(int Seconds);
   int  SkipFrames(int Frames);
   void SkipSeconds(int Seconds);
   virtual bool GetIndex(int &Current, int &Total, bool SnapToIFrame = false);
   virtual bool GetReplayMode(bool &Play, bool &Forward, int &Speed);
};

class cVlcControl : public cControl {
  private:
    cVlcPlayer *player;
    cSkinDisplayReplay *displayReplay;
    bool visible, modeOnly, shown, displayFrames;
    int lastCurrent, lastTotal;
    bool lastPlay, lastForward;
    int lastSpeed;
    const char *name;
    int Current, Total;
    int timeSearchTime, timeSearchPos;
    bool timeSearchActive, timeSearchHide;
  public:
    cVlcControl(const char* Name);
    virtual ~cVlcControl();
    void Stop(void);
    void Pause(void);
    void Play(void);
    void Backward(void);
    void Forward(void);
    void Goto(int Seconds);
    void SkipSeconds(int Seconds);
    int SkipFrames(int Frames);
    bool Active(void);
    virtual void Hide(void);
    virtual eOSState ProcessKey(eKeys Key);
    virtual void Show(void);
    void ShowMode(void);
    bool ShowProgress(bool Initial);
    void TimeSearchDisplay();
    void TimeSearch();
    void TimeSearchProcess(eKeys);
};

#endif

