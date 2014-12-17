#ifndef vlcclient_setup_h
#define vlcclient_setup_h

#include <vdr/menu.h>

struct cVlcClientSetup {
  cVlcClientSetup(void);
  bool SetupParse(const char *Name, const char *Value);
  char RemoteIp[20];
  char StartDir[32];
  int Deinterlace;      
  int AudioSync;
  int PostProQuality;
  int OverlaySubtitles;
  int Resolution;
  int StreamDD;
  int RemotePower;
  int FfmpegHQProfile;
  int FfmpegInterlace;
  int FfmpegHurryUp;
  int FfmpegNoiseRed;
  int FfmpegPreMe;
  int FfmpegInterlacedMe;
  int FfmpegStrict;
};

extern cVlcClientSetup VlcClientSetup;   

class cVlcClientMenuSetupPage : public cMenuSetupPage {
private:
  cVlcClientSetup m_NewSetup;
  const char *videoRatios[6];
  const char *powerStrings[5];
 void Setup();  
protected:
  virtual void Store(void);

public:
  cVlcClientMenuSetupPage(void);
  virtual ~cVlcClientMenuSetupPage();
};

#endif
