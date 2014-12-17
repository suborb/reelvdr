//***************************************************************************
/*
 * status.hpp: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
//***************************************************************************

#ifndef __IPOD_STATUS_H__
#define __IPOD_STATUS_H__

#include <vdr/status.h>

//***************************************************************************
// Class Status
//***************************************************************************

class PluginStatus: public cStatus 
{
   public:

      PluginStatus()  { title = 0; current = 0; }

      const char* Title(void)   { return title; }
      const char* Current(void) { return current; }

   protected:

      virtual void OsdTitle(const char* Title)       { title = Title; }
      virtual void OsdCurrentItem(const char* Text)  { current = Text; }

      // data

      const char* title;
      const char* current;
};

//***************************************************************************
#endif // __IPOD_STATUS_H__
