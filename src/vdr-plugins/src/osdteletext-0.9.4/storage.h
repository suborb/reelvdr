/*************************************************************** -*- c++ -*-
 *       Copyright (c) 2003,2004 by Marcel Wiesweg                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __STORAGE_H
#define __STORAGE_H

#include "rootdir.h"
#include "pageid.h"

#define TELETEXT_PAGESIZE 972

struct StorageHandle {
public:
   StorageHandle() { handle=-1; }
   StorageHandle(const StorageHandle &s) { handle=s.handle; }
   StorageHandle(int h) { handle=h; }
   StorageHandle &operator=(int h) { handle=h; return *this; }
   StorageHandle &operator=(const StorageHandle &s) { handle=s.handle; return *this; }
   operator bool() const { return handle!=-1; }
   operator int() const { return handle; }
private:
   int handle;
};

enum StorageSystem { StorageSystemLegacy, StorageSystemPacked };

class Storage : public RootDir {
public:
   virtual ~Storage();
   enum StorageSystem { StorageSystemLegacy, StorageSystemPacked };
   //must be called before the first call to instance()

   //must be called before operation starts. Set all options (RootDir, maxStorage) before.
   virtual void cleanUp() = 0;

   virtual void getFilename(char *buffer, int bufLength, PageID page);
   virtual void prepareDirectory(tChannelID chan);

   virtual StorageHandle openForWriting(PageID page) = 0;
   virtual StorageHandle openForReading(PageID page, bool countAsAccess) = 0;
   virtual ssize_t write(const void *ptr, size_t size, StorageHandle stream) = 0;
   virtual ssize_t read(void *ptr, size_t size, StorageHandle stream) = 0;
   virtual void close(StorageHandle stream) = 0;
protected:
   Storage();
   int cleanSubDir(const char *dir);
   int doCleanUp();
   virtual int actualFileSize(int netFileSize) { return netFileSize; }
   void freeSpace();
   bool exists(const char* file);

   long byteCount;
   cString currentDir;
private:
   bool failedFreeSpace;
};

#endif
