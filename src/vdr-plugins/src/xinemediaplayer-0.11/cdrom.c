/** -*- adrian-c -*-
     kover - Kover is an easy to use WYSIWYG CD cover printer with CDDB support.
     Copyright (C) 2001-2003 by Adrian Reber 
     
     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 2 of the License, or
     (at your option) any later version.
     
     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.
    
     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
     
     File: cdrom.cc
     
     Description: class for all cdrom accesses
     
     Changes:

     24 Apr 2001: Initial release

*/

#include "cdrom.h"

cdrom::cdrom(char *_path) {
     path = strdup(_path);
     cdrom_fd = -42;
     cdrom_open = false;
}

cdrom::~cdrom() {
     close();
     free (path);
}

int cdrom::open() {
     if ((cdrom_fd = ::open(path, O_RDONLY | O_NONBLOCK)) >= 0) {
          cdrom_open = true;
          return 0;
     }
     else 
          return -1;
}

int cdrom::close() {
     if(::close(cdrom_fd) >=0) {
          cdrom_fd = -42;
          cdrom_open = false;
          return 0;
     }
     else
          return -1;
}

int cdrom::eject() {
     if (!cdrom_open) { 
          if (open() < 0)
                return -1;
     }
     if (cdrom_fd > 0)
#ifdef __FreeBSD__
         {
                  ioctl(cdrom_fd,CDIOCALLOW);
                  ioctl(cdrom_fd,CDIOCEJECT);
         }
#else
                 ;
         // ioctl(cdrom_fd,CDROMEJECT);
#endif
     else
          return -1;
     close();
     return 0;
}
