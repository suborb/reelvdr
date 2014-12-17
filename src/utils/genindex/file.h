/*
 * A VDR index file generator (C++)
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

#ifndef __FILE_H
#define __FILE_H

#include <stdio.h>
#include "tools.h"

// -------------------------------------------------------------------

class cIndex {
private:
  FILE *f;
  bool read;
  const char *dir;
  struct tIndex { int offset; uchar type; uchar number; short reserved; };
public:
  cIndex(const char *Dir);
  ~cIndex();
  bool Open(bool Read=false);
  bool Write(int fileNr, int fileOff, int picType);
  bool Read(int &fileNr, int &fileOff);
  };

// -------------------------------------------------------------------

class cFileName {
private:
  const char *input;
  bool largeOK;
  int fd;
  int fileno;
  long long size;
  char filename[64];
public:
  cFileName(const char *Input, bool LargeOK);
  ~cFileName();
  int Open(void);
  int NextFile(void);
  int OpenWrite(void);
  int NextWriteFile(void);
  int Skip(int nr, int off);
  int FileNumber(void) { return fileno; }
  long long FileSize(void) { return size; }
  };

#endif
