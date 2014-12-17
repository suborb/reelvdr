
#include <iostream>

#include <vdr/device.h>
#include <vdr/plugin.h>

#include "displaytracks.h"
#include "logo.h"
#include "config.h"

/**
 * cSkinReelDisplayTracks
 */

cSkinReelDisplayTracks::cSkinReelDisplayTracks(const char *Title, int NumTracks, const char *const *Tracks)
{
  struct ReelOsdSize OsdSize;
  ReelConfig.GetOsdSize(&OsdSize);

  pFontOsdTitle = ReelConfig.GetFont(FONT_OSDTITLE);
  pFontDate = ReelConfig.GetFont(FONT_DATE);
  pFontListItem = ReelConfig.GetFont(FONT_LISTITEM);

  lastTime = 0;
  fShowSymbol = ReelConfig.showSymbols && ReelConfig.showSymbolsAudio;

  lineHeight = std::max(pFontListItem->Height() , 35);
  int LogoSize = IconHeight;
  LogoSize += (LogoSize % 2 ? 1 : 0);
  currentIndex = -1;
  int ItemsWidth = 0;
  //Get required space for the list items
  for (int i = 0; i < NumTracks; i++)
    ItemsWidth = std::max(ItemsWidth, pFontListItem->Width(Tracks[i]));
  //Add required space for marker or border
  ItemsWidth += (ReelConfig.showMarker ? lineHeight : ListHBorder) + ListHBorder;
  //If OSD title only covers the list then its content must fit too
  if (!ReelConfig.fullTitleWidth)
      ItemsWidth = std::max(ItemsWidth, pFontOsdTitle->Width(Title) + 2 * Roundness + (fShowSymbol ? 0 : (CHANNELINFOSYMBOLWIDTH + SmallGap)));
  //If the symbol is shown the list's width should be at least as wide as the symbol
  if (fShowSymbol)
    ItemsWidth = std::max(ItemsWidth, LogoSize);
  //Now let's calculate the OSD's full width
  if (ReelConfig.fullTitleWidth) {
    //If the symbol is shown, add its width
    if (fShowSymbol)
      ItemsWidth += LogoSize + LogoDecoGap + LogoDecoWidth + LogoDecoGap2;
    //The width must be wide enough for the OSD title
    ItemsWidth = std::max(ItemsWidth, pFontOsdTitle->Width(Title) + 2 * Roundness + (fShowSymbol ? 0 : (CHANNELINFOSYMBOLWIDTH + SmallGap)));
  }

  yTitleTop = 20;
  yTitleBottom = yTitleTop + pFontOsdTitle->Height();
  yTitleDecoTop = yTitleBottom + TitleDecoGap;
  yTitleDecoBottom = yTitleDecoTop + TitleDecoHeight;
  xLogoLeft = 0;
  xLogoRight = xLogoLeft + 32; //LogoSize;
  xLogoDecoLeft = xLogoRight + LogoDecoGap;
  xLogoDecoRight = xLogoDecoLeft + LogoDecoWidth;
  yLogoTop = yTitleDecoBottom + TitleDecoGap2;
  yLogoBottom = yLogoTop + NumTracks * lineHeight;
  xTitleLeft = fShowSymbol ? (ReelConfig.fullTitleWidth ? xLogoLeft : xLogoDecoRight + LogoDecoGap2) : 0;
  xTitleRight = xTitleLeft + FixWidth(ItemsWidth, 2) + Roundness;
  xListLeft = fShowSymbol ? (xLogoDecoRight + LogoDecoGap2) : 0;
  xListRight = xTitleRight;
  yListTop = yLogoTop + 19;
  yListBottom = yLogoBottom;
  xItemLeft = xListLeft + (ReelConfig.showMarker ? lineHeight : ListHBorder);
  xItemRight = xListRight - ListHBorder;
  xBottomLeft = xTitleLeft;
  xBottomRight = xTitleRight;
  yBottomTop = yListBottom + SmallGap + 20;
  yBottomBottom = yBottomTop + pFontDate->Height();

  // create osd
#ifdef REELVDR
  osd = cOsdProvider::NewTrueColorOsd(OsdSize.x, OsdSize.y + OsdSize.h - yBottomBottom - 20, Setup.OSDRandom );
#else
  osd = cOsdProvider::NewTrueColorOsd(OsdSize.x, OsdSize.y + OsdSize.h - yBottomBottom - 20, 0 );
#endif
  tArea Areas[] = { {xTitleLeft, 0 /*yTitleTop*/, xBottomRight - 1, yBottomBottom - 1, 32} };
  if ((Areas[0].bpp < 8 || ReelConfig.singleArea8Bpp) && osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea)) == oeOk) {
    debug("cSkinReelDisplayTracks: using %dbpp single area", Areas[0].bpp);
    osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
  } else {
    debug("cSkinReelDisplayTracks: using multiple areas");
    if (fShowSymbol) {
      tArea Areas[] = { {xTitleLeft, 0 /*yTitleTop*/, xTitleRight - 1, yTitleDecoBottom- 1, 2},
                         {xLogoLeft, yLogoTop, xLogoDecoRight - 1, yLogoBottom - 1, 4},
                         {xListLeft, yListTop, xListRight - 1, yListBottom - 1, 2},
                         {xBottomLeft, yBottomTop, xBottomRight - 1, yBottomBottom - 1, 2}
      };
      int rc = osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea));
      if (rc == oeOk)
        osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
      else {
        error("cSkinReelDisplayTracks: CanHandleAreas() returned %d", rc);
        delete osd;
        osd = NULL;
        throw 1;
        return;
      }
    } else {
      tArea Areas[] = { {xTitleLeft, 0/*yTitleTop*/, xTitleRight - 1, yTitleDecoBottom- 1, 2},
                         {xListLeft, yListTop, xListRight - 1, yListBottom - 1, 2},
                         {xBottomLeft, yBottomTop, xBottomRight - 1, yBottomBottom - 1, 2}
      };
      int rc = osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea));
      if (rc == oeOk)
        osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
      else {
        error("cSkinReelDisplayTracks: CanHandleAreas() returned %d", rc);
        delete osd;
        osd = NULL;
        throw 1;
        return;
      }
    }
  }

  SetImagePaths(osd);

  // clear all
  osd->DrawRectangle(0, 0, osd->Width(), osd->Height(), clrTransparent);
  // draw titlebar
  // Header
  osd->DrawImage(imgMenuHeaderLeft, xTitleLeft-10, yTitleTop, false);
  osd->DrawImage(imgMenuHeaderCenterX, xTitleLeft + Roundness, yTitleTop, false, xTitleRight-xTitleLeft - 2 * Roundness,1);
  osd->DrawImage(imgMenuHeaderRight, xTitleRight - Roundness , yTitleTop, false);

  osd->DrawText(xTitleLeft + Roundness, yTitleTop+15, Title,
                Theme.Color(clrTitleFg), clrTransparent, pFontOsdTitle,
                xTitleRight - Roundness - xTitleLeft - Roundness, yTitleBottom - yTitleTop,
                fShowSymbol ? taCenter : taLeft);
  // draw list area
  osd->DrawRectangle(xListLeft, yListTop, xListRight - 1, yListBottom - 1, Theme.Color(clrBackground));

  // Footer
  osd->DrawImage(imgMenuFooterLeft, xBottomLeft-10, yBottomTop, false);
  osd->DrawImage(imgMenuFooterCenterX, xBottomLeft + Roundness, yBottomTop, false, xBottomRight-xBottomLeft - 2 * Roundness,1);
  osd->DrawImage(imgMenuFooterRight, xBottomRight - Roundness , yBottomTop, false);

  /* only set audio channel if we really are in the audio menu, not when in subtitle-menu */
  if(!strcmp(Title, tr("Subtitles")))
      subTitleMenu = true;
  else
      subTitleMenu = false;

  SetAudioChannel(cDevice::PrimaryDevice()->GetAudioChannel());

  // fill up audio tracks
  for (int i = 0; i < NumTracks; i++)
    SetItem(Tracks[i], i, false);
}

cSkinReelDisplayTracks::~cSkinReelDisplayTracks()
{
  delete osd;
}

void cSkinReelDisplayTracks::SetItem(const char *Text, int Index, bool Current)
{
  int y = yListTop + Index * lineHeight;
  tColor ColorFg, ColorBg;
  if (Current) {
    ColorFg = Theme.Color(clrMenuItemSelectableFg);
    ColorBg = Theme.Color(clrAltBackground);
    currentIndex = Index;
  } else {
    ColorFg = Theme.Color(clrMenuItemSelectableFg);
    ColorBg = Theme.Color(clrBackground);
  }
  // draw track id
  osd->DrawRectangle(xListLeft, y, xListRight-1, y + lineHeight, ColorBg);
  if (ReelConfig.showMarker && Current) {
    osd->DrawImage(imgIconTimeractive, xListLeft + MarkerGap, y + MarkerGap-2, true);
  }
  osd->DrawText(xItemLeft, y + 5, Text, ColorFg, clrTransparent /*ColorBg*/, pFontListItem, xItemRight - xItemLeft-1, lineHeight);
}

void cSkinReelDisplayTracks::SetAudioChannel(int AudioChannel)
{
  if (fShowSymbol) {
    if (!(AudioChannel >= 0 && AudioChannel < MAX_AUDIO_BITMAPS))
      AudioChannel = 0;
    // DRAW AUDIO ICONS
    if(subTitleMenu)
       DrawUnbufferedImage(osd, "icon_subtitle.png", 0, 0, /*xLogoLeft + (xLogoRight - xLogoLeft - 64) / 2, yLogoTop + (yLogoBottom - yLogoTop - 64) / 2,*/ true);
    else
       DrawUnbufferedImage(osd, "icon_audiomenue.png", 0, 0, /*xLogoLeft + (xLogoRight - xLogoLeft - 64) / 2, yLogoTop + (yLogoBottom - yLogoTop - 64) / 2,*/ true);
    //osd->DrawImage(imgIconAudio, xLogoLeft + (xLogoRight - xLogoLeft - 64) / 2, yLogoTop + (yLogoBottom - yLogoTop - 64) / 2, true);
  } else {
    if (!(AudioChannel >= 0 && AudioChannel < MAX_AUDIO_BITMAPS))
      AudioChannel = 0;
  }
}

void cSkinReelDisplayTracks::SetTrack(int Index, const char *const *Tracks)
{
  //Reel::ReelBoxDevice::Instance()->SetAudioTrack(Index);

  if (currentIndex >= 0)
    SetItem(Tracks[currentIndex], currentIndex, false);
  SetItem(Tracks[Index], Index, true);

  int *index=new int; *index = Index;
  if(!subTitleMenu)
      cPluginManager::CallAllServices("ReelSetAudioTrack", index);
}

void cSkinReelDisplayTracks::Flush(void)
{
  /* TODO? remove date
  time_t now = time(NULL);
  if (now != lastTime) {
    lastTime = now;
    cString date = DayDateTime();
    osd->DrawText(xBottomLeft + Rounsvn dness, yBottomTop, date,
                  Theme.Color(clrTitleFg), Theme.Color(clrBottomBg),
                  pFontDate,
                  xBottomRight - Roundness - xBottomLeft - Roundness - 1,
                  yBottomBottom - yBottomTop - 1, taRight);
  }
  */
  osd->Flush();
}

