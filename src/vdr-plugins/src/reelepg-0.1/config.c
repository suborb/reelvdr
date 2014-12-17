/*
 * config.h: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 *
 */
#include "config.h"

#include <getopt.h>
#include <unistd.h>
#include <vdr/menuitems.h>
#include <vdr/plugin.h>
#include <stdarg.h>

#define DEFAULT_EPGIMAGES_PATH "/media/reel/pictures/.iepg/images"

cReelEPGConfig gReelEPGCfg;

cReelEPGConfig::cReelEPGConfig(void) {
    DBG(DBG_FN, "cReelEPGConfig::cReelEPGConfig()");
    mDebug = 0;
    mScaleWidth   = 2;
    mScaleHeight  = 1;
    mMagazineMode = 1;

	mFontPath       = "/usr/share/reel/skinfonts/";
	//mFontName       = "Vera.ttf";
	mFontName       = "lmsans10-regular.otf";
	mFontSizeBig    = 20;
	mFontSizeNormal = 18;
	mFontSizeSmall  = 16;
	mFontWidth      = 100;
	mFontFormat     = 0;

	mEPG_Images_Path = DEFAULT_EPGIMAGES_PATH;
} // cReelEPGConfig::cReelEPGConfig
cReelEPGConfig::~cReelEPGConfig() {
    DBG(DBG_FN, "cReelEPGConfig::~cReelEPGConfig()");
} // cReelEPGConfig::~cReelEPGConfig
bool cReelEPGConfig::SetupParse(const char *Name, const char *Value) {
    DBG(DBG_FN, "cReelEPGConfig::SetupParse(%s,%s)", Name, Value);
    if      (!strcasecmp(Name, "ScaleWidth" )) gReelEPGCfg.mScaleWidth  = atoi(Value);
    else if (!strcasecmp(Name, "ScaleHeight")) gReelEPGCfg.mScaleHeight = atoi(Value);
    else if (!strcasecmp(Name, "Mode")) gReelEPGCfg.mMagazineMode = !strcasecmp(Value, "Magazine");
	else if (!strcasecmp(Name, "TTFontPath")) gReelEPGCfg.mFontPath = Value;
	else if (!strcasecmp(Name, "TTFontName")) gReelEPGCfg.mFontName = Value;
	else if (!strcasecmp(Name, "TTFontSizeBig")) gReelEPGCfg.mFontSizeBig = atoi(Value);
	else if (!strcasecmp(Name, "TTFontSizeSmall")) gReelEPGCfg.mFontSizeSmall = atoi(Value);
	else if (!strcasecmp(Name, "TTFontSizeNormal")) gReelEPGCfg.mFontSizeNormal = atoi(Value);
	else if (!strcasecmp(Name, "TTFontWidth")) gReelEPGCfg.mFontWidth = atoi(Value);
	else if (!strcasecmp(Name, "TTFontFormat")) gReelEPGCfg.mFontFormat = atoi(Value);
	else if (!strcasecmp(Name, "QueryExpression")) gReelEPGCfg.mQueryExpression = Value;
// Settings for query via epgsearch
	else if (!strcasecmp(Name, "QueryActive")) gReelEPGCfg.mQueryActive = atoi(Value);
	else if (!strcasecmp(Name, "QueryMode")) gReelEPGCfg.mQueryMode = atoi(Value);
	else if (!strcasecmp(Name, "QueryChannelNr")) gReelEPGCfg.mQueryChannelNr = atoi(Value);
	else if (!strcasecmp(Name, "QueryUseTitle")) gReelEPGCfg.mQueryUseTitle = atoi(Value);
	else if (!strcasecmp(Name, "QueryUseSubTitle")) gReelEPGCfg.mQueryUseSubTitle = atoi(Value);
	else if (!strcasecmp(Name, "QueryUseDescription")) gReelEPGCfg.mQueryUseDescription = atoi(Value);
	else return false;
    return true;
} // cReelEPGConfig::SetupParse
bool cReelEPGConfig::StoreSetup(cPlugin *fPlugin) {
    DBG(DBG_FN, "cReelEPGConfig::StoreSetup(%ld)", (long)fPlugin);
    if(!fPlugin)
		return false;
    fPlugin->SetupStore("ScaleWidth" , gReelEPGCfg.mScaleWidth);
    fPlugin->SetupStore("ScaleHeight", gReelEPGCfg.mScaleHeight);
    fPlugin->SetupStore("Mode", gReelEPGCfg.mMagazineMode ? "Magazine" : "Timeline");
	fPlugin->SetupStore("TTFontPath", gReelEPGCfg.mFontPath.c_str());
	fPlugin->SetupStore("TTFontName", gReelEPGCfg.mFontName.c_str());
	fPlugin->SetupStore("TTFontSizeBig", gReelEPGCfg.mFontSizeBig);
	fPlugin->SetupStore("TTFontSizeSmall", gReelEPGCfg.mFontSizeSmall);
	fPlugin->SetupStore("TTFontSizeNormal", gReelEPGCfg.mFontSizeNormal);
	fPlugin->SetupStore("TTFontWidth", gReelEPGCfg.mFontWidth);
	fPlugin->SetupStore("TTFontFormat", gReelEPGCfg.mFontFormat);
//
	fPlugin->SetupStore("QueryExpression", gReelEPGCfg.mQueryExpression.c_str());
	fPlugin->SetupStore("QueryActive", gReelEPGCfg.mQueryActive);
	fPlugin->SetupStore("QueryMode", gReelEPGCfg.mQueryMode);
	fPlugin->SetupStore("QueryChannelNr", gReelEPGCfg.mQueryChannelNr);
	fPlugin->SetupStore("QueryUseTitle", gReelEPGCfg.mQueryUseTitle);
	fPlugin->SetupStore("QueryUseSubTitle", gReelEPGCfg.mQueryUseSubTitle);
	fPlugin->SetupStore("QueryUseDescription", gReelEPGCfg.mQueryUseDescription);

	return true;
} // cReelEPGConfig::StoreSetup
const char *cReelEPGConfig::CommandLineHelp(void) {
    DBG(DBG_FN, "cReelEPGConfig::CommandLineHelp()");
    // Return a string that describes all known command line options.
    return " -d DebugLevel -i epg_image_folder\n";
} // cReelEPGConfig::CommandLineHelp
bool cReelEPGConfig::ProcessArgs(int argc, char *argv[]) { 
    DBG(DBG_FN, "cReelEPGConfig::ProcessArgs(%d,...)", argc);
    static struct option long_options[] = {
		{ "debug", required_argument, NULL, 'd' },
		{ "epgimagefolder", optional_argument, NULL, 'i' },
		{ NULL }};
    bool retval=true;
    int c;
    while ((c = getopt_long(argc, argv, "d:i:", long_options, NULL)) != -1) {
	switch (c) {
	    case 'd':
		if(isnumber(optarg))
		    mDebug=atoi(optarg);
		break;
	    case 'i':
		if(strlen(optarg))
		    mEPG_Images_Path=optarg;
		break;
	    default:  
		break;
	} // switch
  } // while
  return retval;
} // cReelEPGConfig::ProcessArgs

