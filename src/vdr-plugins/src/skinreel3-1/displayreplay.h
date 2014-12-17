#ifndef DISPLAYREPLAY_H
#define DISPLAYREPLAY_H

#include "reel.h"

// --- cSkinReelDisplayReplay ---------------------------------------------

class cSkinReelDisplayReplay : public cSkinDisplayReplay, public cSkinReelBaseOsd, public cSkinReelThreadedOsd {
private:
  const cFont *pFontOsdTitle;
  const cFont *pFontReplayTimes;
  const cFont *pFontDate;
  const cFont *pFontLanguage;
  const cFont *pFontMessage;

  int xTitleLeft, xTitleRight, yTitleTop, yTitleBottom;
  int xLogoLeft, xLogoRight, yLogoTop, yLogoBottom;
  int xProgressLeft, xProgressRight, yProgressTop, yProgressBottom;
  int xBottomLeft, xBottomRight, yBottomTop, yBottomBottom;
  int xMessageLeft, xMessageRight, yMessageTop, yMessageBottom;
  int xFirstSymbol;

  char *strLastDate;
  bool modeonly;
  int nJumpWidth;
  int nCurrentWidth;
  bool fShowSymbol;

// hold the 'help key' texts, so that when a function like jump clears it
// it can be redrawn by RedrawHelpKeys()
  cString redText_;
  cString greenText_;
  cString yellowText_;
  cString blueText_;

public:
  cSkinReelDisplayReplay(bool ModeOnly);
  virtual ~ cSkinReelDisplayReplay();
  virtual void SetTitle(const char *Title);
  virtual void SetMode(bool Play, bool Forward, int Speed);
  virtual void SetProgress(int Current, int Total);
  virtual void SetCurrent(const char *Current);
  virtual void SetTotal(const char *Total);
  virtual void SetJump(const char *Jump);
  virtual void SetMessage(eMessageType Type, const char *Text);
  virtual void Flush(void);
  virtual void DrawTitle(const char *s);

  // draw the bottom bar, needed to clear 'Help keys'
  void DrawBottomBar();

  // Draw 'Help keys' and store the r/g/y/b texts in private vars
  void SetHelp(const char* r=NULL, const char* g=NULL, const char*y=NULL, const char*b=NULL);

  // calls SetHelp() with stored 'Help key' text. eg. after jump/drawing a 
  // message etc where the help key texts were cleared
  void RedrawHelpKeys();
};

#endif

