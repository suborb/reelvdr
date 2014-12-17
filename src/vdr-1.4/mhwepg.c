#include "mhwepg.h"

#include <time.h>

#include "osdbase.h"
#include "menu.h"

time_t LocalTime2Utc( time_t Time ) {
    struct tm TmTime;
    gmtime_r( &Time, &TmTime );
    TmTime.tm_isdst = -1;
    return mktime( &TmTime );
}

time_t UTC2LocalTime( time_t Time ) {
    return ( 2 * Time ) - LocalTime2Utc( Time );
}

void GetTimeOffset(time_t *YesterdayTime, int *Yesterday, time_t *YesterdayEpoch) {
    *YesterdayTime = UTC2LocalTime( time( NULL ) - (24*60*60) );
    struct tm DayTime;
    gmtime_r( YesterdayTime, &DayTime );
    *Yesterday = DayTime.tm_wday;
    DayTime.tm_hour = 0;
    DayTime.tm_min = 0;
    DayTime.tm_sec = 0;
    DayTime.tm_isdst = -1;
    *YesterdayEpoch = UTC2LocalTime( mktime( &DayTime ) );
}

time_t cMhwepgFilter::ProviderLocalTime2UTC( time_t T ) {
    if( T < TimeOfChange )
        return T - CurrentOffset;
    else
        return T - NextOffset;
}

unsigned char BcdToDec( unsigned char Data ) {
    return ( ( Data >> 4 ) & 0x0f ) * 10 + ( Data & 0x0f );
}

int TimeZoneCmp( LocalTimeOffsetType *ItemLocalTimeOffset, const char *CountryCode, int RegionId ) {
    return ItemLocalTimeOffset->CountryCode1 == CountryCode[0] &&
           ItemLocalTimeOffset->CountryCode2 == CountryCode[1] &&
           ItemLocalTimeOffset->CountryCode3 == CountryCode[2] &&
           ItemLocalTimeOffset->RegionId == RegionId ? 0 : 1;
}

unsigned int GetInt32( const unsigned char *pData ) {
    return ( pData[0] << 24 ) | ( pData[1] << 16 ) | ( pData[2] << 8 ) | ( pData[3] );
}

unsigned int GetInt16( const unsigned char *pData ) {
    return ( pData[0] << 8 ) | ( pData[1] );
}

//-- cMhwepgFilter -------------------------------------------------------

cMhwepgFilter::cMhwepgFilter( void ) {
    VdrEquivChannelId = -1;
    NumEpgChannels = 0;

    EpgDescription_.Text = NULL;
    memset( ListEpgEvents, 0, sizeof( ListEpgEvents ) );

    for (int i = 0; i < EPG_CHANNELS; i++)
        EpgChannels[i][0] = '\0';

    Set( 0x14, 0x73 );
    Set( 0xd2, 0x90 );
    Set( 0xd3, 0x90 );
    Set( 0xd3, 0x91 );
    Set( 0xd3, 0x92 );
    Set( 0x231, 0xc8 );
    Set( 0x234, 0xe6 );
    Set( 0x236, 0x96 );

    Add( 0x14, 0x73, true);
    Add( 0xd2, 0x90, true );
    Add( 0xd3, 0x90, true );
    Add( 0xd3, 0x91, true );
    Add( 0xd3, 0x92, true );
    Add( 0xd2, 0x90, true );
    Add( 0x231, 0xc8, true );
    Add( 0x234, 0xe6, true );
    Add( 0x236, 0x96, true );
}

cMhwepgFilter::~cMhwepgFilter( void ) {
    if (EpgDescription_.Text)
        free(EpgDescription_.Text);
}

void cMhwepgFilter::Process( u_short Pid, u_char Tid, const u_char *Data, int Length ) {
    //printf("XX pid: %#x tid: %#x\n", Pid, Tid);
    //fprintf( stderr, "LoadEPG: scan provider: %s\n", EpgProviderTitle[MenuItem] );
    //fprintf( stderr, "LoadEPG: found %i equivalences channels\n", NumEpgEquivChannels );

    if( Pid == 0x14 && Tid == 0x73 )
        GetEpgTime( Data, Length );
    else if( Pid == 0xd3 && Tid == 0x91 )
        GetEpgChannels( Data, Length );
    else if( Pid == 0xd3 && Tid == 0x92 ) // Get themes 
        GetEpgThemes( Data, Length );
    else if( Pid == 0xd2 && Tid == 0x90 )
        GetEpgEvents( Data, Length );
    else if( Pid == 0xd3 && Tid == 0x90 )
        GetEpgDescriptions( Data, Length );
    else if( Pid == 0x231 && Tid == 0xc8 ) {
        if( Data[3] == 0 )
            GetEpgChannels2( Data, Length );
        else if( Data[3] == 1 )
            GetEpgThemes2( Data, Length );
    } else if( Pid == 0x234 && Tid == 0xe6 ) //Get programs - mhwepg2 
        GetEpgEvents2( Data, Length );
    else if( Pid == 0x236 && Tid == 0x96 )
        GetEpgDescriptions2( Data, Length );
}

void cMhwepgFilter::GetEpgTime( const unsigned char *Data, int Length ) {
    int i;
    time_t DiffTime, UtcOffset, LocalTimeOffset, NextTimeOffset, YesterdayTime;
    unsigned char *Descriptors;
    EpgTimeType *Tot = ( EpgTimeType * ) Data;
    CurrentTime = time( NULL );
    DiffTime = MjdToEpochTime( Tot->UtcMjd ) + BcdTimeToSeconds( Tot->UtcTime ) - CurrentTime;
    UtcOffset = ( ( DiffTime + 1800 ) / (60*60) ) * (60*60);
    Descriptors = ( unsigned char * ) ( Tot + 1 );
    Length -= ( EPG_TIME_LENGTH + 4 );
    while( Length > 0 ) {
        DescriptorType *Descriptor = ( DescriptorType * ) Descriptors;
        if( Descriptor->Tag == 0x58 ) {
            LocalTimeOffsetType *ItemLocalTimeOffset = ( LocalTimeOffsetType * ) ( Descriptor + 1 );
            for( i = 0; i < ( Descriptor->Length / LOCAL_TIME_OFFSET_LENGTH ); i ++ ) {
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
                        if( ItemLocalTimeOffset->LocalTimeOffsetPolarity ) {
                            LocalTimeOffset = -LocalTimeOffset;
                            NextTimeOffset = -NextTimeOffset;
                        }
                        TimeOfChange = MjdToEpochTime( ItemLocalTimeOffset->TocMjd ) + BcdTimeToSeconds( ItemLocalTimeOffset->TocTime );
                        CurrentOffset = LocalTimeOffset + UtcOffset;
                        NextOffset = NextTimeOffset + UtcOffset;
                        GetTimeOffset(&YesterdayTime, &Yesterday, &YesterdayEpoch);
                }
                ItemLocalTimeOffset ++;
            }
        }
        Descriptors += Descriptor->Length + 2;
        Length -= Descriptor->Length + 2;
    }
    return;
}

void cMhwepgFilter::GetEpgChannels( const unsigned char *Data, int Length ) {
    if (Data == NULL || Length <= 22)
        return;
    int Id, Sid, Nid, Tid, Rid;
    EpgChannelType *EpgChannelData = ( EpgChannelType * ) ( Data + 4 );
    NumEpgChannels = ( Length - 4 ) / sizeof( EpgChannelType );
    bzero( EpgChannels, sizeof( EpgChannels ) );
    if( NumEpgChannels > EPG_CHANNELS ) {
        fprintf( stderr, "LoadEPG: error stream data, epg channels > %i\n", EPG_CHANNELS );
        return;
    }
    EpgSourceId = Source();
    //printf("XXX: NumEpgChannels: %i\n", NumEpgChannels);
    for( Id = 0; Id < NumEpgChannels; Id ++ ) {
        Sid = ( ( EpgChannelData->ChannelIdHigh << 8 )     | EpgChannelData->ChannelIdLow )     & 0xffff;
        Tid = ( ( EpgChannelData->TransponderIdHigh << 8 ) | EpgChannelData->TransponderIdLow ) & 0xffff;
        Nid = ( ( EpgChannelData->NetworkIdHigh << 8 )     | EpgChannelData->NetworkIdLow )     & 0xffff;
        Rid = 0;
        tChannelID EpgChannelId = tChannelID( Source(), Nid, Tid, Sid, Rid );
        cChannel *EpgChannel = Channels.GetByChannelID( EpgChannelId, true );
        if( EpgChannel ) {
            snprintf( EpgChannels[Id], sizeof( EpgChannels[Id] ), "%s-%i-%i-%i-%i %s", *cSource::ToString(Source()), EpgChannel->Nid(), EpgChannel->Tid(), EpgChannel->Sid(), EpgChannel->Rid(), EpgChannel->Name() );
            //printf("XX: ID: %i \"%s\" Channel: \"%s\" Nid: %i Tid: %i Sid: %i Rid: %i\n", Id, EpgChannels[Id], EpgChannel ? EpgChannel->Name() : "NULL", Nid, Tid, Sid, Rid);
        } else {
#if 0 // there's still some bug in here... 
            if (Setup.UpdateChannels >= 4 && Sid != 0 && Nid != 0 && Tid != 0) { // only if "UpdateChannels" >= "add new channels"
                char name[17];
                name[0] = name[16] = '\0';
                cChannel *channel = new cChannel;
                channel->CopyTransponderData(Channel());
                channel->SetId(Nid, Tid, Sid, Rid);

                if(strlen((const char*)&EpgChannelData->Name)) {
                    strncpy(name, (const char*)&EpgChannelData->Name, 16);
                    int ctr = 16;
                    while(ctr >= 0 && (name[ctr] == '\0' || name[ctr] == ' '))
                        name[ctr--] = '\0';
                }

                if (strlen(name) <= 16) { 
                    cCharSetConv conv = cCharSetConv("ISO-8859-1", "UTF-8");
                    char *s2 = MALLOC(char, strlen(name)*2);
                    s2[0] = '\0';
                    conv.Convert(name, s2, strlen(name)*2);

                    channel->SetName((const char*)s2 /*EpgChannelData->Name*/, "", "");

                    free(s2);

                    Channels.Add(channel);
                    Channels.ReNumber();
                    Channels.SetModified(true);
                    isyslog("new channel %d %s", channel->Number(), *channel->ToText());
                    //printf("ADDING: ID: %i \"%s\" \"%s\" Nid: %i Tid: %i Sid: %i Rid: %i Length: %i diff: %i\n", Id, name, EpgChannels[Id], Nid, Tid, Sid, Rid, Length, (const unsigned char*)EpgChannelData - Data);
                }
            }
            //printf("YY: ID: %i \"%s\" \"%s\" Nid: %i Tid: %i Sid: %i Rid: %i\n", Id, EpgChannelData->Name, EpgChannels[Id], Nid, Tid, Sid, Rid);
#endif
        }
        EpgChannelData ++;
    }
    //fprintf( stderr, "LoadEPG: found %i channels\n", NumEpgChannels );
    NumChannels = NumEpgChannels + NumEpgEquivChannels;
}

void cMhwepgFilter::GetEpgChannels2( const unsigned char *Data, int Length ) {
        int Id, Sid, Tid, Nid, Rid, ChannelId;
        if( Length > 119 ) {
            NumEpgChannels = Data[119];
            Id = 120 + ( 8 * NumEpgChannels );
            if( Length > Id ) {
                for( ChannelId = 0; ChannelId < NumEpgChannels; ChannelId ++ ) {
                    Id += ( Data[Id] & 0x0f ) + 1;
                    if( Length < Id )
                        return;
                }
            } else
                return;
        } else
            return;
        NumEpgChannels = Data[119];
        bzero( EpgChannels, sizeof( EpgChannels ) );
        if( NumEpgChannels > EPG_CHANNELS ) {
            fprintf( stderr, "LoadEPG: error stream data, epg channels > %i\n", EPG_CHANNELS );
            return;
        }
        EpgSourceId = Source();
        ChannelId = 1;
        while( ChannelId <= Channels.MaxNumber() ) {
            cChannel *EpgChannel = Channels.GetByNumber( ChannelId, 1 );
            if( EpgChannel ) {
                EpgChannelType2 *EpgChannelData = ( EpgChannelType2 * ) ( Data + 120 );
                for( Id = 0; Id < NumEpgChannels; Id ++ ) {
                    Sid = ( ( EpgChannelData->ChannelIdHigh << 8 ) | EpgChannelData->ChannelIdLow ) & 0xffff;
                    Tid = ( ( EpgChannelData->TransponderIdHigh << 8 ) | EpgChannelData->TransponderIdLow ) & 0xffff;
                    Nid = ( ( EpgChannelData->NetworkIdHigh << 8 ) | EpgChannelData->NetworkIdLow ) & 0xffff;
                    Rid = 0;
                    //if (strlen(EpgChannels[Id]))
                        //printf("YY: ID: %i \"%s\" Nid: %i Tid: %i Sid: %i Rid: %i\n", Id, EpgChannels[Id], Nid, Tid, Sid, Rid);
                    if( EpgChannel->Sid() == Sid && EpgChannel->Tid() == Tid && Nid == EpgChannel->Nid() /* && EpgChannel->Rid() == Rid */ ) {
                        snprintf( EpgChannels[Id], sizeof( EpgChannels[Id] ), "%s-%i-%i-%i-%i %s",  *cSource::ToString(Source()) /*EpgSourceName*/, EpgChannel->Nid(), EpgChannel->Tid(), EpgChannel->Sid(), EpgChannel->Rid(), EpgChannel->Name() );
                    break;
                }
                EpgChannelData ++;
            }
        }
        ChannelId ++;
    }
    //fprintf( stderr, "LoadEPG: found %i channels\n", NumEpgChannels );
    NumChannels += NumEpgEquivChannels;
}

void cMhwepgFilter::GetEpgThemes( const unsigned char *Data, int Length ) {
        int Id, Theme, Offset;
        const unsigned char *EpgThemesIndex = Data + 3;
        typedef unsigned char EpgThemeType[15];
        EpgThemeType *EpgThemeData = ( EpgThemeType * ) ( Data + 19 );
        Theme = 0;
        Offset = 0;
        NumEpgThemes = ( Length - 19 ) / 15;
        if( NumEpgThemes > EPG_THEMES ) {
            fprintf( stderr, "LoadEPG: error stream data, epg themes > %i\n", EPG_THEMES );
            return;
        }
        for( Id = 0; Id < NumEpgThemes; Id ++ ) {
            if( EpgThemesIndex[Theme] == Id ) {
                Offset = ( Offset + 15 ) & 0xf0;
                Theme ++;
            }
            memcpy( EpgThemes[Offset], EpgThemeData, 15 );
            EpgThemes[Offset][15] = 0;
            CleanString( EpgThemes[Offset] );
            Offset ++;
            EpgThemeData ++;
        }
        //fprintf( stderr, "LoadEPG: found %i themes\n", NumEpgThemes );
}

void cMhwepgFilter::GetEpgThemes2( const unsigned char *Data, int Length ) {
        int ThemeId, SubThemeId, Pos1, Pos2, PosThemeName, LenThemeName;
        unsigned char Count, StrLen;
        if( Length > 4 ) {
            for( ThemeId = 0; ThemeId < Data[4]; ThemeId ++ ) {
                Pos1 = ( Data[5+ThemeId*2] << 8 ) + Data[6+ThemeId*2];
                Pos1 += 3;
                if( Length > Pos1 ) {
                    Count = Data[Pos1] & 0x3f;
                    for( SubThemeId = 0; SubThemeId <= Count; SubThemeId ++ ) {
                        Pos2 = ( Data[Pos1+1+SubThemeId*2] << 8 ) + Data[Pos1+2+SubThemeId*2];
                        Pos2 += 3;
                        if( Length > Pos2 ) {
                            StrLen = Data[Pos2] & 0x1f;
                            if( ( StrLen + Pos2 ) > Length )
                                return;
                        } else
                            return;
                    }
                } else
                    return;
        }
    }
    NumEpgThemes = 0;
    bzero( EpgThemes, sizeof( EpgThemes ) );
    bzero( EpgFirstThemes, sizeof( EpgFirstThemes ) );
    for( ThemeId = 0; ThemeId < Data[4]; ThemeId ++ ) {
        Pos1 = ( Data[5+ThemeId*2] << 8 ) + Data[6+ThemeId*2];
        Pos1 += 3;
        Count = Data[Pos1] & 0x3f;
        PosThemeName = 0;
        LenThemeName = 0;
        for( SubThemeId = 0; SubThemeId <= Count; SubThemeId ++ ) {
            Pos2 = ( Data[Pos1+1+SubThemeId*2] << 8 ) + Data[Pos1+2+SubThemeId*2];
            Pos2 += 3;
            StrLen = Data[Pos2] & 0x1f;
            if( SubThemeId == 0 ) {
                PosThemeName = Pos2 + 1;
                LenThemeName = StrLen;
                memcpy( &EpgThemes[NumEpgThemes], Data + PosThemeName, LenThemeName );
                EpgThemes[NumEpgThemes][LenThemeName] = 0;
                EpgFirstThemes[ThemeId] = NumEpgThemes;
            } else {
                if( ( LenThemeName + StrLen + 1 ) < 64 ) {
                    memcpy( &EpgThemes[NumEpgThemes], Data + PosThemeName, LenThemeName );
                    EpgThemes[NumEpgThemes][LenThemeName] = ' ';
                    memcpy( &EpgThemes[NumEpgThemes][LenThemeName+1], Data + Pos2 + 1, StrLen );
                    EpgThemes[NumEpgThemes][LenThemeName+1+StrLen] = 0;
                }
            }
        NumEpgThemes ++;
        }
    }
    if( NumEpgThemes > EPG_THEMES ) {
        fprintf( stderr, "LoadEPG: error stream data, epg themes > %i\n", EPG_THEMES );
        return;
    }
    //fprintf( stderr, "LoadEPG: found %i themes\n", NumEpgThemes );
}

void cMhwepgFilter::GetEpgEvents( const unsigned char *Data, int Length ) {
    EpgEventData = ( EpgEventType * ) Data;

    if( Length == EPG_PROGRAM_LENGTH ) {
        if( EpgEventData->Day == 7 )
            EpgEventData->Day = 0;
        if( EpgEventData->Hours > 15 )
            EpgEventData->Hours = EpgEventData->Hours - 4;
        else if( EpgEventData->Hours > 7 )
            EpgEventData->Hours = EpgEventData->Hours - 2;

        if( EpgEventData->ChannelId != 0xff ) {
            EpgEvent_.ChannelId = EpgEventData->ChannelId;
            EpgEvent_.ThemeId = EpgEventData->ThemeId;
            EpgEvent_.Time = ( ( EpgEventData->Day - Yesterday ) * (24*60*60) ) + ( EpgEventData->Hours * (60*60) ) + ( EpgEventData->Minutes * 60 );
            if( EpgEvent_.Time < ( 6*60*60 ) )
                EpgEvent_.Time += 7 * (24*60*60);
            EpgEvent_.Time = EpgEvent_.Time + YesterdayEpoch;
            EpgEvent_.Time = ProviderLocalTime2UTC( EpgEvent_.Time );
            EpgEvent_.SummaryAvailable = EpgEventData->SummaryAvailable;
            EpgEvent_.Duration = HILO16( EpgEventData->Duration ) * 60;

            cCharSetConv conv = cCharSetConv("ISO-8859-1", "UTF-8");
            char *s2 = MALLOC(char, strlen((const char*)&EpgEventData->Title)*2);
            s2[0] = '\0';
            conv.Convert((const char*)&EpgEventData->Title, s2, strlen((const char*)&EpgEventData->Title)*2);

            memcpy( EpgEvent_.Title, s2 /*&EpgEventData->Title*/, 23 );

            free(s2);

            EpgEvent_.Title[23] = '\0';
            CleanString( EpgEvent_.Title );
            EpgEvent_.PpvId = HILO32( EpgEventData->PpvId );
            EpgEvent_.ProgramId = HILO32( EpgEventData->ProgramId );
            ListEpgEvents[EpgEvent_.ChannelId] ++;

            cSchedulesLock SchedulesLock;
            const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);
            if (Schedules) {
                tChannelID ChanID =  tChannelID::FromString(EpgChannels[EpgEventData->ChannelId-1]);
                cChannel *Chan = Channels.GetByChannelID( ChanID, true );
                if (Chan) {
                    cSchedule *Schedule = (cSchedule *)Schedules->GetSchedule(Chan);
                    if (Schedule && EpgEvent_.ProgramId) {
                        cEvent *event = NULL;
                        event = (cEvent *)Schedule->GetEvent(EpgEvent_.ProgramId, EpgEvent_.Time);
                        if (!event)
                            event = (cEvent *)Schedule->GetEvent(EpgEvent_.ProgramId);

                        bool newEvent = false;
                        if (!event) {
                            event = new cEvent(EpgEvent_.ProgramId);
                            if (event)
                                newEvent = true;
                        }

                        if (event) {
                            //event->SetTableID(Tid);
                            if (EpgEvent_.Time) {
                                //printf("XXXX: Time: %i\n", EpgEvents->Time);
                                event->SetStartTime(EpgEvent_.Time);
                                event->SetDuration(EpgEvent_.Duration);
                            }
                            if(strlen((const char*)EpgEvent_.Title))
                                event->SetTitle((const char*)EpgEvent_.Title);
                            if (newEvent)
                                Schedule->AddEvent(event);
                            //printf("XXXX: Chan: \"%s\" Title: \"%s\" time: %lu duration: %i\n", Chan->Name(), EpgEvent_.Title, EpgEvent_.Time, EpgEvent_.Duration);
                            Schedule->Sort();
                            Schedules->SetModified(Schedule);
                        }
                    }
                }
            }
        }
    }
}

void cMhwepgFilter::GetEpgEvents2( const unsigned char *Data, int Length ) {
    int Pos, Len;
    unsigned int Section;
    bool Check;
    if( Data[Length-1] != 0xff )
        return;
    Pos = 18;
    while( Pos < Length ) {
        Check = false;
        Pos += 7;
        if( Pos < Length ) {
            Pos += 3;
            if( Pos < Length ) {
                if( Data[Pos] > 0xc0 ) {
                    Pos += ( Data[Pos] - 0xc0 );
                    Pos += 4;
                    if( Pos < Length ) {
                        if( Data[Pos] == 0xff ) {
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
    if( SectionInitial == 0 ) {
        SectionInitial = Section;
        memset( ListEpgEvents, 0, sizeof( ListEpgEvents ) );
    } else if( Section == SectionInitial ) {
        SectionInitial = 0;
        return;
    }
    Pos = 18;
    while( Pos < Length ) {
        EpgEvent_.ChannelId = Data[Pos];
        EpgEvent_.Time = ( GetInt16( Data + Pos + 3 ) - 40587 ) * (60*60*24) + ( ( ( Data[Pos+5] & 0xf0 ) >> 4 ) * 10 + ( Data[Pos+5] & 0x0f ) ) * (60*60) + ( ( ( Data[Pos+6] & 0xf0 ) >> 4 ) * 10 + ( Data[Pos+6] & 0x0f ) ) * 60;
        EpgEvent_.Duration = ( GetInt16( Data + Pos + 8 ) >> 4 ) * 60;
        Len = Data[Pos+10] & 0x3f;
        memcpy( EpgEvent_.Title, Data + Pos + 11, Len );
        EpgEvent_.Title[Len] = 0;
        CleanString( EpgEvent_.Title );
        Pos += Len + 11;
        EpgEvent_.ThemeId = EpgFirstThemes[( Data[7] & 0x3f )] + ( Data[Pos] & 0x3f );
        EpgEvent_.ProgramId = GetInt16( Data + Pos + 1 );
        EpgEvent_.SummaryAvailable = 1;
        EpgEvent_.PpvId = 0;
        ListEpgEvents[EpgEvent_.ChannelId+1] ++;
        Pos += 4;
        //printf("chanid: %i title: \"%s\"\n", EpgEvent_.ChannelId, EpgEvent_.Title);

        if (strlen((const char*)EpgEvent_.Title)) {
            //printf("%i ---- %s\n", __LINE__, __PRETTY_FUNCTION__);
            cCharSetConv conv = cCharSetConv("ISO-8859-9", "UTF-8");
            char *s2 = MALLOC(char, strlen((const char*)&EpgEvent_.Title)*2);
            s2[0] = '\0';
            conv.Convert((const char*)&EpgEvent_.Title, s2, strlen((const char*)&EpgEvent_.Title)*2);

            memcpy( EpgEvent_.Title, s2, 23 );
            CleanString( EpgEvent_.Title );

            free(s2);
        }

        cSchedulesLock SchedulesLock;
        const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);

        if (Schedules && strlen(EpgChannels[EpgEvent_.ChannelId])) {
            tChannelID ChanID =  tChannelID::FromString(EpgChannels[EpgEvent_.ChannelId]);

            // Do not load events if channel belongs to blacklist defined in noepgchannels.conf
            if (!Channels.isEPGAllowed(ChanID))
            {
                return;
            }

            cChannel *Chan = Channels.GetByChannelID( ChanID, true );
            if (Chan) {
                cSchedule *Schedule = (cSchedule *)Schedules->GetSchedule(Chan);
                if (Schedule) {
                    cEvent *event = NULL;
                    event = (cEvent *)Schedule->GetEvent(EpgEvent_.ProgramId, EpgEvent_.Time);
                    if (!event)
                        event = (cEvent *)Schedule->GetEvent(EpgEvent_.ProgramId);

                    bool newEvent = false;
                    if (!event && EpgEvent_.ProgramId) {
                        event = new cEvent(EpgEvent_.ProgramId);
                        if (event)
                            newEvent = true;
                    }
                        
                    if (event) {
                        //event->SetTableID(Tid);
                        if (EpgEvent_.Time) {
                            //printf("XXXX: Time: %i\n", EpgEvents->Time);
                            event->SetStartTime(EpgEvent_.Time);
                            event->SetDuration(EpgEvent_.Duration);
                        }
                        if(strlen((const char*)EpgEvent_.Title))
                            event->SetTitle((const char*)EpgEvent_.Title);
                        if (newEvent)
                            Schedule->AddEvent(event);
                        //printf("XXXX: Chan: \"%s\" Title: \"%s\" time: %lu duration: %i\n", Chan->Name(), EpgEvent_.Title, EpgEvent_.Time, EpgEvent_.Duration);
                        Schedule->Sort();
                        Schedules->SetModified(Schedule);
                    }
                }
            }
        }
    }
}

void cMhwepgFilter::GetEpgDescriptions( const unsigned char *Data, int Length )
{
    EpgDescriptionData = ( EpgDescriptionType * ) Data;
    if( EpgDescriptionData->Byte7 == 0xff && EpgDescriptionData->Byte8 == 0xff && EpgDescriptionData->Byte9 == 0xff && EpgDescriptionData->NumReplays < EPG_SUMMARY_NUM_REPLAYS && Length > EPG_SUMMARY_LENGTH ) {
        EpgDescription_.ProgramId = HILO32( EpgDescriptionData->ProgramId );
        TextOffset = EPG_SUMMARY_LENGTH + ( EpgDescriptionData->NumReplays * EPG_SUMMARY_REPLAY_LENGTH );
        TextLength = Length + 1 - TextOffset;
        if (EpgDescription_.Text)
            free(EpgDescription_.Text);
        EpgDescription_.Text = ( unsigned char * ) malloc( TextLength );
        memcpy( EpgDescription_.Text, &Data[TextOffset], TextLength );
        EpgDescription_.Text[TextLength-1] = '\0';

        CleanString( EpgDescription_.Text );

        cCharSetConv conv = cCharSetConv("ISO-8859-1", "UTF-8");
        unsigned char *s2 = MALLOC(unsigned char, strlen((const char*)EpgDescription_.Text )*2);
        s2[0] = '\0';
        conv.Convert((const char*)EpgDescription_.Text , (char*)s2, strlen((const char*)EpgDescription_.Text )*2);
        free( EpgDescription_.Text );             
        EpgDescription_.Text = s2;

        cSchedulesLock SchedulesLock;
        const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);

        if (Schedules) {
            for (unsigned int j = 0; j < EPG_CHANNELS; j++) {
                tChannelID ChanID = tChannelID::FromString(EpgChannels[j]);
                cChannel *Chan = Channels.GetByChannelID( ChanID, true );
                if (Chan) {
                    cSchedule *Schedule = (cSchedule *)Schedules->GetSchedule(Chan);
                    if (Schedule) {
                        cEvent *event = NULL;
                        bool newEvent = false;
                        event = (cEvent *)Schedule->GetEvent(EpgDescription_.ProgramId);
                        if (!event) {
                            event = new cEvent(EpgDescription_.ProgramId);
                            newEvent = true;
                        }
                        if (event) {
                            if(EpgDescription_.Text && strlen((const char*)EpgDescription_.Text) && (!event->Description() || strlen(event->Description()) == 0)) {
                                event->SetDescription((const char*)EpgDescription_.Text);
                                if (newEvent)
                                    Schedule->AddEvent(event);
                                Schedule->Sort();
                                Schedules->SetModified(Schedule);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

void cMhwepgFilter::GetEpgDescriptions2( const unsigned char *Data, int Length ) {
    int Pos, Len, Loop, LenLine;
    unsigned int Section;
    bool Check;
    Check = false;
    if( Length > 18 ) {
        Loop = Data[12];
        Pos = 13 + Loop;
        if( Length > Pos ) {
            Loop = Data[Pos] & 0x0f;
            Pos += 1;
            if( Length > Pos ) {
                Len = 0;
                for( ; Loop > 0; Loop -- ) {
                    if( Length > ( Pos + Len ) ) {
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
        SectionInitial = Section;
    else if( Section == SectionInitial )
        return;
    EpgDescription_.ProgramId = GetInt16( Data2 + 3 );
    Loop = Data2[12];
    Pos = 13 + Loop;
    Loop = Data2[Pos] & 0x0f;
    Pos += 1;
    Len = 0;
    for( ; Loop > 0; Loop -- ) {
        LenLine = Data2[Pos+Len];
        Data2[Pos+Len] = '|';
        Len += LenLine + 1;
    }
    if( Len > 0 )
        Data2[Pos+Len] = 0;
    else
        Data2[Pos+1] = 0;
    if (EpgDescription_.Text)
        free(EpgDescription_.Text);
    EpgDescription_.Text = ( unsigned char * ) malloc( Len + 1 );
    memcpy( EpgDescription_.Text, Data2 + Pos + 1, Len + 1 );
    EpgDescription_.Text[Len] = 0;
    CleanString( EpgDescription_.Text );
}

void cMhwepgFilter::CleanString( unsigned char *String ) {
    unsigned char *Src = String;
    unsigned char *Dst = String;
    int Spaces = 0;
    while( *Src ) {
        if( *Src < 0x20 || ( *Src > 0x7e && *Src < 0xa0 ) )
            *Src = 0x20;
        else if( *Src == 0x20 )
            Spaces ++;
        else
            Spaces = 0;
        if( Spaces < 2 ) {
            *Dst = *Src;
            *Dst ++;
        }
        *Src ++;
    }
    if( Spaces > 0 )
      Dst --;
    *Dst = 0;
}

void cMhwepgFilter::SetStatus(bool On)
{
    NumChannels = 0;
    for (int i = 0; i < EPG_CHANNELS; i++)
        EpgChannels[i][0] = '\0';
    cFilter::SetStatus(On);
}

