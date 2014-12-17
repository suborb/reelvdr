/*
 * timeline.h: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 *
 */
#ifndef REELEPG_TIMELINE_H
#define REELEPG_TIMELINE_H

#include <vdr/plugin.h>
#include <vdr/device.h>
#include <time.h>
#include <string>
#include "config.h"
#include "common.h"

#include "pngutils.h"
#include "service.h"

extern cChannels *mChannelList;

struct tFrameInfo {
	int mImgIndex;
	const char* mFrmColor;
	const char* mBgColor;
	const char* mTxtColor;
	int mBlend;
	int mShadow;
	int mLines;
	int mInsetLeft;
	int mInsetRight;
	int mInsetTop;
	int mInsetBottom;
	int mWidthLeft;
	int mWidthRight;
	int mHeightTop;
	int mHeightBottom;
	int minHeight() {return mHeightTop+mHeightBottom;}
	int minWidth () {return mWidthLeft+mWidthRight;}
	int insetHeight() {return mInsetTop+mInsetBottom;}
	int insetWidth () {return mInsetLeft+mInsetRight;}
}; // tFrameInfo

class cEventInfo : public cListObject {
	public :
        	unsigned char* bmp;  // Bitmap Raw-Format
		cString mPicturePath;

	        int mCategoryHeight;
		int mPicHeight;

		const cEvent *mEventRef;
		int mPosX1;
		int mPosY1;
		int mPosX2;
		int mPosY2;
		bool mDrawTopLeft;
		bool mDrawBottomRight;
		int mOffset;
		int mItemsPos;
		cString mText;
		cString mCategory;
                bool mPicture;

		int mShowTipFlag;
		int mShowLoveitFlag;

		cEventInfo(const cEvent *fEventRef, int fPosX1, int fPosY1, int fPosX2, int fPosY2, cString &fText, bool fDrawTopLeft, bool fDrawBottomRight, int fOffset, int fItemsPos, unsigned char *fbmp, cString fCategory, bool fPicture, cString fPicturePath, int fShowTipFlag, int fShowLoveitFlag) {
			mEventRef = fEventRef;
			mPosX1 = fPosX1;
			mPosY1 = fPosY1;
			mPosX2 = fPosX2;
			mPosY2 = fPosY2;
			mText  = fText;
			mDrawTopLeft = fDrawTopLeft;
			mDrawBottomRight = fDrawBottomRight;
			mOffset = fOffset;
 			mItemsPos = fItemsPos;
// Added MQ
			bmp = fbmp;
			mCategory = fCategory;
                	mPicture = fPicture;
			mPicturePath = fPicturePath;

		        mCategoryHeight = fCategory == "" ? 0 : 30;
			mPicHeight = fPicture ? 120 : 0;

			mShowTipFlag = fShowTipFlag;   // Show TopTipp or Tip of the day if available 
			mShowLoveitFlag = fShowLoveitFlag; // Show MayFavorites icon if available

		} // cEventInfo
}; // cEventInfo

class cTimeLine : public cOsdObject {
  friend class cPluginReelEPG;
	public:
		cTimeLine(class cPlugin *);
		virtual ~cTimeLine();
		virtual void Show(void);
		virtual eOSState ProcessKey(eKeys Key);
		static void AddThemeColors();
		static void LoadFontsNG();
	private:
		cOsd *mOsd;
		cPlugin *lPlugin_ReelChannellist;
		cPlugin *mParent;
		const cFont *mFont;
		cSchedulesLock mSchedLock;
		const cSchedules *mSchedules;
		const cEvent *mSelEvent;
		int mFirstChan; 
		int mSelChan;
		time_t mScaleNow; 
		time_t mStart;
		time_t mEnd;
		int mChanWidth;
		int mItemsWidth;
		int mScaleHeight;
		int mChanHeight;
		int mDetailHeight;
		int mRows;
		int mRowsHeight;
		int mScaleWidth;
		cList<cEventInfo> mEventInfos;
		cList<cChannel> mChannels;
		int mScalePosNow;
		int *mScalePos;
		int mScalePosCount;
		time_t mLastInput;
		int mMaxCharWidth;
		int mUseExtSkin;
		int mShowConflict;
		cOsdMenu *mMenu;		
		void createOsd();
		void deleteOsd();
		void flushOsd();
		void SetDynamicSettingNG();
		void SetImagePaths();
        	void SetImagePathsNG();
        	void AddThemeColorsNG();
        	void SetImagePathsSkinreel3();
//        void AddThemeColorsSkinreel3();
        	void drawFrame(int x1, int y1, int x2, int y2, tFrameInfo &fFrameInfo, bool fTopLeftEnd = true, bool fBottomRightEnd = true);
		void drawText(int x1, int y1, int x2, int y2, tColor fFG, tColor fBG, const cFont* fFont, const char * fText, int fAlign = taDefault);
		void drawCell(int x1, int y1, int x2, int y2, tFrameInfo &fFrameInfo, const char * fText, bool fTopLeftEnd = true, bool fBottomRightEnd = true, tFrameInfo *lRight = NULL, int lSplit = 0, unsigned char *bmp = NULL, cString Category="", bool bPic=false, cString fPicturePath="", int ShowTipFlag = 0, int ShowLoveitFlag = 0);
		void drawButtons(int x1, int y1, int x2, const char *fRed, const char *fGreen, const char *fYellow, const char *fBlue);
		void DrawButton(int fImgIndex, int x1, int y1, int x2);
		void drawScale();
		void drawChannels();
		void drawChannel(int fChan);
		void drawDetail();
		void drawItems();
		int calcItem(int fChan, const cEvent *fEvent, int fLastPos);
		void drawItem(int fChan, const cEvent *fEvent);
		void refresh();
		void setChannel(int fChan);
		int setChannelRange(int fChan);
		void setEvent(bool fNext);
		void switchMode();
		int getTimeRange(int fMode);
		int getChannelCount(int fMode);
		void setScale(int fScaleWidth, int fScaleHeight);
		void setStartTime(time_t fTime);
		void setCurrent();
		void recordEvent();
		void showHelp();
		void showEvent();
		void switchEvent();
		void showGroups();
		bool hasSwitchTimer(const cEvent*fEvent,int *fSwitchMinsBefore, bool *fAnounceOnly);
		bool setSwitchTimer(const cEvent*fEvent,int fSwitchMinsBefore, bool fAnounceOnly);
		bool deleteSwitchTimer(const cEvent*fEvent);

		bool setStartEvent(const cEvent *fEvent);

		static bool mBrowseResultSet;
		static void SetMyFavouritesQuery(Reelepg_setquery_v1_0 *SetQuery);
		static bool GetMyFavouritesResults();

		static std::string mMyFavouriteEvents;
		static std::vector<const cEvent *> lSearchResultEvents;
		static std::vector<const cEvent *>::iterator lIterateResultEvents;

//		static Reelepg_setquery_v1_0 *Reelepg_setquery;

		int GetPrevGroup(int Idx);
  		cChannel *GetCurrentChannel();  // Get Current Channel 
		void SetLocalChannelList();
		int GetVDRChannel(int lChan);

}; // cTimeLine
#endif // REELEPG_TIMELINE_H
