#include <linux/dvb/frontend.h>
#include <vdr/plugin.h>
#include <vdr/device.h>
#include <vdr/tools.h>
#include <vdr/osd.h>
#include <vdr/filter.h>
#include <vdr/channels.h>
#include <vdr/remote.h>
#include <vdr/thread.h>
#include <vdr/svdrp.h>
#include <vdr/interface.h>
#include <getopt.h>
#include "i18n.h"

#ifndef APIVERSNUM
#include <vdr/config.h>
#endif

#define STATUS_NULL 0
#define STATUS_ACTIVE 1
#define STATUS_INIT 2
#define STATUS_START_LOAD 3
#define STATUS_EXEC_LOAD 4
#define STATUS_STOP_LOAD 5
#define STATUS_LOADEPG 6
#define STATUS_END 7
#define STATUS_TIMEOUT 8

#define TIMEOUT_DEFAULT 30  //seconds
#define TIMEOUT_SCRIPT 120  //seconds

#define EPG_PROVIDERS 8

#define PROVIDER_TYPE_SATELLITE 0
#define PROVIDER_TYPE_FILE 1
#define PROVIDER_TYPE_SCRIPT 2

#define DVBFE_FEC_AUTO 999
#define DVBFE_MOD_AUTO 999
#define DVBFE_DELSYS_DVBS 0
#define DVBFE_ROLLOFF_UNKNOWN 0

struct Status
{
    int Plugin;
    int EpgTime;
    int EpgChannels;
    int EpgThemes;
    int EpgEvents;
    int EpgDescriptions;
    int Svdrp;
    int ProviderType[EPG_PROVIDERS];
};

static char *ConfigPlugin;
static char *Filename;
static char *EpgSourceName;
static struct Status Status;
static int MenuItem;
static int VdrEquivChannelId;
static int EpgSourceId;
static int NumChannels;


static int EpgProviders;
static char *EpgProviderTitle[EPG_PROVIDERS];
static char *EpgProviderValue1[EPG_PROVIDERS];
static char *EpgProviderValue2[EPG_PROVIDERS];

//-- Time -----------------------------------------------------------------

#define EPG_TIME_LENGTH 10

static time_t YesterdayEpoch;
static time_t CurrentTime;
static time_t TimeOfChange;
static int Yesterday;
static int CurrentOffset;
static int NextOffset;

//-------------------------------------------------------------------------

//-- Channels -------------------------------------------------------------

#define EPG_CHANNELS 256
#define EPG_CHANNEL_LENGTH 22

static int NumEpgChannels;
static int NumEpgEquivChannels;
static char EpgChannels[EPG_CHANNELS][256];
static char EpgEquivChannels[EPG_CHANNELS][2][256];

//-------------------------------------------------------------------------

//-- Themes ---------------------------------------------------------------

#define EPG_THEMES 256
static int NumEpgThemes;
static unsigned char EpgThemes[EPG_THEMES][64];
static unsigned char EpgFirstThemes[EPG_THEMES];

//-------------------------------------------------------------------------

//-- Programs -------------------------------------------------------------

#define EPG_PROGRAM_LENGTH 46

#define EPG_PROGRAMS 32000
static unsigned int NumEpgEvents;
static unsigned int ListEpgEvents[0xff];
static unsigned char EpgEventInitial[EPG_PROGRAM_LENGTH];
static struct EpgEvent EpgEvents[EPG_PROGRAMS];

//-------------------------------------------------------------------------

//-- Summaries ------------------------------------------------------------

#define EPG_SUMMARY_LENGTH 11
#define EPG_SUMMARY_REPLAY_LENGTH 7
#define EPG_SUMMARY_NUM_REPLAYS 16

#define EPG_SUMMARIES 32000
static unsigned int NumEpgDescriptions;
static unsigned char EpgDescriptionInitial[EPG_SUMMARY_LENGTH];
static struct EpgDescription EpgDescriptions[EPG_SUMMARIES];

//-------------------------------------------------------------------------

//-- cLoadepgFilter -------------------------------------------------------

class cLoadepgFilter : public cFilter
{
    private:
        unsigned int TextLength;
	unsigned int TextOffset;
        struct EpgEventType *EpgEventData;
	struct EpgDescriptionType *EpgDescriptionData;
	unsigned int SectionInitial;
	unsigned char Data2[4096];
    protected:
	virtual void Process( u_short Pid, u_char Tid, const u_char *Data, int Length );
    public:
	cLoadepgFilter( void );
	void CleanString( unsigned char *String );
	void GetEpgTime( const unsigned char *Data, int Length );
	void GetEpgChannels( const unsigned char *Data, int Length );
	void GetEpgChannels2( const unsigned char *Data, int Length );
	void GetEpgThemes( const unsigned char *Data, int Length );
	void GetEpgThemes2( const unsigned char *Data, int Length );
	void SortEpgEvents( struct EpgEvent *Data );
	void GetEpgEvents( const unsigned char *Data, int Length );
	void GetEpgEvents2( const unsigned char *Data, int Length );
	void SortEpgDescriptions( struct EpgDescription *Data );
	void GetEpgDescriptions( const unsigned char *Data, int Length );
	void GetEpgDescriptions2( const unsigned char *Data, int Length );
};

//-------------------------------------------------------------------------

//-- cLoadepgOsd ----------------------------------------------------------

#define PROGRESS_BAR_HEIGHT 30

static const tColor clrBgTextTop = clrBlue;
static const tColor clrFgTextTop = clrWhite;
static const tColor clrBgTextScroll = clrGray50;
static const tColor clrFgTextScroll = clrWhite;
static const tColor clrBgTextBottom = clrYellow;
static const tColor clrFgTextBottom = clrBlack;
static const tColor clrBgMenuItem = clrCyan;
static const tColor clrFgMenuItem = clrBlack;
static const tColor clrBgProgressBar1 = clrCyan;
static const tColor clrBgProgressBar2 = clrBlack;
static const tColor clrFgProgressBar1 = clrWhite;
static const tColor clrFgProgressBar2 = clrRed;

class cLoadepgOsd : public cThread, public cOsdObject
{
    private:
	cOsd *Osd;
	cLoadepgFilter *Filter;
	cChannel *OldChannel;
	cChannel *EpgChannel;
	cDevice *EpgDevice;
	bool UsingPrimaryDevice;
	int Margin;
	int StatusKey;
	int Padding;
	int Left;
	int Top;
	int LineHeight;
	int LineWidth;
	int TextTopX1;
	int TextTopX2;
	int TextTopY1;
	int TextTopY2;
	int TextScrollX1;
	int TextScrollX2;
	int TextScrollY1;
	int TextScrollY2;
	int ProgressBarX1;
	int ProgressBarX2;
	int ProgressBarY1;
	int ProgressBarY2;
	int TextBottomX1;
	int TextBottomX2;
	int TextBottomY1;
	int TextBottomY2;
	int Timeout;
    protected:
        virtual void Action( void );
    public:
	cLoadepgOsd( void );
	~cLoadepgOsd();
	virtual void Show( void );
	virtual eOSState ProcessKey( eKeys Key );
	void Actions( void );
	void DrawOsd( void );
	void SaveEpg( void );
	bool SaveOldChannel( void );
	void RestoreOldChannel( void );
	bool SwitchToEpgChannel( void );
};

//-------------------------------------------------------------------------

//-- cLoadepgSetup --------------------------------------------------------

struct LoadepgConfig
{
    int UseFileEquivalences;
    int DeviceNumber;
    int OldUpdateChannels;
} LoadepgConfig;

class cLoadepgSetup : public cMenuSetupPage
{
    private:
    protected:
	virtual void Store( void );
    public:
	cLoadepgSetup( void );
};

//-------------------------------------------------------------------------

//-- Macros ---------------------------------------------------------------

#define HILO16( x ) ( ( ( x##High << 8 ) | x##Low ) & 0xffff )
#define HILO32( x ) ( ( ( ( ( x##High << 24 ) | ( x##MediumHigh << 16 ) ) | ( x##MediumLow << 8 ) ) | x##Low ) & 0xffffffff )
#define MjdToEpochTime( x ) ( ( ( x##High << 8 | x##Low ) - 40587 ) * 86400 )
#define BcdTimeToSeconds( x ) ( ( 3600 * ( ( 10 * ( ( x##Hours & 0xf0 ) >> 4 ) ) + ( x##Hours & 0x0f ) ) ) + ( 60 * ( ( 10 * ( ( x##Minutes & 0xf0 ) >> 4 ) ) + ( x##Minutes & 0x0f ) ) ) + ( ( 10 * ( ( x##Seconds & 0xf0 ) >> 4 ) ) + ( x##Seconds & 0x0f ) ) )
#define BcdTimeToMinutes( x ) ( ( 60 * ( ( 10 * ( ( x##Hours & 0xf0 ) >> 4 ) ) + ( x##Hours & 0x0f ) ) ) + ( ( ( 10 * ( ( x##Minutes & 0xf0 ) >> 4 ) ) + ( x##Minutes & 0x0f ) ) ) )

//-------------------------------------------------------------------------
