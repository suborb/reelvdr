/*
 * config.h: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 *
 */
#ifndef REELEPG_CONFIG_H
#define REELEPG_CONFIG_H

#include <vdr/config.h>
#include <vdr/menuitems.h>
#include <string>

class cReelEPGConfig {
    public:
	cReelEPGConfig(void);
	~cReelEPGConfig();
	bool SetupParse(const char *Name, const char *Value);
	bool StoreSetup(cPlugin *fPlugin);
	bool ProcessArgs(int argc, char *argv[]);
	const char *CommandLineHelp(void);
	int mDebug;
	int mScaleWidth;
	int mScaleHeight;
	int mMagazineMode;

	std::string mEPG_Images_Path;	

	std::string mFontPath;
	std::string mFontName;
	int mFontSizeBig;
	int mFontSizeSmall;
	int mFontSizeNormal;
	int mFontWidth;
	int mFontFormat;

	bool mQueryActive;
	std::string mQueryExpression;
        int mQueryMode;             	// search mode (0=phrase, 1=and, 2=or, 3=regular expression)
        int mQueryChannelNr;            // channel number to search in (0=any)
        bool mQueryUseTitle;            // search in title
        bool mQueryUseSubTitle;         // search in subtitle
        bool mQueryUseDescription;      // search in description
}; // cReelEpgConfig

extern cReelEPGConfig gReelEPGCfg;

#define DBG_ERR 0
#define DBG_ENV 9
#define DBG_FN 10
#define DBG(l,a...) void( (gReelEPGCfg.mDebug >= l) ? syslog((l?LOG_ERR:LOG_INFO), a) : void() )

#endif 


