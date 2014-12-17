/*
 * radiotools.h - part of radio.c, a plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __RADIO_TOOLS_H
#define __RADIO_TOOLS_H


bool file_exists(const char *filename);

bool enforce_directory(const char *path);

unsigned short crc16_ccitt(unsigned char *daten, int len, bool skipfirst);

char *rds_entitychar(char *text);

char *xhtml2text(char *text);

char *rtrim(char *text);

char *audiobitrate(const unsigned char *data);

void tmc_parser(unsigned char *data, int len);		// Alert-c


#endif //__RADIO_TOOLS_H
