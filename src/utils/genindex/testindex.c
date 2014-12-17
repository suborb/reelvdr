/*
 * A VDR index tester (C++)
 *
 * (C) 2003-2006 Stefan Huelswitt <s.huelswitt@gmx.de>
 *
 * This code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "file.h"

// Picture types:
#define I_FRAME    1
#define P_FRAME    2
#define B_FRAME    3

// --- MAIN --------------------------------------------------------------------

int main(int argc, char *argv[])
{
  cFileName fileName;
  cIndex index;

  int startNo=1;
  if(argc>1) {
    startNo=atoi(argv[1]);
    if(startNo<1) startNo=1;
    if(startNo>=99) startNo=99;
    }

  if(index.Open(true)) {
    int nr, off, fd;
    unsigned char buff[64];
    while(index.Read(nr,off)) {
      if(nr<startNo) continue;
      fd=fileName.Skip(nr,off);
      printf("offset %d\r",off); fflush(stdout);
      if(fd<0) break;
      int r=read(fd,buff,sizeof(buff));
      if(r<0) printf("read vdr: %s\n",strerror(errno));
      if(buff[0]!=0x00 || buff[1]!=0x00 || buff[2]!=0x01 || (buff[3]&0xF0)!=0xE0) {
        printf("bad packet start file=%d off=%d\n",nr,off);
        break;
        }
      }
    }

  return 0;
}
