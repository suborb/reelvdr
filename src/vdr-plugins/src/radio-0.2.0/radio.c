/*
 * radio.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include <vdr/status.h>
#include <vdr/config.h>
#include <vdr/interface.h>
#include <vdr/transfer.h>
#include "getopt.h"
#include "radioaudio.h"
#include "i18n.h"
//file exists
#include <sys/stat.h>

static const char *VERSION        = "0.2.0";
static const char *DESCRIPTION    = "Radio Background-Image/RDS-Text";
static const char *MAINMENUENTRY  = "RDS-Radiotext";

char *ConfigDir;
char *DataDir;
char *LiveFile;
char *ReplayFile;

// Setup-Params
int S_Activate = true;
int S_HMEntry = false;
int S_RtFunc = 1;
int S_RtOsdTitle = 1;
int S_RtOsdTags = 2;
int S_RtOsdPos = 2;
int S_RtOsdRows = 3;
int S_RtOsdLoop = 0;
int S_RtOsdTO = 500;
int S_RtSkinColor = false;
int S_RtBgCol = 0;
int S_RtBgTra = 0xA0;
int S_RtFgCol = 1;
int S_RtDispl = 2;
int S_RtMsgItems = 0;
//int S_RtpMemNo = 25;
int S_QDarText = 1;
uint32_t rt_color[9];
int S_Verbose = 1;
// Radiotext
char RT_Text[5][RT_MEL];
char RTP_Artist[RT_MEL], RTP_Title[RT_MEL];
int RT_Info, RT_Index, RT_PTY;
time_t RTP_Starttime;
bool RT_OsdTO = false, RTplus_Osd = false;
int RT_OsdTOTemp = 0;
// RadioCheck
const cChannel *chan;
int IsRadioOrReplay;


struct RadioTextService_v1_0 {
    int rds_info;		// 0= no / 1= Text / 2= Text + RTplus-Tags (Item,Artist)
    int rds_pty;		// 0-31
    char *rds_text;
    char *rds_title;
    char *rds_artist;
    struct tm *title_start;
};


// --- cRadioCheck -------------------------------------------------------

class cRadioCheck: public cThread {
private:
    static cRadioCheck *RadioCheck;
protected:
    virtual void Action(void);
    void Stop(void);
public:
    cRadioCheck(void);
    virtual ~cRadioCheck();
    static void Init(void);
    static void Exit(void);
};

cRadioCheck *cRadioCheck::RadioCheck = NULL;

cRadioCheck::cRadioCheck(void)
: cThread("radiocheck")
{
    IsRadioOrReplay = 0;
}

cRadioCheck::~cRadioCheck() {
    if (Running())
	Stop();
}

void cRadioCheck::Init(void) {
    /* deactivate standard audio background pics */
    cPlugin *p = cPluginManager::GetPlugin("reelbox");
    if(p) p->Service("ToggleAudioPicPlayer", (void*)false);

    if (RadioCheck == NULL) {
	RadioCheck = new cRadioCheck;
	RadioCheck->Start();
    }
}

void cRadioCheck::Exit(void) {
    /* reactivate standard audio background pics */
    cPlugin *p = cPluginManager::GetPlugin("reelbox");
    if(p) p->Service("ToggleAudioPicPlayer", (void*)true);

    if (RadioCheck != NULL) {
	RadioCheck->Stop();
	DELETENULL(RadioCheck);
    }
}

void cRadioCheck::Stop(void) {
    Cancel(10);
}

void cRadioCheck::Action(void)
{
    if (S_Verbose >= 2)
        printf("vdr-radio: background-checking starts\n");

    while (Running()) {
	cCondWait::SleepMs(100);
	
	// check Live-Radio
	if (IsRadioOrReplay == 1 && chan != NULL && chan->Vpid()) {
	    if (S_Verbose >= 2)
    		printf("vdr-radio: channnel '%s' got Vpid = %d\n", chan->Name(), chan->Vpid());
	    IsRadioOrReplay = 0;
	    Channels.SwitchTo(cDevice::CurrentChannel());
	    //cDevice::PrimaryDevice()->SwitchChannel(chan, true);
	    }

	// temp. OSD-CloseTimeout
	//RT_OsdTOTemp = (RT_OsdTOTemp > 0) ? RT_OsdTOTemp -= 2 : 0;	// in sec like this cycletime
	RT_OsdTOTemp = (RT_OsdTOTemp > 0) ? RT_OsdTOTemp - 2 : 0;    // in sec like this cycletime // TB: "-="?! doesn't make sense....

	}

    if (S_Verbose >= 2)
        printf("vdr-radio: background-checking ends\n");
}


// --- cMenuSetupRadio -------------------------------------------------------

class cMenuSetupRadio : public cMenuSetupPage {
private:
    int newS_Activate;
    int newS_HMEntry;
    int newS_RtFunc;
    int newS_RtOsdTitle;
    int newS_RtOsdTags;
    int newS_RtOsdPos;
    int newS_RtOsdRows;
    int newS_RtOsdLoop;
    int newS_RtOsdTO;
    int newS_RtSkinColor;
    int newS_RtBgCol;
    int newS_RtBgTra;
    int newS_RtFgCol;
    int newS_RtDispl;
    int newS_RtMsgItems;
    //int newS_RtpMemNo;
    int newS_QDarText;
    const char *T_RtFunc[3];
    const char *T_RtOsdTags[3];
    const char *T_RtOsdPos[2];
    const char *T_RtOsdLoop[2];
    const char *T_RtBgColor[9];
    const char *T_RtFgColor[9];
    const char *T_RtDisplay[3];
    const char *T_RtMsgItems[4];
    const char *T_QDarText[3];
protected:
    virtual void Store(void);
public:
    cMenuSetupRadio(void);
};

cMenuSetupRadio::cMenuSetupRadio(void)
{
    T_RtFunc[0] = tr("Off");
    T_RtFunc[1] = tr("only Text");
    T_RtFunc[2] = tr("Text+TagInfo");
    T_RtOsdTags[0] = tr("Off");
    T_RtOsdTags[1] = tr("only, if some");
    T_RtOsdTags[2] = tr("always");
    T_RtOsdPos[0] = tr("Top");
    T_RtOsdPos[1] = tr("Bottom");
    T_RtOsdLoop[0] = tr("latest at Top");
    T_RtOsdLoop[1] = tr("latest at Bottom");
    T_RtBgColor[0] = T_RtFgColor[0] = tr ("Black");
    T_RtBgColor[1] = T_RtFgColor[1] = tr ("White");
    T_RtBgColor[2] = T_RtFgColor[2] = tr ("Red");
    T_RtBgColor[3] = T_RtFgColor[3] = tr ("Green");
    T_RtBgColor[4] = T_RtFgColor[4] = tr ("Yellow");
    T_RtBgColor[5] = T_RtFgColor[5] = tr ("Magenta");
    T_RtBgColor[6] = T_RtFgColor[6] = tr ("Blue");
    T_RtBgColor[7] = T_RtFgColor[7] = tr ("Cyan");
    T_RtBgColor[8] = T_RtFgColor[8] = tr ("Transparent");
    T_RtDisplay[0] = tr("Off");
    T_RtDisplay[1] = tr("about MainMenu");
    T_RtDisplay[2] = tr("Automatic");
    T_RtMsgItems[0] = tr("Off");
    T_RtMsgItems[1] = tr("only Taginfo");
    T_RtMsgItems[2] = tr("only Text");
    T_RtMsgItems[3] = tr("Text+TagInfo");
    T_QDarText[0] = tr("Off");
    T_QDarText[1] = tr("RaSS only");
    T_QDarText[2] = tr("RaSS+Text mixed");

    newS_Activate = S_Activate;
    newS_HMEntry = S_HMEntry;
    newS_RtFunc = S_RtFunc;
    newS_RtOsdTitle = S_RtOsdTitle;
    newS_RtOsdTags = S_RtOsdTags;
    newS_RtOsdPos = S_RtOsdPos;
    newS_RtOsdRows = S_RtOsdRows;
    newS_RtOsdLoop = S_RtOsdLoop;
    newS_RtOsdTO = S_RtOsdTO;
    newS_RtSkinColor = S_RtSkinColor;
    newS_RtBgCol = S_RtBgCol;
    newS_RtBgTra = S_RtBgTra;
    newS_RtFgCol = S_RtFgCol;
    newS_RtDispl = (S_RtDispl > 2 ? 2 : S_RtDispl);
    newS_RtMsgItems = S_RtMsgItems;
    //newS_RtpMemNo = S_RtpMemNo;
    newS_QDarText = S_QDarText;

    Add(new cMenuEditBoolItem( tr("Activate"),      	 		&newS_Activate));
//    Add(new cMenuEditBoolItem( tr("Hide MainMenuEntry"),   		&newS_HMEntry));
    Add(new cMenuEditStraItem( tr("RDSText Function"),     		&newS_RtFunc, 3, T_RtFunc));
    Add(new cMenuEditStraItem( tr("RDSText OSD-Position"),  		&newS_RtOsdPos, 2, T_RtOsdPos));
    Add(new cMenuEditBoolItem( tr("RDSText OSD-Titlerow"),		&newS_RtOsdTitle));
    Add(new cMenuEditIntItem(  tr("RDSText OSD-Rows (1-5)"),		&newS_RtOsdRows, 1, 5));
//    Add(new cMenuEditStraItem( tr("RDSText OSD-Scrollmode"),     	&newS_RtOsdLoop, 2, T_RtOsdLoop));
//    Add(new cMenuEditStraItem( tr("RDSText OSD-Taginfo"),     		&newS_RtOsdTags, 3, T_RtOsdTags));
//    Add(new cMenuEditBoolItem( tr("RDSText OSD-Skincolors used"),	&newS_RtSkinColor));
//      if (newS_RtSkinColor == 0) {
//	Add(new cMenuEditStraItem( tr("RDSText OSD-Backgr.Color"), 	&newS_RtBgCol, 9, T_RtBgColor));
//	Add(new cMenuEditIntItem(  tr("RDSText OSD-Backgr.Transp."), 	&newS_RtBgTra, 1, 255));
//	Add(new cMenuEditStraItem( tr("RDSText OSD-Foregr.Color"), 	&newS_RtFgCol, 8, T_RtFgColor));
//	}
#ifndef VDRP_CLOSEMENU
//    Add(new cMenuEditIntItem(  tr("RDSText OSD-Timeout (0-1440 min)"),	&newS_RtOsdTO, 0, 1440));
#endif
//    Add(new cMenuEditStraItem( tr("RDSText OSD-Display"), 	 	&newS_RtDispl, 3, T_RtDisplay));
//    Add(new cMenuEditStraItem( tr("RDSText StatusMsg (lcdproc & co)"),  &newS_RtMsgItems, 4, T_RtMsgItems));
//      Add(new cMenuEditIntItem(  tr("RDSplus Memorynumber (10-99)"),	&newS_RtpMemNo, 10, 99));
//    Add(new cMenuEditStraItem( tr("RDSText RaSS-Function"),    		&newS_QDarText, 3, T_QDarText));
}

void cMenuSetupRadio::Store(void)
{
    SetupStore("Activate", 	  	 S_Activate = newS_Activate);
    SetupStore("HideMenuEntry",        	 S_HMEntry = newS_HMEntry);
    SetupStore("RDSText-Function",     	 S_RtFunc = newS_RtFunc);
    SetupStore("RDSText-OsdTitle",       S_RtOsdTitle = newS_RtOsdTitle);
    SetupStore("RDSText-OsdTags",        S_RtOsdTags = newS_RtOsdTags);
    SetupStore("RDSText-OsdPosition",  	 S_RtOsdPos = newS_RtOsdPos);
    SetupStore("RDSText-OsdRows",      	 S_RtOsdRows = newS_RtOsdRows);
    SetupStore("RDSText-OsdLooping",     S_RtOsdLoop = newS_RtOsdLoop);
    SetupStore("RDSText-OsdSkinColor",	 S_RtSkinColor = newS_RtSkinColor);
    SetupStore("RDSText-OsdBackgrColor", S_RtBgCol = newS_RtBgCol);
    SetupStore("RDSText-OsdBackgrTrans", S_RtBgTra = newS_RtBgTra);
    SetupStore("RDSText-OsdForegrColor", S_RtFgCol = newS_RtFgCol);
#ifndef VDRP_CLOSEMENU
    SetupStore("RDSText-OsdTimeout",  	 S_RtOsdTO = newS_RtOsdTO);
#endif
    SetupStore("RDSText-Display",      	 S_RtDispl = newS_RtDispl);
    SetupStore("RDSText-MsgItems",     	 S_RtMsgItems = newS_RtMsgItems);
    //SetupStore("RDSplus-MemNumber",  	 S_RtpMemNo = newS_RtpMemNo);
    SetupStore("RDSText-RaSS",  	 S_QDarText = newS_QDarText);
}


// --- cPluginRadio -------------------------------------------------------

class cRadioAudio;

class cPluginRadio : public cPlugin, cStatus {
private:
    // Add any member variables or functions you may need here.
    bool ConfigDirParam;
    bool DataDirParam;
    bool LiveFileParam;
    bool ReplayFileParam;
    cRadioAudio *radioAudio;
public:
    cPluginRadio(void);
    virtual ~cPluginRadio();
    virtual const char *Version(void) { return VERSION; }
    virtual const char *Description(void) { return tr(DESCRIPTION); }
    virtual const char *CommandLineHelp(void);
    virtual bool ProcessArgs(int argc, char *argv[]);
    virtual bool Initialize(void);
    virtual bool Start(void);
    virtual void Stop(void);
    virtual void Housekeeping(void);
#if VDRVERSNUM >= 10347
    virtual void MainThreadHook(void) { }
    virtual cString Active(void) { return NULL; }
#endif
    //virtual const char *MainMenuEntry(void) { return (S_Activate==0 || S_RtFunc==0 || S_RtDispl==0 || S_HMEntry ? NULL : tr(MAINMENUENTRY)); }
    virtual const char* MainMenuEntry(void)  { return tr(MAINMENUENTRY); }
    virtual cOsdObject *MainMenuAction(void);
    virtual cMenuSetupPage *SetupMenu(void);
    virtual bool SetupParse(const char *Name, const char *Value);
    virtual bool Service(const char *Id, void *Data);
    virtual const char **SVDRPHelpPages(void);
    virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
protected:
    virtual void ChannelSwitch(const cDevice *Device, int ChannelNumber);
    bool file_exists(const char * filename);
    virtual void Replaying(const cControl *Control, const char *Name, const char *FileName, bool On);
};

cPluginRadio::cPluginRadio(void)
{
    // Initialize any member variables here.
    // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
    // VDR OBJECTS TO EXIST OR PROADUCE ANY OUTPUT!

    radioAudio = 0;

    ConfigDirParam = false;
    DataDirParam = false;
    LiveFileParam = false;
    ReplayFileParam = false;

    ConfigDir =NULL;
    DataDir   =NULL;
    LiveFile  =NULL;
    ReplayFile=NULL;
    
    rt_color[0] = 0xFF000000;	//Black
    rt_color[1] = 0xFFFCFCFC;	//White
    rt_color[2] = 0xFFFC1414;	//Red
    rt_color[3] = 0xFF24FC24;	//Green
    rt_color[4] = 0xFFFCC024;	//Yellow
    rt_color[5] = 0xFFB000FC;	//Magenta
    rt_color[6] = 0xD9353E70;   //Blue
    rt_color[7] = 0xFF00FCFC;	//Cyan
    rt_color[8] = 0x00000000;	//Transparent
}

cPluginRadio::~cPluginRadio()
{
    // Clean up after yourself!
    if (ConfigDir) 	free(ConfigDir);
    if (DataDir)   	free(DataDir);
    if (LiveFile)  	free(LiveFile);
    if (ReplayFile) 	free(ReplayFile);

    cRadioCheck::Exit();
}


const char *cPluginRadio::CommandLineHelp(void)
{
    // Return a string that describes all known command line options.
    return "  -f dir,   --files=dir      use dir as image directory (default: <vdrconfig>/plugins/radio)\n"
	   "  -d dir,   --data=dir       use dir as temp. data directory (default: <vdrconfig>/plugins/radio)\n"
	   "  -l file,  --live=file      use file as default mpegfile in livemode (default: <dir>/radio.mpg)\n"
	   "  -r file,  --replay=file    use file as default mpegfile in replaymode (default: <dir>/replay.mpg)\n"
	   "  -v level, --verbose=level  set verbose level (default = 1, 0 = Off, 1 = RDS-Text+Tags,    \n"
	   "                                                2 = +RDS-Telegram/Debug, 3 = +RawData 0xfd  \n";
}

bool cPluginRadio::ProcessArgs(int argc, char *argv[])
{
    // Implement command line argument processing here if applicable.

    static struct option long_options[] = {
    	    { "files",      required_argument, NULL, 'f' },
    	    { "data",       required_argument, NULL, 'd' },
    	    { "live",       required_argument, NULL, 'l' },
    	    { "replay",     required_argument, NULL, 'r' },
	    { "verbose",    required_argument, NULL, 'v' },
	    { NULL }
	};
    
    int c;
    while ((c = getopt_long(argc, argv, "f:d:l:r:v:", long_options, NULL)) != -1) {
	switch (c) {
		case 'f':
				printf("vdr-radio: arg files-dir = %s\n", optarg);
				ConfigDir = strdup(optarg);
				ConfigDirParam = true;
				break;
		case 'd':
				printf("vdr-radio: arg data-dir = %s\n", optarg);
				DataDir = strdup(optarg);
				DataDirParam = true;
				break;
		case 'l':
				printf("vdr-radio: arg live-mpeg = %s\n", optarg);
				LiveFile = strdup(optarg);
				LiveFileParam = true;
				break;
		case 'r':
				printf("vdr-radio: arg replay-mpeg = %s\n", optarg);
				ReplayFile = strdup(optarg);
				ReplayFileParam = true;
				break;
		case 'v':
				printf("vdr-radio: arg verbose = %s\n", optarg);
				if (isnumber(optarg))
				    S_Verbose = atoi(optarg);
				break;
		default:
				printf("vdr-radio: arg char = %c\n", c);
				return false;
	    }
	}

    return true;
}

bool cPluginRadio::Initialize(void)
{
    // Initialize any background activities the plugin shall perform.
    return true;
}

bool cPluginRadio::Start(void)
{
    // Start any background activities the plugin shall perform.
    RegisterI18n(Phrases);
    printf("vdr-radio: Radio-Plugin Backgr.Image/RDS-Text starts...\n");

    radioAudio = new cRadioAudio;
    if (!radioAudio)
	return false;
 
    if (!ConfigDirParam) 
	ConfigDir = strdup(ConfigDirectory(Name()));
    if (!DataDirParam) 
	DataDir = strdup(ConfigDirectory(Name()));
    if (!LiveFileParam)
	asprintf(&LiveFile, "%s/radio.mpg", ConfigDir);
    if (!ReplayFileParam)
	asprintf(&ReplayFile, "%s/replay.mpg", ConfigDir);

    cRadioCheck::Init();    

    return true;
}

void cPluginRadio::Stop(void)
{
    cRadioCheck::Exit();

    if (IsRadioOrReplay > 0) 
	radioAudio->DisableRadioTextProcessing();
}

void cPluginRadio::Housekeeping(void)
{
    // Perform any cleanup or other regular tasks.
}   

cOsdObject *cPluginRadio::MainMenuAction(void)
{
    // Perform the action when selected from the main VDR menu.
    if (!cDevice::PrimaryDevice()->Transferring() && !cDevice::PrimaryDevice()->Replaying()) {
	//cRemote::CallPlugin("radio"); // try again later <-- diabled, looping if activate over menu @ tv in dvb-livemode
	}
    else if (S_Activate > 0 && S_RtFunc > 0 && S_RtDispl > 0 && IsRadioOrReplay > 0) {
	if (!RTplus_Osd) {
	    cRadioTextOsd *rtosd = new cRadioTextOsd();
	    return rtosd;
	    }
	else {
	    cRTplusOsd *rtposd = new cRTplusOsd();
	    return rtposd;
	    }
	}

    return NULL;
}

cMenuSetupPage *cPluginRadio::SetupMenu(void)
{
    // Return a setup menu in case the plugin supports one.
    return new cMenuSetupRadio;
}

bool cPluginRadio::SetupParse(const char *Name, const char *Value)
{
    // Parse your own setup parameters and store their values.
    if 	    (!strcasecmp(Name, "Activate"))               S_Activate = atoi(Value);
    else if (!strcasecmp(Name, "HideMenuEntry"))          S_HMEntry = atoi(Value);
    else if (!strcasecmp(Name, "RDSText-Function"))       S_RtFunc = atoi(Value);
    else if (!strcasecmp(Name, "RDSText-OsdTitle"))       S_RtOsdTitle = atoi(Value);
    else if (!strcasecmp(Name, "RDSText-OsdTags"))        S_RtOsdTags = atoi(Value);
    else if (!strcasecmp(Name, "RDSText-OsdPosition"))    S_RtOsdPos = atoi(Value);
    else if (!strcasecmp(Name, "RDSText-OsdRows")) {
       S_RtOsdRows = atoi(Value);
       if (S_RtOsdRows > 5)  S_RtOsdRows = 5;
       }
    else if (!strcasecmp(Name, "RDSText-OsdLooping"))  	  S_RtOsdLoop = atoi(Value);
    else if (!strcasecmp(Name, "RDSText-OsdSkinColor"))   S_RtSkinColor = atoi(Value);
    else if (!strcasecmp(Name, "RDSText-OsdBackgrColor")) S_RtBgCol = atoi(Value);
    else if (!strcasecmp(Name, "RDSText-OsdBackgrTrans")) S_RtBgTra = atoi(Value);
    else if (!strcasecmp(Name, "RDSText-OsdForegrColor")) S_RtFgCol = atoi(Value);
    else if (!strcasecmp(Name, "RDSText-OsdTimeout")) {
       S_RtOsdTO = atoi(Value);
       if (S_RtOsdTO > 1440)  S_RtOsdTO = 1440;
       }
    else if (!strcasecmp(Name, "RDSText-Display"))        S_RtDispl = atoi(Value);
    else if (!strcasecmp(Name, "RDSText-MsgItems"))       S_RtMsgItems = atoi(Value);
    //else if (!strcasecmp(Name, "RDSplus-MemNumber"))	  S_RtpMemNo = atoi(Value);
    else if (!strcasecmp(Name, "RDSText-RaSS"))       	  S_QDarText = atoi(Value);
    else
	return false;

    return true;
}

bool cPluginRadio::Service(const char *Id, void *Data)
{
    if (strcmp(Id,"RadioTextService-v1.0") == 0 && S_Activate > 0 && S_RtFunc >= 1) {
	if (Data) {
    	    RadioTextService_v1_0 *data = (RadioTextService_v1_0*)Data;
	    data->rds_pty = RT_PTY;
	    data->rds_info = (RT_Info < 0) ? 0 : RT_Info;
	    int ind = (RT_Index == 0) ? S_RtOsdRows - 1 : RT_Index - 1;
	    data->rds_text = RT_Text[ind];
    	    data->rds_title = RTP_Title;
    	    data->rds_artist = RTP_Artist;
	    struct tm tm_store;
	    data->title_start = localtime_r(&RTP_Starttime, &tm_store);
    	    }
        return true;
	}

    return false;
}

const char **cPluginRadio::SVDRPHelpPages(void)
{
    static const char *HelpPages[] = {
	"RTINFO\n"
	"  Print the radiotext information.",
	"RTCLOSE\n"
	"  Close the radiotext-osd,\n"
	"  Reopen can only be done over menu or channelswitch.",
	"RTTCLOSE\n"
	"  Close the radiotext-osd temporarily,\n"
	"  Reopen will be done after osd-messagetimeout.",
	NULL
	};

    return HelpPages;
}

cString cPluginRadio::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    if (strcasecmp(Command, "RTINFO") == 0) {
	// we use the default reply code here
        if (RT_Info == 2) {
	    int ind = (RT_Index == 0) ? S_RtOsdRows - 1 : RT_Index - 1;
	    return cString::sprintf(" Radiotext: %s\n RT-Title : %s\n RT-Artist: %s\n", RT_Text[ind], RTP_Title, RTP_Artist);
	    }
        else if (RT_Info == 1) {
	    int ind = (RT_Index == 0) ? S_RtOsdRows - 1 : RT_Index - 1;
	    return cString::sprintf(" Radiotext: %s\n", 	RT_Text[ind]);
	    }
        else
    	    return cString::sprintf(" Radiotext not available (yet)\n");
        }
    else if (strcasecmp(Command, "RTCLOSE") == 0) {
        // we use the default reply code here
        if (RT_OsdTO)
    	    return cString::sprintf("RT-OSD already closed");
        else {
    	    RT_OsdTO = true;
    	    return cString::sprintf("RT-OSD will be closed now");
    	    }
        }
    else if (strcasecmp(Command, "RTTCLOSE") == 0) {
        // we use the default reply code here
	RT_OsdTOTemp = 2 * Setup.OSDMessageTime;
    	return cString::sprintf("RT-OSD will be temporarily closed");
        }

    return NULL;
}

void cPluginRadio::ChannelSwitch(const cDevice *Device, int ChannelNumber)
{
    if (Device != cDevice::PrimaryDevice()) return;

    IsRadioOrReplay = 0;
    radioAudio->DisableRadioTextProcessing();

    if (S_Activate == false) return;

    char *image;
    if (cDevice::CurrentChannel() == ChannelNumber) {
	chan = ChannelNumber ? Channels.GetByNumber(ChannelNumber) : NULL;
	if (chan != NULL && chan->Vpid() == 0 && chan->Apid(0) > 0) {
	    asprintf(&image, "%s/%s.mpg", ConfigDir, chan->Name());
	    if (!file_exists(image)) {
	        dsyslog("vdr-radio: channel-image not found '%s' (Channelname= %s)", image, chan->Name());
	        free(image);
		asprintf(&image, "%s", LiveFile);
		if (!file_exists(image)) {
		    dsyslog("vdr-radio: live-image not found '%s' (Channelname= %s)\n", image, chan->Name());
		    }
		}
	    if (S_Verbose >= 1)
	        printf("vdr-radio: [ChannelSwitch # apid=%d] channelname= %s, use image '%s'\n", chan->Apid(0), chan->Name(), image);
#if VDRVERSNUM >= 10345
	    cDevice::PrimaryDevice()->ForceTransferMode();
#else
    #ifdef VDRP_DTRANSFER
	    cDevice::PrimaryDevice()->ActivateTransferMode();
    #else
	    cDevice *d = cDevice::ActualDevice();
	    if (!cTransferControl::ReceiverDevice()) {
	        cDevice::PrimaryDevice()->AttachPlayer(new cPlayer());
	        cControl::Launch(new cTransferControl(d, 0, chan->Apids(), chan->Dpids(), chan->Spids()));
	        }
    #endif
#endif
	    radioAudio->SetBackgroundImage(image);
    	    radioAudio->EnableRadioTextProcessing(chan->Name(), false);
	    free(image);
	    IsRadioOrReplay = 1;
	    }
	}
}

void cPluginRadio::Replaying(const cControl *Control, const char *Name, const char *FileName, bool On)
{
    IsRadioOrReplay = 0;
    radioAudio->DisableRadioTextProcessing();

    if (S_Activate == false) return;

    bool isRadio = false;

    if (On && FileName != NULL) {
	cFileName fn(FileName, false, true);
	cUnbufferedFile *f = fn.Open();
	if (f) {
	    uchar b[4] = { 0x00, 0x00, 0x00, 0x00 };
	    ReadFrame(f, b, sizeof (b), sizeof (b));
            fn.Close();
	    isRadio = (b[0] == 0x00) && (b[1] == 0x00) && (b[2] == 0x01) && (0xc0 <= b[3] && b[3] <= 0xdf);
	    }
	}

    if (isRadio) {
	if (!file_exists(ReplayFile))
	    dsyslog("vdr-radio: replay-image not found '%s'", ReplayFile);
	else
	    radioAudio->SetBackgroundImage(ReplayFile);
	radioAudio->EnableRadioTextProcessing(Name, true);
	IsRadioOrReplay = 2;
	}
}

bool cPluginRadio::file_exists(const char * filename)
{
    struct stat file_stat;

    return ((stat(filename, &file_stat) == 0) ? true : false);
}


VDRPLUGINCREATOR(cPluginRadio); // Don't touch this!
