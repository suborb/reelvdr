/*************************************************************** -*- c++ -*-
 *       Copyright (c) 2003,2004 by Marcel Wiesweg                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __ROOTDIR_H
#define __ROOTDIR_H

class RootDir {
public:
   static void setRootDir(const char *);
   static const char *getRootDir();
protected:
   static const char *root;
};

#endif
