#ifndef ___REELCAM_H
#define ___REELCAM_H

#include <vdr/thread.h>

#define MAXDVBAPI 4
#define MAXSCCAPS 10

class cDevice;

class cReelCAM {
private:
  static struct CamInfo camInfo[MAXDVBAPI];
  static cMutex camLock;
  static int foundKeys, newKeys;
  //
public:
  static void *Init(const cDevice *dev);
  static void *FindCAM(const cDevice *dev);
  static bool CapCheck(int n);
  static bool Load(const char *cfgdir);
  static void Save(void);
  static void ShutDown(void);
  //
  static void FoundKey(void) { foundKeys++; }
  static void NewKey(void) { newKeys++; }
  static void KeyStats(int &fk, int &nk) { fk=foundKeys; nk=newKeys; }
  static char *CurrKeyStr(int CardNum);
  static bool Active(void);
  };

#endif // ___REELCAM_H

