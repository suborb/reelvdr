/*************************************************************** -*- c++ -*-
 *       Copyright (c) 2003,2004 by Marcel Wiesweg                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "packedstorage.h"

PackedStorage::PackedStorage(int maxMB)
 : LegacyStorage(maxMB) {
}
#define TOC_SIZE 8
//The file structure is simple:
// TOC_SIZE*PageAddress contains the numbers of the following pages
// TOC_SIZE*TELETEXT_PAGESIZE contains the page data
//and the same again.
bool PackedStorage::seekTo(PageID page, int desc, bool create) {
   lseek(desc, 0, SEEK_SET);
   PageAddress addr[TOC_SIZE];

   while (::read(desc, addr, sizeof(addr)) == sizeof(addr)) {
      lseek(desc, 0, SEEK_CUR);
      for (int index=0; index<TOC_SIZE; index++) {
         if (addr[index]==page) {
            lseek(desc, index*TELETEXT_PAGESIZE, SEEK_CUR);
            return true;
         } else if (addr[index].page==0) {
            //0 means: no more pages follow
            if (create) {
               //rewind what was read
               lseek(desc, -(off_t)sizeof(addr), SEEK_CUR);
               //update index
               addr[index]=page;
               if (::write(desc, addr, sizeof(addr)) != sizeof(addr))
                  return false;
               //seek to data position
               lseek(desc, TELETEXT_PAGESIZE*index, SEEK_CUR);
               return true;
            } else
               return false;
         }
      }

      //seek over data area
      lseek(desc, TELETEXT_PAGESIZE*TOC_SIZE, SEEK_CUR);
   }

   int oldSize=actualFileSize(lseek(desc, 0, SEEK_CUR));
   if (create) {
      //create a new set of a TOC and a TOC_SIZE*TELETEXT_PAGESIZE data area
      memset(addr, 0, sizeof(addr));
      //first entry is our page
      addr[0]=page;
      if (::write(desc, addr, sizeof(addr)) != sizeof(addr))
         return false;
      //seek beyond end of file
      lseek(desc, (TELETEXT_PAGESIZE*TOC_SIZE)-1, SEEK_CUR);
      //write one byte to enlarge the file to the sought position
      char c=1;
      if (::write(desc, &c, 1) != 1)
         return false;
      //Now, calculate new file size
      byteCount += ( actualFileSize(lseek(desc, 0, SEEK_CUR)) - oldSize );
      //seek to beginning of data, which is requested
      lseek(desc, -(off_t)(TELETEXT_PAGESIZE*TOC_SIZE), SEEK_CUR);
      return true;
   } else
      return false;
}

void PackedStorage::getFilename(char *buffer, int bufLength, PageID page) {
   //This is a different scheme: page 576_07 will have the name 570s.vtx, the same as e.g. 571_01 or 575_00
   //Think of "the five hundred seventies"
   snprintf(buffer, bufLength, "%s/%s/%03xs.vtx", getRootDir(),
            *page.channel.ToString(), (page.page & 0xFF0));
}

StorageHandle PackedStorage::openForWriting(PageID page) {
   static bool wroteError=false;
   char filename[PATH_MAX];
   getFilename(filename, sizeof(filename), page);
   //first try
   int desc=open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
   if (desc != -1) {
      if (!seekTo(page, desc, true)) {
         ::close(desc);
         return StorageHandle();
      }
      if ( maxBytes && byteCount>maxBytes )
         freeSpace();
      return (StorageHandle)desc;
   }
   //no space on disk? make some space available
   if (errno == ENOSPC)
      freeSpace();
   //second try
   desc=open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
   if (desc==-1 && !wroteError) {
      //report error to syslog - once!
      wroteError=true;
      esyslog("OSD-Teletext: Error opening teletext file %s: %s", filename, strerror(errno));
   }

   if (desc==-1)
      return StorageHandle();
   else if (!seekTo(page, desc, true)) {
      ::close(desc);
      return StorageHandle();
   }

   if ( maxBytes && byteCount>maxBytes )
      freeSpace();
   return (StorageHandle)desc;
}

StorageHandle PackedStorage::openForReading(PageID page, bool countAsAccess) {
   int desc;
   if ( (desc=(int)LegacyStorage::openForReading(page, false))!= -1 ) {
      if (!seekTo(page, desc, false)) {
         //this is not an error condition here, may and shall happen!
         ::close(desc);
      } else {
         return (StorageHandle)desc;
      }
   }
   return StorageHandle();
}
