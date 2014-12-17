
#include <sstream>
#include <iostream>

#include <math.h>

#include <vdr/device.h>
#ifndef DEVICE_ATTRIBUTES
#include <sys/ioctl.h>
#endif
#include <vdr/menu.h>

#ifndef REELVDR
#include <linux/dvb/frontend.h>
#endif
#if VDRVERSNUM >= 10716
#include <linux/dvb/frontend.h>
#endif

#include "displaychannel.h"
#include "config.h"
#include "reel.h"
#include "logo.h"

/**
 * cSkinReelDisplayChannel
 */ 

cSkinReelDisplayChannel::cSkinReelDisplayChannel(bool WithInfo)
{
  debug("cSkinReelDisplayChannel::cSkinReelDisplayChannel(%d)", WithInfo);

  xSymbol = 0;
  isMessage = false;
  strBarWidth = 10000; snrBarWidth = 10000;
  fWithInfo = WithInfo;
  struct ReelOsdSize OsdSize;
  ReelConfig.GetOsdSize(&OsdSize);

  lastAspect = 0;

  pFontOsdTitle = ReelConfig.GetFont(FONT_OSDTITLE);
  pFontDate = ReelConfig.GetFont(FONT_DATE);
  pFontTitle = ReelConfig.GetFont(FONT_CITITLE);
  pFontSubtitle = ReelConfig.GetFont(FONT_CISUBTITLE);
  pFontLanguage = ReelConfig.GetFont(FONT_CILANGUAGE);
  pFontMessage = ReelConfig.GetFont(FONT_MESSAGE);

  fShowLogo = ReelConfig.showLogo;
  xFirstSymbol = 0;
  nMessagesShown = 0;
  strLastDate = NULL;

  static const int MessageHeight = std::max(pFontMessage->Height(), MINMESSAGEHEIGHT);
  int LogoSize = 0;
  if (fWithInfo)
    LogoSize = std::max(pFontTitle->Height() * 2 + pFontSubtitle->Height() * 2 /*TB: he?! + SmallGap*0 */, ChannelLogoHeight);
  else
    LogoSize = std::max(std::max(pFontOsdTitle->Height(), pFontDate->Height()) + TitleDecoGap + TitleDecoHeight + TitleDecoGap2 + MessageHeight + pFontLanguage->Height() + SmallGap, ChannelLogoHeight);

  LogoSize += (LogoSize % 2 ? 1 : 0);
  // title bar & logo area
  xLogoLeft = 10;
  xLogoRight = xLogoLeft + LogoSize;
  xLogoDecoLeft = xLogoRight + LogoDecoGap;
  xLogoDecoRight = xLogoDecoLeft + LogoDecoWidth;
  xTitleLeft = (fShowLogo && (!ReelConfig.fullTitleWidth || !fWithInfo) ? xLogoDecoRight + LogoDecoGap2 : xLogoLeft);
  xTitleRight = xTitleLeft + ((OsdSize.w - xTitleLeft) & ~0x07); // width must be multiple of 8
  yTitleTop = 0;
  yTitleBottom = yTitleTop + std::max(std::max(pFontOsdTitle->Height(), pFontDate->Height()), IMGHEADERHEIGHT) ;
  yTitleDecoTop = yTitleBottom + TitleDecoGap;
  yTitleDecoBottom = yTitleDecoTop + TitleDecoHeight;
  yLogoTop = (fWithInfo ? yTitleDecoBottom + TitleDecoGap2 : yTitleTop);
  //yLogoBottom = yLogoTop + LogoSize;
  xLogoPos = xLogoLeft + (LogoSize - ChannelLogoHeight) / 2;
  yLogoPos = yLogoTop + (LogoSize - ChannelLogoHeight) / 2;
  if (fWithInfo) {
    /* current event area */
    xEventNowLeft = (fShowLogo ? xLogoDecoRight + LogoDecoGap2 : xTitleLeft);
    xEventNowRight = xTitleRight;
    yEventNowTop = yLogoTop;
    yEventNowBottom = yEventNowTop + pFontTitle->Height() + pFontSubtitle->Height();
    /* next event area */
    xEventNextLeft = xEventNowLeft;
    xEventNextRight = xEventNowRight;
    yEventNextTop = yEventNowBottom + SmallGap;
    yEventNextBottom = yEventNextTop + pFontTitle->Height() + pFontSubtitle->Height();
  } else {
    xEventNowLeft   = xEventNextLeft   = (fShowLogo ? xLogoDecoRight + LogoDecoGap2 : xTitleLeft);
    xEventNowRight  = xEventNextRight  = xTitleRight;
    yEventNowTop    = yEventNextTop    = yTitleDecoBottom + TitleDecoGap2;
    yEventNowBottom = yEventNextBottom = yLogoBottom - pFontLanguage->Height() - SmallGap;
  }
  yLogoBottom = yEventNextBottom;

  /* progress bar area */
  xBottomLeft = xTitleLeft;
  xBottomRight = xTitleRight;
  yBottomTop = yEventNextBottom + SmallGap;
  yBottomBottom = yBottomTop + std::max(pFontLanguage->Height(), IMGFOOTERHEIGHT);
  /* message area */
  xMessageLeft = xEventNowLeft;
  xMessageRight = xTitleRight;
  yMessageTop = yLogoTop + (LogoSize - MessageHeight) / 2;
  yMessageBottom = yMessageTop + MessageHeight;
  /* date area */
  cString date = GetFullDateTime();
  int w = pFontDate->Width(date);
  xDateLeft = xTitleRight - Roundness - w - SmallGap;

  /* create osd */
#ifdef REELVDR
  osd = cOsdProvider::NewTrueColorOsd(OsdSize.x, OsdSize.y + (Setup.ChannelInfoPos ? 0 : (OsdSize.h - yBottomBottom)), Setup.OSDRandom );
#else
  osd = cOsdProvider::NewTrueColorOsd(OsdSize.x, OsdSize.y + (Setup.ChannelInfoPos ? 0 : (OsdSize.h - yBottomBottom)), 0 );
#endif
  tArea Areas[] = { {0, 0, xBottomRight - 1, yBottomBottom - 1, 32} };
  if ((Areas[0].bpp < 8 || ReelConfig.singleArea8Bpp) && osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea)) == oeOk) {
    debug("cSkinReelDisplayChannel: using %dbpp single area", Areas[0].bpp);
    osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
    // clear all
    //TB: unnecessary: //osd->DrawRectangle(0, 0, osd->Width(), osd->Height(), clrTransparent);
  } else {
    debug("cSkinReelDisplayChannel: using multiple areas");
    if (fShowLogo) {
      tArea Areas[] = { {0 /*xLogoLeft*/, yLogoTop, xLogoDecoRight - 1, yLogoBottom - 1, 4},
                        {0 /*xTitleLeft*/, yTitleTop, xTitleRight - 1, yTitleDecoBottom - 1, 2},
                        {xEventNowLeft, yEventNowTop, xEventNowRight - 1, yEventNextBottom - 1, 4},
                        {xBottomLeft, yBottomTop, xBottomRight - 1, yBottomBottom - 1, 4}
                      };
      eOsdError rc = osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea));
      if (rc == oeOk)
        osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
      else {
        error("cSkinReelDisplayChannel: CanHandleAreas() returned %d", rc);
        delete osd;
        osd = NULL;
        throw 1;
        return;
      }
    } else {
      tArea Areas[] = { {0 /*xTitleLeft*/, yTitleTop, xTitleRight - 1, yTitleDecoBottom - 1, 2},
                        {xEventNowLeft, yEventNowTop, xEventNowRight - 1, yEventNextBottom - 1, 4},
                        {xBottomLeft, yBottomTop, xBottomRight - 1, yBottomBottom - 1, 4}
      };
      eOsdError rc = osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea));
      if (rc == oeOk)
        osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
      else {
        error("cSkinReelDisplayChannel: CanHandleAreas() returned %d", rc);
        delete osd;
        osd = NULL;
        throw 1;
        return;
      }
    }
  }

  SetImagePaths(osd);

  DrawAreas();
}

cSkinReelDisplayChannel::~cSkinReelDisplayChannel() {
  debug("cSkinReelDisplayChannel::~cSkinReelDisplayChannel()");
  free(strLastDate);
  delete osd;
}

void cSkinReelDisplayChannel::DrawAreas(void) {
  /* draw titlebar */
  /* Header */
  osd->DrawImage(imgMenuHeaderLeft, xTitleLeft-10, yTitleTop, false);
  osd->DrawImage(imgMenuHeaderCenterX, xTitleLeft + Roundness, yTitleTop, false, xTitleRight-xTitleLeft - 2 * Roundness,1);
  osd->DrawImage(imgMenuHeaderRight, xTitleRight - Roundness , yTitleTop, false);

  // about LOGO : Channel symbol header
  osd->DrawImage(imgMenuHeaderLeft, xLogoLeft-10, yTitleTop, false);
  osd->DrawImage(imgMenuHeaderCenterX, xLogoLeft + Roundness, yTitleTop, false, xLogoRight-xLogoLeft - 2 * Roundness,1);
  osd->DrawImage(imgMenuHeaderRight, xLogoRight - Roundness, yTitleTop, true);

  // draw current event area
  osd->DrawRectangle(xEventNowLeft, yEventNowTop, xEventNowRight - 1, yEventNowBottom - 1, Theme.Color(clrBackground));

  if (fWithInfo) /* draw next event area */
    osd->DrawRectangle(xEventNextLeft, yEventNextTop, xEventNextRight - 1, yEventNextBottom - 1, Theme.Color(clrAltBackground));

  /* draw progress bar area */
  /* Footer */
  osd->DrawImage(imgMenuFooterLeft, xBottomLeft-10, yBottomTop, false);
  osd->DrawImage(imgMenuFooterCenterX, xBottomLeft + Roundness, yBottomTop, false, xBottomRight-xBottomLeft - 2 * Roundness,1);
  osd->DrawImage(imgMenuFooterRight, xBottomRight - Roundness , yBottomTop, false);

  /* Channel symbol */
  osd->DrawImage(imgMenuFooterLeft, xLogoLeft-10, yBottomTop, false);
  osd->DrawImage(imgMenuFooterCenterX, xLogoLeft + Roundness, yBottomTop, false, xLogoRight-xLogoLeft - 2 * Roundness,1);
  osd->DrawImage(imgMenuFooterRight, xLogoRight - Roundness , yBottomTop, true);

  /* Draw RMM Logo */
  if(fShowLogo) {
      /* logo area */
      osd->DrawRectangle(xLogoLeft, yLogoTop, xLogoRight - 1, yLogoBottom - 1, Theme.Color(clrBackground));
      osd->DrawRectangle(xLogoDecoLeft, yLogoTop, xLogoDecoRight - 1, yLogoBottom - 1, Theme.Color(clrBackground));
      /* Draw LOGO */
      osd->DrawImage(imgNoChannelIcon,xLogoLeft + (xLogoRight - xLogoLeft - IMGCHANNELLOGOWIDTH) / 2,
              yLogoTop + (yLogoBottom - yLogoTop - IMGCHANNELLOGOHEIGHT) / 2, true);
  }
}

void cSkinReelDisplayChannel::DrawGroupInfo(const cChannel *Channel, int Number) {
  DrawAreas();
  DrawTitle(GetChannelName(Channel));
}

void cSkinReelDisplayChannel::DrawTitle(const char *Title) {
  if (Title) {
    int xName = (fShowLogo && ReelConfig.fullTitleWidth && fWithInfo ? xEventNowLeft : xTitleLeft + Roundness + pFontOsdTitle->Width("0000-") + Gap);
    // draw titlebar
    int y = yTitleTop + (yTitleBottom - yTitleTop - pFontOsdTitle->Height()) / 2 + 5;
    // draw channel group name
    osd->DrawText(xName, y, Title, Theme.Color(clrTitleFg), clrTransparent, pFontOsdTitle, xDateLeft - SmallGap - xName - 3, yTitleBottom - y);
  }
}

void cSkinReelDisplayChannel::DrawChannelInfo(const cChannel *Channel, int Number) {
  //osd->SetAntiAliasGranularity(255,255);
  DrawAreas();

  int xNumber = xTitleLeft + Roundness;
  int xName = xNumber + pFontOsdTitle->Width("0000-") + Gap;
  if (fShowLogo && ReelConfig.fullTitleWidth && fWithInfo) {
    xNumber = xTitleLeft + Roundness;
    xName = xEventNowLeft;
  }

  // draw channel number
  osd->DrawText(xNumber, yTitleTop+12, GetChannelNumber(Channel, Number),
                Theme.Color(clrTitleFg), clrTransparent, pFontOsdTitle,
                xName - xNumber - Gap, yTitleBottom - yTitleTop - 14, taCenter);

  // draw channel name
  DrawTitle(GetChannelName(Channel));

  if (fWithInfo && ReelConfig.showStatusSymbols)
    DrawSymbols(Channel);
}

void cSkinReelDisplayChannel::DrawSymbols(const cChannel *Channel) {
  // draw symbols
  // right edge of logo
  xSymbol = xBottomRight - Roundness;
  // bottom edge of logo
  //int ys = yBottomTop + (yBottomBottom - yBottomTop - SymbolHeight) / 2;
  int ys = yTitleTop + (yTitleBottom - yTitleTop - SymbolHeight) / 2 + 5;

  bool rec = false;
#ifdef USEMYSQL
  if(Setup.ReelboxMode == eModeClient)
  {
      std::vector<cTimer*> InstantRecordings;
      Timers.GetInstantRecordings(&InstantRecordings);
      if(InstantRecordings.size())
      {
          std::vector<cTimer*>::iterator TimerIterator = InstantRecordings.begin();
          while(!rec && TimerIterator != InstantRecordings.end())
          {
              if((*TimerIterator)->Recording() && !strcmp((*TimerIterator)->Channel()->Name(), Channel->Name()))
                  rec=true;
              ++TimerIterator;
          }
      }
  }
  else
#endif
      rec = cRecordControls::Active();

  xSymbol -= (CHANNELINFOSYMBOLWIDTH + SmallGap);
  osd->DrawImage(rec ? imgRecordingOn:imgRecordingOff, xSymbol,ys,true);

  /* draw audio symbol according to current audio channel */
  int AudioMode = cDevice::PrimaryDevice()->GetAudioChannel();
  if (!(AudioMode >= 0 && AudioMode < MAX_AUDIO_BITMAPS))
    AudioMode = 0;
  xSymbol -= ( CHANNELINFOSYMBOLWIDTH + SmallGap);
  osd->DrawImage(imgAudio,xSymbol,ys,true);

  /* draw dolby digital symbol */
  xSymbol -= (CHANNELINFOSYMBOLWIDTH + SmallGap + 12);
  osd->DrawImage(Channel->Dpid(0) ? imgDolbyDigitalOn:imgDolbyDigitalOff, xSymbol,ys,true);

  /* Teletext */
  xSymbol -= (CHANNELINFOSYMBOLWIDTH + SmallGap);
  osd->DrawImage(Channel->Tpid()? imgTeletextOn: imgTeletextOff, xSymbol, ys, true);

  /* draw encryption symbol */
  xSymbol -= (CHANNELINFOSYMBOLWIDTH + SmallGap);
  osd->DrawImage(Channel->Ca()? imgEncryptedOn: imgEncryptedOff, xSymbol, ys, true);
  xSymbol -= (CHANNELINFOSYMBOLWIDTH + SmallGap);

  /* Draw FREE TO AIR, NAGRAVISION in Footer */
  UpdateSignal();
#ifdef REELVDR
  UpdateAspect();
#endif
#define imgChannelInfoIconHeight 18
  int y = yBottomTop + (yBottomBottom - yBottomTop)/2 - imgChannelInfoIconHeight/2 - 5; // Footer
  int xs = xeSignalBar + Gap; // end of signal bar

  std::string imgCa = "";
  switch (Channel->Ca()) {
        case 0x0000: /* Reserved */ osd->DrawImage(imgFreetoair, xs, y, true); break;
        case 0x0100 ... 0x01FF: /* Canal Plus */ imgCa = uimgSeca; break;
        case 0x0500 ... 0x05FF: /* France Telecom */ imgCa = uimgViaccess; break;
        case 0x0600 ... 0x06FF: /* Irdeto */ imgCa = uimgIrdeto; break;
        case 0x0900 ... 0x09FF: /* News Datacom */ imgCa = uimgNds; break;
        case 0x0B00 ... 0x0BFF: /* Norwegian Telekom */ imgCa = uimgConax; break;
        case 0x0D00 ... 0x0DFF: /* Philips */ imgCa = uimgCryptoworks; break;
        case 0x0E00 ... 0x0EFF: /* Scientific Atlanta */ imgCa = uimgPowervu; break;
        case 0x1200 ... 0x12FF: /* BellVu Express */ imgCa = uimgNagravision; break;
        case 0x1700 ... 0x17FF: /* BetaTechnik */ imgCa = uimgBetacrypt; break;
        case 0x1800 ... 0x18FF: /* Kudelski SA */ imgCa = uimgNagravision; break;
        case 0x4A60 ... 0x4A6F: /* @Sky */ imgCa = uimgSkycrypt; break;
        case 0x4AE0 ... 0x4AEF: /* DreCrypt */ imgCa = uimgDrecrypt; break;
        default: imgCa = ""; break;
  }

  if(imgCa != "")
    DrawUnbufferedImage(osd, imgCa, xs, y, true);

#define UGLY_SOURCE_INFO 1
#ifdef UGLY_SOURCE_INFO
  if (ReelConfig.showSourceInChannelinfo)
    osd->DrawText(xDateLeft - 125, yBottomTop - 5 + (yBottomBottom - yBottomTop)/2 - pFontDate->Height()/2, *cSource::ToString(Channel->Source()), Theme.Color(clrTitleFg), clrTransparent, pFontDate, 50, yBottomBottom - yBottomTop-10);
#endif

  xFirstSymbol = DrawStatusSymbols(xBottomLeft + Roundness + MIN_CI_PROGRESS + Gap, xSymbol, yTitleTop, yTitleBottom, Channel) - Gap;
}

cString cSkinReelDisplayChannel::GetChannelName(const cChannel *Channel) {
  std::stringstream buffer;

  /* check if channel exists */
  if (Channel)
#ifndef UGLY_SOURCE_INFO
    if (ReelConfig.showSourceInChannelinfo)
      buffer << Channel->Name() << "  -  (" << *cSource::ToString(Channel->Source()) << ")";
    else
#endif
      buffer << Channel->Name();
  else
    buffer << tr("*** Invalid Channel ***");
  return buffer.str().c_str();
}

cString cSkinReelDisplayChannel::GetChannelNumber(const cChannel *Channel, int Number) {
  static char buffer[256];
  // check if real channel exists
  if (Channel && !Channel->GroupSep()) {
    snprintf(buffer, sizeof(buffer), "%02d%s", Channel->Number(), Number ? "-" : "");
  } else if (Number) {
    // no channel but number
    snprintf(buffer, sizeof(buffer), "%d-", Number);
  } else {
    // no channel and no number
    strcpy(buffer, " ");
  }
  return buffer;
}

void cSkinReelDisplayChannel::DrawChannelLogo(const cChannel *channel) {
 if (!fShowLogo)
   return; /* no logo to be shown */

 if (!channel) { /* Draw RMM logo if no channel/ no logo/no logoDir is set */
     /* logo area */
     osd->DrawRectangle(xLogoLeft, yLogoTop, xLogoRight - 1, yLogoBottom - 1, Theme.Color(clrBackground)); // the real logo background
     osd->DrawRectangle(xLogoDecoLeft, yLogoTop, xLogoDecoRight - 1, yLogoBottom - 1, Theme.Color(clrBackground));
         /* RMM Logo */
     osd->DrawImage(imgNoChannelIcon,xLogoLeft + (xLogoRight - xLogoLeft - IMGCHANNELLOGOWIDTH) / 2,
             yLogoTop + (yLogoBottom - yLogoTop - IMGCHANNELLOGOHEIGHT) / 2, true); 
     dsyslog("Drew RMM Logo since *channel== %#x && fShowLogo == %i \n", (unsigned int)channel, fShowLogo);
     return ;
 }

 std::string logoPath = std::string(ReelConfig.GetLogoDir()) + "/" + channel->Name() + ".png";

 /* check for png file */
 if ( access(logoPath.c_str(),R_OK) == 0 ) { /* file can be read */
     /* logo area */
     osd->DrawRectangle(xLogoLeft, yLogoTop, xLogoRight - 1, yLogoBottom - 1, Theme.Color(clrLogoBg)); // the real logo background
     osd->DrawRectangle(xLogoDecoLeft, yLogoTop, xLogoDecoRight - 1, yLogoBottom - 1, Theme.Color(clrLogoBg));
     /* draw logo */
     osd->SetImagePath(imgUnbufferedEnum, logoPath.c_str());
     osd->DrawImage(imgUnbufferedEnum, xLogoLeft + (xLogoRight - xLogoLeft - IMGCHANNELLOGOWIDTH) / 2,
             yLogoTop + (yLogoBottom - yLogoTop - IMGCHANNELLOGOHEIGHT) / 2, true);
 } else {
     /* logo area */
     osd->DrawRectangle(xLogoLeft, yLogoTop, xLogoRight - 1, yLogoBottom - 1, Theme.Color(clrBackground)); // the real logo background
     osd->DrawRectangle(xLogoDecoLeft, yLogoTop, xLogoDecoRight - 1, yLogoBottom - 1, Theme.Color(clrBackground));
     /* RMM Logo */
     osd->DrawImage(imgNoChannelIcon,xLogoLeft + (xLogoRight - xLogoLeft - IMGCHANNELLOGOWIDTH) / 2,
             yLogoTop + (yLogoBottom - yLogoTop - IMGCHANNELLOGOHEIGHT) / 2, true);
     isyslog("(%s:%i) INFO: no CHANNEL LOGO %s\n",__FILE__,__LINE__,logoPath.c_str());
 }
} //end DrawChannelLogo


void cSkinReelDisplayChannel::SetChannel(const cChannel *Channel, int Number) {
  //printf("cSkinReelDisplayChannel::SetChannel(%#x (%i),%i)\n",Channel, Channel? Channel->Number():-1,Number);

  xFirstSymbol = 0;
  free(strLastDate);
  strLastDate = NULL;

  osd->DrawRectangle(0, 0, osd->Width(), osd->Height(), clrTransparent);

  if (Channel) {
    if (Channel->GroupSep())
      DrawGroupInfo(Channel, Number);
    else
      DrawChannelInfo(Channel, Number);
  }

  DrawChannelLogo(Channel);
}

void cSkinReelDisplayChannel::SetEvents(const cEvent *Present, const cEvent *Following) {
  //TODO: update symbols
  debug("cSkinReelDisplayChannel::SetEvents(%p, %p)", Present, Following);

  if (!fWithInfo)
    return;

  // Draw the event area again, since DrawText does not clear the already drawn text but blends over it!
  // taken from DrawAreas()

  // draw current event area
  osd->DrawRectangle(xEventNowLeft, yEventNowTop, xEventNowRight - 1, yEventNowBottom - 1, Theme.Color(clrBackground));
  // draw next event area
  osd->DrawRectangle(xEventNextLeft, yEventNextTop, xEventNextRight - 1, yEventNextBottom - 1, Theme.Color(clrAltBackground));

  int xTimeLeft = xEventNowLeft + Gap;
  int xTimeWidth = pFontTitle->Width("00:00");
  static const int lineHeightTitle = pFontTitle->Height();
  static const int lineHeightSubtitle = pFontSubtitle->Height();

  // check epg datas
  const cEvent *e = Present;    // Current event
#define VALID_TIME 31536000*2
  bool validTime = (time(NULL) > VALID_TIME);

  if (e) {
    std::stringstream sLen;
    std::stringstream sNow;
    int total = e->Duration();
    if (total >= 120) {
       sLen << total / 60 << " " << tr("mins");
    } else {
       sLen << total / 60 << " " << tr("min");
    }
    int now = (time(NULL) - e->StartTime());
    if ((now < total) && ((now / 60) > 0)) {
      switch (ReelConfig.showRemaining) {
        case 0:
          sNow << "+" << (int)(now / 60) << "'";
          break;
        case 1:
          sNow << "-" << (int)ceil((total - now) / 60.0) << "'";
          break;
        case 2:
          sNow << lrint((ceil((float)now) / total * 100.0)) << "%";
          break;
        default:
          error("Invalid value for ShowRemaining: %d", ReelConfig.showRemaining);
      }
    }

    int xDurationLeft = xEventNowRight - Gap - std::max(pFontTitle->Width(sLen.str().c_str()), pFontSubtitle->Width(sNow.str().c_str()));
    int xDurationWidth = xEventNowRight - Gap - xDurationLeft;
    int xTextLeft = xTimeLeft + xTimeWidth + BigGap;
    int xTextWidth = xDurationLeft - xTextLeft - BigGap;

    /* draw start time */
    osd->DrawText(xTimeLeft, yEventNowTop, e->GetTimeString(), Theme.Color(clrMenuTxtFg), clrTransparent, pFontTitle, xTimeWidth);
    /* draw title */
    osd->DrawText(xTextLeft, yEventNowTop, e->Title(), Theme.Color(clrMenuTxtFg), clrTransparent, pFontTitle, xTextWidth);

    /* draw duration */
    osd->DrawText(xDurationLeft, yEventNowTop, sLen.str().c_str(), Theme.Color(clrMenuTxtFg), clrTransparent,
                  pFontTitle, xDurationWidth, lineHeightTitle, taRight);
    osd->DrawText(xTextLeft, yEventNowTop + lineHeightTitle, e->ShortText(),
                    Theme.Color(clrMenuItemNotSelectableFg), clrTransparent, pFontSubtitle, xTextWidth);

    /* draw duration */
    if (validTime && (now < total) && ((now / 60) > 0)) {
      osd->DrawText(xDurationLeft, yEventNowTop + lineHeightTitle, sNow.str().c_str(), Theme.Color(clrMenuItemNotSelectableFg),
                    clrTransparent, pFontSubtitle, xDurationWidth, lineHeightSubtitle, taRight);
    }

    /* draw timebar */
    int xBarLen = pFontTitle->Width("0000");
    int xBarLeft  = xTimeLeft + SmallGap;
    int yBarThickness = Gap;
    int yBarTop = yEventNowTop + lineHeightTitle + Gap;//*1.5- yBarThickness/2; // center it in the center :)
    int x = 0;

    if (total)
       x = int(xBarLeft + xBarLen *(float)(now)/total - TinyGap);

    x = std::min(x, xBarLeft + xBarLen);

    /* draw progressbar */
    osd->DrawRectangle(xBarLeft, yBarTop , xBarLeft+xBarLen, yBarTop+ yBarThickness, Theme.Color(clrProgressBarBg));//0x00222220);
    osd->DrawRectangle(xBarLeft, yBarTop , x, yBarTop+ yBarThickness, Theme.Color(clrProgressBarFg));
  }

  e = Following;                // Next event
  if (e) {
    std::stringstream sLen;
    std::stringstream sNext;

    if ((int)(e->Duration()/60) > 1) {
       sLen << e->Duration() / 60 << " " << tr("mins");
    } else {
       sLen << e->Duration() / 60 << " " << tr("min");
    }
    if (validTime)
        sNext << "-" << (int)ceil((e->StartTime() - time(NULL)) / 60.0) << "'";

    int xDurationLeft = xEventNowRight - Gap - pFontTitle->Width(sLen.str().c_str());
    int xDurationWidth = xEventNowRight - Gap - xDurationLeft;
    int xTextLeft = xTimeLeft + xTimeWidth + BigGap;
    int xTextWidth = xDurationLeft - xTextLeft - BigGap;

    /* draw start time */
    osd->DrawText(xTimeLeft, yEventNextTop, e->GetTimeString(), Theme.Color(clrMenuTxtFg), clrTransparent, pFontTitle, xTimeWidth);
    /* draw title */
    osd->DrawText(xTextLeft, yEventNextTop, e->Title(), Theme.Color(clrMenuTxtFg), clrTransparent, pFontTitle, xTextWidth);
    /* draw duration */
    osd->DrawText(xDurationLeft, yEventNextTop, sLen.str().c_str(), Theme.Color(clrMenuTxtFg), clrTransparent,
                  pFontTitle, xDurationWidth, lineHeightTitle, taRight);
    /* draw remaining time */
    if (validTime)
        osd->DrawText(xDurationLeft, yEventNextTop + lineHeightTitle, sNext.str().c_str(), Theme.Color(clrMenuItemNotSelectableFg),
                    clrTransparent, pFontSubtitle, xDurationWidth, lineHeightSubtitle, taRight);
    /* draw shorttext */
    osd->DrawText(xTextLeft, yEventNextTop + lineHeightTitle, e->ShortText(),
                  Theme.Color(clrMenuItemNotSelectableFg), clrTransparent, pFontSubtitle, xTextWidth);
  }

}

void cSkinReelDisplayChannel::SetMessage(eMessageType Type, const char *Text) {
  tColor  colorfg = Theme.Color(clrTitleFg);
  int imgMessageIcon;

  static const int x0 = xBottomLeft;
  static const int x1 = x0 + Roundness;
  static const int x2 = xBottomRight - Roundness;

  static const int y0 = yBottomTop;
  static const int y1 = y0 + IMGFOOTERHEIGHT; // height of footer

  pFontMessage = ReelConfig.GetFont(FONT_MESSAGE);

  switch(Type) {
    case mtStatus: imgMessageIcon = imgIconMessageInfo; break;
    case mtWarning: imgMessageIcon = imgIconMessageWarning; break;
    case mtError: imgMessageIcon = imgIconMessageError; break;
    default: imgMessageIcon = imgIconMessageNeutral; break;
  }
  colorfg = 0xFFFFFFFF;

  if (Text) {
   isMessage = true; // prevents signal and date being drawn in Flush()
   strLastDate = NULL;

   // Draw Footer (only the centeral image is cleared since we donot mess with the rounded-borders)
   //osd->DrawImage(imgMenuFooterLeft, xBottomLeft-10, yBottomTop, false);
   osd->DrawImage(imgMenuFooterCenterX, xBottomLeft + Roundness, yBottomTop, false, xBottomRight-xBottomLeft - 2 * Roundness,1);
   //osd->DrawImage(imgMenuFooterRight, xBottomRight - Roundness , yBottomTop, false);


    /* Center icon */
    osd->DrawImage(imgMessageIcon, x1,y0 + (y1 - y0)/2 - 17, true);
   //osd->DrawImage(imgMessageIcon, x1,y0, true);

   #define messageIconW 30
   //display message
   
    int center_x_offset = (x2 - x1 - messageIconW)/2 - pFontMessage->Width(Text)/2;

    char text_to_draw[512];
    strncpy(text_to_draw, Text, 510); text_to_draw[511] = 0;

    if (center_x_offset <= 0) {
        center_x_offset = 0;
        CutStrToPixelLength(text_to_draw, pFontMessage, x2 - x1 - messageIconW );
    }
    osd->DrawText(x1 + messageIconW + center_x_offset , y0 + (y1 - y0)/2 - pFontMessage->Height()/2 - 5, text_to_draw, colorfg, clrTransparent, pFontMessage);
    /** The above computation is needed because the following command does not display the text! */
    //osd->DrawText(x1 + messageIconW, y0 + (y1 - y0)/2 - pFontMessage->Height()/2, Text, colorfg, clrTransparent, pFontMessage, x2 - x1 - messageIconW , y1 - y0, taCenter);

  } else {
    if(isMessage == true) { // Clean the  Footer
        /*(only the centeral image is cleared since we donot mess with the rounded-borders)*/
        //osd->DrawImage(imgMenuFooterLeft, xBottomLeft-10, yBottomTop, false);
        osd->DrawImage(imgMenuFooterCenterX, xBottomLeft + Roundness, yBottomTop, false, xBottomRight-xBottomLeft - 2 * Roundness,1);
        //osd->DrawImage(imgMenuFooterRight, xBottomRight - Roundness , yBottomTop, false);
    }
    isMessage = false;
  }
}

#define FRONTEND_DEVICE "/dev/dvb/adapter%d/frontend%d"

#ifndef DEVICE_ATTRIBUTES
void GetSignal(int &str, int &snr, fe_status_t &status) {
    str = 0;
    snr = 0;
    int const cardIndex = cDevice::ActualDevice()->CardIndex();
    static char dev[256];

    snprintf(dev, sizeof(dev), FRONTEND_DEVICE, cardIndex, 0);
    int fe = open(dev, O_RDONLY | O_NONBLOCK);
    if (fe < 0)
        return;
    ::ioctl(fe, FE_READ_SIGNAL_STRENGTH, &str);
    ::ioctl(fe, FE_READ_SNR, &snr);
#ifdef ROTORTUNER_INFO
    // TB: fe_status is only needed when using rotor-info
    ::ioctl(fe, FE_READ_STATUS, &status);
#endif
    close(fe);
}
#else
void GetSignal(int &str, int &snr, fe_status_t &status) {
    str = 0;
    snr = 0;
    cDevice *cdev = cDevice::ActualDevice();
    if (cdev) {
        uint64_t v=0;
#ifdef ROTORTUNER_INFO
        cdev->GetAttribute("fe.status",&v);
        status=(fe_status_t)v;
#endif
        cdev->GetAttribute("fe.signal",&v);
        str=v&0xFFFF;
        cdev->GetAttribute("fe.snr",&v);
        snr=v&0xFFFF;
    }
}
#endif

void cSkinReelDisplayChannel::UpdateSignal() {
 int str, snr;
 fe_status_t status;
 GetSignal(str, snr, status);

 int y = yBottomTop + (yBottomBottom - yBottomTop)/2 - 5;
 xsSignalBar = xTitleLeft + Gap + Roundness ;
 
//#define ROTORTUNER_INFO 1
#ifdef ROTORTUNER_INFO
 char txt[256];

 // GA: Demo only, this needs to be done much better...
 if ((status>>12)&7) { // !=0 -> special netceiver status delivered
   int rotorstat=(status>>8)&7;  // 0: rotor not moving, 1: move to cached position, 2: move to unknown pos, 3: autofocus
#if OLD_ROTOR_INFO
   char *txt0="";
   char *txt1="Rotor";
   char *txt2="RotorS";
   char *txt3="RotorAF";

   sprintf(txt,"T%i %s",(status>>12)&7,rotorstat==1?txt1:(rotorstat==2?txt2:(rotorstat==3?txt3:txt0)));
   osd->DrawImage(imgChannelInfoFooterCenterX, xDateLeft-120, yBottomTop, false, 120,1);
   int w = std::min(pFontDate->Width(txt), xBottomRight - xDateLeft - Roundness );
   osd->DrawText(xDateLeft-120, yBottomTop + (yBottomBottom - yBottomTop)/2 - pFontDate->Height()/2, txt, Theme.Color(clrTitleFg), 
    clrTransparent, pFontDate, w, yBottomBottom - yBottomTop);
#endif
   osd->DrawImage(imgMenuFooterCenterX, xsSignalBar, yBottomTop, false, 120,1);

   //printf("XXXXXX: ((status>>8)&7): %i ((status>>12)&7): %i\n", rotorstat, (status>>12)&7);
   switch (rotorstat) {
      case 2:
          DrawUnbufferedImage(osd, "signal_rotate.png", xsSignalBar, y - CHANNELINFOSYMBOLHEIGHT/2, true);
          break;
      case 3:
          DrawUnbufferedImage(osd, "signal_focus.png", xsSignalBar, y - CHANNELINFOSYMBOLHEIGHT/2, true);
          break;
      default:
          DrawImage(imgSignal, xsSignalBar, y - CHANNELINFOSYMBOLHEIGHT/2, true);
          break;
   }
 }
#else
 osd->DrawImage(imgSignal, xsSignalBar, y - CHANNELINFOSYMBOLHEIGHT/2, true ); //
#endif
 xsSignalBar += 72 + SmallGap;  // 72 is imgSignal length

 int bw = 45;
 xeSignalBar = xsSignalBar + bw;

 str = str * bw / 0xFFFF;
 snr = snr * bw / 0xFFFF;

 if (1 || str != strBarWidth || snr != snrBarWidth) //TODO: remove 1 and check why is signalbar missing sometimes
   {
    strBarWidth = str;
    snrBarWidth = snr;

    int h = 4;
    int yStr = int(y - 1.75*h);
    int ySnr = int(y + h*.75);

    // Draw Background
    osd->DrawRectangle(xsSignalBar, yStr, xeSignalBar - 1, yStr + h , Theme.Color(clrSignalBg));
    osd->DrawRectangle(xsSignalBar, ySnr, xeSignalBar - 1, ySnr + h , Theme.Color(clrSignalBg));

    // Draw Foreground

    int signalFgColor = Theme.Color(clrSignalHighFg);
    
    // medium signal : ORANGE
    if (str < 0.6*bw && str > 0.5 *bw) 
        signalFgColor = Theme.Color(clrSignalMediumFg);
    else if (str>=0 && str <= 0.5 * bw)
        signalFgColor = Theme.Color(clrSignalLowFg);

    if (str)
        osd->DrawRectangle(xsSignalBar, yStr , xsSignalBar + str - 1, yStr + h , signalFgColor);
    
    signalFgColor = Theme.Color(clrSignalHighFg);
    
    // medium signal : ORANGE
    if (snr < 0.6*bw && snr > 0.5 *bw) 
        signalFgColor = Theme.Color(clrSignalMediumFg);
    // low signal : RED
    else if (snr>=0 && snr <= 0.5 * bw)
        signalFgColor = Theme.Color(clrSignalLowFg);

    if (snr)
        osd->DrawRectangle(xsSignalBar, ySnr , xsSignalBar + snr - 1, ySnr + h , signalFgColor);
   }// if 1
}

void cSkinReelDisplayChannel::UpdateDateTime(void) {
  cString date = GetFullDateTime();
  if ((strLastDate == NULL) || strcmp(strLastDate, (const char*)date) != 0) {
    free(strLastDate);
    strLastDate = strdup((const char*)date);
    // update date string
    int w = std::min(pFontDate->Width(date), xBottomRight - xDateLeft - Roundness );
    // clear the Date by drawing the footer
    osd->DrawImage(imgMenuFooterCenterX, xDateLeft, yBottomTop, false, xBottomRight - xDateLeft - Roundness,1);
    osd->DrawImage(imgMenuFooterRight, xBottomRight - Roundness , yBottomTop, false);

    osd->DrawText(xDateLeft, yBottomTop - 5 + (yBottomBottom - yBottomTop)/2 - pFontDate->Height()/2, date, Theme.Color(clrTitleFg), clrTransparent, pFontDate, w, yBottomBottom - yBottomTop-10);
  }
}

#ifdef REELVDR
void cSkinReelDisplayChannel::UpdateAspect() {
  enum { aspectnone, aspect43, aspect169 }; /* Aspect Ratio */
  static int ys = yTitleTop + (yTitleBottom - yTitleTop - SymbolHeight) / 2 + 5;

  /* clear */
  osd->DrawImage(imgMenuHeaderCenterX, xSymbol, yTitleTop, false, CHANNELINFOSYMBOLWIDTH, 1);

#if VDRVERSNUM < 10716
  lastAspect = cDevice::PrimaryDevice()->GetAspectRatio();

  switch(lastAspect) {
    case aspect43:
      osd->DrawImage(img43, xSymbol, ys, true);
      break;
    case aspect169:
      osd->DrawImage(img169, xSymbol, ys, true);
      break;
    case aspectnone:
    default:
      break;
  }
#else
  int h,w;
  cDevice::PrimaryDevice()->GetVideoSize(w,h,lastAspect);
  if(4.0/3.0 == lastAspect)
      osd->DrawImage(img43, xSymbol, ys, true);
  else if(16.0/9.0 == lastAspect)
      osd->DrawImage(img169, xSymbol, ys, true);
#endif

}
#endif

void cSkinReelDisplayChannel::Flush(void) {
  if(!isMessage) {
    UpdateDateTime();
    UpdateSignal();
#ifdef REELVDR
#if VDRVERSNUM < 10716
    if ((xSymbol != 0) && (cDevice::PrimaryDevice()->GetAspectRatio() != lastAspect))
      UpdateAspect();
#else
    int h,w;
    double a;
    cDevice::PrimaryDevice()->GetVideoSize(w,h,a);
    if ((xSymbol != 0) && (a != lastAspect))
      UpdateAspect();
#endif
#endif
  }
  osd->Flush();

}

