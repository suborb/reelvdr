
#ifndef MHWEPG_H
#define MHWEPG_H

#include "filter.h"


//-- Macros ---------------------------------------------------------------

#define HILO16( x ) ( ( ( x##High << 8 ) | x##Low ) & 0xffff )
#define HILO32( x ) ( ( ( ( ( x##High << 24 ) | ( x##MediumHigh << 16 ) ) | ( x##MediumLow << 8 ) ) | x##Low ) & 0xffffffff )
#define MjdToEpochTime( x ) ( ( ( x##High << 8 | x##Low ) - 40587 ) * 86400 )
#define BcdTimeToSeconds( x ) ( ( 3600 * ( ( 10 * ( ( x##Hours & 0xf0 ) >> 4 ) ) + ( x##Hours & 0x0f ) ) ) + ( 60 * ( ( 10 * ( ( x##Minutes & 0xf0 ) >> 4 ) ) + ( x##Minutes & 0x0f ) ) ) + ( ( 10 * ( ( x##Seconds & 0xf0 ) >> 4 ) ) + ( x##Seconds & 0x0f ) ) )
#define BcdTimeToMinutes( x ) ( ( 60 * ( ( 10 * ( ( x##Hours & 0xf0 ) >> 4 ) ) + ( x##Hours & 0x0f ) ) ) + ( ( ( 10 * ( ( x##Minutes & 0xf0 ) >> 4 ) ) + ( x##Minutes & 0x0f ) ) ) )


#define STATUS_START_LOAD 3
#define STATUS_EXEC_LOAD 4
#define STATUS_STOP_LOAD 5
#define STATUS_END 7

#define EPG_PROVIDERS 8

typedef struct DescriptorType
{
    u_char Tag                      :8;
    u_char Length                   :8;
} DescriptorType;


//-- Time -----------------------------------------------------------------

#define EPG_TIME_LENGTH 10
typedef struct EpgTimeType
{
   u_char TableId                   :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char SectionSyntax_indicator   :1;
   u_char                           :3;
   u_char SectionLengthHigh         :4;
#else
   u_char SectionLengthHigh         :4;
   u_char                           :3;
   u_char SectionSyntaxIndicator    :1;
#endif
   u_char SectionLengthLow          :8;
   u_char UtcMjdHigh                :8;
   u_char UtcMjdLow                 :8;
   u_char UtcTimeHours              :8;
   u_char UtcTimeMinutes            :8;
   u_char UtcTimeSeconds            :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                           :4;
   u_char TimeOffsetInfoLengthHigh  :4;
#else
   u_char TimeOffsetInfoLengthHigh  :4;
   u_char                           :4;
#endif
   u_char TimeOffsetInfoLengthLow   :8;
} EpgTimeType;

#define LOCAL_TIME_OFFSET_LENGTH 13
typedef struct LocalTimeOffsetType
{
    u_char CountryCode1             :8;
    u_char CountryCode2             :8;
    u_char CountryCode3             :8;
#if BYTE_ORDER == BIG_ENDIAN
    u_char RegionId                 :6;
    u_char                          :1;
    u_char LocalTimeOffsetPolarity  :1;
#else
    u_char LocalTimeOffsetPolarity  :1;
    u_char                          :1;
    u_char RegionId                 :6;
#endif
    u_char LocalTimeOffsetHours     :8;
    u_char LocalTimeOffsetMinutes   :8;
    u_char TocMjdHigh               :8;
    u_char TocMjdLow                :8;
    u_char TocTimeHours             :8;
    u_char TocTimeMinutes           :8;
    u_char TocTimeSeconds           :8;
    u_char NextTimeOffsetHours      :8;
    u_char NextTimeOffsetMinutes    :8;
} LocalTimeOffsetType;

//-------------------------------------------------------------------------

//-- Channels -------------------------------------------------------------

#define EPG_CHANNELS 256
#define EPG_CHANNEL_LENGTH 22
typedef struct EpgChannelType
{
    u_char NetworkIdHigh            :8;
    u_char NetworkIdLow             :8;
    u_char TransponderIdHigh        :8;
    u_char TransponderIdLow         :8;
    u_char ChannelIdHigh            :8;
    u_char ChannelIdLow             :8;
    u_char Name                   [16];
} EpgChannelType;

typedef struct EpgChannelType2
{
    u_char NetworkIdHigh            :8;
    u_char NetworkIdLow             :8;
    u_char TransponderIdHigh        :8;
    u_char TransponderIdLow         :8;
    u_char ChannelIdHigh            :8;
    u_char ChannelIdLow             :8;
    u_char                          :8;
    u_char                          :8;
} EpgChannelType2;

//-------------------------------------------------------------------------

//-- Themes ---------------------------------------------------------------

#define EPG_THEMES 256

//-------------------------------------------------------------------------

//-- Programs -------------------------------------------------------------

#define EPG_PROGRAM_LENGTH 46
typedef struct EpgEventType
{
    u_char TableId                  :8;
#if BYTE_ORDER == BIG_ENDIAN
    u_char SectionSyntaxIndicator   :1;
    u_char                          :1;
    u_char                          :2;
    u_char SectionLengthHigh        :4;
#else
    u_char SectionLengthHigh        :4;
    u_char                          :2;
    u_char                          :1;
    u_char SectionSyntaxIndicator   :1;
#endif
    u_char SectionLengthLow         :8;
    u_char ChannelId                :8;
    u_char ThemeId                  :8;
#if BYTE_ORDER == BIG_ENDIAN
    u_char Day                      :3;
    u_char Hours                    :5;
#else
    u_char Hours                    :5;
    u_char Day                      :3;
#endif
#if BYTE_ORDER == BIG_ENDIAN
   u_char Minutes                   :6;
   u_char                           :1;
   u_char SummaryAvailable          :1;
#else
   u_char SummaryAvailable          :1;
   u_char                           :1;
   u_char Minutes                   :6;
#endif
   u_char                           :8;
   u_char                           :8;
   u_char DurationHigh              :8;
   u_char DurationLow               :8;
   u_char Title                   [23];
   u_char PpvIdHigh                 :8;
   u_char PpvIdMediumHigh           :8;
   u_char PpvIdMediumLow            :8;
   u_char PpvIdLow                  :8;
   u_char ProgramIdHigh             :8;
   u_char ProgramIdMediumHigh       :8;
   u_char ProgramIdMediumLow        :8;
   u_char ProgramIdLow              :8;
   u_char                           :8;
   u_char                           :8;
   u_char                           :8;
   u_char                           :8;
} EpgEventType;

struct EpgEvent
{
    unsigned char ChannelId;
    unsigned char ThemeId;
    time_t Time;
    unsigned char SummaryAvailable;
    unsigned int Duration;
    unsigned char Title[65];
    unsigned int PpvId;
    unsigned int ProgramId;
};

//-------------------------------------------------------------------------

//-- Descriptions ------------------------------------------------------------

#define EPG_SUMMARY_LENGTH 11
#define EPG_SUMMARY_REPLAY_LENGTH 7
#define EPG_SUMMARY_NUM_REPLAYS 16

typedef struct EpgDescriptionType {
   u_char TableId                   :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char SectionSyntaxIndicator    :1;
   u_char                           :1;
   u_char                           :2;
   u_char SectionLengthHigh         :4;
#else
   u_char SectionLengthHigh         :4;
   u_char                           :2;
   u_char                           :1;
   u_char SectionSyntaxIndicator    :1;
#endif
   u_char SectionLengthLow          :8;
   u_char ProgramIdHigh             :8;
   u_char ProgramIdMediumHigh       :8;
   u_char ProgramIdMediumLow        :8;
   u_char ProgramIdLow              :8;
   u_char Byte7                     :8;
   u_char Byte8                     :8;
   u_char Byte9                     :8;
   u_char NumReplays                :8;
} EpgDescriptionType;

typedef struct EpgDescriptionReplayType {
   u_char ChannelId                 :8;
   u_char ReplayMjdHigh             :8;
   u_char ReplayMjdLow              :8;
   u_char ReplayTimeHours           :8;
   u_char ReplayTimeMinutes         :8;
   u_char ReplayTimeSeconds         :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char LastReplay                :1;
   u_char                           :1;
   u_char Vo                        :1;
   u_char Vm                        :1;
   u_char                           :3;
   u_char SubtitlesAvailable        :1;
#else
   u_char SubtitlesAvailable        :1;
   u_char                           :3;
   u_char Vm                        :1;
   u_char Vo                        :1;
   u_char                           :1;
   u_char LastReplay                :1;
#endif
} EpgDescriptionReplayType;

struct EpgDescription
{
    unsigned int ProgramId;
    //unsigned char NumReplays;
    unsigned char *Text;
    //time_t Time[EPG_SUMMARY_NUM_REPLAYS];
    //unsigned char ChannelId[EPG_SUMMARY_NUM_REPLAYS];
    //unsigned char SubtitlesAvailable[EPG_SUMMARY_NUM_REPLAYS];
    //unsigned char LastReplay[EPG_SUMMARY_NUM_REPLAYS];
};

//-------------------------------------------------------------------------

//-- cMhwepgFilter -------------------------------------------------------

class cMhwepgFilter : public cFilter
{
    private:
        unsigned int TextLength;
        unsigned int TextOffset;
        struct EpgEventType *EpgEventData;
        struct EpgDescriptionType *EpgDescriptionData;
        unsigned int SectionInitial;
        unsigned char Data2[4096];

        int VdrEquivChannelId;
        int EpgSourceId;
        int NumChannels;

        time_t YesterdayEpoch;
        time_t CurrentTime;
        time_t TimeOfChange;
        int Yesterday;
        int CurrentOffset;
        int NextOffset;

        int NumEpgChannels;
        int NumEpgEquivChannels;
        char EpgChannels[EPG_CHANNELS][256];

        int NumEpgThemes;
        unsigned char EpgThemes[EPG_THEMES][64];
        unsigned char EpgFirstThemes[EPG_THEMES];

        unsigned int ListEpgEvents[0xff];
        struct EpgEvent EpgEvent_;

        struct EpgDescription EpgDescription_;

        void CleanString( unsigned char *String );
        time_t ProviderLocalTime2UTC( time_t T );

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
    protected:
        virtual void Process( u_short Pid, u_char Tid, const u_char *Data, int Length );
    public:
        cMhwepgFilter( void );
        ~cMhwepgFilter( void );

        void SetStatus(bool On);
};

#endif

