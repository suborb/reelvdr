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

#define _LARGEFILE64_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include "file.h"

#define INDEX     "index.vdr"
#define INDEX_ALT "index.vdr.generated"
#define FILENAME  "%03d.vdr"

extern bool quiet;

// --- cString ---------------------------------------------------------------

class cString {
private:
  char *s;
public:
  cString(const char *S = NULL, bool TakePointer = false);
  cString(const cString &String);
  virtual ~cString();
  operator const char * () const { return s; } // for use in (const char *) context
  const char * operator*() const { return s; } // for use in (const void *) context (printf() etc.)
  cString &operator=(const cString &String);
  static cString sprintf(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
  };

cString::cString(const char *S, bool TakePointer)
{
  s = TakePointer ? (char *)S : S ? strdup(S) : NULL;
}

cString::cString(const cString &String)
{
  s = String.s ? strdup(String.s) : NULL;
}

cString::~cString()
{
  free(s);
}

cString &cString::operator=(const cString &String)
{
  if (this == &String)
     return *this;
  free(s);
  s = String.s ? strdup(String.s) : NULL;
  return *this;
}

cString cString::sprintf(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  char *buffer;
  vasprintf(&buffer, fmt, ap);
  return cString(buffer, true);
}

// -----------------------------------------------------------------------------

cString AddDirectory(const char *DirName, const char *FileName)
{
  char *buf;
  asprintf(&buf, "%s/%s", DirName && *DirName ? DirName : ".", FileName);
  return cString(buf, true);
}

// --- cIndex ------------------------------------------------------------------

cIndex::cIndex(const char *Dir)
{
  dir=Dir; f=0;
}

cIndex::~cIndex()
{
  if(f) fclose(f);
}

bool cIndex::Open(bool Read)
{
  int r;
  cString name=AddDirectory(dir,INDEX);
  r=access(name,F_OK);
  read=Read;
  if(!Read) {
    if(r==0) {
      if(!quiet) printf("There is an existing index file. Trying alternative filename.\n");
      name=AddDirectory(dir,INDEX_ALT);
      r=access(name,F_OK);
      if(r==0) printf("Alternative index file exists too. Aborting.\n");
      }
    if(r<0) {
      if(errno==ENOENT) {
        f=fopen(name,"w");
        if(f) return true;
        else printf("index file open: %s\n",strerror(errno));
        }
      else printf("index file check: %s\n",strerror(errno));
      }
    }
  else {
    if(r==0) {
      f=fopen(name,"r");
      if(f) return true;
      else printf("index file open: %s\n",strerror(errno));
      }
    else printf("index file check: %s\n",strerror(errno));
    }
  return false;
}

bool cIndex::Write(int fileNr, int fileOff, int picType)
{
  if(f && !read) {
    struct tIndex in;
    in.offset=fileOff;
    in.number=fileNr;
    in.type=picType;
    in.reserved=0;
    if(fwrite(&in,sizeof(in),1,f)==1) return true;
    else printf("index write failed\n");
    }
  else printf("internal: no open write index file\n");
  return false;
}

bool cIndex::Read(int &fileNr, int &fileOff)
{
  if(f && read) {
    struct tIndex in;
    if(fread(&in,sizeof(in),1,f)==1) {
      fileOff=in.offset;
      fileNr=in.number;
      return true;
      }
    else if(!feof(f)) printf("index read failed\n");
    }
  else printf("internal: no open read index file\n");
  return false;
}

// --- cFileName ---------------------------------------------------------------

cFileName::cFileName(const char *Input, bool LargeOK)
{
  input=Input; largeOK=LargeOK;
  fd=-1;
  fileno=1;
}

cFileName::~cFileName()
{
  close(fd);
}

int cFileName::Open(void)
{
  close(fd); fd=-1;
  cString filename;
  if(input) filename=input;
  else filename=cString::sprintf(FILENAME,fileno);
  fd=open64(filename,O_RDONLY);
  if(fd>=0) {
    struct stat64 st;
    if(fstat64(fd,&st)==0) {
      if(!(st.st_size>>31) || largeOK) {
        size=st.st_size;
        if(!quiet) printf("reading from %s     \n",*filename);
        }
      else {
        printf("%s: filesize exceeds 2GB limit\n",*filename);
        close(fd); fd=-1;
        }
      }
    else {
      printf("fstat: %s\n",strerror(errno));
      close(fd); fd=-1;
      }
    }
  else if(fileno==1 || errno!=ENOENT)
    printf("open %s: %s\n",*filename,strerror(errno));
  return fd;
}

int cFileName::OpenWrite(void)
{
  close(fd); fd=-1;
  cString filename=AddDirectory(input,cString::sprintf(FILENAME,fileno));
  if(access(filename,F_OK)!=0) {
    fd=open64(filename,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd>=0) {
      size=0;
      if(!quiet) printf("writing to %s       \n",*filename);
      }
    else printf("open %s: %s\n",*filename,strerror(errno));
    }
  else printf("%s: file already exists\n",*filename);
  return fd;
}

int cFileName::NextFile(void)
{
  if(!input) {
    fileno++;
    return Open();
    }
  return -1;
}

int cFileName::NextWriteFile(void)
{
  fileno++;
  return OpenWrite();
}

int cFileName::Skip(int nr, int off)
{
  if(fileno!=nr || fd<0) {
    fileno=nr;
    if(Open()<0) return -1;
    }
  if(lseek(fd,off,SEEK_SET)<0) {
    printf("seek: %s\n",strerror(errno));
    return -1;
    }
  return fd;
}
