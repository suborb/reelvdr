/*************************************************************** -*- c++ -*-
 *       Copyright (c) 2003,2004 by Marcel Wiesweg                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "rootdir.h"

const char *RootDir::root = "/var/cache/vdr/vtx";

void RootDir::setRootDir(const char *newRoot) {
   root=newRoot;
}

const char *RootDir::getRootDir() {
   return root;
}
