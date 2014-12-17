/*
 * tools.h: This reelblog plugin is a special version of the RSS Reader Plugin (Version 1.0.2) for the Video Disk Recorder (VDR).
 *
 * See the README file for copyright information and how to reach the author.
 *
 * Copyright (C) 2008 Heiko Westphal
*/

#ifndef __REELBLOG_TOOLS_H
#define __REELBLOG_TOOLS_H

#include <vdr/tools.h>

int   charsetconv(const char *buffer, int buf_len, const char *str, int str_len, const char *from, const char *to);
char *striphtml(char *str);
void *myrealloc(void *ptr, size_t size);

#endif // __REELBLOG_TOOLS_H

