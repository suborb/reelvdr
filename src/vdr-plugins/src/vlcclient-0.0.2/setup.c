#include <vdr/menuitems.h>

#include "setup.h"
#include "common.h"
#include "i18n.h"

cVlcClientSetup VlcClientSetup;

cVlcClientSetup::cVlcClientSetup(void) {
  strcpy(RemoteIp, "127.0.0.1");
  strcpy(StartDir, "C:\\movies");
  Deinterlace = 0;
  AudioSync = 0;
  PostProQuality = 0;
  OverlaySubtitles = 0;
  Resolution = 0;
  StreamDD = 0;
  RemotePower = 0;
  FfmpegHQProfile = 0;
  FfmpegInterlace = 0;
  FfmpegHurryUp = 0;
  FfmpegStrict = 255;
  FfmpegNoiseRed = 0;
  FfmpegHurryUp = 0;
  FfmpegInterlacedMe = 0;
}

bool cVlcClientSetup::SetupParse(const char *Name, const char *Value) {
  //printf("DDD: parse %s - %s\n", Name, Value);
  if (strcmp(Name, "RemoteIp") == 0) {
    if (strcmp(Value, "-none-") == 0)
      strcpy(RemoteIp, "");
    else
      strcpy(RemoteIp, Value);
  }
  else if (strcmp(Name, "StartDir") == 0)            strcpy(StartDir, Value);
  else if (strcmp(Name, "Deinterlace") == 0)         Deinterlace = atoi(Value);
  else if (strcmp(Name, "AudioSync") == 0)           AudioSync = atoi(Value);
  else if (strcmp(Name, "PostProQuality") == 0)      PostProQuality = atoi(Value);
  else if (strcmp(Name, "OverlaySubtitles") == 0)    OverlaySubtitles = atoi(Value);
  else if (strcmp(Name, "Resolution") == 0)          Resolution = atoi(Value);
  else if (strcmp(Name, "StreamDD") == 0)            StreamDD = atoi(Value);
  else if (strcmp(Name, "RemotePower") == 0)         RemotePower = atoi(Value);
  else if (strcmp(Name, "FfmpegHQProfile") == 0)     FfmpegHQProfile = atoi(Value);
  else if (strcmp(Name, "FfmpegInterlace") == 0)     FfmpegInterlace = atoi(Value);
  else if (strcmp(Name, "FfmpegInterlacedMe") == 0)  FfmpegInterlacedMe = atoi(Value);
  else if (strcmp(Name, "FfmpegPreMe") == 0)         FfmpegPreMe = atoi(Value);
  else if (strcmp(Name, "FfmpegHurryUp") == 0)       FfmpegHurryUp = atoi(Value);
  else if (strcmp(Name, "FfmpegNoiseRed") == 0)      FfmpegNoiseRed = atoi(Value);
  else if (strcmp(Name, "FfmpegStrict") == 0)        FfmpegStrict = atoi(Value);
  //else if (strcmp(Name, "") == 0) ;
  else return false;
  return true;
}

cVlcClientMenuSetupPage::cVlcClientMenuSetupPage(void) 
{
  videoRatios[0] = "352x288";
  videoRatios[1] = "352x576";
  videoRatios[2] = "480x576";
  videoRatios[3] = "576x576";
  videoRatios[4] = "704x576";
  videoRatios[5] = "720x576";

  powerStrings[0] = tr("very low");
  powerStrings[1] = tr("low");
  powerStrings[2] = tr("ok");
  powerStrings[3] = tr("good");
  powerStrings[4] = tr("very good");
  Setup();

}

void cVlcClientMenuSetupPage::Setup() 
{
  m_NewSetup = VlcClientSetup;
  Add(new cMenuEditIpItem(tr("IP of remote VLC"),          m_NewSetup.RemoteIp));
  Add(new cMenuEditStrItem(tr("Remote VLC Directory"), m_NewSetup.StartDir, 30, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!\"§$%&/()=?{}[]\\+*~#',;.:-_<>|@´`^°" ));
  Add(new cOsdItem(" ", osUnknown, false), false, NULL);
#ifdef SIMPLE_SETUP
  Add(new cMenuEditStraItem(tr("Streaming Resolution"), &m_NewSetup.Resolution, 6, videoRatios));
  Add(new cMenuEditBoolItem(tr("Stream DD (works only if movie has DD)"), &m_NewSetup.StreamDD, tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Sync on audio"), &m_NewSetup.AudioSync, tr("no"), tr("yes")));
  Add(new cMenuEditStraItem(tr("Remote CPU Power"), &m_NewSetup.RemotePower, 5, powerStrings));
#else
  Add(new cOsdItem("--- VLC-Options ---", osUnknown, false), false, NULL);
  Add(new cMenuEditStraItem(tr("Streaming Resolution"), &m_NewSetup.Resolution, 6, videoRatios));
  Add(new cMenuEditBoolItem(tr("Stream DD (works only if movie has DD)"), &m_NewSetup.StreamDD, tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Deinterlace before encoding"), &m_NewSetup.Deinterlace, tr("no"), tr("yes"))); 
  Add(new cMenuEditBoolItem(tr("Sync on audio"), &m_NewSetup.AudioSync, tr("no"), tr("yes")));
  Add(new cMenuEditIntItem(tr("Post processing quality"), &m_NewSetup.PostProQuality, 0, 255));
  Add(new cMenuEditBoolItem(tr("Render subtitles into stream"), &m_NewSetup.OverlaySubtitles, tr("no"), tr("yes")));
  Add(new cOsdItem(" ", osUnknown, false), false, NULL);
  Add(new cOsdItem("--- ffmpeg-encoding-options ---", osUnknown, false), false, NULL);
  Add(new cMenuEditBoolItem(tr("HQ-profile"), &m_NewSetup.FfmpegHQProfile, tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Interlace"), &m_NewSetup.FfmpegInterlace, tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Interlaced motion estimation"), &m_NewSetup.FfmpegInterlacedMe, tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Pre-motion estimation"), &m_NewSetup.FfmpegPreMe, tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Hurry up"), &m_NewSetup.FfmpegHurryUp, tr("no"), tr("yes")));
  Add(new cMenuEditIntItem(tr("Noise reduction"), &m_NewSetup.FfmpegNoiseRed, 0, 255));
  Add(new cMenuEditIntItem(tr("Strict standard compliance"), &m_NewSetup.FfmpegStrict, 0, 255));
#endif
  SetCurrent(Get(0));
}

cVlcClientMenuSetupPage::~cVlcClientMenuSetupPage() {
}

void cVlcClientMenuSetupPage::Store(void) {
  if (strcmp(m_NewSetup.RemoteIp, "") == 0)
    SetupStore("RemoteIp", "-none-");
  else
    SetupStore("RemoteIp",    m_NewSetup.RemoteIp);
  SetupStore("StartDir",               m_NewSetup.StartDir);
  SetupStore("AudioSync",              m_NewSetup.AudioSync);
  SetupStore("StreamDD",               m_NewSetup.StreamDD);
#ifdef SIMPLE_SETUP
  SetupStore("RemotePower",            m_NewSetup.RemotePower);
  switch(m_NewSetup.RemotePower){
    case 0:
      m_NewSetup.Deinterlace = 0; m_NewSetup.FfmpegInterlace = 0; m_NewSetup.FfmpegPreMe = 0; m_NewSetup.PostProQuality = 0; m_NewSetup.FfmpegStrict = 255;
      m_NewSetup.FfmpegHQProfile = 0; m_NewSetup.FfmpegHurryUp = 1; m_NewSetup.FfmpegNoiseRed = 0; m_NewSetup.FfmpegInterlacedMe = 0; break;
    case 1:
      m_NewSetup.Deinterlace = 0; m_NewSetup.FfmpegInterlace = 0; m_NewSetup.FfmpegPreMe = 0; m_NewSetup.PostProQuality = 0; m_NewSetup.FfmpegStrict = 255;
      m_NewSetup.FfmpegHQProfile = 1; m_NewSetup.FfmpegHurryUp = 1; m_NewSetup.FfmpegNoiseRed = 0; m_NewSetup.FfmpegInterlacedMe = 0; break;
    case 2:
      m_NewSetup.Deinterlace = 0; m_NewSetup.FfmpegInterlace = 0; m_NewSetup.FfmpegPreMe = 0; m_NewSetup.PostProQuality = 0; m_NewSetup.FfmpegStrict = 255;
      m_NewSetup.FfmpegHQProfile = 1; m_NewSetup.FfmpegHurryUp = 0; m_NewSetup.FfmpegNoiseRed = 0; m_NewSetup.FfmpegInterlacedMe = 0; break;
    case 3:
      m_NewSetup.Deinterlace = 1; m_NewSetup.FfmpegInterlace = 1; m_NewSetup.FfmpegPreMe = 0; m_NewSetup.PostProQuality = 255; m_NewSetup.FfmpegStrict = 255;
      m_NewSetup.FfmpegHQProfile = 1; m_NewSetup.FfmpegHurryUp = 0; m_NewSetup.FfmpegNoiseRed = 0; m_NewSetup.FfmpegInterlacedMe = 0; break;
    case 4:
      m_NewSetup.Deinterlace = 1; m_NewSetup.FfmpegInterlace = 1; m_NewSetup.FfmpegPreMe = 1; m_NewSetup.PostProQuality = 255; m_NewSetup.FfmpegStrict = 255;
      m_NewSetup.FfmpegHQProfile = 1; m_NewSetup.FfmpegHurryUp = 0; m_NewSetup.FfmpegNoiseRed = 0; m_NewSetup.FfmpegInterlacedMe = 1; break;
    default:
      break;
  }
#endif
  SetupStore("Deinterlace",            m_NewSetup.Deinterlace);
  SetupStore("PostProQuality",         m_NewSetup.PostProQuality);
  SetupStore("OverlaySubtitles",       m_NewSetup.OverlaySubtitles);
  SetupStore("Resolution",             m_NewSetup.Resolution);
  SetupStore("StreamDD",               m_NewSetup.StreamDD);
  SetupStore("FfmpegHQProfile",        m_NewSetup.FfmpegHQProfile);
  SetupStore("FfmpegInterlace",        m_NewSetup.FfmpegInterlace);
  SetupStore("FfmpegInterlacedMe",     m_NewSetup.FfmpegInterlacedMe);
  SetupStore("FfmpegPreMe",            m_NewSetup.FfmpegPreMe);
  SetupStore("FfmpegHurryUp",          m_NewSetup.FfmpegHurryUp);
  SetupStore("FfmpegNoiseRed",         m_NewSetup.FfmpegNoiseRed);
  SetupStore("FfmpegStrict",           m_NewSetup.FfmpegStrict);
  //SetupStore("",     m_NewSetup.);
  VlcClientSetup = m_NewSetup;
}

