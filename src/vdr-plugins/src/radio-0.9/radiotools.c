/*
 * radiotools.c - part of radio.c, a plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */
 
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <sys/stat.h>
#include <vdr/tools.h>
#include "radiotools.h"
#include "radioaudio.h"


/* ----------------------------------------------------------------------------------------------------------- */

bool file_exists(const char *filename)
{
    struct stat file_stat;

    return ((stat(filename, &file_stat) == 0) ? true : false);
}

bool enforce_directory(const char *path)
{
    struct stat sbuf;

    if (stat(path, &sbuf) != 0) {
        dsyslog("radio: creating directory %s", path);
        if (mkdir(path, ACCESSPERMS)) {
            esyslog("radio: ERROR failed to create directory %s", path);
            return false;
            }
        }
    else {
        if (!S_ISDIR(sbuf.st_mode)) {
            esyslog("radio: ERROR failed to create directory %s: file exists but is not a directory", path);
            return false;
            }
        }

    return true;
}

/* ----------------------------------------------------------------------------------------------------------- */

#define crc_timetest 0
#if crc_timetest
    #include <time.h>
    #include <sys/time.h>
#endif

unsigned short crc16_ccitt(unsigned char *daten, int len, bool skipfirst)
{
#if crc_timetest
    struct timeval t;
    unsigned long long tstart = 0;
    if (gettimeofday(&t, NULL) == 0)
        tstart = t.tv_sec*1000000 + t.tv_usec;
#endif

    // CRC16-CCITT: x^16 + x^12 + x^5 + 1
    // with start 0xffff and result invers
    register unsigned short crc = 0xffff;

    if (skipfirst) *daten++;
    while (len--) {
    crc = (crc >> 8) | (crc << 8);
    crc ^= *daten++;
    crc ^= (crc & 0xff) >> 4;
    crc ^= (crc << 8) << 4;
    crc ^= ((crc & 0xff) << 4) << 1;
    }

#if crc_timetest
    if (tstart > 0 && gettimeofday(&t, NULL) == 0)
        printf("vdr-radio: crc-calctime = %d usec\n", (int)((t.tv_sec*1000000 + t.tv_usec) - tstart));
#endif    

    return ~(crc);
}

/* ----------------------------------------------------------------------------------------------------------- */

#define EntityChars 56
const char *entitystr[EntityChars]  = { "&apos;",   "&amp;",    "&quot;",  "&gt",      "&lt",      "&copy;",   "&times;", "&nbsp;",
                                        "&Auml;",   "&auml;",   "&Ouml;",  "&ouml;",   "&Uuml;",   "&uuml;",   "&szlig;", "&deg;",
                                        "&Agrave;", "&Aacute;", "&Acirc;", "&Atilde;", "&agrave;", "&aacute;", "&acirc;", "&atilde;",
                                        "&Egrave;", "&Eacute;", "&Ecirc;", "&Euml;",   "&egrave;", "&eacute;", "&ecirc;", "&euml;",
                                        "&Igrave;", "&Iacute;", "&Icirc;", "&Iuml;",   "&igrave;", "&iacute;", "&icirc;", "&iuml;",
                                        "&Ograve;", "&Oacute;", "&Ocirc;", "&Otilde;", "&ograve;", "&oacute;", "&ocirc;", "&otilde;",
                                        "&Ugrave;", "&Uacute;", "&Ucirc;", "&Ntilde;", "&ugrave;", "&uacute;", "&ucirc;", "&ntilde;" };
const char *entitychar[EntityChars] = { "'",        "&",        "\"",      ">",        "<",         "c",        "*",      " ",
                                        "Ä",        "ä",        "Ö",       "ö",        "Ü",         "ü",        "ß",      "°",
                                        "À",        "Á",        "Â",       "Ã",        "à",         "á",        "â",      "ã",
                                        "È",        "É",        "Ê",       "Ë",        "è",         "é",        "ê",      "ë",
                                        "Ì",        "Í",        "Î",       "Ï",        "ì",         "í",        "î",      "ï",
                                        "Ò",        "Ó",        "Ô",       "Õ",        "ò",         "ó",        "ô",      "õ", 
                                        "Ù",        "Ú",        "Û",       "Ñ",        "ù",         "ú",        "û",      "ñ" };

char *rds_entitychar(char *text)
{
    int i = 0, l, lof, lre, space;
    char *temp;
    
    while (i < EntityChars) {
        if ((temp = strstr(text, entitystr[i])) != NULL) {
            if ((S_Verbose & 0x0f) >= 2)
                printf("RText-Entity: %s\n", text);
            l = strlen(entitystr[i]);
            lof = (temp-text);
            if (strlen(text) < RT_MEL) {
                lre = strlen(text) - lof - l;
                space = 1;
                }
            else {
                lre =  RT_MEL - 1 - lof - l;
                space = 0;
                }
            memmove(text+lof, entitychar[i], 1);
            memmove(text+lof+1, temp+l, lre);
            if (space != 0)
                memmove(text+lof+1+lre, "       ", l-1);
            }
        else i++;
        }

    return text;
}

/* ----------------------------------------------------------------------------------------------------------- */

#define XhtmlChars 56
const char *xhtmlstr[XhtmlChars]  = {   "&#039;", "&#038;", "&#034;", "&#062;", "&#060;", "&#169;", "&#042;", "&#160;",
                                        "&#196;", "&#228;", "&#214;", "&#246;", "&#220;", "&#252;", "&#223;", "&#176;",
                                        "&#192;", "&#193;", "&#194;", "&#195;", "&#224;", "&#225;", "&#226;", "&#227;",
                                        "&#200;", "&#201;", "&#202;", "&#203;", "&#232;", "&#233;", "&#234;", "&#235;",
                                        "&#204;", "&#205;", "&#206;", "&#207;", "&#236;", "&#237;", "&#238;", "&#239;",
                                        "&#210;", "&#211;", "&#212;", "&#213;", "&#242;", "&#243;", "&#244;", "&#245;",
                                        "&#217;", "&#218;", "&#219;", "&#209;", "&#249;", "&#250;", "&#251;", "&#241;" };
/*  hex todo:                   "&#x27;", "&#x26;", */
/*  see *entitychar[] 
const char *xhtmlychar[EntityChars] = { "'",    "&",      "\"",     ">",      "<",      "c",      "*",      " ",
                                        "Ä",    "ä",      "Ö",      "ö",      "Ü",      "ü",      "ß",      "°",
                                        "À",    "Á",      "Â",      "Ã",      "à",      "á",      "â",      "ã",
                                        "È",    "É",      "Ê",      "Ë",      "è",      "é",      "ê",      "ë",
                                        "Ì",    "Í",      "Î",      "Ï",      "ì",      "í",      "î",      "ï",
                                        "Ò",    "Ó",      "Ô",      "Õ",      "ò",      "ó",      "ô",      "õ", 
                                        "Ù",    "Ú",      "Û",      "Ñ",      "ù",      "ú",      "û",      "ñ" };
*/

char *xhtml2text(char *text)
{
    int i = 0, l, lof, lre, space;
    char *temp;
    
    while (i < XhtmlChars) {
        if ((temp = strstr(text, xhtmlstr[i])) != NULL) {
            if ((S_Verbose & 0x0f) >= 2)
                printf("XHTML-Char: %s\n", text);
            l = strlen(xhtmlstr[i]);
            lof = (temp-text);
            if (strlen(text) < RT_MEL) {
                lre = strlen(text) - lof - l;
                space = 1;
                }
            else {
                lre =  RT_MEL - 1 - lof - l;
                space = 0;
                }
            memmove(text+lof, entitychar[i], 1);
            memmove(text+lof+1, temp+l, lre);
            if (space != 0)
                memmove(text+lof+1+lre, "     ", l-1);
            }
        else i++;
        }

    rds_entitychar(text); 
    return text;
}

/* ----------------------------------------------------------------------------------------------------------- */

char *rtrim(char *text)
{
    char *s = text + strlen(text) - 1;
    while (s >= text && (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r'))
        *s-- = 0;

    return text;
}

/* ----------------------------------------------------------------------------------------------------------- */

const char *bitrates[5][16] = {
    // MPEG 1, Layer 1-3
    { "free", "32k", "64k", "96k", "128k", "160k", "192k", "224k", "256k", "288k", "320k", "352k", "384k", "416k", "448k", "bad" },
    { "free", "32k", "48k", "56k",  "64k",  "80k",  "96k", "112k", "128k", "160k", "192k", "224k", "256k", "320k", "384k", "bad" },
    { "free", "32k", "40k", "48k",  "56k",  "64k",  "80k",  "96k", "112k", "128k", "160k", "192k", "224k", "256k", "320k", "bad" }
/*    MPEG 2/2.5, Layer 1+2/3
    { "free", "32k", "48k", "56k",  "64k",  "80k",  "96k", "112k", "128k", "144k", "160k", "176k", "192k", "224k", "256k", "bad" },
    { "free",  "8k", "16k", "24k",  "32k",  "40k",  "48k",  "56k",  "64k",  "80k",  "96k", "112k", "128k", "144k", "160k", "bad" }
*/
};

char *audiobitrate(const unsigned char *data)
{
    int hl = (data[8] > 0) ? 9 + data[8] : 9;
    //printf("vdr-radio: audioheader = <%02x %02x %02x %02x>\n", data[hl], data[hl+1], data[hl+2], data[hl+3]);

    char *temp;
    if (data[hl] == 0xff && (data[hl+1] & 0xe0) == 0xe0) {  // syncword o.k.
        int layer = (data[hl+1] & 0x06) >> 1;       // Layer description
        if (layer > 0) {
            switch ((data[hl+1] & 0x18) >> 3) {     // Audio Version ID
                case 0x00:  asprintf(&temp, "V2.5");
                            break;
                case 0x01:  asprintf(&temp, "Vres");
                            break;
                case 0x02:  asprintf(&temp, "V2");
                            break;
                case 0x03:  asprintf(&temp, bitrates[3-layer][data[hl+2] >> 4]);
                            break;
                }
            }
        else
            asprintf(&temp, "Lres");
        }
    else
        asprintf(&temp, "???");

    return temp;
}

/* ----------------------------------------------------------------------------------------------------------- */

const char *tmc_duration[8] = {
   "none",
   "15 minutes",
   "30 minutes",
   "1 hour",
   "2 hours",
   "3 hours",
   "4 hours",
   "all day",
};

const char *tmc_direction[2] = { "+", "-" };

const char *tmc_event[2048] = {
    "---",                                      // 0
    "traffic problem",
    "queuing traffic (with average speeds Q). Danger of stationary traffic",
    "..", "..", "..", "..", "..", "..", "..", "..",                 // 10
    "overheight warning system triggered",
    "(Q) accident(s), traffic being directed around accident area",
    "..", "..", "..", 
    "closed, rescue and recovery work in progress",
    "..", "..", "..",
    "service area overcrowded, drive to another service area",              // 20
    "..",
    "service area, fuel station closed",
    "service area, restaurant closed",
    "bridge closed",
    "tunnel closed",
    "bridge blocked",
    "tunnel blocked",
    "road closed intermittently",
    "..", "..",                                     // 30
    "..", "..", "..", "..", "..",
    "fuel station reopened",
    "restaurant reopened",
    "..", "..",
    "smog alert ended",                                 // 40
    "(Q) overtaking lane(s) closed",
    "(Q) overtaking lane(s) blocked",
    "..", "..", "..", "..", "..", "..", "..", "..",                 // 50
    "roadworks, (Q) overtaking lane(s) closed",
    "(Q sets of) roadworks on the hard shoulder",
    "(Q sets of) roadworks in the emergency lane",
    "..",
    "traffic problem expected",
    "traffic congestion expected",
    "normal traffic expected",
    "..", "..", "..",                                   // 60
    "(Q) object(s) on roadway {something that does not neccessarily block the road}",
    "(Q) burst pipe(s)",
    "(Q) object(s) on the road. Danger",
    "burst pipe. Danger",
    "..", "..", "..", "..", "..",
    "traffic congestion, average speed of ?? km/h",                 // 70
    "traffic congestion, average speed of ?? km/h",
    "traffic congestion, average speed of ?? km/h",
    "traffic congestion, average speed of ?? km/h",
    "traffic congestion, average speed of ?? km/h",
    "traffic congestion, average speed of ?? km/h",
    "traffic congestion, average speed of ?? km/h",
    "..", "..", "..", "..",                             // 80
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 90
    "delays (Q) for cars",
    "..", "..", "..", "..", "..", "..", "..", "..", "..",               // 100
    "stationary traffic",
    "stationary traffic for 1 km",
    "stationary traffic for 2 km",
    "stationary traffic for 4 km",
    "stationary traffic for 6 km",
    "stationary traffic for 10 km",
    "stationary traffic expected",
    "queuing traffic (with average speeds Q)",
    "queuing traffic for 1 km (with average speeds Q)",
    "queuing traffic for 2 km (with average speeds Q)",                 // 110
    "queuing traffic for 4 km (with average speeds Q)",
    "queuing traffic for 6 km (with average speeds Q)",
    "queuing traffic for 10 km (with average speeds Q)",
    "queuing traffic expected",
    "slow traffic (with average speeds Q)",
    "slow traffic for 1 km (with average speeds Q)",
    "slow traffic for 2 km (with average speeds Q)",
    "slow traffic for 4 km (with average speeds Q)",
    "slow traffic for 6 km (with average speeds Q)",
    "slow traffic for 10 km (with average speeds Q)",                   // 120
    "slow traffic expected",
    "heavy traffic (with average speeds Q)",
    "heavy traffic expected",
    "traffic flowing freely (with average speeds Q)",
    "traffic building up (with average speeds Q)",
    "no problems to report",
    "traffic congestion cleared",
    "message cancelled",
    "stationary traffic for 3 km",
    "danger of stationary traffic",                         // 130
    "queuing traffic for 3 km (with average speeds Q)",
    "danger of queuing traffic (with average speeds Q)",
    "long queues (with average speeds Q)",
    "slow traffic for 3 km (with average speeds Q)",
    "traffic easing",
    "traffic congestion (with average speeds Q)",
    "traffic lighter than normal (with average speeds Q)",
    "queuing traffic (with average speeds Q). Approach with care",
    "queuing traffic around a bend in the road",
    "queuing traffic over the crest of a hill",                     // 140
    "all accidents cleared, no problems to report",
    "traffic heavier than normal (with average speeds Q)",
    "traffic very much heavier than normal (with average speeds Q)",
    "..", "..", "..", "..", "..", "..", "..",                       // 150
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 160
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 170
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 180
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 190
    "..", "..", "..", "..", "..", "..", "..", "..", "..",
    "multi vehicle pile up. Delays (Q)",                        // 200
    "(Q) accident(s)",
    "(Q) serious accident(s)",
    "multi-vehicle accident (involving Q vehicles)",
    "accident involving (a/Q) heavy lorr(y/ies)",
    "(Q) accident(s) involving hazardous materials",
    "(Q) fuel spillage accident(s)",
    "(Q) chemical spillage accident(s)",
    "vehicles slowing to look at (Q) accident(s)",
    "(Q) accident(s) in the opposing lanes",
    "(Q) shed load(s)",                                 // 210
    "(Q) broken down vehicle(s)",
    "(Q) broken down heavy lorr(y/ies)",
    "(Q) vehicle fire(s)",
    "(Q) incident(s)",
    "(Q) accident(s). Stationary traffic",
    "(Q) accident(s). Stationary traffic for 1 km",
    "(Q) accident(s). Stationary traffic for 2 km",
    "(Q) accident(s). Stationary traffic for 4 km",
    "(Q) accident(s). Stationary traffic for 6 km",
    "(Q) accident(s). Stationary traffic for 10 km",                    // 220
    "(Q) accident(s). Danger of stationary traffic",
    "(Q) accident(s). Queuing traffic",
    "(Q) accident(s). Queuing traffic for 1 km",
    "(Q) accident(s). Queuing traffic for 2 km",
    "(Q) accident(s). Queuing traffic for 4 km",
    "(Q) accident(s). Queuing traffic for 6 km",
    "(Q) accident(s). Queuing traffic for 10 km",
    "(Q) accident(s). Danger of queuing traffic",
    "(Q) accident(s). Slow traffic",
    "(Q) accident(s). Slow traffic for 1 km",                       // 230
    "(Q) accident(s). Slow traffic for 2 km",
    "(Q) accident(s). Slow traffic for 4 km",
    "(Q) accident(s). Slow traffic for 6 km",
    "(Q) accident(s). Slow traffic for 10 km",
    "(Q) accident(s). Slow traffic expected",
    "(Q) accident(s). Heavy traffic",
    "(Q) accident(s). Heavy traffic expected",
    "(Q) accident(s). Traffic flowing freely",
    "(Q) accident(s). Traffic building up",
    "road closed due to (Q) accident(s)",                       // 240
    "(Q) accident(s). Right lane blocked",
    "(Q) accident(s). Centre lane blocked",
    "(Q) accident(s). Left lane blocked",
    "(Q) accident(s). Hard shoulder blocked",
    "(Q) accident(s). Two lanes blocked",
    "(Q) accident(s). Three lanes blocked",
    "accident. Delays (Q)",
    "accident. Delays (Q) expected",
    "accident. Long delays (Q)",
    "vehicles slowing to look at (Q) accident(s). Stationary traffic",          // 250
    "vehicles slowing to look at (Q) accident(s). Stationary traffic for 1 km",
    "vehicles slowing to look at (Q) accident(s). Stationary traffic for 2 km",
    "vehicles slowing to look at (Q) accident(s). Stationary traffic for 4 km",
    "vehicles slowing to look at (Q) accident(s). Stationary traffic for 6 km",
    "vehicles slowing to look at (Q) accident(s). Stationary traffic for 10 km",
    "vehicles slowing to look at (Q) accident(s). Danger of stationary traffic",
    "vehicles slowing to look at (Q) accident(s). Queuing traffic",
    "vehicles slowing to look at (Q) accident(s). Queuing traffic for 1 km",
    "vehicles slowing to look at (Q) accident(s). Queuing traffic for 2 km",
    "vehicles slowing to look at (Q) accident(s). Queuing traffic for 4 km",        // 260
    "vehicles slowing to look at (Q) accident(s). Queuing traffic for 6 km",
    "vehicles slowing to look at (Q) accident(s). Queuing traffic for 10 km",
    "vehicles slowing to look at (Q) accident(s). Danger of queuing traffic",
    "vehicles slowing to look at (Q) accident(s). Slow traffic",
    "vehicles slowing to look at (Q) accident(s). Slow traffic for 1 km",
    "vehicles slowing to look at (Q) accident(s). Slow traffic for 2 km",
    "vehicles slowing to look at (Q) accident(s). Slow traffic for 4 km",
    "vehicles slowing to look at (Q) accident(s). Slow traffic for 6 km",
    "vehicles slowing to look at (Q) accident(s). Slow traffic for 10 km",
    "vehicles slowing to look at (Q) accident(s). Slow traffic expected",       // 270
    "vehicles slowing to look at (Q) accident(s). Heavy traffic",
    "vehicles slowing to look at (Q) accident(s). Heavy traffic expected",
    "..",
    "vehicles slowing to look at (Q) accident(s). Traffic building up",
    "vehicles slowing to look at accident. Delays (Q)",
    "vehicles slowing to look at accident. Delays (Q) expected",
    "vehicles slowing to look at accident. Long delays (Q)",
    "(Q) shed load(s). Stationary traffic",
    "(Q) shed load(s). Stationary traffic for 1 km",
    "(Q) shed load(s). Stationary traffic for 2 km",                    // 280
    "(Q) shed load(s). Stationary traffic for 4 km",
    "(Q) shed load(s). Stationary traffic for 6 km",
    "(Q) shed load(s). Stationary traffic for 10 km",
    "(Q) shed load(s). Danger of stationary traffic",
    "(Q) shed load(s). Queuing traffic",
    "(Q) shed load(s). Queuing traffic for 1 km",
    "(Q) shed load(s). Queuing traffic for 2 km",
    "(Q) shed load(s). Queuing traffic for 4 km",
    "(Q) shed load(s). Queuing traffic for 6 km",
    "(Q) shed load(s). Queuing traffic for 10 km",                  // 290
    "(Q) shed load(s). Danger of queuing traffic",
    "(Q) shed load(s). Slow traffic",
    "(Q) shed load(s). Slow traffic for 1 km",
    "(Q) shed load(s). Slow traffic for 2 km",
    "(Q) shed load(s). Slow traffic for 4 km",
    "(Q) shed load(s). Slow traffic for 6 km",
    "(Q) shed load(s). Slow traffic for 10 km",
    "(Q) shed load(s). Slow traffic expected",
    "(Q) shed load(s). Heavy traffic",
    "(Q) shed load(s). Heavy traffic expected",                     // 300
    "(Q) shed load(s). Traffic flowing freely",
    "(Q) shed load(s). Traffic building up",
    "blocked by (Q) shed load(s)",
    "(Q) shed load(s). Right lane blocked",
    "(Q) shed load(s). Centre lane blocked",
    "(Q) shed load(s). Left lane blocked",
    "(Q) shed load(s). Hard shoulder blocked",
    "(Q) shed load(s). Two lanes blocked",
    "(Q) shed load(s). Three lanes blocked",
    "shed load. Delays (Q)",                                // 310
    "shed load. Delays (Q) expected",
    "shed load. Long delays (Q)",
    "(Q) broken down vehicle(s). Stationary traffic",
    "(Q) broken down vehicle(s). Danger of stationary traffic",
    "(Q) broken down vehicle(s). Queuing traffic",
    "(Q) broken down vehicle(s). Danger of queuing traffic",
    "(Q) broken down vehicle(s). Slow traffic",
    "(Q) broken down vehicle(s). Slow traffic expected",
    "(Q) broken down vehicle(s). Heavy traffic",
    "(Q) broken down vehicle(s). Heavy traffic expected",               // 320
    "(Q) broken down vehicle(s). Traffic flowing freely",
    "(Q) broken down vehicle(s).Traffic building up",
    "blocked by (Q) broken down vehicle(s).",
    "(Q) broken down vehicle(s). Right lane blocked",
    "(Q) broken down vehicle(s). Centre lane blocked",
    "(Q) broken down vehicle(s). Left lane blocked",
    "(Q) broken down vehicle(s). Hard shoulder blocked",
    "(Q) broken down vehicle(s). Two lanes blocked",
    "(Q) broken down vehicle(s). Three lanes blocked",
    "broken down vehicle. Delays (Q)",                          // 330
    "broken down vehicle. Delays (Q) expected",
    "broken down vehicle. Long delays (Q)",
    "accident cleared",
    "message cancelled",
    "accident involving (a/Q) bus(es)",
    "(Q) oil spillage accident(s)",
    "(Q) overturned vehicle(s)",
    "(Q) overturned heavy lorr(y/ies)",
    "(Q) jackknifed trailer(s)",
    "(Q) jackknifed caravan(s)",                            // 340
    "(Q) jackknifed articulated lorr(y/ies)",
    "(Q) vehicle(s) spun around",
    "(Q) earlier accident(s)",
    "accident investigation work",
    "(Q) secondary accident(s)",
    "(Q) broken down bus(es)",
    "(Q) overheight vehicle(s)",
    "(Q) accident(s). Stationary traffic for 3 km",
    "(Q) accident(s). Queuing traffic for 3 km",
    "(Q) accident(s). Slow traffic for 3 km",                       // 350
    "(Q) accident(s) in roadworks area",
    "vehicles slowing to look at (Q) accident(s). Stationary traffic for 3 km",
    "vehicles slowing to look at (Q) accident(s). Queuing traffic for 3 km",
    "vehicles slowing to look at (Q) accident(s). Slow traffic for 3 km",
    "vehicles slowing to look at (Q) accident(s). Danger",
    "(Q) shed load(s). Stationary traffic for 3 km",
    "(Q) shed load(s). Queuing traffic for 3 km",
    "(Q) shed load(s). Slow traffic for 3 km",
    "(Q) shed load(s). Danger",
    "(Q) overturned vehicle(s). Stationary traffic",                    // 360
    "(Q) overturned vehicle(s). Danger of stationary traffic",
    "(Q) overturned vehicle(s). Queuing traffic",
    "(Q) overturned vehicle(s). Danger of queuing traffic",
    "(Q) overturned vehicle(s). Slow traffic",
    "(Q) overturned vehicle(s). Slow traffic expected",
    "(Q) overturned vehicle(s). Heavy traffic",
    "(Q) overturned vehicle(s). Heavy traffic expected",
    "(Q) overturned vehicle(s). Traffic building up",
    "blocked by (Q) overturned vehicle(s)",
    "(Q) overturned vehicle(s). Right lane blocked",                    // 370
    "(Q) overturned vehicle(s). Centre lane blocked",
    "(Q) overturned vehicle(s). Left lane blocked",
    "(Q) overturned vehicle(s). Two lanes blocked",
    "(Q) overturned vehicle(s). Three lanes blocked",
    "overturned vehicle. Delays (Q)",
    "overturned vehicle. Delays (Q) expected",
    "overturned vehicle. Long delays (Q)",
    "(Q) overturned vehicle(s). Danger",
    "Stationary traffic due to (Q) earlier accident(s)",
    "Danger of stationary traffic due to (Q) earlier accident(s)",          // 380
    "Queuing traffic due to (Q) earlier accident(s)",
    "Danger of queuing traffic due to (Q) earlier accident(s)",
    "Slow traffic due to (Q) earlier accident(s)",
    "..",
    "Heavy traffic due to (Q) earlier accident(s)",
    "..",
    "Traffic building up due to (Q) earlier accident(s)",
    "Delays (Q) due to earlier accident",
    "..",
    "Long delays (Q) due to earlier accident",                      // 390
    "accident investigation work. Danger",
    "(Q) secondary accident(s). Danger",
    "(Q) broken down vehicle(s). Danger",
    "(Q) broken down heavy lorr(y/ies). Danger",
    "road cleared",
    "incident cleared",
    "rescue and recovery work in progress",
    "..",
    "message cancelled",
    "..",                                       // 400
    "closed",
    "blocked",
    "closed for heavy vehicles (over Q)",
    "no through traffic for heavy lorries (over Q)",
    "no through traffic",
    "(Q th) entry slip road closed",
    "(Q th) exit slip road closed",
    "slip roads closed",
    "slip road restrictions",
    "closed ahead. Stationary traffic",                         // 410
    "closed ahead. Stationary traffic for 1 km",
    "closed ahead. Stationary traffic for 2 km",
    "closed ahead. Stationary traffic for 4 km",
    "closed ahead. Stationary traffic for 6 km",
    "closed ahead. Stationary traffic for 10 km",
    "closed ahead. Danger of stationary traffic",
    "closed ahead. Queuing traffic",
    "closed ahead. Queuing traffic for 1 km",
    "closed ahead. Queuing traffic for 2 km",
    "closed ahead. Queuing traffic for 4 km",                       // 420
    "closed ahead. Queuing traffic for 6 km",
    "closed ahead. Queuing traffic for 10 km",
    "closed ahead. Danger of queuing traffic",
    "closed ahead. Slow traffic",
    "closed ahead. Slow traffic for 1 km",
    "closed ahead. Slow traffic for 2 km",
    "closed ahead. Slow traffic for 4 km",
    "closed ahead. Slow traffic for 6 km",
    "closed ahead. Slow traffic for 10 km",
    "closed ahead. Slow traffic expected",                      // 430
    "closed ahead. Heavy traffic",
    "closed ahead. Heavy traffic expected",
    "closed ahead. Traffic flowing freely",
    "closed ahead. Traffic building up",
    "closed ahead. Delays (Q)",
    "closed ahead. Delays (Q) expected",
    "closed ahead. Long delays (Q)",
    "blocked ahead. Stationary traffic",
    "blocked ahead. Stationary traffic for 1 km",
    "blocked ahead. Stationary traffic for 2 km",                   // 440
    "blocked ahead. Stationary traffic for 4 km",
    "blocked ahead. Stationary traffic for 6 km",
    "blocked ahead. Stationary traffic for 10 km",
    "blocked ahead. Danger of stationary traffic",
    "blocked ahead. Queuing traffic",
    "blocked ahead. Queuing traffic for 1 km",
    "blocked ahead. Queuing traffic for 2 km",
    "blocked ahead. Queuing traffic for 4 km",
    "blocked ahead. Queuing traffic for 6 km",
    "blocked ahead. Queuing traffic for 10 km",                     // 450
    "blocked ahead. Danger of queuing traffic",
    "blocked ahead. Slow traffic",
    "blocked ahead. Slow traffic for 1 km",
    "blocked ahead. Slow traffic for 2 km",
    "blocked ahead. Slow traffic for 4 km",
    "blocked ahead. Slow traffic for 6 km",
    "blocked ahead. Slow traffic for 10 km",
    "blocked ahead. Slow traffic expected",
    "blocked ahead. Heavy traffic",
    "blocked ahead. Heavy traffic expected",                        // 460
    "blocked ahead. Traffic flowing freely",
    "blocked ahead. Traffic building up",
    "blocked ahead. Delays (Q)",
    "blocked ahead. Delays (Q) expected",
    "blocked ahead. Long delays (Q)",
    "slip roads reopened",
    "reopened",
    "message cancelled",
    "closed ahead",
    "blocked ahead",                                    // 470
    "(Q) entry slip road(s) closed",
    "(Q th) entry slip road blocked",
    "entry blocked",
    "(Q) exit slip road(s) closed",
    "(Q th) exit slip road blocked",
    "exit blocked",
    "slip roads blocked",
    "connecting carriageway closed",
    "parallel carriageway closed",
    "right-hand parallel carriageway closed",                       // 480
    "left-hand parallel carriageway closed",
    "express lanes closed",
    "through traffic lanes closed",
    "local lanes closed",
    "connecting carriageway blocked",
    "parallel carriageway blocked",
    "right-hand parallel carriageway blocked",
    "left-hand parallel carriageway blocked",
    "express lanes blocked",
    "through traffic lanes blocked",                            // 490
    "local lanes blocked",
    "no motor vehicles",
    "restrictions",
    "closed for heavy lorries (over Q)",
    "closed ahead. Stationary traffic for 3 km",
    "closed ahead. Queuing traffic for 3 km",
    "closed ahead. Slow traffic for 3 km",
    "blocked ahead. Stationary traffic for 3 km",
    "blocked ahead. Queuing traffic for 3 km",
    "(Q) lane(s) closed",                               // 500
    "(Q) right lane(s) closed",
    "(Q) centre lane(s) closed",
    "(Q) left lane(s) closed",
    "hard shoulder closed",
    "two lanes closed",
    "three lanes closed",
    "(Q) right lane(s) blocked",
    "(Q) centre lane(s) blocked",
    "(Q) left lane(s) blocked",
    "hard shoulder blocked",                                // 510
    "two lanes blocked",
    "three lanes blocked",
    "single alternate line traffic",
    "carriageway reduced (from Q lanes) to one lane",
    "carriageway reduced (from Q lanes) to two lanes",
    "carriageway reduced (from Q lanes) to three lanes",
    "contraflow",
    "narrow lanes",
    "contraflow with narrow lanes",
    "(Q) lane(s) blocked",                              // 520
    "(Q) lanes closed. Stationary traffic",
    "(Q) lanes closed. Stationary traffic for 1 km",
    "(Q) lanes closed. Stationary traffic for 2 km",
    "(Q) lanes closed. Stationary traffic for 4 km",
    "(Q) lanes closed. Stationary traffic for 6 km",
    "(Q) lanes closed. Stationary traffic for 10 km",
    "(Q) lanes closed. Danger of stationary traffic",
    "(Q) lanes closed. Queuing traffic",
    "(Q) lanes closed. Queuing traffic for 1 km",
    "(Q) lanes closed. Queuing traffic for 2 km",                   // 530
    "(Q) lanes closed. Queuing traffic for 4 km",
    "(Q) lanes closed. Queuing traffic for 6 km",
    "(Q) lanes closed. Queuing traffic for 10 km",
    "(Q) lanes closed. Danger of queuing traffic",
    "(Q) lanes closed. Slow traffic",
    "(Q) lanes closed. Slow traffic for 1 km",
    "(Q) lanes closed. Slow traffic for 2 km",
    "(Q) lanes closed. Slow traffic for 4 km",
    "(Q) lanes closed. Slow traffic for 6 km",
    "(Q) lanes closed. Slow traffic for 10 km",                     // 540
    "(Q) lanes closed. Slow traffic expected",
    "(Q) lanes closed. Heavy traffic",
    "(Q) lanes closed. Heavy traffic expected",
    "(Q)lanes closed. Traffic flowing freely",
    "(Q)lanes closed. Traffic building up",
    "carriageway reduced (from Q lanes) to one lane. Stationary traffic",
    "carriageway reduced (from Q lanes) to one lane. Danger of stationary traffic",
    "carriageway reduced (from Q lanes) to one lane. Queuing traffic",
    "carriageway reduced (from Q lanes) to one lane. Danger of queuing traffic",
    "carriageway reduced (from Q lanes) to one lane. Slow traffic",         // 550
    "carriageway reduced (from Q lanes) to one lane. Slow traffic expected",
    "carriageway reduced (from Q lanes) to one lane. Heavy traffic",
    "carriageway reduced (from Q lanes) to one lane. Heavy traffic expected",
    "carriageway reduced (from Q lanes) to one lane. Traffic flowing freely",
    "carriageway reduced (from Q lanes) to one lane. Traffic building up",
    "carriageway reduced (from Q lanes) to two lanes. Stationary traffic",
    "carriageway reduced (from Q lanes) to two lanes. Danger of stationary traffic",
    "carriageway reduced (from Q lanes) to two lanes. Queuing traffic",
    "carriageway reduced (from Q lanes) to two lanes. Danger of queuing traffic",
    "carriageway reduced (from Q lanes) to two lanes. Slow traffic",            // 560
    "carriageway reduced (from Q lanes) to two lanes. Slow traffic expected",
    "carriageway reduced (from Q lanes) to two lanes. Heavy traffic",
    "carriageway reduced (from Q lanes) to two lanes. Heavy traffic expected",
    "carriageway reduced (from Q lanes) to two lanes. Traffic flowing freely",
    "carriageway reduced (from Q lanes) to two lanes. Traffic building up",
    "carriageway reduced (from Q lanes) to three lanes. Stationary traffic",
    "carriageway reduced (from Q lanes) to three lanes. Danger of stationary traffic",
    "carriageway reduced (from Q lanes) to three lanes. Queuing traffic",
    "carriageway reduced (from Q lanes) to three lanes. Danger of queuing traffic",
    "carriageway reduced (from Q lanes) to three lanes. Slow traffic",          // 570
    "carriageway reduced (from Q lanes) to three lanes. Slow traffic expected",
    "carriageway reduced (from Q lanes) to three lanes. Heavy traffic",
    "carriageway reduced (from Q lanes) to three lanes. Heavy traffic expected",
    "carriageway reduced (from Q lanes) to three lanes. Traffic flowing freely",
    "carriageway reduced (from Q lanes) to three lanes. Traffic building up",
    "contraflow. Stationary traffic",
    "contraflow. Stationary traffic for 1 km",
    "contraflow. Stationary traffic for 2 km",
    "contraflow. Stationary traffic for 4 km",
    "contraflow. Stationary traffic for 6 km",                      // 580
    "contraflow. Stationary traffic for 10 km",
    "contraflow. Danger of stationary traffic",
    "contraflow. Queuing traffic",
    "contraflow. Queuing traffic for 1 km",
    "contraflow. Queuing traffic for 2 km",
    "contraflow. Queuing traffic for 4 km",
    "contraflow. Queuing traffic for 6 km",
    "contraflow. Queuing traffic for 10 km",
    "contraflow. Danger of queuing traffic",
    "contraflow. Slow traffic",                             // 590
    "contraflow. Slow traffic for 1 km",
    "contraflow. Slow traffic for 2 km",
    "contraflow. Slow traffic for 4 km",
    "contraflow. Slow traffic for 6 km",
    "contraflow. Slow traffic for 10 km",
    "contraflow. Slow traffic expected",
    "contraflow. Heavy traffic",
    "contraflow. Heavy traffic expected",
    "contraflow. Traffic flowing freely",
    "contraflow. Traffic building up",                          // 600
    "contraflow. Carriageway reduced (from Q lanes) to one lane",
    "contraflow. Carriageway reduced (from Q lanes) to two lanes",
    "contraflow. Carriageway reduced (from Q lanes) to three lanes",
    "narrow lanes. Stationary traffic",
    "narrow lanes. Danger of stationary traffic",
    "narrow lanes. Queuing traffic",
    "narrow lanes. Danger of queuing traffic",
    "narrow lanes. Slow traffic",
    "narrow lanes. Slow traffic expected",
    "narrow lanes. Heavy traffic",                          // 610
    "narrow lanes. Heavy traffic expected",
    "narrow lanes. Traffic flowing freely",
    "narrow lanes. Traffic building up",
    "contraflow with narrow lanes. Stationary traffic",
    "contraflow with narrow lanes. Stationary traffic. Danger of stationary traffic",
    "contraflow with narrow lanes. Queuing traffic",
    "contraflow with narrow lanes. Danger of queuing traffic",
    "contraflow with narrow lanes. Slow traffic",
    "contraflow with narrow lanes. Slow traffic expected",
    "contraflow with narrow lanes. Heavy traffic",                  // 620
    "contraflow with narrow lanes. Heavy traffic expected",
    "contraflow with narrow lanes. Traffic flowing freely",
    "contraflow with narrow lanes. Traffic building up",
    "lane closures removed",
    "message cancelled",
    "blocked ahead. Slow traffic for 3 km",
    "no motor vehicles without catalytic converters",
    "no motor vehicles with even-numbered registration plates",
    "no motor vehicles with odd-numbered registration plates",
    "open",                                     // 630
    "road cleared",
    "entry reopened",
    "exit reopened",
    "all carriageways reopened",
    "motor vehicle restrictions lifted",
    "traffic restrictions lifted {reopened for all traffic}",
    "emergency lane closed",
    "turning lane closed",
    "crawler lane closed",
    "slow vehicle lane closed",                             // 640
    "one lane closed",
    "emergency lane blocked",
    "turning lane blocked",
    "crawler lane blocked",
    "slow vehicle lane blocked",
    "one lane blocked",
    "(Q person) carpool lane in operation",
    "(Q person) carpool lane closed",
    "(Q person) carpool lane blocked",
    "carpool restrictions changed (to Q persons per vehicle)",              // 650
    "(Q) lanes closed. Stationary traffic for 3 km",
    "(Q) lanes closed. Queuing traffic for 3 km",
    "(Q) lanes closed. Slow traffic for 3 km",
    "contraflow. Stationary traffic for 3 km",
    "contraflow. Queuing traffic for 3 km",
    "contraflow. Slow traffic for 3 km",
    "lane blockages cleared",
    "contraflow removed",
    "(Q person) carpool restrictions lifted",
    "lane restrictions lifted",                             // 660
    "use of hard shoulder allowed",
    "normal lane regulations restored",
    "all carriageways cleared",
    "..", "..", "..", "..", "..", "..", "..",                       // 670
    "bus lane available for carpools (with at least Q occupants)",
    "message cancelled",
    "message cancelled",
    "..", "..",
    "bus lane blocked",
    "..",
    "heavy vehicle lane closed",
    "heavy vehicle lane blocked",
    "reopened for through traffic",                         // 680
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 690
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 700
    "(Q sets of) roadworks",
    "(Q sets of) major roadworks",
    "(Q sets of) maintenance work",
    "(Q sections of) resurfacing work",
    "(Q sets of) central reservation work",
    "(Q sets of) road marking work",
    "bridge maintenance work (at Q bridges)",
    "(Q sets of) temporary traffic lights",
    "(Q sections of) blasting work",
    "(Q sets of) roadworks. Stationary traffic",                    // 710
    "(Q sets of) roadworks. Stationary traffic for 1 km",
    "(Q sets of) roadworks. Stationary traffic for 2 km",
    "(Q sets of) roadworks. Stationary traffic for 4 km",
    "(Q sets of) roadworks. Stationary traffic for 6 km",
    "(Q sets of) roadworks. Stationary traffic for 10 km",
    "(Q sets of) roadworks. Danger of stationary traffic",
    "(Q sets of) roadworks. Queuing traffic",
    "(Q sets of) roadworks. Queuing traffic for 1 km",
    "(Q sets of) roadworks. Queuing traffic for 2 km",
    "(Q sets of) roadworks. Queuing traffic for 4 km",                  // 720
    "(Q sets of) roadworks. Queuing traffic for 6 km",
    "(Q sets of) roadworks. Queuing traffic for 10 km",
    "(Q sets of) roadworks. Danger of queuing traffic",
    "(Q sets of) roadworks. Slow traffic",
    "(Q sets of) roadworks. Slow traffic for 1 km",
    "(Q sets of) roadworks. Slow traffic for 2 km",
    "(Q sets of) roadworks. Slow traffic for 4 km",
    "(Q sets of) roadworks. Slow traffic for 6 km",
    "(Q sets of) roadworks. Slow traffic for 10 km",
    "(Q sets of) roadworks. Slow traffic expected",                 // 730
    "(Q sets of) roadworks. Heavy traffic",
    "(Q sets of) roadworks. Heavy traffic expected",
    "(Q sets of) roadworks. Traffic flowing freely",
    "(Q sets of) roadworks. Traffic building up",
    "closed due to (Q sets of) roadworks",
    "(Q sets of) roadworks. Right lane closed",
    "(Q sets of) roadworks. Centre lane closed",
    "(Q sets of) roadworks. Left lane closed",
    "(Q sets of) roadworks. Hard shoulder closed",
    "(Q sets of) roadworks. Two lanes closed",                      // 740
    "(Q sets of) roadworks. Three lanes closed",
    "(Q sets of) roadworks. Single alternate line traffic",
    "roadworks. Carriageway reduced (from Q lanes) to one lane",
    "roadworks. Carriageway reduced (from Q lanes) to two lanes",
    "roadworks. Carriageway reduced (from Q lanes) to three lanes",
    "(Q sets of) roadworks. Contraflow",
    "roadworks. Delays (Q)",
    "roadworks. Delays (Q) expected",
    "roadworks. Long delays (Q)",
    "(Q sections of) resurfacing work. Stationary traffic",             // 750
    "(Q sections of) resurfacing work. Stationary traffic for 1 km",
    "(Q sections of) resurfacing work. Stationary traffic for 2 km",
    "(Q sections of) resurfacing work. Stationary traffic for 4 km",
    "(Q sections of) resurfacing work. Stationary traffic for 6 km",
    "(Q sections of) resurfacing work. Stationary traffic for 10 km",
    "(Q sections of) resurfacing work. Danger of stationary traffic",
    "(Q sections of) resurfacing work. Queuing traffic",
    "(Q sections of) resurfacing work. Queuing traffic for 1 km",
    "(Q sections of) resurfacing work. Queuing traffic for 2 km",
    "(Q sections of) resurfacing work. Queuing traffic for 4 km",           // 760
    "(Q sections of) resurfacing work. Queuing traffic for 6 km",
    "(Q sections of) resurfacing work. Queuing traffic for 10 km",
    "(Q sections of) resurfacing work. Danger of queuing traffic",
    "(Q sections of) resurfacing work. Slow traffic",
    "(Q sections of) resurfacing work. Slow traffic for 1 km",
    "(Q sections of) resurfacing work. Slow traffic for 2 km",
    "(Q sections of) resurfacing work. Slow traffic for 4 km",
    "(Q sections of) resurfacing work. Slow traffic for 6 km",
    "(Q sections of) resurfacing work. Slow traffic for 10 km",
    "(Q sections of) resurfacing work. Slow traffic expected",              // 770
    "(Q sections of) resurfacing work. Heavy traffic",
    "(Q sections of) resurfacing work. Heavy traffic expected",
    "(Q sections of) resurfacing work. Traffic flowing freely",
    "(Q sections of) resurfacing work. Traffic building up",
    "(Q sections of) resurfacing work. Single alternate line traffic",
    "resurfacing work. Carriageway reduced (from Q lanes) to one lane",
    "resurfacing work. Carriageway reduced (from Q lanes) to two lanes",
    "resurfacing work. Carriageway reduced (from Q lanes) to three lanes",
    "(Q sections of) resurfacing work. Contraflow",
    "resurfacing work. Delays (Q)",                         // 780
    "resurfacing work. Delays (Q) expected",
    "resurfacing work. Long delays (Q)",
    "(Q sets of) road marking work. Stationary traffic",
    "(Q sets of) road marking work. Danger of stationary traffic",
    "(Q sets of) road marking work. Queuing traffic",
    "(Q sets of) road marking work. Danger of queuing traffic",
    "(Q sets of) road marking work. Slow traffic",
    "(Q sets of) road marking work. Slow traffic expected",
    "(Q sets of) road marking work. Heavy traffic",
    "(Q sets of) road marking work. Heavy traffic expected",                // 790
    "(Q sets of) road marking work. Traffic flowing freely",
    "(Q sets of) road marking work. Traffic building up",
    "(Q sets of) road marking work. Right lane closed",
    "(Q sets of) road marking work. Centre lane closed",
    "(Q sets of) road marking work. Left lane closed",
    "(Q sets of) road marking work. Hard shoulder closed",
    "(Q sets of) road marking work. Two lanes closed",
    "(Q sets of) road marking work. Three lanes closed",
    "closed for bridge demolition work (at Q bridges)",
    "roadworks cleared",                                // 800
    "message cancelled",
    "(Q sets of) long-term roadworks",
    "(Q sets of) construction work",
    "(Q sets of) slow moving maintenance vehicles",
    "bridge demolition work (at Q bridges)",
    "(Q sets of) water main work",
    "(Q sets of) gas main work",
    "(Q sets of) work on buried cables",
    "(Q sets of) work on buried services",
    "new roadworks layout",                             // 810
    "new road layout",
    "(Q sets of) roadworks. Stationary traffic for 3 km",
    "(Q sets of) roadworks. Queuing traffic for 3 km",
    "(Q sets of) roadworks. Slow traffic for 3 km",
    "(Q sets of) roadworks during the day time",
    "(Q sets of) roadworks during off-peak periods",
    "(Q sets of) roadworks during the night",
    "(Q sections of) resurfacing work. Stationary traffic for 3 km",
    "(Q sections of) resurfacing work. Queuing traffic for 3 km",
    "(Q sections of) resurfacing work. Slow traffic for 3 km",              // 820
    "(Q sets of) resurfacing work during the day time",
    "(Q sets of) resurfacing work during off-peak periods",
    "(Q sets of) resurfacing work during the night",
    "(Q sets of) road marking work. Danger",
    "(Q sets of) slow moving maintenance vehicles. Stationary traffic",
    "(Q sets of) slow moving maintenance vehicles. Danger of stationary traffic",
    "(Q sets of) slow moving maintenance vehicles. Queuing traffic",
    "(Q sets of) slow moving maintenance vehicles. Danger of queuing traffic",
    "(Q sets of) slow moving maintenance vehicles. Slow traffic",
    "(Q sets of) slow moving maintenance vehicles. Slow traffic expected",      // 830
    "(Q sets of) slow moving maintenance vehicles. Heavy traffic",
    "(Q sets of) slow moving maintenance vehicles. Heavy traffic expected",
    "(Q sets of) slow moving maintenance vehicles. Traffic flowing freely",
    "(Q sets of) slow moving maintenance vehicles. Traffic building up",
    "(Q sets of) slow moving maintenance vehicles. Right lane closed",
    "(Q sets of) slow moving maintenance vehicles. Centre lane closed",
    "(Q sets of) slow moving maintenance vehicles. Left lane closed",
    "(Q sets of) slow moving maintenance vehicles. Two lanes closed",
    "(Q sets of) slow moving maintenance vehicles. Three lanes closed",
    "water main work. Delays (Q)",                          // 840
    "water main work. Delays (Q) expected",
    "water main work. Long delays (Q)",
    "gas main work. Delays (Q)",
    "gas main work. Delays (Q) expected",
    "gas main work. Long delays (Q)",
    "work on buried cables. Delays (Q)",
    "work on buried cables. Delays (Q) expected",
    "work on buried cables. Long delays (Q)",
    "work on buried services. Delays (Q)",
    "work on buried services. Delays (Q) expected",                 // 850
    "work on buried services. Long delays (Q)",
    "construction traffic merging",
    "roadwork clearance in progress",
    "maintenance work cleared",
    "road layout unchanged",
    "construction traffic merging. Danger",
    "..", "..", "..", "..",                             // 860
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 870
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 880
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 890
    "..", "..", "..", "..", "..", "..", "..",
    "obstruction warning withdrawn",
    "clearance work in progress, road free again",
    "flooding expected",                                // 900
    "(Q) obstruction(s) on roadway {something that does block the road or part of",
    "(Q) obstructions on the road. Danger",
    "spillage on the road",
    "storm damage",
    "(Q) fallen trees",
    "(Q) fallen trees. Danger",
    "flooding",
    "flooding. Danger",
    "flash floods",
    "danger of flash floods",                               // 910
    "avalanches",
    "avalanche risk",
    "rockfalls",
    "landslips",
    "earthquake damage",
    "road surface in poor condition",
    "subsidence",
    "(Q) collapsed sewer(s)",
    "burst water main",
    "gas leak",                                     // 920
    "serious fire",
    "animals on roadway",
    "animals on the road. Danger",
    "clearance work",
    "blocked by storm damage",
    "blocked by (Q) fallen trees",
    "(Q) fallen tree(s). Passable with care",
    "flooding. Stationary traffic",
    "flooding. Danger of stationary traffic",
    "flooding. Queuing traffic",                            // 930
    "flooding. Danger of queuing traffic",
    "flooding. Slow traffic",
    "flooding. Slow traffic expected",
    "flooding. Heavy traffic",
    "flooding. Heavy traffic expected",
    "flooding. Traffic flowing freely",
    "flooding. Traffic building up",
    "closed due to flooding",
    "flooding. Delays (Q)",
    "flooding. Delays (Q) expected",                            // 940
    "flooding. Long delays (Q)",
    "flooding. Passable with care",
    "closed due to avalanches",
    "avalanches. Passable with care (above Q hundred metres)",
    "closed due to rockfalls",
    "rockfalls. Passable with care",
    "road closed due to landslips",
    "landslips. Passable with care",
    "closed due to subsidence",
    "subsidence. Single alternate line traffic",                    // 950
    "subsidence. Carriageway reduced (from Q lanes) to one lane",
    "subsidence. Carriageway reduced (from Q lanes) to two lanes",
    "subsidence. Carriageway reduced (from Q lanes) to three lanes",
    "subsidence. Contraflow in operation",
    "subsidence. Passable with care",
    "closed due to sewer collapse",
    "road closed due to burst water main",
    "burst water main. Delays (Q)",
    "burst water main. Delays (Q) expected",
    "burst water main. Long delays (Q)",                        // 960
    "closed due to gas leak",
    "gas leak. Delays (Q)",
    "gas leak. Delays (Q) expected",
    "gas leak. Long delays (Q)",
    "closed due to serious fire",
    "serious fire. Delays (Q)",
    "serious fire. Delays (Q) expected",
    "serious fire. Long delays (Q)",
    "closed for clearance work",
    "road free again",                                  // 970
    "message cancelled",
    "storm damage expected",
    "fallen power cables",
    "sewer overflow",
    "ice build-up",
    "mud slide",
    "grass fire",
    "air crash",
    "rail crash",
    "blocked by (Q) obstruction(s) on the road",                    // 980
    "(Q) obstructions on the road. Passable with care",
    "blocked due to spillage on roadway",
    "spillage on the road. Passable with care",
    "spillage on the road. Danger",
    "storm damage. Passable with care",
    "storm damage. Danger",
    "blocked by fallen power cables",
    "fallen power cables. Passable with care",
    "fallen power cables. Danger",
    "sewer overflow. Danger",                               // 990
    "flash floods. Danger",
    "avalanches. Danger",
    "closed due to avalanche risk",
    "avalanche risk. Danger",
    "closed due to ice build-up",
    "ice build-up. Passable with care (above Q hundred metres)",
    "ice build-up. Single alternate traffic",
    "rockfalls. Danger",
    "landslips. Danger",
    "earthquake damage. Danger",                            // 1000
    "hazardous driving conditions (above Q hundred metres)",
    "danger of aquaplaning",
    "slippery road (above Q hundred metres)",
    "mud on road",
    "leaves on road",
    "ice (above Q hundred metres)",
    "danger of ice (above Q hundred metres)",
    "black ice (above Q hundred metres)",
    "freezing rain (above Q hundred metres)",
    "wet and icy roads (above Q hundred metres)",                   // 1010
    "slush (above Q hundred metres)",
    "snow on the road (above Q hundred metres)",
    "packed snow (above Q hundred metres)",
    "fresh snow (above Q hundred metres)",
    "deep snow (above Q hundred metres)",
    "snow drifts (above Q hundred metres)",
    "slippery due to spillage on roadway",
    "slippery road (above Q hundred metres) due to snow",
    "slippery road (above Q hundred metres) due to frost",
    "road blocked by snow (above Q hundred metres)",                    // 1020
    "snow on the road. Carriageway reduced (from Q lanes) to one lane",
    "snow on the road. Carriageway reduced (from Q lanes) to two lanes",
    "snow on the road. Carriageway reduced (from Q lanes) to three lanes",
    "conditions of road surface improved",
    "message cancelled",
    "subsidence. Danger",
    "sewer collapse. Delays (Q)",
    "sewer collapse. Delays (Q) expected",
    "sewer collapse. Long delays (Q)",
    "sewer collapse. Danger",                               // 1030
    "burst water main. Danger",
    "gas leak. Danger",
    "serious fire. Danger",
    "clearance work. Danger",
    "impassable (above Q hundred metres)",
    "almost impassable (above Q hundred metres)",
    "extremely hazardous driving conditions (above Q hundred metres)",
    "difficult driving conditions (above Q hundred metres)",
    "passable with care (up to Q hundred metres)",
    "passable (up to Q hundred metres)",                        // 1040
    "surface water hazard",
    "loose sand on road",
    "loose chippings",
    "oil on road",
    "petrol on road",
    "icy patches (above Q hundred metres)",
    "danger of icy patches (above Q hundred metres)",
    "danger of black ice (above Q hundred metres)",
    "..", "..",                                     // 1050 
    "..", "..", "..",
    "slippery due to loose sand on roadway",
    "mud on road. Danger",
    "loose chippings. Danger",
    "oil on road. Danger",
    "petrol on road. Danger",
    "road surface in poor condition. Danger",
    "icy patches (above Q hundred metres) on bridges",                  // 1060
    "danger of icy patches (above Q hundred metres) on bridges",
    "icy patches (above Q hundred metres) on bridges, in shaded areas and on s",
    "impassable for heavy vehicles (over Q)",
    "impassable (above Q hundred metres) for vehicles with trailers",
    "driving conditions improved",
    "rescue and recovery work in progress. Danger",
    "large animals on roadway",
    "herds of animals on roadway",
    "skid hazard reduced",
    "snow cleared",                                 // 1070
    "..", "..",
    "extremely hazardous driving conditions expected (above Q hundred meters",
    "freezing rain expected (above Q hundred metres)",
    "danger of road being blocked by snow (above Q hundred metres)",
    "..", "..", "..",
    "temperature falling rapidly (to Q)",
    "extreme heat (up to Q)",                               // 1080
    "extreme cold (of Q)",
    "less extreme temperatures",
    "current temperature (Q)",
    "..", "..", "..", "..", "..", "..", "..",                       // 1090
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1100
    "heavy snowfall (Q)",
    "heavy snowfall (Q). Visibility reduced to < ?? m",
    "heavy snowfall (Q). Visibility reduced to < ?? m",
    "snowfall (Q)",
    "snowfall (Q). Visibility reduced to < ?? m",
    "hail (visibility reduced to Q)",
    "sleet (visibility reduced to Q)",
    "thunderstorms (visibility reduced to Q)",
    "heavy rain (Q)",
    "heavy rain (Q). Visibility reduced to < ?? m",                 // 1110
    "heavy rain (Q). Visibility reduced to < ?? m",
    "rain (Q)",
    "rain (Q). Visibility reduced to < ?? m",
    "showers (visibility reduced to Q)",
    "heavy frost",
    "frost",
    "..", "..", "..", "..",                             // 1120
    "..", "..", "..", "..", "..",
    "weather situation improved",
    "message cancelled",
    "winter storm (visibility reduced to Q)",
    "..",
    "blizzard (visibility reduced to Q)",                       // 1130
    "..",
    "damaging hail (visibility reduced to Q)",
    "..",
    "heavy snowfall. Visibility reduced (to Q)",
    "snowfall. Visibility reduced (to Q)",
    "heavy rain. Visibility reduced (to Q)",
    "rain. Visibility reduced (to Q)",
    "..", "..", "..",                                   // 1140
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1150
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1160
    "..", "..", "..", "..", "..", "..", "..", "..", "..",
    "heavy snowfall (Q) expected",                          // 1170
    "heavy rain (Q) expected",
    "weather expected to improve",
    "blizzard (with visibility reduced to Q) expected",
    "damaging hail (with visibility reduced to Q) expected",
    "reduced visibility (to Q) expected",
    "freezing fog expected (with visibility reduced to Q). Danger of slippery roads",
    "dense fog (with visibility reduced to Q) expected",
    "patchy fog (with visibility reduced to Q) expected",
    "visibility expected to improve",
    "adverse weather warning withdrawn",                        // 1180
    "..", "..", "..", "..", "..", "..", "..", "..", "..",
    "severe smog",                                  // 1190
    "severe exhaust pollution",
    "..", "..", "..", "..", "..", "..", "..", "..", "..",               // 1200
    "tornadoes",
    "hurricane force winds (Q)",
    "gales (Q)",
    "storm force winds (Q)",
    "strong winds (Q)",
    "..", "..", "..",
    "gusty winds (Q)",
    "crosswinds (Q)",                                   // 1210
    "strong winds (Q) affecting high-sided vehicles",
    "closed for high-sided vehicles due to strong winds (Q)",
    "strong winds easing",
    "message cancelled",
    "restrictions for high-sided vehicles lifted",
    "..",
    "tornado warning ended",
    "..", "..", "..",                                   // 1220
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1230
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1240
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1250
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1260
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1270
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1280
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1290
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1300
    "dense fog (visibility reduced to Q)",
    "dense fog. Visibility reduced to < ?? m",
    "dense fog. Visibility reduced to < ?? m",
    "fog (visibility reduced to Q)",
    "fog. Visibility reduced to < ?? m",
    "..",
    "patchy fog (visibility reduced to Q)",
    "freezing fog (visibility reduced to Q)",
    "smoke hazard (visibility reduced to Q)",
    "blowing dust (visibility reduced to Q)",                       // 1310
    "..",
    "snowfall and fog (visibility reduced to Q)",
    "visibility improved",
    "message cancelled",
    "..", "..", "..",
    "visibility reduced (to Q)",
    "visibility reduced to < ?? m",
    "visibility reduced to < ?? m",                         // 1320
    "visibility reduced to < ?? m",
    "white out (visibility reduced to Q)",
    "blowing snow (visibility reduced to Q)",
    "spray hazard (visibility reduced to Q)",
    "low sun glare",
    "sandstorms (visibility reduced to Q)",
    "..", "..", "..", "..",                             // 1330
    "..",
    "smog alert",
    "..", "..", "..", "..",
    "freezing fog (visibility reduced to Q). Slippery roads",
    "no motor vehicles due to smog alert",
    "..",
    "swarms of insects (visibility reduced to Q)",                  // 1340
    "..", "..", "..", "..",
    "fog clearing",
    "fog forecast withdrawn",
    "..", "..", "..", "..",                             // 1350
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1360
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1370
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1380
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1390
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1400
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1410
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1420
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1430
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1440
    "..", "..", "..", "..", "..", "..", "..", "..", "..",
    "international sports meeting",                         // 1450
    "match",
    "tournament",
    "athletics meeting",
    "ball game",
    "boxing tournament",
    "bull fight",
    "cricket match",
    "cycle race",
    "football match",
    "golf tournament",                                  // 1460
    "marathon",
    "race meeting",
    "rugby match",
    "show jumping",
    "tennis tournament",
    "water sports meeting",
    "winter sports meeting",
    "funfair",
    "trade fair",
    "procession",                                   // 1470
    "sightseers obstructing access",
    "people on roadway",
    "children on roadway",
    "cyclists on roadway",
    "strike",
    "security incident",
    "police checkpoint",
    "terrorist incident",
    "gunfire on roadway, danger",
    "civil emergency",                                  // 1480
    "air raid, danger",
    "people on roadway. Danger",
    "children on roadway. Danger",
    "cyclists on roadway. Danger",
    "closed due to security incident",
    "security incident. Delays (Q)",
    "security incident. Delays (Q) expected",
    "security incident. Long delays (Q)",
    "police checkpoint. Delays (Q)",
    "police checkpoint. Delays (Q) expected",                       // 1490
    "police checkpoint. Long delays (Q)",
    "security alert withdrawn",
    "sports traffic cleared",
    "evacuation",
    "evacuation. Heavy traffic",
    "traffic disruption cleared",
    "..", "..", "..", "..",                             // 1500
    "major event",
    "sports event meeting",
    "show",
    "festival",
    "exhibition",
    "fair",
    "market",
    "ceremonial event",
    "state occasion",
    "parade",                                       // 1510
    "crowd",
    "march",
    "demonstration",
    "public disturbance",
    "security alert",
    "bomb alert",
    "major event. Stationary traffic",
    "major event. Danger of stationary traffic",
    "major event. Queuing traffic",
    "major event. Danger of queuing traffic",                       // 1520
    "major event. Slow traffic",
    "major event. Slow traffic expected",
    "major event. Heavy traffic",
    "major event. Heavy traffic expected",
    "major event. Traffic flowing freely",
    "major event. Traffic building up",
    "closed due to major event",
    "major event. Delays (Q)",
    "major event. Delays (Q) expected",
    "major event. Long delays (Q)",                         // 1530
    "sports meeting. Stationary traffic",
    "sports meeting. Danger of stationary traffic",
    "sports meeting. Queuing traffic",
    "sports meeting. Danger of queuing traffic",
    "sports meeting. Slow traffic",
    "sports meeting. Slow traffic expected",
    "sports meeting. Heavy traffic",
    "sports meeting. Heavy traffic expected",
    "sports meeting. Traffic flowing freely",
    "sports meeting. Traffic building up",                      // 1540
    "closed due to sports meeting",
    "sports meeting. Delays (Q)",
    "sports meeting. Delays (Q) expected",
    "sports meeting. Long delays (Q)",
    "fair. Stationary traffic",
    "fair. Danger of stationary traffic",
    "fair. Queuing traffic",
    "fair. Danger of queuing traffic",
    "fair. Slow traffic",
    "fair. Slow traffic expected",                          // 1550
    "fair. Heavy traffic",
    "fair. Heavy traffic expected",
    "fair. Traffic flowing freely",
    "fair. Traffic building up",
    "closed due to fair",
    "fair. Delays (Q)",
    "fair. Delays (Q) expected",
    "fair. Long delays (Q)",
    "closed due to parade",
    "parade. Delays (Q)",                               // 1560
    "parade. Delays (Q) expected",
    "parade. Long delays (Q)",
    "closed due to strike",
    "strike. Delays (Q)",
    "strike. Delays (Q) expected",
    "strike. Long delays (Q)",
    "closed due to demonstration",
    "demonstration. Delays (Q)",
    "demonstration. Delays (Q) expected",
    "demonstration. Long delays (Q)",                           // 1570
    "security alert. Stationary traffic",
    "security alert. Danger of stationary traffic",
    "security alert. Queuing traffic",
    "security alert. Danger of queuing traffic",
    "security alert. Slow traffic",
    "security alert. Slow traffic expected",
    "security alert. Heavy traffic",
    "security alert. Heavy traffic expected",
    "security alert. Traffic building up",
    "closed due to security alert",                         // 1580
    "security alert. Delays (Q)",
    "security alert. Delays (Q) expected",
    "security alert. Long delays (Q)",
    "traffic has returned to normal",
    "message cancelled",
    "security alert. Traffic flowing freely",
    "air raid warning cancelled",
    "civil emergency cancelled",
    "message cancelled",
    "several major events",                             // 1590
    "information about major event no longer valid",
    "..", "..", "..", "..", "..", "..", "..", "..", "..",               // 1600
    "delays (Q)",
    "delays up to ?? minutes",
    "delays up to ?? minutes",
    "delays up to one hour",
    "delays up to two hours",
    "delays of several hours",
    "delays (Q) expected",
    "long delays (Q)",
    "delays (Q) for heavy vehicles",
    "delays up to ?? minutes for heavy lorr(y/ies)",                    // 1610
    "delays up to ?? minutes for heavy lorr(y/ies)",
    "delays up to one hour for heavy lorr(y/ies)",
    "delays up to two hours for heavy lorr(y/ies)",
    "delays of several hours for heavy lorr(y/ies)",
    "service suspended (until Q)",
    "(Q) service withdrawn",
    "(Q) service(s) fully booked",
    "(Q) service(s) fully booked for heavy vehicles",
    "normal services resumed",
    "message cancelled",                                // 1620
    "delays up to ?? minutes",
    "delays up to ?? minutes",
    "delays up to ?? minutes",
    "delays up to ?? minutes",
    "delays up to ?? minutes",
    "delays up to ?? minutes",
    "delays up to ?? minutes",
    "delays up to three hours",
    "delays up to four hours",
    "delays up to five hours",                              // 1630
    "very long delays (Q)",
    "delays of uncertain duration",
    "delayed until further notice",
    "cancellations",
    "park and ride service not operating (until Q)",
    "special public transport services operating (until Q)",
    "normal services not operating (until Q)",
    "rail services not operating (until Q)",
    "bus services not operating (until Q)",
    "shuttle service operating (until Q)",                      // 1640
    "free shuttle service operating (until Q)",
    "delays (Q) for heavy lorr(y/ies)",
    "delays (Q) for buses",
    "(Q) service(s) fully booked for heavy lorr(y/ies)",
    "(Q) service(s) fully booked for buses",
    "next departure (Q) for heavy lorr(y/ies)",
    "next departure (Q) for buses",
    "delays cleared",
    "rapid transit service not operating (until Q)",
    "delays (Q) possible",                              // 1650
    "underground service not operating (until Q)",
    "cancellations expected",
    "long delays expected",
    "very long delays expected",
    "all services fully booked (until Q)",
    "next arrival (Q)",
    "rail services irregular. Delays (Q)",
    "bus services irregular. Delays (Q)",
    "underground services irregular",
    "normal public transport services resumed",                     // 1660
    "ferry service not operating (until Q)",
    "park and ride trip time (Q)",
    "delay expected to be cleared",
    "..", "..", "..", "..", "..", "..", "..",                       // 1670
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1680
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1690
    "..", "..", "..", "..",
    "current trip time (Q)",
    "expected trip time (Q)",
    "..", "..", "..",
    "(Q) slow moving maintenance vehicle(s)",                       // 1700
    "(Q) vehicle(s) on wrong carriageway",
    "dangerous vehicle warning cleared",
    "message cancelled",
    "(Q) reckless driver(s)",
    "(Q) prohibited vehicle(s) on the roadway",
    "(Q) emergency vehicles",
    "(Q) high-speed emergency vehicles",
    "high-speed chase (involving Q vehicles)",
    "spillage occurring from moving vehicle",
    "objects falling from moving vehicle",                      // 1710
    "emergency vehicle warning cleared",
    "road cleared",
    "..", "..", "..", "..", "..", "..", "..",
    "rail services irregular",                              // 1720
    "public transport services not operating",
    "..", "..", "..", "..", "..", "..", "..", "..", "..",               // 1730
    "(Q) abnormal load(s), danger",
    "(Q) wide load(s), danger",
    "(Q) long load(s), danger",
    "(Q) slow vehicle(s), danger",
    "(Q) track-laying vehicle(s), danger",
    "(Q) vehicle(s) carrying hazardous materials. Danger",
    "(Q) convoy(s), danger",
    "(Q) military convoy(s), danger",
    "(Q) overheight load(s), danger",
    "abnormal load causing slow traffic. Delays (Q)",                   // 1740
    "convoy causing slow traffic. Delays (Q)",
    "..", "..", "..", "..", "..", "..", "..", "..", "..",               // 1750
    "(Q) abnormal load(s)",
    "(Q) wide load(s)",
    "(Q) long load(s)",
    "(Q) slow vehicle(s)",
    "(Q) convoy(s)",
    "abnormal load. Delays (Q)",
    "abnormal load. Delays (Q) expected",
    "abnormal load. Long delays (Q)",
    "convoy causing delays (Q)",
    "convoy. Delays (Q) expected",                          // 1760
    "convoy causing long delays (Q)",
    "exceptional load warning cleared",
    "message cancelled",
    "(Q) track-laying vehicle(s)",
    "(Q) vehicle(s) carrying hazardous materials",
    "(Q) military convoy(s)",
    "(Q) abnormal load(s). No overtaking",
    "Vehicles carrying hazardous materials have to stop at next safe place!",
    "hazardous load warning cleared",
    "convoy cleared",                                   // 1770
    "warning cleared",
    "..", "..", "..", "..", "..", "..", "..", "..", "..",               // 1780
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1790
    "..", "..", "..", "..", "..", "..", "..", "..", "..", "..",             // 1800
    "lane control signs not working",
    "emergency telephones not working",
    "emergency telephone number not working",
    "(Q sets of) traffic lights not working",
    "(Q sets of) traffic lights working incorrectly",
    "level crossing failure",
    "(Q sets of) traffic lights not working. Stationary traffic",
    "(Q sets of) traffic lights not working. Danger of stationary traffic",
    "(Q sets of) traffic lights not working. Queuing traffic",
    "(Q sets of) traffic lights not working. Danger of queuing traffic",        // 1810
    "(Q sets of) traffic lights not working. Slow traffic",
    "(Q sets of) traffic lights not working. Slow traffic expected",
    "(Q sets of) traffic lights not working. Heavy traffic",
    "(Q sets of) traffic lights not working. Heavy traffic expected",
    "(Q sets of) traffic lights not working. Traffic flowing freely",
    "(Q sets of) traffic lights not working. Traffic building up",
    "traffic lights not working. Delays (Q)",
    "traffic lights not working. Delays (Q) expected",
    "traffic lights not working. Long delays (Q)",
    "level crossing failure. Stationary traffic",                   // 1820
    "level crossing failure. Danger of stationary traffic",
    "level crossing failure. Queuing traffic",
    "level crossing failure. Danger of queuing traffic",
    "level crossing failure. Slow traffic",
    "level crossing failure. Slow traffic expected",
    "level crossing failure. Heavy traffic",
    "level crossing failure. Heavy traffic expected",
    "level crossing failure. Traffic flowing freely",
    "level crossing failure. Traffic building up",
    "level crossing failure. Delays (Q)",                       // 1830
    "level crossing failure. Delays (Q) expected",
    "level crossing failure. Long delays (Q)",
    "electronic signs repaired",
    "emergency call facilities restored",
    "traffic signals repaired",
    "level crossing now working normally",
    "message cancelled",
    "lane control signs working incorrectly",
    "lane control signs operating",
    "variable message signs not working",                       // 1840
    "variable message signs working incorrectly",
    "variable message signs operating",
    "(Q sets of) ramp control signals not working",
    "(Q sets of) ramp control signals working incorrectly",
    "(Q sets of) temporary traffic lights not working",
    "(Q sets of) temporary traffic lights working incorrectly",
    "traffic signal control computer not working",
    "traffic signal timings changed",
    "tunnel ventilation not working",
    "lane control signs not working. Danger",                       // 1850
    "temporary width limit (Q)",
    "temporary width limit lifted",
    "..",
    "traffic regulations have been changed",
    "less than parking spaces available",
    "no parking information available (until Q)",
    "message cancelled",
    "..", "..", "..",                                   // 1860
    "temporary height limit (Q)",
    "temporary height limit lifted",
    "..",
    "lane control signs working incorrectly. Danger",
    "emergency telephones out of order. Extra police patrols in operation",
    "emergency telephones out of order. In emergency, wait for police patrol",
    "(Q sets of) traffic lights not working. Danger",
    "traffic lights working incorrectly. Delays (Q)",
    "traffic lights working incorrectly. Delays (Q) expected",
    "traffic lights working incorrectly. Long delays (Q)",              // 1870
    "temporary axle load limit (Q)",
    "temporary gross weight limit (Q)",
    "temporary gross weight limit lifted",
    "temporary axle weight limit lifted",
    "(Q sets of) traffic lights working incorrectly. Danger",
    "temporary traffic lights not working. Delays (Q)",
    "temporary traffic lights not working. Delays (Q) expected",
    "temporary traffic lights not working. Long delays (Q)",
    "(Q sets of) temporary traffic lights not working. Danger",
    "traffic signal control computer not working. Delays (Q)",              // 1880
    "temporary length limit (Q)",
    "temporary length limit lifted",
    "message cancelled",
    "traffic signal control computer not working. Delays (Q) expected",
    "traffic signal control computer not working. Long delays (Q)",
    "normal parking restrictions lifted",
    "special parking restrictions in force",
    "10% full",
    "20% full",
    "30% full",                                     // 1890
    "40% full",
    "50% full",
    "60% full",
    "70% full",
    "80% full",
    "90% full",
    "less than ?? parking spaces available",
    "less than ?? parking spaces available",
    "less than ?? parking spaces available",
    "less than ?? parking spaces available",                        // 1900
    "next departure (Q)",
    "next departure (Q) for heavy vehicles",
    "car park (Q) full",
    "all car parks (Q) full",
    "less than (Q) car parking spaces available",
    "park and ride service operating (until Q)",
    "(null event)",
    "switch your car radio (to Q)",
    "alarm call: important new information on this frequency follows now in normal",
    "alarm set: new information will be broadcast between these times in normal",   // 1910
    "message cancelled",
    "..",
    "switch your car radio (to Q)",
    "no information available (until Q)",
    "this message is for test purposes only (number Q), please ignore",
    "no information available (until Q) due to technical problems",
    "..",
    "full",
    "..",
    "only a few parking spaces available",                      // 1920
    "(Q) parking spaces available",
    "expect car park to be full",
    "expect no parking spaces available",
    "multi story car parks full",
    "no problems to report with park and ride services",
    "no parking spaces available",
    "no parking (until Q)",
    "special parking restrictions lifted",
    "urgent information will be given (at Q) on normal programme broadcasts",
    "this TMC-service is not active (until Q)",                     // 1930
    "detailed information will be given (at Q) on normal programme broadcasts",
    "detailed information is provided by another TMC service",
    "..",
    "no park and ride information available (until Q)",
    "..", "..", "..",
    "park and ride information service resumed",
    "..",
    "additional regional information is provided by another TMC service",       // 1940
    "additional local information is provided by another TMC service",
    "additional public transport information is provided by another TMC service",
    "national traffic information is provided by another TMC service",
    "this service provides major road information",
    "this service provides regional travel information",
    "this service provides local travel information",
    "no detailed regional information provided by this service",
    "no detailed local information provided by this service",
    "no cross-border information provided by this service",
    "information restricted to this area",                      // 1950
    "no new traffic information available (until Q)",
    "no public transport information available",
    "this TMC-service is being suspended (at Q)",
    "active TMC-service will resume (at Q)",
    "reference to audio programmes no longer valid",
    "reference to other TMC services no longer valid",
    "previous announcement about this or other TMC services no longer valid",
    "..", "..", "..",                                   // 1960
    "allow emergency vehicles to pass in the carpool lane",
    "carpool lane available for all vehicles",
    "police directing traffic via the carpool lane",
    "..", "..", "..", "..", "..", "..", "..",                       // 1970
    "police directing traffic",
    "buslane available for all vehicles",
    "police directing traffic via the buslane",
    "allow emergency vehicles to pass",
    "..", "..",
    "allow emergency vehicles to pass in the heavy vehicle lane",
    "heavy vehicle lane available for all vehicles",
    "police directing traffic via the heavy vehicle lane",
    "..",                                       // 1980
    "..",
    "buslane closed",
    "..", "..", "..", "..", "..", "..", "..", "..",                 // 1990
    "..", "..", "..", "..", "..", "..", "..", "..", "..",
    "closed due to smog alert (until Q)",                       // 2000
    "..", "..", "..", "..", "..", 
    "closed for vehicles with less than three occupants {not valid for lorries}",
    "closed for vehicles with only one occupant {not valid for lorries}",
    "..", "..", "..",                                   // 2010
    "..", "..",
    "service area busy",
    "..", "..", "..", "..", "..", "..", "..",                       // 2020
    "service not operating, substitute service available",
    "public transport strike",
    "..", "..", "..", "..", "..",
    "message cancelled",
    "message cancelled",
    "message cancelled",                                // 2030
    "..", "..",
    "message cancelled",
    "message cancelled",
    "message cancelled",
    "..", "..",
    "message cancelled",
    "message cancelled",
    "message cancelled",                                // 2040
    "..", "..", "..", "..", "..", "..",
    "(null message)",                                   // last = 2047
};

const char *tmc_mglabel[16] = {
    "Duration",
    "Control code",
    "Length",
    "Speed limit",
    "Quantifier",
    "Quantifier",
    "Info code",
    "Start time",
    "Stop time",
    "Event",
    "Diversion",
    "Location",
    "unknown",
    "Location",
    "NOP",
    "unknown",
};
int tmc_mgsize[16] = { 3, 3, 5, 5, 5, 8, 8, 8, 8, 11, 16, 16, 16, 16, 0, 0 };
    
// TMC, Alert-C Coding
void tmc_parser(unsigned char *data, int len)   
{
    static char lastdata[6];
    
    if (len < 6) {
    printf("TMC Length only '%d' bytes (<6).\n", len);
    return;
        }

    if (memcmp(data, lastdata, 6) == 0) {
    printf("TMC Repeating.\n");
    return; 
        }
    memcpy(lastdata, data, 6);
    
    // Buffer = data[0], todo or not :D
    
    // check Encrypted-Service, TMC Pro ?
    if ((data[1] & 0x1f) == 0x00) { // Type+DP = '00000'
    printf("TMC Encrypted Service detected, TMC-Pro?\n");
    return; 
        }

    int type = (data[1] & 0x18)>>3;     // Type = User-,TuningInformation & Multi-,Singlegroup Message
    int dp = data[1] & 0x07;            // Duration+Persistance or Continuity Index
    int da = (data[2] & 0x80)>>7;       // DiversionAdvice or GroupIndicator
    int di = (data[2] & 0x40)>>6;       // Direction (-/+) or 2.GroupIndicator
    int ex = (data[2] & 0x38)>>3;       // Extent
    int ev = (data[2] & 0x07)<<8 | data[3]; // Event
    int lo = data[4]<<8 | data[5];      // Location
    
    switch (type) {
    case 0: // Multigroup-Message
            printf("TMC Multi-Group Message, ");
            if (da == 1) {
            printf("First:\n");
            printf("    CI: '%d', Direction: %s, Extent: '%d'\n", dp, tmc_direction[di], ex);
            printf("    Event:    '%d' = %s\n", ev, tmc_event[ev]);
            printf("    Location: '%d' > LT not available yet :-(\n", lo);
                }
            else {
            int gsi = (data[2] & 0x30)>>4;          // GroupSequenceIdentifier
            printf("Subsequent:\n");
            printf("    CI: '%d', 2.GI: '%d', GSI: '%d', Block_0x: '%02x%02x%02x%02x'\n", dp, di, gsi, data[2]&0xf, data[3], data[4], data[5]);
            if (di == 0) {
                printf("    SecondGroupIndicator = 0 -> todo, exit here.\n\n");
                return;
               }            
            unsigned int block = (data[2]&0x0f)<<24 | data[3]<<16 | data[4]<<8 | data[5];
            int lc = 1;
            int rbits = 28;
            while (rbits > 0) {
            int lb = block>>(rbits-4);
            rbits -= 4;
            if (lb <= 0)
                return;
            block = block & ((unsigned long int)(pow(2,rbits)) - 1);
            rbits -= tmc_mgsize[lb];
            int val = block>>(rbits);
            printf("    #%d: Label '%02d' = %s", lc, lb, tmc_mglabel[lb]);
            if (val > 0) {
                    switch (lb) {
                    case 0:  printf(", Value '%d' min.?\n", val);
                     break;
                    case 2:  printf(", Value '%d' km?\n", val);
                         break;
                    case 3:  printf(", Value '%d' km/h?\n", val);
                         break;
                    case 9:  printf(", Value '%d' = %s\n", val, tmc_event[val]);
                     break;
                    case 11:
                    case 13: printf(", Value '%d' > LT not available yet :-(\n", val);
                     break;
                    case 14:  
                    case 15: printf("  ---\n");
                     break;
                    default: printf(", Value '%d'\n", val);
                        }
                    }
                else {
                if (block > 0) 
                    printf(", rest block_0x '%04x'\n", (int)block);
                else
                    printf(", ...\n");
                    }
                block = block & ((unsigned int)(pow(2,rbits)) - 1);
                lc++;
                }
            }
        break;
    case 1: // Singlegroup-Message
        printf("TMC Single-Group Message:\n");
        printf("    Duration: %s, Diversion: '%d', Direction: %s, Extent: '%d'\n", tmc_duration[dp], da, tmc_direction[di], ex);
        printf("    Event:    '%d' = %s\n", ev, tmc_event[ev]);
        printf("    Location: '%d' > LT not available yet :-(\n", lo);
            break;  
    case 2:
    case 3: // System,Tuning
        printf("TMC Tuning/System Information:\n");
        switch (data[1] & 0x0f) {
            case 9:  printf("    LTN: '%d', MGS: '%d', SID: '%d' %04x.\n", data[2]>>2, (data[2] & 0x03)<<2 | data[3]>>6, data[3] & 0x3f, lo);
                 break;
            default: printf("    todo, exit.\n");
                }
        }

}


//--------------- End -----------------------------------------------------------------
