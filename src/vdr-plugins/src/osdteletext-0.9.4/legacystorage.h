/*************************************************************** -*- c++ -*-
 *       Copyright (c) 2003,2004 by Marcel Wiesweg                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __LEGACYSTORAGE_H
#define __LEGACYSTORAGE_H

#include "storage.h"

class LegacyStorage : public Storage {
private:
   void initMaxStorage(int maxMB=-1);
public:
   LegacyStorage(int maxMB);
   virtual ~LegacyStorage();
   virtual void cleanUp();

   virtual StorageHandle openForWriting(PageID page);
   virtual StorageHandle openForReading(PageID page, bool countAsAccess);
   virtual ssize_t write(const void *ptr, size_t size, StorageHandle stream);
   virtual ssize_t read(void *ptr, size_t size, StorageHandle stream)
     { return ::read((int)stream, ptr, size); }
   virtual void close(StorageHandle stream)
     { ::close((int)stream); }
protected:
   void registerFile(PageID page);
   virtual int actualFileSize(int netFileSize);
   //int maxPages;
   long maxBytes;
   int fsBlockSize;
   int pageBytes;
};

#endif
