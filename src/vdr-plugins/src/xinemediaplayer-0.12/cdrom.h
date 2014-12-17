/** -*- adrian-c -*-
     kover - Kover is an easy to use WYSIWYG CD cover printer with CDDB support.
     Copyright (C) 2001 by Adrian Reber 
     
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
     
     File: cdrom.h 
     
     Description: cdrom class header
     
     Changes:
     
     24 Apr 2001: Initial release
     
*/

#ifndef CDROM_H
#define CDROM_H

extern "C" {

//#include "../config.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef __FreeBSD__
#include <sys/cdio.h>
#endif

//#ifdef HAVE_LINUX_CDROM_H
//assuming this header is always present
#include <linux/cdrom.h>
//#endif

#ifdef HAVE_LINUX_UCDROM_H
#include <linux/ucdrom.h>
#endif

}

class cdrom {
public:
     cdrom(char *_path);
     ~cdrom();
     int open();
     int close();
     int eject();
protected:
     int cdrom_fd;
private:
     char *path;
     bool cdrom_open;
};

#endif
