/*
 * loadepg.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include "loadepg.h"

static const char *VERSION        = "0.1.12";
#if APIVERSNUM >= 10507
static const char *DESCRIPTION    = trNOOP( "Load EPG Data sent to Mediahighway receivers" );
static const char *MAINMENUENTRY  = trNOOP( "LoadEPG" );
#else
static const char *DESCRIPTION    = "Load EPG Data sent to Mediahighway receivers";
static const char *MAINMENUENTRY  = "LoadEPG";
#endif

//-- class cPluginLoadepg -------------------------------------------------

class cPluginLoadepg : public cPlugin {
private:
  // Add any member variables or functions you may need here.

public:
  cPluginLoadepg(void);
  virtual ~cPluginLoadepg();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr( DESCRIPTION ); }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual const char *MainMenuEntry(void) { return tr( MAINMENUENTRY ); }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  virtual bool HasSetupOptions(void) { return false; } // no setup entry
  };


//-- cLoadepgFilter -------------------------------------------------------

cLoadepgFilter::cLoadepgFilter( void )
{
    Set( 0x14, 0x73 );
}

void cLoadepgFilter::Process( u_short Pid, u_char Tid, const u_char *Data, int Length )
{
    if( Status.Plugin == STATUS_INIT )
    {
        fprintf( stderr, "LoadEPG: scan provider: %s\n", EpgProviderTitle[MenuItem] );
        fprintf( stderr, "LoadEPG: found %i equivalences channels\n", NumEpgEquivChannels );
        Status.Plugin = STATUS_EXEC_LOAD;
        Status.EpgTime = STATUS_START_LOAD;
        Status.EpgChannels = STATUS_NULL;
        Status.EpgThemes = STATUS_NULL;
        Status.EpgEvents = STATUS_NULL;
        Status.EpgDescriptions = STATUS_NULL;
        VdrEquivChannelId = -1;
        NumEpgChannels = 0;
    }
    else if( Status.Plugin != STATUS_NULL && Status.Plugin != STATUS_ACTIVE )
    {
        // Get time
        if( Pid == 0x14 && Tid == 0x73 )
            GetEpgTime( Data, Length );
        // Get channels
        else if( Pid == 0xd3 && Tid == 0x91 ) {
            GetEpgChannels( Data, Length );
            Del( 0x231, 0xc8 );
        } else if( Pid == 0xd3 && Tid == 0x92 ) // Get themes 
            GetEpgThemes( Data, Length );
        // Get programs
        else if( Pid == 0xd2 && Tid == 0x90 )
            GetEpgEvents( Data, Length );
        //Get summaries
        else if( Pid == 0xd3 && Tid == 0x90 )
            GetEpgDescriptions( Data, Length );
        //Get channels && themes - mhwepg2
        else if( Pid == 0x231 && Tid == 0xc8 ) {
            if( Data[3] == 0 )
                GetEpgChannels2( Data, Length );
            else if( Data[3] == 1 )
                GetEpgThemes2( Data, Length );
            Del( 0xd3, 0x91 );
        } else if( Pid == 0x234 && Tid == 0xe6 ) //Get programs - mhwepg2 
            GetEpgEvents2( Data, Length );
        //Get summaries - mhwepg2
        else if( Pid == 0x236 && Tid == 0x96 )
            GetEpgDescriptions2( Data, Length );
    
        if( Status.EpgTime == STATUS_STOP_LOAD && Status.EpgChannels == STATUS_STOP_LOAD && Status.EpgThemes == STATUS_STOP_LOAD && Status.EpgEvents == STATUS_STOP_LOAD && Status.EpgDescriptions == STATUS_STOP_LOAD )
            Status.Plugin = STATUS_LOADEPG;
    }
}

time_t ProviderLocalTime2UTC( time_t T )
{
    if( T < TimeOfChange )
        return T - CurrentOffset;
    else
        return T - NextOffset;
}

time_t LocalTime2Utc( time_t Time )
{
    struct tm *TmTime;
    TmTime = gmtime( &Time );
    TmTime->tm_isdst = -1;
    return mktime( TmTime );
}

time_t UTC2LocalTime( time_t Time )
{
    return ( 2 * Time ) - LocalTime2Utc( Time );
}

unsigned char BcdToDec( unsigned char Data )
{
    return ( ( Data >> 4 ) & 0x0f ) * 10 + ( Data & 0x0f );
}

int TimeZoneCmp( LocalTimeOffsetType *ItemLocalTimeOffset, const char *CountryCode, int RegionId )
{
    return ItemLocalTimeOffset->CountryCode1 == CountryCode[0] &&
           ItemLocalTimeOffset->CountryCode2 == CountryCode[1] &&
       ItemLocalTimeOffset->CountryCode3 == CountryCode[2] &&
       ItemLocalTimeOffset->RegionId == RegionId ? 0 : 1;
}

void cLoadepgFilter::GetEpgTime( const unsigned char *Data, int Length )
{
    if( Status.EpgTime == STATUS_START_LOAD )
    {
      int i;
      time_t DiffTime, UtcOffset, LocalTimeOffset, NextTimeOffset, YesterdayTime;
      struct tm *DayTime;
      unsigned char *Descriptors;
      EpgTimeType *Tot = ( EpgTimeType * ) Data;
      CurrentTime = time( NULL );
      DiffTime = MjdToEpochTime( Tot->UtcMjd ) + BcdTimeToSeconds( Tot->UtcTime ) - CurrentTime;
      UtcOffset = ( ( DiffTime + 1800 ) / 3600 ) * 3600;
      Descriptors = ( unsigned char * ) ( Tot + 1 );
      Length -= ( EPG_TIME_LENGTH + 4 );
      while( Length > 0 )
      {
        DescriptorType *Descriptor = ( DescriptorType * ) Descriptors;
        if( Descriptor->Tag == 0x58 )
        {
            LocalTimeOffsetType *ItemLocalTimeOffset = ( LocalTimeOffsetType * ) ( Descriptor + 1 );
            for( i = 0; i < ( Descriptor->Length / LOCAL_TIME_OFFSET_LENGTH ); i ++ )
            {
                if( ! TimeZoneCmp( ItemLocalTimeOffset, "FRA", 0 ) ||
                    ! TimeZoneCmp( ItemLocalTimeOffset, "POL", 0 ) ||
                    ! TimeZoneCmp( ItemLocalTimeOffset, "NLD", 0 ) ||
                    ! TimeZoneCmp( ItemLocalTimeOffset, "NED", 0 ) ||
                    ! TimeZoneCmp( ItemLocalTimeOffset, "GER", 0 ) ||
                    ! TimeZoneCmp( ItemLocalTimeOffset, "DEU", 0 ) ||
                    ! TimeZoneCmp( ItemLocalTimeOffset, "ITA", 0 ) ||
                    ! TimeZoneCmp( ItemLocalTimeOffset, "DNK", 0 ) ||
                    ! TimeZoneCmp( ItemLocalTimeOffset, "SWE", 0 ) ||
                    ! TimeZoneCmp( ItemLocalTimeOffset, "FIN", 0 ) ||
                    ! TimeZoneCmp( ItemLocalTimeOffset, "NOR", 0 ) ||
                    ! TimeZoneCmp( ItemLocalTimeOffset, "ESP", 1 ) )
                {
                        LocalTimeOffset = 60 * BcdTimeToMinutes( ItemLocalTimeOffset->LocalTimeOffset );
                        NextTimeOffset = 60 * BcdTimeToMinutes( ItemLocalTimeOffset->NextTimeOffset );
                        if( ItemLocalTimeOffset->LocalTimeOffsetPolarity )
                        {
                            LocalTimeOffset = -LocalTimeOffset;
                            NextTimeOffset = -NextTimeOffset;
                        }
                        TimeOfChange = MjdToEpochTime( ItemLocalTimeOffset->TocMjd ) + BcdTimeToSeconds( ItemLocalTimeOffset->TocTime );
                        CurrentOffset = LocalTimeOffset + UtcOffset;
                        NextOffset = NextTimeOffset + UtcOffset;
                        goto GetTimeOffset;
                }
                ItemLocalTimeOffset ++;
            }
        }
        Descriptors += Descriptor->Length + 2;
        Length -= Descriptor->Length + 2;
      }
      return;

      GetTimeOffset:;
      YesterdayTime = UTC2LocalTime( time( NULL ) - 86400 );
      DayTime = gmtime( &YesterdayTime );
      Yesterday = DayTime->tm_wday;
      DayTime->tm_hour = 0;
      DayTime->tm_min = 0;
      DayTime->tm_sec = 0;
      DayTime->tm_isdst = -1;
      YesterdayEpoch = UTC2LocalTime( mktime( DayTime ) );
      Status.EpgTime = STATUS_STOP_LOAD;
      Status.EpgChannels = STATUS_START_LOAD;
      Del( 0x14, 0x73 );
      Set( 0xd3, 0x91 );
      Set( 0x231, 0xc8 );
    }
}

void cLoadepgFilter::GetEpgChannels( const unsigned char *Data, int Length )
{
    if( Status.EpgChannels == STATUS_START_LOAD )
    {
        int Id, Sid, Nid, Tid, Rid;
        char *Dummy;
        char FileName[255];
        FILE *File;
        EpgChannelType *EpgChannelData = ( EpgChannelType * ) ( Data + 4 );
        NumEpgChannels = ( Length - 4 ) / sizeof( EpgChannelType );
        bzero( EpgChannels, sizeof( EpgChannels ) );
        if( NumEpgChannels > EPG_CHANNELS )
        {
            fprintf( stderr, "LoadEPG: error stream data, epg channels > %i\n", EPG_CHANNELS );
            Status.Plugin = STATUS_END;
            Status.EpgTime = STATUS_STOP_LOAD;
            Status.EpgChannels = STATUS_STOP_LOAD;
            Status.EpgThemes = STATUS_STOP_LOAD;
            Status.EpgEvents = STATUS_STOP_LOAD;
            Status.EpgDescriptions = STATUS_STOP_LOAD;
            Del( 0xd3, 0x91 );
            return;
        }
        memset( FileName, 0, sizeof( FileName ) );
        snprintf( FileName, sizeof( FileName ), "/tmp/%s.channels", EpgProviderTitle[MenuItem] );
        File = fopen( FileName, "w" );
        if( sscanf( EpgProviderValue1[MenuItem], "%a[^:]:%a[^:]:%a[^:]:%a[^:]:%a[^:]", &Dummy, &Dummy, &Dummy, &EpgSourceName, &Dummy ) == 5 )
            EpgSourceId = cSource::FromString( EpgSourceName );
        for( Id = 0; Id < NumEpgChannels; Id ++ )
        {
            Sid = ( ( EpgChannelData->ChannelIdHigh << 8 ) | EpgChannelData->ChannelIdLow ) & 0xffff;
            Tid = ( ( EpgChannelData->TransponderIdHigh << 8 ) | EpgChannelData->TransponderIdLow ) & 0xffff;
            Nid = ( ( EpgChannelData->NetworkIdHigh << 8 ) | EpgChannelData->NetworkIdLow ) & 0xffff;
            Rid = 0;
            tChannelID EpgChannelId = tChannelID( EpgSourceId, Nid, Tid, Sid, Rid );
            cChannel *EpgChannel = Channels.GetByChannelID( EpgChannelId, true );
            if( EpgChannel )
            {
                snprintf( EpgChannels[Id], sizeof( EpgChannels[Id] ), "%s-%i-%i-%i-%i %s", EpgSourceName, EpgChannel->Nid(), EpgChannel->Tid(), EpgChannel->Sid(), EpgChannel->Rid(), EpgChannel->Name() );
                if( File != NULL )
                    fprintf( File, "%s\n", EpgChannels[Id] );
            }
        EpgChannelData ++;
    }
    if( File != NULL )
    {
        fclose( File );
        fprintf( stderr, "LoadEPG: epg channels list saved in %s\n", FileName );
    }
    fprintf( stderr, "LoadEPG: found %i channels\n", NumEpgChannels );
    NumChannels = NumEpgChannels + NumEpgEquivChannels;
    Status.EpgChannels = STATUS_STOP_LOAD;
    Status.EpgThemes = STATUS_START_LOAD;
    Del( 0xd3, 0x91 );
    Set( 0xd3, 0x92 );
    }
}

void cLoadepgFilter::GetEpgChannels2( const unsigned char *Data, int Length )
{
    if( Status.EpgChannels == STATUS_START_LOAD )
    {
        int Id, Sid, Tid, Nid, Rid, ChannelId;
        char *Dummy;
        char FileName[255];
        FILE *File;
        if( Length > 119 )
        {
            NumEpgChannels = Data[119];
            Id = 120 + ( 8 * NumEpgChannels );
            if( Length > Id )
            {
                for( ChannelId = 0; ChannelId < NumEpgChannels; ChannelId ++ )
                {
                    Id += ( Data[Id] & 0x0f ) + 1;
                    if( Length < Id )
                        return;
                }
            }
            else
            {
                return;
            }
        }
        else
        {
            return;
        }
        NumEpgChannels = Data[119];
        bzero( EpgChannels, sizeof( EpgChannels ) );
        if( NumEpgChannels > EPG_CHANNELS )
        {
            fprintf( stderr, "LoadEPG: error stream data, epg channels > %i\n", EPG_CHANNELS );
            Status.Plugin = STATUS_END;
            Status.EpgTime = STATUS_STOP_LOAD;
            Status.EpgChannels = STATUS_STOP_LOAD;
            Status.EpgThemes = STATUS_STOP_LOAD;
            Status.EpgEvents = STATUS_STOP_LOAD;
            Status.EpgDescriptions = STATUS_STOP_LOAD;
            Del( 0x231, 0xc8 );
            return;
        }
        memset( FileName, 0, sizeof( FileName ) );
        snprintf( FileName, sizeof( FileName ), "/tmp/%s.channels", EpgProviderTitle[MenuItem] );
        File = fopen( FileName, "w" );
        if( sscanf( EpgProviderValue1[MenuItem], "%a[^:]:%a[^:]:%a[^:]:%a[^:]:%a[^:]", &Dummy, &Dummy, &Dummy, &EpgSourceName, &Dummy ) == 5 )
            EpgSourceId = cSource::FromString( EpgSourceName );
        ChannelId = 1;
        while( ChannelId <= Channels.MaxNumber() )
        {
            cChannel *EpgChannel = Channels.GetByNumber( ChannelId, 1 );
            if( EpgChannel )
            {
                EpgChannelType2 *EpgChannelData = ( EpgChannelType2 * ) ( Data + 120 );
                for( Id = 0; Id < NumEpgChannels; Id ++ )
                {
                    Sid = ( ( EpgChannelData->ChannelIdHigh << 8 ) | EpgChannelData->ChannelIdLow ) & 0xffff;
                    Tid = ( ( EpgChannelData->TransponderIdHigh << 8 ) | EpgChannelData->TransponderIdLow ) & 0xffff;
                    Nid = ( ( EpgChannelData->NetworkIdHigh << 8 ) | EpgChannelData->NetworkIdLow ) & 0xffff;
                    Rid = 0;
                    if( EpgChannel->Sid() == Sid && EpgChannel->Tid() == Tid && Nid == EpgChannel->Nid() && EpgChannel->Rid() == Rid )
                    {
                        snprintf( EpgChannels[Id], sizeof( EpgChannels[Id] ), "%s-%i-%i-%i-%i %s", EpgSourceName, EpgChannel->Nid(), EpgChannel->Tid(), EpgChannel->Sid(), EpgChannel->Rid(), EpgChannel->Name() );
                        if( File != NULL )
                            fprintf( File, "%s\n", EpgChannels[Id] );
                    break;
                }
                EpgChannelData ++;
            }
        }
        ChannelId ++;
    }
    if( File != NULL )
    {
        fclose( File );
        fprintf( stderr, "LoadEPG: epg channels list saved in %s\n", FileName );
    }
    fprintf( stderr, "LoadEPG: found %i channels\n", NumEpgChannels );
    NumChannels = NumEpgChannels + NumEpgEquivChannels;
    Status.EpgChannels = STATUS_STOP_LOAD;
    Status.EpgThemes = STATUS_START_LOAD;
    }
}

void cLoadepgFilter::GetEpgThemes( const unsigned char *Data, int Length )
{
    if( Status.EpgThemes == STATUS_START_LOAD )
    {
        int Id, Theme, Offset;
        const unsigned char *EpgThemesIndex = Data + 3;
        typedef unsigned char EpgThemeType[15];
        EpgThemeType *EpgThemeData = ( EpgThemeType * ) ( Data + 19 );
        Theme = 0;
        Offset = 0;
        NumEpgThemes = ( Length - 19 ) / 15;
        if( NumEpgThemes > EPG_THEMES )
        {
            fprintf( stderr, "LoadEPG: error stream data, epg themes > %i\n", EPG_THEMES );
            Status.Plugin = STATUS_END;
            Status.EpgTime = STATUS_STOP_LOAD;
            Status.EpgChannels = STATUS_STOP_LOAD;
            Status.EpgThemes = STATUS_STOP_LOAD;
            Status.EpgEvents = STATUS_STOP_LOAD;
            Status.EpgDescriptions = STATUS_STOP_LOAD;
            Del( 0xd3, 0x92 );
            return;
        }
        for( Id = 0; Id < NumEpgThemes; Id ++ )
        {
            if( EpgThemesIndex[Theme] == Id )
            {
                Offset = ( Offset + 15 ) & 0xf0;
                Theme ++;
            }
            memcpy( EpgThemes[Offset], EpgThemeData, 15 );
            EpgThemes[Offset][15] = 0;
            CleanString( EpgThemes[Offset] );
            Offset ++;
            EpgThemeData ++;
        }
        fprintf( stderr, "LoadEPG: found %i themes\n", NumEpgThemes );
        Status.EpgThemes = STATUS_STOP_LOAD;
        Status.EpgEvents = STATUS_START_LOAD;
        Del( 0xd3, 0x92 );
        Set( 0xd2, 0x90 );
    }
}

void cLoadepgFilter::GetEpgThemes2( const unsigned char *Data, int Length )
{
    if( Status.EpgThemes == STATUS_START_LOAD )
    {
        int ThemeId, SubThemeId, Pos1, Pos2, PosThemeName, LenThemeName;
        unsigned char Count, StrLen;
        if( Length > 4 )
        {
            for( ThemeId = 0; ThemeId < Data[4]; ThemeId ++ )
            {
                Pos1 = ( Data[5+ThemeId*2] << 8 ) + Data[6+ThemeId*2];
                Pos1 += 3;
                if( Length > Pos1 )
                {
                    Count = Data[Pos1] & 0x3f;
                    for( SubThemeId = 0; SubThemeId <= Count; SubThemeId ++ )
                    {
                        Pos2 = ( Data[Pos1+1+SubThemeId*2] << 8 ) + Data[Pos1+2+SubThemeId*2];
                        Pos2 += 3;
                        if( Length > Pos2 )
                        {
                            StrLen = Data[Pos2] & 0x1f;
                            if( ( StrLen + Pos2 ) > Length )
                                return;
                        }
                        else
                        {
                            return;
                        }
                    }
                }
                else
                {
                    return;
                }
        }
    }
    NumEpgThemes = 0;
    bzero( EpgThemes, sizeof( EpgThemes ) );
    bzero( EpgFirstThemes, sizeof( EpgFirstThemes ) );
    for( ThemeId = 0; ThemeId < Data[4]; ThemeId ++ )
    {
        Pos1 = ( Data[5+ThemeId*2] << 8 ) + Data[6+ThemeId*2];
        Pos1 += 3;
        Count = Data[Pos1] & 0x3f;
        PosThemeName = 0;
        LenThemeName = 0;
        for( SubThemeId = 0; SubThemeId <= Count; SubThemeId ++ )
        {
            Pos2 = ( Data[Pos1+1+SubThemeId*2] << 8 ) + Data[Pos1+2+SubThemeId*2];
            Pos2 += 3;
            StrLen = Data[Pos2] & 0x1f;
            if( SubThemeId == 0 )
            {
                PosThemeName = Pos2 + 1;
                LenThemeName = StrLen;
                memcpy( &EpgThemes[NumEpgThemes], Data + PosThemeName, LenThemeName );
                EpgThemes[NumEpgThemes][LenThemeName] = 0;
                EpgFirstThemes[ThemeId] = NumEpgThemes;
            }
            else
            {
                if( ( LenThemeName + StrLen + 1 ) < 64 )
                {
                    memcpy( &EpgThemes[NumEpgThemes], Data + PosThemeName, LenThemeName );
                    EpgThemes[NumEpgThemes][LenThemeName] = ' ';
                    memcpy( &EpgThemes[NumEpgThemes][LenThemeName+1], Data + Pos2 + 1, StrLen );
                    EpgThemes[NumEpgThemes][LenThemeName+1+StrLen] = 0;
                }
            }
        NumEpgThemes ++;
        }
    }
    if( NumEpgThemes > EPG_THEMES )
    {
        fprintf( stderr, "LoadEPG: error stream data, epg themes > %i\n", EPG_THEMES );
        Status.Plugin = STATUS_END;
        Status.EpgTime = STATUS_STOP_LOAD;
        Status.EpgChannels = STATUS_STOP_LOAD;
        Status.EpgThemes = STATUS_STOP_LOAD;
        Status.EpgEvents = STATUS_STOP_LOAD;
        Status.EpgDescriptions = STATUS_STOP_LOAD;
        Del( 0x231, 0xc8 );
        return;
    }
    fprintf( stderr, "LoadEPG: found %i themes\n", NumEpgThemes );
    Status.EpgThemes = STATUS_STOP_LOAD;
    Status.EpgEvents = STATUS_START_LOAD;
    Del( 0x231, 0xc8 );
    Set( 0x234, 0xe6 );
    SectionInitial = 0;
    }
}

static int PatternSortEpgEvents( const void *a, const void *b )
{
    struct EpgEvent *EpgEventA = ( struct EpgEvent * ) a;
    struct EpgEvent *EpgEventB = ( struct EpgEvent * ) b;
    if( EpgEventA->ChannelId > EpgEventB->ChannelId )
        return 1;
    else if( EpgEventA->ChannelId < EpgEventB->ChannelId )
        return -1;
    else if( EpgEventA->ChannelId == EpgEventB->ChannelId )
    {
        if( EpgEventA->Time > EpgEventB->Time )
           return 1;
        else if( EpgEventA->Time < EpgEventB->Time )
           return -1;
    }
    return 0;
}

void cLoadepgFilter::GetEpgEvents( const unsigned char *Data, int Length )
{
    EpgEventData = ( EpgEventType * ) Data;
    if( Length == EPG_PROGRAM_LENGTH )
    {
        if( EpgEventData->Day == 7 )
            EpgEventData->Day = 0;
        if( EpgEventData->Hours > 15 )
            EpgEventData->Hours = EpgEventData->Hours - 4;
        else if( EpgEventData->Hours > 7 )
            EpgEventData->Hours = EpgEventData->Hours - 2;

        if( Status.EpgEvents == STATUS_START_LOAD )
        {
            if( EpgEventData->ChannelId != 0xff )
            {
                NumEpgEvents = 0;
                memset( ListEpgEvents, 0, sizeof( ListEpgEvents ) );
                memcpy( EpgEventInitial, EpgEventData, EPG_PROGRAM_LENGTH );
                Status.EpgEvents = STATUS_EXEC_LOAD;
                return;
            }
        }
        if( Status.EpgEvents == STATUS_EXEC_LOAD )
        {
            if( EpgEventData->ChannelId != 0xff )
            {
                ( EpgEvents + NumEpgEvents )->ChannelId = EpgEventData->ChannelId;
                ( EpgEvents + NumEpgEvents )->ThemeId = EpgEventData->ThemeId;
                ( EpgEvents + NumEpgEvents )->Time = ( ( EpgEventData->Day - Yesterday ) * 86400 ) + ( EpgEventData->Hours * 3600 ) + ( EpgEventData->Minutes * 60 );
                if( ( EpgEvents + NumEpgEvents )->Time < ( 6 * 3600 ) )
                    ( EpgEvents + NumEpgEvents )->Time = ( EpgEvents + NumEpgEvents )->Time + ( 7 * 86400 );
                ( EpgEvents + NumEpgEvents )->Time = ( EpgEvents + NumEpgEvents )->Time + YesterdayEpoch;
                ( EpgEvents + NumEpgEvents )->Time = ProviderLocalTime2UTC( ( EpgEvents + NumEpgEvents )->Time );
                ( EpgEvents + NumEpgEvents )->SummaryAvailable = EpgEventData->SummaryAvailable;
                ( EpgEvents + NumEpgEvents )->Duration = HILO16( EpgEventData->Duration ) * 60;
                memcpy( ( EpgEvents + NumEpgEvents )->Title, &EpgEventData->Title, 23 );
                ( EpgEvents + NumEpgEvents )->Title[23] = 0;
                CleanString( ( EpgEvents + NumEpgEvents )->Title );
                ( EpgEvents + NumEpgEvents )->PpvId = HILO32( EpgEventData->PpvId );
                ( EpgEvents + NumEpgEvents )->ProgramId = HILO32( EpgEventData->ProgramId );
                ListEpgEvents[( EpgEvents + NumEpgEvents )->ChannelId] ++;
                NumEpgEvents ++;
                if( NumEpgEvents > EPG_PROGRAMS )
                {
                    fprintf( stderr, "LoadEPG: error stream data, epg programs > %i\n", EPG_PROGRAMS );
                    Status.Plugin = STATUS_END;
                    Status.EpgTime = STATUS_STOP_LOAD;
                    Status.EpgChannels = STATUS_STOP_LOAD;
                    Status.EpgThemes = STATUS_STOP_LOAD;
                    Status.EpgEvents = STATUS_STOP_LOAD;
                    Status.EpgDescriptions = STATUS_STOP_LOAD;
                    Del( 0xd2, 0x90 );
                    return;
                }
                if( ! memcmp( EpgEventInitial, Data, EPG_PROGRAM_LENGTH ) )
                {
                    qsort( &EpgEvents[0], NumEpgEvents, sizeof( EpgEvents[0] ), &PatternSortEpgEvents );
                    fprintf( stderr, "LoadEPG: found %i programs\n", NumEpgEvents );
                    Status.EpgEvents = STATUS_STOP_LOAD;
                    Status.EpgDescriptions = STATUS_START_LOAD;
                    Del( 0xd2, 0x90 );
                    Set( 0xd3, 0x90 );
                }
            }
        }
    }
}

unsigned int GetInt32( const unsigned char *pData )
{
    return ( pData[0] << 24 ) | ( pData[1] << 16 ) | ( pData[2] << 8 ) | ( pData[3] );
}

unsigned int GetInt16( const unsigned char *pData )
{
    return ( pData[0] << 8 ) | ( pData[1] );
}

void cLoadepgFilter::GetEpgEvents2( const unsigned char *Data, int Length )
{
    if( Status.EpgEvents == STATUS_START_LOAD )
    {
        int Pos, Len;
        unsigned int Section;
        bool Check;
        if( Data[Length-1] != 0xff )
            return;
        Pos = 18;
        while( Pos < Length )
        {
            Check = false;
            Pos += 7;
            if( Pos < Length )
            {
                Pos += 3;
                if( Pos < Length )
                {
                    if( Data[Pos] > 0xc0 )
                    {
                        Pos += ( Data[Pos] - 0xc0 );
                        Pos += 4;
                        if( Pos < Length )
                        {
                            if( Data[Pos] == 0xff )
                            {
                                Pos += 1;
                                Check = true;
                            }
                        }
                    }
                }
            }
            if( Check == false )
                return;
        }
        Section = GetInt32( Data + 3 );
        if( SectionInitial == 0 )
        {
            SectionInitial = Section;
            NumEpgEvents = 0;
            memset( ListEpgEvents, 0, sizeof( ListEpgEvents ) );
        }
        else if( Section == SectionInitial )
        {
            qsort( &EpgEvents[0], NumEpgEvents, sizeof( EpgEvents[0] ), &PatternSortEpgEvents );
            fprintf( stderr, "LoadEPG: found %i programs\n", NumEpgEvents );
            Status.EpgEvents = STATUS_STOP_LOAD;
            Status.EpgDescriptions = STATUS_START_LOAD;
            Del( 0x234, 0xe6 );
            Set( 0x236, 0x96 );
            SectionInitial = 0;
            return;
        }
        Pos = 18;
        while( Pos < Length )
        {
            ( EpgEvents + NumEpgEvents )->ChannelId = Data[Pos];
            ( EpgEvents + NumEpgEvents )->Time = ( GetInt16( Data + Pos + 3 ) - 40587 ) * 86400 + ( ( ( Data[Pos+5] & 0xf0 ) >> 4 ) * 10 + ( Data[Pos+5] & 0x0f ) ) * 3600 + ( ( ( Data[Pos+6] & 0xf0 ) >> 4 ) * 10 + ( Data[Pos+6] & 0x0f ) ) * 60;
            ( EpgEvents + NumEpgEvents )->Duration = ( GetInt16( Data + Pos + 8 ) >> 4 ) * 60;
            Len = Data[Pos+10] & 0x3f;
            memcpy( ( EpgEvents + NumEpgEvents )->Title, Data + Pos + 11, Len );
            ( EpgEvents + NumEpgEvents )->Title[Len] = 0;
            CleanString( ( EpgEvents + NumEpgEvents )->Title );
            Pos += Len + 11;
            ( EpgEvents + NumEpgEvents )->ThemeId = EpgFirstThemes[( Data[7] & 0x3f )] + ( Data[Pos] & 0x3f );
            ( EpgEvents + NumEpgEvents )->ProgramId = GetInt16( Data + Pos + 1 );
            ( EpgEvents + NumEpgEvents )->SummaryAvailable = 1;
            ( EpgEvents + NumEpgEvents )->PpvId = 0;
            ListEpgEvents[( EpgEvents + NumEpgEvents )->ChannelId+1] ++;
            Pos += 4;
            NumEpgEvents ++;
            if( NumEpgEvents > EPG_PROGRAMS )
            {
                fprintf( stderr, "LoadEPG: error stream data, epg programs > %i\n", EPG_PROGRAMS );
                Status.Plugin = STATUS_END;
                Status.EpgTime = STATUS_STOP_LOAD;
                Status.EpgChannels = STATUS_STOP_LOAD;
                Status.EpgThemes = STATUS_STOP_LOAD;
                Status.EpgEvents = STATUS_STOP_LOAD;
                Status.EpgDescriptions = STATUS_STOP_LOAD;
                Del( 0x234, 0xe6 );
                return;
            }
        }
    }
}

static int PatternSortEpgDescriptions( const void *a, const void *b )
{
    struct EpgDescription *EpgDescriptionA = ( struct EpgDescription * ) a;
    struct EpgDescription *EpgDescriptionB = ( struct EpgDescription * ) b;
    if( EpgDescriptionA->ProgramId > EpgDescriptionB->ProgramId )
        return 1;
    else if( EpgDescriptionA->ProgramId < EpgDescriptionB->ProgramId )
        return -1;
    else return 0;
}

void cLoadepgFilter::GetEpgDescriptions( const unsigned char *Data, int Length )
{
    EpgDescriptionData = ( EpgDescriptionType * ) Data;
    if( EpgDescriptionData->Byte7 == 0xff && EpgDescriptionData->Byte8 == 0xff && EpgDescriptionData->Byte9 == 0xff && EpgDescriptionData->NumReplays < EPG_SUMMARY_NUM_REPLAYS && Length > EPG_SUMMARY_LENGTH )
    {
        if( Status.EpgDescriptions == STATUS_START_LOAD )
        {
            NumEpgDescriptions = 0;
            memcpy( EpgDescriptionInitial, EpgDescriptionData, EPG_SUMMARY_LENGTH );
            Status.EpgDescriptions = STATUS_EXEC_LOAD;
            return;
        }
        else if( Status.EpgDescriptions == STATUS_EXEC_LOAD )
        {
            ( EpgDescriptions + NumEpgDescriptions )->ProgramId = HILO32( EpgDescriptionData->ProgramId );
            TextOffset = EPG_SUMMARY_LENGTH + ( EpgDescriptionData->NumReplays * EPG_SUMMARY_REPLAY_LENGTH );
            TextLength = Length + 1 - TextOffset;
            ( EpgDescriptions + NumEpgDescriptions )->Text = ( unsigned char * ) malloc( TextLength );
            memcpy( ( EpgDescriptions + NumEpgDescriptions )->Text, &Data[TextOffset], TextLength );
            ( EpgDescriptions + NumEpgDescriptions )->Text[TextLength-1] = 0;
        
            CleanString( ( EpgDescriptions + NumEpgDescriptions )->Text );
            NumEpgDescriptions ++;
            if( NumEpgDescriptions > EPG_SUMMARIES )
            {
                fprintf( stderr, "LoadEPG: error stream data, epg summaries > %i\n", EPG_SUMMARIES );
                Status.Plugin = STATUS_END;
                Status.EpgTime = STATUS_STOP_LOAD;
                Status.EpgChannels = STATUS_STOP_LOAD;
                Status.EpgThemes = STATUS_STOP_LOAD;
                Status.EpgEvents = STATUS_STOP_LOAD;
                Status.EpgDescriptions = STATUS_STOP_LOAD;
                Del( 0xd3, 0x90 );
                return;
            }
            if( ! memcmp( EpgDescriptionInitial, EpgDescriptionData, EPG_SUMMARY_LENGTH ) )
            {
                qsort( &EpgDescriptions[0], NumEpgDescriptions, sizeof( EpgDescriptions[0] ), &PatternSortEpgDescriptions );
                fprintf( stderr, "LoadEPG: found %i summaries\n", NumEpgDescriptions );
                Status.EpgDescriptions = STATUS_STOP_LOAD;
                Del( 0xd3, 0x90 );
            }
        }
    }
}

void cLoadepgFilter::GetEpgDescriptions2( const unsigned char *Data, int Length )
{
    if( Status.EpgDescriptions == STATUS_START_LOAD )
    {
        int Pos, Len, Loop, LenLine;
        unsigned int Section;
        bool Check;
        Check = false;
        if( Length > 18 )
        {
            Loop = Data[12];
            Pos = 13 + Loop;
            if( Length > Pos )
            {
                Loop = Data[Pos] & 0x0f;
                Pos += 1;
                if( Length > Pos )
                {
                    Len = 0;
                    for( ; Loop > 0; Loop -- )
                    {
                        if( Length > ( Pos + Len ) )
                        {
                            LenLine = Data[Pos+Len];
                            Len += LenLine + 1;
                        }
                    }
                    if( Length > ( Pos + Len + 1 ) && ( Data[Pos+Len+1] == 0xff ))
                        Check = true;
                }
            }
        }
        if( Check == false )
            return;
        memcpy( Data2, Data, Length );
        Section = GetInt32( Data2 + 3 );
        if( SectionInitial == 0 )
        {
            SectionInitial = Section;
            NumEpgDescriptions = 0;
        }
        else if( Section == SectionInitial )
        {
            qsort( &EpgDescriptions[0], NumEpgDescriptions, sizeof( EpgDescriptions[0] ), &PatternSortEpgDescriptions );
            fprintf( stderr, "LoadEPG: found %i summaries\n", NumEpgDescriptions );
            Status.EpgDescriptions = STATUS_STOP_LOAD;
            Del( 0x236, 0x96 );
            return;
        }
        ( EpgDescriptions + NumEpgDescriptions )->ProgramId = GetInt16( Data2 + 3 );
        Loop = Data2[12];
        Pos = 13 + Loop;
        Loop = Data2[Pos] & 0x0f;
        Pos += 1;
        Len = 0;
        for( ; Loop > 0; Loop -- )
        {
            LenLine = Data2[Pos+Len];
            Data2[Pos+Len] = '|';
            Len += LenLine + 1;
        }
        if( Len > 0 )
            Data2[Pos+Len] = 0;
        else
            Data2[Pos+1] = 0;
        ( EpgDescriptions + NumEpgDescriptions )->Text = ( unsigned char * ) malloc( Len + 1 );
        memcpy( ( EpgDescriptions + NumEpgDescriptions )->Text, Data2 + Pos + 1, Len + 1 );
        ( EpgDescriptions + NumEpgDescriptions )->Text[Len] = 0;
        CleanString( ( EpgDescriptions + NumEpgDescriptions )->Text );
        NumEpgDescriptions ++;
        if( NumEpgDescriptions > EPG_SUMMARIES )
        {
            fprintf( stderr, "LoadEPG: error stream data, epg summaries > %i\n", EPG_SUMMARIES );
            Status.Plugin = STATUS_END;
            Status.EpgTime = STATUS_STOP_LOAD;
            Status.EpgChannels = STATUS_STOP_LOAD;
            Status.EpgThemes = STATUS_STOP_LOAD;
            Status.EpgEvents = STATUS_STOP_LOAD;
            Status.EpgDescriptions = STATUS_STOP_LOAD;
            Del( 0x236, 0x96 );
            return;
        }
    }
}

void cLoadepgFilter::CleanString( unsigned char *String )
{
    unsigned char *Src = String;
    unsigned char *Dst = String;
    int Spaces = 0;
    while( *Src )
    {
        if( *Src < 0x20 || ( *Src > 0x7e && *Src < 0xa0 ) )
            *Src = 0x20;
        else if( *Src == 0x20 )
            Spaces ++;
        else
            Spaces = 0;
        if( Spaces < 2 )
        {
            *Dst = *Src;
            *Dst ++;
        }
        *Src ++;
    }
    if( Spaces > 0 )
        Dst --;
    *Dst = 0;
}

//-------------------------------------------------------------------------

//-- cLoadepgOsd ----------------------------------------------------------

cLoadepgOsd::cLoadepgOsd( void )
{
    Osd = NULL;
    Filter = NULL;
    OldChannel = NULL;
    EpgChannel = NULL;
    Status.Plugin = STATUS_ACTIVE;
    Status.EpgTime = STATUS_NULL;
    Status.EpgChannels = STATUS_NULL;
    Status.EpgThemes = STATUS_NULL;
    Status.EpgEvents = STATUS_NULL;
    Status.EpgDescriptions = STATUS_NULL;
    MenuItem = 0;
    Timeout = 0;
    LoadepgConfig.OldUpdateChannels = Setup.UpdateChannels;
    Setup.UpdateChannels = 0;
    EpgDevice=NULL;  
    for (int i=0; i<cDevice::NumDevices() && EpgDevice==NULL; i++) {
      if (cDevice::GetDevice(i)->HasInput()) {
      //if (cDevice::GetDevice(i)->CardIndex()==LoadepgConfig.DeviceNumber -1) {
        EpgDevice=cDevice::GetDevice(i);
      }
    }

    if ( EpgDevice && EpgDevice->Priority() >= 0 ) // other than live-viewing (= -1)
    { 
     esyslog("loadepg (%s:%d): EpgDevice has priority greater >= 0. Searching for a free SatTuner",__FILE__,__LINE__);
     // find the first free SAT tuner and use it as EpgDevice
     cDevice *tmpDevice=NULL;
     EpgDevice = NULL;
     for (int i=0 ; i<cDevice::NumDevices() ; i++)
     {
        tmpDevice = cDevice::GetDevice(i);
        if ( tmpDevice && tmpDevice->Priority() < 0 && tmpDevice->ProvidesSource(cSource::stSat) ) // free sat tuner
        {
            EpgDevice = tmpDevice;
            esyslog("loadepg (%s:%d): Using Tuner #%d as EpgTuner",__FILE__,__LINE__,i);
            break;
        }

     }
    }
    if( EpgDevice == NULL )
    {
     esyslog("loadepg (%s:%d): ERROR EpgDevice null!! ",__FILE__,__LINE__);
     Status.Plugin = STATUS_END; /// Does this make the plugin exit smoothly without crash???
    }
}

cLoadepgOsd::~cLoadepgOsd( void )
{
    memset( &Status, STATUS_NULL, sizeof( struct Status ) );
    Status.Plugin = STATUS_NULL;
    Status.Svdrp = STATUS_NULL;
    MenuItem = 0;
    Setup.UpdateChannels = LoadepgConfig.OldUpdateChannels;
    if( Filter && EpgDevice )
    {
        EpgDevice->Detach( Filter );
        delete Filter;
    }
    if( Osd )
        delete Osd;
}

void cLoadepgOsd::Action( void )
{
    int Result;
    switch( Status.ProviderType[MenuItem] )
    {
        case PROVIDER_TYPE_SATELLITE:
        if( SaveOldChannel() )
        {
            if( SwitchToEpgChannel() )
            {
                Filter = new cLoadepgFilter();
                EpgDevice->AttachFilter( Filter );
            }
            else
            {
                Status.Plugin = STATUS_END;
                return;
            }
        }
        else
        {
            Status.Plugin = STATUS_END;
            return;
        }
        break;
    case PROVIDER_TYPE_FILE:
        fprintf( stderr, "LoadEPG: loading external file: %s\n", EpgProviderValue1[MenuItem] );
        SaveEpg();
        sleep( 3 );
        Status.Plugin = STATUS_END;
        break;
    case PROVIDER_TYPE_SCRIPT:
        fprintf( stderr, "LoadEPG: executing external script: %s\n", EpgProviderValue1[MenuItem] );
        Result = SystemExec( EpgProviderValue1[MenuItem] );
        if( Result == -1 )
            fprintf( stderr, "LoadEPG: failed to execute script %s\n", EpgProviderValue1[MenuItem] );       
        else
        {
            fprintf( stderr, "LoadEPG: loading external file: %s\n", EpgProviderValue2[MenuItem] );
            SaveEpg();
        }
        Status.Plugin = STATUS_END;
        break;
    default:
        break;
    }
    return;
}

void cLoadepgOsd::Show( void )
{
    if ( EpgDevice == NULL) return;
    Osd = cOsdProvider::NewTrueColorOsd( 145, 88, false );
    if( Osd )
    {
        tArea Area = { 0, 0, 429, 399, 32 };
        Osd->SetAreas( &Area, 1 );
        Osd->DrawRectangle( 0, 0, 429, 399, clrTransparent );
        DrawOsd();
    }
    if( Status.Plugin == STATUS_ACTIVE && Status.Svdrp != STATUS_NULL )
    {
      MenuItem = Status.Svdrp - 1;
      cRemote::Put( kOk );
    }
}

bool cLoadepgOsd::SaveOldChannel( void )
{
    if (EpgDevice == NULL) return false;
    OldChannel = Channels.GetByNumber( EpgDevice->CurrentChannel() );
    if( OldChannel )
    {
        return true;
    }
    return false;
}

void cLoadepgOsd::RestoreOldChannel( void )
{
    if( OldChannel )
    {
         if (UsingPrimaryDevice)
            cDevice::PrimaryDevice()->SwitchChannel( OldChannel, true);
         else 
            EpgDevice->SwitchChannel( OldChannel, false );
    }
}

bool cLoadepgOsd::SwitchToEpgChannel( void )
{
    char ProviderName[64];
    char Frequency[64];
    char Polarization[64];
    char SourceName[64];
    char SymbolRate[64];
    if( OldChannel )
    {
        EpgChannel = new cChannel();
        *EpgChannel = *OldChannel;
        sscanf( EpgProviderValue1[MenuItem], "%[^:]:%[^:]:%[^:]:%[^:]:%[^:]", ProviderName, Frequency, Polarization, SourceName, SymbolRate );
#if APIVERSNUM >= 10514
        EpgChannel->cChannel::SetSatTransponderData( cSource::FromString( SourceName ), atoi( Frequency ), Polarization[0], atoi( SymbolRate ), DVBFE_FEC_AUTO, DVBFE_MOD_AUTO, DVBFE_DELSYS_DVBS, DVBFE_ROLLOFF_UNKNOWN );
#else   
        EpgChannel->cChannel::SetSatTransponderData( cSource::FromString( SourceName ), atoi( Frequency ), Polarization[0], atoi( SymbolRate ), FEC_AUTO );
#endif  
    //cDevice::GetDevice( LoadepgConfig.DeviceNumber - 1 )->SwitchChannel( EpgChannel, true );
        if (EpgDevice==cDevice::ActualDevice() && cDevice::GetDevice(EpgChannel,0
#if APIVERSNUM >= 10500      
          ,true
#endif      
              ) == EpgDevice) {
              cDevice::PrimaryDevice()->SwitchChannel( EpgChannel, true );
              UsingPrimaryDevice = true;
        } else {
              EpgDevice->SwitchChannel( EpgChannel, false );
              UsingPrimaryDevice = false;
        }
        usleep( 2000000 );
        //if( cDevice::GetDevice( LoadepgConfig.DeviceNumber - 1 )->HasLock() )
        if( EpgDevice->HasLock() )
            return true;
        else
            fprintf( stderr, "LoadEPG: failed switch to epg channel data\n" );
    }
    return false;
}

eOSState cLoadepgOsd::ProcessKey( eKeys Key )
{
    if (EpgDevice == NULL ) {
      Skins.Message(mtError,tr("Card not available"));
      return osEnd;
    }
    eOSState state = cOsdObject::ProcessKey( Key );
    StatusKey = 0;
    if( state == osUnknown )
    {
        switch( Key )
        {
            case kOk:
                StatusKey = 1;
                if( Status.Plugin == STATUS_ACTIVE )
                {
                    NumEpgChannels = 0;
                    if( Status.ProviderType[MenuItem] == PROVIDER_TYPE_SCRIPT )
                        Timeout = time( NULL ) + TIMEOUT_SCRIPT;
                    else
                        Timeout = time( NULL ) + TIMEOUT_DEFAULT;
                    Status.Plugin = STATUS_INIT;
                    Osd->DrawRectangle( 0, 0, 429, 399, clrTransparent );
                    DrawOsd();
                    Start();
                }
                break;
            case kUp:
            case kUp | k_Repeat:
                StatusKey = 1;
                if( Status.Plugin == STATUS_ACTIVE )
                {
                    MenuItem = MenuItem - 1;
                    if( MenuItem < 0 )
                        MenuItem = 0;
                }
                break;
            case kDown:
            case kDown | k_Repeat:
                StatusKey = 1;
                if( Status.Plugin == STATUS_ACTIVE )
                {
                    MenuItem = MenuItem + 1;
                    if( MenuItem >= EpgProviders )
                        MenuItem = EpgProviders - 1;
                }
                break;
            case kBack:
                StatusKey = 1;
                Status.Plugin = STATUS_END;
                break;
            default:
                break;
        }
        Actions();
        state = osContinue;
        if( Status.Plugin == STATUS_INIT || Status.Plugin == STATUS_EXEC_LOAD )
        {
            if( NumEpgChannels == 0 )
            {
                if( time( NULL ) > Timeout )
                {
                    Status.Plugin = STATUS_TIMEOUT;
                    fprintf( stderr, "LoadEPG: timeout, no data epg available\n" );
                    Timeout = 0;
                }
            }
        }
        else if( Status.Plugin == STATUS_END )
        {
            Cancel( 3 );
            if( Status.ProviderType[MenuItem] == PROVIDER_TYPE_SATELLITE )
                RestoreOldChannel();
            Status.Plugin = STATUS_NULL;
            Status.Svdrp = STATUS_NULL;
            Osd->DrawRectangle( 0, 0, 429, 399, clrTransparent );
            DrawOsd();
            return osEnd;
        }
        else if( Status.Plugin == STATUS_TIMEOUT )
        {
            DrawOsd();
            if( Timeout == 0 )
                Timeout = time( NULL ) + 4;
            else if( time( NULL ) > Timeout )
                Status.Plugin = STATUS_END;
        }
    }
    return state;
}

static int PatternSearchSummary( const void *a, const void *b )
{
    struct EpgDescription *EpgDescriptionA = ( struct EpgDescription * ) a;
    struct EpgDescription *EpgDescriptionB = ( struct EpgDescription * ) b;
    if( EpgDescriptionA->ProgramId > EpgDescriptionB->ProgramId )
        return 1;
    else if( EpgDescriptionA->ProgramId < EpgDescriptionB->ProgramId )
        return -1;
    else
        return 0;
}

void cLoadepgOsd::SaveEpg( void )
{
    int ProgramId, BeginProgramId, LastProgramId, ChannelId;
    struct EpgDescription Key, *Result;
    char *FileName;
    FILE *File;
    VdrEquivChannelId = -1;
    if( Status.ProviderType[MenuItem] == PROVIDER_TYPE_FILE )
    {
      asprintf( &FileName, "%s", EpgProviderValue1[MenuItem] );
      goto LoadFile;
    }
    else if( Status.ProviderType[MenuItem] == PROVIDER_TYPE_SCRIPT )
    {
      asprintf( &FileName, "%s", EpgProviderValue2[MenuItem] );
      goto LoadFile;
    }
    asprintf( &FileName, "/tmp/epg.tmp" );
    if( ( File = fopen( FileName, "w" ) ) != NULL )
    {
        BeginProgramId = 0;
        for( ChannelId = 0; ChannelId < NumEpgChannels; ChannelId ++ )
        {
            LastProgramId = BeginProgramId + ListEpgEvents[ChannelId+1];
            if( strlen( EpgChannels[ChannelId] ) > 0 )
            {
                if( ListEpgEvents[ChannelId+1] > 0 )
                {
                    BeginLoop:;
                    if( VdrEquivChannelId > -1 )
                    {
                        fprintf( File, "C %s\n", EpgEquivChannels[VdrEquivChannelId][1] );
                        EpgEquivChannels[VdrEquivChannelId][0][0] = 0;
                    }
                    else
                        fprintf( File, "C %s\n", EpgChannels[ChannelId] );
                    for( ProgramId = BeginProgramId; ProgramId < LastProgramId; ProgramId ++ )
                    {
                        if( ( ( EpgEvents + ProgramId )->Time + ( EpgEvents + ProgramId )->Duration ) > ( unsigned int ) CurrentTime )
                        {
                            fprintf( File, "E %u %u %u 90\n", ( EpgEvents + ProgramId )->ProgramId, ( unsigned int ) ( EpgEvents + ProgramId )->Time, ( EpgEvents + ProgramId )->Duration );
                            fprintf( File, "T %s\n", ( EpgEvents + ProgramId )->Title );
                            fprintf( File, "S %s - %d\n", EpgThemes[( EpgEvents + ProgramId )->ThemeId], ( ( EpgEvents + ProgramId )->Duration / 60 ) );
                            if( ( EpgEvents + ProgramId )->SummaryAvailable == 1 )
                            {
                                Key.ProgramId = ( EpgEvents + ProgramId )->ProgramId;
                                Result = ( EpgDescription * ) bsearch( &Key, EpgDescriptions, NumEpgDescriptions, sizeof( EpgDescriptions[0] ), &PatternSearchSummary );
                                if( Result != NULL )
                                    fprintf( File, "D %s\n", Result->Text );
                            }
                            fprintf( File, "e\n" );
                        }
                    }
                    fprintf( File, "c\n" );
                }
            }
            if( LoadepgConfig.UseFileEquivalences == true )
            {
                for( VdrEquivChannelId = 0; VdrEquivChannelId < NumEpgEquivChannels; VdrEquivChannelId ++ )
                {
                    if( ! memcmp( EpgChannels[ChannelId], EpgEquivChannels[VdrEquivChannelId][0], sizeof( EpgChannels[ChannelId] ) ) )
                        goto BeginLoop;
                }
            }
            VdrEquivChannelId = -1;
            BeginProgramId = BeginProgramId + ListEpgEvents[ChannelId+1];
        }
        fclose( File );
#if APIVERSNUM >= 10500      
        const char *syschar=cCharSetConv::SystemCharacterTable();
        if (syschar==NULL || !strcasestr(syschar, "ISO-8859-15")) {
            char *cmd;
            asprintf(&cmd, "iconv -f ISO-8859-15 -t %s -o \"%s.converted\" \"%s\"", syschar ? syschar : "UTF-8",FileName,FileName);
            SystemExec(cmd);
            free(cmd);
            asprintf(&cmd, "mv \"%s.converted\" \"%s\"", FileName,FileName); 
            SystemExec(cmd);
            free(cmd);
        }
#endif     
    }
    LoadFile:;
    if( ( File = fopen( FileName, "r" ) ) != NULL )
    {
        rewind( File );
        if( cSchedules::Read( File ) )
            cSchedules::Cleanup( true );
        fclose( File );
        fprintf( stderr, "LoadEPG: epg saved\n" );
    }
    else
      fprintf( stderr, "LoadEPG: \"%s\" file not found\n", FileName );
    free( FileName );
    Status.Plugin = STATUS_END;
}

void cLoadepgOsd::Actions( void )
{
    if( StatusKey == 1 )
    {
        DrawOsd();
        StatusKey = 0;
    }
    if( Status.Plugin == STATUS_LOADEPG )
        SaveEpg();
}

void cLoadepgOsd::DrawOsd( void )
{
    int Id;
    char *ProviderName;
    const cFont *Font = cFont::GetFont( fontOsd );
    Margin = 10;
    Padding = 1;
    if( Osd )
    {
            if( Status.Plugin == STATUS_ACTIVE )
            {
                LineHeight = Font->Height() + ( Padding * 2 );
                LineWidth = Font->Width( tr( "LoadEPG" ) );
                if( Font->Width( tr( "Select a provider and press Ok" ) ) > LineWidth )
                    LineWidth = Font->Width( tr( "Select a provider and press Ok" ) );
                LineWidth = LineWidth + ( Margin * 2 );
                if( LineWidth > 429 )
                    LineWidth = 429;
                Left = ( 429 - LineWidth ) / 2;
                Top = ( 399 - ( ( LineHeight * ( EpgProviders + 2 ) ) + ( 2 * Margin ) ) ) / 2;
                
                // Draw text top
                TextTopX1 = Left;
                TextTopY1 = Top;
                TextTopX2 = TextTopX1 + LineWidth;
                TextTopY2 = TextTopY1 + LineHeight;
                Osd->DrawRectangle( TextTopX1, TextTopY1, TextTopX2, TextTopY2, clrBgTextTop );
                Osd->DrawText( TextTopX1 + Margin, TextTopY1 + Padding, tr( "LoadEPG" ), clrFgTextTop, clrBgTextTop, Font );
                
                // Draw text scroll
                TextScrollX1 = Left;
                TextScrollX2 = TextScrollX1 + LineWidth;
                Osd->DrawRectangle( TextScrollX1, TextTopY2, TextScrollX2, TextTopY2 + ( LineHeight * EpgProviders ) + ( 2 * Margin ), clrBgTextScroll );
                for( Id = 0; Id < EpgProviders; Id ++ )
                {
                    TextScrollY1 = TextTopY2 + ( (LineHeight) * Id ) + Margin;
                    TextScrollY2 = TextScrollY1 + LineHeight ;
                    asprintf( &ProviderName, "%i %s", ( Id + 1 ), EpgProviderTitle[Id] );
                    if( MenuItem == Id )
                    {
                        int textWidth = cFont::GetFont(fontOsd)->Width(ProviderName);
                        Osd->DrawRectangle( TextScrollX1 + Margin*2 + textWidth , TextScrollY1+Padding, TextScrollX2 - Margin, TextScrollY2-2*Padding, clrBgMenuItem );
                        Osd->DrawRectangle( TextTopX1 , TextScrollY1+Padding, TextScrollX1+Margin*2, TextScrollY2-2*Padding, clrBgMenuItem );
                        Osd->DrawText( TextScrollX1 + ( Margin * 2 ), TextScrollY1 + Padding, ProviderName, clrFgMenuItem, clrBgMenuItem, Font );
                    }
                    else
                    {
                        Osd->DrawRectangle( TextScrollX1 + Margin , TextScrollY1, TextScrollX2 - Margin, TextScrollY2, clrBgTextScroll );
                        Osd->DrawText( TextScrollX1 + ( Margin * 2 ), TextScrollY1 + Padding, ProviderName, clrFgTextScroll, clrBgTextScroll, Font );
                    }
                    free( ProviderName );
                }
                
                // Draw text bottom
                TextBottomX1 = Left;
                TextBottomY1 = TextScrollY2 + Margin;
                TextBottomX2 = TextBottomX1 + LineWidth;
                TextBottomY2 = TextBottomY1 + LineHeight;
                int textWidth = cFont::GetFont(fontOsd)->Width(tr( "Select a provider and press Ok" ));
                Osd->DrawRectangle( TextBottomX1 + textWidth + Margin, TextBottomY1+Padding, TextBottomX2, TextBottomY2-2*Padding, clrBgTextBottom );
                Osd->DrawRectangle( TextTopX1, TextBottomY1+Padding, TextBottomX1+Margin, TextBottomY2-2*Padding, clrBgTextBottom );
                Osd->DrawText( TextBottomX1 + Margin, TextBottomY1 + Padding, tr( "Select a provider and press Ok" ), clrFgTextBottom, clrBgTextBottom, Font );
                
            }
            else if( Status.Plugin == STATUS_INIT )
            {
                if( UsingPrimaryDevice )
                {
                    LineHeight = Font->Height() + ( Padding * 2 );
                    LineWidth = 429;
                    Left = 0;
                    Top = ( 399 - ( LineHeight * 2 ) ) / 2;
                    TextTopX1 = Left;
                    TextTopY1 = Top;
                    TextTopX2 = TextTopX1 + LineWidth;
                    TextTopY2 = TextTopY1 + LineHeight;
                    Osd->DrawRectangle( TextTopX1, TextTopY1, TextTopX2, TextTopY2, clrBgTextTop );
                    Osd->DrawText( TextTopX1 + Margin, TextTopY1 + Padding, tr( "LoadEPG" ), clrFgTextTop, clrBgTextTop, Font );
                    TextBottomX1 = Left;
                    TextBottomY1 = TextTopY2 + 2;
                    TextBottomX2 = TextBottomX1 + LineWidth;
                    TextBottomY2 = TextBottomY1 + LineHeight;
                    Osd->DrawRectangle( TextBottomX1, TextBottomY1, TextBottomX2, TextBottomY2, clrBgTextBottom );
                    Osd->DrawText( TextBottomX1 + ( LineWidth - Font->Width( tr( "Wait load epg data" ) ) ) / 2, TextBottomY1 + Padding, tr( "Wait load epg data" ), clrFgTextBottom, clrBgTextBottom, Font );
                }
                else
                {
                    LineHeight = Font->Height() + ( Padding * 2 );
                    LineWidth = Font->Width( tr( "LoadEPG" ) ) + ( Margin * 2 );
                    Left = 429 - ( LineWidth + 1 );
                    Top = 399 - ( LineHeight + 1 );
                    TextTopX1 = Left;
                    TextTopY1 = Top;
                    TextTopX2 = TextTopX1 + LineWidth;
                    TextTopY2 = TextTopY1 + LineHeight;
                    Osd->DrawRectangle( TextTopX1, TextTopY1, TextTopX2, TextTopY2, clrBgTextTop );
                    Osd->DrawText( TextTopX1 + Margin, TextTopY1 + Padding, tr( "LoadEPG" ), clrFgTextTop, clrBgTextTop, Font );
                }
            }
            else if( Status.Plugin == STATUS_TIMEOUT )
            {
              LineHeight = Font->Height() + ( Padding * 2 );
              LineWidth = 429;
              Left = 0;
              Top = ( 399 - ( LineHeight * 2 ) ) / 2;
              TextTopX1 = Left;
              TextTopY1 = Top;
              TextTopX2 = TextTopX1 + LineWidth;
              TextTopY2 = TextTopY1 + LineHeight;
              Osd->DrawRectangle( 0, 0, 429, 399, clrTransparent );
              Osd->DrawRectangle( TextTopX1, TextTopY1, TextTopX2, TextTopY2, clrRed );
              Osd->DrawText( TextTopX1 + ( LineWidth - Font->Width( tr( "No epg data available" ) ) ) / 2, TextTopY1 + Padding, tr( "No data epg available" ), clrWhite, clrRed, Font );
            }
            Osd->Flush();
    }
}

cLoadepgSetup::cLoadepgSetup( void )
{
    Clear();
    Add( new cMenuEditBoolItem( tr( "Use file channels equivalences" ), &LoadepgConfig.UseFileEquivalences, tr( "No" ), tr( "Yes" ) ) );
    Add( new cMenuEditIntItem( tr( "DVB-S device for scanning" ), &LoadepgConfig.DeviceNumber, 1, cDevice::NumDevices() ) );
    Display();
}

void cLoadepgSetup::Store( void )
{
    SetupStore( "UseFileEquivalences", LoadepgConfig.UseFileEquivalences );
    SetupStore( "DeviceNumber", LoadepgConfig.DeviceNumber );
}

//-------------------------------------------------------------------------

cPluginLoadepg::cPluginLoadepg(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
  LoadepgConfig.UseFileEquivalences = false;
  LoadepgConfig.DeviceNumber = 1;
}

cPluginLoadepg::~cPluginLoadepg()
{
  // Clean up after yourself!
}

const char *cPluginLoadepg::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  static char *HelpString = 0;
  free( HelpString );
  asprintf( &HelpString,
            "  -c PATH,  --config=PATH  to specify directory of configuration data\n"
        "                           relative to VDR Plugin configuration directory\n"
        "                           (default: \"/video/plugins\")\n"
      );
  return HelpString;
}

bool cPluginLoadepg::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  ConfigPlugin = NULL;
  static struct option long_options[] = {
    { "config", required_argument, NULL, 'c' },
    { NULL }
  };
  int c;
  while( ( c = getopt_long( argc, argv, "c:", long_options, NULL ) ) != -1 )
  {
    switch( c )
    {
      case 'c':
        ConfigPlugin = ( char * ) malloc( sizeof ( char ) * ( strlen( optarg ) + 1 ) );
        snprintf( ConfigPlugin, ( strlen( optarg ) + 1 ), "%s", optarg );
        break;
      default:
        return false;
    }
  }
  return true;
}

bool cPluginLoadepg::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  return true;
}

bool cPluginLoadepg::Start(void)
{
  // Start any background activities the plugin shall perform.
#if APIVERSNUM >= 10507
  I18nRegister( "loadepg" );
#else
  RegisterI18n( LoadepgPhrases );
#endif
  memset( &Status, STATUS_NULL, sizeof( struct Status ) );
  Status.Plugin  = STATUS_NULL;
  return true;
}

void cPluginLoadepg::Stop(void)
{
  // Stop any background activities the plugin shall perform.
#if APIVERSNUM < 10507
  RegisterI18n( NULL );
#endif
}

void cPluginLoadepg::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

cOsdObject *cPluginLoadepg::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  
  // if no Free-tuners available donot loadepg!
  if( !AnyFreeTuners() ) 
   {
    Skins.Message(mtError, "No free Tuners, please stop recording and retry.", 3);
    return NULL;
   }

  FILE *File;
  char Buffer[1024];
  char *Line;
  char *Dummy;
  char *Channel1;
  char *Channel2;
  char *SourceName1;
  char *SourceName2;
  int SourceId1, Nid1, Tid1, Sid1, Rid1;
  int SourceId2, Nid2, Tid2, Sid2, Rid2;
  EpgProviders = 0;
  if( ConfigPlugin )
    asprintf( &Filename, "%s/%s", ConfigPlugin, "loadepg.conf" );
  else
    asprintf( &Filename, "%s/%s", cPlugin::ConfigDirectory(), "loadepg.conf" );
  File = fopen( Filename, "r" );
  if( File )
  {
    while( ( Line = fgets( Buffer, sizeof( Buffer ), File ) ) != NULL )
    {
        if( ( strlen( Buffer ) - 1 ) >= 0 && Buffer[ strlen( Buffer ) - 1 ] == '\n' )
            Buffer[ strlen( Buffer ) - 1 ] = 0;
        Line = compactspace( skipspace( stripspace( Line ) ) );
        if( ! isempty( Line ) )
        {
            if( sscanf( Line, "PROVIDER=%a[^:]:%a[^\n]", &EpgProviderTitle[EpgProviders], &EpgProviderValue1[EpgProviders] ) == 2 )
            {
                EpgProviderTitle[EpgProviders] = compactspace( skipspace( stripspace( EpgProviderTitle[EpgProviders] ) ) );
                EpgProviderValue1[EpgProviders] = compactspace( skipspace( stripspace( EpgProviderValue1[EpgProviders] ) ) );
                asprintf( &EpgProviderValue1[EpgProviders], "LOADEPG_CHANNEL:%s:0:0:0:0:1234:1234:1234:1234", EpgProviderValue1[EpgProviders] );
                Status.ProviderType[EpgProviders] = PROVIDER_TYPE_SATELLITE;
                //fprintf( stderr, "LoadEPG: |%s|%s|\n", EpgProviderTitle[EpgProviders], EpgProviderValue1[EpgProviders] );
                EpgProviders ++;
            }
            if( sscanf( Line, "FILE=%a[^:]:%a[^\n]", &EpgProviderTitle[EpgProviders], &EpgProviderValue1[EpgProviders] ) == 2 )
            {
                EpgProviderTitle[EpgProviders] = compactspace( skipspace( stripspace( EpgProviderTitle[EpgProviders] ) ) );
                EpgProviderValue1[EpgProviders] = compactspace( skipspace( stripspace( EpgProviderValue1[EpgProviders] ) ) );
                Status.ProviderType[EpgProviders] = PROVIDER_TYPE_FILE;
                //fprintf( stderr, "LoadEPG: |%s|%s|\n", EpgProviderTitle[EpgProviders], EpgProviderValue1[EpgProviders] );
                EpgProviders ++;
            }
            if( sscanf( Line, "SCRIPT=%a[^:]:%a[^:]:%a[^\n]", &EpgProviderTitle[EpgProviders], &EpgProviderValue1[EpgProviders], &EpgProviderValue2[EpgProviders] )  == 3 )
            {
                EpgProviderTitle[EpgProviders] = compactspace( skipspace( stripspace( EpgProviderTitle[EpgProviders] ) ) );
                EpgProviderValue1[EpgProviders] = compactspace( skipspace( stripspace( EpgProviderValue1[EpgProviders] ) ) );
                EpgProviderValue2[EpgProviders] = compactspace( skipspace( stripspace( EpgProviderValue2[EpgProviders] ) ) );
                Status.ProviderType[EpgProviders] = PROVIDER_TYPE_SCRIPT;
                //fprintf( stderr, "LoadEPG: |%s|%s|%s|\n", EpgProviderTitle[EpgProviders], EpgProviderValue1[EpgProviders], EpgProviderValue2[EpgProviders] );
                EpgProviders ++;
            }
        }
    }
    fclose( File );
  }
  else
  {
    fprintf( stderr, "LoadEPG: open file config \"%s\" failed\n", Filename );
    return NULL;
  }
  NumEpgEquivChannels = 0;

  if( LoadepgConfig.UseFileEquivalences == true )
  {
    if( ConfigPlugin )
      asprintf( &Filename, "%s/%s", ConfigPlugin, "loadepg.equiv" );
    else
      asprintf( &Filename, "%s/%s", cPlugin::ConfigDirectory(), "loadepg.equiv" );
    File = fopen( Filename, "r" );
    if( File )
    {
        while( ( Line = fgets( Buffer, sizeof( Buffer ), File ) ) != NULL )
        {
            if( ( strlen( Buffer ) - 1 ) >= 0 && Buffer[ strlen( Buffer ) - 1 ] == '\n' )
                Buffer[ strlen( Buffer ) - 1 ] = 0;
            Line = compactspace( skipspace( stripspace( Line ) ) );
            if( ! isempty( Line ) )
            {
                if( sscanf( Line, "%a[^ ] %a[^ ] %a[^ ]\n", &Channel1, &Channel2, &Dummy ) == 3 )
                {
                    if( sscanf( Channel1, "%a[^-] -%i-%i-%i-%i", &SourceName1, &Nid1, &Tid1, &Sid1, &Rid1 ) >= 4 )
                    {
                        SourceId1 = cSource::FromString( SourceName1 );
                        tChannelID ChannelId1 = tChannelID( SourceId1, Nid1, Tid1, Sid1, Rid1 );
                        cChannel *EpgChannel = Channels.GetByChannelID( ChannelId1, true );
                        if( sscanf( Channel2, "%a[^-] -%i-%i-%i-%i", &SourceName2, &Nid2, &Tid2, &Sid2, &Rid2 ) >= 4 )
                        {
                            SourceId2 = cSource::FromString( SourceName2 );
                            tChannelID ChannelId2 = tChannelID( SourceId2, Nid2, Tid2, Sid2, Rid2 );
                            cChannel *EpgEquivChannel = Channels.GetByChannelID( ChannelId2, true );
                            if( EpgChannel && EpgEquivChannel )
                            {
                                snprintf( EpgEquivChannels[NumEpgEquivChannels][0], sizeof( EpgEquivChannels[NumEpgEquivChannels][0] ), "%s-%i-%i-%i-%i %s", SourceName1, EpgChannel->Nid(), EpgChannel->Tid(), EpgChannel->Sid(), EpgChannel->Rid(), EpgChannel->Name());
                                snprintf( EpgEquivChannels[NumEpgEquivChannels][1], sizeof( EpgEquivChannels[NumEpgEquivChannels][0] ), "%s-%i-%i-%i-%i %s", SourceName1, EpgEquivChannel->Nid(), EpgEquivChannel->Tid(), EpgEquivChannel->Sid(), EpgEquivChannel->Rid(), EpgEquivChannel->Name());
                                NumEpgEquivChannels ++;
                            }
                            else
                                fprintf( stderr, "LoadEPG: error in loadepg.equiv for: %s\n", Line );
                        }
                    }
                }
            }
        }
        fclose( File );
    }
    else
    {
        fprintf( stderr, "LoadEPG: open file equivalences \"%s\" failed\n", Filename );
        return NULL;
    }
  }
  return new cLoadepgOsd();
}

cMenuSetupPage *cPluginLoadepg::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return new cLoadepgSetup();
}

bool cPluginLoadepg::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  if( !strcasecmp( Name, "UseFileEquivalences" ) )
    LoadepgConfig.UseFileEquivalences = atoi( Value );
  else if( !strcasecmp( Name, "DeviceNumber" ) )
    LoadepgConfig.DeviceNumber = atoi( Value );
  else
    return false;
  return true;
}

bool cPluginLoadepg::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginLoadepg::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  static const char *HelpPages[] = {
    "LOAD <PROVIDER_NUMBER>\n"
    "    Load epg data.",
    NULL
  };
  return HelpPages;
}

cString cPluginLoadepg::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  if( strcasecmp( Command, "LOAD" ) == 0 )
  {
    if( Status.Plugin == STATUS_NULL )
    {
      Status.Svdrp = atoi( Option );
      if( Status.Svdrp >= 1 && Status.Svdrp <= EPG_PROVIDERS )
      {
        cRemote::CallPlugin( "loadepg" );
        ReplyCode = 250;
        return cString( tr( "Wait load epg data" ) );
      }
      else
      {
        ReplyCode = 500;
        return cString( tr( "Invalid provider number" ) );
      }
    }
    else
    {
      ReplyCode = 500;
      return cString( tr( "The plugin is already active" ) );
    }
  }
  return NULL;
}

VDRPLUGINCREATOR(cPluginLoadepg); // Don't touch this!
