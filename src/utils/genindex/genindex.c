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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>

#include "file.h"
#include "pes.h"
#include "version.h"

#define MAX_FILE_NR 99

// Picture types:
#define I_FRAME    1
#define P_FRAME    2
#define B_FRAME    3

char *destdir=0;
char *input=0;
int splitsize=2000;
bool rewrite=false;
bool quiet=false;

// --- cGenIndex ---------------------------------------------------------------

class cGenIndex : public cPES {
private:
  cFileName *fileName, *writeName;
  cIndex *index;
  //
  int replayFile, writeFile;
  int fileNo, fileOffset, packetOffset, packetNo;
  bool eof, scan, error;
  int state, skip;
  int readNo;
  long long readOffset;
  long long fileSize[MAX_FILE_NR+1];
  //
  unsigned int sSize;
  int splitOffset;
  bool pstart;
  //
  unsigned char buff[KILOBYTE(256)];
  uchar store[KILOBYTE(66)];
  //
  bool NextFile(void);
  void Count(int len);
  bool SafeWrite(int fd, const unsigned char *data, int len);
protected:
  virtual int Output(const uchar *data, int len);
  virtual int Action1(uchar type, uchar *data, int len);
  virtual int Action2(uchar type, uchar *data, int len);
  virtual void Skipped(uchar *data, int len);
public:
  cGenIndex(void);
  ~cGenIndex();
  void Work(void);
  void Skip(int type);
  };

cGenIndex::cGenIndex(void)
{
  index=new cIndex(destdir);
  fileName=new cFileName(input,input && rewrite);
  writeName=new cFileName(destdir,true);

  if(rewrite) {
    SetDefaultRule(prSkip);
    SetRuleR(0xE0,0xEF,prAct2);
    SetRuleR(0xC0,0xCF,prAct1);
    SetRule(0xBD,prAct1);
    SetRule(0xBE,prAct1);
    }
  else {
    SetDefaultRule(prAct1);
    SetRuleR(0xE0,0xEF,prAct2);
    }
}

cGenIndex::~cGenIndex()
{
  delete fileName;
  delete writeName;
  delete index;
}

void cGenIndex::Skip(int type)
{
  if(rewrite) SetRule(type,prSkip);
}

bool cGenIndex::NextFile(void)
{
  if(replayFile>=0 && eof) {
    replayFile=fileName->NextFile();
    readNo=fileName->FileNumber();
    readOffset=0;
    fileSize[readNo]=fileName->FileSize();
    }
  eof=false;
  return replayFile>=0;
}

void cGenIndex::Work(void)
{
  eof=error=pstart=false;
  memset(fileSize,0,sizeof(fileSize));

  if(rewrite) {
    writeFile=writeName->OpenWrite();
    if(writeFile<0) {
      printf("Failed to open output file(s)\n");
      return;
      }
    }

  replayFile=fileName->Open();
  readNo=fileName->FileNumber();
  fileSize[readNo]=fileName->FileSize();
  readOffset=0;

  fileNo=rewrite ? 1 : readNo;
  fileOffset=0;
  splitOffset=splitsize*MEGABYTE(1);
  sSize=0;

  if(replayFile>=0) {
    if(index->Open()) {
      int lastoff=0;
      while(!error && NextFile()) {
        int count=read(replayFile,buff,sizeof(buff));
        if(count<0) {
          printf("read vdr: %s\n",strerror(errno));
          return;
          }
        else if(count==0) {
          if(fileSize[readNo]!=readOffset)
            printf("file %d read/size mismatch\n",readNo);
          eof=true;
          continue;
          }
        else {
          readOffset+=count;

          if(!quiet && (readOffset<lastoff || readOffset>lastoff+KILOBYTE(256)) && fileSize[readNo]) {
            printf("offset %lld %d%%\r",readOffset,(int)(readOffset*100/fileSize[readNo])); fflush(stdout);
            lastoff=readOffset;
            }

          int used=Process(buff,count);
          if(used<0) {
            error=true;
            break;
            }
          if(count-used) printf("bummer, count!=0\n");
          }
        }
      if(!error && !quiet) Statistics();
      }
    }
  else printf("Failed to open input file(s)\n");
}

void cGenIndex::Count(int len)
{
  fileOffset+=len;
  if(!rewrite && fileOffset>=fileSize[fileNo]) {
    fileOffset-=fileSize[fileNo];
    fileNo++;
    }
}

int cGenIndex::Output(const uchar *data, int len)
{
  if(rewrite && pstart && sSize>0) {
    if(!SafeWrite(writeFile,store,sSize)) {
      error=true;
      return -1;
      }
    sSize=0;
    }

  int ftype=0;
  if(scan) {
    const uchar *d=data;
    for(int i=len ; i>0 ; i--) {
      uchar c=*d++;
      if(skip>0) { skip--; continue; }
      // searching for sequence 00 00 01 00 xx PIC
      switch(state) {
        case 0:
        case 1:
        case 3:
          if(c==0x00) state++; else state=0;
          break;
        case 2:
          if(c==0x01) state++; 
          else if(c!=0x00) state=0;
          break;
        case 4:
          state++;
          break;
        case 5:
          ftype=(c>>3)&0x07;
          if(ftype<I_FRAME || ftype>B_FRAME) {
            printf("unknown picture type %d at %d\n",ftype,packetOffset);
            ftype=0;
            }
          scan=false; state=0; i=0;
          break;
        }
      }
    }

  if(rewrite) {
    if(scan && fileOffset>=splitOffset && (pstart || sSize)) {
      if(sSize+len>sizeof(store)) {
        printf("Oops! Packet buffer overflow\n");
        error=true;
        return -1;
        }
      memcpy(store+sSize,data,len);
      sSize+=len;
      }
    else {
      if(fileOffset>=splitOffset && sSize && ftype==I_FRAME) {
        writeFile=writeName->NextWriteFile();
        if(writeFile<0) {
          printf("Failed to open output file(s)\n");
          error=true;
          return -1;
          }
        packetNo=fileNo=writeName->FileNumber();
        packetOffset=0;
        fileOffset=sSize;
        }

      if(ftype>=I_FRAME) {
        index->Write(packetNo,packetOffset,ftype);
        }

      if(sSize>0) {
        if(!SafeWrite(writeFile,store,sSize)) {
          error=true;
          return -1;
          }
        sSize=0;
        }

      if(!SafeWrite(writeFile,data,len)) {
        error=true;
        return -1;
        }
      }
    }
  else {
    if(ftype>=I_FRAME) index->Write(packetNo,packetOffset,ftype);
    }

  Count(len);
  pstart=false;
  return len;
}

int cGenIndex::Action1(uchar type, uchar *data, int len)
{
  if(SOP) {
    scan=false; pstart=true;
    }
  return len;
}

int cGenIndex::Action2(uchar type, uchar *data, int len)
{
  if(SOP) {
    packetOffset=fileOffset; packetNo=fileNo;
    scan=true; pstart=true; skip=headerSize; state=0;
    }
  return len;
}

void cGenIndex::Skipped(uchar *data, int len)
{
  if(!rewrite) Count(len);
}

bool cGenIndex::SafeWrite(int fd, const unsigned char *data, int len)
{
  while(len>0) {
    int r=write(fd,data,len);
    if(r<0) {
      if(errno==EINTR) {
        printf("EINTR on write. Retrying\n");
        continue;
        }
      printf("write failed: %s\n",strerror(errno));
      return false;
      }
    data+=r; len-=r;
    }
  return true;
}

// --- MAIN --------------------------------------------------------------------

void about(void)
{
  static bool done=false;
  if(!quiet && !done) {
    printf("This is %s %s (%s), (C) 2003-2006 Stefan Hülswitt\n"
           "Released under the GNU Public License\n\n",
           PRG_NAME,PRG_VERSION,__DATE__);
    done=true;
    }
}

#define MAX_STR 16

int main(int argc, char *argv[])
{
  static struct option long_options[] = {
      { "rewrite",  no_argument,       NULL, 'r' },
      { "quiet",    no_argument,       NULL, 'q' },
      { "dest",     required_argument, NULL, 'd' },
      { "split",    required_argument, NULL, 's' },
      { "input",    required_argument, NULL, 'i' },
      { "skip",     required_argument, NULL, 's'|0x100 },
      { NULL }
    };

  int s=0, str[MAX_STR];
  int c;
  while((c=getopt_long(argc,argv,"rqd:s:i:",long_options,NULL))!=-1) {
    switch (c) {
      case 'r':
        rewrite=true;
        break;
      case 'q':
        quiet=true;
        break;
      case 'd':
        destdir=optarg;
        break;
      case 'i':
        input=optarg;
        break;
      case 's':
        splitsize=atoi(optarg);
        if(splitsize<1 || splitsize>2000) {
          about();
          printf("Splitsize must be between 1 and 2.000 MB\n");
          return 2;
          }
        break;
      case 's'|0x100:
        {
        int n=strtol(optarg,0,16);
        if(n<0 || n>255) {
          about();
          printf("Stream type out of range\n");
          return 2;
          }
        if(s>=MAX_STR) {
          about();
          printf("Too many streams\n");
          return 2;
          }
        str[s]=n; s++;
        break;
        }
      case '?':
      case ':':
        about();
        printf("\nUsage: genindex [-r] [-d DIR] [-s SIZE]\n\n"
               "  -i FILE, --input FILE  use a single input file\n"
               "  -r,      --rewrite     rewrite video files\n"
               "  -d DIR,  --dest=DIR    set destination dir\n"
               "  -s SIZE, --split=SIZE  split files at megabyte count\n"
               "           --skip=STR    skip stream STR (hex)\n"
               "  -q,      --quiet       no output, except errors\n"
               "\n");
      default:
        return 2;
      }
    }

  about();
  cGenIndex gi;
  if(s>0) {
    if(!rewrite) {
      printf("Cannot skip streams in non-rewrite mode\n");
      return 2;
      }
    if(!quiet) printf("Skipping stream");
    for(int i=0; i<s; i++) {
      if(!quiet) printf(" 0x%02X",str[i]);
      gi.Skip(str[i]);
      }
    if(!quiet) printf("\n\n");
    }
  gi.Work();
  return 0;
}
