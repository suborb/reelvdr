/*************************************************************** -*- c++ -*-
 *       Copyright (c) 2003,2004 by Marcel Wiesweg                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __PACKEDSTORAGE_H
#define __PACKEDSTORAGE_H

#include "legacystorage.h"

class PackedStorage : public LegacyStorage {
public:
   PackedStorage(int maxMB);

   virtual void getFilename(char *buffer, int bufLength, PageID page);
   virtual StorageHandle openForWriting(PageID page);
   virtual StorageHandle openForReading(PageID page, bool countAsAccess);
protected:
   struct PageAddress {
      bool operator==(const PageID &id) const
        { return page==id.page && subPage==id.subPage; }
      void operator=(const PageID &id)
        { page=id.page; subPage=id.subPage; }
      int page;
      int subPage;
   };
   bool seekTo(PageID page, int fd, bool create);
   void registerFile(PageID page);
};

#endif
