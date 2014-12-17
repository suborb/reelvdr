
#ifndef DISPLAYTRACKS_H
#define DISPLAYTRACKS_H

#include "reel.h"

/**
 * cSkinReelDisplayTracks
 */

class cSkinReelDisplayTracks : public cSkinDisplayTracks, public cSkinReelBaseOsd { // reimplement this like ReelSkin (old)
private:
  const cFont *pFontOsdTitle;
  const cFont *pFontDate;
  const cFont *pFontListItem;

  int xTitleLeft, xTitleRight, yTitleTop, yTitleBottom, yTitleDecoTop, yTitleDecoBottom;
  int xLogoLeft, xLogoRight, xLogoDecoLeft, xLogoDecoRight, yLogoTop, yLogoBottom;
  int xListLeft, xListRight, yListTop, yListBottom, xItemLeft, xItemRight;
  int xBottomLeft, xBottomRight, yBottomTop, yBottomBottom;

  int lineHeight;
  int currentIndex;
  bool fShowSymbol;
  time_t lastTime;
  bool subTitleMenu;

  void SetItem(const char *Text, int Index, bool Current);
public:
  cSkinReelDisplayTracks(const char *Title, int NumTracks, const char *const *Tracks);
  virtual ~cSkinReelDisplayTracks();
  virtual void SetTrack(int Index, const char *const *Tracks);
  virtual void SetAudioChannel(int AudioChannel);
  virtual void Flush(void);
};

#endif


