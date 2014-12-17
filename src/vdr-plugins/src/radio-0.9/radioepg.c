/*
 * radioepg.c - part of radio.c, a plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "radioepg.h"


// Premiere-Radio
int epg_premiere(const char *epgtitle, const char *epgdescr, time_t epgstart, time_t epgend) 
{
    int i;
    const char *p;
    char artist[RT_MEL], titel[RT_MEL], album[RT_MEL], jahr[RT_MEL];
    struct tm tm_store;

    // EPG not actual
    if (epgtitle == NULL || epgdescr == NULL) {
        dsyslog("radio: epg_premiere called, no title or description, nextCall in 5 s\n");
        return 5;
        }

    // Interpret
    p = strstr(epgtitle, PEPG_ARTIST);
    if (p != NULL) {
        p += strlen(PEPG_ARTIST);
        strcpy(artist, p);
        // Titel
        p = strstr(epgdescr, PEPG_TITEL);
        if (p != NULL) {
            p += strlen(PEPG_TITEL);
            i = 1;
            while (*++p!='\n' && *p!='\0' && i<(RT_MEL-1)) ++i;
            memcpy(titel, p-i, i);
            titel[i] = '\0';
            }
        else
            sprintf(titel, "---");
        // Album
        p = strstr(epgdescr, PEPG_ALBUM);
        if (p != NULL) {
            p += strlen(PEPG_ALBUM);
            i = 1;
            while (*++p!='\n' && *p!='\0' && i<(RT_MEL-1)) ++i;
            memcpy(album, p-i, i);
            album[i] = '\0';
            }
        else
            sprintf(album, "---");
        }
    else {          // P-Klassik ?
        // Komponist
        p = strstr(epgtitle, PEPG_KOMP);
        if (p != NULL) {
            p += strlen(PEPG_KOMP);
            strcpy(artist, p);
            // und Interpret
            p = strstr(epgdescr, PEPG_ARTIST);
            if (p != NULL) {
                strcpy(album, artist);      // Album = Komponist
                p += strlen(PEPG_ARTIST);
                i = 1;
                while (*++p!='\n' && *p!='\0' && i<(RT_MEL-1)) ++i;
                memcpy(artist, p-i, i);
                artist[i] = '\0';
                }
            else
                sprintf(album, "---");
            // Werk
            p = strstr(epgdescr, PEPG_WERK);
            if (p != NULL) {
                p += strlen(PEPG_WERK);
                i = 1;
                while (*++p!='\n' && *p!='\0' && i<(RT_MEL-1)) ++i;
                memcpy(titel, p-i, i);
                titel[i] = '\0';
                }
            else
                sprintf(titel, "---");
            }
        else {
            sprintf(artist, "---");
            sprintf(titel, "---");
            }
        }
    
    // Jahr
    p = strstr(epgdescr, PEPG_JAHR);
    if (p != NULL) {
        p += strlen(PEPG_JAHR);
        i = 1;
        while (*++p!='\n' && *p!='\0' && i<(RT_MEL-1)) ++i;
        memcpy(jahr, p-i, i);
        jahr[i] = '\0';
        }
    else
        sprintf(jahr, "----");
    // quick hack for epg-blabla
    if (strlen(jahr) > 24)
        sprintf(jahr, "----");
    
    int nextevent = epgend - time(NULL);

    if (strcmp(RTP_Artist, artist)!=0 || strcmp(RTP_Title, titel)!=0) {
        snprintf(RTP_Artist, RT_MEL, artist);
        snprintf(RTP_Title, RT_MEL, titel);
        RTP_Starttime = epgstart;
        struct tm *ts = localtime_r(&RTP_Starttime, &tm_store);
        if (++rtp_content.rt_Index >= 2*MAX_RTPC) 
            rtp_content.rt_Index = 0;
        asprintf(&rtp_content.radiotext[rtp_content.rt_Index], "%02d:%02d  %s: %s",  ts->tm_hour, ts->tm_min, RTP_Artist, RTP_Title);
        snprintf(RT_Text[RT_Index], RT_MEL, "%s", rtp_content.radiotext[rtp_content.rt_Index]);
        RT_Index +=1; if (RT_Index >= S_RtOsdRows) RT_Index = 0;
        if (strcmp(album, "---") != 0) {
            if (++rtp_content.rt_Index >= 2*MAX_RTPC) 
                rtp_content.rt_Index = 0;
            asprintf(&rtp_content.radiotext[rtp_content.rt_Index], "   ... %s (%s)", album, jahr);
            snprintf(RT_Text[RT_Index], RT_MEL, "%s", rtp_content.radiotext[rtp_content.rt_Index]);
            RT_Index +=1; if (RT_Index >= S_RtOsdRows) RT_Index = 0;
            }
        if (++rtp_content.item_Index >= MAX_RTPC)
            rtp_content.item_Index = 0;
        if (rtp_content.item_Index >= 0) {
            rtp_content.item_Start[rtp_content.item_Index] = RTP_Starttime;
            asprintf(&rtp_content.item_Artist[rtp_content.item_Index], "%s", RTP_Artist);
            asprintf(&rtp_content.item_Title[rtp_content.item_Index], "%s", RTP_Title);
            }
        RT_MsgShow = RT_PlusShow = true;
        (RT_Info > 0) ? : RT_Info = 2;
        radioStatusMsg();
        if ((S_Verbose & 0x0f) >= 1)
            printf("Premiereradio: %s / %s\n", RTP_Artist, RTP_Title);
        }
    
    dsyslog("radio: epg_premiere called, nextEvent in %ds\n", nextevent); 
    return (nextevent < 0) ? 10 : nextevent+2; 
}


// Kabel Deutschland Radio
int epg_kdg(const char *epgdescr, time_t epgstart, time_t epgend) 
{
    int i;
    const char *p;
    char artist[RT_MEL], titel[RT_MEL], album[RT_MEL], komp[RT_MEL];
    struct tm tm_store;

    // EPG not actual
    if (epgdescr == NULL) {
        dsyslog("radio: epg_kdg called, no description, nextCall in 5s\n"); 
        return 5;
        }

    // Titel
    p = strstr(epgdescr, KDEPG_TITEL);
    if (p != NULL) {
        p += strlen(KDEPG_TITEL);
        i = 1;
        while (*++p!='\n' && *p!='\0' && i<(RT_MEL-1)) ++i;
        memcpy(titel, p-i, i);
        titel[i] = '\0';
        }
    else
        sprintf(titel, "---");
    // Interpret
    p = strstr(epgdescr, KDEPG_ARTIST);
    if (p != NULL) {
        p += strlen(KDEPG_ARTIST);
        i = 1;
        while (*++p!='\n' && *p!='\0' && i<(RT_MEL-1)) ++i;
        memcpy(artist, p-i, i);
        artist[i] = '\0';
        }
    else
        sprintf(artist, "---");
    // Album
    p = strstr(epgdescr, KDEPG_ALBUM);
    if (p != NULL) {
        p += strlen(KDEPG_ALBUM);
        i = 1;
        while (*++p!='\n' && *p!='\0' && i<(RT_MEL-1)) ++i;
        memcpy(album, p-i, i);
        album[i] = '\0';
        }
    else
        sprintf(album, "---");
    // Komponist
    p = strstr(epgdescr, KDEPG_KOMP);
    if (p != NULL) {
        p += strlen(KDEPG_KOMP);
        i = 1;
        while (*++p!='\n' && *p!='\0' && i<(RT_MEL-1)) ++i;
        memcpy(komp, p-i, i);
        komp[i] = '\0';
        }
    else
        sprintf(komp, "---");
    
    int nextevent = epgend - time(NULL);

    if (strcmp(RTP_Artist, artist)!=0 || strcmp(RTP_Title, titel)!=0) {
        snprintf(RTP_Artist, RT_MEL, artist);
        snprintf(RTP_Title, RT_MEL, titel);
        RTP_Starttime = epgstart;
        struct tm *ts = localtime_r(&RTP_Starttime, &tm_store);
        if (++rtp_content.rt_Index >= 2*MAX_RTPC) 
            rtp_content.rt_Index = 0;
        asprintf(&rtp_content.radiotext[rtp_content.rt_Index], "%02d:%02d  %s: %s",  ts->tm_hour, ts->tm_min, RTP_Artist, RTP_Title);
        snprintf(RT_Text[RT_Index], RT_MEL, "%s", rtp_content.radiotext[rtp_content.rt_Index]);
        RT_Index +=1; if (RT_Index >= S_RtOsdRows) RT_Index = 0;
        if (strcmp(album, "---") != 0) {
            if (++rtp_content.rt_Index >= 2*MAX_RTPC) 
                rtp_content.rt_Index = 0;
            asprintf(&rtp_content.radiotext[rtp_content.rt_Index], "   ... %s (%s)", album, komp);
            snprintf(RT_Text[RT_Index], RT_MEL, "%s", rtp_content.radiotext[rtp_content.rt_Index]);
            RT_Index +=1; if (RT_Index >= S_RtOsdRows) RT_Index = 0;
            }
        if (++rtp_content.item_Index >= MAX_RTPC)
            rtp_content.item_Index = 0;
        if (rtp_content.item_Index >= 0) {
            rtp_content.item_Start[rtp_content.item_Index] = RTP_Starttime;
            asprintf(&rtp_content.item_Artist[rtp_content.item_Index], "%s", RTP_Artist);
            asprintf(&rtp_content.item_Title[rtp_content.item_Index], "%s", RTP_Title);
            }
        RT_MsgShow = RT_PlusShow = true;
        (RT_Info > 0) ? : RT_Info = 2;
        radioStatusMsg();
        if ((S_Verbose & 0x0f) >= 1)
            printf("KDG-Radio: %s / %s\n", RTP_Artist, RTP_Title);
        }
    
    dsyslog("radio: epg_kdg called, nextEvent in %ds\n", nextevent); 
    return (nextevent < 0) ? 10 : nextevent+2; 
}


// Unity Media - Music Choice, Kabel
int epg_unitymedia(const char *epgtitle, const char *epgdescr, time_t epgstart, time_t epgend) 
{
    int i;
    const char *p1, *p2;
    char titel[2*RT_MEL], artist[2*RT_MEL];
    struct tm tm_store;

    // EPG not actual
    if (epgtitle == NULL || epgdescr == NULL) {
        dsyslog("radio: epg_unitymedia called, no title or description, nextCall in 5s\n"); 
        return 5;
        }
        
    // Title
    strcpy(titel, epgtitle);
    
    // Interpet+Album, no delimiter between title, artist + album :-(
    p1 = strstr(epgdescr, titel);
    if (p1 != NULL) {
        p1 += strlen(titel) + 1;
        p2 = strstr(epgdescr, UMEPG_SUFFIX);
        if ((p2-p1) > 2) {
            p2 -= 2;
            i = 1;
            while (++p1<p2 && *p1!='\n' && *p1!='\0' && i<(2*RT_MEL-1)) ++i;
            memcpy(artist, p1-i, i);
            artist[i]= '\0';
            }
        else
            sprintf(artist, "---");
        }
    else
        sprintf(artist, "---");
        
    
    int nextevent = epgend - time(NULL);

    if (strcmp(RTP_Artist, artist)!=0 || strcmp(RTP_Title, titel)!=0) {
        snprintf(RTP_Artist, RT_MEL, artist);
        snprintf(RTP_Title, RT_MEL, titel);
        RTP_Starttime = epgstart;
        struct tm *ts = localtime_r(&RTP_Starttime, &tm_store);
        if (++rtp_content.rt_Index >= 2*MAX_RTPC) 
            rtp_content.rt_Index = 0;
        asprintf(&rtp_content.radiotext[rtp_content.rt_Index], "%02d:%02d  %s von %s",  ts->tm_hour, ts->tm_min, RTP_Title, RTP_Artist);
        snprintf(RT_Text[RT_Index], RT_MEL, "%s", rtp_content.radiotext[rtp_content.rt_Index]);
        RT_Index +=1; if (RT_Index >= S_RtOsdRows) RT_Index = 0;
        if (++rtp_content.item_Index >= MAX_RTPC)
            rtp_content.item_Index = 0;
        if (rtp_content.item_Index >= 0) {
            rtp_content.item_Start[rtp_content.item_Index] = RTP_Starttime;
            asprintf(&rtp_content.item_Artist[rtp_content.item_Index], "%s", RTP_Artist);
            asprintf(&rtp_content.item_Title[rtp_content.item_Index], "%s", RTP_Title);
            }
        RT_MsgShow = RT_PlusShow = true;
        (RT_Info > 0) ? : RT_Info = 2;
        radioStatusMsg();
        if ((S_Verbose & 0x0f) >= 1)
            printf("UnityMedia: %s / %s\n", RTP_Artist, RTP_Title);
        }
    
    dsyslog("radio: epg_unitymedia called, nextEvent in %ds\n", nextevent); 
    return (nextevent < 0) ? 10 : nextevent+2; 
}

// end
