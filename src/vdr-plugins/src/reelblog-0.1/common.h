/*
 * common.h: This reelblog plugin is a special version of the RSS Reader Plugin (Version 1.0.2) for the Video Disk Recorder (VDR).
 *
 * See the README file for copyright information and how to reach the author.
 *
 * Copyright (C) 2008 Heiko Westphal
*/

#ifndef __REELBLOG_COMMON_H
#define __REELBLOG_COMMON_H

#ifdef DEBUG
#define debug(x...) dsyslog("Reelblog: " x);
#define error(x...) esyslog("Reelblog: " x);
#else
#define debug(x...) ;
#define error(x...) esyslog("Reelblog: " x);
#endif

// User agent string for servers
#define REELBLOG_USERAGENT "libcurl-agent/1.0"

#endif // __REELBLOG_COMMON_H

