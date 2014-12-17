/*
 * videodir.h: Functions to maintain a distributed video directory
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: videodir.h 2.0 2008/02/16 12:53:11 kls Exp $
 */

#ifndef __VIDEODIR_H
#define __VIDEODIR_H

#include <stdlib.h>
#include "tools.h"

extern const char *VideoDirectory;
#ifdef USE_LIVEBUFFER
extern const char *BufferDirectory;
extern int BufferDirectoryMinFree;
#endif /*USE_LIVEBUFFER*/

cUnbufferedFile *OpenVideoFile(const char *FileName, int Flags);
int CloseVideoFile(cUnbufferedFile *File);
bool RenameVideoFile(const char *OldName, const char *NewName);
bool RemoveVideoFile(const char *FileName);
#ifdef USE_HARDLINKCUTTER
bool HardLinkVideoFile(const char *OldName, const char *NewName);
#endif /* HARDLINKCUTTER */
bool VideoFileSpaceAvailable(int SizeMB);
int VideoDiskSpace(int *FreeMB = NULL, int *UsedMB = NULL); // returns the used disk space in percent
cString PrefixVideoFileName(const char *FileName, char Prefix);
void RemoveEmptyVideoDirectories(void);
bool IsOnVideoDirectoryFileSystem(const char *FileName);

#endif //__VIDEODIR_H
