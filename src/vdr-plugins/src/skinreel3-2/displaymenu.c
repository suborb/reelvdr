
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

#include "pngutils.h"

#include <vdr/debug.h>
#include <vdr/plugin.h>
#include <vdr/status.h>

#include "displaymenu.h"
#include "config.h"
#include "reel.h"
#include "logo.h"
#include "status.h"
#include "tools.h"

extern cTheme Theme;

static char oldThumb[512][NR_UNBUFFERED_SLOTS];
static int thumbCleared[NR_UNBUFFERED_SLOTS] = {1};
static cPlugin *rbPlugin = NULL;
static bool itemsBgSaved = false;
extern int progressbarheight;
static time_t old_menulist_mtime = 0; /** The last time the "menulist"-file has changed */

//#define DEBUG_TIMES 1
#ifdef DEBUG_TIMES
#include <sys/time.h>
#include <time.h>
extern struct timeval menuTimeVal1;
#endif

#define RECORDINGINFO_IMAGE_XPOS 14
#define RECORDINGINFO_IMAGE_YPOS 340
#define RECORDINGINFO_IMAGE_RESIZE_WIDTH 140

#define RECORDINGINFO_ZOOMED_IMAGE_YPOS 80
#define RECORDINGINFO_ZOOMED_IMAGE_RESIZE_WIDTH 500
#define RECORDINGINFO_ZOOMED_IMAGE_XPOS (720 - 500)/2

#define EPG_IMAGE_XPOS 445
#define EPG_IMAGE_YPOS 50
#define EPG_IMAGE_RESIZE_WIDTH 176

//#define DEBUG_OSD

void ClearBg(cOsd* osd, int btnx, int btny, int btnx0, int btny0, int btnx1, int btny1);

/** cSkinReelDisplayMenu */

int cSkinReelDisplayMenu::WrapText(int x, int y, const char *sText, tColor fFG, tColor fBG, const cFont *fFont, int lWidth, int MaxHeight, bool squeeze)
{
	int lHeight = fFont->Height(sText);
	cTextWrapper lTextWrap (sText, fFont, lWidth);
	int nVerticalSpaceRequired = 0;
	if (MaxHeight < 0 ) // ie no vertical space restriction
	    MaxHeight = INT_MAX;

	int nSqueeze = 0;
	if(lTextWrap.Lines() > 1 && squeeze)
		nSqueeze = fFont->Height() / 5;  // Not enough room to display more lines -> reduce space between lines and offset
	const char *currLine = NULL;
	char tmpBuffer[256];

	for (int i = 0; i < lTextWrap.Lines(); i++)
	{
	    if (MaxHeight <= nVerticalSpaceRequired + (lHeight - nSqueeze))
	    { // reached vertical space limit; dont draw next line
		break;
	    }

	    currLine = lTextWrap.GetLine(i);

	    /* check if this is the last possible line to be drawn
	     and add ellipsis '...' as the last word
         AND there is at least on more line to be drawn (so that '...' has meaning)*/
	    if (MaxHeight <= nVerticalSpaceRequired + 2*(lHeight - nSqueeze)
		    && i < lTextWrap.Lines()-1)
	    {
		/* check if ellipsis can be appended to the current line without
		    over shooting the width : lWidth */
		snprintf(tmpBuffer, 255, "%s...", currLine);

		if (fFont->Width(tmpBuffer) > lWidth) /* overshot the lWidth constraint */
		{

		    strncpy(tmpBuffer, currLine, 255);

		    int len = strlen(tmpBuffer);

		    char* lastWord = strrchr(tmpBuffer, ' '); // last space

		    // remove last word and replace with ...
		    if (lastWord)
		    {
			lastWord++;
			strcpy(lastWord, "...");
			lastWord[3] = '\0';
		    }//if
		    else  /* no space found (word), hence replace
			    last three characters with "..."*/
		    {
			int startJ = std::min(len-1, 255);
			for (int j=startJ, count = 0;
			     j >= 0 && count < 3 /* only three '.' */;
			     count ++, --j)
			    tmpBuffer[j] = '.';
		    } // else
		} // if
		// else we show tmpbuffer with appended '...'

		/* show this string */
		currLine = tmpBuffer;

	    } // if
		osd->DrawText(x, y - nSqueeze + (lHeight - nSqueeze)*i, currLine, fFG, fBG, fFont, lWidth, lHeight, taLeft);

		nVerticalSpaceRequired += (lHeight - nSqueeze);
		if(i==1) nVerticalSpaceRequired -= nSqueeze; // Calculate vertical space (more than 1 line -> reduce offset)
	}

	if(lTextWrap.Lines() > 1) nVerticalSpaceRequired += Gap;  // A bit distance to next line if text have been squeezed
	return nVerticalSpaceRequired;
}


cSkinReelDisplayMenu::cSkinReelDisplayMenu(void) {

#ifdef DEBUG_TIMES
  struct timeval now;
  gettimeofday(&now, NULL);
  float secs2 = ((float)((1000000 * now.tv_sec + now.tv_usec) - (menuTimeVal1.tv_sec * 1000000 + menuTimeVal1.tv_usec))) / 1000000;
  printf("\n=== cSkinReelDisplayMenu-Start time since key: secs: %f\n", secs2);
#endif

  for(int i=0; i<NR_UNBUFFERED_SLOTS; i++) {
    strcpy(ReelConfig.curThumb[i].path, "");
    thumbCleared[i] = 1;
    ReelConfig.curThumb[i].x = ReelConfig.curThumb[i].y = ReelConfig.curThumb[i].w = ReelConfig.curThumb[i].h = 0;
  }

  /*clear little-icons list*/
  LittleIcons.clear();

  struct ReelOsdSize OsdSize;
  ReelConfig.GetOsdSize(&OsdSize);

  withTitleImage = false;
  old_menulist_mtime = 0;

  fSetupAreasDone = false;
  setlocale(LC_TIME, tr("en_US"));
  osd = NULL;

  pFontList = ReelConfig.GetFont(FONT_LISTITEM);
  pFontOsdTitle = ReelConfig.GetFont(FONT_OSDTITLE);

  const cFont *pFontDate = ReelConfig.GetFont(FONT_DATE);
  const cFont *pFontDetailsTitle = ReelConfig.GetFont(FONT_DETAILSTITLE);
  const cFont *pFontDetailsSubtitle = ReelConfig.GetFont(FONT_DETAILSSUBTITLE);
  const cFont *pFontDetailsDate = ReelConfig.GetFont(FONT_DETAILSDATE);

#if APIVERSNUM >= 10718
  regularFont15 = cFont::CreateFont("/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf", 15);
  regularFont13 = cFont::CreateFont("/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf", 13);
  regularFont12 = cFont::CreateFont("/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf", 12);
  regularFont11 = cFont::CreateFont("/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf", 11);
  boldFont15 = cFont::CreateFont("/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf", 15);
#endif
  isMainPage = 0;
  isCAPS = false;
  strTitle = NULL;
  strLastDate = NULL;
  strTheme = strdup(Theme.Name());
  isMainMenu = false;
  nMessagesShown = 0;
  nNumImageColors = 2;
  alreadyCleared = true;
  itemsBgSaved = false;

  PluginName = "";
  PluginIconFile = "";


  int LogoHeight = std::max(std::max(pFontOsdTitle->Height(), pFontDate->Height()) + TitleDeco + pFontDetailsTitle->Height() + Gap + pFontDetailsSubtitle->Height(),
                            std::max(3 * pFontDate->Height(), (ReelConfig.showImages ? std::max(ReelConfig.imageHeight, IconHeight) : IconHeight)));
  int LogoWidth = ReelConfig.showImages ? std::max(IconWidth, ReelConfig.imageWidth) : IconWidth;
  cString date = GetFullDateTime();
  int RightColWidth = (SmallGap + Gap + std::max(MIN_DATEWIDTH + LogoWidth, pFontDate->Width(date)) + Gap) & ~0x07; // must be multiple of 8

  const cFont *pFontMessage = ReelConfig.GetFont(FONT_MESSAGE);
  int MessageHeight =  std::max(pFontMessage->Height(), MINMESSAGEHEIGHT);

  /* title bar */
  xTitleLeft = 20;
  xTitleRight = OsdSize.w - RightColWidth;
  yTitleTop = 10;
  yTitleBottom = std::max( std::max(pFontOsdTitle->Height(), pFontDate->Height()), IMGHEADERHEIGHT);
  yTitleDecoTop = yTitleBottom + TitleDecoGap;
  yTitleDecoBottom = yTitleDecoTop + TitleDecoHeight;
  /* Footer area */
  xFooterLeft = xTitleLeft;
  xFooterRight = OsdSize.w;
  yFooterTop = OsdSize.h - IMGFOOTERHEIGHT;
  yFooterBottom = OsdSize.h;
  /* help buttons */
  xButtonsLeft = xTitleLeft + IMGMENUWIDTH;
  xButtonsRight = OsdSize.w - Roundness - 25 ;
  yButtonsTop = yFooterTop + 1 + (IMGFOOTERHEIGHT - IMGBUTTONSHEIGHT) / 2 - 5;
  yButtonsBottom =  yButtonsTop + IMGBUTTONSHEIGHT /*- 5*/;

  /* content area with items */
  /* Body */
  xBodyLeft = xTitleLeft;
  xBodyRight = xTitleRight;
  yBodyTop = yTitleBottom + Gap;
  /* message area */
  xMessageLeft = xBodyLeft;
  xMessageRight = OsdSize.w;
  /* on help buttons */
  yMessageBottom = yFooterTop - 2*Gap;
  yMessageTop = yMessageBottom - MessageHeight;
  yBodyBottom = yMessageTop;
  /* logo box */
  xLogoLeft = OsdSize.w - LogoWidth;
  xLogoRight = OsdSize.w;
  yLogoTop = yTitleTop;
  yLogoBottom = yLogoTop + LogoHeight + SmallGap;
  /* info box */
  xInfoLeft = OsdSize.w - RightColWidth;
  xInfoRight = OsdSize.w;
  yInfoTop = yLogoBottom+1;
  yInfoBottom = yBodyBottom+1;
  /* date box */
  xDateLeft = xTitleRight;
  xDateRight = OsdSize.w;
  yDateTop = yTitleTop;
  yDateBottom = yLogoBottom;

  /* create osd */
#ifdef REELVDR
  osd = cOsdProvider::NewTrueColorOsd(OsdSize.x, OsdSize.y,Setup.OSDRandom);
#if REELVDR && APIVERSNUM >= 10718
  enableSideNote = false;
  xSideLeft = xBodyLeft + Gap;
  xSideRight = Roundness + IMGMENUWIDTH - 4*Gap;
#endif /* REELVDR && APIVERSNUM >= 10718 */
#else
  osd = cOsdProvider::NewTrueColorOsd(OsdSize.x, OsdSize.y,0);
#endif

  tArea Areas[] = { {xTitleLeft, yTitleTop, xMessageRight, yButtonsBottom, 32} };
  if (ReelConfig.singleArea8Bpp && osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea)) == oeOk) {
    debug("cSkinReelDisplayMenu: using %dbpp single area", Areas[0].bpp);
    osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
    nNumImageColors = 230; //TODO: find correct number of colors
  } else {
    debug("cSkinReelDisplayMenu: using multiple areas");
    tArea Areas[] = { {xTitleLeft,   0, xTitleRight - 1, yTitleDecoBottom - 1, 2}, //title area
                      {xBodyLeft,    yBodyTop, xBodyRight - 1, yInfoTop + pFontDetailsDate->Height() - 1, 2}, //body area (beside date/logo/symbols area)
                      {xDateLeft,    yDateTop, xLogoRight - 1, yInfoTop - 1, 4}, //date/logo area
                      {xInfoLeft,    0, xInfoRight - 1, yInfoTop + pFontDetailsDate->Height() - 1, 4}, //area for symbols in event/recording info
                      {xBodyLeft,    yInfoTop + pFontDetailsDate->Height(), xInfoRight - 1, (ReelConfig.statusLineMode == 1 ? yBodyBottom : yMessageTop) - 1, 2}, // body/info area (below symbols area)
                      {xMessageLeft, yMessageTop, xButtonsRight - 1, yButtonsBottom - 1, 4} //buttons/message area
    };

    eOsdError rc = osd->CanHandleAreas(Areas, sizeof(Areas) / sizeof(tArea));
    if (rc == oeOk)
      osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
    else {
      error("cSkinReelDisplayMenu: CanHandleAreas() [1] returned %d", rc);
      delete osd;
      osd = NULL;
      throw 1;
      return;
    }

    nNumImageColors = 13; // "16 available colors" - "clrTransparent" - "clrLogoBg" - "clrMenuTxtFg"
  }
#if 0 //def DEBUG_TIMES
  gettimeofday(&now, NULL);
  secs2 = ((float)((1000000 * now.tv_sec + now.tv_usec) - (menuTimeVal1.tv_sec * 1000000 + menuTimeVal1.tv_usec))) / 1000000;
  printf("\n=== cSkinReelDisplayMenu-Middle-C time since key: secs: %f\n", secs2);
#endif

  lineHeight = std::max(pFontList->Height() , 35); // change
  xItemLeft = xBodyLeft + (ReelConfig.showMarker ? lineHeight : ListHBorder);

  xItemRight = xInfoRight - ScrollbarWidth - ListHBorder;
  yItemTop = 0; /* yItemTop is used in MaxItems but uninitialised yet (Thx Quacks@vdr-portal!) */
  int numItems = MaxItems();
  yItemTop = yBodyTop + ((ReelConfig.statusLineMode == 2 ? yMessageTop : yBodyBottom) - yBodyTop - numItems * lineHeight) / 2;

  SetImagePaths(osd);
  CreateTitleVector();

  textScroller.SetGlobals(xItemLeft, yBodyTop, xItemRight);

#ifdef USE_WAREAGLEICON
  // use the icons instead of the font symbols
  Icons::InitCharSet();
#endif
#ifdef DEBUG_TIMES
  gettimeofday(&now, NULL);
  secs2 = ((float)((1000000 * now.tv_sec + now.tv_usec) - (menuTimeVal1.tv_sec * 1000000 + menuTimeVal1.tv_usec))) / 1000000;
  printf("\n=== cSkinReelDisplayMenu-End time since key: secs: %f\n", secs2);
#endif

}

void cSkinReelDisplayMenu::SetupAreas(void) {
  fSetupAreasDone = true;

  xItemRight = xInfoRight - ScrollbarWidth - ListHBorder;

  DrawTitle(strTitle);

#ifndef REELVDR
  /* clear background */
  osd->DrawImage(imgMenuBodyUpperPart, xBodyLeft, yBodyTop, false, xInfoRight - xBodyLeft-10, 1);
  osd->DrawImage(imgMenuBodyLowerPart, xBodyLeft, yBodyTop+205, false, xInfoRight - xBodyLeft-10, 1);
#endif

  /* the shadowed border on the right */
  osd->DrawImage(imgMenuBodyRightUpperPart, xInfoRight -10, yBodyTop, false);
  osd->DrawImage(imgMenuBodyRightLowerPart, xInfoRight -10, yBodyTop+205, false);
}

cSkinReelDisplayMenu::~cSkinReelDisplayMenu() {
  free(strTheme);
  free(strTitle);
  free(strLastDate);
  delete osd;
#if APIVERSNUM >= 10718
  delete regularFont15;
  delete regularFont13;
  delete regularFont12;
  delete regularFont11;
  delete boldFont15;
#endif
}

#if APIVERSNUM >= 10718 && REELVDR
// displays string list, one string on one line
void cSkinReelDisplayMenu::SideNote(const cStringList * strList) {


    if (!enableSideNote)
      return;


    if (!strList) {
        DrawReelBanner();
        return;
    }


    /// ID3 tags
    if (strcmp(strList->At(0), "ID3Tags" )==0) {
        ID3Tags.clear();
        struct ID3Tag tag;
        for (int i=1; i<strList->Size(); ++i) {
            tag.value = strList->At(i);
            ID3Tags.push_back(tag);
        } // for
        //printf("ID3Tags : %d\n", ID3Tags.size());
        PrintID3Infos();
        return;
    } // if

    /*y boundries*/
    int y = yItemTop;
    int i = 0;

    /// STARTFROM
#define STARTFROM "STARTFROM"
    if (strncmp(strList->At(0), STARTFROM, strlen(STARTFROM))==0) {
        std::string str = strList->At(0);
        str = str.substr(strlen(STARTFROM));
        y = atoi(str.c_str());

        i++;
    }
    else

    EmptySideNote();

    const char *s = NULL;
    /* strings drawn within */
    //int xSideLeft  = xBodyLeft + Gap;
    //int xSideRight = Roundness + IMGMENUWIDTH - Gap;

    /* to yBodyBottom */
    const cFont *sideNoteFont = boldFont15;

    // Draw each string in its own line
    for ( ; i < strList->Size(); ++i) {
        s = strList->At(i);

        if (strcmp(s, "FOLDER ICON") == 0) {
            DrawUnbufferedImage(osd, "recordings_folder.png", xSideLeft, y, true);
            y += 108 + 2*Gap;
        } else if (strstr(s, "FOLDER NAME:")) {
            int lT = WrapText(xSideLeft, y, strchr(s,':')+1,
                              Theme.Color(clrRecTitleFg),
                              clrTransparent, boldFont15,
                              xSideRight - xSideLeft - 2*Gap);
            y += lT;
        }
        else {

            osd->DrawText(xSideLeft, y,
                          s,
                          Theme.Color(clrMenuItemNotSelectableFg), clrTransparent,
                          sideNoteFont,
                          xSideRight-xSideLeft);

            y += sideNoteFont->Height();
        }
    } // for

} // SideNote(const cStringList*)


/* draw icons on the last 'line' of sidenote area
   starts to draw from right edge

   Only icons relevant for recordings handled here
 */
void cSkinReelDisplayMenu::SideNoteIcons(const char *icons) {

#if 1
    if (!enableSideNote || !icons) return;

    /* strings drawn within */
    //int xSideLeft  = xBodyLeft + Gap;
    //int xSideRight = Roundness + IMGMENUWIDTH - Gap;

    int xt = xSideLeft;
    int y = yBodyBottom - IMGICONWIDTH - 2*Gap;

    int icon_enum = -1;
    for (; *icons; ++icons) {
        //printf("icons %d", *icons);
        icon_enum = -1;
        switch (*icons) {

        case char (80):
            icon_enum = imgIconPinProtect;
            break;

        case char(129):
            icon_enum = imgIconArchive;
            break;

        case char(132):
            icon_enum = imgIconNewrecording;
            break;

        case char(133):
            icon_enum = imgIconCut;
            break;

        case '#':
        case char(136): /// moved
            icon_enum = imgIconRecord;
            break;

        case char(142):
            icon_enum = imgIconHd;
            break;

        case char(143):
            icon_enum = imgIconHdKey;
            break;

        case char(144):
            DrawUnbufferedImage(osd, "icon_hd_new.png", xt, y+1, true);
            xt += (IMGICONWIDTH + 2*Gap);
            break;

        case char (145):
            DrawUnbufferedImage(osd, "icon_key_new.png", xt, y+1, true);
            xt += (IMGICONWIDTH + 2*Gap);
            break;

        case char (148):
            DrawUnbufferedImage(osd, "icon_compressed.png", xt, y+1, true);
            xt += (IMGICONWIDTH + 2*Gap);
            break;

        default:
            icon_enum = -1;
            break;
        } // switch

        if (icon_enum > 0) {
            if (xt < xSideRight) {
                osd->DrawImage(icon_enum, xt, y, true);
                xt += (IMGICONWIDTH + 2*Gap);
            }
            else {
                printf("too many icons drawn? overflowing on the left\n");
                break;
            } // else
        } // if
    } //for


    /** any progress bars?*/
    int pb_start, pb_end, progressCount;

    // get the length of progress bar if any
    int pbtotal = pgbar(icons, pb_start, pb_end, progressCount);

    // no progressbar
    if (!pbtotal)
        return;

    // get thickness in term of hard numbers
    int pbthickness = 6;
    int pbLength = 78;
    int xPB = xSideLeft;
    //XXX check if it exceeds OSD
    int yPB = yBodyBottom - 2*IMGICONWIDTH + (IMGICONWIDTH - pbthickness)/2;

    int percentCompleted = (progressCount*100)/pbtotal;
    /* check if percentage is within [0,100] */
    if ( percentCompleted < 0 )
      percentCompleted = 0;
    else if (percentCompleted > 100)
      percentCompleted = 100;

    /** draw progressbar */
    DrawProgressBar(osd, xPB, yPB, xPB + (int)(pbLength*percentCompleted/100), xPB + pbLength);

#endif
}

void cSkinReelDisplayMenu::SideNote(const cRecording* Recording) {

    if (!enableSideNote) return;



    if (!Recording) {
        DrawReelBanner();
        return;
    } // if

    EmptySideNote();

    const cRecordingInfo *Info = Recording->Info();
    if (Info == NULL) {
      //TODO: draw error message
      return;
    }


    /* strings drawn within */
    //int xSideLeft  = xBodyLeft + Gap;
    //int xSideRight = Roundness + IMGMENUWIDTH - Gap;


    const cFont *pFontDetailsTitle = boldFont15;
    const cFont *pFontDetailsSubtitle = regularFont13;
    const cFont *pFontDetailsDate = regularFont12;
    const cFont * pFontDescription = pFontDetailsDate;

    isMainMenu = false;

    string PngPath = Recording->FileName();
    PngPath += "/preview_vdr.png"; // or "vdr.png" got from iepg plugin?

    // If image is available change background on left size in order to display the image
    struct stat results;
    if ( stat ( PngPath.c_str(), &results ) == 0 && results.st_size > 0) // file exists and is not empty
    {
#if 0
          if(memcmp((char *)Recording->Info()->Title(), "ZOOMPIC$", 8) == 0)
          {
              // ReDraw original banner
              SetTitle(I18nTranslate("Recording info", "vdr"));

              DrawZoomedImage(Recording);
              osd->Flush();
              return;  // Do nothing else -> return
          }
#endif


      #define W 126
      #define H 128
              int x1 = xSideLeft;
              //int dx1 = xSideLeft - (xBodyLeft - Roundness);
             /* printf("xSideLeft(%d) - xBodyLeft (%d) = %d", xSideLeft, xBodyLeft,
                     xSideLeft - xBodyLeft);

              printf("current width %d\n", xBodyLeft-Roundness + IMGMENUWIDTH - dx1 -x1);
              */

              int y1 = yBodyTop + Roundness;
              /*
              osd->DrawRectangle(x1, y1,
                                 xBodyLeft-Roundness + IMGMENUWIDTH - dx1, y1+H,
                                 clrMagenta);

                */

              osd->SetImagePath(imgUnbufferedEnum, PngPath.c_str());
              osd->DrawCropImage(imgUnbufferedEnum, x1, y1, x1, y1, x1 + W, y1 + H, false);
              //osd->DrawRectangle(x1, y1,
              //                   x1 + W, y1 + H,
              //                   clrMagenta);


    } // if (stat )
    else /* no preview available, draw reel logo */
    {

        int x, y;
        /* middle of rectangle starting at xSideLeft, yBodyTop+Roundnes
           and with width W height H */

        /* reel logo 80x80 */
        x = xSideLeft + (W-80)/2;
        y = yBodyTop + Roundness + (H-80)/2;

        // draw reel logo
        osd->DrawImage(imgNoChannelIcon, x, y, true);
    }

    int y = yBodyTop + 10;
    /// TODO Use DrawCropImage()
#if 0
    if(PngPath.size() && 0)
        DrawImage((char *)PngPath.c_str(), 'P' /* R=raw, P=png, J=jpeg */,
                  xBodyLeft + Roundness, y /*RECORDINGINFO_IMAGE_YPOS*/,
                  RECORDINGINFO_IMAGE_RESIZE_WIDTH - 30);
    //else
#endif



   /* calculate height from width  OR
     TODO : have a fixed rectange into which preview/logo is drawn */
    y += (3*RECORDINGINFO_IMAGE_RESIZE_WIDTH/4) + pFontDetailsTitle->Height();
    y = yBodyTop + Roundness + H + 2*Gap;

    /* draw additional information */
    std::stringstream sstrInfo;

    std::stringstream sstrDate_time;

#if APIVERSNUM < 10721
    sstrDate_time << *DateString(Recording->start)
                  << "  " << *TimeString(Recording->start) ;
#else
    sstrDate_time << *DateString(Recording->Start())
                  << "  " << *TimeString(Recording->Start());
#endif

    const char *Title = Info->Title();

    if (isempty(Title))
      Title = Recording->Name();

    if (isempty(Title))
        Title = tr("Unknown Recording");


    /** draw recording title */
    // show full title, so wrap it
    int lT = WrapText(xSideLeft, y, Title,
                      Theme.Color(clrRecTitleFg),
                      clrTransparent, pFontDetailsTitle,
                      xSideRight - xSideLeft);
    y += lT;




    /** draw recording short text */
    const char* strDescr = NULL;
    if (isempty(Info->Description())) {
      if (!isempty(Info->ShortText())) { /* check if short text */
        strDescr = Info->ShortText();
        /* draw short text as description, if no description available */
      }
    } else {
      if (!isempty(Info->ShortText())) { /* check if short text */
        //y += pFontDetailsTitle->Height() + Gap; /* draw below Title */
        /* draw short text */
#if 0
        osd->DrawText(xSideLeft, y, Info->ShortText(),
                      Theme.Color(clrMenuItemNotSelectableFg), clrTransparent,
                      pFontDetailsSubtitle,
                      xSideRight - xSideLeft ,
                      pFontDetailsSubtitle->Height());
        y +=  pFontDetailsSubtitle->Height();
#endif
        /*draw at least 3 lines of short text*/
        int lT = WrapText(xSideLeft, y,  Info->ShortText(),
                          Theme.Color(clrMenuItemNotSelectableFg),
                          clrTransparent, pFontDetailsSubtitle,
                          xSideRight - xSideLeft,  pFontDetailsSubtitle->Height()*3);

        y += lT;
      }
      strDescr = Info->Description(); /* draw description */
    }

    /* jump a half 'line' before date & time */
    y +=  pFontDetailsDate->Height()/2;
    /** draw Date and time */
    osd->DrawText(xSideLeft, y,
                  sstrDate_time.str().c_str(),
                  Theme.Color(clrRecDateFg), clrTransparent, pFontDetailsDate,
                  xSideRight - xSideLeft - Gap - 1 - Roundness,
                  pFontDetailsDate->Height());
    y += pFontDetailsDate->Height();


    /*duration: read size of index.vdr OR index file
      and divide by appropriate factor to get duration in minutes.
      See extrecmenu for details */
    int duration = -1;
    string indexPath = string(Recording->FileName()) + "/index";

    int fps = 25;
#if APIVERSNUM > 10700
    fps = Recording->FramesPerSecond();
#endif

    struct stat st;
    if (stat(indexPath.c_str(), &st) == 0) /*file exists*/
        duration = (int)(st.st_size/480/fps);
    else
    {
        indexPath = string(Recording->FileName()) + "/index.vdr";
        if (stat(indexPath.c_str(), &st) == 0)
            duration = (int)(st.st_size/480/fps);
    }


    /** draw duration */
    if (duration >= 0) {
        std::stringstream strDuration;
        strDuration << duration << " " << tr("Minutes");

        osd->DrawText(xSideLeft, y,
                      strDuration.str().c_str(),
                      Theme.Color(clrRecDateFg), clrTransparent, pFontDetailsDate,
                      xSideRight - xSideLeft - Gap - 1 - Roundness,
                      pFontDetailsDate->Height());
        y += pFontDetailsDate->Height();

    } // duration
    //printf("after date+time y = %d,  %d minutes\n", y, duration);


    std::string stringInfo = sstrInfo.str();
    const char *strInfo = stringInfo.empty() ? NULL : stringInfo.c_str();

    if (strDescr || strInfo) {
      //y = yHeadlineBottom + SmallGap + 2 * pFontDetailsDate->Height();

      std::stringstream mytext;

      if (ReelConfig.showAuxInfo) {
        if(strDescr)
           mytext << strDescr;
        if(strInfo && strDescr)
          mytext <<  std::endl << std::endl;
        if(strInfo)
          mytext << strInfo;
      } else {
        if(strInfo)
           mytext << strInfo;
        if(strInfo && strDescr)
           mytext << std::endl << std::endl;
        if(strDescr)
           mytext << strDescr;
      }


      //printf("cRecordings: '%s'\n", mytext.str().c_str());
      y += pFontDetailsDate->Height()/2;
      //printf("before wrap y = %d\n", y);

      WrapText(xSideLeft, y, mytext.str().c_str(),
               Theme.Color(clrMenuTxtFg), clrTransparent,
               pFontDescription,
               xSideRight - xSideLeft - Gap,
               yBodyBottom - y - Gap
               - 2*max(IMGICONWIDTH, pFontDescription->Height()) /* last line for icons*/,
               false);
    }


#if 0
    /// TODO in seperate function
    /// Draw progressbar + icons
    /** Draw percent Completed bar */
    int pbthickness = (int)( 0.4*pFontDetailsDate->Height("|") );
    int pbLength = pFontDescription->Width("Wed 01.01.2008");
    int xPB = xSideLeft;
    //XXX check if it exceeds OSD
    int yPB = yBodyBottom - 2*IMGICONWIDTH +
            (int)(0.5*pFontDetailsDate->Height() - 0.5*pbthickness );

    int percentCompleted = 60;
    /* check if percentage is within [0,100] */
    if ( percentCompleted < 0 )
      percentCompleted = 0;
    else if (percentCompleted > 100)
      percentCompleted = 100;

    /** draw progressbar */
    DrawProgressBar(osd, xPB, yPB, xPB + (int)(pbLength*percentCompleted/100), xPB + pbLength);

    printf("progressbar thichness %d length %d\n", pbthickness, pbLength);
#endif

#if 0
    /** DRAW ICONS */
    int xt = xSideRight-IMGICONWIDTH;
    y = yBodyBottom - IMGICONWIDTH; /* icons are square 20x20*/

    /* TODO char to icon enum mapping */
    osd->DrawImage(imgIconArchive, xt, y, true);
    xt -=IMGICONWIDTH;
    osd->DrawImage(imgIconCut, xt, y, true);

    char icons_str[32];
    icons_str[0] = char(132);
    icons_str[1] = char(80);
    icons_str[2] = char(142);
    icons_str[3] = char(145);
    icons_str[4] = char(136);
    SideNoteIcons(icons_str);
#endif

}

void cSkinReelDisplayMenu::SideNote(const cChannel *channel) {

    if (!enableSideNote)
      return;

    if (!channel) {
        DrawReelBanner();
        return;
    }

    /// find current event,
    /// if present show it via SideNote(const cEvent*)
    const cEvent *event = NULL;

    { // lock schedules only for this block
        cSchedulesLock schedulesLock;
        const cSchedules *schedules = cSchedules::Schedules(schedulesLock);

        if (schedules)
        {
            const cSchedule* schedule = schedules->GetSchedule(channel->GetChannelID());

            if (schedule)
                event = schedule->GetPresentEvent();

        } // if
    }// Lock

    // show current event info
    if (event) {
        SideNote(event);
        return;
    }

    // no event info available, so
    // draw half empty banner since only LOGO is being drawn
    int yMenuMiddle = yBodyTop + IMGMENUTOPHEIGHT; //top picture
    int yMenuBottom = yBodyTop + IMGMENUTOPHEIGHT +IMGMENUHEIGHT; //top + middle
    DrawUnbufferedImage(osd, "banner_halfempty_bottom.png", xBodyLeft-10, yMenuBottom, false);
    DrawUnbufferedImage(osd, "banner_halfempty_middle.png", xBodyLeft-10, yMenuMiddle, false);
    DrawUnbufferedImage(osd, "banner_halfempty_top.png", xBodyLeft-10, yBodyTop, false);

    /// Current event not found,
    /// show only Channel logo

#define H 128
#define W 126

    std::string logoPath;
    logoPath = ReelConfig.GetLogoPath(channel->Name());

    //printf("=== looking for logo in '%s'\n", logoPath.c_str());
    /* check for png file */
    bool drawReelLogo = true;

    if (access(logoPath.c_str(),R_OK) == 0 ) {

        cPngUtils imgFile;
        imgFile.ReadPng(logoPath.c_str());
        int w = imgFile.Width();
        int h = imgFile.Height();

        if (w > W || h > H) {
            // excedes box. TOO large draw reel logo instead

        } else {
            int x = xSideLeft + (W-w)/2;
            int y = yBodyTop + Roundness + (H-h)/2;
            osd->SetImagePath(imgUnbufferedEnum, logoPath.c_str());
            osd->DrawImage(imgUnbufferedEnum, x,y, true);
            drawReelLogo = false;
        }
    } // if

    if (drawReelLogo) { // No channel logo available, Draw Reel logo
        int x, y;
        /* middle of rectangle starting at xSideLeft, yBodyTop+Roundnes
           and with width W height H */

        /* reel logo 80x80 */
        x = xSideLeft + (W-80)/2;
        y = yBodyTop + Roundness + (H-80)/2;

        // draw reel logo
        osd->DrawImage(imgNoChannelIcon, x, y, true);
    } // if (drawReelLogo)

}

void cSkinReelDisplayMenu::SideNote(const cEvent *Event, const cStringList *sl) {



    if (!enableSideNote)
      return;



    if (!Event) {
        // do not leave side note area empty
        // draw reelbanner
        DrawReelBanner();
        return;
    }

    EmptySideNote();

    /* strings drawn within */
    //int xSideLeft  = xBodyLeft + Gap;
    //int xSideRight = Roundness + IMGMENUWIDTH - Gap;
    /*y boundries*/
    int y = yItemTop;

    const cFont *pFontDetailsTitle = boldFont15;
    const cFont *pFontDetailsSubtitle;
    const cFont *pFontDetailsDate = regularFont11;
    const cFont *pFontDescription = regularFont13;
    pFontDetailsSubtitle = pFontDetailsDate;

    char sPath[512];
    sprintf(sPath, "%s/%ld.raw", ReelConfig.GetEPGImagesDir(), (long)Event->EventID());
    struct stat results;
    bool bPicAvailable = ( stat ( sPath, &results ) == 0 );

    time_t now = time(NULL);


    int percentCompleted = 0; /* percent of current program completed */
    /* draw recording date string */
    std::stringstream sstrDate;
    std::stringstream sstrDuration;

    sstrDate << *Event->GetDateString() << "  " << *Event->GetTimeString()
             << " - " << *Event->GetEndTimeString();

    if (now > Event->StartTime()) {
      percentCompleted = ((now - Event->StartTime())*100) /60;
      sstrDuration << (now - Event->StartTime()) / 60 << '/';
    } else
      percentCompleted = 0;

    sstrDuration << Event->Duration() / 60 << tr("min") ;

    if ( Event->Duration() > 0 )
        percentCompleted /= ( Event->Duration() / 60 );
    else
        percentCompleted = 0;

#if 0
    int xs = xSideLeft;
    /* check if event has VPS */
    if (ReelConfig.showVps && Event->Vps()) {
      /* draw VPS symbol */
      if (Event->Vps() != Event->StartTime()) {
        static char vps[12];
        struct tm tm_r;
        time_t t_vps = Event->Vps();
        strftime(vps, sizeof(vps), "VPS: %H:%M", localtime_r(&t_vps, &tm_r));
        xs -= pFontDetailsDate->Width(vps);
        osd->DrawText(xSideLeft + Roundness /*xs*/, yDateBottom, vps,
                      Theme.Color(clrMenuTxtFg), clrTransparent,
                      pFontDetailsDate,
                      xSideRight - xSideLeft- Gap, pFontDetailsDate->Height());
        xs -= TinyGap;
      }
    }
#endif

#define H 128
#define W 126

    std::string logoPath;

    const cChannel *channel = Channels.GetByChannelID(Event->ChannelID());
    if (channel)
        logoPath = ReelConfig.GetLogoPath(channel->Name());

    //printf("=== looking for logo in '%s'\n", logoPath.c_str());
    /* check for png file */
    bool drawReelLogo = true;

    if ( !bPicAvailable && access(logoPath.c_str(),R_OK) == 0 ) {

        cPngUtils imgFile;
        imgFile.ReadPng(logoPath.c_str());
        int w = imgFile.Width();
        int h = imgFile.Height();

        if (w > W || h > H) {
            // excedes box. TOO large draw reel logo instead

        } else {
            int x = xSideLeft + (W-w)/2;
            int y = yBodyTop + Roundness + (H-h)/2;
            osd->SetImagePath(imgUnbufferedEnum, logoPath.c_str());
            osd->DrawImage(imgUnbufferedEnum, x,y, true);
            drawReelLogo = false;
        }
    } // if

    if (drawReelLogo) { // No channel logo available, Draw Reel logo
        int x, y;
        /* middle of rectangle starting at xSideLeft, yBodyTop+Roundnes
           and with width W height H */

        /* reel logo 80x80 */
        x = xSideLeft + (W-80)/2;
        y = yBodyTop + Roundness + (H-80)/2;

        // draw reel logo
        osd->DrawImage(imgNoChannelIcon, x, y, true);
    } // if (drawReelLogo)

    y += H + Gap;

    std::string stringInfo;

    int xHeadlineRight = xInfoRight;
    //TB osd->DrawRectangle(xBodyLeft, yBodyTop, xHeadlineRight - 1, yHeadlineBottom - 1, Theme.Color(clrAltBackground));

    if(bPicAvailable)
       xHeadlineRight -= W;


    /** draw recording title */
    int TitleHeight = WrapText(xSideLeft, y, Event->Title(),
                               Theme.Color(clrRecTitleFg), clrTransparent,
                               pFontDetailsTitle,
                               xSideRight - xSideLeft);
    y += TitleHeight;


    /** draw date and time */
    osd->DrawText(xSideLeft, y, sstrDate.str().c_str(),
                  Theme.Color(clrMenuItemNotSelectableFg), clrTransparent,
                  pFontDetailsDate,
                  xSideRight - xSideLeft,
                  pFontDetailsDate->Height());
    y += pFontDetailsDate->Height();


    /** Draw percent Completed bar */
    int pbthickness = (int)( 0.4*pFontDetailsDate->Height("|") );
    int pbLength = pFontDetailsDate->Width("Wed 01.01.2008");
    int xPB = xSideLeft;
    //XXX check if it exceeds OSD
    int yPB = y + (int)(0.5*pFontDetailsDate->Height() - 0.5*pbthickness );

    /* check if percentage is within [0,100] */
    if ( percentCompleted < 0 )
      percentCompleted = 0;
    else if (percentCompleted > 100)
      percentCompleted = 100;

    /** draw progressbar */
    DrawProgressBar(osd, xPB, yPB, xPB + (int)(pbLength*percentCompleted/100), xPB + pbLength);

    /** Event Duration */
    osd->DrawText( xPB + pbLength + 4*Gap, y , sstrDuration.str().c_str(),
                   Theme.Color(clrMenuItemNotSelectableFg),
                   clrTransparent /*Theme.Color(clrBackground)*/,
                   pFontDetailsDate);

    y += pFontDetailsDate->Height();


    /** draw recording short text and description */
    const char *strDescr = NULL;
    if (isempty(Event->Description())) {
      if (!isempty(Event->ShortText())) /* check if short text */
        strDescr = Event->ShortText(); /* draw short text as description, if no description available */
    } else {
      if (!isempty(Event->ShortText())) { /* check if short text */
        y += pFontDetailsSubtitle->Height()/2; // add more vertical space

        /* draw short text */
        int h = WrapText(xSideLeft, y, Event->ShortText(),
                 Theme.Color(clrMenuItemNotSelectableFg), clrTransparent,
                  pFontDetailsSubtitle,
                 xSideRight - xSideLeft);

        y += h;
      }
      strDescr = Event->Description(); /* draw description */
    }

    std::string stringReruns;

    const char *strFirst = NULL;
    const char *strSecond = NULL;
    const char *strThird = stringReruns.empty() ? NULL : stringReruns.c_str();

    if (ReelConfig.showAuxInfo) {
      strFirst = strDescr;
      strSecond = stringInfo.empty() ? NULL : stringInfo.c_str();
    } else {
      strFirst = stringInfo.empty() ? NULL : stringInfo.c_str();
      strSecond = strDescr;
    }

    if(bPicAvailable)
      DrawImage(sPath, 'R' /* R=raw, P=png, J=jpeg */, xSideLeft, yBodyTop + Roundness, W);

    /* draw description text till ...*/
    int yDescEnd = yBodyBottom - Gap - (sl&&sl->Size()?4:2)*pFontDetailsTitle->Height();
    /* a line for satellite name, 2 lines for cStringList 'sl',
       last line for icons*/

    if (strFirst || strSecond || strSecond) {

      std::stringstream mytext;
      if(strFirst)
        mytext << strFirst;
      if(strSecond)
        mytext << strSecond;
      if((strFirst || strSecond) && strThird)
        mytext << std::endl << std::endl;
      if(strThird)
        mytext << strThird;

      y += pFontDescription->Height();
      y += WrapText(xSideLeft, y, mytext.str().c_str(),
               Theme.Color(clrMenuTxtFg), Theme.Color(clrBackground),
               pFontDescription,
               xSideRight - xSideLeft - Gap,
               yDescEnd - y,
               false);

    }

    /* show satellite name/source of channel */
    if (channel) {
        int Code = channel->Source();
         cSource *src = Sources.Get(Code);

        if (src) {
            cString sourceText;
            if (cSource::IsCable(Code))
                sourceText = tr("Cable TV");
            else if (cSource::IsSat(Code) && *cSource::ToString(Code))
                sourceText = cString::sprintf("%s: %s", tr("Satellite"),
                                              *cSource::ToString(Code));
            else if (cSource::IsTerr(Code))
                sourceText = tr("Terrestrial TV");

            /*if CI-slot was assigned, show info*/
            if (channel->Ca() == CA_MCLI_UPPER)
                sourceText = cString::sprintf("%s  %s", *sourceText, tr("CI-slot: Upper"));
            else if (channel->Ca() == CA_MCLI_LOWER)
                sourceText = cString::sprintf("%s  %s", *sourceText, tr("CI-slot: Lower"));

            /* draw source name, if available */
            if (*sourceText) {
                y = yDescEnd;
                osd->DrawText(xSideLeft, y, *sourceText,
                              Theme.Color(clrMenuItemNotSelectableFg),
                              clrTransparent, pFontDetailsSubtitle,
                              xSideRight - xSideLeft - Gap);
                y += pFontDetailsSubtitle->Height();
            }// text available
        } // if valid source
    } // if channel

    /* last two lines reserved for side note 'footnote' */
    if (sl) {
            /* leave a small gap between "main" text and showing of directories */
            y += pFontDetailsSubtitle->Height()/2;
        for (int i=0;
             i < sl->Size() && y < yBodyBottom - pFontDetailsTitle->Height();
             ++i) {
            osd->DrawText(xSideLeft, y, sl->At(i),
                          Theme.Color(clrMenuItemNotSelectableFg),
                          clrTransparent, pFontDetailsSubtitle,
                          xSideRight - xSideLeft - Gap);
            y += pFontDetailsSubtitle->Height();
        } // for
    } // if

  }



void cSkinReelDisplayMenu::ClearSideNote() {

    if (enableSideNote) {
        ClearBg(osd,
                xBodyLeft, yBodyTop,
                xBodyLeft, yBodyTop,
                xBodyLeft+IMGMENUWIDTH, yBodyBottom);

        // Draw a lighter rectangle too ?
    }
}


void cSkinReelDisplayMenu::DrawReelBanner() {

    int yMenuMiddle = yBodyTop + IMGMENUTOPHEIGHT; //top picture
    int yMenuBottom = yBodyTop + IMGMENUTOPHEIGHT +IMGMENUHEIGHT; //top + middle

    osd->DrawImage(imgMenuTitleBottom, xBodyLeft-Roundness, yMenuBottom, false);
    osd->DrawImage(imgMenuTitleMiddle, xBodyLeft-Roundness, yMenuMiddle, false);
    osd->DrawImage(imgMenuTitleTop, xBodyLeft-Roundness, yBodyTop, false);

} // DrawReelBanner()

void cSkinReelDisplayMenu::EmptySideNote() {

    if (!enableSideNote) return;

    /* clear side note area*/
    ClearSideNote();

    /** draw background */
    // lighter background
     /*osd->DrawRectangle(Roundness, yBodyTop,
                       xBodyLeft+IMGMENUWIDTH, yBodyBottom,
                       clrRed); */
     int yMenuMiddle = yBodyTop + IMGMENUTOPHEIGHT; //top picture
     int yMenuBottom = yBodyTop + IMGMENUTOPHEIGHT +IMGMENUHEIGHT; //top + middle

#if 0
     DrawUnbufferedImage(osd, "banner_empty_bottom.png", xBodyLeft- Roundness, yMenuBottom, false);
     DrawUnbufferedImage(osd, "banner_empty_middle.png", xBodyLeft- Roundness, yMenuMiddle, false);
     DrawUnbufferedImage(osd, "banner_empty_top.png",    xBodyLeft- Roundness, yBodyTop,    false);
#else
     osd->DrawImage(imgEmptyBannerBottom, xBodyLeft- Roundness, yMenuBottom, false);
     osd->DrawImage(imgEmptyBannerMiddle, xBodyLeft- Roundness, yMenuMiddle, false);
     osd->DrawImage(imgEmptyBannerTop, xBodyLeft- Roundness, yBodyTop, false);
#endif

} // EmptySideNote()


void cSkinReelDisplayMenu::EnableSideNote(bool On) {

    enableSideNote = On;

    if (On) {
        /* side note: same width as 'menu banners' ie IMGMENUWIDTH */
        xItemLeft = xBodyLeft + IMGMENUWIDTH
                + (ReelConfig.showMarker ? lineHeight : ListHBorder);
    }  // if
    else {
        if (MenuType == menuNormal) {
            /* normal values */
            xItemLeft = xBodyLeft +
                    (ReelConfig.showMarker ? lineHeight : ListHBorder);
        } // if
    } // else

} // EnableSideNote()

#endif /* APIVERSNUM >= 10718 && REELVDR */



void cSkinReelDisplayMenu::CreateTitleVector() {
  std::fstream file;
  MenuTypeStruct tempMenuType;
  std::string strBuff;
  int index;
  struct stat fstats;

  /* only reread file if it has changed */
  if (stat(FILENAMEMENULIST, &fstats)==0 && fstats.st_mtime != old_menulist_mtime) {
   old_menulist_mtime = fstats.st_mtime;

   file.open(FILENAMEMENULIST, std::ios::in);

   MenuListWildcard.clear();
   MenuList.clear();

   if (file) {
    getline(file, strBuff);
    while (!file.eof()) {
      tempMenuType.Title = "";
      tempMenuType.Type = menuNormal;
      tempMenuType.ImgNumber = false;
      tempMenuType.UpperCase = false;
      tempMenuType.Buttons = false;
      tempMenuType.SmallButtons = false;

      if(strBuff.find("#") != 0) /* Skip the comments */
        if(strBuff.find(";") != std::string::npos) { /* Skip the faulty lines (missing ;-seperator) */
          index = strBuff.find(";", 0); /* find the separator */

          tempMenuType.Title.assign(strBuff, 0, index);
          strBuff = strBuff.substr(index+1).c_str();

          index = strBuff.find("#", 0); /* find the comment, which will be removed */
          if (index != -1)
            strBuff.resize(index); /* cutoff the comment */

          if (strBuff.find(";smallbuttons", 0) != std::string::npos) {
            tempMenuType.Buttons = false;
            tempMenuType.SmallButtons = true;
            tempMenuType.ImgNumber = true;
          } else if (strBuff.find(";buttons", 0) != std::string::npos) {
            tempMenuType.Buttons = true;
            tempMenuType.ImgNumber = true;
          } else {
            tempMenuType.Buttons = false;
            tempMenuType.ImgNumber = false;
          }

          if (strBuff.find(";imgnum", 0) != std::string::npos)
            tempMenuType.ImgNumber = true;
          else
            tempMenuType.ImgNumber = false;

          if (strBuff.find(";uppercase", 0) != std::string::npos)
            tempMenuType.UpperCase = true;
          else
            tempMenuType.UpperCase = false;

          if (strBuff.find("menunormal") == 0)
            tempMenuType.Type = menuMenuNormal;
          else if (strBuff.find("menucentered") == 0)
            tempMenuType.Type = menuMenuCentered;
          else if (strBuff.find("setupnormal") == 0)
            tempMenuType.Type = menuSetupNormal;
          else if (strBuff.find("setupcentered") == 0)
            tempMenuType.Type = menuSetupCentered;
          else if (strBuff.find("help") == 0)
            tempMenuType.Type = menuHelp;
          else if (strBuff.find("bouquets") == 0)
            tempMenuType.Type = menuBouquets;
          else if (strBuff.find("poweroff") == 0) {
            tempMenuType.ImgNumber = true;
            tempMenuType.SmallButtons = true;
            tempMenuType.Type = menuPowerOff;
          } else if (strBuff.find("image") == 0)
            tempMenuType.Type = menuImage;
          else if (strBuff.find("internetradio") == 0) {
            tempMenuType.SmallButtons = true;
            tempMenuType.Type = menuInternetradio;
          } else if (strBuff.find("musicpictures") == 0) {
            tempMenuType.Type = menuMusicPictures;
            tempMenuType.ImgNumber = true;
            tempMenuType.Buttons = true;
          } else if (strBuff.find("music") == 0)
            tempMenuType.Type = menuMusic;
          else if (strBuff.find("video") == 0)
            tempMenuType.Type = menuVideo;
          else if (strBuff.find("tvradio") == 0) {
            tempMenuType.Type = menuTvRadio;
            tempMenuType.ImgNumber = true;
            tempMenuType.Buttons = true;
          } else if (strBuff.find("filmsdvd") == 0) {
            tempMenuType.Type = menuFilmsDvd;
            tempMenuType.ImgNumber = true;
            tempMenuType.Buttons = true;
          } else if (strBuff.find("internetextras") == 0) {
            tempMenuType.Type = menuInternetExtras;
            tempMenuType.ImgNumber = true;
            tempMenuType.Buttons = true;
          } else if (strBuff.find("optsoftware") == 0) {
            tempMenuType.Type = menuOptSoftware;
            tempMenuType.ImgNumber = true;
            tempMenuType.Buttons = true;
          } else if (strBuff.find("multifeed") == 0)
            tempMenuType.Type = menuMultifeed;
          else if (strBuff.find("systemsettings") == 0) {
            tempMenuType.SmallButtons = true;
            tempMenuType.Type = menuSystemSettings;
          } else if (strBuff.find("menugimmick") == 0)
            tempMenuType.Type = menuGimmick;
          else
            tempMenuType.Type = menuNormal; /* every menu without types are shown as normal */

          if(tempMenuType.Title.find("*") == tempMenuType.Title.length() - 1) {
            tempMenuType.Title.resize(tempMenuType.Title.length() - 1);
            MenuListWildcard.push_back(tempMenuType);
          } else
            MenuList.push_back(tempMenuType);
        }
      getline(file, strBuff);
    }
   } else
      debug("Error: Couldn't load the file %s", FILENAMEMENULIST);

   file.close();
  }
}

/** Get the MenuType of a menu according to the title-string */
int cSkinReelDisplayMenu::GetMenuType(std::string Title) {
  unsigned int i = 0;

  /* First try to detect Menutype by checking if Keyword == Title */
  while (i < MenuList.size()) {
    if(Title == trVDR(MenuList.at(i).Title.c_str()))
      return MenuList.at(i).Type;
    ++i;
  }

  /* Not found? Then try to find Keyword (only the ones with wildcard) in Title */
  i = 0;
  while (i < MenuListWildcard.size()) {
    if(Title.find(trVDR(MenuListWildcard.at(i).Title.c_str())) == 0 )
      return MenuListWildcard.at(i).Type;
    ++i;
  }
  debug("no menuType found. returning menuNormal");
  return menuNormal;
}

bool cSkinReelDisplayMenu::UseButtons(std::string Title) {
  unsigned int i = 0;
  while (i < MenuList.size()) {
    if(Title == trVDR(MenuList.at(i).Title.c_str()))
      return MenuList.at(i).Buttons;
    ++i;
  }

  i = 0;
  while (i < MenuListWildcard.size()) {
    if(Title.find(trVDR(MenuListWildcard.at(i).Title.c_str())) == 0 )
      return MenuListWildcard.at(i).Buttons;
    ++i;
  }
  return false;
}

bool cSkinReelDisplayMenu::UseSmallButtons(std::string Title) {
  unsigned int i = 0;
  while (i < MenuList.size()) {
    if(Title == trVDR(MenuList.at(i).Title.c_str()))
      return MenuList.at(i).SmallButtons;
    ++i;
  }

  i = 0;
  while (i < MenuListWildcard.size()) {
    if(Title.find(trVDR(MenuListWildcard.at(i).Title.c_str())) == 0 )
      return MenuListWildcard.at(i).SmallButtons;
    ++i;
  }
  return false;
}

bool cSkinReelDisplayMenu::UseImageNumber(std::string Title) {
  unsigned int i = 0;
  while (i < MenuList.size()) {
    if(Title == trVDR(MenuList.at(i).Title.c_str()))
      return MenuList.at(i).ImgNumber;
    ++i;
  }

  i = 0;
  while (i < MenuListWildcard.size()) {
    if(Title.find(trVDR(MenuListWildcard.at(i).Title.c_str())) == 0 )
      return MenuListWildcard.at(i).ImgNumber;
    ++i;
  }

  return false;
}

/**
 * returns "true" if the title should be converted to
 * upper case according to the menulist-file
 */
bool cSkinReelDisplayMenu::IsUpperCase(std::string Title) {
  unsigned int i = 0;
  while (i < MenuList.size()) {
    if(Title == trVDR(MenuList.at(i).Title.c_str()))
      return MenuList.at(i).UpperCase;
    ++i;
  }

  i=0;
  while (i < MenuListWildcard.size()) {
    if(Title.find(trVDR(MenuListWildcard.at(i).Title.c_str())) == 0 )
      return MenuListWildcard.at(i).UpperCase;
    ++i;
  }

  return false;
}

void cSkinReelDisplayMenu::SetScrollbar(void) {
  /* check if scrollbar is needed */

  if (textScroller.CanScroll() && strcmp(strTitle, trVDR("Main Menu"))) {
    int yt = textScroller.Top();
    int yb = yt + textScroller.Height();
    int st = yt + ScrollbarHeight;
    int sb = yb - ScrollbarHeight;
    int tt = st + (sb - st) * textScroller.Offset() / textScroller.Total();
    int tb = tt + (sb - st) * textScroller.Shown() / textScroller.Total();
    int xl = textScroller.Width() + xItemLeft;
    /* arrow up */
    DrawUnbufferedImage(osd, "scrollbar_top.png", xl + ScrollbarWidth, yt + SmallGap, true);

    /* draw background of scrollbar */
    DrawUnbufferedImage(osd, "scrollbar_inactive_x.png", xl + ScrollbarWidth, st, true, 1, sb-st);

    /* draw visible area of scrollbar */
    DrawUnbufferedImage(osd, "scrollbar_active_x.png", xl + ScrollbarWidth, tt, true, 1, tb-tt);

    /* arrow down */
    DrawUnbufferedImage(osd, "scrollbar_bottom.png", xl + ScrollbarWidth, yb-19, true);
  }
}

void cSkinReelDisplayMenu::Scroll(bool Up, bool Page) {
  textScroller.Scroll(Up, Page);
  SetScrollbar();
}

inline int cSkinReelDisplayMenu::MaxItems(void) {
  /* max number of items */
  int mx = ( yBodyBottom - yItemTop ) / lineHeight ; /* to leave a line gap at the end */
  return  mx;
}

void cSkinReelDisplayMenu::Clear(void) {

#ifdef DEBUG_OSD
  std::cout << "CLEAR" << std::endl;
#endif

  textScroller.Reset();

  if (strcmp(strTheme, Theme.Name()) != 0) {
    free(strTheme);
    strTheme = strdup(Theme.Name());
    SetupAreas();
  } else {
    /** clear the complete right part, there could have been a big scrollbar from the previous menu */
    ClearBg(osd, xDateRight - Gap - ScrollbarWidth-5, yBodyTop, xDateRight - Gap - ScrollbarWidth-5, yBodyTop, xDateRight - Gap, yBodyBottom );

    /* clear items area */
    if(!itemsBgSaved) {
        osd->DrawImage(imgMenuBodyUpperPart, xBodyLeft, yBodyTop, false, xInfoRight - xBodyLeft - 1, 1);
        osd->DrawImage(imgMenuBodyLowerPart, xBodyLeft, yBodyTop+205, false, xInfoRight - xBodyLeft - 1, 1);
        osd->SaveRegion(xBodyLeft, yBodyTop, xInfoRight - xBodyLeft - 1, yBodyTop+410);
        itemsBgSaved = true;
    } else
        osd->RestoreRegion();
    alreadyCleared = true;
  }

  // thumbnails have been cleared
  for (int i=0; i<NR_UNBUFFERED_SLOTS; ++i)
      thumbCleared[i] = 1;
}

void cSkinReelDisplayMenu::DrawTitleImage() {
    /* clear the space for drawing */
    switch (MenuType) {
        case menuMenuNormal:
        case menuMenuCentered:
        case menuSetupNormal:
        case menuSetupCentered:
        case menuHelp:
        case menuBouquets:
        case menuImage:
        case menuMusic:
        case menuVideo:
        case menuMultifeed:
        case menuTvRadio:
        case menuFilmsDvd:
        case menuInternetExtras:
        case menuMusicPictures:
        case menuOptSoftware:
        case menuGimmick:
        case menuPowerOff:
        case menuSystemSettings:
        case menuEmptyBanner:
        case menuHalfEmptyBanner:
            /* draw background */
#if 0
            osd->DrawImage(imgMenuHighlightUpperPart, xBodyLeft-10, yBodyTop, false, xBodyLeft+IMGMENUWIDTH-19, 1);
            osd->DrawImage(imgMenuHighlightLowerPart, xBodyLeft-10, yBodyTop+205, false, xBodyLeft+IMGMENUWIDTH-19, 1);
#endif
            withTitleImage = true;
            break;
        default:
            withTitleImage = false;
            break;
    }

    /* draw the shadowed border on the left */
    if (withTitleImage) {
       osd->DrawImage(imgMenuHighlightLeftUpperPart, 0, yBodyTop, false);
       osd->DrawImage(imgMenuHighlightLeftLowerPart, 0, yBodyTop + 205, false);
    } else {
       osd->DrawImage(imgMenuBodyLeftUpperPart, 0, yBodyTop, false);
       osd->DrawImage(imgMenuBodyLeftLowerPart, 0, yBodyTop + 205, false);
    }

    int yMenuMiddle = yBodyTop + IMGMENUTOPHEIGHT; //top picture
    int yMenuBottom = yBodyTop + IMGMENUTOPHEIGHT +IMGMENUHEIGHT; //top + middle
    /* draw Title Image */
    switch(MenuType) {
        case menuMenuNormal:
        case menuMenuCentered:
        case menuGimmick:
            osd->DrawImage(imgMenuTitleBottom, xBodyLeft-10, yMenuBottom, false);
            osd->DrawImage(imgMenuTitleMiddle, xBodyLeft-10, yMenuMiddle, false);
            osd->DrawImage(imgMenuTitleTop, xBodyLeft-10, yBodyTop, false);
            /* TB: dependencies on the font size destroy the alignment of the buttons */
            xItemLeft = 242; //TB = xBodyLeft + IMGMENUWIDTH + (xBodyRight - xBodyLeft  )/2 - pFontList->Width("D") * 10;//(ReelConfig.showMarker ? lineHeight : ListHBorder);
#if VDRVERSNUM >= 10716
            // Allow to draw little more to the left or menuGimmick icons may be out of the area
            if (MenuType == menuGimmick)
                xItemRight = xInfoRight /*- ScrollbarWidth */- ListHBorder;
            else
                xItemRight = xInfoRight - ScrollbarWidth - ListHBorder;
#endif
            break;
        case menuMusicPictures:
            DrawUnbufferedImage(osd, "banner_musicpictures_bottom.png", xBodyLeft-10, yMenuBottom, false);
            DrawUnbufferedImage(osd, "banner_musicpictures_middle.png", xBodyLeft-10, yMenuMiddle, false);
            DrawUnbufferedImage(osd, "banner_musicpictures_top.png", xBodyLeft-10, yBodyTop, false);
            xItemLeft = xBodyLeft + IMGMENUWIDTH + (xBodyRight - xBodyLeft  )/2 - pFontList->Width("D") * 10;//(ReelConfig.showMarker ? lineHeight : ListHBorder);
            break;
        case menuTvRadio:
            DrawUnbufferedImage(osd, "banner_tvradio_bottom.png", xBodyLeft-10, yMenuBottom, false);
            DrawUnbufferedImage(osd, "banner_tvradio_middle.png", xBodyLeft-10, yMenuMiddle, false);
            DrawUnbufferedImage(osd, "banner_tvradio_top.png", xBodyLeft-10, yBodyTop, false);
            xItemLeft = xBodyLeft + IMGMENUWIDTH + (xBodyRight - xBodyLeft  )/2 - pFontList->Width("D") * 10;//(ReelConfig.showMarker ? lineHeight : ListHBorder);
            break;
        case menuFilmsDvd:
            DrawUnbufferedImage(osd, "banner_dvdmovies_bottom.png", xBodyLeft-10, yMenuBottom, false);
            DrawUnbufferedImage(osd, "banner_dvdmovies_middle.png", xBodyLeft-10, yMenuMiddle, false);
            DrawUnbufferedImage(osd, "banner_dvdmovies_top.png", xBodyLeft-10, yBodyTop, false);
            xItemLeft = xBodyLeft + IMGMENUWIDTH + (xBodyRight - xBodyLeft  )/2 - pFontList->Width("D") * 10;//(ReelConfig.showMarker ? lineHeight : ListHBorder);
            break;
        case menuInternetExtras:
            DrawUnbufferedImage(osd, "banner_internetextras_bottom.png", xBodyLeft-10, yMenuBottom, false);
            DrawUnbufferedImage(osd, "banner_internetextras_middle.png", xBodyLeft-10, yMenuMiddle, false);
            DrawUnbufferedImage(osd, "banner_internetextras_top.png", xBodyLeft-10, yBodyTop, false);
            xItemLeft = xBodyLeft + IMGMENUWIDTH + (xBodyRight - xBodyLeft  )/2 - pFontList->Width("D") * 10;//(ReelConfig.showMarker ? lineHeight : ListHBorder);
            break;
        case menuOptSoftware:
            DrawUnbufferedImage(osd, "banner_software_bottom.png", xBodyLeft-10, yMenuBottom, false);
            DrawUnbufferedImage(osd, "banner_software_middle.png", xBodyLeft-10, yMenuMiddle, false);
            DrawUnbufferedImage(osd, "banner_software_top.png", xBodyLeft-10, yBodyTop, false);
            xItemLeft = xBodyLeft + IMGMENUWIDTH + (xBodyRight - xBodyLeft  )/2 - pFontList->Width("D") * 10;//(ReelConfig.showMarker ? lineHeight : ListHBorder);
            break;
        case menuSystemSettings:
        case menuSetupNormal:
        case menuSetupCentered:
            DrawUnbufferedImage(osd, "banner_setup_bottom.png", xBodyLeft-10, yMenuBottom, false);
            DrawUnbufferedImage(osd, "banner_setup_middle.png", xBodyLeft-10, yMenuMiddle, false);
            DrawUnbufferedImage(osd, "banner_setup_top.png", xBodyLeft-10, yBodyTop, false);
            xItemLeft = xBodyLeft + IMGMENUWIDTH + (xBodyRight - xBodyLeft  )/2 - pFontList->Width("D") * 10;
            break;
        case menuHelp:
            DrawUnbufferedImage(osd, "banner_help_bottom.png", xBodyLeft-10, yMenuBottom, false);
            DrawUnbufferedImage(osd, "banner_help_middle.png", xBodyLeft-10, yMenuMiddle, false);
            DrawUnbufferedImage(osd, "banner_help_top.png", xBodyLeft-10, yBodyTop, false);
            break;
        case menuBouquets:
            DrawUnbufferedImage(osd, "banner_bouquets_bottom.png", xBodyLeft-10, yMenuBottom, false);
            DrawUnbufferedImage(osd, "banner_bouquets_middle.png", xBodyLeft-10, yMenuMiddle, false);
            DrawUnbufferedImage(osd, "banner_bouquets_top.png", xBodyLeft-10, yBodyTop, false);
            break;
        case menuImage:
            DrawUnbufferedImage(osd, "banner_images_bottom.png", xBodyLeft-10, yMenuBottom, false);
            DrawUnbufferedImage(osd, "banner_images_middle.png", xBodyLeft-10, yMenuMiddle, false);
            DrawUnbufferedImage(osd, "banner_images_top.png", xBodyLeft-10, yBodyTop, false);
            break;
        case menuInternetradio:
        case menuMusic:
            osd->DrawImage(imgMusicBodyBottom, xBodyLeft-10, yMenuBottom, false);
            osd->DrawImage(imgMusicBodyMiddle, xBodyLeft-10, yMenuMiddle, false);
            osd->DrawImage(imgMusicBodyTop, xBodyLeft-10, yBodyTop, false);
            xItemRight = xInfoRight - ScrollbarWidth - ListHBorder;
            break;
        case menuVideo:
            DrawUnbufferedImage(osd, "banner_movies_bottom.png", xBodyLeft-10, yMenuBottom, false);
            DrawUnbufferedImage(osd, "banner_movies_middle.png", xBodyLeft-10, yMenuMiddle, false);
            DrawUnbufferedImage(osd, "banner_movies_top.png", xBodyLeft-10, yBodyTop, false);
            break;
        case menuMultifeed:
            DrawUnbufferedImage(osd, "banner_multifeed_bottom.png", xBodyLeft-10, yMenuBottom, false);
            DrawUnbufferedImage(osd, "banner_multifeed_middle.png", xBodyLeft-10, yMenuMiddle, false);
            DrawUnbufferedImage(osd, "banner_multifeed_top.png", xBodyLeft-10, yBodyTop, false);
            break;
        case menuPowerOff:
            DrawUnbufferedImage(osd, "banner_standby_bottom.png", xBodyLeft-10, yMenuBottom, false);
            DrawUnbufferedImage(osd, "banner_standby_middle.png", xBodyLeft-10, yMenuMiddle, false);
            DrawUnbufferedImage(osd, "banner_standby_top.png", xBodyLeft-10, yBodyTop, false);
            break;
        case menuEmptyBanner:
#if 0
            DrawUnbufferedImage(osd, "banner_empty_bottom.png", xBodyLeft-10, yMenuBottom, false);
            DrawUnbufferedImage(osd, "banner_empty_middle.png", xBodyLeft-10, yMenuMiddle, false);
            DrawUnbufferedImage(osd, "banner_empty_top.png", xBodyLeft-10, yBodyTop, false);
#else
            osd->DrawImage(imgEmptyBannerBottom, xBodyLeft- Roundness, yMenuBottom, false);
            osd->DrawImage(imgEmptyBannerMiddle, xBodyLeft- Roundness, yMenuMiddle, false);
            osd->DrawImage(imgEmptyBannerTop, xBodyLeft- Roundness, yBodyTop, false);
#endif
            break;
        case menuHalfEmptyBanner:
            DrawUnbufferedImage(osd, "banner_halfempty_bottom.png", xBodyLeft-10, yMenuBottom, false);
            DrawUnbufferedImage(osd, "banner_halfempty_middle.png", xBodyLeft-10, yMenuMiddle, false);
            DrawUnbufferedImage(osd, "banner_halfempty_top.png", xBodyLeft-10, yBodyTop, false);
            break;
        default:
            xItemLeft = xBodyLeft + (ReelConfig.showMarker ? lineHeight : ListHBorder);
            xItemRight = xInfoRight - ScrollbarWidth - ListHBorder;
            break;
    }
}

void cSkinReelDisplayMenu::SetTitle(const char *Title) {

#ifdef DEBUG_OSD
  printf("SetTitle: \"%s\"\n", Title);
#endif

  bool fTitleChanged = false;
  if (Title && strTitle) {
    if (strcmp(Title, strTitle) != 0) {
      fTitleChanged = true;
      free(strTitle);
      strTitle = strdup(Title);
    }
  } else {
    free(strTitle);
    if (Title)
      strTitle = strdup(Title);
    else
      strTitle = NULL;
  }

  static char strTitlePrefix[256];
  snprintf(strTitlePrefix, 256, "%s  -  ", trVDR("VDR"));

  if ((Title == NULL) || (isMainMenu && strncmp(strTitlePrefix, Title, strlen(strTitlePrefix)) == 0)) {
      DrawTitle(Title);
  } else {
    bool old_isMainMenu = isMainMenu;

    if (strTitle == NULL || strncmp(strTitlePrefix, strTitle, strlen(strTitlePrefix)) == 0)
      isMainMenu = true;
    else
      isMainMenu = false;

    if (!fSetupAreasDone || old_isMainMenu != isMainMenu)
      SetupAreas();
    else
      DrawTitle(Title);
  }

  isNumberImage = UseImageNumber(strTitle);
  MenuType = GetMenuType(strTitle);
  isButtoned = UseButtons(strTitle);
  isSmallButtoned = UseSmallButtons(strTitle);

#define HIDDENSTRING "menunormalhidden$"
#define MP3COVERSTRING "SHOWINGMP3COVER$"
  if (Title) {
     char* pos = (char *) strstr(Title, HIDDENSTRING);
     if (pos > 0) {
        MenuType = menuMenuNormal;
        DrawTitle(pos+strlen(HIDDENSTRING));
     }
     pos = (char *) strstr(Title, MP3COVERSTRING);
     if (pos > 0) {
        MenuType = menuEmptyBanner;
        DrawTitle(pos + strlen(MP3COVERSTRING));
     } else if (ID3Tags.size()) { /** no mp3-cover, but id3-infos */
        MenuType = menuHalfEmptyBanner;
     }

  }

  isCAPS = IsUpperCase(strTitle);
  pFontList = ReelConfig.GetFont(FONT_LISTITEM, pFontList);

  if(isButtoned)
      lineHeight = std::max(pFontList->Height() , 57);
  else if (isSmallButtoned)
      lineHeight = std::max(pFontList->Height() , 40);
  else if (isNumberImage)
      if(MenuType == menuGimmick)
         lineHeight = std::max(pFontList->Height() , 57);
      else
         lineHeight = std::max(pFontList->Height() , 40);
  else
      lineHeight = pFontList->Height() ; //change

  /* Draw menu/setup/help according to menutype */
  DrawTitleImage();
#if REELVDR && APIVERSNUM >= 10718
  if (MenuType == menuNormal)
      EmptySideNote();
#endif


  if (MenuType == menuMenuNormal || MenuType == menuSetupNormal || MenuType == menuInternetradio ||
      MenuType == menuHelp || MenuType == menuBouquets || MenuType == menuSystemSettings ||
      MenuType == menuImage || MenuType == menuMusic || MenuType == menuOptSoftware || MenuType == menuHalfEmptyBanner ||
      MenuType == menuVideo || MenuType == menuMultifeed || MenuType == menuPowerOff || MenuType == menuEmptyBanner) {
          xItemLeft = xBodyLeft + IMGMENUWIDTH + (ReelConfig.showMarker ? lineHeight : ListHBorder);
#ifndef REELVDR
         /* if an image is drawn we must decrease the size of the first tab */
         /* devide IMGMENUWIDTH by 12 - 12 is the expected average character width in SetTabs in vdr/skins.c
          * That's strange and should by changed ... */
         int t1w = (Tab(1)/12)-(IMGMENUWIDTH/12) > 0 ? (Tab(1)/12)-(IMGMENUWIDTH/12) : Tab(1)/12;
         SetTabs(t1w, Tab(2), Tab(3), Tab(4), Tab(5));
#endif
  }

  if (MenuType == menuTvRadio || MenuType == menuOptSoftware || MenuType == menuMusicPictures ||
      MenuType == menuFilmsDvd || MenuType == menuInternetExtras)
    yItemTop = yBodyTop + lineHeight; // The items is centered in the menu
  else if(isButtoned)
    yItemTop = yBodyTop + 15;
  else if((MenuType == menuMenuCentered) || (MenuType == menuSetupCentered) || (MenuType == menuSystemSettings) || (MenuType == menuGimmick) || (MenuType == menuPowerOff))
    yItemTop = yBodyTop + lineHeight; // The items is centered in the menu
  else
    yItemTop = yBodyTop + 15; // lineHeight ; // leave a line blank

  free(strLastDate);
  strLastDate = NULL;
}

///< absolute path of the thumbnail is expected
///< draws the thumbnail only if the side-Images are present: like menu/setup/Help etc.
bool cSkinReelDisplayMenu::DrawThumbnail(const char* thumbnail_filename) {
    if ( !withTitleImage ) /* if no Title image then do not draw thumbnail */
        return false;

    if (thumbnail_filename == NULL) { /* if filename == NULL, clear thumbnail by drawing the Title-Image again */
        DrawTitleImage();
        return true;
    }

    if ( access(thumbnail_filename, R_OK) != 0 ) { /* if file not readable */
        esyslog("Could not access thumbnail file '%s'", thumbnail_filename);
        return false;
    }

    int leftTop_x = 43;
    int top_y = yBodyTop + 20;

    /* Draw thumbnail */
    osd->SetImagePath(imgUnbufferedEnum, thumbnail_filename);
    osd->DrawImage(imgUnbufferedEnum, leftTop_x, top_y, true, 1, 1);

    return true;
}

void cSkinReelDisplayMenu::DrawTitle(const char *Title) {

#ifdef DEBUG_OSD
  printf("TITLE: \"%s\"\n", Title);
#endif
  LittleIcons.clear();

  /* draw titlebar */
  osd->DrawImage(imgMenuHeaderLeft, 0, 0, false);
  osd->DrawImage(imgMenuHeaderCenterX, Roundness*2, 0, false, xDateRight - 2*Roundness, 1);
  osd->DrawImage(imgMenuHeaderRight, xDateRight - Roundness, 0, false);

  if (Title) {
    pFontOsdTitle = ReelConfig.GetFont(FONT_OSDTITLE, pFontOsdTitle); //TODO? get current font which might have been patched meanwhile
    int y = yTitleTop + (yTitleBottom - yTitleTop - (pFontOsdTitle ? pFontOsdTitle->Height() : 22)) / 2 /*+ 5*/;
#if 0 //TB: we don't have progressbars in the title any longer
    int pg_start, pg_end, pg_count, pg_total;
    pg_total = pgbar(Title, pg_start, pg_end, pg_count);

    /* no progress bar found in Title */
    if (pg_total == 0) {
#endif
       osd->DrawText(/*xTitleLeft +*/10 + Roundness, y, Title,
                  Theme.Color(clrTitleFg), clrTransparent, pFontOsdTitle,
                  xTitleRight - xTitleLeft - Roundness - 3, yTitleBottom - y);
       return;
#if 0 //TB: we don't have progressbars in the title any longer
    } else {
       /* progress bar found in Title */
       static char title_array[512];
       int pg_length = 50;
       int pg_thickness = 6;
       strncpy(title_array, Title, pg_start);
       title_array[pg_start] = '\0';

       /* first part of Title */
       int x_coord = /*xTitleLeft +*/10 + Roundness;
       osd->DrawText(x_coord, y, title_array, Theme.Color(clrTitleFg), clrTransparent,
                  pFontOsdTitle, xDateLeft -Roundness - x_coord , yTitleBottom - y);

       x_coord += pFontOsdTitle->Width(title_array);
       x_coord += 2*Gap;

       int py0 = y + ( yTitleBottom - y ) / 2 - pg_thickness; /* top y */
       int py1 = py0 + pg_thickness;                          /* bottom y */

       /* Draw Progress bar */
       osd->DrawRectangle(x_coord, py0 , x_coord + pg_length, py1, Theme.Color(clrProgressBarBg));
       osd->DrawRectangle(x_coord, py0 , x_coord + (int)( pg_length* pg_count*1.0/pg_total ), py1, Theme.Color(clrProgressBarFg));

       x_coord += pg_length + 2*Gap;

       strncpy(title_array, Title+pg_end+1, strlen(Title) - pg_end);
       title_array[strlen(Title) - pg_end] = '\0';

       /* second part of the title */
       osd->DrawText(x_coord, y, title_array, Theme.Color(clrTitleFg), clrTransparent,
                  pFontOsdTitle, xDateLeft - Roundness- x_coord , yTitleBottom - y);
    } // end if_else (pg_total==0)
#endif
  }// end if (Title)
}

inline void cSkinReelDisplayMenu::DrawFooter() {
  /* draw footer */
  osd->DrawImage(imgMenuFooterLeft, 0, yFooterTop, false);
  osd->DrawImage(imgMenuFooterCenterX, Roundness+10, yFooterTop, false, xFooterRight - 2*Roundness, 1);
  osd->DrawImage(imgMenuFooterRight, xFooterRight - Roundness, yFooterTop, false);
}

void cSkinReelDisplayMenu::SetButtons(const char *Red, const char *Green, const char *Yellow, const char *Blue) {

  const cFont *pFontHelpKeys = ReelConfig.GetFont(FONT_HELPKEYS);
  int w = (xButtonsRight - xButtonsLeft) / 4;
  int t3 = xButtonsRight - w;
  int t2 = t3 - w;
  int t1 = t2 - w;
  int t0 = t1 - w;/*let the error not accumulate*/ //xButtonsLeft;

  /* draw Footer */
  DrawFooter();

  //TB: TODO: render image and text into one image, do the alpha-blending here on the host instead of on the hd-ext
  //TB: TODO: position the text correctly
  // draw color buttons
  if (Blue) {
    osd->DrawImage(imgButtonBlueX, t3, yButtonsTop, true);
    osd->DrawText(t3+12, yButtonsTop, Blue, Theme.Color(clrButtonBlueFg), 0, pFontHelpKeys, w, yButtonsBottom - yButtonsTop, taCenter);
  } else {
    t2 += w; t1 += w; t0 += w; //TB no blue button - all others switch one step to the right
  }
  if (Yellow) {
    osd->DrawImage(imgButtonYellowX, t2, yButtonsTop, true);
    osd->DrawText(t2+12, yButtonsTop, Yellow, Theme.Color(clrButtonYellowFg), 0, pFontHelpKeys, w, yButtonsBottom - yButtonsTop, taCenter);
  } else {
    t1 += w; t0 += w;
  }
  if (Green) {
  osd->DrawImage(imgButtonGreenX, t1, yButtonsTop, true);
  osd->DrawText(t1+12, yButtonsTop, Green, Theme.Color(clrButtonGreenFg), 0, pFontHelpKeys, w, yButtonsBottom - yButtonsTop, taCenter);
  }  else {
    t0 += w;
  }
  if (Red) {
    osd->DrawImage(imgButtonRedX, t0, yButtonsTop, true);
    osd->DrawText(t0+12, yButtonsTop, Red, Theme.Color(clrButtonRedFg), 0, pFontHelpKeys, w, yButtonsBottom - yButtonsTop, taCenter);
  }
}

void cSkinReelDisplayMenu::SetKeys(const eKeys *Keys) {
#if REELVDR && APIVERSNUM >= 10718
  osd->DrawImage(HasKey(Keys, kInfo) ? imgIconHelpActive : imgIconHelpInactive, xDateRight - 27, yButtonsTop, true);
#else
  osd->DrawImage(imgIconHelpInactive, xDateRight - 27, yButtonsTop, true);
#endif
} // cSkinReelDisplayMenu::SetKeys

void cSkinReelDisplayMenu::SetMessage(eMessageType Type, const char *Text) {
  /* check if message */
#ifdef DEBUG_OSD
    std::cout << "MESSAGE: " << Text << std::endl;
#endif
    if (Text) {
        if (strcmp(Text, "(?)") == 0) {
            osd->DrawImage(imgMenuMessageLeft, 0, yMessageTop, false);
            osd->DrawImage(imgMenuMessageCenterX, /*xMessageLeft +*/ Roundness, yMessageTop, false, xMessageRight-xMessageLeft/* - 2*Roundness*/,1);
            osd->DrawImage(imgMenuMessageRight, xMessageRight-Roundness, yMessageTop, false);
            osd->DrawImage(imgIconHelpActive, xDateRight - 27, yButtonsTop, true);
        } else {
            /* Clear Messagearea & Remove (?)-Icon */
            osd->DrawImage(imgMenuMessageLeft, 0, yMessageTop, false);
            osd->DrawImage(imgMenuMessageCenterX, /*xMessageLeft +*/ Roundness, yMessageTop, false, xMessageRight-xMessageLeft/* - 2*Roundness*/,1);
            osd->DrawImage(imgMenuMessageRight, xMessageRight-Roundness, yMessageTop, false);
#if !(REELVDR && APIVERSNUM >= 10718) // do not clear help icon, it is set and cleared by SetKeys()
            osd->DrawImage(imgIconHelpInactive, xDateRight - 27, yButtonsTop, true);
#endif

            // save osd region
            //if (nMessagesShown == 0)
            //  osd->SaveRegion(xMessageLeft, yMessageTop, xMessageRight, yMessageBottom); // cannot save regions

            if (Text) {
            /* draw message */
                int imgMessageIcon;
                tColor  colorfg = 0xFFFFFFFF;
                switch(Type) {
                    case mtStatus:  imgMessageIcon = imgIconMessageInfo;     break;
                    case mtWarning: imgMessageIcon = imgIconMessageWarning;  break;
                    case mtError:   imgMessageIcon = imgIconMessageError;    break;
                    default:        imgMessageIcon = imgIconMessageNeutral;  break;
                } //end switch

                const cFont *pFontMessage = ReelConfig.GetFont(FONT_MESSAGE);
                int MessageWidth = pFontMessage->Width(Text);
                osd->DrawImage(imgMessageIcon, xMessageLeft + (xMessageRight-xMessageLeft - MessageWidth)/2 - 30 - 2*Gap, yMessageTop - Gap+2, true, 1, 1);
                int _xMessageStart_ = xMessageLeft + (xMessageRight-xMessageLeft - MessageWidth)/2;
                int MessageHeight = pFontMessage->Height(Text) ;

                /*center the message, vertically*/
                int yText = yMessageTop + (yMessageBottom - yMessageTop - MessageHeight) / 2 + 2;


                int pb_start, pb_end, progressCount; // Progress Bar
                int offset=0;
                int pbtotal = pgbar(Text, pb_start, pb_end, progressCount); // get the length of progress bar if any
                if ( pbtotal < 3) { // donot show pbar! [], [ ], [  ] will be displayed as such
                    osd->DrawText( xMessageLeft , yText /*yMessageTop*/, Text, colorfg, clrTransparent, pFontMessage, xMessageRight - xMessageLeft, 0, taCenter );
                } else {
                    char *text1=NULL;
                    if (pb_start > 0) { /* some text before progress bar */
                        text1 = new char[pb_start+1];
                        strncpy(text1,Text, pb_start); // i_start is the index of '['
                        text1[pb_start] = '\0';

                        offset = pFontMessage->Width(text1);
                        osd->DrawText( _xMessageStart_ , yText/*yMessageTop*/, text1, colorfg, clrTransparent, pFontMessage, offset, 0, taCenter );
                        delete[] text1;
                        text1 = NULL;
                    }

                    // draw progress bar
                    int pblength = pbtotal * pFontMessage->Width("|");
                    int pbthickness = (int)( 0.5*pFontMessage->Height("|") );
                    int pbTop = (yMessageBottom + yMessageTop - pbthickness)/2;
                    /* draw progress bar */
                    DrawProgressBar(osd, _xMessageStart_ + offset, pbTop, _xMessageStart_ + offset + progressCount*pblength/pbtotal, _xMessageStart_ + offset + pblength);

                    offset += pblength +  pFontMessage->Width(' ');

                    if (pb_end + 1 < (int)strlen(Text)) { /* display rest of the text */
                        int len = strlen(Text) - pb_end ; /* including '\0' */
                        text1 = new char[len];
                        strncpy(text1, Text+pb_end+1, len-1);
                        text1[len-1] = '\0';
                        osd->DrawText( _xMessageStart_ + offset , yText/*yMessageTop*/, text1, colorfg, clrTransparent, pFontMessage, xMessageRight- ( _xMessageStart_ + offset ),0,taLeft  );
                        delete[] text1;
                    }
                } // end else
            } // end if(Text)
            nMessagesShown = 1; //was: nMessagesShown++;
        }
    } else {
        if (nMessagesShown > 0)
            --nMessagesShown;
        /* restore saved osd region */
        if (nMessagesShown == 0) {
            // osd->RestoreRegion();
            osd->DrawImage(imgMenuMessageLeft, 0, yMessageTop, false);
            osd->DrawImage(imgMenuMessageCenterX, xMessageLeft /*+ Roundness*/, yMessageTop, false, xMessageRight-xMessageLeft /*- 2*Roundness*/,1);
            osd->DrawImage(imgMenuMessageRight, xMessageRight-Roundness, yMessageTop, false);
#if !(REELVDR && APIVERSNUM >= 10718) // do not clear help icon, it is set and cleared by SetKeys()
            osd->DrawImage(imgIconHelpInactive, xDateRight - 27, yButtonsTop, true);
#endif
        }
    }
}

void CutStrToPixelLength(char *s, const cFont * font, const int px_len) {
    if ( !s || *s == '\0') return;

    int len_in_px = font->Width(s);
    int len = strlen(s);

    if (len_in_px <= px_len) return;

    while( len &&  len_in_px > px_len) {
        // remove the last character
        s[--len] = '\0';
        // update string length in pixel
        len_in_px = font->Width(s);
    }
}

inline bool cSkinReelDisplayMenu::HasTabbedText(const char *s, int Tab) {
  if (!s)
    return false;

  const char *b = strchrnul(s, '\t');
  while (*b && Tab-- > 0) {
    b = strchrnul(b + 1, '\t');
  }
  if (!*b)
    return (Tab <= 0) ? true : false;
  return true;
}

/** Clear a line to prevent overwriting */
void ClearBg(cOsd* osd, int btnx, int btny, int btnx0, int btny0, int btnx1, int btny1) {
       //std::cout << "x: " << btnx << " y: " << btny << " x0: " << btnx0 << " y0: " << btny0 << " x1: " << btnx1 << " y1: " << btny1 << std::endl;
       /** Clear the line to prevent overwriting */
#if 1
       if(btny1 <= btny+205) /* completely in the upper part */
         osd->DrawCropImage(imgMenuBodyUpperPart, btnx, btny, btnx0, btny0, btnx1, btny1, false);
       else if (btny0 <= btny+205 && btny1 >= btny+205) { /* between */
         osd->DrawCropImage(imgMenuBodyUpperPart, btnx, btny, btnx0, btny0, btnx1, btny+205, false);
         osd->DrawCropImage(imgMenuBodyLowerPart, btnx, btny+205, btnx0, btny+205, btnx1, btny1, false);
       }
       else /* completely in the lower part */
         osd->DrawCropImage(imgMenuBodyLowerPart, btnx, btny+205, btnx0, btny0, btnx1, btny1, false);
#else
       osd->DrawRectangle(btnx0, btny0, btnx1, btny1, clrRed);
#endif
}

void cSkinReelDisplayMenu::SetItem(const char *Text, int Index, bool Current, bool Selectable) {

#ifdef DEBUG_OSD
  std::cout << "SETITEM Current: " << Current << " Text: \"" << Text << "\"" << std::endl;
#endif

  if (Text == NULL)
    return;

  /* force clearing of background when drawning items on menu */
  alreadyCleared = false;
  
  
  if (MenuType == menuOptSoftware) {
      // Get plugin name derived from description text / title
      PluginName = GetPluginName(Text + 5) /* each menu entry is expanded by Index x like "  x  " */;
  } else {
      PluginName = "";
  }
  
  if(PluginName.size())
  {
     PluginIconFile = "icon_optsoftware_";
     PluginIconFile += PluginName;
     PluginIconFile += ".png";

     bPluginIconFileExists = PluginIconFileExists(PluginIconFile.c_str());
  } else
  {
     PluginIconFile = "";
     bPluginIconFileExists = false;
  }

  if (MenuType == menuGimmick) {
    /** Catch texts for Icon at the bottom of main-menu */
    if (strcasestr(Text, trVDR(" Stop recording ")) != NULL) {
      //printf("STOP recording: %i %s\n", Index, Text);
      LittleIcons[Index] = cLittleIconInfo(STOP_RECORDING_ICON, Current);
      SetMessage( mtStatus, Current?Text:NULL);
      return;
    } else if (strcasestr(Text, tr("background process")) != NULL) {
      //printf("BG process: %i %s\n", Index, Text);
      LittleIcons[Index] = cLittleIconInfo(BACKGROUND_PROCESS_ICON, Current);
      SetMessage( mtStatus, Current?Text:NULL);
      return;
    } else if (strcasestr(Text, trVDR(" Stop replaying")) != NULL) {
      //printf("STOP replay: %i %s\n", Index, Text);
      LittleIcons[Index] =  cLittleIconInfo(STOP_REPLAY_ICON,Current);
      SetMessage( mtStatus, Current?Text:NULL);
      return;
    } else if (strcasestr(Text, tr("Play Disk"))!=NULL) {
      //printf("PLAY media: Mediad: %i %s\n", Index, Text);
      LittleIcons[Index] = cLittleIconInfo(PLAY_MEDIA_ICON, Current);
      SetMessage( mtStatus, Current?Text:NULL);
      return;
    }
  }

  /* select colors */
  tColor ColorFg = Theme.Color(clrMenuItemCurrentFg);
  if (!Current) {
      if (Selectable)
          ColorFg = Theme.Color(clrMenuItemSelectableFg);
      else
          ColorFg = Theme.Color(clrMenuItemNotSelectableFg);
  }

  char *Text_tmp = strdup (Text);

  int y = yItemTop + Index * lineHeight;

  int btnx, btny, btny0, btnx1, btny1;

  if(!alreadyCleared && MenuType == menuGimmick/*&& !Current*/)
    if(Index % 2 == 1) { // every second line - these have their icons on the right side
      if(Current) {
#if VDRVERSNUM < 10716
       btnx = xItemLeft-15;
#else
       btnx = xItemLeft-60;
#endif
       btny = yBodyTop;
       btny0 = y;
       btny1 = y /* + lineHeight */ + 50;
#if VDRVERSNUM < 10716
       btnx1 = xItemRight-62;
#else
       btnx1 = xItemRight-60;
#endif
//#define DEBUG_CLEAR 1
#ifndef DEBUG_CLEAR
       //ClearBg(osd, btnx, btny, btnx, btny0, btnx1, btny1);
       ClearBg(osd, btnx1-25, btny, btnx1-25, btny0-10, btnx1+45, btny1+10);
       ClearBg(osd, btnx1-50, btny, btnx1-50, btny0-7, btnx1-25, btny1+7);
#else
       //osd->DrawRectangle(btnx, btny0, btnx1, btny1, clrWhite);
       osd->DrawRectangle(btnx1-25, btny0-10, btnx1+45, btny1+10, clrWhite);
       osd->DrawRectangle(btnx1-50, btny0-7, btnx1-25, btny1+7, clrWhite);
       /* covers the whole area exactly: osd->DrawRectangle(xItemLeft-25, y-10, xItemRight-20, y+60, clrWhite); */
#endif

      } else { // if(Current)
#if VDRVERSNUM < 10716
       btnx = xItemLeft-15;
#else
       btnx = xItemLeft-60;
#endif
       btny = yBodyTop;
       btny0 = y;
       btny1 = y /* + lineHeight */ + 50;
#if VDRVERSNUM < 10716
       btnx1 = xItemRight-62;
#else
       btnx1 = xItemRight-60;
#endif
#ifndef DEBUG_CLEAR
       ClearBg(osd, btnx, btny, btnx, btny0, btnx1, btny1+1);
       ClearBg(osd, btnx1-25, btny, btnx1-25, btny0-10, btnx1+45, btny1+10);
       ClearBg(osd, btnx1-55, btny, btnx1-55, btny0-8, btnx1-25, btny1+8);
#else
       osd->DrawRectangle(btnx, btny0, btnx1, btny1+1, clrYellow);
       osd->DrawRectangle(btnx1-25, btny0-10, btnx1+45, btny1+10, clrYellow);
       osd->DrawRectangle(btnx1-55, btny0-8, btnx1-25, btny1+8, clrYellow);
       /* covers the whole area: osd->DrawRectangle(xItemLeft-40, y-5, xItemRight-55, y+55, clrYellow); */
#endif

      }
    } else { // oriented at the left side - these have their icons on the left side
      if(Current) {
       btnx = xItemLeft-15;
       btny = yBodyTop;
       btny0 = y;
       btny1 = y /* + lineHeight */ + 50;
#if VDRVERSNUM < 10716
       btnx1 = xItemRight-62;
#else
       btnx1 = xItemRight-15;
#endif
#ifndef DEBUG_CLEAR
       //ClearBg(osd, btnx, btny, btnx, btny0, btnx1, btny1);
       ClearBg(osd, btnx-45, btny, btnx-45, btny0-12, btnx+18, btny1+12);
       ClearBg(osd, btnx+18, btny, btnx+18, btny0-5, btnx+40, btny1+7);
#else
       //osd->DrawRectangle(btnx, btny0, btnx1, btny1, clrGreen);
       osd->DrawRectangle(btnx-45, btny0-12, btnx+18, btny1+12, clrGreen);
       osd->DrawRectangle(btnx+18, btny0-5, btnx+40, btny1+7, clrGreen);
       /* covers the whole area exactly: osd->DrawRectangle(xItemLeft-70, y-10, xItemRight-55, y+60, clrGreen); */
#endif

      } else { // if(Current)
       btnx = xItemLeft-15;
       btny = yBodyTop;
       btny0 = y;
       btny1 = y /* + lineHeight */ + 50;
#if VDRVERSNUM < 10716
       btnx1 = xItemRight-62;
#else
       btnx1 = xItemRight-15;
#endif
#ifndef DEBUG_CLEAR
       ClearBg(osd, btnx, btny, btnx, btny0, btnx1, btny1+1);
       ClearBg(osd, btnx-45, btny, btnx-45, btny0-12, btnx+20, btny1+12);
       ClearBg(osd, btnx+20, btny, btnx+20, btny0-7, btnx+35, btny1+7);
#else
       osd->DrawRectangle(btnx, btny0, btnx1, btny1+1, clrRed);
       osd->DrawRectangle(btnx-45, btny0-12, btnx+20, btny1+12, clrRed);
       osd->DrawRectangle(btnx+20, btny0-7, btnx+35, btny1+7, clrRed);
       /* covers all exactly: osd->DrawRectangle(xItemLeft-40, y-5, xItemRight-55, y+55, clrRed); */

#endif
      }
    }
  else
    if(!alreadyCleared && !isButtoned)
#ifndef DEBUG_CLEAR
       ClearBg(osd, xItemLeft, yBodyTop, xItemLeft, y, xItemRight, y+lineHeight);
#else
       osd->DrawRectangle(xItemLeft, y, xItemRight-10, y+lineHeight, clrRed);
#endif
    else if(!alreadyCleared)
#ifndef DEBUG_CLEAR
       ClearBg(osd, xItemLeft-20, yBodyTop, xItemLeft-20, y, xItemRight-10, y+lineHeight);
#else
       osd->DrawRectangle(xItemLeft-20, y, xItemRight-10, y+lineHeight, clrRed);
#endif

  bool isNumImg = false;
  /* Display numbers as images */
  int tmp;

  if (MenuType == menuInternetradio) {
   if(strlen(Text) > 0) {
    if(Current)
       osd->DrawImage(imgButtonSmallActive, xItemLeft+10, y - 5 + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
    else
       osd->DrawImage(imgButtonSmallInactive, xItemLeft+10, y - 5 + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
       isNumImg = true; // need to adjust the x-coord of the Text
   }
  } else if ((MenuType == menuSystemSettings) && sscanf(Text,"%d%[^\n]", &tmp, Text_tmp)>0 && Text_tmp[0]==' ') {
    if (Text_tmp[1] != ' ') { //TB: all entries should be aligned the same way, so add a space to lines beginning with only one
        int len = strlen(Text_tmp+1);
        memmove(Text_tmp+2, Text_tmp+1, len);
        Text_tmp[len+2] = '\0';
        Text_tmp[1] = ' ';
    }
    if(Current)
       osd->DrawImage(imgButtonSmallActive, xItemLeft+10, y - 5 + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
    else
       osd->DrawImage(imgButtonSmallInactive, xItemLeft+10, y - 5 + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
       isNumImg = true; // need to adjust the x-coord of the Text
  } else if ( (isNumberImage || isButtoned || isSmallButtoned) && sscanf(Text,"%d%[^\n]", &tmp, Text_tmp)>0 && Text_tmp[0]==' ') {
  /* space needed after number
   1 Media
   2 Setup
   1Media : will be displayed as such */
      if (tmp < 10) {
            std::stringstream buf;
            buf << tmp;
            if(Current)
              buf << "_active.png";
            else
              buf << "_inactive.png";

            //if (!Current) imgNumber ++;

            // Draw number
            /** Draw the number in the middle of the line, (Hack : and then a little lower, +3) */
            //osd->DrawImage (imgNumber, xItemLeft,y + (lineHeight - IMGNUMBERHEIGHT)/2  + "added number here for aligning the text and number images" , true );
            if (MenuType == menuInternetExtras) {
              std::string btn; /* the button's path */
              if(Current)
                osd->DrawImage(imgButtonActive, xItemLeft, y - 10 + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
              else
                osd->DrawImage(imgButtonInactive, xItemLeft, y - 10 + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
              isNumImg = true; // need to adjust the x-coord of the Text
              int icon_width = 36;
              switch(Index) {
                  case 0:  btn = "icon_dateimanager.png"; icon_width = 60 ; break;
                  case 1:  btn = "icon_webbrowser.png";   icon_width = 50 ; break;
                  //case 2:  btn = "icon_internetradio.png";icon_width = 51 ; break;
                  case 2:  btn = "icon_reelblog.png";     icon_width = 66 ; break;
                  default:
		     // Menu item will start plugin and a valid icon for this entry was found -> display it
		     if(bPluginIconFileExists)
		     {
		        btn = PluginIconFile;
			icon_width = 60;
                     } else
		     {
			 btn = "hm_internetextras_inactive.png";
                        icon_width = 50;
		     }
		     break;
               }
               if (!btn.empty()) //((Index >= 0 && Index <= 3 ) || bPluginIconFileExists)
                  DrawUnbufferedImage(osd, btn.c_str(), xItemLeft - icon_width/2, y - 15 + (lineHeight - IMGNUMBERHEIGHT)/2, true);
               else
                  DrawUnbufferedImage(osd, buf.str().c_str(), xItemLeft, y + (lineHeight - IMGNUMBERHEIGHT)/2 -10, true);
            } else if (MenuType == menuFilmsDvd) {
              std::string btn; /* the button's path */
              if(Current)
                osd->DrawImage(imgButtonActive, xItemLeft, y - 10 + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
              else
                osd->DrawImage(imgButtonInactive, xItemLeft, y - 10 + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
              isNumImg = true; // need to adjust the x-coord of the Text
              std::stringstream myText;
              int icon_width = 36;
              switch(Index) {
                  case 0: btn = "icon_filmarchiv.png";      icon_width = 52;      break;
                  case 1: btn = "icon_tv-aufnahmen.png";    icon_width =  50;     break;
                  case 2: btn = "icons_tv-aufnahnem_brennen.png";icon_width = 51;  break;
                  case 3: btn = "icon_videodvd_archivieren.png";icon_width = 52;  break;
                  default:
		     // Menu item will start plugin and a valid icon for this entry was found -> display it
		     if(bPluginIconFileExists)
		     {
		        btn = PluginIconFile;
			icon_width = 60;
                     } else
		     {
			  btn = "hm_filmedvd_inactive.png";
			  icon_width = 50;
		     }
		     break;
              }
              /** special case: "Undelete Recording" shall appear */
              if (Index == 2
                  && strstr(Text, I18nTranslate("Undelete Recordings", "vdr-extrecmenu")) != NULL) {
                      btn = "icon_geloescht.png";
              } else if (Index == 3
                  && strstr(Text, I18nTranslate("Burn Recordings to DVD", "vdr-burn")) != NULL) {
                     btn = "icons_tv-aufnahnem_brennen.png";
              } else if (Index == 4
                  && strstr(Text, I18nTranslate("Archive Video-DVD", "vdr-dvdswitch")) != NULL) {
                     btn = "icon_videodvd_archivieren.png";
              }/** end special case "with Undelete Recordings" */
               if  (!btn.empty()) //((Index >= 0 && Index <= 4 ) || bPluginIconFileExists)
                  DrawUnbufferedImage(osd, btn.c_str(), xItemLeft-icon_width/2, y - 15 + (lineHeight - IMGNUMBERHEIGHT)/2, true);
               else
                  DrawUnbufferedImage(osd, buf.str().c_str(), xItemLeft, y + (lineHeight - IMGNUMBERHEIGHT)/2 -10, true);
            } else if (MenuType == menuOptSoftware) {
              std::string btn; /* the button's path */
              if(Current)
                osd->DrawImage(imgButtonActive, xItemLeft, y - 10 + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
              else
                osd->DrawImage(imgButtonInactive, xItemLeft, y - 10 + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
              isNumImg = true; // need to adjust the x-coord of the Text

              // Menu item will start plugin and a valid icon for this entry was found -> display it
	      if(bPluginIconFileExists)
	         btn = PluginIconFile;
              else
	      {
                 switch(Index%3) {
                     case 0:  btn = "icon_software_01.png";   break;
                     case 1:  btn = "icon_software_02.png";   break;
                     case 2:  btn = "icon_software_03.png";   break;
                     default:                                 break;
                  }
               }
               DrawUnbufferedImage(osd, btn.c_str(), xItemLeft-25, y - 15 + (lineHeight - IMGNUMBERHEIGHT)/2, true);
            } else if (MenuType == menuMusicPictures) {
              std::string btn; /* the button's path */
              if(Current)
                osd->DrawImage(imgButtonActive, xItemLeft, y - 10 + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
              else
                osd->DrawImage(imgButtonInactive, xItemLeft, y - 10 + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
              isNumImg = true; // need to adjust the x-coord of the Text
              int icon_width = 36;
              switch(Index) {
                  case 0:  btn = "icon_musikarchiv.png";  icon_width = 67;  break;
                  case 1:  btn = "icon_bildarchiv.png";   icon_width = 66;   break;
                  case 2:  btn = "icon_internetradio.png";icon_width = 51; break;
                  case 3:  btn = "icon_audiocdarchiv.png";icon_width = 54; break;
                  default:
		      // Menu item will start plugin and a valid icon for this entry was found -> display it
		      if(bPluginIconFileExists)
		      {
		          btn = PluginIconFile;
			  icon_width = 60;
                      } else
		      {
			  btn = "hm_musikbilder_inactive.png";
                          icon_width = 50;
		      }
	              break;
              }
              if (!btn.empty()) //((Index >= 0 && Index <= 3 ) || bPluginIconFileExists)
                  DrawUnbufferedImage(osd, btn.c_str(), xItemLeft - icon_width/2, y - 15 + (lineHeight - IMGNUMBERHEIGHT)/2, true);
              else
                  DrawUnbufferedImage(osd, buf.str().c_str(), xItemLeft, y + (lineHeight - IMGNUMBERHEIGHT)/2 -10, true);
            } else if (MenuType == menuTvRadio) {
              std::string btn; /* the button's path */
              if(Current)
                osd->DrawImage(imgButtonActive, xItemLeft, y - 10 + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
              else
                osd->DrawImage(imgButtonInactive, xItemLeft, y - 10 + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
              isNumImg = true; // need to adjust the x-coord of the Text
              int icon_width = 36;
              switch(Index) {
                  case 0: btn = "icon_senderliste.png";  icon_width = 40;   break;
                  case 1: btn = "icon_epg.png";          icon_width = 59;   break;
                  case 2: btn = "icon_timer.png";        icon_width = 52;   break;
                  case 3: btn = "icon_suchtimer.png";    icon_width = 49;   break;
                  //case 5: btn = "icon_kindersicherung.png"; icon_width = 46; break;
                  default:
		     // Menu item will start plugin and a valid icon for this entry was found -> display it
		     if(bPluginIconFileExists)
		     {
		        btn = PluginIconFile;
			icon_width = 60;
                     } else
		     {
			btn = "hm_tvradio_inactive.png";
                        icon_width = 50;
		     }
		     break;
               }
               if  (!btn.empty()) //((Index >= 0 && Index <= 5 ) || bPluginIconFileExists)
                  DrawUnbufferedImage(osd, btn.c_str(), xItemLeft-icon_width/2, y - 15 + (lineHeight - IMGNUMBERHEIGHT)/2, true);
               else
                  DrawUnbufferedImage(osd, buf.str().c_str(), xItemLeft, y + (lineHeight - IMGNUMBERHEIGHT)/2 -10, true);
            } else if (MenuType != menuGimmick && isSmallButtoned) {
              if(Current)
                osd->DrawImage(imgButtonSmallActive, xItemLeft+10, y - 8 + (lineHeight - IMGNUMBERHEIGHT)/2, true);
              else
                osd->DrawImage(imgButtonSmallInactive, xItemLeft+10, y - 8 + (lineHeight - IMGNUMBERHEIGHT)/2, true);
              DrawUnbufferedImage(osd, buf.str().c_str(), xItemLeft, y + (lineHeight - IMGNUMBERHEIGHT)/2 -9, true);
              isNumImg = true; // need to adjust the x-coord of the Text
            } else if (MenuType != menuGimmick && isButtoned) {
              if(Current)
                osd->DrawImage(imgButtonActive, xItemLeft+10, y - 10 + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
              else
                osd->DrawImage(imgButtonInactive, xItemLeft+10, y - 10 + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
              isNumImg = true; // need to adjust the x-coord of the Text
            } else if(MenuType != menuGimmick) {
              DrawUnbufferedImage(osd, buf.str().c_str(), xItemLeft, y + (lineHeight - IMGNUMBERHEIGHT)/2 -10, true);
              isNumImg = true; // need to adjust the x-coord of the Text
            } else {
	      /// Main menu
              y -= 50;
              std::string btn; /* the button's path */
              int xbtn;      /* x-coordinate of the button */
              std::stringstream myText;
              if(Current)
                 xbtn = xItemLeft-15;
              else
                 xbtn = xItemLeft+5;
              // TODO: check if file exists
              if (Current)
              {
                 osd->DrawImage(imgButtonBigActive, xbtn, 35 + y + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
                 if (strstr(Text, I18nTranslate("TV & Radio", "vdr")) != NULL) {
                     btn = "hm_tvradio_active.png";
                 } else if (strstr(Text, I18nTranslate("Music & Pictures", "vdr")) != NULL) {
                     btn = "hm_musikbilder_active.png";
                 } else if (strstr(Text, I18nTranslate("Films & DVD", "vdr")) != NULL) {
                     btn = "hm_filmedvd_active.png";
                 } else if (strstr(Text, I18nTranslate("Internet & Extras", "vdr")) != NULL) {
                     btn = "hm_internetextras_active.png";
                 } else if (strstr(Text, I18nTranslate("Additional Software", "vdr")) != NULL) {
                     btn = "hm_software_active.png";
                 }
                 if(Index%2==1)
                   DrawUnbufferedImage(osd, btn.c_str(), xbtn+310, 23 + y + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
                 else
                   DrawUnbufferedImage(osd, btn.c_str(), xbtn-45, 23 + y + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
              } else {
                 osd->DrawImage(imgButtonInactive, xbtn, 38 + y + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
                 if (strstr(Text, I18nTranslate("TV & Radio", "vdr")) != NULL) {
                     btn = "hm_tvradio_inactive.png";
                 } else if (strstr(Text, I18nTranslate("Music & Pictures", "vdr")) != NULL) {
                     btn = "hm_musikbilder_inactive.png";
                 } else if (strstr(Text, I18nTranslate("Films & DVD", "vdr")) != NULL) {
                     btn = "hm_filmedvd_inactive.png";
                 } else if (strstr(Text, I18nTranslate("Internet & Extras", "vdr")) != NULL) {
                     btn = "hm_internetextras_inactive.png";
                 } else if (strstr(Text, I18nTranslate("Additional Software", "vdr")) != NULL) {
                     btn = "hm_software_inactive.png";
                 }
                 if(Index%2==1)
                   DrawUnbufferedImage(osd, btn.c_str(), xbtn+270, 30 + y + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
                 else
                   DrawUnbufferedImage(osd, btn.c_str(), xbtn-40, 30 + y + (lineHeight - IMGNUMBERHEIGHT)/2 -2, true);
              }
              isNumImg = true;
            }
        } else { /* got number more than 9! */
            debug("Got number %d: cannot display it as images are available only till 9", tmp);
            isNumImg = false; /* need to adjust the x-coord of the Text */
            free(Text_tmp);
            Text_tmp = strdup (Text);
        }
  } else {
      free(Text_tmp);
      Text_tmp = strdup (Text);
  }

  /** Set 'y' : such that any text drawn is in the middle of the lineHeightTitle */
  y = yItemTop + Index * lineHeight + (lineHeight - /* TB 22 */ pFontList->Height() ) /2; /* Draw Texts in the middle of the line */

  if (nMessagesShown > 0 && y >= yMessageTop)
    return; /* Don't draw above messages */

  /* change to CAPS if necessary: for MainMenu or Main Setup page */
  if(Text_tmp && isCAPS) {
    /* convert Text to all caps */
    for (int i = 0; Text_tmp[i] != '\0'; i++)
      Text_tmp[i] = toupper(Text_tmp[i]);
  }

  if (Current)
    pFontList = ReelConfig.GetFont(FONT_LISTITEMCURRENT); //TODO? get current font which might have been patched meanwhile
  else
    pFontList = ReelConfig.GetFont(FONT_LISTITEM); //TODO? get current font which might have been patched meanwhile

  /* draw item */
  static char s[512];
  for (int i = 0; i < MaxTabs; i++) {
    const char *s1 = GetTabbedText(Text_tmp, i);
        if (s1)
        {
            strncpy(s,s1,511);
            if (Tab(i+1) && GetTabbedText(Text_tmp, i+1) && !IsProgressBarStr(s))
                // Cut string only if there is a next tab  or next tab in Text_tmp?
                CutStrToPixelLength(s, pFontList, Tab(i+1)-Tab(i));
        }
    else
      s[0]='\0';

    if (s[0]) {
      static char buffer[10];
      int xt = xItemLeft + Tab(i) + (isNumImg ? 35:0); // add 35px if an image of number was drawn
      unsigned int len_s = strlen(s);

      bool iseventinfo = false;
      bool isnewrecording = false;
      bool isNewHDRecording = false;
      bool isNewRecording = false; // show 'NEW' icon at the beginning of text
      bool isarchived = false;
      bool isprogressbar = false;
      bool isHD = false;
      bool isFavouriteFolder = false;
      bool isFolder = false;
      bool isCheckmark = false;
      bool isRunningnow = false;
      bool isSharedDir = false;
      bool isMusic = false;
      bool isFolderUp = false;
      bool isArchive = false;
      bool isVdrRec = false; // char 140
      bool isVideo = false; // char 141
      bool isForbiddenFolder = false; // char 146
      bool isCompressed = false; // char 148
      int singlechar = eNone;
      int now = 0, total = 0;
      int w;

      if (len_s == 1)
        switch ((int) s[0]) {
          case 142: singlechar = eHD;                break;
          case 143: singlechar = eHdEncrypted;       break;
          case 144: singlechar = eNewHD;             break;
          case 145: singlechar = eNewEncrypted;      break;
          case  80: singlechar = ePinProtect;        break;
          case 129: singlechar = eArchive;           break;
          case 128: singlechar = eBack;              break;
          case 130: singlechar = eFolder;            break;
          case 132: singlechar = eNewrecording;      break;
          case 133: singlechar = eCut;               break;
          case 134: singlechar = eRunningNow;        break; // TB: used by extrecmenu when moving folders
          case '#': singlechar = eRecord;            break;
          case '>': singlechar = eTimeractive;       break;
          case '!': singlechar = eSkipnextrecording; break;
          case 136: singlechar = eRunningNow;        break;
          case 137: singlechar = eNetwork;           break;
          case 138: singlechar = eDirShared;         break;
          case 139: singlechar = eMusic;             break;
          case 147: singlechar = eYellowButton;      break;
          case 148: singlechar = eCompressed;        break;
          default:                                   break;
        }

#ifdef USE_WAREAGLEICON
      // use the icons instead of the font symbols
      // TODO; This is not save. This really really ugly and bad.
      // TODO: works only for UTF8!
      if (len_s == 3) { // && s[0] == char(-18)) {
       if (! strcmp(s, Icons::Directory()) )
         singlechar = eFolder;
       if (! strcmp(s, Icons::New()) )
         singlechar = eNewrecording;
       if (! strcmp(s, Icons::Scissor()) )
         singlechar = eCut;
       // TODO: add more, better algo :)
     }
     //TODO: it's ugly once more
     // * in the first column is assumed as new recording
     if (! strcmp(s, "*") && i == 0 ) {
       singlechar = eNewrecording;
       isnewrecording = true;
     }
#endif

     /*the following is needed since, there is no otherway to display only
      * the FolderUp icon in it's own column. Either FolderUp icon
      * and ".." are drawn or FolderUp icon alone cannot be put in a column*/
      if (len_s == 2 && s[0]== (char)128 && s[1]== ' ') {isFolderUp = true; s[0] = ' ';}
      else if (len_s == 2 && s[0]== (char)130 && s[1]== ' ') {singlechar = eFolder;}
      else if (len_s == 2 && s[0]== (char)130 && s[1]== 'p') {singlechar = ePinProtect; isFolder = true;}
      /* following three cases for displaying both a 'record/red button'
         together with a tick sign
         Used by setup plugin in "Media Device" menu
       */
      else if (len_s == 2 && s[0]== '#' && s[1]== '>') {singlechar = eRecordTick;}
      else if (len_s == 2 && s[0]== '#' && s[1]== ' ') {singlechar = eRecord;}
      else if (len_s == 2 && s[0]== ' ' && s[1]== '>') {singlechar = eBlankTick;}

      else if (len_s == 2 && s[1] == (char)133) {
        if (s[0]==(char)132)
          singlechar = eNewCut;
        else if (s[0]==(char)129)
          singlechar = eArchivedCut;
      // check if event info symbol: "StTV*" "R"
      // check if event info characters
      } else if (len_s == 3 && strchr(" StTR", s[0]) && strchr(" V", s[1]) && strchr(" *", s[2])) {
        iseventinfo = true; /* update status */
      // check if event info symbol: "RTV*" strlen=4
      } else if (len_s == 4 && strchr(" tTR", s[1]) && strchr(" V", s[2]) && strchr(" *", s[3])) {
        iseventinfo = true;
      // check if new recording: "01.01.06*", "10:10*"
      } else if ((len_s == 6 && s[5] == '*' && s[2] == ':' && isdigit(*s)
           && isdigit(*(s + 1)) && isdigit(*(s + 3)) && isdigit(*(s + 4)))
           || (len_s == 9 && s[8] == '*'
              && s[5] == s[2] && s[5] == '.'
              && isdigit(*s) && isdigit(*(s + 1)) && isdigit(*(s + 3))
              && isdigit(*(s + 4)) && isdigit(*(s + 6)) && isdigit(*(s + 7)))) {
        isnewrecording = true; /* update status */
        strncpy(buffer, s, strlen(s)); /* make a copy */
        buffer[strlen(s) - 1] = '\0'; /* remove the '*' character */
      // check if progress bar: "[|||||||   ]"
      } else if (!isarchived && ReelConfig.showProgressbar && (len_s > 5 && s[0] == '[' && s[strlen(s) - 1] == ']')) {
        const char *p = s + 1;
        isprogressbar = true; /* update status */
        for (; *p != ']'; ++p) {
          if (*p == ' ' || *p == '|') { /* check if progressbar characters */
            ++total; /* update counter */
            if (*p == '|')
              ++now;
          } else {
            isprogressbar = false; /* wrong character detected; not a progressbar */
            break;
          }
        }
      }

      if(!iseventinfo && !isnewrecording && !isarchived && !isprogressbar && len_s > 3 && s[1] == ' ') {
        switch(s[0]) {
          case (char)134: isHD              = true; s[0]=' '; break;
          case (char)130: isFolder          = true; s[0]=' '; break;
          case (char)131: isFavouriteFolder = true; s[0]=' '; break;
          case (char)135: isCheckmark       = true; s[0]=' '; break;
          case (char)136: isRunningnow      = true; s[0]=' '; break;
          case (char)138: isSharedDir       = true; s[0]=' '; break;
          case (char)139: isMusic           = true; s[0]=' '; break;
          case (char)128: isFolderUp        = true; s[0]=' '; break;
          case (char)129: isArchive         = true; s[0]=' '; break;
          case (char)140: isVdrRec          = true; s[0]=' '; break;
          case (char)141: isVideo           = true; s[0]=' '; break;
          case (char)146: isForbiddenFolder = true; s[0]=' '; break;
          case (char)144: isNewHDRecording  = true; s[0]=' '; break;
          case (char)132: isNewRecording    = true; s[0]=' '; break;
          case (char)148: isCompressed      = true; s[0]=' '; break;
          default: break;
        }
      }

      if (iseventinfo) {
        int evx = xt + Gap;
        const char *p = s;
        // draw background
        // osd->DrawRectangle(xt, y, xItemRight, y + lineHeight - 1, ColorBg);
        // draw symbols
        for (; *p; ++p) {
          switch (*p) {
            case 't': osd->DrawImage(imgIconTimerpart, evx, y + 1, true); break; /* partial timer event */
            case 'T': osd->DrawImage(imgIconTimer, evx, y + 1, true); break; /* timer event */
            case 'R': osd->DrawImage(imgIconRecord, evx, y + 1, true); break; /* recording event (epgsearch) */
            case 'V': osd->DrawImage(imgIconVps, evx, y + 1, true); break; /* vps event */
            case 'S': osd->DrawImage(imgIconZappingtimer, evx, y + 1, true); break;
            case '*':
              // running event
              // osd->DrawImage(imgIconRunningnow, evx, y + 1, true);
              // evx += IMGICONWIDTH;
              // break;
            case ' ': /* let's ignore space character */
            default:
              evx -= IMGICONWIDTH; /* no icon - so revert shifting the text to the right */
              break;
          }
          evx += IMGICONWIDTH; /* shift the text a bit to the right to have space for the icon */
        }
      } else if (isnewrecording || isarchived) {
        /* draw text */
        osd->DrawText(xt, y, buffer, ColorFg, clrTransparent, pFontList, xItemRight - xt);
        /* draw symbol */
        if (isarchived) {
          osd->DrawImage(imgIconArchive, xt + pFontList->Width(buffer), y + 1, true);
          if (isnewrecording)
            osd->DrawImage(imgIconNewrecording, xt + pFontList->Width(buffer) + IMGICONWIDTH, y + 1, true);
        } else
          osd->DrawImage(imgIconNewrecording, xt + pFontList->Width(buffer), y + 1, true);
      } else if (isprogressbar) {
        // define x coordinates of progressbar
        int px0 = xt;
        //int px1 = (Selectable ? (Tab(i + 1) ? Tab(i + 1) : xItemRight - Roundness) : xItemRight-Roundness) ;
        int px1 = px0 ; //= (Tab(i + 1) ? Tab(i + 1) : xItemRight - Roundness)  ;
        // a better calculation of progressbar length
        if( Tab(i+1) > 0 )
#ifdef REELVDR
            px1 = px0 + Tab(i+1) - Tab(i) - (ReelConfig.showMarker ? lineHeight : ListHBorder) ;
#else
            px1 = px0 + (Tab(i+1)/12) - (ReelConfig.showMarker ? lineHeight : ListHBorder) ;
#endif
         else // no more tabs: progress bar goes till the end
            px1 = xItemRight - Roundness;

#ifndef REELVDR
    //This is ugly:
    //because of Tab(i) is divided by 12
    //we have to check here for an too small bar
    //this is happening at the "EPG-Current-View"
    if (px1 - px0 < 20)
       px1 = px0 + 40;
#endif

        // when the string complete string is a progressbar, ignore tabs
    if (len_s == strlen(Text_tmp)) px1 = xItemRight - Roundness;

        int px = px0 + std::max((now * (px1 - px0) / total), ListProgressBarBorder);
        // define y coordinates of progressbar
        int py0 = y + (lineHeight - progressbarheight) / 2;
        // draw progressbar
        DrawProgressBar(osd, px0, py0, px-1, px1);
      } else if (singlechar == ePinProtect && isFolder) {
            osd->DrawImage(imgIconFolder, xt, y + 1, true);
            osd->DrawImage(imgIconPinProtect, xt+20, y + 1, true);
      } else if (singlechar != eNone) {
        switch (singlechar)
        {
          case eHdEncrypted:
            osd->DrawImage(imgIconHdKey, xt, y+1, true);
            break;
        case eNewHD:
            DrawUnbufferedImage(osd, "icon_hd_new.png", xt, y+1, true);
            break;
        case eNewEncrypted:
            DrawUnbufferedImage(osd, "icon_key_new.png", xt, y+1, true);
            break;
        case eNewHDEncrypted:
            DrawUnbufferedImage(osd, "icon_hd_key_new.png", xt, y+1, true);
            break;
          case eHD:
            osd->DrawImage(imgIconHd, xt, y+1, true);
            break;
          case ePinProtect:
            osd->DrawImage(imgIconPinProtect, xt, y + 1, true);
            break;
          case eArchive:
            osd->DrawImage(imgIconArchive, xt, y + 1, true);
            break;
          case eBack:
            {
            osd->DrawImage(imgIconFolderUp, xt, y + 1, true);
            xt += 20;
            w = (Tab(i + 1) && HasTabbedText(Text_tmp, i + 1) ? (xItemLeft + Tab(i + 1)) : xItemRight) - xt;
            std::string txt = std::string("  ") + tr("up");
            osd->DrawText(xt, y, txt.c_str(), ColorFg, clrTransparent, pFontList, w, nMessagesShown ? std::min(yMessageTop - y, lineHeight) : 0 );
            break;
            }
          case eFolder:
            osd->DrawImage(imgIconFolder, xt, y + 1, true);
            break;
          case eNewrecording:
            osd->DrawImage(imgIconNewrecording, xt, y + 1, true);
            break;
          case eCut:
            osd->DrawImage(imgIconCut, xt, y + 1, true);
            break;
          case eNewCut:
            osd->DrawImage(imgIconNewrecording, xt, y + 1, true);
            osd->DrawImage(imgIconCut, xt+IMGICONWIDTH, y + 1, true);
            break;
          case eArchivedCut:
            osd->DrawImage(imgIconArchive, xt, y + 1, true);
            osd->DrawImage(imgIconCut, xt+IMGICONWIDTH, y + 1, true);
            break;
          case eBlankTick:
	    //space before a tick image
            osd->DrawImage(imgIconTimeractive, xt+IMGICONWIDTH, y + 1, true);
            break;
          case eRecordTick:
            osd->DrawImage(imgIconRecord, xt, y + 1, true);
            osd->DrawImage(imgIconTimeractive, xt+IMGICONWIDTH, y + 1, true);
            break;
          case eRecord:
            osd->DrawImage(imgIconRecord, xt, y + 1, true);
            break;
          case eTimeractive:
            osd->DrawImage(imgIconTimeractive, xt, y + 1, true);
            break;
          case eSkipnextrecording:
            osd->DrawImage(imgIconSkipnextrecording, xt, y + 1, true);
            break;
          case eNetwork:
            DrawUnbufferedImage(osd, "icon_network_20.png", xt, y+1, true);
            break;
          case eDirShared:
            DrawUnbufferedImage(osd, "icon_foldershare_20.png", xt, y+1, true);
            break;
          case eMusic:
        DrawUnbufferedImage(osd, "icon_music_20.png", xt, y+1, true);
        break;
          case eRunningNow:
            osd->DrawImage(imgIconRunningnow, xt, y + 1, true);
            break;
        case eYellowButton:
            DrawUnbufferedImage(osd, "small_button_yellow.png", xt, y+1, true);
            break;
        case eCompressed:
            DrawUnbufferedImage(osd, "icon_compressed.png", xt, y+1, true);
            break;
          default:
            break;
        }
      } else {
        if (isCompressed)
          DrawUnbufferedImage(osd, "icon_compressed.png", xt, y+1, true);
        else if (isHD)
          osd->DrawImage(imgIconHd, xt, y + 1, true);
        else if (isFavouriteFolder)
          osd->DrawImage(imgIconFavouriteFolder, xt, y + 1, true);
        else if (isFolder)
          osd->DrawImage(imgIconFolder, xt, y + 1, true);
        else if (isCheckmark)
          osd->DrawImage(imgIconTimeractive, xt, y + 1, true);
        else if (isRunningnow)
          osd->DrawImage(imgIconRunningnow, xt, y + 1, true);
        else if (isSharedDir)
          DrawUnbufferedImage(osd, "icon_foldershare_20.png", xt, y+1, true);
        else if (isMusic)
          DrawUnbufferedImage(osd, "icon_music_20.png", xt, y+1, true);
        else if (isFolderUp)
            osd->DrawImage(imgIconFolderUp, xt, y + 1, true);
        else if (isArchive)
            osd->DrawImage(imgIconArchive, xt, y + 1, true);
        else if (isVdrRec)
            DrawUnbufferedImage(osd, "icon_vdr_rec_20.png", xt, y+1, true);
        else if (isVideo)
            DrawUnbufferedImage(osd, "icon_video_20.png", xt, y+1, true);
        else if (isForbiddenFolder)
            DrawUnbufferedImage(osd, "icon_forbidden_folder_20.png", xt, y+1, true);
        else if (isNewHDRecording)
            DrawUnbufferedImage(osd, "icon_hd_new.png", xt, y+1, true);
        else if(isNewRecording)
            osd->DrawImage(imgIconNewrecording, xt, y + 1, true);
        else
            xt -= 20; /* no image, so revert increasing the x-coord coming in the next line */
        xt += 20; /* increase x-coord for all images */

        w = (Tab(i + 1) && HasTabbedText(Text_tmp, i + 1) ? (xItemLeft + Tab(i + 1)) : xItemRight) - xt;
        //TODO? int w = xItemRight - xt;
        /* draw text */
        osd->DrawText(xt, y, s, ColorFg, clrTransparent, pFontList, (xt + pFontList->Width(s) < xItemRight) ? pFontList->Width(s)+1 : (xItemRight - xt - 1), nMessagesShown ? std::min(yMessageTop - y, lineHeight) : 0 );

      }
    }
  }
  /* set editable width */
  SetEditableWidth(xItemRight - Tab(1) - xItemLeft);

  free(Text_tmp);

  if (Current)
      pFontList = ReelConfig.GetFont(FONT_LISTITEM);  // pFontList is used elsewhere
}

const char *cSkinReelDisplayMenu::GetPluginMainMenuName(const char *plugin) {
  cPlugin *p = cPluginManager::GetPlugin(plugin);
  if (p) {
    const char *entry = p->MainMenuEntry();
    if (entry)
      return entry;
  }
  return plugin;
}

bool cSkinReelDisplayMenu::PluginIconFileExists(const char *PluginIconFile)
{
  if(!PluginIconFile)
     return false;

  char *imgPath = strdup(ReelConfig.GetImagesDir());
  string sPath = imgPath;
  free(imgPath);

  sPath += "/";
  sPath += PluginIconFile;
  struct stat results;
  if ( stat ( sPath.c_str(), &results ) == 0 )
      return true;
  else
  {
     return false;
  }
}

const char *cSkinReelDisplayMenu::GetPluginName(const char *MainMenuEntry) {
  for(int nIndex=0; cPlugin *p = cPluginManager::GetPlugin(nIndex);nIndex++)
  {
      if(!MainMenuEntry)
	   return "";
      if (p)
      {
          const char *entry = p->MainMenuEntry();
         if (entry && strcmp(entry, MainMenuEntry) == 0)
	  {
              return p->Name();
	  }
      }
  }
  return "";   // Plugin not found in Pluginmanager
}


void cSkinReelDisplayMenu::SetEvent(const cEvent *Event) {
  if (!Event)
    return;

  const cFont *pFontDetailsTitle = ReelConfig.GetFont(FONT_DETAILSTITLE);
  const cFont *pFontDetailsSubtitle = ReelConfig.GetFont(FONT_DETAILSSUBTITLE);
  const cFont *pFontDetailsDate = ReelConfig.GetFont(FONT_DETAILSDATE);
  const cFont *pFontDetailsText = ReelConfig.GetFont(FONT_DETAILSTEXT);

  char sPath[512];
  sprintf(sPath, "%s/%ld.raw", ReelConfig.GetEPGImagesDir(), (long)Event->EventID());
  struct stat results;
  bool bPicAvailable = ( stat ( sPath, &results ) == 0 );

  isMainMenu = false;

  SetupAreas();

  time_t now = time(NULL);

  int percentCompleted = 0; /* percent of current program completed */
  /* draw recording date string */
  std::stringstream sstrDate;
  std::stringstream sstrDuration;

  sstrDate << *Event->GetDateString() << "  " << *Event->GetTimeString() << " - " << *Event->GetEndTimeString();
  sstrDuration << " (";

  if (now > Event->StartTime()) {
    percentCompleted = ((now - Event->StartTime())*100) /60;
    sstrDuration << (now - Event->StartTime()) / 60 << '/';
  } else
    percentCompleted = 0;

  sstrDuration << Event->Duration() / 60 << tr("min") << ')';

  if ( Event->Duration() > 0 )
      percentCompleted /= ( Event->Duration() / 60 );
  else
      percentCompleted = 0;

  /* draw background */
  if(!alreadyCleared) {
    osd->RestoreRegion();
  }
  /** clear the complete right part, there could have been a big scrollbar from the previous menu */
  ClearBg(osd, xDateRight - Gap - ScrollbarWidth-5, yBodyTop, xDateRight - Gap - ScrollbarWidth-5, yBodyTop, xDateRight - Gap, yBodyBottom );

  int y = yDateBottom + (pFontDetailsDate->Height() - CHANNELINFOSYMBOLHEIGHT) / 2;
  int xs = xDateRight - Gap;
#if 0 /* show VPS in after date-time string*/
  /* check if event has VPS */
  if (ReelConfig.showVps && Event->Vps()) {
    /* draw VPS symbol */
    if (Event->Vps() != Event->StartTime()) {
      static char vps[12];
      struct tm tm_r;
      time_t t_vps = Event->Vps();
      strftime(vps, sizeof(vps), "VPS: %H:%M", localtime_r(&t_vps, &tm_r));
      xs -= pFontDetailsDate->Width(vps);
      osd->DrawText(xItemLeft + Roundness /*xs*/, yDateBottom, vps, Theme.Color(clrMenuTxtFg), clrTransparent, pFontDetailsDate, pFontDetailsDate->Width(vps), pFontDetailsDate->Height());
      xs -= TinyGap;
    }
  }
#endif
  std::string stringInfo;
  const cComponents *Components = Event->Components();
  if (Components) {
    std::stringstream sstrInfo;
    for (int i = 0; i < Components->NumComponents(); i++) {
      const tComponent *p = Components->Component(i);
      if (p && (p->stream == 2) && p->language) {
        if (p->description) {
          sstrInfo << p->description << " (" << p->language << "), ";
        } else {
          sstrInfo << p->language << ", ";
        }
      }
    }
    /* strip out the last delimiter */
    if (!sstrInfo.str().empty()) {
      stringInfo = "\n";
      stringInfo += tr("Languages");
      stringInfo += ": ";
      stringInfo += sstrInfo.str().substr(0, sstrInfo.str().length() - 2);
    }
  }
  int yHeadlineBottom = yDateBottom + pFontDetailsTitle->Height();

  int xHeadlineRight = xInfoRight;
  //TB osd->DrawRectangle(xItemLeft/*xBodyLeft*/, yBodyTop, xHeadlineRight - 1, yHeadlineBottom - 1, Theme.Color(clrAltBackground));

  if(bPicAvailable)
     xHeadlineRight -= EPG_IMAGE_RESIZE_WIDTH;

  int th = pFontDetailsTitle->Height() + (!isempty(Event->Description()) && !isempty(Event->ShortText()) ? Gap + pFontDetailsSubtitle->Height() : 0);
  y = yBodyTop + (yHeadlineBottom - yBodyTop - th) / 2;

  /* draw recording title */
  int TitleHeight = WrapText(xItemLeft + Roundness, y, Event->Title(), Theme.Color(clrRecTitleFg), clrTransparent,
                pFontDetailsTitle, xHeadlineRight - xItemLeft - Roundness - 1);

  osd->DrawText(xItemLeft + Roundness, yHeadlineBottom, sstrDate.str().c_str(), Theme.Color(clrMenuItemNotSelectableFg), clrTransparent,
                pFontDetailsDate, xs - xItemLeft - Roundness*2 - 1, pFontDetailsDate->Height());

  /* check if event has VPS */
  if (ReelConfig.showVps && Event->Vps()) {
    /* draw VPS symbol */
    if (Event->Vps() != Event->StartTime()) {
      static char vps[12];
      struct tm tm_r;
      time_t t_vps = Event->Vps();
      strftime(vps, sizeof(vps), " %H:%M", localtime_r(&t_vps, &tm_r));
      //xs -= pFontDetailsDate->Width(vps);
      int xx = xItemLeft + Roundness
              + pFontDetailsDate->Width(sstrDate.str().c_str()) + BigGap;
      osd->DrawImage(imgIconVps, xx, yHeadlineBottom, true);
      osd->DrawText(xx + 20, yHeadlineBottom, vps,
                    Theme.Color(clrMenuItemNotSelectableFg),
                    clrTransparent, pFontDetailsDate,
                    pFontDetailsDate->Width(vps), pFontDetailsDate->Height());

    }
  }

  /* Draw percent Completed bar */
  int pbthickness = (int)( 0.4*pFontDetailsDate->Height("|") );
  int pbLength = pFontDetailsDate->Width("Wed 01.01.2008");
  int xPB = xItemLeft + Roundness*2;// + 4*Gap + pFontDetailsDate->Width(sstrDate.str().c_str()); //XXX check if it exceeds OSD
  int yPB = yHeadlineBottom + (int)(1.5*lineHeight - 0.5*pbthickness );

  /* check if percentage is within [0,100] */
  if ( percentCompleted < 0 )
    percentCompleted = 0;
  else if (percentCompleted > 100)
    percentCompleted = 100;

  /* draw progressbar */
  DrawProgressBar(osd, xPB, yPB, xPB + (int)(pbLength*percentCompleted/100), xPB + pbLength);

  /* Event Duration */
  osd->DrawText( xPB + pbLength + 4*Gap, yHeadlineBottom + lineHeight, sstrDuration.str().c_str(),
     Theme.Color(clrMenuItemNotSelectableFg), clrTransparent /*Theme.Color(clrBackground)*/, pFontDetailsDate);

  /* draw recording short text and description */
  const char *strDescr = NULL;
  if (isempty(Event->Description())) {
    if (!isempty(Event->ShortText())) /* check if short text */
      strDescr = Event->ShortText(); /* draw short text as description, if no description available */
  } else {
    if (!isempty(Event->ShortText())) { /* check if short text */
      y += TitleHeight + Gap; /* draw below Event->Title */
      /* draw short text */
      WrapText(xItemLeft + Roundness*2, y, Event->ShortText(), Theme.Color(clrMenuItemNotSelectableFg), clrTransparent,
                pFontDetailsSubtitle, xHeadlineRight - xItemLeft - Roundness*2 - 1);
    }
    strDescr = Event->Description(); /* draw description */
  }

  std::string stringReruns;

  const char *strFirst = NULL;
  const char *strSecond = NULL;
  const char *strThird = stringReruns.empty() ? NULL : stringReruns.c_str();

  if (ReelConfig.showAuxInfo) {
    strFirst = strDescr;
    strSecond = stringInfo.empty() ? NULL : stringInfo.c_str();
  } else {
    strFirst = stringInfo.empty() ? NULL : stringInfo.c_str();
    strSecond = strDescr;
  }

  if(bPicAvailable)
    DrawImage(sPath, 'R' /* R=raw, P=png, J=jpeg */, /* 468 */ EPG_IMAGE_XPOS, EPG_IMAGE_YPOS, EPG_IMAGE_RESIZE_WIDTH);

  if (strFirst || strSecond || strSecond) {
    y = yHeadlineBottom + SmallGap + 3 * pFontDetailsDate->Height(); /* 2->3 for the EPG event progressbar */
    std::stringstream mytext;
    if(strFirst)
      mytext << strFirst;
    if(strSecond)
      mytext << strSecond;
    if((strFirst || strSecond) && strThird)
      mytext << std::endl << std::endl;
    if(strThird)
      mytext << strThird;
    textScroller.Set(osd, xItemLeft + Roundness, y, xInfoRight - SmallGap - ScrollbarWidth - xItemLeft - 40,
                     yBodyBottom - y, mytext.str().c_str(), pFontDetailsText, Theme.Color(clrMenuTxtFg), clrTransparent);
    SetScrollbar();
  }
}

int cSkinReelDisplayMenu::ReadSizeVdr(const char *strPath) {
  int dirSize = -1;
  char buffer[20];
  std::stringstream strFilename;
  strFilename << strPath <<  "/size.vdr";
  struct stat st;
  if (stat(strFilename.str().c_str(), &st) == 0) {
    int fd = open(strFilename.str().c_str(), O_RDONLY);
    if (fd >= 0) {
      if (safe_read(fd, &buffer, sizeof(buffer)) >= 0) {
        dirSize = atoi(buffer);
      }
      close(fd);
    }
  }
  return dirSize;
}

void cSkinReelDisplayMenu::DrawZoomedImage(const cRecording *Recording)
{
  string PngPath = Recording->FileName();
  PngPath += "/vdr.png";

  const cFont *pFontOsdTitle;
  pFontOsdTitle = ReelConfig.GetFont(FONT_DETAILSSUBTITLE);

  osd->DrawRectangle(RECORDINGINFO_ZOOMED_IMAGE_XPOS - 2, RECORDINGINFO_ZOOMED_IMAGE_YPOS - (pFontOsdTitle->Height() + 4), RECORDINGINFO_ZOOMED_IMAGE_XPOS + RECORDINGINFO_ZOOMED_IMAGE_RESIZE_WIDTH + 2, RECORDINGINFO_ZOOMED_IMAGE_YPOS - 1, Theme.Color(clrBackground));

  string sTitle = tr("Image for recording");
  sTitle += " \"";
  sTitle += Recording->Info()->ShortText();
  sTitle += "\"";

  osd->DrawText(RECORDINGINFO_ZOOMED_IMAGE_XPOS + 5, RECORDINGINFO_ZOOMED_IMAGE_YPOS - (pFontOsdTitle->Height() + 2), sTitle.c_str(), Theme.Color(clrMenuTxtFg), clrTransparent, pFontOsdTitle);

  DrawImage((char *)PngPath.c_str(), 'P' /* R=raw, P=png, J=jpeg */, RECORDINGINFO_ZOOMED_IMAGE_XPOS, RECORDINGINFO_ZOOMED_IMAGE_YPOS, RECORDINGINFO_ZOOMED_IMAGE_RESIZE_WIDTH);
}

void cSkinReelDisplayMenu::SetRecording(const cRecording *Recording)
{
  if (!Recording)
    return;

  const cRecordingInfo *Info = Recording->Info();
  if (Info == NULL) {
    //TODO: draw error message
    return;
  }

  const cFont *pFontDetailsTitle = ReelConfig.GetFont(FONT_DETAILSTITLE);
  const cFont *pFontDetailsSubtitle = ReelConfig.GetFont(FONT_DETAILSSUBTITLE);
  const cFont *pFontDetailsDate = ReelConfig.GetFont(FONT_DETAILSDATE);
  const cFont *pFontDetailsText = ReelConfig.GetFont(FONT_DETAILSTEXT);

  isMainMenu = false;

  string PngPath = Recording->FileName();
  PngPath += "/vdr.png";

  // If image is available change background on left size in order to display the image
  struct stat results;
  if ( stat ( PngPath.c_str(), &results ) == 0 )
  {
	if(memcmp((char *)Recording->Info()->Title(), "ZOOMPIC$", 8) == 0)
	{
	    // ReDraw original banner
      	    SetTitle(I18nTranslate("Recording info", "vdr"));

	    DrawZoomedImage(Recording);
	    osd->Flush();
	    return;  // Do nothing else -> return
	}

#if true
      /** Hack: insert "SHOWINGMP3COVER$" in the title to tell the skin to use another banner,
       * the skin will filter the "SHOWINGMP3COVER$" out again */
      std::string dummy = "SHOWINGMP3COVER$";
      dummy += I18nTranslate("Recording info", "vdr");
      SetTitle(dummy.c_str());
#endif
      // Image will be drawn at the end of this function
  }

  /* draw additional information */
  std::stringstream sstrInfo;
  int dirSize = -1;
  if (ReelConfig.showRecSize > 0) {
    if ((dirSize = ReadSizeVdr(Recording->FileName())) < 0 && ReelConfig.showRecSize == 2) {
      dirSize = DirSizeMB(Recording->FileName());
    }
  }
  cChannel *channel = Channels.GetByChannelID(((cRecordingInfo *)Info)->ChannelID());
  std::stringstream sstrChannelName_Date;
  if (channel) {
      // draw recording date string
#if APIVERSNUM < 10721
      sstrChannelName_Date  << trVDR("Channel") << ": " << channel->Number() << " - "
          << std::string(channel->Name()).substr(0,13).c_str() << "      " << *DateString(Recording->start)
          << "   " << *TimeString(Recording->start) ;
  } else
      sstrChannelName_Date << "      " << *DateString(Recording->start) << "   " << *TimeString(Recording->start) ;
#else
      sstrChannelName_Date  << trVDR("Channel") << ": " << channel->Number() << " - "
          << std::string(channel->Name()).substr(0,13).c_str() << "      " << *DateString(Recording->Start())
          << "   " << *TimeString(Recording->Start()) ;
  } else
      sstrChannelName_Date << "      " << *DateString(Recording->Start()) << "   " << *TimeString(Recording->Start()) ;
#endif

  const char * priority;
#if APIVERSNUM < 10721
  switch (Recording->priority) {
     case 10: priority = trVDR("low");               break;
     case 50: priority = trVDR("normal");            break;
     case 99: priority = trVDR("high");              break;
     default: priority = *itoa(Recording->priority); break;
#else
  switch (Recording->Priority()) {
     case 10: priority = trVDR("low");               break;
     case 50: priority = trVDR("normal");            break;
     case 99: priority = trVDR("high");              break;
     default: priority = *itoa(Recording->Priority()); break;
#endif

  }

  if (dirSize >= 0)
    sstrInfo << tr("Size") << ": " << std::setprecision(3) << (dirSize > 1023 ? dirSize / 1024.0 : dirSize) << (dirSize > 1023 ? "GB\n" : "MB\n");
  sstrInfo << trVDR("Priority") << ": " << priority << std::endl << trVDR("Lifetime") << ": ";
#if APIVERSNUM < 10721
  sstrInfo << (Recording->lifetime == 99 ? trVDR("unlimited") : *itoa(Recording->lifetime)) << std::endl;
#else
  sstrInfo << (Recording->Lifetime() == 99 ? trVDR("unlimited") : *itoa(Recording->Lifetime())) << std::endl;
#endif
  if (Info->Aux()) {
    sstrInfo << std::endl << tr("Auxiliary information") << ":\n" << parseaux(Info->Aux());
  }
  int xs = xDateRight - Gap;
  const cComponents *Components = Info->Components();
  if (Components) {
    std::stringstream info;
    for (int i = 0; i < Components->NumComponents(); i++) {
      const tComponent *p = Components->Component(i);
      if ((p->stream == 2) && p->language) {
        if (p->description)
          info << p->description << " (" << p->language << "), ";
        else
          info << p->language << ", ";
      }
    }
    /* strip out the last delimiter */
    if (!info.str().empty()) {
      sstrInfo << tr("Languages") << ": " << info.str().substr(0, info.str().length() - 2);
    }
  }

  int yHeadlineBottom = yDateBottom + pFontDetailsTitle->Height();
  int xHeadlineRight  = xInfoRight;
  const char *Title = Info->Title();
  if (isempty(Title))
    Title = Recording->Name();
  //osd->DrawRectangle(xItemLeft, yBodyTop, xHeadlineRight - 1, yHeadlineBottom - 1, Theme.Color(clrAltBackground));

  /* draw background */
  if (!alreadyCleared) {
    osd->RestoreRegion();
  }

  int th = pFontDetailsTitle->Height() + (!isempty(Info->Description()) && !isempty(Info->ShortText()) ? Gap + pFontDetailsSubtitle->Height() : 0);
  int y = yBodyTop + (yHeadlineBottom - yBodyTop - th) / 2;

  /* draw recording title */
  osd->DrawText(xItemLeft + Gap, y, Title, Theme.Color(clrRecTitleFg), clrTransparent,
                pFontDetailsTitle, xHeadlineRight - xItemLeft - Gap - 1 - Roundness, pFontDetailsTitle->Height());

  osd->DrawText(xItemLeft + Gap, yHeadlineBottom, sstrChannelName_Date.str().c_str(),
                Theme.Color(clrRecDateFg), clrTransparent, pFontDetailsDate, xs - xItemLeft - Gap - 1 - Roundness, pFontDetailsDate->Height());
  /* draw recording short text and description */
  const char* strDescr = NULL;
  if (isempty(Info->Description())) {
    if (!isempty(Info->ShortText())) { /* check if short text */
      strDescr = Info->ShortText(); /* draw short text as description, if no description available */
    }
  } else {
    if (!isempty(Info->ShortText())) { /* check if short text */
      y += pFontDetailsTitle->Height() + Gap; /* draw below Title */
      /* draw short text */
      osd->DrawText(xItemLeft + Gap, y, Info->ShortText(), Theme.Color(clrMenuItemNotSelectableFg), clrTransparent,
                    pFontDetailsSubtitle, xHeadlineRight - xItemLeft - Gap - Roundness - 1, pFontDetailsSubtitle->Height());
    }
    strDescr = Info->Description(); /* draw description */
  }

  std::string stringInfo = sstrInfo.str();
  const char *strInfo = stringInfo.empty() ? NULL : stringInfo.c_str();

  if (strDescr || strInfo) {
    y = yHeadlineBottom + SmallGap + 2 * pFontDetailsDate->Height();
    std::stringstream mytext;

    if (ReelConfig.showAuxInfo) {
      if(strDescr)
         mytext << strDescr;
      if(strInfo && strDescr)
        mytext <<  std::endl << std::endl;
      if(strInfo)
        mytext << strInfo;
    } else {
      if(strInfo)
         mytext << strInfo;
      if(strInfo && strDescr)
         mytext << std::endl << std::endl;
      if(strDescr)
         mytext << strDescr;
    }

    if(PngPath.size())
        DrawImage((char *)PngPath.c_str(), 'P' /* R=raw, P=png, J=jpeg */, RECORDINGINFO_IMAGE_XPOS, RECORDINGINFO_IMAGE_YPOS, RECORDINGINFO_IMAGE_RESIZE_WIDTH);


    /** clear the complete right part, there could have been a big scrollbar from the previous menu */
    ClearBg(osd, xDateRight - Gap - ScrollbarWidth-5, yBodyTop, xDateRight - Gap - ScrollbarWidth-5, yBodyTop, xDateRight - Gap, yBodyBottom );
    textScroller.Set(osd, xItemLeft, y, xInfoRight - SmallGap- ScrollbarWidth - xItemLeft - 20 ,
                     yBodyBottom - y, mytext.str().c_str(), pFontDetailsText, Theme.Color(clrMenuTxtFg), Theme.Color(clrBackground));
    SetScrollbar();
  }
}

bool cSkinReelDisplayMenu::DrawImage(const char *sPath, char cType /* R=raw, P=png, J=jpeg */, int x, int y, int resize_x)
{
    cPngUtils pic;
    bool bPic = false;

    switch(cType)
    {
	case 'R':
    	    bPic = pic.ReadRawBitmap(sPath);
	    break;
	case 'P':
    	    bPic = pic.ReadPng (sPath);
	    break;
	case 'J':
    	    bPic = pic.ReadJpeg (sPath);
	    break;
	default:
	    esyslog("skinreel3: Drawimage: unkkown type %c for image %s", cType, sPath);
	    return false;
    }
    if (bPic)
    {

#ifndef RBMINI
	pic.resize_image(resize_x);
#else
	pic.resize_image_nearest_neighbour(resize_x);	// more performance, less quality
#endif

        int nImageIdPos = 0;

        // Draw underlying frame
        osd->DrawRectangle(x - 2, y - 2, x + 2 + pic.Width(), y + 2 +  pic.Height(), Theme.Color(clrLogoBg));
        osd->DrawRectangle(x - 1, y - 1, x + 1 + pic.Width(),y + 1 +  pic.Height(), Theme.Color(clrTitleBg));

        pic.WriteFileOsd ( osd, 250, x, y, 0, &nImageIdPos, /* true */ true);
        return true;
    } else
	return false;

}

void cSkinReelDisplayMenu::SetText(const char *Text, bool FixedFont) {
  /* draw text */
  textScroller.Set(osd, xItemLeft, yBodyTop + Gap, GetTextAreaWidth()-15, yBodyBottom - Gap - yBodyTop - Gap, Text,
                   GetTextAreaFont(FixedFont), Theme.Color(clrMenuTxtFg), Theme.Color(clrBackground));
  SetScrollbar();
}

inline int cSkinReelDisplayMenu::GetTextAreaWidth(void) const {
  /* max text area width */
  return xInfoRight - Gap - SmallGap - ScrollbarWidth - SmallGap -xItemLeft;
}

inline const cFont *cSkinReelDisplayMenu::GetTextAreaFont(bool FixedFont) const {
  /* text area font */
  return FixedFont ? ReelConfig.GetFont(FONT_FIXED) : ReelConfig.GetFont(FONT_LISTITEM);
}

int cSkinReelDisplayMenu::getDateWidth(const cFont *pFontDate) {
  /* only called from constructor, so pFontDate should be OK */
  int w = MIN_DATEWIDTH;
  struct tm tm_r;
  time_t t = time(NULL);
  tm *tm = localtime_r(&t, &tm_r);

  int nWeekday = tm->tm_wday;
  if (0 <= nWeekday && nWeekday < 7)
    w = std::max(w, pFontDate->Width(WEEKDAY(nWeekday)));

  char temp[32];
  strftime(temp, sizeof(temp), "%d.%m.%Y", tm);
  w = std::max(w, pFontDate->Width(temp));

  cString time = TimeString(t);
  w = std::max(w, pFontDate->Width(time));

  return w;
}

/* Draws all little-icons after clearing the bottom 50 pixels of the menu!*/
void cSkinReelDisplayMenu::DrawLittleIcons()
{
    if (LittleIcons.empty()) return ;

    /*Clear bottom 50px of the menu */
    ClearBg( osd, xItemLeft,yBodyTop,   xItemLeft, yBodyBottom - 50, xItemRight, yBodyBottom );

    std::map<int, cLittleIconInfo> :: iterator it = LittleIcons.begin(); // show last 4 recordings only ;LIFO
    int x = xItemLeft + 4;
    std::string iconName;
    int count_recording = 0;

    for(; it != LittleIcons.end(); ++it)
    {
        switch( it->second.Type() )
        {
            case BACKGROUND_PROCESS_ICON:
                iconName = "slot6_";
                break;

            case PLAY_MEDIA_ICON:
                iconName = "slot5_";
                break;

            case STOP_REPLAY_ICON:
                if(it->second.Selected())
                   iconName = "slot5_active_stop.png";
                else
                   iconName = "slot5_inactive.png";
                break;

            case STOP_RECORDING_ICON:
            default:
                iconName = "slot1_" ;
                iconName.at(4) = char('1' + count_recording);
                ++count_recording;
                if (count_recording>3)  // only 4 icons for 'stop recording' available
                    count_recording = 0;
                break;
        }
        if(it->second.Type() != STOP_REPLAY_ICON)
          iconName += it->second.Selected()?"active.png" : "inactive.png";

        DrawUnbufferedImage(osd, iconName.c_str(), x , yBodyBottom - 49, true);
        //printf("%i drew %s for type %i\n", it->first, iconName.c_str(),it->second.Type()  );
        x += 48 + 4;
    }

}

void cSkinReelDisplayMenu::Flush(void) {

    //std::cout << "FLUSH : " << LittleIcons.size()<< std::endl;

    /* Draw all little icons*/
    if (strTitle && strcasestr(strTitle, trVDR("Main Menu")) != NULL )// only in mainmenu
        DrawLittleIcons();

    /* clear Little icon list*/
    //LittleIcons.clear();

    cString date = GetFullDateTime();
    if ((strLastDate == NULL) || strcmp(strLastDate, (const char*)date) != 0) {

        free(strLastDate);
        strLastDate = strdup((const char*)date);
        const cFont *pFontDate = ReelConfig.GetFont(FONT_DATE);
        osd->DrawImage(imgMenuHeaderCenterX, xDateLeft, 0, false, xDateRight - xDateLeft -Roundness,1); // to clean before drawing date
        osd->DrawText(xDateLeft + SmallGap, yDateTop /*+ 5*/, date, Theme.Color(clrMenuTxtFg),
                  clrTransparent, pFontDate, xDateRight - xDateLeft - SmallGap - Roundness,
                  yTitleDecoBottom - yDateTop - 2, taRight);
    }

#ifdef THUMB_LEFT_SIDE
#define TX 20
#define TY 50
#define TW 100
#define TH 100
#define BCOLOR 0xaaffffff
#else
#define TX 450
#define TY 75
#define TW 150
#define TH 150
#define BCOLOR 0x00000000
#endif
#define BW 2

    //if(MenuType == menuImage){
#ifndef RBMINI
    if (rbPlugin == NULL)
        rbPlugin = cPluginManager::GetPlugin("reelbox");
    if (rbPlugin == NULL)
        rbPlugin = cPluginManager::GetPlugin("reelice");
#else
    if (rbPlugin == NULL)
        rbPlugin = cPluginManager::GetPlugin("rbmini");
#endif
    static struct imgSize picSize[NR_UNBUFFERED_SLOTS];

    for(int i = 0; i<NR_UNBUFFERED_SLOTS; i++) {
        if (strncmp(ReelConfig.curThumb[i].path, oldThumb[i], 512)) { /* if thumb has changed */
            if (strncmp(ReelConfig.curThumb[i].path, "", 512)) { /* if the path isn't empty */
#ifdef DEBUG_OSD
                std::cout << "i: " << i << " showing PIC: " << ReelConfig.curThumb[i].path << std::endl;
#endif
                strncpy((char*)&oldThumb[i], (const char*)&ReelConfig.curThumb[i].path, 512);
                osd->SetImagePath(imgUnbufferedEnum+i, ReelConfig.curThumb[i].path);
                /* only clear & draw border if called by filebrowser - that means with x==0 and y==0 */
#ifdef THUMB_LEFT_SIDE
                if (!ReelConfig.curThumb[i].x && !ReelConfig.curThumb[i].y) {
                    /** clear previous image, since the TitleImages are blended: they get 'brighter' with every overdraw without a clear */
                    //osd->DrawRectangle( 0, yBodyTop, IMGMENUWIDTH, yBodyTop+TY+TH+2*BW, clrTransparent);
                    /** draw background */
                    //TB: TODO osd->DrawRectangle( 0, yBodyTop, IMGMENUWIDTH, yBodyTop+TY+TH+2*BW,  Theme.Color(clrBackground) );
                    osd->DrawImage(imgImagesTitleTopX, 0, yBodyTop, false, 1, yBodyBottom - yBodyTop - IMGMENUHEIGHT*2 + 16  /* TB: todo fix 16 */); // repeat this image vertically
#endif
                    if (!ReelConfig.curThumb[i].x && !ReelConfig.curThumb[i].y) {
                        osd->DrawImage(imgUnbufferedEnum+i, TX, TY, ReelConfig.curThumb[i].blend);
                    } else
                        osd->DrawImage(imgUnbufferedEnum+i, ReelConfig.curThumb[i].x, ReelConfig.curThumb[i].y, ReelConfig.curThumb[i].blend);

                    /* only draw border if called by filebrowser - that means with x==0 and y==0 */
#ifndef THUMB_LEFT_SIDE
                    if (!ReelConfig.curThumb[i].x && !ReelConfig.curThumb[i].y) {
                        ClearBg(osd, TX-BW, yBodyTop, TX-BW, TY-BW, TX+TW+BW+1, TY+TH+BW);
                    }
#endif
                    picSize[i].slot = i;
                    if(!ReelConfig.curThumb[i].w && !ReelConfig.curThumb[i].h) {
                        if (rbPlugin) rbPlugin->Service("GetUnbufferedPicSize", &picSize[i]);
                    } else {
                        picSize[i].height = ReelConfig.curThumb[i].h;
                        picSize[i].width = ReelConfig.curThumb[i].w;
                    }
#ifdef DEBUG_OSD
                    std::cout << "i: " << i << " SIZE: w: " << picSize[i].width << " h: " << picSize[i].height << std::endl;
#endif

#ifndef THUMB_LEFT_SIDE
                    if (!ReelConfig.curThumb[i].x && !ReelConfig.curThumb[i].y)
                        osd->DrawImage(imgUnbufferedEnum+i, TX + TW - picSize[i].width, TY, ReelConfig.curThumb[i].blend);
                    else {
                       if(picSize[i].width && picSize[i].height) {
                          ClearBg(osd, ReelConfig.curThumb[i].x, yBodyTop, ReelConfig.curThumb[i].x, ReelConfig.curThumb[i].y, ReelConfig.curThumb[i].x + picSize[i].width, ReelConfig.curThumb[i].y + picSize[i].height);
                        }
                        osd->DrawImage(imgUnbufferedEnum+i, ReelConfig.curThumb[i].x, ReelConfig.curThumb[i].y, ReelConfig.curThumb[i].blend);
                    }
                    if (!ReelConfig.curThumb[i].x && !ReelConfig.curThumb[i].y) {
                        /* draw border */
                        if (picSize[i].width && picSize[i].height) {
                            osd->DrawRectangle(TX - BW + TW - picSize[i].width, TY - BW, TX + TW, TY, BCOLOR); /* top */
                            osd->DrawRectangle(TX - BW + TW - picSize[i].width, TY - BW, TX + TW - picSize[i].width, TY + picSize[i].height, BCOLOR); /* left */
                            osd->DrawRectangle(TX + picSize[i].width + TW - picSize[i].width, TY - BW, TX + BW + TW, TY + picSize[i].height, BCOLOR); /* right */
                            osd->DrawRectangle(TX - BW + TW - picSize[i].width, TY + picSize[i].height, TX + BW + TW, TY + BW + picSize[i].height, BCOLOR); /* bottom */
                        }
                    } else if (ReelConfig.curThumb[i].w == 125 && ReelConfig.curThumb[i].h == 125 &&
                               ReelConfig.curThumb[i].x == 22 && ReelConfig.curThumb[i].y == 315 ) { /** mp3-cover: at (22,315) and (125,125) big */
                            #define BCOLOR1 Theme.Color(clrMenuTxtFg)
                            osd->DrawRectangle(20, 313, 147, 314, BCOLOR1); /* top */
                            osd->DrawRectangle(20, 313, 21, 438, BCOLOR1); /* left */
                            osd->DrawRectangle(147, 313, 148, 440, BCOLOR1); /* right */
                            osd->DrawRectangle(20, 439, 147, 440, BCOLOR1); /* bottom */
                    }
#else
                    if (!ReelConfig.curThumb[i].x && !ReelConfig.curThumb[i].y)
                        osd->DrawImage(imgUnbufferedEnum+i, TX, TY, ReelConfig.curThumb[i].blend);
                    else {
                        if(picSize[i].width && picSize[i].height) {
                            ClearBg(osd, ReelConfig.curThumb[i].x, yBodyTop, ReelConfig.curThumb[i].x, ReelConfig.curThumb[i].y, ReelConfig.curThumb[i].x+picSize[i].width, ReelConfig.curThumb[i].y + picSize[i].height);
                        }
                        osd->DrawImage(imgUnbufferedEnum+i, ReelConfig.curThumb[i].x, ReelConfig.curThumb[i].y, ReelConfig.curThumb[i].blend);
                    }
                    if (!ReelConfig.curThumb[i].x && !ReelConfig.curThumb[i].y) {
                        /* draw border */
                        if (picSize[i].width && picSize[i].height) {
                            osd->DrawRectangle(TX - BW, TY - BW, TX + picSize[i].width, TY, BCOLOR); /* top */
                            osd->DrawRectangle(TX - BW, TY - BW, TX, TY + picSize[i].height, BCOLOR); /* left */
                            osd->DrawRectangle(TX + picSize[i].width, TY - BW, TX + BW + picSize[i].width, TY + picSize[i].height, BCOLOR); /* right */
                            osd->DrawRectangle(TX - BW, TY + picSize[i].height, TX + BW + picSize[i].width, TY + BW + picSize[i].height, BCOLOR); /* bottom */
                        }
                    }
#endif
                    thumbCleared[i] = 0;
                } else {
                    strcpy((char*)&oldThumb[i], "");
                    if (!thumbCleared[i]) {
#ifdef THUMB_LEFT_SIDE
                        if (!ReelConfig.curThumb[i].x && !ReelConfig.curThumb[i].y) {
                            /** clear previous image, since the TitleImages are blended: they get 'brighter' with every overdraw without a clear */
                            //osd->DrawRectangle(0, yBodyTop, IMGMENUWIDTH, yBodyTop+TY+TH+2*BW, clrTransparent);
                            /** draw background */
                            //osd->DrawRectangle(0, yBodyTop, IMGMENUWIDTH, yBodyTop+TY+TH+2*BW,  Theme.Color(clrBackground) );
                            osd->DrawImage(imgImagesTitleTopX, 0, yBodyTop, false, 1, yBodyBottom - yBodyTop - IMGMENUHEIGHT*2 + 16); // repeat this image vertically
                        }
#else
                        if (!ReelConfig.curThumb[0].x && !ReelConfig.curThumb[0].y && !ReelConfig.curThumb[1].x && !ReelConfig.curThumb[1].y) {
                            //osd->DrawRectangle(TX - BW, TY - BW, TX + TW + BW, TY + TH + BW, Theme.Color(clrBackground));
                            ClearBg(osd, TX-BW, yBodyTop, TX-BW, TY-BW, TX+TW+BW+1, TY+TH+BW);
                        }
#endif
                        thumbCleared[i] = 1;
                    }
                }
            }
        }

        alreadyCleared = false;

        // for newer versions of vdr, use SideNote(const StringList*) with
        // ID3TAGS as first element
#if APIVERSNUM < 10700
        PrintID3Infos();
#endif

        osd->Flush();
#if 0 //def DEBUG_TIMES
        struct timeval now;
        gettimeofday(&now, NULL);
        float secs2 = ((float)((1000000 * now.tv_sec + now.tv_usec) - (menuTimeVal1.tv_sec * 1000000 + menuTimeVal1.tv_usec))) / 1000000;
        printf("\n=== cSkinReelDisplayMenu::Flush time since key: secs: %f\n", secs2);
#endif

}

void cSkinReelDisplayMenu::PrintID3Infos() {

    if (MenuType == menuEmptyBanner || MenuType == menuHalfEmptyBanner) {
        //printf("XXXXXXXX: %i ID3tags: \n", ID3Tags.size());
        DrawTitleImage();
        int y = 70;
        for (unsigned int i = 0; i < ID3Tags.size(); i++) {
            // Tag #1, Title, is shown in different font and color
            const cFont *font = ( (i==0)?
                        ReelConfig.GetFont(FONT_OSDTITLE):
                                       ReelConfig.GetFont(FONT_CISUBTITLE));
            //printf("Tag %i: \"%s\" \"%s\"\n", i, ID3Tags.at(i).name.c_str(), ID3Tags.at(i).value.c_str());
#ifdef ORIENTED_LEFT
            osd->DrawText(22, 50 + i*40, ID3Tags.at(i).name.c_str(), Theme.Color(clrMenuTxtFg), clrTransparent, font);
            osd->DrawText(22, 70 + i*40, ID3Tags.at(i).value.c_str(), Theme.Color(clrMenuTxtFg), clrTransparent, font);
#else
            if (font->Width(ID3Tags.at(i).value.c_str()) > 120) {
                for (unsigned int j = 0; j < 3; j++)
                    ID3Tags.at(i).value.at(ID3Tags.at(i).value.size()-1-j) = '.';
                while (font->Width(ID3Tags.at(i).value.c_str()) > 120)
                    ID3Tags.at(i).value.erase(ID3Tags.at(i).value.size()-4, 1);
            }
            int x = 145 - std::min(font->Width(ID3Tags.at(i).value.c_str()), 120);
            //osd->DrawText(22, 50 + i*40, ID3Tags.at(i).name.c_str(), Theme.Color(clrMenuTxtFg), clrTransparent, font);
            osd->DrawText(x, y,
                          ID3Tags.at(i).value.c_str(),
                          // title has a different color
                          i==0?Theme.Color(clrRecTitleFg):Theme.Color(clrMenuTxtFg),
                          clrTransparent,
                          font,
                          std::min(font->Width(ID3Tags.at(i).value.c_str()), 120)
                          );

            // leave a larger vertical gap after title (i==0)
            y += font->Height() * ( 1 + 0.5 *(i==0));
#endif
        }// for
    } // if
    else {
        printf("Wrong menu type: %d. Expecting %d OR %d\n", MenuType, menuEmptyBanner,
               menuHalfEmptyBanner);
    }

}

void cSkinReelDisplayMenu::SetScrollbar(int Total, int Offset) {
  /* avoid division by zero */
  if (Total == 0)
       Total = 1;

  int yScrollbarTop = yItemTop;
  int yScrollbarBottom = yBodyBottom - SmallGap;
  int xScrollbarRight = xDateRight - Gap;
  int xScrollbarLeft = xScrollbarRight - ScrollbarWidth ;
  int yScrollAreaTop = yScrollbarTop + ScrollbarHeight;
  int yScrollAreaBottom = yScrollbarBottom - ScrollbarHeight;
  int ScrollAreaHeight = yScrollAreaBottom - yScrollAreaTop;
  int xScrollAreaRight = xScrollbarRight;
  int xScrollAreaLeft = xScrollAreaRight - SmallGap;
  int yScrollerTop = yScrollAreaTop + (Offset * ScrollAreaHeight) / Total;
  int yScrollerBottom = yScrollerTop + (MaxItems() * ScrollAreaHeight) / Total;

  /* avoid a "active scrollbar" size of 0 */
  if(yScrollerBottom-yScrollerTop == 0)
    yScrollerBottom++;
  //std::cout << "SetScrollbar: title: " << strTitle << " total: " << Total << " Offset: " << Offset << " Diff: " << yScrollerBottom-yScrollerTop << std::endl;

  if(strcmp(strTitle, trVDR("Main Menu")) && Total > MaxItems()) {
    /* disabling this fixes the scrollbar with menuorg-plugin at least for vanilla vdr */
#ifdef REELVDR
    /* draw background of scrollbar (Clear area) */
    if (!alreadyCleared) {
      osd->DrawImage(imgMenuBodyUpperPart, xScrollbarLeft, yBodyTop, false, xScrollbarRight-xScrollbarLeft, 1);
      osd->DrawImage(imgMenuBodyLowerPart, xScrollbarLeft, yBodyTop+205, false, xScrollbarRight-xScrollbarLeft, 1);
    }
#endif

    /* arrow up */
    DrawUnbufferedImage(osd, "scrollbar_top.png", xScrollbarLeft, yScrollbarTop, true);

    /* draw background of the scrollbar */
    DrawUnbufferedImage(osd, "scrollbar_inactive_x.png", xScrollAreaLeft-15, yScrollbarTop+19, true, 1, yScrollerTop-yScrollbarTop-19);
    DrawUnbufferedImage(osd, "scrollbar_inactive_x.png", xScrollAreaLeft-15, yScrollerBottom, true, 1, yScrollbarBottom-yScrollerBottom-19);

    /* draw visible area of scrollbar */
    DrawUnbufferedImage(osd, "scrollbar_active_x.png", xScrollAreaLeft-15, yScrollerTop, true, 1, yScrollerBottom-yScrollerTop);

    /* arrow down */
    DrawUnbufferedImage(osd, "scrollbar_bottom.png", xScrollbarLeft, yScrollbarBottom-19, true);
  }
}

