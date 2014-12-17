/*
 * Frontend Status Monitor plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <ctype.h>
#include "femoncfg.h"
#include "femonreceiver.h"
#include "femontools.h"
#include "femonosd.h"
#include "vdr/s2reel_compat.h"
#include "vdr/config.h"

#include "symbols/device.xpm"
#include "symbols/stereo.xpm"
#include "symbols/monoleft.xpm"
#include "symbols/monoright.xpm"
#include "symbols/zero.xpm"
#include "symbols/one.xpm"
#include "symbols/two.xpm"
#include "symbols/three.xpm"
#include "symbols/four.xpm"
#include "symbols/five.xpm"
#include "symbols/svdrp.xpm"
#include "symbols/ar11.xpm"
#include "symbols/ar169.xpm"
#include "symbols/ar2211.xpm"
#include "symbols/ar43.xpm"
#include "symbols/ntsc.xpm"
#include "symbols/pal.xpm"
#include "symbols/dolbydigital.xpm"
#include "symbols/dolbydigital20.xpm"
#include "symbols/dolbydigital51.xpm"
#include "symbols/lock.xpm"
#include "symbols/signal.xpm"
#include "symbols/carrier.xpm"
#include "symbols/viterbi.xpm"
#include "symbols/sync.xpm"
#include "symbols/lock_red.xpm"
#include "symbols/signal_red.xpm"
#include "symbols/carrier_red.xpm"
#include "symbols/viterbi_red.xpm"
#include "symbols/sync_red.xpm"

#define CHANNELINPUT_TIMEOUT      1000

#define OSDHEIGHT                 femonConfig.osdheight   // in pixels
#define OSDWIDTH                  600                     // in pixels
#define OSDROWHEIGHT              m_Font->Height()        // in pixels
#define OSDINFOHEIGHT             (OSDROWHEIGHT * 12)     // in pixels (12 rows)
#define OSDSTATUSHEIGHT           (OSDROWHEIGHT * 6)      // in pixels (6 rows)
#define OSDSPACING                5
#define OSDCORNERING              10
#define IS_OSDCORNERING           (femonConfig.skin == eFemonSkinElchi)

#define OSDINFOWIN_Y(offset)      (femonConfig.position ? (OSDHEIGHT - OSDINFOHEIGHT + offset) : offset)
#define OSDINFOWIN_X(col)         ((col == 4) ? 455 : (col == 3) ? 305 : (col == 2) ? 155 : 15)
#define OSDSTATUSWIN_Y(offset)    (femonConfig.position ? offset : (OSDHEIGHT - OSDSTATUSHEIGHT + offset))
#define OSDSTATUSWIN_X(col)       ((col == 7) ? 475 : (col == 6) ? 410 : (col == 5) ? 275 : (col == 4) ? 220 : (col == 3) ? 125 : (col == 2) ? 70 : 15)
#define OSDSTATUSWIN_XSYMBOL(c,w) (c * ((OSDWIDTH - (5 * w)) / 6) + ((c - 1) * w))
#define OSDBARWIDTH(x)            (OSDWIDTH * x / 100)

#define SVDRPPLUGIN               "svdrpservice"

cBitmap cFemonOsd::bmDevice(device_xpm);
cBitmap cFemonOsd::bmStereo(stereo_xpm);
cBitmap cFemonOsd::bmMonoLeft(monoleft_xpm);
cBitmap cFemonOsd::bmMonoRight(monoright_xpm);
cBitmap cFemonOsd::bmNumbers[MAX_BMNUMBERS] = {
   cBitmap(zero_xpm),  cBitmap(one_xpm),  cBitmap(two_xpm),
   cBitmap(three_xpm), cBitmap(four_xpm), cBitmap(five_xpm)
};
cBitmap cFemonOsd::bmSVDRP(svdrp_xpm);
cBitmap cFemonOsd::bmAspectRatio_1_1(ar11_xpm);
cBitmap cFemonOsd::bmAspectRatio_16_9(ar169_xpm);
cBitmap cFemonOsd::bmAspectRatio_2_21_1(ar2211_xpm);
cBitmap cFemonOsd::bmAspectRatio_4_3(ar43_xpm);
cBitmap cFemonOsd::bmPAL(pal_xpm);
cBitmap cFemonOsd::bmNTSC(ntsc_xpm);
cBitmap cFemonOsd::bmDD(dolbydigital_xpm);
cBitmap cFemonOsd::bmDD20(dolbydigital20_xpm);
cBitmap cFemonOsd::bmDD51(dolbydigital51_xpm);
cBitmap cFemonOsd::bmLock(lock_xpm);
cBitmap cFemonOsd::bmSignal(signal_xpm);
cBitmap cFemonOsd::bmCarrier(carrier_xpm);
cBitmap cFemonOsd::bmViterbi(viterbi_xpm);
cBitmap cFemonOsd::bmSync(sync_xpm);
cBitmap cFemonOsd::bmLockRed(lock_red_xpm);
cBitmap cFemonOsd::bmSignalRed(signal_red_xpm);
cBitmap cFemonOsd::bmCarrierRed(carrier_red_xpm);
cBitmap cFemonOsd::bmViterbiRed(viterbi_red_xpm);
cBitmap cFemonOsd::bmSyncRed(sync_red_xpm);

cFemonOsd *cFemonOsd::pInstance = NULL;

cFemonOsd *cFemonOsd::Instance(bool create)
{
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  if (pInstance == NULL && create)
  {
     pInstance = new cFemonOsd();
  }
  return (pInstance);
}

cFemonOsd::cFemonOsd()
:cOsdObject(true), cThread("femon osd")
{
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  m_Osd = NULL;
  m_Receiver = NULL;
  m_Frontend = -1;
  m_SvdrpVideoBitrate = -1.0;
  m_SvdrpAudioBitrate = -1.0;
  m_SvdrpFrontend = -1;
  m_SvdrpConnection.handle = -1;
  m_SvdrpPlugin = NULL;
  m_Number = 0;
  m_OldNumber = 0;
  m_Signal = 0;
  m_SNR = 0;
  m_BER = 0;
  m_UNC = 0;
  m_DisplayMode = femonConfig.displaymode;
  m_InputTime.Set(0);
  m_Mutex = new cMutex();
  if (Setup.UseSmallFont == 0) {
     // Dirty hack to force the small fonts...
     Setup.UseSmallFont = 1;
     m_Font = cFont::GetFont(fontSml);
     Setup.UseSmallFont = 0;
     }
  else
     m_Font = cFont::GetFont(fontSml);
}

cFemonOsd::~cFemonOsd(void)
{
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  if (Running())
     Cancel(3);
  if (m_SvdrpConnection.handle >= 0) {
     m_SvdrpPlugin = cPluginManager::GetPlugin(SVDRPPLUGIN);
     if (m_SvdrpPlugin)
        m_SvdrpPlugin->Service("SvdrpConnection-v1.0", &m_SvdrpConnection);
     }
  if (m_Receiver)
     delete m_Receiver;
  if (m_Osd)
     delete m_Osd;
  pInstance = NULL;
}

void cFemonOsd::DrawStatusWindow(void)
{
  cMutexLock lock(m_Mutex);
  unsigned int number = 0;
  cBitmap *bm = NULL;
  int snr = m_SNR / 655;
  int signal = m_Signal / 655;
  int offset = 0;
  int x = OSDWIDTH - OSDCORNERING;
  int y = 0;
  eTrackType track = cDevice::PrimaryDevice()->GetCurrentAudioTrack();
  cChannel *channel = Channels.GetByNumber(cDevice::CurrentChannel());

  if (m_Osd) {
     m_Osd->DrawRectangle(0, OSDSTATUSWIN_Y(0), OSDWIDTH, OSDSTATUSWIN_Y(OSDSTATUSHEIGHT), femonTheme[femonConfig.theme].clrBackground);
     m_Osd->DrawRectangle(0, OSDSTATUSWIN_Y(offset), OSDWIDTH, OSDSTATUSWIN_Y(offset+OSDROWHEIGHT-1), femonTheme[femonConfig.theme].clrTitleBackground);
     m_Osd->DrawText(OSDSTATUSWIN_X(1), OSDSTATUSWIN_Y(offset), *cString::sprintf("%d%s %s", m_Number ? m_Number : channel->Number(), m_Number ? "-" : "", channel->ShortName(true)), femonTheme[femonConfig.theme].clrTitleText, femonTheme[femonConfig.theme].clrTitleBackground, m_Font);
     if (IS_OSDCORNERING) {
        m_Osd->DrawEllipse(0, OSDSTATUSWIN_Y(0), OSDCORNERING, OSDSTATUSWIN_Y(OSDCORNERING), clrTransparent, -2);
        m_Osd->DrawEllipse((OSDWIDTH-OSDCORNERING), OSDSTATUSWIN_Y(0), OSDWIDTH, OSDSTATUSWIN_Y(OSDCORNERING), clrTransparent, -1);
        }
     if (m_SvdrpFrontend >= 0) {
        x -= bmSVDRP.Width() + OSDSPACING;
        y = (OSDROWHEIGHT - bmSVDRP.Height()) / 2;
        if (y < 0) y = 0;
        m_Osd->DrawBitmap(x, OSDSTATUSWIN_Y(offset+y), bmSVDRP, femonTheme[femonConfig.theme].clrTitleText, femonTheme[femonConfig.theme].clrTitleBackground);
        }
#ifdef RB_LITE
    number = cDevice::ActualDevice()->CardIndex()+1;
    if (number < MAX_BMNUMBERS) {
       x -= bmNumbers[number].Width() + OSDSPACING;
       y = (OSDROWHEIGHT - bmNumbers[number].Height()) / 2;
       if (y < 0) y = 0;
       m_Osd->DrawBitmap(x, OSDSTATUSWIN_Y(offset+y), bmNumbers[number], femonTheme[femonConfig.theme].clrTitleText, femonTheme[femonConfig.theme].clrTitleBackground);
       x -= bmDevice.Width();
       y = (OSDROWHEIGHT - bmDevice.Height()) / 2;
       if (y < 0) y = 0;
       m_Osd->DrawBitmap(x, OSDSTATUSWIN_Y(offset+y), bmDevice, femonTheme[femonConfig.theme].clrTitleText, femonTheme[femonConfig.theme].clrTitleBackground);
       }
#else
    // Show NetCeiver tuner slot
    number = ((m_FrontendStatus >> 12))&7;
    if (number) {
	    int slot,subslot;
	    slot=1+((number-1)>>1);
	    subslot=1+((number&1)^1);
	    m_Osd->DrawText(OSDWIDTH - OSDCORNERING- (3*bmNumbers[0].Width() + OSDSPACING), OSDSTATUSWIN_Y(offset+(OSDROWHEIGHT - bmDevice.Height()) / 2), 
			    *cString::sprintf("Tuner %i.%i", slot,subslot), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
    }
    else {
	    m_Osd->DrawText(OSDWIDTH - OSDCORNERING- (3*bmNumbers[0].Width() + OSDSPACING), OSDSTATUSWIN_Y(offset+(OSDROWHEIGHT - bmDevice.Height()) / 2), 
			    "Tuner ?", femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);	    
    }
    x -= 3*bmNumbers[0].Width() + OSDSPACING;
    x -= bmDevice.Width();
#endif    


     if (IS_AUDIO_TRACK(track)) {
        number = int(track - ttAudioFirst);
        if (number < MAX_BMNUMBERS) {
           x -= bmNumbers[number].Width() + OSDSPACING;
           y = (OSDROWHEIGHT - bmNumbers[number].Height()) / 2;
           if (y < 0) y = 0;
           m_Osd->DrawBitmap(x, OSDSTATUSWIN_Y(offset+y), bmNumbers[number], femonTheme[femonConfig.theme].clrTitleText, femonTheme[femonConfig.theme].clrTitleBackground);
           }
        switch (cDevice::PrimaryDevice()->GetAudioChannel()) {
           case 1:  bm = &bmMonoLeft;  break;
           case 2:  bm = &bmMonoRight; break;
           default: bm = &bmStereo;    break;
           }
        if (bm) {
           x -= bm->Width();
           y = (OSDROWHEIGHT - bm->Height()) / 2;
           if (y < 0) y = 0;
           m_Osd->DrawBitmap(x, OSDSTATUSWIN_Y(offset+y), *bm, femonTheme[femonConfig.theme].clrTitleText, femonTheme[femonConfig.theme].clrTitleBackground);
           }
        }
     else if (m_Receiver && m_Receiver->AC3Valid() && IS_DOLBY_TRACK(track)) {
        if (m_Receiver->AC3_5_1())      bm = &bmDD51;
        else if (m_Receiver->AC3_2_0()) bm = &bmDD20;
        else                            bm = &bmDD;
        if (bm) {
           x -= bm->Width() + OSDSPACING;
           y = (OSDROWHEIGHT - bm->Height()) / 2;
           if (y < 0) y = 0;
           m_Osd->DrawBitmap(x, OSDSTATUSWIN_Y(offset+y), *bm, femonTheme[femonConfig.theme].clrTitleText, femonTheme[femonConfig.theme].clrTitleBackground);
           }
        }
     if (m_Receiver) {
        switch (m_Receiver->VideoFormat()) {
           case VF_PAL:  bm = &bmPAL;  break;
           case VF_NTSC: bm = &bmNTSC; break;
           default:      bm = NULL;    break;
           }
        if (bm) {
           x -= bm->Width() + OSDSPACING;
           y = (OSDROWHEIGHT - bm->Height()) / 2;
           if (y < 0) y = 0;
           m_Osd->DrawBitmap(x, OSDSTATUSWIN_Y(offset+y), *bm, femonTheme[femonConfig.theme].clrTitleText, femonTheme[femonConfig.theme].clrTitleBackground);
           }
        switch (m_Receiver->VideoAspectRatio()) {
           case AR_1_1:    bm = &bmAspectRatio_1_1;    break;
           case AR_4_3:    bm = &bmAspectRatio_4_3;    break;
           case AR_16_9:   bm = &bmAspectRatio_16_9;   break;
           case AR_2_21_1: bm = &bmAspectRatio_2_21_1; break;
           default:        bm = NULL;                  break;
           }
        if (bm) {
           x -= bm->Width() + OSDSPACING;
           y = (OSDROWHEIGHT - bm->Height()) / 2;
           if (y < 0) y = 0;
           m_Osd->DrawBitmap(x, OSDSTATUSWIN_Y(offset+y), *bm, femonTheme[femonConfig.theme].clrTitleText, femonTheme[femonConfig.theme].clrTitleBackground);
           }
        }
     offset += OSDROWHEIGHT;
     if (signal > 0) {
        signal = OSDBARWIDTH(signal);
        m_Osd->DrawRectangle(0, OSDSTATUSWIN_Y(offset+3), min(OSDBARWIDTH(femonConfig.redlimit), signal), OSDSTATUSWIN_Y(offset+OSDROWHEIGHT-3), femonTheme[femonConfig.theme].clrRed);
        if (signal > OSDBARWIDTH(femonConfig.redlimit)) {
           m_Osd->DrawRectangle(OSDBARWIDTH(femonConfig.redlimit), OSDSTATUSWIN_Y(offset+3), min((OSDWIDTH * femonConfig.greenlimit / 100), signal), OSDSTATUSWIN_Y(offset+OSDROWHEIGHT-3), femonTheme[femonConfig.theme].clrYellow);
           }
        if (signal > OSDBARWIDTH(femonConfig.greenlimit)) {
           m_Osd->DrawRectangle(OSDBARWIDTH(femonConfig.greenlimit), OSDSTATUSWIN_Y(offset+3), signal, OSDSTATUSWIN_Y(offset+OSDROWHEIGHT-3), femonTheme[femonConfig.theme].clrGreen);
           }
        }
     offset += OSDROWHEIGHT;
     if (snr > 0) {
        snr = OSDBARWIDTH(snr);
        m_Osd->DrawRectangle(0, OSDSTATUSWIN_Y(offset+3), min(OSDBARWIDTH(femonConfig.redlimit), snr), OSDSTATUSWIN_Y(offset+OSDROWHEIGHT-3), femonTheme[femonConfig.theme].clrRed);
        if (snr > OSDBARWIDTH(femonConfig.redlimit)) {
           m_Osd->DrawRectangle(OSDBARWIDTH(femonConfig.redlimit), OSDSTATUSWIN_Y(offset+3), min(OSDBARWIDTH(femonConfig.greenlimit), snr), OSDSTATUSWIN_Y(offset+OSDROWHEIGHT-3), femonTheme[femonConfig.theme].clrYellow);
           }
        if (snr > OSDBARWIDTH(femonConfig.greenlimit)) {
           m_Osd->DrawRectangle(OSDBARWIDTH(femonConfig.greenlimit),  OSDSTATUSWIN_Y(offset+3), snr, OSDSTATUSWIN_Y(offset+OSDROWHEIGHT-3), femonTheme[femonConfig.theme].clrGreen);
           }
        }
     offset += OSDROWHEIGHT;
     m_Osd->DrawText(OSDSTATUSWIN_X(1), OSDSTATUSWIN_Y(offset), "STR:", femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
     m_Osd->DrawText(OSDSTATUSWIN_X(2), OSDSTATUSWIN_Y(offset), *cString::sprintf("%04x", m_Signal), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
     m_Osd->DrawText(OSDSTATUSWIN_X(3), OSDSTATUSWIN_Y(offset), *cString::sprintf("(%2d%%)", m_Signal / 655), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
     m_Osd->DrawText(OSDSTATUSWIN_X(4), OSDSTATUSWIN_Y(offset), "BER:", femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
     m_Osd->DrawText(OSDSTATUSWIN_X(5), OSDSTATUSWIN_Y(offset), *cString::sprintf("%08x", m_BER), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
     m_Osd->DrawText(OSDSTATUSWIN_X(6), OSDSTATUSWIN_Y(offset), *cString::sprintf("%s:", tr("Video")), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
     m_Osd->DrawText(OSDSTATUSWIN_X(7), OSDSTATUSWIN_Y(offset), *getBitrateMbits(m_Receiver ? m_Receiver->VideoBitrate() : (m_SvdrpFrontend >= 0 ? m_SvdrpVideoBitrate : -1.0)), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
     offset += OSDROWHEIGHT;
     m_Osd->DrawText(OSDSTATUSWIN_X(1), OSDSTATUSWIN_Y(offset), "SNR:", femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
     m_Osd->DrawText(OSDSTATUSWIN_X(2), OSDSTATUSWIN_Y(offset), *cString::sprintf("%04x", m_SNR), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
     m_Osd->DrawText(OSDSTATUSWIN_X(3), OSDSTATUSWIN_Y(offset), *cString::sprintf("(%2d%%)", m_SNR / 655), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
     m_Osd->DrawText(OSDSTATUSWIN_X(4), OSDSTATUSWIN_Y(offset), "UNC:", femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
     m_Osd->DrawText(OSDSTATUSWIN_X(5), OSDSTATUSWIN_Y(offset), *cString::sprintf("%08x", m_UNC), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
     m_Osd->DrawText(OSDSTATUSWIN_X(6), OSDSTATUSWIN_Y(offset), *cString::sprintf("%s:", (m_Receiver && m_Receiver->AC3Valid() && IS_DOLBY_TRACK(track)) ? tr("AC-3") : tr("Audio")), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
     m_Osd->DrawText(OSDSTATUSWIN_X(7), OSDSTATUSWIN_Y(offset), *getBitrateKbits(m_Receiver ? ((m_Receiver->AC3Valid() && IS_DOLBY_TRACK(track)) ? m_Receiver->AC3Bitrate() : m_Receiver->AudioBitrate()) : (m_SvdrpFrontend >= 0 ? m_SvdrpAudioBitrate : -1.0)), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
     offset += OSDROWHEIGHT;
     x = bmLock.Width();
     y = (OSDROWHEIGHT - bmLock.Height()) / 2;
     m_Osd->DrawBitmap(OSDSTATUSWIN_XSYMBOL(1, x), OSDSTATUSWIN_Y(offset + y), (m_FrontendStatus & FE_HAS_LOCK) ? bmLock : bmLockRed, (m_FrontendStatus & FE_HAS_LOCK) ? femonTheme[femonConfig.theme].clrActiveText : femonTheme[femonConfig.theme].clrRed, femonTheme[femonConfig.theme].clrBackground);
     m_Osd->DrawBitmap(OSDSTATUSWIN_XSYMBOL(2, x), OSDSTATUSWIN_Y(offset + y), (m_FrontendStatus & FE_HAS_SIGNAL) ? bmSignal : bmSignalRed, (m_FrontendStatus & FE_HAS_SIGNAL) ? femonTheme[femonConfig.theme].clrActiveText : femonTheme[femonConfig.theme].clrRed, femonTheme[femonConfig.theme].clrBackground);
     m_Osd->DrawBitmap(OSDSTATUSWIN_XSYMBOL(3, x), OSDSTATUSWIN_Y(offset + y), (m_FrontendStatus & FE_HAS_CARRIER) ? bmCarrier : bmCarrierRed, (m_FrontendStatus & FE_HAS_CARRIER) ? femonTheme[femonConfig.theme].clrActiveText : femonTheme[femonConfig.theme].clrRed, femonTheme[femonConfig.theme].clrBackground);
     m_Osd->DrawBitmap(OSDSTATUSWIN_XSYMBOL(4, x), OSDSTATUSWIN_Y(offset + y), (m_FrontendStatus & FE_HAS_VITERBI) ? bmViterbi : bmViterbiRed, (m_FrontendStatus & FE_HAS_VITERBI) ? femonTheme[femonConfig.theme].clrActiveText : femonTheme[femonConfig.theme].clrRed, femonTheme[femonConfig.theme].clrBackground);
     m_Osd->DrawBitmap(OSDSTATUSWIN_XSYMBOL(5, x), OSDSTATUSWIN_Y(offset + y), (m_FrontendStatus & FE_HAS_SYNC) ? bmSync : bmSyncRed, (m_FrontendStatus & FE_HAS_SYNC) ? femonTheme[femonConfig.theme].clrActiveText : femonTheme[femonConfig.theme].clrRed, femonTheme[femonConfig.theme].clrBackground);
     if (IS_OSDCORNERING) {
        m_Osd->DrawEllipse(0, OSDSTATUSWIN_Y(OSDSTATUSHEIGHT-OSDCORNERING), OSDCORNERING, OSDSTATUSWIN_Y(OSDSTATUSHEIGHT), clrTransparent, -3);
        m_Osd->DrawEllipse((OSDWIDTH-OSDCORNERING), OSDSTATUSWIN_Y(OSDSTATUSHEIGHT-OSDCORNERING), OSDWIDTH, OSDSTATUSWIN_Y(OSDSTATUSHEIGHT), clrTransparent, -4);
        }
     m_Osd->Flush();
     }
}

void cFemonOsd::DrawInfoWindow(void)
{
  cMutexLock lock(m_Mutex);
  int offset = 0;
  cChannel *channel = Channels.GetByNumber(cDevice::CurrentChannel());
  eTrackType track = cDevice::PrimaryDevice()->GetCurrentAudioTrack();

  if (m_Osd) {
#if APIVERSNUM >= 10716
     cDvbTransponderParameters dtp(channel->Parameters());
#endif
     if (m_DisplayMode == eFemonModeTransponder) {
        m_Osd->DrawRectangle(0, OSDINFOWIN_Y(0), OSDWIDTH, OSDINFOWIN_Y(OSDINFOHEIGHT), femonTheme[femonConfig.theme].clrBackground);
        m_Osd->DrawRectangle(0, OSDINFOWIN_Y(offset), OSDWIDTH, OSDINFOWIN_Y(offset+OSDROWHEIGHT-1), femonTheme[femonConfig.theme].clrTitleBackground);
        m_Osd->DrawText( OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Transponder Information"), femonTheme[femonConfig.theme].clrTitleText, femonTheme[femonConfig.theme].clrTitleBackground, m_Font);
        if (IS_OSDCORNERING) {
           m_Osd->DrawEllipse(0, OSDINFOWIN_Y(0), OSDCORNERING, OSDINFOWIN_Y(OSDCORNERING), clrTransparent, -2);
           m_Osd->DrawEllipse((OSDWIDTH-OSDCORNERING), OSDINFOWIN_Y(0), OSDWIDTH, OSDINFOWIN_Y(OSDCORNERING), clrTransparent, -1);
           }
        offset += OSDROWHEIGHT;
        m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Vpid"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *cString::sprintf("%d", channel->Vpid()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), tr("Ppid"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *cString::sprintf("%d", channel->Ppid()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        offset += OSDROWHEIGHT;
        m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Apid"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *getApids(channel), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), tr("Dpid"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *getDpids(channel), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        offset += OSDROWHEIGHT;
        m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Tpid"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *cString::sprintf("%d", channel->Tpid()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        offset += OSDROWHEIGHT;
        m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("CA"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *getCAids(channel, femonConfig.showcasystem), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        offset += OSDROWHEIGHT;
        m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Sid"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *cString::sprintf("%d", channel->Sid()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), tr("Nid"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *cString::sprintf("%d", channel->Nid()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        offset += OSDROWHEIGHT;
        m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Tid"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *cString::sprintf("%d", channel->Tid()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), tr("Rid"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *cString::sprintf("%d", channel->Rid()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        offset += OSDROWHEIGHT;
        switch (m_FrontendInfo.type) {
          case FE_DVBS2:
          case FE_QPSK:
               m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), *cString::sprintf("%s #%d - %s", tr("Satellite Card"), (m_SvdrpFrontend >= 0) ? m_SvdrpFrontend : cDevice::ActualDevice()->CardIndex()+1, m_FrontendInfo.name), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
               offset += OSDROWHEIGHT;
               m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Frequency"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
               m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *getFrequencyMHz(channel->Frequency()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
               m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), tr("Source"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
               m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *cSource::ToString(channel->Source()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
               offset += OSDROWHEIGHT;
               m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Srate"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
               m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *cString::sprintf("%d", channel->Srate()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
               m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), tr("Polarization"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#if APIVERSNUM>=10716
               m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *cString::sprintf("%c", toupper(dtp.Polarization())), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#else
               m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *cString::sprintf("%c", toupper(channel->Polarization())), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#endif
               offset += OSDROWHEIGHT;
               m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Inversion"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#if APIVERSNUM>=10716
               m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *getInversion(dtp.Inversion()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#else
               m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *getInversion(channel->Inversion()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#endif
               m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), tr("CoderateH"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#if APIVERSNUM>=10716
               m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *getCoderate(dtp.CoderateH()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#else
               m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *getCoderate(channel->CoderateH()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#endif
               break;

          case FE_QAM:
               m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), *cString::sprintf("%s #%d - %s", tr("Cable Card"), (m_SvdrpFrontend >= 0) ? m_SvdrpFrontend : cDevice::ActualDevice()->CardIndex()+1, m_FrontendInfo.name), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
               offset += OSDROWHEIGHT;
               m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Frequency"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
               m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *getFrequencyMHz(channel->Frequency()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
               m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), tr("Source"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
               m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *cSource::ToString(channel->Source()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
               offset += OSDROWHEIGHT;
               m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Srate"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
               m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *cString::sprintf("%d", channel->Srate()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
               m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), tr("Modulation"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#if APIVERSNUM>=10716
               m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *getModulation(dtp.Modulation()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#else
               m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *getModulation(channel->Modulation()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#endif
               offset += OSDROWHEIGHT;
               m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Inversion"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#if APIVERSNUM>=10716
               m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *getInversion(dtp.Inversion()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#else
               m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *getInversion(channel->Inversion()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#endif
               m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), tr("CoderateH"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#if APIVERSNUM>=10716
               m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *getCoderate(dtp.CoderateH()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#else
               m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *getCoderate(channel->CoderateH()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#endif
               break;

          case FE_OFDM:
               m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), *cString::sprintf("%s #%d - %s", tr("Terrestrial Card"), (m_SvdrpFrontend >= 0) ? m_SvdrpFrontend : cDevice::ActualDevice()->CardIndex()+1, m_FrontendInfo.name), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
               offset += OSDROWHEIGHT;
               m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Frequency"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
               m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *getFrequencyMHz(channel->Frequency()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
               m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), tr("Transmission"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#if APIVERSNUM>=10716
               m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *getTransmission(dtp.Transmission()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#else
               m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *getTransmission(channel->Transmission()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#endif
               offset += OSDROWHEIGHT;
               m_Osd->DrawText( OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Bandwidth"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#if APIVERSNUM>=10716
               m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *getBandwidth(dtp.Bandwidth()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#else
               m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *getBandwidth(channel->Bandwidth()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#endif
               m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), tr("Modulation"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#if APIVERSNUM>=10716
               m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *getModulation(dtp.Modulation()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#else
               m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *getModulation(channel->Modulation()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#endif
               offset += OSDROWHEIGHT;
               m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Inversion"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#if APIVERSNUM>=10716
               m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *getInversion(dtp.Inversion()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#else
               m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *getInversion(channel->Inversion()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#endif
               m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), tr("Coderate"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#if APIVERSNUM>=10716
               m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *cString::sprintf("%s (H) %s (L)", *getCoderate(dtp.CoderateH()), *getCoderate(dtp.CoderateL())), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#else
               m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *cString::sprintf("%s (H) %s (L)", *getCoderate(channel->CoderateH()), *getCoderate(channel->CoderateL())), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#endif
               offset += OSDROWHEIGHT;
               m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Hierarchy"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#if APIVERSNUM>=10716
               m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *getHierarchy(dtp.Hierarchy()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#else
               m_Osd->DrawText(OSDINFOWIN_X(2), OSDINFOWIN_Y(offset), *getHierarchy(channel->Hierarchy()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#endif
               m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), tr("Guard"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#if APIVERSNUM>=10716
               m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *getGuard(dtp.Guard()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#else
               m_Osd->DrawText(OSDINFOWIN_X(4), OSDINFOWIN_Y(offset), *getGuard(channel->Guard()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
#endif
               break;

          default:
               break;
          }
        if (IS_OSDCORNERING) {
           m_Osd->DrawEllipse(0, OSDINFOWIN_Y(OSDINFOHEIGHT-OSDCORNERING), OSDCORNERING, OSDINFOWIN_Y(OSDINFOHEIGHT), clrTransparent, -3);
           m_Osd->DrawEllipse((OSDWIDTH-OSDCORNERING), OSDINFOWIN_Y(OSDINFOHEIGHT-OSDCORNERING), OSDWIDTH, OSDINFOWIN_Y(OSDINFOHEIGHT), clrTransparent, -4);
           }
        }
     else if (m_DisplayMode == eFemonModeStream) {
        m_Osd->DrawRectangle(0, OSDINFOWIN_Y(0), OSDWIDTH, OSDINFOWIN_Y(OSDINFOHEIGHT), femonTheme[femonConfig.theme].clrBackground);
        m_Osd->DrawRectangle(0, OSDINFOWIN_Y(offset), OSDWIDTH, OSDINFOWIN_Y(offset+OSDROWHEIGHT-1), femonTheme[femonConfig.theme].clrTitleBackground);
        m_Osd->DrawText( OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Stream Information"), femonTheme[femonConfig.theme].clrTitleText, femonTheme[femonConfig.theme].clrTitleBackground, m_Font);
        if (IS_OSDCORNERING) {
           m_Osd->DrawEllipse(0, OSDINFOWIN_Y(0), OSDCORNERING, OSDINFOWIN_Y(OSDCORNERING), clrTransparent, -2);
           m_Osd->DrawEllipse((OSDWIDTH-OSDCORNERING), OSDINFOWIN_Y(0), OSDWIDTH, OSDINFOWIN_Y(OSDCORNERING), clrTransparent, -1);
           }
        offset += OSDROWHEIGHT;
        m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Video Stream"), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), *cString::sprintf("#%d", channel->Vpid()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        offset += OSDROWHEIGHT;
        m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Bitrate"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), m_Receiver ? *cString::sprintf("%s (%s)", *getBitrateMbits(m_Receiver->VideoStreamBitrate()), *getBitrateMbits(m_Receiver->VideoBitrate())) : *cString::sprintf("---"), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        offset += OSDROWHEIGHT;
        m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Aspect Ratio"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), *getAspectRatio(m_Receiver ? m_Receiver->VideoAspectRatio() : -1), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        offset += OSDROWHEIGHT;
        m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Frame Rate"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), m_Receiver ? *cString::sprintf("%.2f %s", m_Receiver->VideoFrameRate(), tr("Hz")) : *cString::sprintf("---"), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        offset += OSDROWHEIGHT;
        m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Video Format"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), *getVideoFormat(m_Receiver ? m_Receiver->VideoFormat() : -1), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        offset += OSDROWHEIGHT;
        m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Resolution"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), m_Receiver ? *cString::sprintf("%d x %d", m_Receiver->VideoHorizontalSize(), m_Receiver->VideoVerticalSize()) : *cString::sprintf("---"), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        offset += OSDROWHEIGHT;
        m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Audio Stream"), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), *cString::sprintf("#%d %s", IS_AUDIO_TRACK(track) ? channel->Apid(int(track - ttAudioFirst)) : channel->Apid(0), IS_AUDIO_TRACK(track) ? channel->Alang(int(track - ttAudioFirst)) : channel->Alang(0)), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        offset += OSDROWHEIGHT;
        m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Bitrate"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), *getAudioBitrate(m_Receiver ? m_Receiver->AudioBitrate() : (double)FR_NOTVALID, m_Receiver ? m_Receiver->AudioStreamBitrate() : (double)FR_NOTVALID), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        offset += OSDROWHEIGHT;
        m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("MPEG Layer"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), m_Receiver ? *cString::sprintf("%d", m_Receiver->AudioMPEGLayer()) : *cString::sprintf("---"), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        offset += OSDROWHEIGHT;
        m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Sampling Frequency"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), *getAudioSamplingFreq(m_Receiver ? m_Receiver->AudioSamplingFreq() : FR_NOTVALID), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
        if (IS_OSDCORNERING) {
           m_Osd->DrawEllipse(0, OSDINFOWIN_Y(OSDINFOHEIGHT-OSDCORNERING), OSDCORNERING, OSDINFOWIN_Y(OSDINFOHEIGHT), clrTransparent, -3);
           m_Osd->DrawEllipse((OSDWIDTH-OSDCORNERING), OSDINFOWIN_Y(OSDINFOHEIGHT-OSDCORNERING), OSDWIDTH, OSDINFOWIN_Y(OSDINFOHEIGHT), clrTransparent, -4);
           }
        }
     else if (m_DisplayMode == eFemonModeAC3) {
        m_Osd->DrawRectangle(0, OSDINFOWIN_Y(0), OSDWIDTH, OSDINFOWIN_Y(OSDINFOHEIGHT), femonTheme[femonConfig.theme].clrBackground);
        m_Osd->DrawRectangle(0, OSDINFOWIN_Y(offset), OSDWIDTH, OSDINFOWIN_Y(offset+OSDROWHEIGHT-1), femonTheme[femonConfig.theme].clrTitleBackground);
        m_Osd->DrawText( OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), *cString::sprintf("%s - %s #%d %s", tr("Stream Information"), tr("AC-3 Stream"), IS_DOLBY_TRACK(track) ? channel->Dpid(int(track - ttDolbyFirst)) : channel->Dpid(0), IS_DOLBY_TRACK(track) ? channel->Dlang(int(track - ttDolbyFirst)) : channel->Dlang(0)), femonTheme[femonConfig.theme].clrTitleText, femonTheme[femonConfig.theme].clrTitleBackground, m_Font);
        if (IS_OSDCORNERING) {
           m_Osd->DrawEllipse(0, OSDINFOWIN_Y(0), OSDCORNERING, OSDINFOWIN_Y(OSDCORNERING), clrTransparent, -2);
           m_Osd->DrawEllipse((OSDWIDTH-OSDCORNERING), OSDINFOWIN_Y(0), OSDWIDTH, OSDINFOWIN_Y(OSDCORNERING), clrTransparent, -1);
           }
        offset += OSDROWHEIGHT;
        if (m_Receiver && m_Receiver->AC3Valid() && IS_DOLBY_TRACK(track)) {
           m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Bitrate"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), *cString::sprintf("%s (%s)", *getBitrateKbits(m_Receiver->AC3StreamBitrate()), *getBitrateKbits(m_Receiver->AC3Bitrate())), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           offset += OSDROWHEIGHT;
           m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Sampling Frequency"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), *cString::sprintf("%.1f %s", m_Receiver->AC3SamplingFreq() / 1000., tr("kHz")), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           offset += OSDROWHEIGHT;
           m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Frame Size"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), *cString::sprintf("%d", m_Receiver->AC3FrameSize()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           offset += OSDROWHEIGHT;
           m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Bit Stream Mode"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), *getAC3BitStreamMode(m_Receiver->AC3BitStreamMode(), m_Receiver->AC3AudioCodingMode()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           offset += OSDROWHEIGHT;
           m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Audio Coding Mode"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), *getAC3AudioCodingMode(m_Receiver->AC3AudioCodingMode(), m_Receiver->AC3BitStreamMode()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           offset += OSDROWHEIGHT;
           m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Center Mix Level"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), *getAC3CenterMixLevel(m_Receiver->AC3CenterMixLevel()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           offset += OSDROWHEIGHT;
           m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Surround Mix Level"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), *getAC3SurroundMixLevel(m_Receiver->AC3SurroundMixLevel()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           offset += OSDROWHEIGHT;
           m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Dolby Surround Mode"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), *getAC3DolbySurroundMode(m_Receiver->AC3DolbySurroundMode()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           offset += OSDROWHEIGHT;
           m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Low Frequency Effects"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), *cString::sprintf("%s", m_Receiver->AC3LfeOn() ? tr("On") : tr("Off")), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           offset += OSDROWHEIGHT;
           m_Osd->DrawText(OSDINFOWIN_X(1), OSDINFOWIN_Y(offset), tr("Dialogue Normalization"), femonTheme[femonConfig.theme].clrInactiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           m_Osd->DrawText(OSDINFOWIN_X(3), OSDINFOWIN_Y(offset), *getAC3DialogLevel(m_Receiver->AC3DialogLevel()), femonTheme[femonConfig.theme].clrActiveText, femonTheme[femonConfig.theme].clrBackground, m_Font);
           }
        if (IS_OSDCORNERING) {
           m_Osd->DrawEllipse(0, OSDINFOWIN_Y(OSDINFOHEIGHT-OSDCORNERING), OSDCORNERING, OSDINFOWIN_Y(OSDINFOHEIGHT), clrTransparent, -3);
           m_Osd->DrawEllipse((OSDWIDTH-OSDCORNERING), OSDINFOWIN_Y(OSDINFOHEIGHT-OSDCORNERING), OSDWIDTH, OSDINFOWIN_Y(OSDINFOHEIGHT), clrTransparent, -4);
           }
        }
     else /* eFemonModeBasic */ {
        m_Osd->DrawRectangle(0, OSDINFOWIN_Y(0), OSDWIDTH, OSDINFOWIN_Y(OSDINFOHEIGHT), clrTransparent);
        }
     m_Osd->Flush();
     }
}

void cFemonOsd::Action(void)
{
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  cTimeMs t;
  SvdrpCommand_v1_0 cmd;
  cmd.command = cString::sprintf("PLUG %s INFO\r\n", PLUGIN_NAME_I18N);
  while (Running()) {
    t.Set(0);
    m_SvdrpFrontend = -1;
    m_SvdrpVideoBitrate = -1.0;
    m_SvdrpAudioBitrate = -1.0;
#ifndef DEVICE_ATTRIBUTES
    if (m_Frontend != -1) {

       CHECK(ioctl(m_Frontend, FE_READ_STATUS, &m_FrontendStatus));
       CHECK(ioctl(m_Frontend, FE_READ_SIGNAL_STRENGTH, &m_Signal));
       CHECK(ioctl(m_Frontend, FE_READ_SNR, &m_SNR));
       CHECK(ioctl(m_Frontend, FE_READ_BER, &m_BER));
       CHECK(ioctl(m_Frontend, FE_READ_UNCORRECTED_BLOCKS, &m_UNC));
       DrawInfoWindow();
       DrawStatusWindow();
       }
    else if (m_SvdrpConnection.handle >= 0) {
       cmd.handle = m_SvdrpConnection.handle; 
       m_SvdrpPlugin->Service("SvdrpCommand-v1.0", &cmd);
       if (cmd.responseCode == 900) {
          for (cLine *line = cmd.reply.First(); line; line = cmd.reply.Next(line)) {
             const char *s = line->Text();
	      if (strncasecmp(s, "CARD:", 5) == 0)
                m_SvdrpFrontend = strtol(s + 5, NULL, 10);
             else if (strncasecmp(s, "TYPE:", 5) == 0)
                m_FrontendInfo.type = (fe_type_t) strtol(s + 5, NULL, 10);
             else if (strncasecmp(s, "NAME:", 5) == 0)
                strn0cpy(m_FrontendInfo.name, s + 5, sizeof(m_FrontendInfo.name));
             else if (strncasecmp(s, "STAT:", 5) == 0)
                m_FrontendStatus = (fe_status_t) strtol(s + 5, NULL, 16);
             else if (strncasecmp(s, "SGNL:", 5) == 0)
                m_Signal = strtol(s + 5, NULL, 16);
             else if (strncasecmp(s, "SNRA:", 5) == 0)
                m_SNR = strtol(s + 5, NULL, 16);
             else if (strncasecmp(s, "BERA:", 5) == 0)
                m_BER = strtol(s + 5, NULL, 16);
             else if (strncasecmp(s, "UNCB:", 5) == 0)
                m_UNC = strtol(s + 5, NULL, 16);
             else if (strncasecmp(s, "VIBR:", 5) == 0)
                m_SvdrpVideoBitrate = strtol(s + 5, NULL, 10);
             else if (strncasecmp(s, "AUBR:", 5) == 0)
                m_SvdrpAudioBitrate = strtol(s + 5, NULL, 10);
             }
          }
       DrawInfoWindow();
       DrawStatusWindow();
       }
#else
       cDevice *cdev=cDevice::ActualDevice();
       if (cdev) {
           uint64_t v=0;
           cdev->GetAttribute("fe.status",&v);
           m_FrontendStatus=(fe_status_t)v;
           cdev->GetAttribute("fe.signal",&v);
           m_Signal=v;
           cdev->GetAttribute("fe.snr",&v);
           m_SNR=v;
           cdev->GetAttribute("fe.ber",&v);
           m_BER=v;
           cdev->GetAttribute("fe.unc",&v);
           m_UNC=v;
           cdev->GetAttribute("fe.type",&v);
           m_FrontendInfo.type = (fe_type_t)v;
       }
       DrawInfoWindow();
       DrawStatusWindow();
#endif

    cCondWait::SleepMs(100 * femonConfig.updateinterval - t.Elapsed());
    }
}

void cFemonOsd::Show(void)
{
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  int apid[2] = {0, 0};
  int dpid[2] = {0, 0};
  eTrackType track = cDevice::PrimaryDevice()->GetCurrentAudioTrack();
#ifndef DEVICE_ATTRIBUTES
  char *dev = NULL;
  asprintf(&dev, FRONTEND_DEVICE, cDevice::ActualDevice()->CardIndex(), 0);
  m_Frontend = open(dev, O_RDONLY | O_NONBLOCK);
  free(dev);
  if (m_Frontend >= 0) {
     if (ioctl(m_Frontend, FE_GET_INFO, &m_FrontendInfo) < 0) {
        esyslog("ERROR: cFemonOsd::Show() cannot read frontend info.");
        close(m_Frontend);
        m_Frontend = -1;
        return;
        }
     }
  else if (femonConfig.usesvdrp) {
     if (!SvdrpConnect() || !SvdrpTune())
        return;
     }
  else {
     esyslog("ERROR: cFemonOsd::Show() cannot open frontend device.");
     return;
  }
#else
  
  cDevice *cdev= cDevice::ActualDevice();
  strcpy(m_FrontendInfo.name,"UNKNOWN");
  m_FrontendInfo.type=FE_QPSK;
  if (cdev) {
	  uint64_t v;
	  cdev->GetAttribute("fe.name",m_FrontendInfo.name, sizeof(m_FrontendInfo.name));
	  cdev->GetAttribute("fe.type",&v);
	  m_FrontendInfo.type=(fe_type_t)v;
  }
#endif

  m_Osd = cOsdProvider::NewTrueColorOsd(((Setup.OSDWidth - OSDWIDTH) / 2) + Setup.OSDLeft + femonConfig.osdoffset, ((Setup.OSDHeight - OSDHEIGHT) / 2) + Setup.OSDTop, false);
  if (m_Osd) {
     tArea Areas1[] = { { 0, 0, OSDWIDTH, OSDHEIGHT, femonTheme[femonConfig.theme].bpp } };
     if (m_Osd->CanHandleAreas(Areas1, sizeof(Areas1) / sizeof(tArea)) == oeOk) {
        m_Osd->SetAreas(Areas1, sizeof(Areas1) / sizeof(tArea));
        }
     else {
        tArea Areas2[] = { { 0, OSDSTATUSWIN_Y(0),          (OSDWIDTH-1), OSDSTATUSWIN_Y(OSDSTATUSHEIGHT-1), femonTheme[femonConfig.theme].bpp },
                           { 0, OSDINFOWIN_Y(0),            (OSDWIDTH-1), OSDINFOWIN_Y(OSDROWHEIGHT-1),      femonTheme[femonConfig.theme].bpp },
                           { 0, OSDINFOWIN_Y(OSDROWHEIGHT), (OSDWIDTH-1), OSDINFOWIN_Y(OSDINFOHEIGHT-1),     2 } };
        m_Osd->SetAreas(Areas2, sizeof(Areas2) / sizeof(tArea));
        }
     m_Osd->DrawRectangle(0, OSDINFOWIN_Y(0), OSDWIDTH, OSDINFOWIN_Y(OSDINFOHEIGHT), clrTransparent);
     m_Osd->Flush();
     if (m_Receiver)
        delete m_Receiver;
     if (femonConfig.analyzestream) {
        cChannel *channel = Channels.GetByNumber(cDevice::CurrentChannel());
        IS_AUDIO_TRACK(track) ? apid[0] = channel->Apid(int(track - ttAudioFirst)) : apid[0] = channel->Apid(0);
        IS_DOLBY_TRACK(track) ? dpid[0] = channel->Dpid(int(track - ttDolbyFirst)) : dpid[0] = channel->Dpid(0);
#if VDRVERSNUM < 10727
        m_Receiver = new cFemonReceiver(channel->GetChannelID(), channel->Ca(), channel->Vpid(), channel->Ppid(), apid, dpid);
#else
        m_Receiver = new cFemonReceiver(channel, channel->Ca(), channel->Vpid(), channel->Ppid(), apid, dpid);
#endif
        cDevice::ActualDevice()->AttachReceiver(m_Receiver);
        }
     Start();
     }
}

void cFemonOsd::ChannelSwitch(const cDevice * device, int channelNumber)
{
  Dprintf("%s(%d,%d)\n", __PRETTY_FUNCTION__, device->DeviceNumber(), channelNumber);
  int apid[2] = {0, 0};
  int dpid[2] = {0, 0};
  eTrackType track = cDevice::PrimaryDevice()->GetCurrentAudioTrack();
  if (!device->IsPrimaryDevice() || !channelNumber || cDevice::PrimaryDevice()->CurrentChannel() != channelNumber)
     return;
#ifndef DEVICE_ATTRIBUTES
  char *dev = NULL;
  close(m_Frontend);
  asprintf(&dev, FRONTEND_DEVICE, cDevice::ActualDevice()->CardIndex(), 0);
  m_Frontend = open(dev, O_RDONLY | O_NONBLOCK);
  free(dev);

  if (m_Frontend >= 0) {
     if (ioctl(m_Frontend, FE_GET_INFO, &m_FrontendInfo) < 0) {
        esyslog("ERROR: cFemonOsd::ChannelSwitch() cannot read frontend info.");
        close(m_Frontend);
        m_Frontend = -1;
        return;
        }
     }
  else if (femonConfig.usesvdrp) {
     if (!SvdrpConnect() || !SvdrpTune())
        return;
     }
  else {
     esyslog("ERROR: cFemonOsd::ChannelSwitch() cannot open frontend device.");
     return;
  }
#else
  {
	  cDevice *cdev= cDevice::ActualDevice();
	  strcpy(m_FrontendInfo.name,"UNKNOWN");
	  m_FrontendInfo.type=FE_QPSK;
	  if (cdev) {
		  uint64_t v;
		  cdev->GetAttribute("fe.name",m_FrontendInfo.name, sizeof(m_FrontendInfo.name));
		  cdev->GetAttribute("fe.type",&v);
		  m_FrontendInfo.type=(fe_type_t)v;
	  }
  }
#endif

  if (m_Receiver)
     delete m_Receiver;
  if (femonConfig.analyzestream) {
     cChannel *channel = Channels.GetByNumber(cDevice::CurrentChannel());
     IS_AUDIO_TRACK(track) ? apid[0] = channel->Apid(int(track - ttAudioFirst)) : apid[0] = channel->Apid(0);
     IS_DOLBY_TRACK(track) ? dpid[0] = channel->Dpid(int(track - ttDolbyFirst)) : dpid[0] = channel->Dpid(0);
#if VDRVERSNUM < 10727
     m_Receiver = new cFemonReceiver(channel->GetChannelID(), channel->Ca(), channel->Vpid(), channel->Ppid(), apid, dpid);
#else
     m_Receiver = new cFemonReceiver(channel, channel->Ca(), channel->Vpid(), channel->Ppid(), apid, dpid);
#endif
     cDevice::ActualDevice()->AttachReceiver(m_Receiver);
     }
}

void cFemonOsd::SetAudioTrack(int Index, const char * const *Tracks)
{
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  int apid[2] = {0, 0};
  int dpid[2] = {0, 0};
  eTrackType track = cDevice::PrimaryDevice()->GetCurrentAudioTrack();
  if (m_Receiver)
     delete m_Receiver;
  if (femonConfig.analyzestream) {
     cChannel *channel = Channels.GetByNumber(cDevice::CurrentChannel());
     IS_AUDIO_TRACK(track) ? apid[0] = channel->Apid(int(track - ttAudioFirst)) : apid[0] = channel->Apid(0);
     IS_DOLBY_TRACK(track) ? dpid[0] = channel->Dpid(int(track - ttDolbyFirst)) : dpid[0] = channel->Dpid(0);
#if VDRVERSNUM < 10727
     m_Receiver = new cFemonReceiver(channel->GetChannelID(), channel->Ca(), channel->Vpid(), channel->Ppid(), apid, dpid);
#else
     m_Receiver = new cFemonReceiver(channel, channel->Ca(), channel->Vpid(), channel->Ppid(), apid, dpid);
#endif
     cDevice::ActualDevice()->AttachReceiver(m_Receiver);
     }
}

bool cFemonOsd::DeviceSwitch(int direction)
{
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  int device = cDevice::ActualDevice()->DeviceNumber();
  direction = sgn(direction);
  if (device >= 0) {
     cChannel *channel = Channels.GetByNumber(cDevice::CurrentChannel());
     for (int i = 0; i < cDevice::NumDevices() - 1; i++) {
        if (direction) {
           if (++device >= cDevice::NumDevices())
              device = 0;
           }
        else {
           if (--device < 0)
              device = cDevice::NumDevices() - 1;
           }
        if (cDevice::GetDevice(device)->ProvidesChannel(channel, 0)) {
           Dprintf("%s(%d) device(%d)\n", __PRETTY_FUNCTION__, direction, device);
#if VDRVERSNUM < 10727
           cStatus::MsgChannelSwitch(cDevice::PrimaryDevice(), 0);
#else
           cStatus::MsgChannelSwitch(cDevice::PrimaryDevice(), 0, 0);
#endif
           cControl::Shutdown();
           cDevice::GetDevice(device)->SwitchChannel(channel, true);
           if (cDevice::GetDevice(device) == cDevice::PrimaryDevice())
              cDevice::GetDevice(device)->ForceTransferMode();
#if VDRVERSNUM < 10716
#if defined(APIVERSNUM) && APIVERSNUM < 10500
           cControl::Launch(new cTransferControl(cDevice::GetDevice(device), channel->Vpid(), channel->Ppid(), channel->Apids(), channel->Dpids(), channel->Spids()));
#else
           cControl::Launch(new cTransferControl(cDevice::GetDevice(device), channel->GetChannelID(), channel->Vpid(), channel->Ppid(), channel->Apids(), channel->Dpids(), channel->Spids()));
#endif
#else
           cControl::Launch(new cTransferControl(cDevice::GetDevice(device), channel));
#endif
#if VDRVERSNUM < 10727
           cStatus::MsgChannelSwitch(cDevice::PrimaryDevice(), channel->Number());
#else
           cStatus::MsgChannelSwitch(cDevice::PrimaryDevice(), channel->Number(), 0);
#endif
           return (true);
           }
        }
     }
   return (false);
}

bool cFemonOsd::SvdrpConnect(void)
{
   if (m_SvdrpConnection.handle < 0) {
      m_SvdrpPlugin = cPluginManager::GetPlugin(SVDRPPLUGIN);
      if (m_SvdrpPlugin) {
         m_SvdrpConnection.serverIp = femonConfig.svdrpip;
         m_SvdrpConnection.serverPort = femonConfig.svdrpport;
         m_SvdrpConnection.shared = true;
         m_SvdrpPlugin->Service("SvdrpConnection-v1.0", &m_SvdrpConnection);
         if (m_SvdrpConnection.handle >= 0) {
            SvdrpCommand_v1_0 cmd;
            cmd.handle = m_SvdrpConnection.handle; 
            cmd.command = cString::sprintf("PLUG %s\r\n", PLUGIN_NAME_I18N);
            m_SvdrpPlugin->Service("SvdrpCommand-v1.0", &cmd);
            if (cmd.responseCode != 214) {
               m_SvdrpPlugin->Service("SvdrpConnection-v1.0", &m_SvdrpConnection); // close connection
               esyslog("ERROR: cFemonOsd::SvdrpConnect() cannot find plugin '%s' on server %s.", PLUGIN_NAME_I18N, *m_SvdrpConnection.serverIp);
               }
            }
         else
            esyslog("ERROR: cFemonOsd::SvdrpConnect() cannot connect to SVDRP server.");
         }
      else
         esyslog("ERROR: cFemonOsd::SvdrpConnect() cannot find plugin '%s'.", SVDRPPLUGIN);
      }
   return m_SvdrpConnection.handle >= 0;
}

bool cFemonOsd::SvdrpTune(void)
{
   if (m_SvdrpPlugin && m_SvdrpConnection.handle >= 0) {
      cChannel *channel = Channels.GetByNumber(cDevice::CurrentChannel());
      if (channel) {
         SvdrpCommand_v1_0 cmd;
         cmd.handle = m_SvdrpConnection.handle; 
         cmd.command = cString::sprintf("CHAN %s\r\n", *channel->GetChannelID().ToString());
         m_SvdrpPlugin->Service("SvdrpCommand-v1.0", &cmd);
         if (cmd.responseCode == 250)
            return true;
         esyslog("ERROR: cFemonOsd::SvdrpTune() cannot tune server channel.");
         }
      else
         esyslog("ERROR: cFemonOsd::SvdrpTune() invalid channel.");
      }
   else
      esyslog("ERROR: cFemonOsd::SvdrpTune() unexpected connection state.");
   return false;
}

double cFemonOsd::GetVideoBitrate(void)
{
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  double value = 0.0;

  if (m_Receiver)
     value = m_Receiver->VideoBitrate();

  return (value);
}

double cFemonOsd::GetAudioBitrate(void)
{
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  double value = 0.0;

  if (m_Receiver)
     value = m_Receiver->AudioBitrate();

  return (value);
}

double cFemonOsd::GetDolbyBitrate(void)
{
  Dprintf("%s()\n", __PRETTY_FUNCTION__);
  double value = 0.0;

  if (m_Receiver)
     value = m_Receiver->AC3Bitrate();

  return (value);
}

eOSState cFemonOsd::ProcessKey(eKeys Key)
{ 
  eOSState state = cOsdObject::ProcessKey(Key);
  if (state == osUnknown) {
     switch (Key) {
       case k0:
            if ((m_Number == 0) && (m_OldNumber != 0)) {
               m_Number = m_OldNumber;
               m_OldNumber = cDevice::CurrentChannel();
               Channels.SwitchTo(m_Number);
               m_Number = 0;
               return osContinue;
               }
       case k1 ... k9:
            if (m_Number >= 0) {
               m_Number = m_Number * 10 + Key - k0;
               if (m_Number > 0) {
                  DrawStatusWindow();
                  cChannel *ch = Channels.GetByNumber(m_Number);
                  m_InputTime.Set(0);
                  // Lets see if there can be any useful further input:
                  int n = ch ? m_Number * 10 : 0;
                  while (ch && (ch = Channels.Next(ch)) != NULL) {
                        if (!ch->GroupSep()) {
                           if (n <= ch->Number() && ch->Number() <= n + 9) {
                              n = 0;
                              break;
                              }
                           if (ch->Number() > n)
                              n *= 10;
                           }
                        }
                  if (n > 0) {
                     // This channel is the only one that fits the input, so let's take it right away:
                     m_OldNumber = cDevice::CurrentChannel();
                     Channels.SwitchTo(m_Number);
                     m_Number = 0;
                     }
                  }
               }
            break;
       case kBack:
            return osEnd;
       case kGreen:
            {
            eTrackType types[ttMaxTrackTypes];
            eTrackType CurrentAudioTrack = cDevice::PrimaryDevice()->GetCurrentAudioTrack();
            int numTracks = 0;
            int oldTrack = 0;
            int track = 0;
            for (int i = ttAudioFirst; i <= ttDolbyLast; i++) {
                const tTrackId *TrackId = cDevice::PrimaryDevice()->GetTrack(eTrackType(i));
                if (TrackId && TrackId->id) {
                   types[numTracks] = eTrackType(i);
                   if (i == CurrentAudioTrack)
                      track = numTracks;
                   numTracks++;
                   }
                }
            oldTrack = track;
            if (++track >= numTracks)
               track = 0;
            if (track != oldTrack) {
               cDevice::PrimaryDevice()->SetCurrentAudioTrack(types[track]);
               Setup.CurrentDolby = IS_DOLBY_TRACK(types[track]);
               }
            }
            break;
       case kYellow:
            if (IS_AUDIO_TRACK(cDevice::PrimaryDevice()->GetCurrentAudioTrack())) {
               int audioChannel = cDevice::PrimaryDevice()->GetAudioChannel();
               int oldAudioChannel = audioChannel;
               if (++audioChannel > 2)
                  audioChannel = 0;
               if (audioChannel != oldAudioChannel) {
                  cDevice::PrimaryDevice()->SetAudioChannel(audioChannel);
                  }
               }
            break;
/* TB: hasn't got any effect 'cause of the netceiver */
#if defined(RBLITE) || !defined(REELVDR)
       case kRight:
            DeviceSwitch(1);
            break;
       case kLeft:
            DeviceSwitch(0);
            break;
#endif
       case kUp|k_Repeat:
       case kUp:
       case kDown|k_Repeat:
       case kDown:
            m_OldNumber = cDevice::CurrentChannel();
            cDevice::SwitchChannel(NORMALKEY(Key) == kUp ? 1 : -1);
            m_Number = 0;
            break;
       case kNone:
            if (m_Number && (m_InputTime.Elapsed() > CHANNELINPUT_TIMEOUT)) {
               if (Channels.GetByNumber(m_Number)) {
                  m_OldNumber = cDevice::CurrentChannel();
                  Channels.SwitchTo(m_Number);
                  m_Number = 0;
                  }
               else {
                  m_InputTime.Set(0);
                  m_Number = 0;
                  }
               }
            break;
       case kOk:
            // toggle between display modes
            if (++m_DisplayMode == eFemonModeAC3 && !Channels.GetByNumber(cDevice::CurrentChannel())->Dpid(0)) m_DisplayMode++;
            if (m_DisplayMode >= eFemonModeMaxNumber) m_DisplayMode = 0;
            DrawInfoWindow();
            break;
       default:
            break;
       }
     state = osContinue;
  }
  return state;
}
