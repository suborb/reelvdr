/*
 * Frontend Status Monitor plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <string.h>
#include "femoncfg.h"

cFemonConfig femonConfig;

cFemonConfig::cFemonConfig(void)
{
  hidemenu       = 0;
  displaymode    = 0;
  skin           = 0;
  theme          = 3;
  position       = 0;
  redlimit       = 33;
  greenlimit     = 66;
  updateinterval = 5;
  analyzestream  = 1;
  calcinterval   = 20;
  showcasystem   = 1;
#ifdef NTSC
  osdheight      = 420;
#else
  osdheight      = 480;
#endif
  osdoffset      = 0;
  usesvdrp       = 0;
  svdrpport      = 2001;
  strncpy(svdrpip, "0.0.0.0", sizeof(svdrpip));
}

const cFemonTheme femonTheme[eFemonThemeMaxNumber] =
{
  {
    // eFemonThemeClassic
    32,          // bpp
    0x7F000000, // clrBackground
    0xFFFCFCFC, // clrTitleBackground
    0xFF000000, // clrTitleText
    0xFFFCC024, // clrActiveText
    0xFFFCFCFC, // clrInactiveText
    0xFFFC1414, // clrRed
    0xFFFCC024, // clrYellow
    0xFF24FC24, // clrGreen
  },
  {
    // eFemonThemeElchi
    32,          // bpp
    0xC8000066, // clrBackground
    0xC833AAEE, // clrTitleBackground
    0xFF000000, // clrTitleText
    0xFFCCBB22, // clrActiveText
    0xFFFFFFFF, // clrInactiveText
    0xFFFF0000, // clrRed
    0xFFFFEE00, // clrYellow
    0xFF33CC33, // clrGreen
  },
  {
    // eFemonThemeDeepBlue
    32,          // bpp
    0xC80C0C0C, // clrBackground
    0xC832557A, // clrTitleBackground
    0xFF000000, // clrTitleText
    0xFFCE7B00, // clrActiveText
    0xFF9A9A9A, // clrInactiveText
    0xFF992900, // clrRed
    0xFFCE7B00, // clrYellow
    0xFF336600, // clrGreen
  },
/*
  {
    // eFemonThemeMoronimo
    32,          // bpp
    0xDF294A6B, // clrBackground
    0xDF3E5578, // clrTitleBackground
    0xFF9BBAD7, // clrTitleText
    0xFFCE7B00, // clrActiveText
    0xFF9A9A9A, // clrInactiveText
    0xFF992900, // clrRed
    0xFFCE7B00, // clrYellow
    0xFF336600, // clrGreen
  },
*/
 {
    // ReelBlue
    32,          // bpp
    0xEF0C1F33, // clrBackground
    0xEF0D1D50, // clrTitleBackground
    0xFFEEEEEE, // clrTitleText
    0xFFCCBB22, // clrActiveText
    0xFFFFFFFF, // clrInactiveText
    0xFFFF0000, // clrRed
    0xFFFFEE00, // clrYellow
    0xFF33CC33, // clrGreen
  },
  {
    // eFemonThemeEnigma
    32,          // bpp
    0xB8DEE5FA, // clrBackground
    0xB84158BC, // clrTitleBackground
    0xFFFFFFFF, // clrTitleText
    0xFF000000, // clrActiveText
    0xFF000000, // clrInactiveText
    0xB8C40000, // clrRed
    0xB8C4C400, // clrYellow
    0xB800C400, // clrGreen
  },
  {
    // eFemonThemeEgalsTry
    32,          // bpp
    0xCA2B1B9E, // clrBackground
    0xDFBEBAC3, // clrTitleBackground
    0xFF280249, // clrTitleText
    0xFFD4D7DB, // clrActiveText
    0xDFCFCFCF, // clrInactiveText
    0xFFFF0000, // clrRed
    0xFFFCC024, // clrYellow
    0xFF20980B, // clrGreen
  },
  {
    // eFemonThemeDuotone
    2,          // bpp
    0x7F000000, // clrBackground
    0xFFFCFCFC, // clrTitleBackground
    0x7F000000, // clrTitleText
    0xFFFCFCFC, // clrActiveText
    0xFFFCFCFC, // clrInactiveText
    0xFFFC1414, // clrRed
    0xFFFCFCFC, // clrYellow
    0xFFFCFCFC, // clrGreen
  },
  {
    // eFemonThemeSilverGreen
    32,          // bpp
    0xD9526470, // clrBackground
    0xD9293841, // clrTitleBackground
    0xFFB3BDCA, // clrTitleText
    0xFFCE7B00, // clrActiveText
    0xFFB3BDCA, // clrInactiveText
    0xFF992900, // clrRed
    0xFFCE7B00, // clrYellow
    0xFF336600, // clrGreen
  },
};
