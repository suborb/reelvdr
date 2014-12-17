/*************************************************************** -*- c++ -*-
 *       Copyright (c) 2003,2004 by Marcel Wiesweg                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <sys/vfs.h>

#include "legacystorage.h"

LegacyStorage::LegacyStorage(int maxMB)
  : fsBlockSize(1), pageBytes(TELETEXT_PAGESIZE)
{
   initMaxStorage(maxMB);
}

LegacyStorage::~LegacyStorage() {
}

/*
static inline int FilesForMegabytes(double MB, int blocksize) {
   double pageBytes;
   if (TELETEXT_PAGESIZE<=blocksize)
      pageBytes=blocksize;
   else
      pageBytes=((TELETEXT_PAGESIZE/blocksize)+1)*blocksize;
   //reserve 10% for directories
   return (int)( (1024.0 * 1024.0 * (MB-MB*0.1)) / pageBytes );
}*/

int LegacyStorage::actualFileSize(int netFileSize) {
   if (netFileSize<=0)
      return 0;
   if (netFileSize<=fsBlockSize)
      return fsBlockSize;
   else
      return ((netFileSize/fsBlockSize)+1)*fsBlockSize;
}

//max==0 means unlimited, max==-1 means a reasonable default value shall be calculated
void LegacyStorage::initMaxStorage(int maxMB) {
   struct statfs fs;
   if (statfs(getRootDir(), &fs)!=0) {
      esyslog("OSD-Teletext: Error statfs'ing root directory \"%s\": %s, cache size uncontrolled", getRootDir(), strerror(errno));
      return;
   }
   fsBlockSize=fs.f_bsize;

   pageBytes=actualFileSize(TELETEXT_PAGESIZE);

   if (maxMB>=0) {
      if (maxMB<3) {
         esyslog("OSD-Teletext: Request to use at most %d MB for caching. This is not enough, using 3 MB", maxMB);
         maxMB=3;
      }
      maxBytes=MEGABYTE(maxMB);
   } else if (maxMB==-1) {
      //calculate a default value
      double blocksPerMeg = 1024.0 * 1024.0 / fs.f_bsize;
      double capacityMB=fs.f_blocks / blocksPerMeg;
      double freeMB=(fs.f_bavail / blocksPerMeg);
      if (capacityMB<=50 || freeMB<50) {
         //small (<=50MB) filesystems as root dir are assumed to be dedicated for use as a teletext cache
         //for others, the maximum default size is set to 50 MB
         maxBytes=MEGABYTE((int)freeMB);
         //maxPages= FilesForMegabytes(freeMB, fs.f_bsize);
         if (freeMB<3.0) {
            esyslog("OSD-Teletext: Less than %.1f MB free on filesystem of root directory \"%s\"!", freeMB, getRootDir());
            maxBytes=MEGABYTE(3);
         }
      } else {
         //the maximum default size is set to 50 MB
         maxBytes=MEGABYTE(50);
      }
      //printf("Set maxBytes to %ld, %.2f %.2f\n", maxBytes, capacityMB, freeMB);
   }
}

void LegacyStorage::cleanUp() {
   byteCount -= Storage::doCleanUp();
}

void LegacyStorage::registerFile(PageID page) {
   //pageBytes is already effective size
   if ( maxBytes && (byteCount+=pageBytes)>maxBytes )
      freeSpace();
}

StorageHandle LegacyStorage::openForReading(PageID page, bool countAsAccess) {
   //the countAsAccess argument was intended for use in a LRU cache, currently unused
   char filename[PATH_MAX];
   getFilename(filename, sizeof(filename), page);
   StorageHandle ret=(StorageHandle)open(filename, O_RDONLY);
   return ret;
}

StorageHandle LegacyStorage::openForWriting(PageID page) {
   static bool wroteError=false;
   char filename[PATH_MAX];
   getFilename(filename, sizeof(filename), page);
   bool existed=exists(filename);
   //first try
   StorageHandle fd=(StorageHandle)open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
   if (fd) {
      if (!existed)
         registerFile(page);
      return fd;
   }
   //no space on disk? make some space available
   if (errno == ENOSPC)
      freeSpace();
   //second try
   fd=(StorageHandle)open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
   if (!fd && !wroteError) {
      //report error to syslog - once!
      wroteError=true;
      esyslog("OSD-Teletext: Error opening teletext file %s: %s", filename, strerror(errno));
   }
   //make sure newly created files are counted
   if (fd && !existed)
      registerFile(page);
   return fd;
}

ssize_t LegacyStorage::write(const void *ptr, size_t size, StorageHandle stream) {
   ssize_t written;
   if (!(written=::write((int)stream, ptr, size)) ) {
      switch (errno) {
      case ENOSPC:
         freeSpace();
         return ::write((int)stream, ptr, size);
      case EINTR:
         esyslog("OSD-Teletext: EINTR while writing. Please contact the author and tell him this happened.");
         break;
      default:
         break;
      }
   }
   return written;
}
