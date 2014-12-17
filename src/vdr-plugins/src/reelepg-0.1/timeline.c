/*
 * timeline.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 *
 */
#include "timeline.h"
#include "ReelSkin.h"
#include "../epgsearch/services.h"
#include <vdr/menu.h>
#include <vdr/plugin.h>
#include <vdr/themes.h>
#include <vdr/status.h>
#include <vdr/help.h>
#include <time.h>
#include "common.h"

// #include <vdr/interface.h>
#include "pngutils.h"
#include <list>
#include "service.h"

#ifdef USE_TFT
#include "font.h"
cGraphtftFont gFontCache;
int gLastFontSize = -1;
#endif
#define TFT_TITLE "NgTitleFont"


#define SWITCHTIMER

#define DEFAULT_OPT_PATH "/opt/lib/vdr/img"
#define DEFAULT_IMG_SKINREEL3_PATH "/usr/share/reel/skinreel3"
#define DEFAULT_BASE "/usr/share/reel"
//TODO: Is DEFAULT_NG_BASE correct?? Or should it be like DEFAULT_BASE (for Lite)
#define DEFAULT_NG_BASE DEFAULT_IMG_PATH"ng"
#define DEFAULT_SKINREEL3_BASE DEFAULT_IMG_PATH"3"
#define DEFAULT_NG_IMG_PATH DEFAULT_IMG_PATH"ng/Blue"
#define DEFAULT_SKINREEL3_IMG_PATH DEFAULT_IMG_PATH"3/Blue"



#define IDLE_TIMEOUT 	(5*60)
#define SWITCH_START 1

#define CLR_TRANSPARENT_NOT_0 0x00FFFFFF

#define SKIN_SKINREELNG 1
#define SKIN_SKINREEL3 2

#define GET_CLR(dName) Skins.Current()->Theme()->Color(dName)
#define DEFINE_CLR(dName) const char *dName=#dName
#define ADD_THEME_CLR(dName, dColor, dDefault) DBG(DBG_FN,"ADD_THEME_CLR %s %d", dName, Skins.Current()->Theme()->Color(dName)); if(!Skins.Current()->Theme()->Color(dName)) { \
                                      if(dDefault && GET_CLR((char *)dDefault)) \
                                      	Skins.Current()->Theme()->AddColor(dName, GET_CLR((char *)dDefault)); \
                                      else \
                                      	Skins.Current()->Theme()->AddColor(dName, dColor); \
                                    }
#define SET_THEME_CLR(dName, dColor, dDefault) \
                                      if(dDefault && GET_CLR((char *)dDefault)) \
										Skins.Current()->Theme()->AddColor(dName, GET_CLR((char *)dDefault)); \
									  else \
										Skins.Current()->Theme()->AddColor(dName, dColor)

DEFINE_CLR(themeClrDateBg);
DEFINE_CLR(themeClrDateTxt);
DEFINE_CLR(themeClrScaleBg);
DEFINE_CLR(themeClrScaleTxt);
DEFINE_CLR(themeClrDetailLeftBg);
DEFINE_CLR(themeClrDetailLeftTxt);
DEFINE_CLR(themeClrDetailRightBg);
DEFINE_CLR(themeClrDetailRightTxt);
DEFINE_CLR(themeClrChannelNormalBg);
DEFINE_CLR(themeClrChannelNormalTxt);
DEFINE_CLR(themeClrChannelSelectBg);
DEFINE_CLR(themeClrChannelSelectTxt);
DEFINE_CLR(themeClrChannelsBg);
DEFINE_CLR(themeClrItemNormalBg);
DEFINE_CLR(themeClrItemNormalTxt);
DEFINE_CLR(themeClrItemSelectBg);
DEFINE_CLR(themeClrItemSelectTxt);
DEFINE_CLR(themeClrItemRecordNormalBg);
DEFINE_CLR(themeClrItemRecordNormalTxt);
DEFINE_CLR(themeClrItemRecordSelectBg);
DEFINE_CLR(themeClrItemRecordSelectTxt);
DEFINE_CLR(themeClrItemSwitchNormalBg);
DEFINE_CLR(themeClrItemSwitchNormalTxt);
DEFINE_CLR(themeClrItemSwitchSelectBg);
DEFINE_CLR(themeClrItemSwitchSelectTxt);
DEFINE_CLR(themeClrTxtShadow);
DEFINE_CLR(themeClrTimeScaleBg);
DEFINE_CLR(themeClrTimeScaleFg);
DEFINE_CLR(themeClrTimeNowBg);
DEFINE_CLR(themeClrTimeNowFg);
DEFINE_CLR(themeClrItemsBg);
DEFINE_CLR(themeClrButtonRedFg);
DEFINE_CLR(themeClrButtonGreenFg);
DEFINE_CLR(themeClrButtonYellowFg);
DEFINE_CLR(themeClrButtonBlueFg);

DEFINE_CLR(themeClrDateFrame);
DEFINE_CLR(themeClrScaleFrame);
DEFINE_CLR(themeClrDetailLeftFrame);
DEFINE_CLR(themeClrDetailRightFrame);
DEFINE_CLR(themeClrChannelNormalFrame);
DEFINE_CLR(themeClrChannelSelectFrame);
DEFINE_CLR(themeClrChannelsFrame);
DEFINE_CLR(themeClrItemsFrame);
DEFINE_CLR(themeClrItemNormalFrame);
DEFINE_CLR(themeClrItemSelectFrame);
DEFINE_CLR(themeClrRecordNormalFrame);
DEFINE_CLR(themeClrRecordSelectFrame);
DEFINE_CLR(themeClrSwitchNormalFrame);
DEFINE_CLR(themeClrSwitchSelectFrame);

DEFINE_CLR(themeOptScaleFrame);
DEFINE_CLR(themeOptItemsFrame);
DEFINE_CLR(themeOptChannelsFrame);
DEFINE_CLR(themeOptShowDate);
DEFINE_CLR(themeOptShowTime);
DEFINE_CLR(themeOptShowSR);
DEFINE_CLR(themeOptShowRN);

DEFINE_CLR(themeOptFrameLeft);
DEFINE_CLR(themeOptFrameTop);
DEFINE_CLR(themeOptFrameRight);
DEFINE_CLR(themeOptFrameBottom);


tFrameInfo gDateFrame_Lite             = {imgBlue1        , themeClrDateFrame,          themeClrDateBg            , themeClrDateTxt            , false, true, false, 11, 11, 11, 11, imgBlue1Width, imgBlue3Width, imgBlue1Height, imgBlue7Height};
tFrameInfo gScaleFrame_Lite            = {imgBlue1        , themeClrScaleFrame,         themeClrScaleBg           , themeClrScaleTxt           , false, true, false, 11, 11, 11, 11, imgBlue1Width, imgBlue3Width, imgBlue1Height, imgBlue7Height};
tFrameInfo gDetailLeftFrame_Lite       = {imgTeal1        , themeClrDetailLeftFrame,    themeClrDetailLeftBg      , themeClrDetailLeftTxt      , false, true, false, 11,  5,  6, 16, imgBlue1Width, imgBlue3Width, imgBlue1Height, imgBlue7Height};
tFrameInfo gDetailRightFrame_Lite      = {imgBlue1        , themeClrDetailRightFrame,   themeClrDetailRightBg     , themeClrDetailRightTxt     , false, true, false,  5, 11,  6, 16, imgBlue1Width, imgBlue3Width, imgBlue1Height, imgBlue7Height};
tFrameInfo gChannelNormalFrame_Lite    = {imgBlue1        , themeClrChannelNormalFrame, themeClrChannelNormalBg   , themeClrChannelNormalTxt   , false, true, false, 11, 11, 11, 11, imgBlue1Width, imgBlue3Width, imgBlue1Height, imgBlue7Height};
tFrameInfo gChannelSelectFrame_Lite    = {imgTeal1        , themeClrChannelSelectFrame, themeClrChannelSelectBg   , themeClrChannelSelectTxt   , false, true, false, 11, 11, 11, 11, imgBlue1Width, imgBlue3Width, imgBlue1Height, imgBlue7Height};
tFrameInfo gChannelsFrame_Lite         = {imgBlue1        , themeClrChannelsFrame,      themeClrChannelsBg        , themeClrScaleTxt           , false, true, false,  9, 12, 11, 11, imgBlue1Width, imgBlue3Width, imgBlue1Height, imgBlue7Height};
tFrameInfo gItemsFrame_Lite            = {imgBlue1        , themeClrItemsFrame,         themeClrItemsBg           , themeClrScaleTxt           , false, true, false,  9, 12, 10, 12, imgBlue1Width, imgBlue3Width, imgBlue1Height, imgBlue7Height};
tFrameInfo gItemNormalFrame_Lite       = {imgSmallBlue1   , themeClrItemNormalFrame,    themeClrItemNormalBg      , themeClrItemNormalTxt      , true, false, true,   5,  5,  5,  5, 5, 6, 5, 6};
tFrameInfo gItemSelectFrame_Lite       = {imgSmallSBlue1  , themeClrItemSelectFrame,    themeClrItemSelectBg      , themeClrItemSelectTxt      , true, false, true,   5,  5,  5,  5, 5, 6, 5, 6};
tFrameInfo gItemRecordNormalFrame_Lite = {imgSmallRed1    , themeClrRecordNormalFrame,  themeClrItemRecordNormalBg, themeClrItemRecordNormalTxt, true, false, true,   5,  5,  5,  5, 5, 6, 5, 6};
tFrameInfo gItemRecordSelectFrame_Lite = {imgSmallSRed1   , themeClrRecordSelectFrame,  themeClrItemRecordSelectBg, themeClrItemRecordSelectTxt, true, false, true,   5,  5,  5,  5, 5, 6, 5, 6};
tFrameInfo gItemSwitchNormalFrame_Lite = {imgSmallYellow1 , themeClrSwitchNormalFrame,  themeClrItemSwitchNormalBg, themeClrItemSwitchNormalTxt, true, false, true,   5,  5,  5,  5, 5, 6, 5, 6};
tFrameInfo gItemSwitchSelectFrame_Lite = {imgSmallSYellow1, themeClrSwitchSelectFrame,  themeClrItemSwitchSelectBg, themeClrItemSwitchSelectTxt, true, false, true,   5,  5,  5,  5, 5, 6, 5, 6};

tFrameInfo gDateFrame_NG             = {imgBlue1        , themeClrDateFrame,          themeClrDateBg            , themeClrDateTxt            , false, true, false, 11, 11, 4, 4, 10, 12, 25, 0};
tFrameInfo gScaleFrame_NG            = {imgBlue1        , themeClrScaleFrame,         themeClrScaleBg           , themeClrScaleTxt           , false, true, false, 11, 11, 4, 4, 10, 10, 25, 0};
tFrameInfo gDetailLeftFrame_NG       = {imgBlue1        , themeClrDetailLeftFrame,    themeClrDetailLeftBg      , themeClrDetailLeftTxt      , false, true, false, 11, 11, 4, 4, 10, 12, 0, 30};
tFrameInfo gDetailRightFrame_NG      = {imgBlue1        , themeClrDetailRightFrame,   themeClrDetailRightBg     , themeClrDetailRightTxt     , false, true, false, 11, 11, 4, 4, 10, 10, 0, 30};
tFrameInfo gChannelNormalFrame_NG    = {imgBlue1        , themeClrChannelNormalFrame, themeClrChannelNormalBg   , themeClrChannelNormalTxt   , false, true, false, 4, 4, 4, 4, 10, 12, 30, 0};
tFrameInfo gChannelSelectFrame_NG    = {imgBlue1        , themeClrChannelSelectFrame, themeClrChannelSelectBg   , themeClrChannelSelectTxt   , false, true, false, 4, 4, 4, 4, 10, 12, 30, 0};
tFrameInfo gChannelsFrame_NG         = {imgBlue1        , themeClrChannelsFrame,      themeClrChannelsBg        , themeClrScaleTxt           , false, true, false,  0, 0, 1, 1, 0, 0, 0, 0};
tFrameInfo gItemsFrame_NG            = {imgBlue1        , themeClrItemsFrame,         themeClrItemsBg           , themeClrScaleTxt           , false, true, false,  0, 0, 1, 1, 0, 0, 0, 0};
tFrameInfo gItemNormalFrame_NG       = {imgSmallBlue1   , themeClrItemNormalFrame,    themeClrItemNormalBg      , themeClrItemNormalTxt      , true, false, true,   5,  5,  5,  5, 5, 6, 5, 6};
tFrameInfo gItemSelectFrame_NG       = {imgSmallSBlue1  , themeClrItemSelectFrame,    themeClrItemSelectBg      , themeClrItemSelectTxt      , true, false, true,   5,  5,  5,  5, 5, 6, 5, 6};
tFrameInfo gItemRecordNormalFrame_NG = {imgSmallRed1    , themeClrRecordNormalFrame,  themeClrItemRecordNormalBg, themeClrItemRecordNormalTxt, true, false, true,   5,  5,  5,  5, 5, 6, 5, 6};
tFrameInfo gItemRecordSelectFrame_NG = {imgSmallSRed1   , themeClrRecordSelectFrame,  themeClrItemRecordSelectBg, themeClrItemRecordSelectTxt, true, false, true,   5,  5,  5,  5, 5, 6, 5, 6};
tFrameInfo gItemSwitchNormalFrame_NG = {imgSmallYellow1 , themeClrSwitchNormalFrame,  themeClrItemSwitchNormalBg, themeClrItemSwitchNormalTxt, true, false, true,   5,  5,  5,  5, 5, 6, 5, 6};
tFrameInfo gItemSwitchSelectFrame_NG = {imgSmallSYellow1, themeClrSwitchSelectFrame,  themeClrItemSwitchSelectBg, themeClrItemSwitchSelectTxt, true, false, true,   5,  5,  5,  5, 5, 6, 5, 6};

tFrameInfo gDateFrame_Skinreel3             = {imgBlue1        , themeClrDateFrame,          themeClrDateBg            , themeClrDateTxt            , false, true, false, 11, 11, 4, 4, 10, 12, 25, 0};
tFrameInfo gScaleFrame_Skinreel3            = {imgBlue1        , themeClrScaleFrame,         themeClrScaleBg           , themeClrScaleTxt           , false, true, false, 11, 11, 4, 4, 10, 10, 25, 0};
tFrameInfo gDetailLeftFrame_Skinreel3       = {imgBlue1        , themeClrDetailLeftFrame,    themeClrDetailLeftBg      , themeClrDetailLeftTxt      , false, true, false, 11, 11, 4, 4, 10, 12, 0, 30};
tFrameInfo gDetailRightFrame_Skinreel3      = {imgBlue1        , themeClrDetailRightFrame,   themeClrDetailRightBg     , themeClrDetailRightTxt     , false, true, false, 11, 11, 4, 4, 10, 10, 0, 30};
tFrameInfo gChannelNormalFrame_Skinreel3    = {imgBlue1        , themeClrChannelNormalFrame, themeClrChannelNormalBg   , themeClrChannelNormalTxt   , false, true, false, 4, 4, 4, 4, 10, 12, 30, 0};
tFrameInfo gChannelSelectFrame_Skinreel3    = {imgBlue1        , themeClrChannelSelectFrame, themeClrChannelSelectBg   , themeClrChannelSelectTxt   , false, true, false, 4, 4, 4, 4, 10, 12, 30, 0};
tFrameInfo gChannelsFrame_Skinreel3         = {imgBlue1        , themeClrChannelsFrame,      themeClrChannelsBg        , themeClrScaleTxt           , false, true, false,  0, 0, 1, 1, 0, 0, 0, 0};
tFrameInfo gItemsFrame_Skinreel3            = {imgBlue1        , themeClrItemsFrame,         themeClrItemsBg           , themeClrScaleTxt           , false, true, false,  0, 0, 1, 1, 0, 0, 0, 0};
tFrameInfo gItemNormalFrame_Skinreel3       = {imgSmallBlue1   , themeClrItemNormalFrame,    themeClrItemNormalBg      , themeClrItemNormalTxt      , true, false, true,   5,  5,  5,  5, 5, 6, 5, 6};
tFrameInfo gItemSelectFrame_Skinreel3       = {imgSmallSBlue1  , themeClrItemSelectFrame,    themeClrItemSelectBg      , themeClrItemSelectTxt      , true, false, true,   5,  5,  5,  5, 5, 6, 5, 6};
tFrameInfo gItemRecordNormalFrame_Skinreel3 = {imgSmallRed1    , themeClrRecordNormalFrame,  themeClrItemRecordNormalBg, themeClrItemRecordNormalTxt, true, false, true,   5,  5,  5,  5, 5, 6, 5, 6};
tFrameInfo gItemRecordSelectFrame_Skinreel3 = {imgSmallSRed1   , themeClrRecordSelectFrame,  themeClrItemRecordSelectBg, themeClrItemRecordSelectTxt, true, false, true,   5,  5,  5,  5, 5, 6, 5, 6};
tFrameInfo gItemSwitchNormalFrame_Skinreel3 = {imgSmallYellow1 , themeClrSwitchNormalFrame,  themeClrItemSwitchNormalBg, themeClrItemSwitchNormalTxt, true, false, true,   5,  5,  5,  5, 5, 6, 5, 6};
tFrameInfo gItemSwitchSelectFrame_Skinreel3 = {imgSmallSYellow1, themeClrSwitchSelectFrame,  themeClrItemSwitchSelectBg, themeClrItemSwitchSelectTxt, true, false, true,   5,  5,  5,  5, 5, 6, 5, 6};


tFrameInfo gDateFrame;
tFrameInfo gScaleFrame;
tFrameInfo gDetailLeftFrame;
tFrameInfo gDetailRightFrame;
tFrameInfo gChannelNormalFrame;
tFrameInfo gChannelSelectFrame;
tFrameInfo gChannelsFrame;
tFrameInfo gItemsFrame;
tFrameInfo gItemNormalFrame;
tFrameInfo gItemSelectFrame;
tFrameInfo gItemRecordNormalFrame;
tFrameInfo gItemRecordSelectFrame;
tFrameInfo gItemSwitchNormalFrame;
tFrameInfo gItemSwitchSelectFrame;


std::string cTimeLine::mMyFavouriteEvents = "";
// Reelepg_setquery_v1_0 *cTimeLine::Reelepg_setquery;
std::vector<const cEvent *> cTimeLine::lSearchResultEvents;
std::vector<const cEvent *>::iterator cTimeLine::lIterateResultEvents;
bool cTimeLine::mBrowseResultSet;

class cMenuGroups : public cOsdMenu {
	public:
	cMenuGroups(int fChan, cPlugin *lPlugin_ReelChannellist);
}; // cMenuGroups
class cItemGroup : public cOsdItem {
	public:
	cItemGroup(const cChannel *fChan, cPlugin *lPlugin_ReelChannellist);
	const cChannel *mChannel;
}; // cItemGroup


class cMenuEventInfo : public cOsdMenu {
private:
	const cEvent *mEvent;
	cOsdMenu *mMenu;
public:
	cMenuEventInfo(const cEvent *fEvent) :cOsdMenu(tr("Event")) {
		mMenu  = NULL;
		mEvent = fEvent;
		if (!mEvent)
			return;
		cChannel *lChannel = mChannelList->GetByChannelID(fEvent->ChannelID(), true);
		if (!lChannel)
			return;
		SetTitle(lChannel->Name());
		SetHelp(tr("Record"), tr("Search"), NULL, tr("Searchtimer"));
	} // cMenuEventInfo
	virtual ~cMenuEventInfo(void) {
		if (mMenu)
			delete mMenu;
	} // ~cMenuEventInfo
	virtual void Display(void) {
		cOsdMenu::Display();
		DisplayMenu()->SetEvent(mEvent);
		if (mEvent->Description())
			cStatus::MsgOsdTextItem(mEvent->Description());
	} // Display
	virtual eOSState ProcessKey(eKeys fKey) {
		DBG(DBG_FN,"cMenuEventInfo::ProcessKey(%d)",fKey);
		if (mMenu) {
			eOSState lState = mMenu->ProcessKey(fKey);
			if (lState == osBack) {
				lState = osContinue;
				delete mMenu;
				mMenu=NULL;
				Display();
			} // if
			return lState;
		} // if
		switch (fKey) {
			case kUp|k_Repeat:
			case kUp:
			case kDown|k_Repeat:
			case kDown:
			case kLeft|k_Repeat:
			case kLeft:
			case kRight|k_Repeat:
			case kRight:
				DisplayMenu()->Scroll(NORMALKEY(fKey) == kUp || NORMALKEY(fKey) == kLeft, NORMALKEY(fKey) == kLeft || NORMALKEY(fKey) == kRight);
				cStatus::MsgOsdTextItem(NULL, NORMALKEY(fKey) == kUp);
				return osContinue;
			default: break;
		} // switch
		eOSState lState = cOsdMenu::ProcessKey(fKey);
		if (lState == osUnknown) {
			switch (fKey) {
				case kRed: {
					int  lMatch = tmNone;
					cTimer *lTimer = Timers.GetMatch(mEvent, &lMatch);
					if (lMatch!=tmFull)
						lTimer = NULL;
					cPlugin *lPlugin = cPluginManager::GetPlugin("epgsearch");
					if(!lPlugin || !mEvent)
						return osContinue;
					Epgsearch_exttimeredit_v1_0 lData = { lTimer?lTimer:new cTimer(mEvent), !lTimer, mEvent};
					if(!lPlugin->Service("Epgsearch-exttimeredit-v1.0",(void *)&lData) || !lData.pTimerMenu)
						return osContinue;
					mMenu = lData.pTimerMenu;
					mMenu->Display();
					return osContinue;
				} // kRed
				case kGreen: {
					cPlugin *lPlugin = cPluginManager::GetPlugin("epgsearch");
					if(!lPlugin || !mEvent)
						return osContinue;
					Epgsearch_search_v1_0 lData = {(char *)mEvent->Title()/*query*/,0/*mode*/,0/*channel*/,true/*title*/,false/*subtitle*/,false/*description*/,NULL/*resultmenu*/};
					if(!lPlugin->Service("Epgsearch-search-v1.0",(void *)&lData) || !lData.pResultMenu)
						return osContinue;
					mMenu = lData.pResultMenu;
					mMenu->Display();
					return osContinue;
				} // kGreen
				case kYellow:
					return osContinue;
				case kBlue: {
					cPlugin *lPlugin = cPluginManager::GetPlugin("epgsearch");
					if(!lPlugin || !mEvent)
						return osContinue;
					Epgsearch_searchmenu_v1_1 lData = {0, mEvent->Title()};
					if(!lPlugin->Service("Epgsearch-searchmenu-v1.1",(void *)&lData) || !lData.pSearchMenu)
						if(!lPlugin->Service("Epgsearch-searchmenu-v1.0",(void *)&lData) || !lData.pSearchMenu)
							return osContinue;
					mMenu = lData.pSearchMenu;
					mMenu->Display();
					return osContinue;
				} // kBlue
				case kOk:     return osBack;
				default: break;
			} // switch
		} // if
		return lState;
	} // ProcessKey
}; // cMenuEventInfo

cString ShortDateString(time_t t){
	char buf[32];
	struct tm tm_r;
	tm *tm = localtime_r(&t, &tm_r);
	char *p = stpcpy(buf, WeekDayName(tm->tm_wday));
	*p++ = ' ';
	strftime(p, sizeof(buf) - (p - buf), "%d.%m.", tm);
	return buf;
} // ShortDateString

cString DateOnlyString(time_t t){
	char buf[32];
	struct tm tm_r;
	tm *tm = localtime_r(&t, &tm_r);
	strftime(buf, sizeof(buf), "%d.%m.", tm);
	return buf;
} // DateOnlyString

cChannel *getGroup(int fChan) {
        cChannel *lChan = mChannelList->GetByNumber(fChan);
        if (!lChan)
                return NULL;
        int lPos = mChannelList->GetPrevGroup(lChan->Index());
        if (lPos < 0)
                return NULL;
        return mChannelList->Get(lPos);
} // getGroup

void cTimeLine::SetLocalChannelList()
{
	lPlugin_ReelChannellist = NULL;
	lPlugin_ReelChannellist = cPluginManager::GetPlugin("reelchannellist");
        struct Data_t {cChannels* channels;};
	Data_t lData;
	mChannelList = &Channels;
	if(lPlugin_ReelChannellist && cPluginManager::GetPlugin("bouquets") == NULL)
	{
		lPlugin_ReelChannellist->Service("reload favourites list",(void *)&lData);
		if(lPlugin_ReelChannellist->Service("get favourites list",(void *)&lData))
		{	
			dsyslog("reelepg: using favourites.conf");
			mChannelList = lData.channels;
		} else
			lPlugin_ReelChannellist = NULL;
	} else
	{	
		lPlugin_ReelChannellist = NULL;
		dsyslog("reelepg: using channels.conf");
	}
}

cTimeLine::cTimeLine(class cPlugin *fParent) {
	DBG(DBG_FN,"cTimeLine::cTimeLine(...)");


	mOsd=NULL;
	mMenu=NULL;
	mParent=fParent;
	mFont = cFont::GetFont(fontSml);
	mSchedules = cSchedules::Schedules(mSchedLock);
	mScalePosNow   = -1;
	mScalePosCount = 0;
	mScalePos      = NULL;
	mStart         = 0;
	mMaxCharWidth  = 0;
	mFirstChan     = 0;
	mUseExtSkin         = 0;
	mShowConflict  = 0;

	for (int i = 0; i <= 0xFF; i++)
		if (mMaxCharWidth < mFont->Width(i))
			mMaxCharWidth = mFont->Width(i);

	SetLocalChannelList();

} // cTimeLine::cTimeLin&Channels
cTimeLine::~cTimeLine(void) {
	DBG(DBG_FN,"cTimeLine::~cTimeLine()");
	gReelEPGCfg.StoreSetup(mParent);
	if(mMenu)
		delete mMenu;
	mMenu=NULL;
	deleteOsd();
        mBrowseResultSet = false;
} // cTimeLine::~cTimeLine
void cTimeLine::Show(void){
	DBG(DBG_FN,"cTimeLine::Show()");
	mLastInput = time(NULL);
	createOsd();
	setCurrent();
} // cTimeLine::Show
eOSState cTimeLine::ProcessKey(eKeys fKey) {
	if (fKey != kNone)
		DBG(DBG_FN,"cTimeLine::ProcessKey(%d)",fKey);
	eOSState lState = osUnknown;
	if (mMenu) {
		lState = mMenu->ProcessKey(fKey);
		DBG(DBG_FN,"cTimeLine::ProcessKey(%d)->mMenu->ProcessKey = %d",fKey,lState);
syslog(LOG_INFO,"cTimeLine::ProcessKey(%d)->mMenu->ProcessKey = %d",fKey,lState);
		if (dynamic_cast<cMenuGroups*>(mMenu) && (lState == osUnknown)) {
			if (fKey == kOk) {
				cItemGroup* lItem = dynamic_cast<cItemGroup*>(mMenu->Get(mMenu->Current()));
                                if (lItem) {
                                        cChannel *lChan = lItem->mChannel ? mChannelList->Get(mChannelList->GetNextNormal(lItem->mChannel->Index()))
                                                                          : mChannelList->Get(mChannelList->GetNextNormal(-1));
					mSelChan = 0;
					if (lChan)
						mSelChan = setChannelRange(lChan->Number());
					int lChannels = mRows;
					if (gReelEPGCfg.mMagazineMode)
						lChannels = getChannelCount(gReelEPGCfg.mScaleWidth);
					mFirstChan = (mSelChan / lChannels)*lChannels;
					const cSchedule *lSched = (mSchedules && lChan) ? mSchedules->GetSchedule(lChan->GetChannelID()) : NULL;
					mSelEvent = lSched ? lSched->GetPresentEvent() : NULL;
				} // if
				delete mMenu;
				mMenu=NULL;
				createOsd();
				refresh();
				return osContinue;
			} // if
		} // if
		if (lState>osContinue) {
			delete mMenu;
			mMenu=NULL;
			switch (lState) {
				case osRecord   : recordEvent(); break;
				case osSwitchDvb: Channels.SwitchTo(GetVDRChannel(mChannels.Get(mSelChan)->Number())); return osEnd;
				default:
					if(!mShowConflict) {
					    cPlugin *lPlugin = cPluginManager::GetPlugin("epgsearch");
					    if(lPlugin) {
						EpgSearchMenu_v1_0 lData;
						if(lPlugin->Service("Epgsearch-conflictmenu-v1.0",(void *)&lData) && lData.Menu) {
						    if(lData.Menu->Current()) {
							mShowConflict = 1;
							mMenu = lData.Menu;
							mMenu->Display();
							break;
						    } else
							delete lData.Menu;
						} // if
					    } // if
					}
					mShowConflict = 0;
					createOsd();
					refresh();
					break;
			} // switch
			return osContinue;
		} // if
		return lState;
	} // if
	switch(fKey) {
		case kBack    : lState = osEnd; break;
		case kUp   | k_Repeat:
		case kUp      : if (gReelEPGCfg.mMagazineMode) {setEvent(false);} else {setChannel(mSelChan-1);} lState = osContinue; break;
		case kDown | k_Repeat:
		case kDown    : if (gReelEPGCfg.mMagazineMode) {setEvent(true);} else {setChannel(mSelChan+1);} lState = osContinue; break;
		case kLeft | k_Repeat:
		case kLeft    : if (gReelEPGCfg.mMagazineMode) {setChannel(mSelChan-1);} else {setEvent(false);} lState = osContinue; break;
		case kRight| k_Repeat:
		case kRight   : if (gReelEPGCfg.mMagazineMode) {setChannel(mSelChan+1);} else {setEvent(true);} lState = osContinue; break;
		case kPlay:
			if (lSearchResultEvents.size())
			{
				const cEvent *x = *lIterateResultEvents;
				bool bValidEvent = false;
				while(lIterateResultEvents != lSearchResultEvents.end() && !bValidEvent)
				{
					if(x != *lIterateResultEvents)
						bValidEvent = setStartEvent(*lIterateResultEvents);
					lIterateResultEvents++;
				}
				if(!bValidEvent)
				{
					lIterateResultEvents = lSearchResultEvents.begin();
					setStartEvent(*lIterateResultEvents);
				}
			}
			lState = osContinue;
			break;
// No more key available !!!!!! Jumnping back in browse results not possible
// Channel up / down is also not available withion plugin
		case kChanDn:
			if (lSearchResultEvents.size())
			{
				const cEvent *x = *lIterateResultEvents;
				bool bValidEvent = false;
				while(lIterateResultEvents != lSearchResultEvents.begin() && !bValidEvent)
				{
					if(x != *lIterateResultEvents)
						bValidEvent = setStartEvent(*lIterateResultEvents);
					lIterateResultEvents--;
				}
				if(!bValidEvent)
				{
					lIterateResultEvents = lSearchResultEvents.end();
					setStartEvent(*lIterateResultEvents);
				}
			}
			lState = osContinue;
			break;
		case kFastRew :
			{
				if (gReelEPGCfg.mMagazineMode)
				    {setChannel(mSelChan-getChannelCount(gReelEPGCfg.mScaleWidth));}
				else
				    {setChannel(mSelChan-mRows);}
			}
			lState = osContinue;
			break;
		case kFastFwd :
			{
				if (gReelEPGCfg.mMagazineMode)
				    {setChannel(mSelChan+getChannelCount(gReelEPGCfg.mScaleWidth));}
				else
				    {setChannel(mSelChan + 1 == mChannels.Count() ? 0 :  mSelChan+mRows >= mChannels.Count() ? mChannels.Count() - 1 : mSelChan+mRows);}
			}
			lState = osContinue;
			break;
                case kOk      : lState = mChannels.Get(mSelChan) ? Channels.SwitchTo(GetVDRChannel(mChannels.Get(mSelChan)->Number())) ? osEnd : osContinue : osEnd; break;
case k1       : setScale(1, gReelEPGCfg.mScaleHeight); lState = osContinue; break;
		case k2       : setScale(2, gReelEPGCfg.mScaleHeight); lState = osContinue; break;
		case k3       : setScale(3, gReelEPGCfg.mScaleHeight); lState = osContinue; break;
		case k4       : setScale(gReelEPGCfg.mScaleWidth,  1); lState = osContinue; break;
		case k5       : setScale(gReelEPGCfg.mScaleWidth,  2); lState = osContinue; break;
		case k6       : setScale(gReelEPGCfg.mScaleWidth,  3); lState = osContinue; break;
		case k7       : setStartTime(mStart-(60l*60l*24l)); lState = osContinue; break;
		case k8       : setStartTime(time(NULL)); lState = osContinue; break;
		case k9       : setStartTime(mStart+(60l*60l*24l)); lState = osContinue; break;
		case k0       : setCurrent (); lState = osContinue; break;
		case kGreater : switchMode (); lState = osContinue; break;
		case kRed     : recordEvent(); lState = osContinue; break;
		case kGreen   : showGroups (); lState = osContinue; break;
		case kYellow  : showEvent  (); lState = osContinue; break;
		case kBlue    : switchEvent(); lState = osContinue; break;
		case kInfo    :
		case kHelp    : showHelp   (); lState = osContinue; break;
		case kStop    : if(mMyFavouriteEvents.size()) { mMyFavouriteEvents = ""; lSearchResultEvents.clear(); mBrowseResultSet = false; refresh();} ; lState = osContinue; break;
		default       : lState = cOsdObject::ProcessKey(fKey);
	} // switch
	if (lState != osUnknown)
		mLastInput = time(NULL);
	if ((time(NULL)-mLastInput) > IDLE_TIMEOUT)
		return osEnd;
	return lState;
} // cTimeLine::ProcessKey
void cTimeLine::createOsd() {
	DBG(DBG_FN,"cTimeLine::createOsd()");
	if(mOsd)
		delete mOsd;
	mOsd = cOsdProvider::NewTrueColorOsd(Setup.OSDLeft, Setup.OSDTop, Setup.OSDRandom/*, OSD_LEVEL_DEFAULT, true*/);
	if(!mOsd)
		return;
#ifndef RBLITE
        if(strcmp("Reel",Skins.Current()->Name()) == 0)
                mUseExtSkin = SKIN_SKINREEL3;
	else if(strcmp("ReelNG", Skins.Current()->Name()) == 0)
                mUseExtSkin = SKIN_SKINREELNG;
#endif // RBLITE
	tArea lAreas[] = { { 0, 0, Setup.OSDWidth, Setup.OSDHeight, 32 } };
	eOsdError err = mOsd->CanHandleAreas(lAreas, sizeof(lAreas) / sizeof(tArea));
	if(err == oeOk)
		err = mOsd->SetAreas(lAreas, sizeof(lAreas) / sizeof(tArea));
	if(err != oeOk) {
		esyslog("Areas failed %d", err);
		return;
	} // if
	switch(mUseExtSkin)
	{
            case SKIN_SKINREEL3:
#ifdef USE_TFT
		LoadFontsNG();
		if(::Setup.FontSizes) {
			char *lName;
			asprintf(&lName, "%s_%i", TFT_TITLE, ::Setup.FontSizes);
			mFont = gFontCache.GetFont(lName);
			free(lName);
		} else
			mFont = cFont::GetFont(fontSml);
#else
		mFont = cFont::GetFont(fontSml);
#endif
		SetImagePathsSkinreel3();
		AddThemeColorsNG();
		SetDynamicSettingNG();
		gDateFrame             = gDateFrame_Skinreel3;
		gScaleFrame            = gScaleFrame_Skinreel3;
		gDetailLeftFrame       = gDetailLeftFrame_Skinreel3;
		gDetailRightFrame      = gDetailRightFrame_Skinreel3;
		gChannelNormalFrame    = gChannelNormalFrame_Skinreel3;
		gChannelSelectFrame    = gChannelSelectFrame_Skinreel3;
		gChannelsFrame         = gChannelsFrame_Skinreel3;
		gItemsFrame            = gItemsFrame_Skinreel3;
		gItemNormalFrame       = gItemNormalFrame_Skinreel3;
		gItemSelectFrame       = gItemSelectFrame_Skinreel3;
		gItemRecordNormalFrame = gItemRecordNormalFrame_Skinreel3;
		gItemRecordSelectFrame = gItemRecordSelectFrame_Skinreel3;
		gItemSwitchNormalFrame = gItemSwitchNormalFrame_Skinreel3;
		gItemSwitchSelectFrame = gItemSwitchSelectFrame_Skinreel3;

	        break;
            case SKIN_SKINREELNG:
#ifdef USE_TFT
		LoadFontsNG();
		if(::Setup.FontSizes) {
			char *lName;
			asprintf(&lName, "%s_%i", TFT_TITLE, ::Setup.FontSizes);
			mFont = gFontCache.GetFont(lName);
			free(lName);
		} else
			mFont = cFont::GetFont(fontSml);
#else
		mFont = cFont::GetFont(fontSml);
#endif
		SetImagePathsNG();
		AddThemeColorsNG();
		SetDynamicSettingNG();
		gDateFrame             = gDateFrame_NG;
		gScaleFrame            = gScaleFrame_NG;
		gDetailLeftFrame       = gDetailLeftFrame_NG;
		gDetailRightFrame      = gDetailRightFrame_NG;
		gChannelNormalFrame    = gChannelNormalFrame_NG;
		gChannelSelectFrame    = gChannelSelectFrame_NG;
		gChannelsFrame         = gChannelsFrame_NG;
		gItemsFrame            = gItemsFrame_NG;
		gItemNormalFrame       = gItemNormalFrame_NG;
		gItemSelectFrame       = gItemSelectFrame_NG;
		gItemRecordNormalFrame = gItemRecordNormalFrame_NG;
		gItemRecordSelectFrame = gItemRecordSelectFrame_NG;
		gItemSwitchNormalFrame = gItemSwitchNormalFrame_NG;
		gItemSwitchSelectFrame = gItemSwitchSelectFrame_NG;
	    break;
	default:
		mFont = cFont::GetFont(fontSml);
		SetImagePaths();
		AddThemeColors();
		gDateFrame             = gDateFrame_Lite;
		gScaleFrame            = gScaleFrame_Lite;
		gDetailLeftFrame       = gDetailLeftFrame_Lite;
		gDetailRightFrame      = gDetailRightFrame_Lite;
		gChannelNormalFrame    = gChannelNormalFrame_Lite;
		gChannelSelectFrame    = gChannelSelectFrame_Lite;
		gChannelsFrame         = gChannelsFrame_Lite;
		gItemsFrame            = gItemsFrame_Lite;
		gItemNormalFrame       = gItemNormalFrame_Lite;
		gItemSelectFrame       = gItemSelectFrame_Lite;
		gItemRecordNormalFrame = gItemRecordNormalFrame_Lite;
		gItemRecordSelectFrame = gItemRecordSelectFrame_Lite;
		gItemSwitchNormalFrame = gItemSwitchNormalFrame_Lite;
		gItemSwitchSelectFrame = gItemSwitchSelectFrame_Lite;

		break;
	} // switch

} // cTimeLine::createOsd
void cTimeLine::deleteOsd() {
	DBG(DBG_FN,"cTimeLine::deleteOsd()");
	if(mOsd)
		delete mOsd;
	mOsd = NULL;
} // cTimeLine::deleteOsd
void cTimeLine::flushOsd() {
	DBG(DBG_FN,"cTimeLine::flushOsd()");
	if(mOsd)
		mOsd->Flush();
} // cTimeLine::flushOsd
#define NG_TFT_FONT_MAX_INDEX 3
void cTimeLine::LoadFontsNG() {
#ifdef USE_TFT
	if (gLastFontSize == ::Setup.FontSizes)
		return;
	int lIndex = ::Setup.FontSizes;
	int lSize[NG_TFT_FONT_MAX_INDEX+1] = {0, gReelEPGCfg.mFontSizeBig, gReelEPGCfg.mFontSizeSmall, gReelEPGCfg.mFontSizeNormal};
	if(lSize[lIndex]) {
		char *lName, *lFile;
		asprintf(&lName, "%s_%i", TFT_TITLE, lIndex);
		asprintf(&lFile, "%s%s", gReelEPGCfg.mFontPath.c_str(), gReelEPGCfg.mFontName.c_str());
		if(!gFontCache.Load(lFile, lName, lSize[lIndex], ::Setup.OSDLanguage, gReelEPGCfg.mFontWidth, gReelEPGCfg.mFontFormat))
			DBG(DBG_ERR,"cTimeLine::LoadFontsNG-Failed to load Font (%s)!",lFile);
		free(lName);
		free(lFile);
	} // if
	gLastFontSize = ::Setup.FontSizes;
#endif /* USE_TFT */
} // cTimeLine::LoadFontsNG
void cTimeLine::SetImagePathsNG() {
	DBG(DBG_FN,"SetImagePathsNG()");
	if (!mOsd)
		return;
	cPlugin *lPlugin = cPluginManager::GetPlugin("reelbox");
	if(lPlugin)
		lPlugin->Service("ResetImagePaths",NULL);
	char *path;
	char *m_path = (char*)malloc(256);
	cTheme *lTheme = Skins.Current() ? Skins.Current()->Theme() : NULL;
	if (lTheme) {
		if(!strcmp(lTheme->Name(), "Blue")) {
			asprintf(&path, "%s", DEFAULT_NG_IMG_PATH);
		} else {
			DIR *dir = NULL;
			char *pathTry;
			asprintf(&pathTry, "%s/%s", DEFAULT_NG_BASE, lTheme->Name());
			if( (dir = opendir(pathTry))!= NULL) {
				path = pathTry;
				closedir(dir);
			} else {
				free(pathTry);
				asprintf(&path, "%s/%s", cThemes::GetThemesDirectory(), lTheme->Name());
			} // if
		} // if
	} else {
		asprintf(&path, "%s", DEFAULT_NG_IMG_PATH);
	} // if
	int fd;
	IMGPATH(mOsd, imgBlue1, "menu_header_left.png", path, fd);
	IMGPATH(mOsd, imgBlue2, "menu_header_center_x.png", path, fd);
	IMGPATH(mOsd, imgBlue3, "menu_header_right.png", path, fd);
	IMGPATH(mOsd, imgBlue7, "menu_footer_left.png", path, fd);
	IMGPATH(mOsd, imgBlue8, "menu_footer_center_x.png", path, fd);
	IMGPATH(mOsd, imgBlue9, "menu_footer_right.png", path, fd);
	IMGPATH(mOsd, imgButtonRed1, "button_red_x.png", path, fd);
	IMGPATH(mOsd, imgButtonGreen1, "button_green_x.png", path, fd);
	IMGPATH(mOsd, imgButtonYellow1, "button_yellow_x.png", path, fd);
	IMGPATH(mOsd, imgButtonBlue1, "button_blue_x.png", path, fd);
	free(path);
	free(m_path);
} // cTimeLine::SetImagePathsNG

void cTimeLine::SetImagePathsSkinreel3() {
	DBG(DBG_FN,"SetImagePathsSkinreel3()");
	if (!mOsd)
		return;
	cPlugin *lPlugin = cPluginManager::GetPlugin("reelbox");
	if(lPlugin)
		lPlugin->Service("ResetImagePaths",NULL);
	char *path;
	char *m_path = (char*)malloc(256);
	cTheme *lTheme = Skins.Current() ? Skins.Current()->Theme() : NULL;
	if (lTheme) {
		if(!strcmp(lTheme->Name(), "Blue")) {
			asprintf(&path, "%s", DEFAULT_SKINREEL3_IMG_PATH);
		} else {
			DIR *dir = NULL;
			char *pathTry;
			asprintf(&pathTry, "%s/%s", DEFAULT_SKINREEL3_BASE, lTheme->Name());
			if( (dir = opendir(pathTry))!= NULL) {
				path = pathTry;
				closedir(dir);
			} else {
				free(pathTry);
				asprintf(&path, "%s/%s", cThemes::GetThemesDirectory(), lTheme->Name());
			} // if
		} // if
	} else {
		asprintf(&path, "%s", DEFAULT_SKINREEL3_IMG_PATH);
	} // if
	int fd;
	IMGPATH(mOsd, imgBlue1, "images_header_left.png", path, fd);
	IMGPATH(mOsd, imgBlue2, "images_header_center_x.png", path, fd);
	IMGPATH(mOsd, imgBlue3, "images_header_right.png", path, fd);
	IMGPATH(mOsd, imgBlue7, "images_footer_left.png", path, fd);
	IMGPATH(mOsd, imgBlue8, "images_footer_center_x.png", path, fd);
	IMGPATH(mOsd, imgBlue9, "images_footer_right.png", path, fd);
	IMGPATH(mOsd, imgButtonRed1, "button_red_x.png", path, fd);
	IMGPATH(mOsd, imgButtonGreen1, "button_green_x.png", path, fd);
	IMGPATH(mOsd, imgButtonYellow1, "button_yellow_x.png", path, fd);
	IMGPATH(mOsd, imgButtonBlue1, "button_blue_x.png", path, fd);

	IMGPATH(mOsd, imgButtonHelp, "icon_help_inactive.png", path, fd);

	//asprintf(imgIconTipp, DEFAULT_SKINREEL3_BASE"/icon_favourites.png");
	IMGPATH(mOsd, imgIconTipp, "icon_favourites.png", DEFAULT_SKINREEL3_BASE, fd);
	IMGPATH(mOsd, imgIconFavorites, "icon_loveit.png", DEFAULT_SKINREEL3_BASE, fd);
	free(path);
	free(m_path);
} // cTimeLine::SetImagePathsSkinreel3


void cTimeLine::AddThemeColorsNG() {
	DBG(DBG_FN,"AddThemeColorsNG()");
	ADD_THEME_CLR(themeClrDateBg              , clrBlue5,      "clrBackground");
	ADD_THEME_CLR(themeClrDateTxt             , clrWhiteTxt,   "clrWhiteText");
	ADD_THEME_CLR(themeClrScaleBg             , clrBlue5,      "clrBackground");
	ADD_THEME_CLR(themeClrScaleTxt            , clrWhiteTxt,   "clrWhiteText");
	ADD_THEME_CLR(themeClrDetailLeftBg        , clrBlue5,      "clrBackground");
	ADD_THEME_CLR(themeClrDetailLeftTxt       , clrWhiteTxt,   "clrWhiteText");
	ADD_THEME_CLR(themeClrDetailRightBg       , clrBlue5,      "clrBackground");
	ADD_THEME_CLR(themeClrDetailRightTxt      , clrWhiteTxt,   "clrWhiteText");
	ADD_THEME_CLR(themeClrItemNormalBg        , 0xF00C1F33, NULL);//     "clrBackground");
	ADD_THEME_CLR(themeClrItemNormalTxt       , 0xFF707070, NULL);//clrWhiteTxt,   "clrWhiteText");
	ADD_THEME_CLR(themeClrItemSelectBg        , 0xF0355E70, NULL);//    "clrPreBackground");
	ADD_THEME_CLR(themeClrItemSelectTxt       , 0xFFEEEEEE, NULL);;//clrYellow,     "clrWhiteText");
	ADD_THEME_CLR(themeClrItemRecordNormalBg  , 0xF0704835, NULL);
	ADD_THEME_CLR(themeClrItemRecordNormalTxt , 0xFFEEEEEE, NULL);;//0xFF707070, NULL);//clrRecordTxt,  NULL);
	ADD_THEME_CLR(themeClrItemRecordSelectBg  , 0xF07E3827, NULL);
	ADD_THEME_CLR(themeClrItemRecordSelectTxt , 0xFFEEEEEE, NULL);//clrSRecordTxt, NULL);
	ADD_THEME_CLR(themeClrItemSwitchNormalBg  , 0xF0706E35, NULL);
	ADD_THEME_CLR(themeClrItemSwitchNormalTxt , 0xFFEEEEEE, NULL);;//0xFF707070, NULL);//clrSwitchTxt,  NULL);
	ADD_THEME_CLR(themeClrItemSwitchSelectBg  , 0xF086841F, NULL);
	ADD_THEME_CLR(themeClrItemSwitchSelectTxt , 0xFFEEEEEE, NULL);//clrSSwitchTxt, NULL);
	ADD_THEME_CLR(themeClrTxtShadow           , clrBlack,      NULL);
	ADD_THEME_CLR(themeClrButtonRedFg         , clrWhiteTxt,   "clrButtonRedFg");
	ADD_THEME_CLR(themeClrButtonGreenFg       , clrWhiteTxt,   "clrButtonGreenFg");
	ADD_THEME_CLR(themeClrButtonYellowFg      , clrWhiteTxt,   "clrButtonYellowFg");
	ADD_THEME_CLR(themeClrButtonBlueFg        , clrWhiteTxt,   "clrButtonBlueFg");

	ADD_THEME_CLR(themeOptScaleFrame   , 1, NULL);
	ADD_THEME_CLR(themeOptItemsFrame   , 1, NULL);
	ADD_THEME_CLR(themeOptChannelsFrame, 1, NULL);
	ADD_THEME_CLR(themeOptShowDate     , 1, NULL);
	ADD_THEME_CLR(themeOptShowTime     , 0, NULL);
	ADD_THEME_CLR(themeOptShowSR       , 0, NULL);
	ADD_THEME_CLR(themeOptShowRN       , 0, NULL);

	ADD_THEME_CLR(themeOptFrameLeft  , 0, NULL);
	ADD_THEME_CLR(themeOptFrameTop   , 1, NULL);
	ADD_THEME_CLR(themeOptFrameRight , 2, NULL);
	ADD_THEME_CLR(themeOptFrameBottom, 1, NULL);

	ADD_THEME_CLR(themeClrDateFrame         , 0, NULL);
	ADD_THEME_CLR(themeClrScaleFrame        , 0, NULL);
	ADD_THEME_CLR(themeClrDetailLeftFrame   , 0, NULL);
	ADD_THEME_CLR(themeClrDetailRightFrame  , 0, NULL);
	ADD_THEME_CLR(themeClrChannelNormalFrame, CLR_TRANSPARENT_NOT_0, NULL);//0xF00C1F33, NULL);
	ADD_THEME_CLR(themeClrChannelSelectFrame, CLR_TRANSPARENT_NOT_0, NULL);//0xF00C1F33, NULL);
	ADD_THEME_CLR(themeClrChannelsFrame     , CLR_TRANSPARENT_NOT_0, NULL);
	ADD_THEME_CLR(themeClrChannelsBg        , 0xF00C1F33, NULL);
	ADD_THEME_CLR(themeClrItemsFrame        , CLR_TRANSPARENT_NOT_0, NULL);
	ADD_THEME_CLR(themeClrItemsBg           , CLR_TRANSPARENT_NOT_0, NULL);//0xF00C1F33, NULL);
	ADD_THEME_CLR(themeClrChannelNormalBg   , 0xF00C1F33, NULL);
	ADD_THEME_CLR(themeClrChannelNormalTxt  , 0xFF707070, NULL);
	ADD_THEME_CLR(themeClrChannelSelectBg   , 0xF00C1F33, NULL);
	ADD_THEME_CLR(themeClrChannelSelectTxt  , 0xFFEEEEEE, NULL);
	ADD_THEME_CLR(themeClrTimeScaleBg       , 0xF00C1F33, NULL);
	ADD_THEME_CLR(themeClrTimeScaleFg       , 0xFF303030, NULL);
	ADD_THEME_CLR(themeClrTimeNowBg         , 0xF00C1F33, NULL);
	ADD_THEME_CLR(themeClrTimeNowFg         , 0xF07E3827, NULL);

	ADD_THEME_CLR(themeClrItemNormalFrame   , CLR_TRANSPARENT_NOT_0, NULL);
	ADD_THEME_CLR(themeClrItemSelectFrame   , CLR_TRANSPARENT_NOT_0, NULL);
	ADD_THEME_CLR(themeClrRecordNormalFrame , CLR_TRANSPARENT_NOT_0, NULL);
	ADD_THEME_CLR(themeClrRecordSelectFrame , CLR_TRANSPARENT_NOT_0, NULL);
	ADD_THEME_CLR(themeClrSwitchNormalFrame , CLR_TRANSPARENT_NOT_0, NULL);
	ADD_THEME_CLR(themeClrSwitchSelectFrame , CLR_TRANSPARENT_NOT_0, NULL);
} // cTimeLine::AddThemeColorsNG
void cTimeLine::SetDynamicSettingNG() {
	if(gReelEPGCfg.mMagazineMode) {
		SET_THEME_CLR(themeClrChannelNormalFrame, 0, NULL);
		SET_THEME_CLR(themeClrChannelSelectFrame, 0, NULL);
		SET_THEME_CLR(themeClrScaleFrame        , CLR_TRANSPARENT_NOT_0, NULL);
	} else {
		SET_THEME_CLR(themeClrChannelNormalFrame, CLR_TRANSPARENT_NOT_0, NULL);
		SET_THEME_CLR(themeClrChannelSelectFrame, CLR_TRANSPARENT_NOT_0, NULL);
		SET_THEME_CLR(themeClrScaleFrame        , 0, NULL);
	} // if
} // cTimeLine::SetDynamicSettingNG
void cTimeLine::SetImagePaths() {
	DBG(DBG_FN,"SetImagePaths()");
	if (!mOsd)
		return;
	cPlugin *lPlugin = cPluginManager::GetPlugin("reelbox");
	if(lPlugin)
		lPlugin->Service("ResetImagePaths",NULL);
	char *path;
	char *m_path = (char*)malloc(256);
	cTheme *lTheme = Skins.Current() ? Skins.Current()->Theme() : NULL;
	if (lTheme) {
		if(!strcmp(lTheme->Name(), "Blue")) {
			asprintf(&path, "%s", DEFAULT_IMG_PATH);
		} else {
			DIR *dir = NULL;
			char *pathTry;
			asprintf(&pathTry, "%s/%s", DEFAULT_BASE, lTheme->Name());
			if( (dir = opendir(pathTry))!= NULL) {
				path = pathTry;
				closedir(dir);
			} else {
				free(pathTry);
				asprintf(&path, "%s/%s", cThemes::GetThemesDirectory(), lTheme->Name());
			} // if
		} // if
	} else {
		asprintf(&path, "%s", DEFAULT_IMG_PATH);
	} // if
	int fd;
	IMGPATH(mOsd, imgBlue1, IMG_BLUE_1, path, fd);
	IMGPATH(mOsd, imgBlue2, IMG_BLUE_2, path, fd);
	IMGPATH(mOsd, imgBlue3, IMG_BLUE_3, path, fd);
	IMGPATH(mOsd, imgBlue4, IMG_BLUE_4, path, fd);
	IMGPATH(mOsd, imgBlue6, IMG_BLUE_6, path, fd);
	IMGPATH(mOsd, imgBlue7, IMG_BLUE_7, path, fd);
	IMGPATH(mOsd, imgBlue8, IMG_BLUE_8, path, fd);
	IMGPATH(mOsd, imgBlue9, IMG_BLUE_9, path, fd);
	IMGPATH(mOsd, imgButtonRed1, IMG_BUTTON_RED_1, path, fd);
	IMGPATH(mOsd, imgButtonRed2, IMG_BUTTON_RED_2, path, fd);
	IMGPATH(mOsd, imgButtonRed3, IMG_BUTTON_RED_3, path, fd);
	IMGPATH(mOsd, imgButtonGreen1, IMG_BUTTON_GREEN_1, path, fd);
	IMGPATH(mOsd, imgButtonGreen2, IMG_BUTTON_GREEN_2, path, fd);
	IMGPATH(mOsd, imgButtonGreen3, IMG_BUTTON_GREEN_3, path, fd);
	IMGPATH(mOsd, imgButtonYellow1, IMG_BUTTON_YELLOW_1, path, fd);
	IMGPATH(mOsd, imgButtonYellow2, IMG_BUTTON_YELLOW_2, path, fd);
	IMGPATH(mOsd, imgButtonYellow3, IMG_BUTTON_YELLOW_3, path, fd);
	IMGPATH(mOsd, imgButtonBlue1, IMG_BUTTON_BLUE_1, path, fd);
	IMGPATH(mOsd, imgButtonBlue2, IMG_BUTTON_BLUE_2, path, fd);
	IMGPATH(mOsd, imgButtonBlue3, IMG_BUTTON_BLUE_3, path, fd);
	IMGPATH(mOsd, imgTeal1, IMG_TEAL_1, path, fd);
	IMGPATH(mOsd, imgTeal2, IMG_TEAL_2, path, fd);
	IMGPATH(mOsd, imgTeal3, IMG_TEAL_3, path, fd);
	IMGPATH(mOsd, imgTeal4, IMG_TEAL_4, path, fd);
	IMGPATH(mOsd, imgTeal6, IMG_TEAL_6, path, fd);
	IMGPATH(mOsd, imgTeal7, IMG_TEAL_7, path, fd);
	IMGPATH(mOsd, imgTeal8, IMG_TEAL_8, path, fd);
	IMGPATH(mOsd, imgTeal9, IMG_TEAL_9, path, fd);
	IMGPATH(mOsd, imgSmallBlue1   , IMG_SMALL_BLUE_1   , path, fd);
	IMGPATH(mOsd, imgSmallBlue2   , IMG_SMALL_BLUE_2   , path, fd);
	IMGPATH(mOsd, imgSmallBlue3   , IMG_SMALL_BLUE_3   , path, fd);
	IMGPATH(mOsd, imgSmallBlue4   , IMG_SMALL_BLUE_4   , path, fd);
	IMGPATH(mOsd, imgSmallBlue6   , IMG_SMALL_BLUE_6   , path, fd);
	IMGPATH(mOsd, imgSmallBlue7   , IMG_SMALL_BLUE_7   , path, fd);
	IMGPATH(mOsd, imgSmallBlue8   , IMG_SMALL_BLUE_8   , path, fd);
	IMGPATH(mOsd, imgSmallBlue9   , IMG_SMALL_BLUE_9   , path, fd);
	IMGPATH(mOsd, imgSmallSBlue1  , IMG_SMALL_SBLUE_1  , path, fd);
	IMGPATH(mOsd, imgSmallSBlue2  , IMG_SMALL_SBLUE_2  , path, fd);
	IMGPATH(mOsd, imgSmallSBlue3  , IMG_SMALL_SBLUE_3  , path, fd);
	IMGPATH(mOsd, imgSmallSBlue4  , IMG_SMALL_SBLUE_4  , path, fd);
	IMGPATH(mOsd, imgSmallSBlue6  , IMG_SMALL_SBLUE_6  , path, fd);
	IMGPATH(mOsd, imgSmallSBlue7  , IMG_SMALL_SBLUE_7  , path, fd);
	IMGPATH(mOsd, imgSmallSBlue8  , IMG_SMALL_SBLUE_8  , path, fd);
	IMGPATH(mOsd, imgSmallSBlue9  , IMG_SMALL_SBLUE_9  , path, fd);
	IMGPATH(mOsd, imgSmallRed1    , IMG_SMALL_RED_1    , path, fd);
	IMGPATH(mOsd, imgSmallRed2    , IMG_SMALL_RED_2    , path, fd);
	IMGPATH(mOsd, imgSmallRed3    , IMG_SMALL_RED_3    , path, fd);
	IMGPATH(mOsd, imgSmallRed4    , IMG_SMALL_RED_4    , path, fd);
	IMGPATH(mOsd, imgSmallRed6    , IMG_SMALL_RED_6    , path, fd);
	IMGPATH(mOsd, imgSmallRed7    , IMG_SMALL_RED_7    , path, fd);
	IMGPATH(mOsd, imgSmallRed8    , IMG_SMALL_RED_8    , path, fd);
	IMGPATH(mOsd, imgSmallRed9    , IMG_SMALL_RED_9    , path, fd);
	IMGPATH(mOsd, imgSmallSRed1   , IMG_SMALL_SRED_1   , path, fd);
	IMGPATH(mOsd, imgSmallSRed2   , IMG_SMALL_SRED_2   , path, fd);
	IMGPATH(mOsd, imgSmallSRed3   , IMG_SMALL_SRED_3   , path, fd);
	IMGPATH(mOsd, imgSmallSRed4   , IMG_SMALL_SRED_4   , path, fd);
	IMGPATH(mOsd, imgSmallSRed6   , IMG_SMALL_SRED_6   , path, fd);
	IMGPATH(mOsd, imgSmallSRed7   , IMG_SMALL_SRED_7   , path, fd);
	IMGPATH(mOsd, imgSmallSRed8   , IMG_SMALL_SRED_8   , path, fd);
	IMGPATH(mOsd, imgSmallSRed9   , IMG_SMALL_SRED_9   , path, fd);
	IMGPATH(mOsd, imgSmallYellow1 , IMG_SMALL_YELLOW_1 , path, fd);
	IMGPATH(mOsd, imgSmallYellow2 , IMG_SMALL_YELLOW_2 , path, fd);
	IMGPATH(mOsd, imgSmallYellow3 , IMG_SMALL_YELLOW_3 , path, fd);
	IMGPATH(mOsd, imgSmallYellow4 , IMG_SMALL_YELLOW_4 , path, fd);
	IMGPATH(mOsd, imgSmallYellow6 , IMG_SMALL_YELLOW_6 , path, fd);
	IMGPATH(mOsd, imgSmallYellow7 , IMG_SMALL_YELLOW_7 , path, fd);
	IMGPATH(mOsd, imgSmallYellow8 , IMG_SMALL_YELLOW_8 , path, fd);
	IMGPATH(mOsd, imgSmallYellow9 , IMG_SMALL_YELLOW_9 , path, fd);
	IMGPATH(mOsd, imgSmallSYellow1, IMG_SMALL_SYELLOW_1, path, fd);
	IMGPATH(mOsd, imgSmallSYellow2, IMG_SMALL_SYELLOW_2, path, fd);
	IMGPATH(mOsd, imgSmallSYellow3, IMG_SMALL_SYELLOW_3, path, fd);
	IMGPATH(mOsd, imgSmallSYellow4, IMG_SMALL_SYELLOW_4, path, fd);
	IMGPATH(mOsd, imgSmallSYellow6, IMG_SMALL_SYELLOW_6, path, fd);
	IMGPATH(mOsd, imgSmallSYellow7, IMG_SMALL_SYELLOW_7, path, fd);
	IMGPATH(mOsd, imgSmallSYellow8, IMG_SMALL_SYELLOW_8, path, fd);
	IMGPATH(mOsd, imgSmallSYellow9, IMG_SMALL_SYELLOW_9, path, fd);
	free(path);
	free(m_path);
} // cTimeLine::SetImagePaths
void cTimeLine::AddThemeColors() {
	DBG(DBG_FN,"AddThemeColors()");
	ADD_THEME_CLR(themeClrDateBg              , clrBlue5,      "clrBackground");
	ADD_THEME_CLR(themeClrDateTxt             , clrWhiteTxt,   "clrWhiteText");
	ADD_THEME_CLR(themeClrScaleBg             , clrBlue5,      "clrBackground");
	ADD_THEME_CLR(themeClrScaleTxt            , clrWhiteTxt,   "clrWhiteText");
	ADD_THEME_CLR(themeClrDetailLeftBg        , clrTeal5,      "clrPreBackground");
	ADD_THEME_CLR(themeClrDetailLeftTxt       , clrWhiteTxt,   "clrWhiteText");
	ADD_THEME_CLR(themeClrDetailRightBg       , clrBlue5,      "clrBackground");
	ADD_THEME_CLR(themeClrDetailRightTxt      , clrWhiteTxt,   "clrWhiteText");
	ADD_THEME_CLR(themeClrChannelNormalBg     , clrBlue5,      "clrBackground");
	ADD_THEME_CLR(themeClrChannelNormalTxt    , clrWhiteTxt,   "clrWhiteText");
	ADD_THEME_CLR(themeClrChannelSelectBg     , clrTeal5,      "clrPreBackground");
	ADD_THEME_CLR(themeClrChannelSelectTxt    , clrYellow,     "clrMenuEventDescription");
	ADD_THEME_CLR(themeClrChannelsBg          , clrBlue5,      "clrBackground");
	ADD_THEME_CLR(themeClrItemNormalBg        , clrBlue5,      "clrBackground");
	ADD_THEME_CLR(themeClrItemNormalTxt       , clrWhiteTxt,   "clrWhiteText");
	ADD_THEME_CLR(themeClrItemSelectBg        , clrSBlue5,     "clrPreBackground");
	ADD_THEME_CLR(themeClrItemSelectTxt       , clrYellow,     "clrWhiteText");
	ADD_THEME_CLR(themeClrItemRecordNormalBg  , clrRecordBg,   NULL);
	ADD_THEME_CLR(themeClrItemRecordNormalTxt , clrRecordTxt,  NULL);
	ADD_THEME_CLR(themeClrItemRecordSelectBg  , clrSRecordBg,  NULL);
	ADD_THEME_CLR(themeClrItemRecordSelectTxt , clrSRecordTxt, NULL);
	ADD_THEME_CLR(themeClrItemSwitchNormalBg  , clrSwitchBg,   NULL);
	ADD_THEME_CLR(themeClrItemSwitchNormalTxt , clrSwitchTxt,  NULL);
	ADD_THEME_CLR(themeClrItemSwitchSelectBg  , clrSSwitchBg,  NULL);
	ADD_THEME_CLR(themeClrItemSwitchSelectTxt , clrSSwitchTxt, NULL);
	ADD_THEME_CLR(themeClrTxtShadow           , clrBlack,      NULL);
	ADD_THEME_CLR(themeClrTimeScaleBg         , clrBlue,       NULL);
	ADD_THEME_CLR(themeClrTimeScaleFg         , clrWhiteTxt,   NULL);
	ADD_THEME_CLR(themeClrTimeNowBg           , clrRed,        NULL);
	ADD_THEME_CLR(themeClrTimeNowFg           , clrWhiteTxt,   NULL);
	ADD_THEME_CLR(themeClrItemsBg             , clrBlue5,      "clrBackground");
	ADD_THEME_CLR(themeClrButtonRedFg         , clrWhiteTxt,   "clrButtonRedFg");
	ADD_THEME_CLR(themeClrButtonGreenFg       , clrWhiteTxt,   "clrButtonGreenFg");
	ADD_THEME_CLR(themeClrButtonYellowFg      , clrWhiteTxt,   "clrButtonYellowFg");
	ADD_THEME_CLR(themeClrButtonBlueFg        , clrWhiteTxt,   "clrButtonBlueFg");

	ADD_THEME_CLR(themeOptScaleFrame   , 1, NULL);
	ADD_THEME_CLR(themeOptItemsFrame   , 1, NULL);
	ADD_THEME_CLR(themeOptChannelsFrame, 0, NULL);
	ADD_THEME_CLR(themeOptShowDate     , 1, NULL);
	ADD_THEME_CLR(themeOptShowTime     , 0, NULL);
	ADD_THEME_CLR(themeOptShowSR       , 0, NULL);
	ADD_THEME_CLR(themeOptShowRN       , 0, NULL);

	ADD_THEME_CLR(themeOptFrameLeft  , 2, NULL);
	ADD_THEME_CLR(themeOptFrameTop   , 2, NULL);
	ADD_THEME_CLR(themeOptFrameRight , 2, NULL);
	ADD_THEME_CLR(themeOptFrameBottom, 2, NULL);

	ADD_THEME_CLR(themeClrDateFrame         , 0, NULL);
	ADD_THEME_CLR(themeClrScaleFrame        , 0, NULL);
	ADD_THEME_CLR(themeClrDetailLeftFrame   , 0, NULL);
	ADD_THEME_CLR(themeClrDetailRightFrame  , 0, NULL);
	ADD_THEME_CLR(themeClrChannelNormalFrame, 0, NULL);
	ADD_THEME_CLR(themeClrChannelSelectFrame, 0, NULL);
	ADD_THEME_CLR(themeClrChannelsFrame     , 0, NULL);
	ADD_THEME_CLR(themeClrItemsFrame        , 0, NULL);
	ADD_THEME_CLR(themeClrItemNormalFrame   , 0, NULL);
	ADD_THEME_CLR(themeClrItemSelectFrame   , 0, NULL);
	ADD_THEME_CLR(themeClrRecordNormalFrame , 0, NULL);
	ADD_THEME_CLR(themeClrRecordSelectFrame , 0, NULL);
	ADD_THEME_CLR(themeClrSwitchNormalFrame , 0, NULL);
	ADD_THEME_CLR(themeClrSwitchSelectFrame , 0, NULL);
} // cTimeLine::AddThemeColors

void cTimeLine::drawFrame(int x1, int y1, int x2, int y2, tFrameInfo &fFrameInfo, bool fTopLeftEnd, bool fBottomRightEnd) {
	//DBG(DBG_FN,"cTimeLine::drawFrame(%d,%d,%d,%d,...,%d,%d)",x1, y1, x2, y2, fTopLeftEnd, fBottomRightEnd);
	if(!mOsd)
		return;
	if (gReelEPGCfg.mMagazineMode) {
		if((y2-y1 < ((fTopLeftEnd?fFrameInfo.mHeightTop:0 ) + (fBottomRightEnd?fFrameInfo.mHeightBottom:0))) ||
			(x2-x1 < (fFrameInfo.mWidthLeft+fFrameInfo.mWidthRight)))
		return;
	} else {
		if ((x2-x1 < ((fTopLeftEnd?fFrameInfo.mWidthLeft:0 ) + (fBottomRightEnd?fFrameInfo.mWidthRight:0))) ||
			(y2-y1 < (fFrameInfo.mHeightTop+fFrameInfo.mHeightBottom)))
		return;
	} // if
	if (!GET_CLR(fFrameInfo.mFrmColor)) {
		if (gReelEPGCfg.mMagazineMode) {
			int lImgHeightTop   = 0;
			int lImgHeightBottom = 0;
			if (fTopLeftEnd && fFrameInfo.mHeightTop) {
				lImgHeightTop = fFrameInfo.mHeightTop;
				mOsd->DrawImage(fFrameInfo.mImgIndex+0, x1, y1, fFrameInfo.mBlend);
				mOsd->DrawImage(fFrameInfo.mImgIndex+1, x1+fFrameInfo.mWidthLeft, y1, fFrameInfo.mBlend, x2-x1-fFrameInfo.mWidthLeft-fFrameInfo.mWidthRight);
				mOsd->DrawImage(fFrameInfo.mImgIndex+2, x2-fFrameInfo.mWidthRight, y1, fFrameInfo.mBlend);
			} // if
			if (fBottomRightEnd && fFrameInfo.mHeightBottom) {
				lImgHeightBottom = fFrameInfo.mHeightBottom;
				mOsd->DrawImage(fFrameInfo.mImgIndex+5, x1, y2-fFrameInfo.mHeightBottom, fFrameInfo.mBlend);
				mOsd->DrawImage(fFrameInfo.mImgIndex+6, x1+fFrameInfo.mWidthLeft, y2-fFrameInfo.mHeightBottom, fFrameInfo.mBlend, x2-x1-fFrameInfo.mWidthLeft-fFrameInfo.mWidthRight);
				mOsd->DrawImage(fFrameInfo.mImgIndex+7, x2-fFrameInfo.mWidthRight, y2-fFrameInfo.mHeightBottom, fFrameInfo.mBlend);
			} // if
			int lTopExtra = 0;
			int lBottomExtra = 0;
			int lWidth = 0;
			switch(mUseExtSkin)
			{
			    case SKIN_SKINREEL3:
			    case SKIN_SKINREELNG:
				lTopExtra    = fFrameInfo.mHeightTop?fFrameInfo.mHeightTop+2:0;
				lBottomExtra = fFrameInfo.mHeightBottom?fFrameInfo.mHeightBottom+2:0;
				lWidth       = fFrameInfo.mWidthRight - fFrameInfo.mWidthLeft;

				mOsd->DrawRectangle(x1, y1 + lTopExtra, x2-lWidth-1, y2-lBottomExtra-1, GET_CLR(fFrameInfo.mBgColor));
				break;
			    default:
				mOsd->DrawImage(fFrameInfo.mImgIndex+3, x1, y1 + lImgHeightTop, fFrameInfo.mBlend, 1, y2-y1-lImgHeightTop-lImgHeightBottom);
				mOsd->DrawRectangle(x1+fFrameInfo.mWidthLeft, y1 + lImgHeightTop, x2-fFrameInfo.mWidthRight, y2-lImgHeightBottom, GET_CLR(fFrameInfo.mBgColor));
				mOsd->DrawImage(fFrameInfo.mImgIndex+4, x2-fFrameInfo.mWidthRight, y1 + lImgHeightTop, fFrameInfo.mBlend, 1, y2-y1-lImgHeightTop-lImgHeightBottom);
			        break;
			} // switch
		} else {
			int lImgWidthLeft  = 0;
			int lImgWidthRight = 0;
			if (fTopLeftEnd) {
				lImgWidthLeft = fFrameInfo.mWidthLeft;
				mOsd->DrawImage(fFrameInfo.mImgIndex+0, x1, y1, fFrameInfo.mBlend);
				switch(mUseExtSkin)
				{
				    case SKIN_SKINREELNG:
					break;
				    case SKIN_SKINREEL3:
					break;
				    default:
					mOsd->DrawImage(fFrameInfo.mImgIndex+3, x1, y1 + fFrameInfo.mHeightTop, fFrameInfo.mBlend, 1, y2-y1-fFrameInfo.mHeightTop-fFrameInfo.mHeightBottom);
				}
				if(fFrameInfo.mHeightBottom)
			  		mOsd->DrawImage(fFrameInfo.mImgIndex+5, x1, y2-fFrameInfo.mHeightBottom, fFrameInfo.mBlend);
			} // if
			if (fBottomRightEnd) {
				lImgWidthRight = fFrameInfo.mWidthRight;
				mOsd->DrawImage(fFrameInfo.mImgIndex+2, x2-lImgWidthRight, y1, fFrameInfo.mBlend);
				switch(mUseExtSkin)
				{
				    case SKIN_SKINREELNG:
					break;
				    case SKIN_SKINREEL3:
					break;
				    default:
					mOsd->DrawImage(fFrameInfo.mImgIndex+4, x2-lImgWidthRight, y1 + fFrameInfo.mHeightTop, fFrameInfo.mBlend, 1, y2-y1-fFrameInfo.mHeightTop-fFrameInfo.mHeightBottom);
					break;
				}
				if(fFrameInfo.mHeightBottom)
					mOsd->DrawImage(fFrameInfo.mImgIndex+7, x2-lImgWidthRight, y2-fFrameInfo.mHeightBottom, fFrameInfo.mBlend);
			} // if
			if(fFrameInfo.mHeightTop)
				mOsd->DrawImage(fFrameInfo.mImgIndex+1, x1+lImgWidthLeft, y1, fFrameInfo.mBlend, x2-x1-lImgWidthLeft-lImgWidthRight);
			if(mUseExtSkin) {
				int lTopExtra    = fFrameInfo.mHeightTop?fFrameInfo.mHeightTop+2:0;
				int lBottomExtra = fFrameInfo.mHeightBottom?fFrameInfo.mHeightBottom+2:0;
				int lWidth       = fFrameInfo.mWidthRight - fFrameInfo.mWidthLeft;
				mOsd->DrawRectangle(x1, y1 + lTopExtra, x2-lWidth-1, y2-lBottomExtra-1, GET_CLR(fFrameInfo.mBgColor));
			} else
				mOsd->DrawRectangle(x1+lImgWidthLeft, y1 + fFrameInfo.mHeightTop, x2-lImgWidthRight, y2-fFrameInfo.mHeightBottom-1, GET_CLR(fFrameInfo.mBgColor));
			if(fFrameInfo.mHeightBottom)
				mOsd->DrawImage(fFrameInfo.mImgIndex+6, x1+lImgWidthLeft, y2-fFrameInfo.mHeightBottom, fFrameInfo.mBlend, x2-x1-lImgWidthLeft-lImgWidthRight);
		} // if
	} else {
		int lColor = GET_CLR(fFrameInfo.mFrmColor);
		if(lColor == CLR_TRANSPARENT_NOT_0)
			lColor = clrTransparent;
		if (gReelEPGCfg.mMagazineMode) {
			mOsd->DrawRectangle(x1, y1, x2, y2, lColor);//GET_CLR(fFrameInfo.mFrmColor));
			mOsd->DrawRectangle(x1+GET_CLR(themeOptFrameLeft), y1+(fTopLeftEnd?GET_CLR(themeOptFrameTop):0), x2-GET_CLR(themeOptFrameRight)-1, y2-(fBottomRightEnd?GET_CLR(themeOptFrameBottom):0)-1, GET_CLR(fFrameInfo.mBgColor));
		} else {
			mOsd->DrawRectangle(x1, y1, x2-1, y2-1, lColor);//GET_CLR(fFrameInfo.mFrmColor));
			mOsd->DrawRectangle(x1+(fTopLeftEnd?GET_CLR(themeOptFrameLeft):0), y1+GET_CLR(themeOptFrameTop), x2-(fBottomRightEnd?GET_CLR(themeOptFrameRight):0)-1, y2-GET_CLR(themeOptFrameBottom)-1, GET_CLR(fFrameInfo.mBgColor));
		} // if
	} // if

} // cTimeLine::drawFrame
void cTimeLine::drawText(int x1, int y1, int x2, int y2, tColor fFG, tColor fBG, const cFont* fFont, const char *fText, int fAlign) {
//	DBG(DBG_FN,"cTimeLine::drawText(%d,%d,%d,%d,%d,%d,%s,%d)",x1, y1, x2, y2, fFG, fBG, fText, fAlign);
	if (!mOsd || !fFont || (x1 >= x2) || (y1 >= y2))
		return;
	if (strchr(fText, '\t')) {
		char *t1 = (char *) strchr(fText, '\t');
		*t1 = 0;
		cString lTime (fText);
		cString lTitle("");
		cString lDescr("");
		char *t2 = strchr(t1+1, '\n');
		if (t2) {
			*t2 = 0;
			lTitle = cString(t1+1);
			lDescr = cString(t2+1);
			*t2 = '\n';
		} else {
			lTitle = cString(t1+1);
		} // if
		*t1 = '\t';

		int lWidthDate = mFont->Width(lTime)+mFont->Width("_");
		int lHeight = mFont->Height(fText);
		cTextWrapper lTitleWrap (lTitle, fFont, x2-x1-lWidthDate);
		cTextWrapper lDescrWrap (lDescr, fFont, x2-x1-lWidthDate);
		int lRows = (y2-y1)/lHeight;


		for (int i = 0; i < lRows; i++) {
			if (i==0) mOsd->DrawText(x1, y1, lTime, fFG, fBG, fFont, lWidthDate, lHeight, taLeft);
			if (i < lTitleWrap.Lines()) {
				mOsd->DrawText(x1+lWidthDate, y1+lHeight*i, lTitleWrap.GetLine(i), fFG, fBG, fFont, x2-x1-lWidthDate, lHeight, taLeft);
			} else if (i - lTitleWrap.Lines() < lDescrWrap.Lines()) {
				mOsd->DrawText(x1+lWidthDate, y1+lHeight*i, lDescrWrap.GetLine(i-lTitleWrap.Lines()), fFG, fBG, fFont, x2-x1-lWidthDate, lHeight, taLeft);
			} // if

		}
	} else {
		if ((mFont->Width(fText) > (x2-x1)) && ((x2-x1) > mMaxCharWidth)) {
			cTextWrapper lWrapper(fText, fFont, x2-x1);
			int lHeight = fFont->Height(fText);
			int lRows = min(lWrapper.Lines(),(y2-y1)/lHeight);
			int lPos  = ((y2-y1-lRows*lHeight)/2)+y1;
			for (int i = 0; i < lRows; i++) {
				mOsd->DrawText(x1, lPos+lHeight*i, lWrapper.GetLine(i), fFG, fBG, fFont, x2-x1, lHeight, fAlign);
			}
		} else
			mOsd->DrawText(x1, y1, fText, fFG, fBG, fFont, x2-x1, y2-y1, fAlign);
	} // if
} // cTimeLine::drawText
void cTimeLine::drawCell(int x1, int y1, int x2, int y2, tFrameInfo &fFrameInfo, const char * fText, bool fTopLeftEnd, bool fBottomRightEnd, tFrameInfo *fBottomRight, int fSplit, unsigned char *bmp, cString Category, bool bPic, cString PicturePath, int ShowTipFlag, int ShowLoveitFlag) {
//	DBG(DBG_FN,"cTimeLine::drawCell(%d,%d,%d,%d,...,%s,%d,%d,%ld,%d)",x1, y1, x2, y2, fText, fLeftEnd, fRightEnd, (long)lRight, lSplit);
	if (fBottomRight) {
		if (gReelEPGCfg.mMagazineMode) {
			if ((fSplit-y1) < fFrameInfo.mHeightTop) {
				drawFrame(x1, y1, x2, y2, *fBottomRight, fTopLeftEnd, fBottomRightEnd);
			} else if ((y2-fSplit) < fBottomRight->mHeightBottom) {
				drawFrame(x1, y1, x2, y2, fFrameInfo, fTopLeftEnd, fBottomRightEnd);
			} else {
				drawFrame(x1, y1, x2, fSplit, fFrameInfo, fTopLeftEnd, false);
				drawFrame(x1, fSplit, x2, y2, *fBottomRight, false, fBottomRightEnd);
			} // if
		} else {
			if ((fSplit-x1) < fFrameInfo.mWidthLeft) {
				drawFrame(x1, y1, x2, y2, *fBottomRight, fTopLeftEnd, fBottomRightEnd);
			} else if ((x2-fSplit) < fBottomRight->mWidthRight) {
				drawFrame(x1, y1, x2, y2, fFrameInfo, fTopLeftEnd, fBottomRightEnd);
			} else {
				drawFrame(x1, y1, fSplit, y2, fFrameInfo, fTopLeftEnd, false);
				drawFrame(fSplit, y1, x2, y2, *fBottomRight, false, fBottomRightEnd);
			} // if
		} // if
	} else
		drawFrame(x1, y1, x2, y2, fFrameInfo, fTopLeftEnd, fBottomRightEnd);
	if (fFrameInfo.mShadow)
		drawText (x1+fFrameInfo.mInsetLeft, y1+fFrameInfo.mInsetTop-1, x2-fFrameInfo.mInsetRight, y2-fFrameInfo.mInsetBottom-1, GET_CLR(themeClrTxtShadow), clrTransparent, mFont, fText, taCenter);
	if (fFrameInfo.mLines) {
		if (gReelEPGCfg.mMagazineMode) {
/* Keine horizontalen Linien, da dies Flackern verursachen kann.
			for (int i = 0; i < mScalePosCount; i++) {
				if ((mScalePos[i] > y1) && (mScalePos[i] < y2) ) {
					mOsd->DrawRectangle(x1, mScalePos[i]-1, x2, mScalePos[i]+1, GET_CLR(themeClrTimeScaleBg));
					mOsd->DrawRectangle(x1, mScalePos[i], x2, mScalePos[i], GET_CLR(themeClrTimeScaleFg));
				} // if
			} // for
			if ((mScalePosNow > y1) && (mScalePosNow < y2)) {
				mOsd->DrawRectangle(x1, mScalePosNow-1, x2, mScalePosNow+1, GET_CLR(themeClrTimeNowBg));
				mOsd->DrawRectangle(x1, mScalePosNow, x2, mScalePosNow, GET_CLR(themeClrTimeNowFg));
			} // if
*/
		} else {
			for (int i = 0; i < mScalePosCount; i++) {
				if ((mScalePos[i] >= x1) && (mScalePos[i] <= x2) ) {
					mOsd->DrawRectangle(mScalePos[i]-1, y1, mScalePos[i]+1, y2, GET_CLR(themeClrTimeScaleBg));
					mOsd->DrawRectangle(mScalePos[i], y1, mScalePos[i], y2, GET_CLR(themeClrTimeScaleFg));
				} // if
			} // for
			if ((mScalePosNow >= x1) && (mScalePosNow <= x2)) {
				mOsd->DrawRectangle(mScalePosNow-1, y1, mScalePosNow+1, y2, GET_CLR(themeClrTimeNowBg));
				mOsd->DrawRectangle(mScalePosNow, y1, mScalePosNow, y2, GET_CLR(themeClrTimeNowFg));
			} // if
		} // if
	} // if

	// added MQ
	int available_height = y2 - y1 - fFrameInfo.mInsetTop - fFrameInfo.mInsetBottom;
	int category_height = mFont->Height("0") + fFrameInfo.mInsetTop + fFrameInfo.mInsetBottom;
	int lHeader = 0;

	if(!isempty(Category) && available_height > (category_height + mFont->Height("0")) /* (2 * mFont->Height("0") */)
	{
		mOsd->DrawRectangle(x1, y1, max(x2-3, 0), y1+category_height /* fFrameInfo.mInsetTop+20 */, GET_CLR(fFrameInfo.mTxtColor));
		drawText (x1, y1+fFrameInfo.mInsetTop, x2, y1+category_height /* fFrameInfo.mInsetTop+20 */,  GET_CLR(fFrameInfo.mBgColor), clrTransparent, mFont, Category, taCenter);

		if(mUseExtSkin == SKIN_SKINREEL3)
		{
			if(ShowTipFlag)
			{
				mOsd->DrawImage(imgIconTipp, x2-fFrameInfo.mInsetRight - imgSmallIconsWidth, y1+fFrameInfo.mInsetTop + max((mFont->Height("0") - imgSmallIconsHeigth), 0), true);
			}
			if(ShowTipFlag > 1)
			{
				mOsd->DrawImage(imgIconTipp, x2-fFrameInfo.mInsetRight - imgSmallIconsWidth  - imgSmallIconsWidth/2, y1+fFrameInfo.mInsetTop + max((mFont->Height("0") - imgSmallIconsHeigth), 0), true);
			}

			if(ShowLoveitFlag)
				mOsd->DrawImage(imgIconFavorites, x1+fFrameInfo.mInsetLeft, y1+fFrameInfo.mInsetTop + max((mFont->Height("0") - imgSmallIconsHeigth), 0), true);
		}
		lHeader = category_height + 1 /* mFont->Height("0") + 10 */;
		available_height -= category_height /* (mFont->Height("0") + 10) */;
	}


	if(available_height > 2 * mFont->Height("0"))
	{
		if(bPic)
		{
			cPngUtils pic;
			bPic = pic.ReadRawBitmap((char *)(const char*)PicturePath);
			if(bPic)
			{
#ifndef RBMINI
				pic.resize_image((x2-x1) - 2);
#else
				pic.resize_image_nearest_neighbour((x2-x1) - 2);	// more performance, less quality
#endif
                        	if(available_height > (pic.Height() +  2 * mFont->Height("0")))
     				{
   					int nImageIdPos;
					pic.WriteFileOsd(mOsd, 250, x1, y1 + lHeader, available_height>=pic.Height() ? 0 : available_height, &nImageIdPos, true);
   					lHeader += pic.Height();
				}
			}
		}
	}

	drawText (x1+fFrameInfo.mInsetLeft, y1+fFrameInfo.mInsetTop + lHeader, max(x2-fFrameInfo.mInsetRight, 0), max(y2-fFrameInfo.mInsetBottom, 0), GET_CLR(fFrameInfo.mTxtColor), clrTransparent, mFont, fText, taCenter);

} // cTimeLine::drawCell


void cTimeLine::drawButtons(int x1, int y1, int x2, const char *fRed, const char *fGreen, const char *fYellow, const char *fBlue) {
	DBG(DBG_FN,"cTimeLine::drawButtons(%d,%d,%d,%s,%s,%s,%s)",x1, y1, x2, fRed, fGreen, fYellow, fBlue);
	if(!mOsd)
		return;
	int lCount = 0;

	int Y1;

	// Adjust vertical offset for the skinreel3-Buttons
	if(mUseExtSkin == SKIN_SKINREEL3)
	   Y1 = y1 + 3;
	else
	   Y1 = y1;

	if (fRed)
		++ lCount;
	if (fGreen)
		++ lCount;
	if (fYellow)
		++ lCount;
	if (fBlue)
		++ lCount;

	// If skinreel3 paint the help-icon
	if(mUseExtSkin == SKIN_SKINREEL3)
        {
	    DrawButton(imgButtonHelp, x2 - imgIconHelpSkinreel3Width, Y1, 0);
	}
	if (!lCount)
		return;

	int lWidth;
	int lTxtWidth;
	int lPos;
	int lTxtPos;
	int lTxtOfs;

        switch(mUseExtSkin)
        {
	    case SKIN_SKINREEL3:
		lWidth    = imgButtonSkinreel3Width - imgButtonSkinreel3Roundness;
		lTxtWidth = imgButtonSkinreel3Width;
		lPos      = x2 - lCount * (imgButtonSkinreel3Width - imgButtonSkinreel3Roundness) - imgIconHelpSkinreel3Width;
		lTxtPos   = ((imgButtonSkinreel3Height-mFont->Height("0"))/2)+Y1;
		lTxtOfs   = 0;

		break;
	    case SKIN_SKINREELNG:
		lWidth    = (x2-x1-gDetailLeftFrame.mWidthLeft-gDetailRightFrame.mWidthRight) / 4;
		lTxtWidth = lWidth - gDetailLeftFrame.mInsetLeft-gDetailRightFrame.mInsetRight;
		lPos      = (x2-x1-gDetailLeftFrame.mWidthLeft-gDetailRightFrame.mWidthRight-lWidth*lCount)/2+x1+gDetailLeftFrame.mWidthLeft;
		lTxtPos   = ((gDetailLeftFrame.mHeightBottom-mFont->Height("0"))/2)+Y1;
		lTxtOfs   = gDetailLeftFrame.mInsetLeft;
		Y1 += (gDetailLeftFrame.mHeightBottom-24)/2;

		break;
            default:
		lWidth    = (x2-x1-imgBlue1Width-imgBlue3Width) / lCount;
		lTxtWidth = lWidth - imgButton1Width - imgButton3Width;
		lPos      = (x2-x1-imgBlue1Width-imgBlue3Width-lWidth*lCount)+x1+imgBlue1Width;
		lTxtPos   = ((imgButtonHeight-mFont->Height("0"))/2)+Y1;
		lTxtOfs   = imgButton1Width;

		break;
        }
	if (fRed) {
		DrawButton(imgButtonRed1, lPos, Y1, lPos+lWidth);
		mOsd->DrawText(lPos + lTxtOfs, lTxtPos, fRed, GET_CLR(themeClrButtonRedFg), clrTransparent, mFont, lTxtWidth, 0, taCenter);
		lPos += lWidth;
	} // if
	if (fGreen) {
		DrawButton(imgButtonGreen1, lPos, Y1, lPos+lWidth);
		mOsd->DrawText(lPos + lTxtOfs, lTxtPos, fGreen, GET_CLR(themeClrButtonGreenFg), clrTransparent, mFont, lTxtWidth, 0, taCenter);
		lPos += lWidth;
	} // if
	if (fYellow) {
		DrawButton(imgButtonYellow1, lPos, Y1, lPos+lWidth);
		mOsd->DrawText(lPos + lTxtOfs, lTxtPos, fYellow, GET_CLR(themeClrButtonYellowFg), clrTransparent, mFont, lTxtWidth, 0, taCenter);
		lPos += lWidth;
	} // if
	if (fBlue) {
		DrawButton(imgButtonBlue1, lPos, Y1, lPos+lWidth);
		mOsd->DrawText(lPos + lTxtOfs, lTxtPos, fBlue, GET_CLR(themeClrButtonBlueFg), clrTransparent, mFont, lTxtWidth, 0, taCenter);
	} // if
} // cTimeLine::drawButtons
void cTimeLine::DrawButton(int fImgIndex, int x1, int y1, int x2) {
	DBG(DBG_FN,"cTimeLine::DrawButton(%d,%d,%d,%d)",fImgIndex, x1, y1, x2);
	switch(mUseExtSkin) {
	    case SKIN_SKINREELNG:
		mOsd->DrawImage(fImgIndex, x1, y1, true, (x2-x1));
		break;
	    case SKIN_SKINREEL3:
		mOsd->DrawImage(fImgIndex, x1, y1, true);
		break;
	    default:
		mOsd->DrawImage(fImgIndex + 0, x1, y1, true);
		mOsd->DrawImage(fImgIndex + 1, x1 + imgButton1Width, y1, true, (x2-x1) - imgButton1Width - imgButton3Width);
		mOsd->DrawImage(fImgIndex + 2, x2 - imgButton3Width, y1, true);
		break;
	}
} // cTimeLine::DrawButton

void cTimeLine::drawScale() {
	DBG(DBG_FN,"cTimeLine::drawScale()");
	if (!mOsd || !mFont)
		return;
	if (gReelEPGCfg.mMagazineMode) {
		int lItemHeight  = mFont->Height("00:00")+gScaleFrame.minHeight();
		long lStep = 15l*60l;
		time_t lStart = (mStart / lStep)*lStep;
		if (GET_CLR(themeOptShowDate)) {
				cString lDate = DateOnlyString(lStart);
				drawCell(0, 0, mScaleWidth, mChanHeight, gDateFrame, lDate);
		} else if (GET_CLR(themeOptShowTime)) {
				cString lDate = TimeString(time(NULL));
				drawCell(0, 0, mScaleWidth, mChanHeight, gDateFrame, lDate);
		} // if

		mScaleNow      = time(NULL);
		mScalePosNow   = -1;
		mScalePosCount = 0;
		if (mScalePos)
			delete [] mScalePos;
		mScalePos = new int[(mEnd-mStart)/lStep];

		int lOffsetX     = gScaleFrame.mInsetLeft;
		int lOffsetY     = mChanHeight+2*gItemsFrame.mInsetTop+gItemNormalFrame.mInsetTop;
		int lScaleWidth  = mScaleWidth- gScaleFrame.insetWidth();;
		int lItemsHeight = mScaleHeight - 2*gItemsFrame.insetHeight();

//		int lItemsWidth  = mItemsWidth;

		long lEndPos = mChanHeight+gScaleFrame.mInsetTop-lOffsetY;
		drawFrame(mScaleWidth, mChanHeight, mScaleWidth+mItemsWidth, mChanHeight+mScaleHeight, gItemsFrame);
		mOsd->DrawRectangle(0, mChanHeight, mScaleWidth, mChanHeight+mScaleHeight, clrTransparent);
		drawFrame(0, mChanHeight, mScaleWidth, mChanHeight+mScaleHeight, gScaleFrame);

		while (lStart < mEnd) {
			long lStartPos = ((lStart-mStart)*lItemsHeight) / ((mEnd-mStart));
			if((lEndPos < lStartPos) && ((lStartPos+lItemHeight) < lItemsHeight)) {
					cString lTime = TimeString(lStart);
					drawText (lOffsetX, lStartPos+lOffsetY, lScaleWidth+lOffsetX, lStartPos+lItemHeight+lOffsetY, GET_CLR(gScaleFrame.mTxtColor), clrTransparent, mFont, lTime, taLeft);
/* Keine horizontalen Linien, da dies Flackern verursachen kann.
					mScalePos[mScalePosCount++] = mChanHeight+lStartPos+lOffsetY;
					mOsd->DrawRectangle(mScaleWidth+gItemsFrame.mInsetLeft, mChanHeight+lStartPos-1+lOffsetY, mScaleWidth+lItemsWidth-gItemsFrame.insetWidth(), mChanHeight+lStartPos+1+lOffsetY, GET_CLR(themeClrTimeScaleBg));
					mOsd->DrawRectangle(mScaleWidth+gItemsFrame.mInsetLeft, mChanHeight+lStartPos+lOffsetY, mScaleWidth+lItemsWidth-gItemsFrame.insetWidth(), mChanHeight+lStartPos+lOffsetY, GET_CLR(themeClrTimeScaleFg));
*/
					lEndPos = lStartPos+lItemHeight;
			} // if
			lStart += lStep;
		} // while
/* Keine horizontalen Linien, da dies Flackern verursachen kann.
		long lStartPos = ((mScaleNow-mStart)*lItemsHeight) / ((mEnd-mStart));
		if ((lStartPos > 0) && (lStartPos < lItemsHeight)) {
			mScalePosNow = mChanHeight+lStartPos+lOffsetY;
			mOsd->DrawRectangle(mScaleWidth+gItemsFrame.mInsetLeft, mScalePosNow-1, mScaleWidth+mItemsWidth-gItemsFrame.insetWidth(), mScalePosNow+1, GET_CLR(themeClrTimeNowBg));
			mOsd->DrawRectangle(mScaleWidth+gItemsFrame.mInsetLeft, mScalePosNow, mScaleWidth+mItemsWidth-gItemsFrame.insetWidth(), mScalePosNow, GET_CLR(themeClrTimeNowFg));
		} // if
*/
	} else {
		int lItemWidth  = mFont->Width("00:00")+gScaleFrame.minWidth();
		long lStep = 15l*60l;
		time_t lStart = (mStart / lStep)*lStep;
		if (GET_CLR(themeOptShowDate)) {
				cString lDate = ShortDateString(lStart);
				drawCell(0, 0, mChanWidth, mScaleHeight, gDateFrame, lDate);
		} else if (GET_CLR(themeOptShowTime)) {
				cString lDate = TimeString(time(NULL));
				drawCell(0, 0, mChanWidth, mScaleHeight, gDateFrame, lDate);
		} // if

		mScaleNow      = time(NULL);
		mScalePosNow   = -1;
		mScalePosCount = 0;
		if (mScalePos)
			delete [] mScalePos;
		mScalePos = new int[(mEnd-mStart)/lStep];

		int lOffset     = 0;
		int lItemsWidth = mItemsWidth;
		if (GET_CLR(themeOptItemsFrame)) {
			lOffset = gItemsFrame.mInsetLeft;
			lItemsWidth -= gItemsFrame.insetWidth();
		} // if

		long lEndPos = 0;
		mOsd->DrawRectangle(mChanWidth, 0, mChanWidth+mItemsWidth, mScaleHeight, clrTransparent);
		if (GET_CLR(themeOptItemsFrame)) {
			drawFrame(mChanWidth, mScaleHeight, mChanWidth+mItemsWidth, mScaleHeight+mRowsHeight, gItemsFrame, !GET_CLR(themeOptChannelsFrame), true);
		}	else
			mOsd->DrawRectangle(mChanWidth, mScaleHeight, mChanWidth+mItemsWidth, mScaleHeight+mRowsHeight, GET_CLR(themeClrItemsBg));
		if (GET_CLR(themeOptScaleFrame))
			drawFrame(mChanWidth, 0, mChanWidth+mItemsWidth, mScaleHeight, gScaleFrame);
		while (lStart < mEnd) {
			long lStartPos = ((lStart-mStart)*lItemsWidth) / ((mEnd-mStart));
			if((lEndPos < lStartPos-lItemWidth/2) && ((lStartPos+lItemWidth/2) < lItemsWidth)) {
					cString lTime = TimeString(lStart);
					if (GET_CLR(themeOptScaleFrame))
						drawText (mChanWidth+lStartPos-lItemWidth/2+lOffset, gScaleFrame.mInsetTop, mChanWidth+lStartPos+lItemWidth/2+lOffset, mScaleHeight-gScaleFrame.mInsetBottom, GET_CLR(gScaleFrame.mTxtColor), clrTransparent, mFont, lTime, taCenter);
					else
						drawCell(mChanWidth+lStartPos-lItemWidth/2+lOffset, 0, mChanWidth+lStartPos+lItemWidth/2+lOffset, mScaleHeight, gScaleFrame, lTime);
					mScalePos[mScalePosCount++] = mChanWidth+lStartPos+lOffset;
					mOsd->DrawRectangle(mChanWidth+lStartPos-1+lOffset, mScaleHeight, mChanWidth+lStartPos+1+lOffset, mScaleHeight+mRowsHeight, GET_CLR(themeClrTimeScaleBg));
					mOsd->DrawRectangle(mChanWidth+lStartPos+lOffset, mScaleHeight, mChanWidth+lStartPos+lOffset, mScaleHeight+mRowsHeight, GET_CLR(themeClrTimeScaleFg));
					lEndPos = lStartPos+lItemWidth/2;
			} // if
			lStart += lStep;
		} // while
		long lStartPos = ((mScaleNow-mStart)*lItemsWidth) / ((mEnd-mStart));
		if ((lStartPos > 0) && (lStartPos < lItemsWidth)) {
			mScalePosNow = mChanWidth+lStartPos+lOffset;
			mOsd->DrawRectangle(mScalePosNow-1, mScaleHeight, mScalePosNow+1, mScaleHeight+mRowsHeight, GET_CLR(themeClrTimeNowBg));
			mOsd->DrawRectangle(mScalePosNow, mScaleHeight, mScalePosNow, mScaleHeight+mRowsHeight, GET_CLR(themeClrTimeNowFg));
		} // if
	} // if
} // cTimeLine::drawScale
void cTimeLine::drawChannels() {
	DBG(DBG_FN,"cTimeLine::drawChannels()");
	if (gReelEPGCfg.mMagazineMode) {
		mOsd->DrawRectangle(mScaleWidth, 0, mScaleWidth+mChanWidth, mChanHeight, clrTransparent);
		for (int i = 0; i < getChannelCount(gReelEPGCfg.mScaleWidth); i++)
			drawChannel(mFirstChan+i);
	} else {
		if (GET_CLR(themeOptChannelsFrame))
			drawFrame(0, mScaleHeight, mChanWidth, mScaleHeight+mRowsHeight, gChannelsFrame, true, true);
		else
			mOsd->DrawRectangle(0, mScaleHeight, mChanWidth, mScaleHeight+mRowsHeight, clrTransparent);
		for (int i = 0; i < mRows; i++)
			drawChannel(mFirstChan+i);
	} // if
} // cTimeLine::drawChannels
void cTimeLine::drawChannel(int fChan) {
	DBG(DBG_FN,"cTimeLine::drawChannel(%d)",fChan);
	if (gReelEPGCfg.mMagazineMode) {
		mOsd->DrawRectangle(mScaleWidth+gItemsFrame.mInsetLeft+mChanWidth*(fChan-mFirstChan), 0, mScaleWidth+gItemsFrame.mInsetLeft+mChanWidth*(fChan-mFirstChan+1), mChanHeight, clrTransparent);
		cChannel *lChan = mChannels.Get(fChan);
		if (lChan)
			drawCell(mScaleWidth+gItemsFrame.mInsetLeft+mChanWidth*(fChan-mFirstChan), 0, mScaleWidth+gItemsFrame.mInsetLeft+mChanWidth*(fChan-mFirstChan+1), mChanHeight,(fChan==mSelChan)?gChannelSelectFrame:gChannelNormalFrame, lChan->ShortName(true));
		else if (gChannelsFrame.mFrmColor)
			drawCell(mScaleWidth+gItemsFrame.mInsetLeft+mChanWidth*(fChan-mFirstChan), 0, mScaleWidth+gItemsFrame.mInsetLeft+mChanWidth*(fChan-mFirstChan+1), mChanHeight,gChannelNormalFrame, " ");
		else
			mOsd->DrawRectangle(mScaleWidth+gItemsFrame.mInsetLeft+mChanWidth*(fChan-mFirstChan), 0, mScaleWidth+gItemsFrame.mInsetLeft+mChanWidth*(fChan-mFirstChan+1), mChanHeight, clrTransparent);
	} else {
		int lOffsetX = 0;
		int lOffsetY = 0;
		int lChanWidth = mChanWidth;
		if (GET_CLR(themeOptItemsFrame) && GET_CLR(themeOptChannelsFrame)) {
			lOffsetX = gChannelsFrame.mInsetLeft;
			lChanWidth -= gChannelsFrame.mInsetLeft+gChannelsFrame.mInsetRight;
		} else if (GET_CLR(themeOptChannelsFrame)) {
			lOffsetX = gChannelsFrame.mInsetLeft;
			lChanWidth -= gChannelsFrame.insetWidth();
		} // if
		if (GET_CLR(themeOptItemsFrame) || GET_CLR(themeOptChannelsFrame))
			lOffsetY = max(gChannelsFrame.mInsetTop, gItemsFrame.mInsetTop);
		cChannel *lChan = mChannels.Get(fChan);
		if (lChan)
			drawCell(lOffsetX, mScaleHeight+mChanHeight*(fChan-mFirstChan)+lOffsetY, lOffsetX+lChanWidth, mScaleHeight+mChanHeight*(fChan-mFirstChan+1)+lOffsetY,(fChan==mSelChan)?gChannelSelectFrame:gChannelNormalFrame, lChan->ShortName(true));
		else if (gChannelsFrame.mFrmColor)
			drawCell(lOffsetX, mScaleHeight+mChanHeight*(fChan-mFirstChan)+lOffsetY, lOffsetX+lChanWidth, mScaleHeight+mChanHeight*(fChan-mFirstChan+1)+lOffsetY,(fChan==mSelChan)?gChannelSelectFrame:gChannelNormalFrame, " ");
		else
			mOsd->DrawRectangle(lOffsetX, mScaleHeight+mChanHeight*(fChan-mFirstChan)+lOffsetY, lOffsetX+lChanWidth, mScaleHeight+mChanHeight*(fChan-mFirstChan+1)+lOffsetY, clrTransparent);
	} // if
} // cTimeLine::drawChannel
void cTimeLine::drawDetail() {
	DBG(DBG_FN,"cTimeLine::drawDetail() [%d+%d,%d]",mChanWidth,mItemsWidth,mDetailHeight);
	if (gReelEPGCfg.mMagazineMode) {
		const char *lRed    = NULL;
		const char *lGreen  = lPlugin_ReelChannellist ? tr("Favourites") : tr("Bouquets");
		const char *lYellow = NULL;
		const char *lBlue   = NULL;
		int  lMatch = tmNone;
		if (mSelEvent) {
			lYellow = tr("Information");
			lRed   = tr("Record");
			if (Timers.GetMatch(mSelEvent, &lMatch) && (lMatch==tmFull))
				lRed = tr("Edit");
			if (mSelEvent->EndTime() < time(NULL))
			  lRed = tr("Search");
			if (hasSwitchTimer(NULL,NULL,NULL) && (mSelEvent->StartTime() > time(NULL))) {
				bool lAnounce = false;
				if(hasSwitchTimer(mSelEvent,NULL,&lAnounce))
					lBlue = lAnounce ? tr("Normal") : tr("Anounce");
				else
					lBlue = tr("Switch");
			} // if
		} // if
		int lOffsetY = 0;
		switch(mUseExtSkin)
		{
		    case SKIN_SKINREEL3:
		    case SKIN_SKINREELNG:
			lOffsetY = mScaleHeight+mChanHeight+mDetailHeight;
			drawFrame(0, lOffsetY-gDetailLeftFrame.mHeightBottom, mScaleWidth+mItemsWidth, lOffsetY, gDetailLeftFrame, true, true);
 			drawButtons(0, lOffsetY-gDetailLeftFrame.mHeightBottom, mScaleWidth+mItemsWidth, lRed, lGreen, lYellow, lBlue);
			break;
		    default:
			mOsd->DrawRectangle(0, mChanHeight+mScaleHeight-gItemsFrame.mHeightBottom, mScaleWidth+mItemsWidth, mChanHeight+mScaleHeight+imgButtonHeight/2, clrTransparent);
			mOsd->DrawImage(gScaleFrame.mImgIndex+5, 0, mChanHeight+mScaleHeight-gScaleFrame.mHeightBottom, gScaleFrame.mBlend);
			mOsd->DrawImage(gScaleFrame.mImgIndex+6, gScaleFrame.mWidthLeft, mChanHeight+mScaleHeight-gScaleFrame.mHeightBottom, gScaleFrame.mBlend, mScaleWidth-gScaleFrame.mWidthLeft-gScaleFrame.mWidthRight);
			mOsd->DrawImage(gScaleFrame.mImgIndex+7, mScaleWidth-gScaleFrame.mWidthRight, mChanHeight+mScaleHeight-gScaleFrame.mHeightBottom, gScaleFrame.mBlend);

			mOsd->DrawImage(gItemsFrame.mImgIndex+5, mScaleWidth, mChanHeight+mScaleHeight-gItemsFrame.mHeightBottom, gItemsFrame.mBlend);
			mOsd->DrawImage(gItemsFrame.mImgIndex+6, mScaleWidth+gItemsFrame.mWidthLeft, mChanHeight+mScaleHeight-gItemsFrame.mHeightBottom, gItemsFrame.mBlend, mItemsWidth-gItemsFrame.mWidthLeft-gItemsFrame.mWidthRight);
			mOsd->DrawImage(gItemsFrame.mImgIndex+7, mScaleWidth+mItemsWidth-gItemsFrame.mWidthRight, mChanHeight+mScaleHeight-gItemsFrame.mHeightBottom, gItemsFrame.mBlend);
			drawButtons(0, mChanHeight+mScaleHeight-imgButtonHeight/2, mScaleWidth+mItemsWidth, lRed, lGreen, lYellow, lBlue);
			break;
		} // switch
	} else {
		cString lDate = mSelEvent ? ShortDateString(mSelEvent->StartTime()) : ShortDateString(mStart);
		drawFrame(0, mScaleHeight+mRowsHeight, mChanWidth, mScaleHeight+mRowsHeight+mDetailHeight, gDetailLeftFrame, true, false);
		drawFrame(mChanWidth, mScaleHeight+mRowsHeight, mChanWidth+mItemsWidth, mScaleHeight+mRowsHeight+mDetailHeight, gDetailRightFrame, false, true);

		if(mOsd)
			mOsd->DrawRectangle(0, mScaleHeight+mRowsHeight+mDetailHeight, mChanWidth+mItemsWidth, mScaleHeight+mRowsHeight+mDetailHeight+imgButtonHeight, clrTransparent);
		cChannel *lChan = mChannels.Get(mSelChan);

		const char *lRed    = NULL;
		const char *lGreen  = lPlugin_ReelChannellist ? tr("Favourites") : tr("Bouquets");
		const char *lYellow = NULL;
		const char *lBlue   = NULL;
		if (lChan) {
			int lHeight   = mFont->Height("0");
			int lPosLeft  = mScaleHeight+mRowsHeight+gDetailLeftFrame.mInsetTop +(mDetailHeight-gDetailLeftFrame.insetHeight ()-2*lHeight)/2;
			int lPosRight = mScaleHeight+mRowsHeight+gDetailRightFrame.mInsetTop+(mDetailHeight-gDetailRightFrame.insetHeight()-2*lHeight)/2;
			if(mUseExtSkin) {
				lPosLeft  = mScaleHeight+mRowsHeight+gDetailLeftFrame.mInsetTop;
				lPosRight = mScaleHeight+mRowsHeight+gDetailRightFrame.mInsetTop;
			} // if
			cChannel *lGroup =  getGroup(mChannels.Get(mSelChan)->Number());

			cString lText = cString::sprintf("%02d", lChan->Number());
			const cFont *lNumFont = cFont::GetFont(fontOsd);
			int lStart = lNumFont->Width(lText)+gDetailRightFrame.mInsetLeft;
			int lNumOff = (2*lHeight-lNumFont->Height(lText))/2;
			drawText (mChanWidth+gDetailRightFrame.mInsetLeft, lPosRight+lNumOff, mChanWidth+mItemsWidth-gDetailRightFrame.mInsetRight, lPosRight+2*lHeight, GET_CLR(gItemNormalFrame.mTxtColor), clrTransparent, lNumFont, lText);

			lText = cString::sprintf("%s (%s)", lChan->Name(), lGroup ? lGroup->Name() : "");
			drawText (gDetailLeftFrame.mInsetLeft, lPosLeft, mChanWidth+mItemsWidth+mChanWidth-gDetailLeftFrame.mInsetRight, lPosLeft+lHeight, GET_CLR(gDetailLeftFrame.mTxtColor), clrTransparent, mFont, lDate);
			drawText (mChanWidth+gDetailRightFrame.mInsetLeft+lStart, lPosRight, mChanWidth+mItemsWidth-gDetailRightFrame.mInsetRight, lPosRight+lHeight, GET_CLR(gDetailRightFrame.mTxtColor), clrTransparent, mFont, lText);
			int len = mFont->Width(lText);
			len += mFont->Width(" - ");
			lText = cString::sprintf("%s (%s)", (const char *)cSource::ToString(lChan->Source()), lChan->Provider() ? lChan->Provider() : "");
			drawText (mChanWidth+gDetailRightFrame.mInsetLeft+len+lStart, lPosRight, mChanWidth+mItemsWidth-gDetailRightFrame.mInsetRight, lPosRight+lHeight, GET_CLR(gItemNormalFrame.mTxtColor), clrTransparent, mFont, lText);
			if (mSelEvent) {
				lPosLeft  += lHeight;
				lPosRight += lHeight;
				lText = cString::sprintf("%s-%s",(const char *)mSelEvent->GetTimeString(), (const char *)mSelEvent->GetEndTimeString());
				drawText (gDetailLeftFrame.mInsetLeft, lPosLeft, mChanWidth+mItemsWidth+mChanWidth-gDetailLeftFrame.mInsetRight, lPosLeft+lHeight, GET_CLR(gDetailLeftFrame.mTxtColor), clrTransparent, mFont, lText);
				drawText (mChanWidth+gDetailRightFrame.mInsetLeft+lStart, lPosRight, mChanWidth+mItemsWidth-gDetailRightFrame.mInsetRight, lPosRight+lHeight, GET_CLR(gDetailRightFrame.mTxtColor), clrTransparent, mFont, mSelEvent->Title() ? mSelEvent->Title() : "");
				if(mSelEvent->ShortText()) {
					len = mFont->Width(mSelEvent->Title() ? mSelEvent->Title() : "");
					len += mFont->Width(" - ");
					drawText (mChanWidth+gDetailRightFrame.mInsetLeft+len+lStart, lPosRight, mChanWidth+mItemsWidth-gDetailRightFrame.mInsetRight, lPosRight+lHeight, GET_CLR(gItemNormalFrame.mTxtColor), clrTransparent, mFont, mSelEvent->ShortText());
				} // if
				int  lMatch = tmNone;
				lRed   = tr("Record");
				lYellow = tr("Information");
				if (Timers.GetMatch(mSelEvent, &lMatch) && (lMatch==tmFull))
					lRed = tr("Edit");
				if (mSelEvent->EndTime() < time(NULL))
				  lRed = tr("Search");
				if (hasSwitchTimer(NULL,NULL,NULL) && (mSelEvent->StartTime() > time(NULL))) {
					bool lAnounce = false;
					if(hasSwitchTimer(mSelEvent,NULL,&lAnounce))
						lBlue = lAnounce ? tr("Normal") : tr("Anounce");
					else
						lBlue = tr("Switch");
				} // if
			} // if
		} // if
		drawButtons(0, mScaleHeight+mRowsHeight+mDetailHeight-(mUseExtSkin?gDetailLeftFrame.mHeightBottom:imgButtonHeight/2), mChanWidth+mItemsWidth, lRed, lGreen, lYellow, lBlue);
	} // if
} // cTimeLine::drawDetail
void cTimeLine::drawItems() {
	DBG(DBG_FN,"cTimeLine::drawItems()");
	if (!mSelEvent)
		drawDetail();
	int lChannels = mRows;
	if (gReelEPGCfg.mMagazineMode)
		lChannels = getChannelCount(gReelEPGCfg.mScaleWidth);
	mEventInfos.Clear();
	for (int i = 0; i < lChannels; i++) {
		if(mUseExtSkin) {
			if (gReelEPGCfg.mMagazineMode) {
				int lOffsetX = mScaleWidth+gItemsFrame.mInsetLeft+mChanWidth*i;
				int lOffsetY = mChanHeight + gItemsFrame.mInsetTop;
				drawCell(lOffsetX, lOffsetY, lOffsetX+mChanWidth, lOffsetY+mScaleHeight-2*gItemsFrame.mInsetTop, gItemNormalFrame, " ", false, false);

			} else {
				int lOffsetY    = mScaleHeight+mChanHeight*i;
				if (GET_CLR(themeOptItemsFrame) || GET_CLR(themeOptChannelsFrame))
					lOffsetY += max(gChannelsFrame.mInsetTop, gItemsFrame.mInsetTop);
				drawCell(mChanWidth, lOffsetY, mChanWidth+mItemsWidth, lOffsetY+mChanHeight, gItemNormalFrame, " ", false, false);
			} // if
		} // if
		cChannel *lChan = mChannels.Get(mFirstChan+i);
		if (!lChan)
			continue;
		const cSchedule *lSched = mSchedules->GetSchedule(lChan->GetChannelID());
		if(!lSched)
			continue;
		const cList<cEvent> *lEvents = lSched->Events();
		if(!lEvents)
			continue;
		int lLastPos = 0;
		for (	cEvent *lEvent = lEvents->First(); lEvent; lEvent = lEvents->Next(lEvent)) {
			lLastPos = calcItem(mFirstChan+i,lEvent,lLastPos);
			drawItem(mFirstChan+i,lEvent);
		} // for
	} // for
} // cTimeLine::drawItems
int cTimeLine::calcItem(int fChannel, const cEvent *fEvent, int fLastPos) {
	int nShowTipFlag = 0;
	int nShowLoveitFlag = 0;

	if(!fEvent)
		return fLastPos;
	if ((mEnd < fEvent->StartTime()) || (mStart > fEvent->EndTime()))
		return fLastPos;
	DBG(DBG_FN,"cTimeLine::calcItem(%d,%ld,%d)",fChannel,(long)fEvent,fLastPos);

// added MQ
	cString sPicturePath = "";

  	cPngUtils pic;
  	char sPath[512];
  	sprintf(sPath, "%s/%ld.raw", gReelEPGCfg.mEPG_Images_Path.c_str(), (long)fEvent->EventID());

     	bool bPic = pic.PngIsReadable(sPath);
     	if(bPic)
     	{
		sPicturePath = sPath;
     	}
	// Check for Genre/Category
	char sCat[30];
	int nLen;

	if(mMyFavouriteEvents.find(itoa(fEvent->EventID())) != std::string::npos)
		nShowLoveitFlag = true;

	memset(sCat, 0, sizeof(sCat));
	if(fEvent->Description())
	{
		char *checktip = (char *) strstr(fEvent->Description(), tr ( "Toptip: " ));
		if(checktip != NULL)
		{
			nShowTipFlag = 1;
			if(strstr(checktip, tr ( "Tip of the day" )) != NULL)
				nShowTipFlag++;
		}

		if(strstr(fEvent->Description(), tr("Genre:")) != NULL)
		{
			char *ptr1 = (char *) strstr(fEvent->Description(), tr("Genre:"));
			char *ptr2 = (char *) strchr(fEvent->Description(), '\n');
			if(ptr2 == NULL) ptr2 = (char *) strchr(fEvent->Description(), '.');
				if(ptr2 == NULL) ptr2 = (char *) strchr(fEvent->Description(), ' ');

			nLen = ptr2 - ptr1 - strlen(tr("Genre:")) + 1;

			if(ptr1 != NULL && ptr2 != NULL && nLen < (int)sizeof(sCat))
			{
				memset(sCat, 0, sizeof(sCat));
				strn0cpy(sCat, ptr1+strlen(tr("Genre:")), nLen);
			}
			else
			{
				esyslog("reelpeg2: Category not set or exceeds max size of 30 bytes (%d) pointer1:%ld, pointer2:%ld", ptr2 - ptr1, (long)ptr1, (long)ptr2);
			}
		}
	}

	// remove trailing Blanks or "." or control chars
	nLen = strlen(sCat);
	while(nLen && (sCat[nLen-1] == ' ' || sCat[nLen-1] == '.' || sCat[nLen-1] < 0x20))
		nLen--;
	sCat[nLen] = '\0';

	cString fCat = cString::sprintf("%s", sCat);

	if (gReelEPGCfg.mMagazineMode) {
		int lItemsHeight = mScaleHeight - 2*gItemsFrame.insetHeight();
		if(mUseExtSkin)
			lItemsHeight = mScaleHeight - gItemsFrame.insetHeight() -1;
		int lOffsetX     = mScaleWidth+gItemsFrame.mInsetLeft+mChanWidth*(fChannel-mFirstChan);
		int lOffsetY     = mChanHeight+2*gItemsFrame.mInsetTop;

		long lStartPos = ((fEvent->StartTime()-mStart)*lItemsHeight) / ((mEnd-mStart));
		long lEndPos   = ((fEvent->EndTime()  -mStart)*lItemsHeight) / ((mEnd-mStart));
		bool lDrawTop    = true;
		bool lDrawBottom = true;
		if (lStartPos < 0) {
			lDrawTop = false;
			lStartPos = 0;
		} // if
		if (lEndPos > lItemsHeight) {
			lDrawBottom = false;
			lEndPos = lItemsHeight;
		} // if
		int lMoveY = 0;
		if (fLastPos > lStartPos+lOffsetY) {
			lMoveY = fLastPos - (lStartPos+lOffsetY);
			lStartPos = fLastPos-lOffsetY;
		} // if
		if ((lEndPos-lStartPos) < (gItemNormalFrame.insetHeight()+mFont->Height("0")))
			lEndPos = lStartPos + gItemNormalFrame.insetHeight()+mFont->Height("0");
		if (lEndPos > lItemsHeight)
			lEndPos = lItemsHeight;
		cString lText = cString::sprintf("%s\t%s\n%s",(const char *)TimeString(fEvent->StartTime()),
		                                              fEvent->Title()?fEvent->Title():"",
		                                              fEvent->ShortText()?fEvent->ShortText():"");
		mEventInfos.Add(new cEventInfo(fEvent, lOffsetX, lStartPos+lOffsetY, mChanWidth+lOffsetX, lEndPos+lOffsetY, lText, lDrawTop, lDrawBottom, lOffsetY+lMoveY, lItemsHeight, NULL /* xbmp */, fCat /* fCategory */, bPic, sPicturePath, nShowTipFlag, nShowLoveitFlag));
		return lEndPos+lOffsetY;
	} else {
		int lItemsWidth = mItemsWidth;
		int lOffsetX    = mChanWidth;
		int lOffsetY    = mScaleHeight+mChanHeight*(fChannel-mFirstChan);
		if (GET_CLR(themeOptItemsFrame) && GET_CLR(themeOptChannelsFrame)) {
			lItemsWidth -= gItemsFrame.mInsetRight;
		} else if (GET_CLR(themeOptItemsFrame)) {
			lOffsetX += gItemsFrame.mInsetLeft;
			lItemsWidth -= gItemsFrame.insetWidth();
		} // if
		if (GET_CLR(themeOptItemsFrame) || GET_CLR(themeOptChannelsFrame))
			lOffsetY += max(gChannelsFrame.mInsetTop, gItemsFrame.mInsetTop);
		long lStartPos = ((fEvent->StartTime()-mStart)*lItemsWidth) / ((mEnd-mStart));
		long lEndPos   = ((fEvent->EndTime()  -mStart)*lItemsWidth) / ((mEnd-mStart));
		bool lDrawLeft  = true;
		bool lDrawRight = true;
		if (lStartPos < 0) {
			lDrawLeft = false;
			lStartPos = 0;
		} // if
		if (lEndPos > lItemsWidth) {
			lDrawRight = false;
			lEndPos = lItemsWidth;
		} // if
		int lMoveX = 0;
		if (fLastPos > lStartPos+lOffsetX) {
			lMoveX = fLastPos - (lStartPos+lOffsetX);
			lStartPos = fLastPos-lOffsetX;
		} // if
		if ((lEndPos-lStartPos) < gItemNormalFrame.insetWidth())
			lEndPos = lStartPos + gItemNormalFrame.insetWidth();
		if (lEndPos > lItemsWidth)
			lEndPos = lItemsWidth;
		cString lText (fEvent->Title());
		mEventInfos.Add(new cEventInfo(fEvent, lStartPos+lOffsetX, lOffsetY, lEndPos+lOffsetX, mChanHeight+lOffsetY, lText, lDrawLeft, lDrawRight, lOffsetX+lMoveX, lItemsWidth, NULL, "", false, sPicturePath, nShowTipFlag, nShowLoveitFlag));
		return lEndPos+lOffsetX;
	} // if
} // cTimeLine::calcItem
void cTimeLine::drawItem(int fChannel, const cEvent *fEvent) {
	cEventInfo *lInfo = NULL;
	for (int i = 0; !lInfo && (i < mEventInfos.Count()); i++)
		if (mEventInfos.Get(i)->mEventRef == fEvent)
			lInfo = mEventInfos.Get(i);
	if(!lInfo)
		return;
//	DBG(DBG_FN,"cTimeLine::drawItem(%d,%ld,%ld,%ld,%ld,%ld)",fChannel,(long)fEvent, lStartPos, lEndPos, lRecStartPos, lRecEndPos);
	cTimer *lTimer = Timers.GetMatch(fEvent);
	if (lTimer) {
		long lRecStartPos = ((lTimer->StartTime()-mStart)*lInfo->mItemsPos) / ((mEnd-mStart));
		long lRecEndPos   = ((lTimer->StopTime() -mStart)*lInfo->mItemsPos) / ((mEnd-mStart));
		long lStartPos = ((fEvent->StartTime()-mStart)*lInfo->mItemsPos) / ((mEnd-mStart));
		long lEndPos   = ((fEvent->EndTime()  -mStart)*lInfo->mItemsPos) / ((mEnd-mStart));
		tFrameInfo *lNormal = (fEvent == mSelEvent) ? &gItemSelectFrame : &gItemNormalFrame;
		tFrameInfo *lRecord = (fEvent == mSelEvent) ? &gItemRecordSelectFrame : &gItemRecordNormalFrame;
		if (!GET_CLR(themeOptShowRN))
			lRecord = lNormal;
		if ( hasSwitchTimer(fEvent, NULL, NULL) && (fEvent->StartTime() > time(NULL))) {
			lNormal = (fEvent == mSelEvent) ? &gItemSwitchSelectFrame : &gItemSwitchNormalFrame;
			if (!GET_CLR(themeOptShowSR))
				lRecord = lNormal;
		} // if
		if ((lRecStartPos > lStartPos) && (lRecStartPos < lEndPos))
			drawCell(lInfo->mPosX1, lInfo->mPosY1, lInfo->mPosX2, lInfo->mPosY2,
			         *lNormal,
			         lInfo->mText, lInfo->mDrawTopLeft, lInfo->mDrawBottomRight,
			         lRecord,
			         lInfo->mOffset+lRecStartPos);
		else if ((lRecEndPos > lStartPos) && (lRecEndPos < lEndPos))
			drawCell(lInfo->mPosX1, lInfo->mPosY1, lInfo->mPosX2, lInfo->mPosY2,
			         *lRecord,
			         lInfo->mText, lInfo->mDrawTopLeft, lInfo->mDrawBottomRight,
			         lNormal,
			         lInfo->mOffset+lRecEndPos);
		else
			drawCell(lInfo->mPosX1, lInfo->mPosY1, lInfo->mPosX2, lInfo->mPosY2,
			         (fEvent == mSelEvent) ? gItemRecordSelectFrame : gItemRecordNormalFrame,
			         lInfo->mText, lInfo->mDrawTopLeft, lInfo->mDrawBottomRight, NULL, 0, /* lInfo->bmp */ NULL, lInfo->mCategory, lInfo->mPicture, lInfo->mPicturePath, lInfo->mShowTipFlag, lInfo->mShowLoveitFlag);  // MQ: added last six(8 - 2 Default) parameters
	} else if ( hasSwitchTimer(fEvent, NULL, NULL) && (fEvent->StartTime() > time(NULL)))
		drawCell(lInfo->mPosX1, lInfo->mPosY1, lInfo->mPosX2, lInfo->mPosY2,
		         (fEvent == mSelEvent) ? gItemSwitchSelectFrame : gItemSwitchNormalFrame,
		         lInfo->mText, lInfo->mDrawTopLeft, lInfo->mDrawBottomRight, NULL, 0, /* lInfo->bmp */ NULL, lInfo->mCategory, lInfo->mPicture, lInfo->mPicturePath, lInfo->mShowTipFlag, lInfo->mShowLoveitFlag);  // MQ: added last six(8 - 2 Default) parameters
	else
		drawCell(lInfo->mPosX1, lInfo->mPosY1, lInfo->mPosX2, lInfo->mPosY2,
		         (fEvent == mSelEvent) ? gItemSelectFrame : gItemNormalFrame,
		         lInfo->mText, lInfo->mDrawTopLeft, lInfo->mDrawBottomRight, NULL, 0, /* lInfo->bmp */ NULL, lInfo->mCategory, lInfo->mPicture, lInfo->mPicturePath, lInfo->mShowTipFlag, lInfo->mShowLoveitFlag);  // MQ: added last six(8 - 2 Default) parameters
	if (fEvent == mSelEvent)
		drawDetail();

} // cTimeLine::drawItem

void cTimeLine::refresh() {
	DBG(DBG_FN,"cTimeLine::refresh()");
	if(mOsd)
		mOsd->DrawRectangle(0, 0, Setup.OSDWidth, Setup.OSDHeight, clrTransparent);
	drawScale();
	drawChannels();
	drawDetail();
	drawItems();
	flushOsd();
} //cTimeLine::refresh
void cTimeLine::setChannel(int fChan) {
	DBG(DBG_FN,"cTimeLine::setChannel(%d,%d,%d)",fChan,mFirstChan,mRows);
	if (fChan < 0)
		fChan = mChannels.Count()-1;
	if (fChan >= mChannels.Count())
		fChan = 0;
	int           lOldSelChan  = mSelChan;
	const cEvent *lOldSelEvent = mSelEvent;
	mSelChan = fChan;
	int lChanCount = mRows;
	if(gReelEPGCfg.mMagazineMode)
		lChanCount = getChannelCount(gReelEPGCfg.mScaleWidth);
	if ((mSelChan < mFirstChan) || (mSelChan >= mFirstChan+lChanCount)) {
		mFirstChan    = (((mSelChan) / lChanCount) * lChanCount);
		cChannel *lChan = mChannels.Get(mSelChan);
		const cSchedule *lSched = (mSchedules && lChan) ? mSchedules->GetSchedule(lChan->GetChannelID()) : NULL;
		mSelEvent = lSched ? lSched->GetEventAround(max (lOldSelEvent?lOldSelEvent->StartTime():0, mStart) + ((min(lOldSelEvent?lOldSelEvent->EndTime():INT_MAX,mEnd)-max (lOldSelEvent?lOldSelEvent->StartTime():0, mStart)) / 2)) : NULL;
		drawScale();
		drawChannels();
		drawItems();
	} else {
		cChannel *lChan = mChannels.Get(mSelChan);
		const cSchedule *lSched = (mSchedules && lChan) ? mSchedules->GetSchedule(lChan->GetChannelID()) : NULL;
		mSelEvent = lSched ? lSched->GetEventAround(max (lOldSelEvent?lOldSelEvent->StartTime():0, mStart) + ((min(lOldSelEvent?lOldSelEvent->EndTime():INT_MAX,mEnd)-max (lOldSelEvent?lOldSelEvent->StartTime():0, mStart)) / 2)) : NULL;
		if (!mSelEvent)
			drawDetail();
		drawChannel(lOldSelChan);
		drawItem(lOldSelChan, lOldSelEvent);
		drawChannel(mSelChan);
		drawItem(mSelChan, mSelEvent);
	} // if
	flushOsd();
} // cTimeLine::setChannel
int cTimeLine::setChannelRange(int fChan) {
	int lRet = 0;
	mChannels.Clear();
	cChannel *lGroup =  getGroup(fChan);
	int lPos = -1;
	if (lGroup)
		lPos = lGroup->Index();
	cChannel *lChan = mChannelList->Get(++lPos);
	int lIndex = 0;
	while (lChan && !lChan->GroupSep()) {
		if (fChan == lChan->Number())
			lRet = lIndex;
//		mChannels.Add(new cChannel(*Channels.Get(lPos)));
		mChannels.Add(new cChannel(*mChannelList->Get(lPos)));
		lIndex++;
		lChan = mChannelList->Get(++lPos);
		if(lChan) lPos = lChan->Index();
	} // while
	return lRet;
} // cTimeLine::setChannelRange
void cTimeLine::setEvent(bool fNext) {
	DBG(DBG_FN,"cTimeLine::setEvent(%d)",fNext);
	const cEvent *lOldSelEvent = mSelEvent;
	if (lOldSelEvent) {
		if(fNext) {
			if (lOldSelEvent->EndTime() > mEnd) {
				mSelEvent = lOldSelEvent;
				mStart = mEnd;
				if(gReelEPGCfg.mMagazineMode)
					mEnd   = mStart + getTimeRange(gReelEPGCfg.mScaleHeight);
				else
					mEnd   = mStart + getTimeRange(gReelEPGCfg.mScaleWidth);
				drawScale();
				drawItems();
			} else {
				mSelEvent	= (const cEvent *)lOldSelEvent->Next();
				drawItem(mSelChan, lOldSelEvent);
				drawItem(mSelChan, mSelEvent);
				if(!mSelEvent)
					drawDetail();
			} // if
		} else {
			if (lOldSelEvent->StartTime() < mStart) {
				mSelEvent = lOldSelEvent;
				mEnd   = mStart;
				if(gReelEPGCfg.mMagazineMode)
					mStart = mEnd - getTimeRange(gReelEPGCfg.mScaleHeight);
				else
					mStart = mEnd - getTimeRange(gReelEPGCfg.mScaleWidth);
				drawScale();
				drawItems();
			} else {
				mSelEvent	= (const cEvent *)lOldSelEvent->Prev();
				drawItem(mSelChan, lOldSelEvent);
				drawItem(mSelChan, mSelEvent);
				if(!mSelEvent)
					drawDetail();
			} // if
		} // if
	} else {
			if (fNext) {
				cChannel *lChan = mChannels.Get(mSelChan);
				const cSchedule *lSched = (mSchedules && lChan) ? mSchedules->GetSchedule(lChan->GetChannelID()) : NULL;
				mSelEvent = lSched ? lSched->GetEventAround(mEnd) : NULL;
				while (mSelEvent && mSelEvent->Prev() && ((const cEvent *)mSelEvent->Prev())->EndTime() > mStart)
					mSelEvent = (const cEvent *)mSelEvent->Prev();
				if (mSelEvent && mSelEvent->StartTime() < mEnd) {
					drawItem(mSelChan, mSelEvent);
				} else {
					mStart = mEnd;
					if(gReelEPGCfg.mMagazineMode)
						mEnd   = mStart + getTimeRange(gReelEPGCfg.mScaleHeight);
					else
						mEnd   = mStart + getTimeRange(gReelEPGCfg.mScaleWidth);
					if (mSelEvent && mSelEvent->StartTime() > mEnd)
						mSelEvent = NULL;
					drawScale();
					drawItems();
				} // if
			} else {
				cChannel *lChan = mChannels.Get(mSelChan);
				const cSchedule *lSched = (mSchedules && lChan) ? mSchedules->GetSchedule(lChan->GetChannelID()) : NULL;
				mSelEvent = lSched ? lSched->GetEventAround(mStart) : NULL;
				while (mSelEvent && mSelEvent->Next() && ((const cEvent *)mSelEvent->Next())->StartTime() < mEnd)
					mSelEvent = (const cEvent *)mSelEvent->Next();
				if (mSelEvent && mSelEvent->EndTime() > mStart) {
					drawItem(mSelChan, mSelEvent);
				} else {
					mEnd   = mStart;
					if(gReelEPGCfg.mMagazineMode)
						mStart = mEnd - getTimeRange(gReelEPGCfg.mScaleHeight);
					else
						mStart = mEnd - getTimeRange(gReelEPGCfg.mScaleWidth);
					if (mSelEvent && mSelEvent->EndTime() < mStart)
						mSelEvent = NULL;
					drawScale();
					drawItems();
				} // if
			} // if
	} // if
	flushOsd();
} // cTimeLine::setEvent
void cTimeLine::switchMode() {
	gReelEPGCfg.mMagazineMode = !gReelEPGCfg.mMagazineMode;
	if(mUseExtSkin)
		SetDynamicSettingNG();
	mOsd->DrawRectangle(0, 0, Setup.OSDWidth, Setup.OSDHeight, clrTransparent);
	setScale(gReelEPGCfg.mScaleWidth, gReelEPGCfg.mScaleHeight);
} // cTimeLine::switchMode
int cTimeLine::getTimeRange(int fMode) {
	if(gReelEPGCfg.mMagazineMode) {
		if (Setup.OSDHeight > 720) { // HD 1080
			switch(fMode) {
				case 1 : return 120*60;
				case 2 : return 240*60;
				default: return 360*60;
			} // switch
		} else if (Setup.OSDHeight > 576) { // HD 720
			switch(fMode) {
				case 1 : return  90*60;
				case 2 : return 180*60;
				default: return 270*60;
			} // switch
		} else { // SD
			switch(fMode) {
				case 1 : return  60*60;
				case 2 : return 120*60;
				default: return 180*60;
			} // switch
		} // if
	} else {
		if (Setup.OSDHeight > 720) { // HD 1080
			switch(fMode) {
				case 1 : return 180*60;
				case 2 : return 360*60;
				default: return 540*60;
			} // switch
		} else if (Setup.OSDHeight > 576) { // HD 720
			switch(fMode) {
				case 1 : return 120*60;
				case 2 : return 270*60;
				default: return 420*60;
			} // switch
		} else { // SD
			switch(fMode) {
				case 1 : return  90*60;
				case 2 : return 180*60;
				default: return 270*60;
			} // switch
		} // if
	} // if
} // cTimeLine::getTimeRange
int cTimeLine::getChannelCount(int fMode) {
	if (Setup.OSDHeight > 720) { // HD 1080
		switch(fMode) {
			case 1 : return 4;
			case 2 : return 5;
			default: return 6;
		} // switch
	} else if (Setup.OSDHeight > 576) { // HD 720
		switch(fMode) {
			case 1 : return 3;
			case 2 : return 4;
			default: return 5;
		} // switch
	} else { // SD
		switch(fMode) {
			case 1 : return 2;
			case 2 : return 3;
			default: return 4;
		} // switch
	} // if
} // cTimeLine::getChannelCount
void cTimeLine::setScale(int fScaleWidth, int fScaleHeight) {
	DBG(DBG_FN,"cTimeLine::setScale(%d,%d)",fScaleWidth, fScaleHeight);
	if (fScaleWidth < 1)
		fScaleWidth = 1;
	if (fScaleWidth > 6)
		fScaleWidth = 6;
	if (fScaleHeight < 1)
		fScaleHeight = 1;
	if (fScaleHeight > 3)
		fScaleHeight = 3;

	gReelEPGCfg.mScaleWidth  = fScaleWidth;
	gReelEPGCfg.mScaleHeight = fScaleHeight;

	time_t lPoint = mStart+(mEnd - mStart)/2;
	if (mSelEvent)
		lPoint = mSelEvent->StartTime();
	else if(!mStart)
		lPoint = time(NULL);
	if (lPoint < time(NULL))
		lPoint = time(NULL);

	if(gReelEPGCfg.mMagazineMode) {
		mStart = lPoint;//-10l*60l*gReelEPGCfg.mScaleHeight;
		mEnd   = mStart + getTimeRange(gReelEPGCfg.mScaleHeight);

		mDetailHeight = 0;
		mScaleWidth  = max(mFont->Width("88.88.") + gScaleFrame.insetWidth(), gScaleFrame.minWidth());
		mChanHeight  = max(mFont->Height("00:00")+gChannelsFrame.insetHeight(), gChannelsFrame.minHeight());
		if(mUseExtSkin) {
			mDetailHeight = gDetailLeftFrame.mHeightBottom+1;
			mChanHeight  = max(mChanHeight, gChannelNormalFrame.mHeightTop+1);
		} // if
		mChanWidth   = (Setup.OSDWidth - mScaleWidth-gItemsFrame.insetWidth()) / getChannelCount(gReelEPGCfg.mScaleWidth);
		mItemsWidth  = mChanWidth * getChannelCount(gReelEPGCfg.mScaleWidth) + gItemsFrame.insetWidth();

		mScaleHeight  = Setup.OSDHeight - mDetailHeight - mChanHeight - imgButtonHeight/2;
		if(mUseExtSkin)
			mScaleHeight  = Setup.OSDHeight - mDetailHeight - mChanHeight;

//		mRows         =
//		mRowsHeight   =
		mFirstChan    = (((mSelChan) / getChannelCount(gReelEPGCfg.mScaleWidth)) * getChannelCount(gReelEPGCfg.mScaleWidth));
	} else {
		mStart = lPoint-getTimeRange(gReelEPGCfg.mScaleWidth)/20;
//		mStart = lPoint-(30*60)/gReelEPGCfg.mScaleWidth;
		mEnd   = mStart + getTimeRange(gReelEPGCfg.mScaleWidth);

		mChanWidth = mFont->Width("00:00-00:00") + gDetailLeftFrame.insetWidth();
		if (GET_CLR(themeOptShowDate))
			mChanWidth = max ( mChanWidth, mFont->Width(ShortDateString(time(NULL))) + gDateFrame.insetWidth());

		mItemsWidth   = Setup.OSDWidth-mChanWidth;
		mScaleHeight  = max(mFont->Height("00:00")+gScaleFrame.insetHeight(), gScaleFrame.minHeight());
		mChanHeight   = max(mFont->Height("0")*gReelEPGCfg.mScaleHeight+gItemNormalFrame.insetHeight(), gChannelNormalFrame.minHeight());

		mDetailHeight = max(mFont->Height("0")*2+gDetailLeftFrame.insetHeight()+imgButtonHeight/2,gDetailLeftFrame.minHeight());
		if(mUseExtSkin) {
			mScaleHeight  = min(mFont->Height("00:00")+gScaleFrame.insetHeight(), gScaleFrame.minHeight());
			mDetailHeight = max(mFont->Height("0")*2+gDetailLeftFrame.insetHeight()+gDetailLeftFrame.mHeightBottom+gDetailLeftFrame.mInsetTop,gDetailLeftFrame.minHeight());
		} // if

		mRows         = (Setup.OSDHeight - mScaleHeight - mDetailHeight - imgButtonHeight/2) / mChanHeight;
		if (GET_CLR(themeOptItemsFrame) || GET_CLR(themeOptChannelsFrame))
			mRows         = (Setup.OSDHeight - mScaleHeight - mDetailHeight - imgButtonHeight/2 - max(gChannelsFrame.insetHeight() , gItemsFrame.insetHeight())) / mChanHeight;
		if(mUseExtSkin)
			mRows     = (Setup.OSDHeight - mScaleHeight - mDetailHeight) / mChanHeight;
		mRowsHeight   = mRows * mChanHeight;
		if (GET_CLR(themeOptItemsFrame) || GET_CLR(themeOptChannelsFrame))
			mRowsHeight += max(gChannelsFrame.insetHeight() , gItemsFrame.insetHeight());

		mFirstChan    = (((mSelChan) / mRows) * mRows);
	} // if
	refresh();
} // cTimeLine::setScale
bool cTimeLine::setStartEvent(const cEvent *fEvent) {
	if(!fEvent)
		return true;	// No more events: return true to stop next / previous

	if(gReelEPGCfg.mMagazineMode) {
		mStart = fEvent->StartTime() - 60 * 30;
		mEnd   = mStart + getTimeRange(gReelEPGCfg.mScaleHeight);
	} else {
		mStart = fEvent->StartTime()  - 60 * 30;
		mEnd   = mStart + getTimeRange(gReelEPGCfg.mScaleWidth);
	} // if

	cChannel *lChan = mChannelList->GetByChannelID(fEvent->ChannelID());
	if(lChan)
	{
		mSelEvent = fEvent;
		mSelChan = setChannelRange(lChan->Number());
	} else
		return false;

	setScale(gReelEPGCfg.mScaleWidth, gReelEPGCfg.mScaleHeight);
	refresh();
	return true;
} // cTimeLine::setStartEvent
void cTimeLine::setStartTime(time_t fTime) {
	if(gReelEPGCfg.mMagazineMode) {
		mStart = fTime;
		mEnd   = mStart + getTimeRange(gReelEPGCfg.mScaleHeight);
	} else {
		mStart = fTime;
		mEnd   = mStart + getTimeRange(gReelEPGCfg.mScaleWidth);
	} // if
	cChannel *lChan = mChannels.Get(mSelChan);
	const cSchedule *lSched = (mSchedules && lChan) ? mSchedules->GetSchedule(lChan->GetChannelID()) : NULL;
	mSelEvent = lSched ? lSched->GetEventAround(mStart) : NULL;
	refresh();
} // cTimeLine::setStartTime
void cTimeLine::setCurrent() {
	bool bFound = false;
	if(mBrowseResultSet)
	{
		lIterateResultEvents = lSearchResultEvents.begin();
		while(lIterateResultEvents != lSearchResultEvents.end() && !bFound)
		{
			if(*lIterateResultEvents)
			{
				if((*lIterateResultEvents)->StartTime() >= time(NULL))
					bFound = true;
				else
					lIterateResultEvents++;
			}
		}
	}
	if(mBrowseResultSet && bFound)
	{
		cChannel *lChan = mChannelList->GetByChannelID((*lIterateResultEvents)->ChannelID());
		if (lChan)
			mSelChan = setChannelRange(lChan->Number());
		mSelEvent      = *lIterateResultEvents;
	} else
	{
		cChannel *lChan = GetCurrentChannel();
		mSelChan = 0;
		if (!lChan) {
			DBG(DBG_ERR,"cTimeLine::setCurrent(%d)-Failed to get Channel! Using first available channel.",cDevice::CurrentChannel());
			lChan = mChannelList->Get(mChannelList->GetNextNormal(-1));
		} // if
		if (lChan)
			mSelChan = setChannelRange(lChan->Number());
		else
			DBG(DBG_ERR,"cTimeLine::setCurrent(%d)-No Channel found!",cDevice::CurrentChannel());
		const cSchedule *lSched = (mSchedules && lChan) ? mSchedules->GetSchedule(lChan->GetChannelID()) : NULL;
		mSelEvent      = lSched ? lSched->GetPresentEvent() : NULL;
	}
	setScale(gReelEPGCfg.mScaleWidth, gReelEPGCfg.mScaleHeight);
} // cTimeLine::setCurrent
void cTimeLine::recordEvent() {
	DBG(DBG_FN,"cTimeLine::recordEvent()");
	cPlugin *lPlugin = cPluginManager::GetPlugin("epgsearch");
	if(!lPlugin)
		return;
	if(!mSelEvent)
		return;
	if (mSelEvent->EndTime() < time(NULL)) {
		deleteOsd();
		Epgsearch_search_v1_0 lData = {(char *)mSelEvent->Title()/*query*/,0/*mode*/,0/*channel*/,true/*title*/,false/*subtitle*/,false/*description*/,NULL/*resultmenu*/};
		if(!lPlugin->Service("Epgsearch-search-v1.0",(void *)&lData) || !lData.pResultMenu) {
			createOsd();
			return;
		 } // if
		mMenu = lData.pResultMenu;
		mMenu->Display();
	} else {
		int  lMatch = tmNone;
		cTimer *lTimer = Timers.GetMatch(mSelEvent, &lMatch);
		if (lMatch!=tmFull)
			lTimer = NULL;
		deleteOsd();
		Epgsearch_exttimeredit_v1_0 lData = { lTimer?lTimer:new cTimer(mSelEvent), !lTimer, mSelEvent};
		if(!lPlugin->Service("Epgsearch-exttimeredit-v1.0",(void *)&lData) || !lData.pTimerMenu) {
		createOsd();
		return;
		 } // if
		mMenu = lData.pTimerMenu;
		mMenu->Display();
	} // if
} // cTimeLine::recordEvent
void cTimeLine::showHelp() {
	DBG(DBG_FN,"cTimeLine::showHelp()");
	if(!HelpMenus.Load())
		return;
	deleteOsd();
	cString lTitle = cString::sprintf("%s %s (%s)",
	                                  "ReelEPG",
	                                  gReelEPGCfg.mMagazineMode?tr("Magazine view"):tr("Timeline view"),
	                                  Setup.OSDHeight > 720 ? "HD 1080" : Setup.OSDHeight > 576 ? "HD 720" : "SD");
	cHelpSection *hs = HelpMenus.GetSectionByTitle(lTitle);
	mMenu = new cMenuHelp(hs, lTitle);
	mMenu->Display();
} // cTimeLine::showHelp
void cTimeLine::showEvent() {
	DBG(DBG_FN,"cTimeLine::showEvent()");
	if(!mSelEvent)
		return;

	deleteOsd();
	mMenu = new cMenuEventInfo(mSelEvent);
	mMenu->Display();
} // cTimeLine::showEvent
void cTimeLine::switchEvent() {
	DBG(DBG_FN,"cTimeLine::switchEvent()");
	if(!mSelEvent || !hasSwitchTimer(NULL,NULL,NULL) || !(mSelEvent->StartTime() > time(NULL)))
		return;
	bool lAnounce = false;
	if ( hasSwitchTimer(mSelEvent, NULL, &lAnounce)) {
		if (lAnounce)
			deleteSwitchTimer(mSelEvent);
		else
			setSwitchTimer(mSelEvent, SWITCH_START, true);
	} else {
		setSwitchTimer(mSelEvent, SWITCH_START, false);
	} // if
	drawItem(mSelChan,mSelEvent);
	flushOsd();
} // cTimeLine::switchEvent
void cTimeLine::showGroups() {
	DBG(DBG_FN,"cTimeLine::showGroups()");
	deleteOsd();
	mMenu = new cMenuGroups(mChannels.Get(mSelChan) ? mChannels.Get(mSelChan)->Number() : -1, lPlugin_ReelChannellist);
	mMenu->Display();
} // cTimeLine::showGroups
bool cTimeLine::hasSwitchTimer(const cEvent*fEvent,int *fSwitchMinsBefore, bool *fAnounceOnly) {
	DBG(DBG_FN,"cTimeLine::hasSwitchTimer()");
#ifdef SWITCHTIMER
	cPlugin *lPlugin = cPluginManager::GetPlugin("epgsearch");
	if(!lPlugin)
		return false;
	if(!fEvent)
		return lPlugin->Service("Epgsearch-switchtimer-v1.0",NULL);
	Epgsearch_switchtimer_v1_0 lData = {fEvent,0/*query existance*/,0,0,false};
	if(!lPlugin->Service("Epgsearch-switchtimer-v1.0",(void *)&lData))
		return false;
	if(!lData.success)
		return false;
	if(fSwitchMinsBefore)
		*fSwitchMinsBefore = lData.switchMinsBefore;
	if(fAnounceOnly)
		*fAnounceOnly = lData.announceOnly;
	return true;
#else
	return false;
#endif
} // cTimeLine::hasSwitchTimer
bool cTimeLine::setSwitchTimer(const cEvent*fEvent,int fSwitchMinsBefore, bool fAnounceOnly) {
	DBG(DBG_FN,"cTimeLine::setSwitchTimer()");
#ifdef SWITCHTIMER
	cPlugin *lPlugin = cPluginManager::GetPlugin("epgsearch");
	if(!lPlugin)
		return false;
	Epgsearch_switchtimer_v1_0 lData = {fEvent,1/*add/modify*/,fSwitchMinsBefore,fAnounceOnly,false};
	if(!lPlugin->Service("Epgsearch-switchtimer-v1.0",(void *)&lData))
		return false;
	return lData.success;
#else
	return false;
#endif
} // cTimeLine::setSwitchTimer
bool cTimeLine::deleteSwitchTimer(const cEvent*fEvent) {
	DBG(DBG_FN,"cTimeLine::deleteSwitchTimer()");
#ifdef SWITCHTIMER
	cPlugin *lPlugin = cPluginManager::GetPlugin("epgsearch");
	if(!lPlugin)
		return false;
	Epgsearch_switchtimer_v1_0 lData = {fEvent,2/*delete*/,0,0,false};
	if(!lPlugin->Service("Epgsearch-switchtimer-v1.0",(void *)&lData))
		return false;
	return lData.success;
#else
	return false;
#endif
} // cTimeLine::deleteSwitchTimer

bool cTimeLine::GetMyFavouritesResults()
{
	if(!mBrowseResultSet && !gReelEPGCfg.mQueryActive)	// No "search and browse" and no search for favourites defined -> do nothing
		return false;
	if(!gReelEPGCfg.mQueryExpression.size())	// query expression defined?
		return false;

	cPlugin *lPlugin = cPluginManager::GetPlugin("epgsearch");
	if(!lPlugin)
		return false;

	Epgsearch_searchresults_v1_0 lData = {(char *)gReelEPGCfg.mQueryExpression.c_str() /*query*/,gReelEPGCfg.mQueryMode/*mode*/,gReelEPGCfg.mQueryChannelNr/*channel*/,gReelEPGCfg.mQueryUseTitle/*title*/,gReelEPGCfg.mQueryUseSubTitle/*subtitle*/,gReelEPGCfg.mQueryUseDescription/*description*/, NULL};

	if(!lPlugin->Service("Epgsearch-searchresults-v1.0",(void *)&lData))
		return false;

	mMyFavouriteEvents = "";
	lSearchResultEvents.clear();

    	// Check if events in list
    	// epgsearch crashes during search with no events in list
    	bool bEventsAvailable = false;
    	cSchedulesLock SchedulesLock;
    	cSchedules *s = ( cSchedules * ) cSchedules::Schedules(SchedulesLock);
	if (s)
    	{
		for (cSchedule *p = s->First(); p  && !bEventsAvailable; p = s->Next(p))
		{
	    		const cList<cEvent> *lEvents = p->Events();
	    		if(lEvents->Count())
	        		bEventsAvailable = true;
		}
    	}
    	if(!bEventsAvailable)
    	{
		esyslog("reelepg; No events available -> do not call epgsearch");
		return true;
	}

	if(!lData.pResultList->Count())
		return false;

	// move resultset into browsing result set and construct list of events (channelnr:eventid) found
	cList<Epgsearch_searchresults_v1_0::cServiceSearchResult> *rptr = lData.pResultList;

	for(Epgsearch_searchresults_v1_0::cServiceSearchResult *it = rptr->First();it;it = rptr->Next(it))
	{
		lSearchResultEvents.push_back(it->event);
		mMyFavouriteEvents += itoa( mChannelList->GetByChannelID(it->event->Schedule()->ChannelID())->Number());
		mMyFavouriteEvents += ":";
		mMyFavouriteEvents += itoa(it->event->EventID());
		mMyFavouriteEvents += ",";
	}

	lIterateResultEvents = lSearchResultEvents.begin();

	return true;
} // cTimeLine::GetMyFavouritesResults()

void cTimeLine::SetMyFavouritesQuery(Reelepg_setquery_v1_0 *SetQuery)
{
	// Save query not set -> assume browse result set
	mBrowseResultSet = !SetQuery->save_query;

//	Reelepg_setquery = SetQuery;
	gReelEPGCfg.mQueryActive = SetQuery->save_query;
	gReelEPGCfg.mQueryExpression = SetQuery->query;
	gReelEPGCfg.mQueryMode = SetQuery->mode;
	gReelEPGCfg.mQueryChannelNr = SetQuery->channelNr;
	gReelEPGCfg.mQueryUseTitle = SetQuery->useTitle;
	gReelEPGCfg.mQueryUseSubTitle = SetQuery->useSubTitle;
	gReelEPGCfg.mQueryUseDescription = SetQuery->useDescription;
	GetMyFavouritesResults();
}

int cTimeLine::GetPrevGroup(int Idx)
{
  cChannel *channel = mChannelList->Get(Idx);
  while (channel && !(channel->GroupSep()))
        channel = mChannelList->Get(--Idx);
  return channel ? Idx : -1;
}

cChannel *cTimeLine::GetCurrentChannel()
{
	cChannel *lChan;
	tChannelID cid;

	if(!mChannelList)
		SetLocalChannelList();

        int chn = cDevice::PrimaryDevice()->CurrentChannel();

	if(lPlugin_ReelChannellist)
	{
            	cChannel *channel = Channels.GetByNumber(chn);

		cid = channel->GetChannelID();
		lChan = mChannelList->GetByChannelID(cid);

	} else
	{
		DBG(DBG_FN,"cTimeLine::setCurrent(%d)", chn);
		lChan = mChannelList->GetByNumber(chn);
	}
	return lChan;
}

int cTimeLine::GetVDRChannel(int lChan)
{
	if(!lPlugin_ReelChannellist)
		return lChan;

	tChannelID cid;
	cChannel *chan = mChannelList->GetByNumber(lChan);
	cid = chan->GetChannelID();
	cChannel *Chan = Channels.GetByChannelID(cid);

	return Chan->Number();
}

cMenuGroups::cMenuGroups(int fChan, cPlugin *lPlugin_ReelChannellist):cOsdMenu(lPlugin_ReelChannellist ? tr("Favourites") :tr("Favourites selection")) {
	DBG(DBG_FN,"cMenuGroups::cMenuGroups(%d)",fChan);
	cChannel *lGroup = getGroup(fChan);
	cItemGroup *lItem = NULL;
	if (mChannelList->First() && !mChannelList->First()->GroupSep()) {
		Add(lItem = new cItemGroup(NULL, lPlugin_ReelChannellist));
		if (lGroup == NULL)
			SetCurrent(lItem);
	} // if
	for (cChannel *lChannel = mChannelList->First(); lChannel; lChannel = mChannelList->Next(lChannel)) {
		if (lChannel->GroupSep()) {
			Add(lItem = new cItemGroup(lChannel, lPlugin_ReelChannellist));
			if (lGroup == lChannel)
				SetCurrent(lItem);
		} // if
	} // for
} // cMenuGroups::cMenuGroups
cItemGroup::cItemGroup(const cChannel *fChannel, cPlugin *lPlugin_ReelChannellist) : cOsdItem(fChannel ? fChannel->Name() : lPlugin_ReelChannellist ? tr("Main favourite") : tr("Main bouquet")) {
	mChannel = fChannel;
} // cItemGroup::cItemGroup
