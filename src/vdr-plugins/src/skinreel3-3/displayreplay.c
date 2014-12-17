
#include <iostream>

#include "displayreplay.h"
#include "reel.h"
#include "config.h"
#include "logo.h"

/**
 * cSkinReelDisplayReplay
 */

cSkinReelDisplayReplay::cSkinReelDisplayReplay(bool ModeOnly)
{
  struct ReelOsdSize OsdSize;
  ReelConfig.GetOsdSize(&OsdSize);

  //std::cout << "NEW DISPLAYREPLAY" << std::endl;

  pFontOsdTitle = ReelConfig.GetFont(FONT_OSDTITLE);
  pFontReplayTimes = ReelConfig.GetFont(FONT_REPLAYTIMES);
  pFontDate = ReelConfig.GetFont(FONT_DATE);
  pFontLanguage = ReelConfig.GetFont(FONT_CILANGUAGE);
  pFontMessage = ReelConfig.GetFont(FONT_MESSAGE);

  strLastDate = NULL;

  int MessageHeight = std::max( pFontMessage->Height(), MINMESSAGEHEIGHT);

  modeonly = ModeOnly;
  nJumpWidth = 0;
  nCurrentWidth = 0;
  fShowSymbol = ReelConfig.showSymbols && ReelConfig.showSymbolsReplay;

  int LogoSize = Gap + IconHeight + Gap;
  LogoSize += (LogoSize % 2 ? 1 : 0);
  yTitleTop = 10;
  yTitleBottom = yTitleTop + IMGHEADERHEIGHT;
  xLogoLeft = (IMGCHANNELLOGOWIDTH-IMGREPLAYLOGOHEIGHT)/2;
  xLogoRight = xLogoLeft + IMGREPLAYLOGOWIDTH + Gap;
  xTitleLeft = ReelConfig.fullTitleWidth ? 0 : xLogoRight + Gap;
  xTitleRight = OsdSize.w;
  yLogoTop = yTitleBottom + Gap - 2; // -2 is finetuning, so the Icon looks well next to Progressbar
  yLogoBottom = yLogoTop + IMGREPLAYLOGOHEIGHT;
  xProgressLeft = xTitleLeft;
  xProgressRight = xTitleRight;
  yProgressTop = yTitleBottom + Gap;
  yProgressBottom = yLogoBottom - 11; // -11 is finetuning, so the Icon looks well next to Progressbar
  yBottomTop = yProgressBottom + Gap;
  xBottomLeft = xTitleLeft;
  xBottomRight = xTitleRight;
  yBottomBottom = yBottomTop + std::max(pFontDate->Height(), pFontLanguage->Height()) + 20;
  xMessageLeft = xProgressLeft;
  xMessageRight = xProgressRight;
  yMessageTop = yLogoTop + (LogoSize - MessageHeight) / 2;
  yMessageBottom = yMessageTop + MessageHeight;
  xFirstSymbol = xBottomRight - Roundness;

  // create osd
#ifdef REELVDR
  osd = cOsdProvider::NewTrueColorOsd(OsdSize.x, OsdSize.y + OsdSize.h - yBottomBottom, Setup.OSDRandom);
#else
  osd = cOsdProvider::NewTrueColorOsd(OsdSize.x, OsdSize.y + OsdSize.h - yBottomBottom, 0);
#endif
  tArea Areas[] = { {std::min(xTitleLeft, xLogoLeft), 0 /*yTitleTop*/, xBottomRight - 1, yBottomBottom - 1, 32} }; // TrueColorOsd complain if <32
  if ((Areas[0].bpp < 8 || ReelConfig.singleArea8Bpp) && osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea)) == oeOk) {
    debug("cSkinReelDisplayReplay: using %dbpp single area", Areas[0].bpp);
    osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
  } else {
    debug("cSkinReelDisplayReplay: using multiple areas");
    int rc = osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea));
    if (rc == oeOk)
      osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
    else {
      error("cSkinReelDisplayReplay: CanHandleAreas() returned %d", rc);
      delete osd;
      osd = NULL;
      throw 1;
      return;
    }
  }
  // clear all
  osd->DrawRectangle(0, 0, osd->Width(), osd->Height(), clrTransparent);
  if (!modeonly)
  {
    // draw progress bar area
    osd->DrawRectangle(xProgressLeft, yProgressTop, xProgressRight - 1, yProgressBottom - 1, Theme.Color(clrReplayBarAreaBg));

    // draw bottom area
    DrawBottomBar();

    xFirstSymbol = DrawStatusSymbols(0, xFirstSymbol, yBottomTop, yBottomBottom) - Gap;
  }
}

void cSkinReelDisplayReplay::DrawBottomBar()
{
    osd->DrawImage(imgMenuFooterLeft, xBottomLeft-10, yBottomTop, false);
    osd->DrawImage(imgMenuFooterCenterX, xBottomLeft + Roundness, yBottomTop,
                   false, xBottomRight - xBottomLeft - Roundness*2);
    osd->DrawImage(imgMenuFooterRight, xBottomRight - Roundness, yBottomTop, false);
}


void cSkinReelDisplayReplay::RedrawCurrent()
{
    SetCurrent(Current_);
}


void cSkinReelDisplayReplay::RedrawTotal()
{
    SetTotal(Total_);
}


void cSkinReelDisplayReplay::SetHelp(const char* redText, const char* greenText,
                                     const char* yellowText, const char* blueText)
{
    int len = (xBottomRight - xBottomLeft);
    int space_btwn_icons = len/6;
    int start_x = xBottomLeft + Roundness + space_btwn_icons;
    int icon_y = (yBottomBottom + yBottomTop)/2 - 18 /*height of icons*/ + 5;

    const cFont *fontHelpText = pFontReplayTimes;

    /**/
    redText_    = redText;
    greenText_  = greenText;
    yellowText_ = yellowText;
    blueText_   = blueText;

    if (redText)
    {
        DrawUnbufferedImage(osd, "small_button_red.png", start_x, icon_y, true);
        osd->DrawText(start_x + 18 + Gap, icon_y, redText,
                      Theme.Color(clrTitleFg), clrTransparent, fontHelpText,
                      space_btwn_icons - 18 - 2*Gap,
                      yBottomBottom - yBottomTop -10);
    }
    start_x += space_btwn_icons;

    if (greenText)
    {
        DrawUnbufferedImage(osd, "small_button_green.png", start_x, icon_y, true);
        osd->DrawText(start_x + 18 + Gap, icon_y, greenText,
                      Theme.Color(clrTitleFg), clrTransparent, fontHelpText,
                      space_btwn_icons - 18 - 2*Gap,
                      yBottomBottom - yBottomTop -10);
    }
    start_x += space_btwn_icons;

    if (yellowText)
    {
        DrawUnbufferedImage(osd, "small_button_yellow.png", start_x, icon_y, true);
        osd->DrawText(start_x + 18 + Gap, icon_y, yellowText,
                      Theme.Color(clrTitleFg), clrTransparent, fontHelpText,
                      space_btwn_icons - 18 - 2*Gap,
                      yBottomBottom - yBottomTop -10);
    }
    start_x += space_btwn_icons;

    if (blueText)
    {
        DrawUnbufferedImage(osd, "small_button_blue.png", start_x, icon_y, true);
        osd->DrawText(start_x + 18 + Gap, icon_y, blueText,
                      Theme.Color(clrTitleFg), clrTransparent, fontHelpText,
                      space_btwn_icons - 18 - 2*Gap,
                      yBottomBottom - yBottomTop -10);
    }

}


cSkinReelDisplayReplay::~cSkinReelDisplayReplay()
{
  free(strLastDate);
  delete osd;
}

void cSkinReelDisplayReplay::SetTitle(const char *Title)
{
    DrawTitle(Title);
}

void cSkinReelDisplayReplay::DrawTitle(const char *Title)
{

  // draw title area
  osd->DrawImage(imgMenuHeaderLeft, xTitleLeft-10, yTitleTop, false);
  osd->DrawImage(imgMenuHeaderCenterX, xTitleLeft + Roundness, yTitleTop, false, xTitleRight - xTitleLeft - Roundness*2);
  osd->DrawImage(imgMenuHeaderRight, xTitleRight - Roundness, yTitleTop, false);

  if (Title) {
    debug("REPLAY TITLE: %s", Title);
    // draw titlebar
    osd->DrawText(xTitleLeft + Roundness+10, yTitleTop+15, Title,
                  Theme.Color(clrTitleFg), clrTransparent, pFontOsdTitle,
                  xTitleRight - Roundness - xTitleLeft - Roundness - 3 -10,
                  yTitleBottom - yTitleTop-10);
  }
}

void cSkinReelDisplayReplay::SetMode(bool Play, bool Forward, int Speed)
{
  if (Speed < -1)
    Speed = -1;

  if (modeonly)
    osd->DrawRectangle(xLogoLeft, yLogoTop, xLogoRight - 1, yLogoBottom - 1, clrTransparent);

  // clear ControlIcon
  osd->DrawRectangle(xLogoLeft, yLogoTop, xLogoRight, yBottomBottom, clrTransparent);

  if (Speed == -1)
    osd->DrawImage(Play?imgIconControlPlay:imgIconControlPause, xLogoLeft, yLogoTop, true);
  else if (Play) {
    if (Speed > MAX_SPEED_BITMAPS) {
      error("MAX SPEED %d > 9", Speed);
      Speed = MAX_SPEED_BITMAPS;
    }

    osd->DrawImage(Forward?imgIconControlFastFwd+Speed:imgIconControlFastRew+Speed, xLogoLeft, yLogoTop, true);
  } else {                      // trick speed
    if (Speed > MAX_TRICKSPEED_BITMAPS) {
      error("MAX SPEED %d > 3", Speed);
      Speed = MAX_TRICKSPEED_BITMAPS;
    }
    osd->DrawImage(Forward?imgIconControlSkipFwd+Speed:imgIconControlSkipRew+Speed, xLogoLeft, yLogoTop, true);
  }
}

void cSkinReelDisplayReplay::SetProgress(int Current, int Total)
{
 // create progressbar
  cProgressBar pb(xProgressRight - xProgressLeft - 2 * BigGap,
                  yProgressBottom - yProgressTop - 2 * BigGap, Current, Total,
                  marks, Theme.Color(clrReplayProgressSeen),
                  Theme.Color(clrReplayProgressRest),
                  Theme.Color(clrReplayProgressSelected),
                  Theme.Color(clrReplayProgressMark),
                  Theme.Color(clrReplayProgressCurrent));
  // draw progressbar
  osd->DrawBitmap(xProgressLeft + BigGap, yProgressTop + BigGap, pb);
}

void cSkinReelDisplayReplay::SetCurrent(const char *Current)
{
  if (!Current)
    return;

  Current_ = Current;

  // draw current time
  int w = pFontReplayTimes->Width(Current);
  //FLOH
  osd->DrawImage(imgMenuFooterCenterX, xBottomLeft + Roundness, yBottomTop, false, w);
  osd->DrawText(xBottomLeft + Roundness, yBottomTop-5, Current,
                Theme.Color(clrReplayCurrent), clrTransparent, pFontReplayTimes,
                w, yBottomBottom - yBottomTop, taLeft);
  if (nCurrentWidth > w)
    osd->DrawImage(imgMenuFooterCenterX, xBottomLeft + Roundness + w, yBottomTop, false, xBottomLeft + Roundness + nCurrentWidth);
  nCurrentWidth = w;
}

void cSkinReelDisplayReplay::SetTotal(const char *Total)
{
  if (!Total)
    return;

  Total_ = Total;

  // draw total time
  int w = pFontReplayTimes->Width(Total);
  osd->DrawImage(imgMenuFooterCenterX, xBottomRight - Roundness - w, yBottomTop, false, w);
  osd->DrawText(xBottomRight - Roundness - w, yBottomTop-5, Total, Theme.Color(clrReplayTotal), clrTransparent, pFontReplayTimes, w, yBottomBottom - yBottomTop, taRight);
}

void cSkinReelDisplayReplay::SetJump(const char *Jump)
{
    int len = (xBottomRight - xBottomLeft);
    int space_btwn_icons = len/6;
  /* erase old prompt */
  osd->DrawRectangle(xBottomLeft + (xBottomRight - xBottomLeft - nJumpWidth) / 2, yBottomTop+15, xBottomLeft + (xBottomRight - xBottomLeft - nJumpWidth) / 2 + nJumpWidth+3, yBottomTop + 16, clrTransparent); /* TB: without this line the DrawImage() in the line below cannot be seen???? */
  osd->DrawImage( imgMenuFooterCenterX, xBottomLeft + space_btwn_icons, yBottomTop, false, space_btwn_icons*4, 1);


  if (Jump) {
    /* draw jump prompt */
    nJumpWidth = pFontReplayTimes->Width(Jump);
    osd->DrawText(xBottomLeft + (xBottomRight - xBottomLeft - nJumpWidth) / 2, yBottomTop-5, Jump, Theme.Color(clrReplayModeJump), clrTransparent, pFontReplayTimes, nJumpWidth, yBottomBottom - yBottomTop, taLeft);
  } else
      RedrawHelpKeys(); // redraw help keys
}

void cSkinReelDisplayReplay::RedrawHelpKeys()
{
    SetHelp(redText_, greenText_, yellowText_, blueText_);
}

void cSkinReelDisplayReplay::SetMessage(eMessageType Type, const char *Text)
{

  int x0 = xMessageLeft, x1 = xMessageLeft + Roundness, x2 = xMessageRight - Roundness;
  int y0 = yMessageTop, y2 = y0 + 30; // BarimageHeight

  int y1 = (y2 + y0 - pFontMessage->Height()) / 2; // yText

  tColor  colorfg = 0xFFFFFFFF;
  int imgMessageIcon;

       switch(Type)
        {
            case mtStatus:
                imgMessageIcon = imgIconMessageInfo;
                break;
            case mtWarning:
                imgMessageIcon = imgIconMessageWarning;
                break;
            case mtError:
                imgMessageIcon = imgIconMessageError;
                break;
            default:
                imgMessageIcon = imgIconMessageNeutral;
                break;
        }
        colorfg = 0xFFFFFFFF;

        // clear bottom bar
        DrawBottomBar();

    if (Text)
    {
#define messageIconW 30

        osd->DrawImage(imgMessageIcon, xBottomLeft + Roundness, yBottomTop, true);


        int x = xBottomLeft + Roundness + messageIconW;
        int w = xBottomRight -Roundness - x;
        int y = (yBottomBottom + yBottomTop)/2 - 18 /*height of icons*/ + 5; // taken from SetHelp()

        //display message
          osd->DrawText(x, y, Text,
                        colorfg, clrTransparent,
                        pFontMessage,
                        w, 0,
                        taCenter);
    }
    else
    {
        // no text, so restore the bottom bar, draw helpkeys, current and total
      RedrawHelpKeys();
      RedrawTotal();
      RedrawCurrent();
    }
}

void cSkinReelDisplayReplay::Flush(void)
{
  osd->Flush();
}

