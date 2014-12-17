#include <vdr/menu.h>

class myReplayControl:public cReplayControl
{
 private:
   bool CheckRemainingHDDSpace();
   bool timesearchactive;
   int exitType_; // 0:unknown 1:kBack 2:kStop
 public:
   myReplayControl();
   ~myReplayControl();
   eOSState ProcessKey(eKeys Key);

   static bool active;
};
