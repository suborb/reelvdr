/*************************************************************** -*- c++ -*-
 *       Copyright (c) 2003,2004 by Marcel Wiesweg                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __PAGEID_H
#define __PAGEID_H

#include <vdr/status.h>
#include <vdr/receiver.h>
#include <vdr/thread.h>
#include <vdr/ringbuffer.h>

#include <stdio.h>
#include <unistd.h>

struct PageID {
   PageID() { page=subPage=0; }
   PageID(tChannelID id, int p, int s) { set(id, p, s); }
   void set(tChannelID id, int p, int s) { channel=id; page=p; subPage=s; }
   tChannelID channel;
   int page;
   int subPage;
};

#endif
