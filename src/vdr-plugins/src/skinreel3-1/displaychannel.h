#ifndef DISPLAYCHANNEL_H
#define DISPLAYCHANNEL_H

#include "reel.h"

// --- cSkinReelDisplayChannel --------------------------------------------

class cSkinReelDisplayChannel : public cSkinDisplayChannel, public cSkinReelBaseOsd, public cSkinReelThreadedOsd {
private:
  bool fShowLogo;
  bool fWithInfo;
  char *strLastDate;
  int nMessagesShown;

  const cFont *pFontOsdTitle;
  const cFont *pFontDate;
  const cFont *pFontTitle;
  const cFont *pFontSubtitle;
  const cFont *pFontLanguage;
  const cFont *pFontMessage;

  int xLogoLeft, xLogoRight, yLogoTop, yLogoBottom, xLogoDecoLeft, xLogoDecoRight, xLogoPos, yLogoPos;
  int xTitleLeft, xTitleRight, yTitleTop, yTitleBottom, yTitleDecoTop, yTitleDecoBottom;
  int xEventNowLeft, xEventNowRight, yEventNowTop, yEventNowBottom;
  int xEventNextLeft, xEventNextRight, yEventNextTop, yEventNextBottom;
  int xBottomLeft, xBottomRight, yBottomTop, yBottomBottom;
  int xMessageLeft, xMessageRight, yMessageTop, yMessageBottom;
  int xFirstSymbol, xDateLeft;
  int xSymbol;
#if VDRVERSNUM < 10716
  int lastAspect;
#else
  double lastAspect;
#endif

  int xeSignalBar, xsSignalBar;

  int strBarWidth, snrBarWidth;

  bool isMessage;

  void UpdateSignal();
  void UpdateDateTime();
#ifdef REELVDR
  void UpdateAspect();
#endif

  void DrawAreas(void);
  void DrawGroupInfo(const cChannel *Channel, int Number);
  void DrawChannelInfo(const cChannel *Channel, int Number);
  void DrawSymbols(const cChannel *Channel);
  void DrawChannelLogo(const cChannel *channel);
  cString GetChannelName(const cChannel *Channel);
  cString GetChannelNumber(const cChannel *Channel, int Number);
public:
  cSkinReelDisplayChannel(bool WithInfo);
  virtual ~ cSkinReelDisplayChannel();
  virtual void SetChannel(const cChannel *Channel, int Number);
  virtual void SetEvents(const cEvent *Present, const cEvent *Following);
  virtual void SetMessage(eMessageType Type, const char *Text);
  virtual void Flush(void);
  virtual void DrawTitle(const char *Title);
};

#endif

