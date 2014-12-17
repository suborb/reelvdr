/*
 * A VDR stream removing tool (C++)
 *
 * (C) 2005 Stefan Huelswitt <s.huelswitt@gmx.de>
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

#include "pes.h"
#include "version.h"

// --- cRmStream ---------------------------------------------------------------

class cRmStream : public cPES {
private:
  int infile, outfile;
  unsigned char buff[KILOBYTE(256)];
protected:
  virtual int Output(const uchar *data, int len);
public:
  cRmStream(void);
  void Work(const char *inName, const char *outName);
  void Skip(int type);
  };

cRmStream::cRmStream(void)
{
  SetDefaultRule(prPass);
}

void cRmStream::Skip(int type)
{
  SetRule(type,prSkip);
}

void cRmStream::Work(const char *inName, const char *outName)
{
  infile=open(inName,O_RDONLY);
  if(infile>=0) {
    if(access(outName,F_OK)<0) {
      if(errno==ENOENT) {
        outfile=open(outName,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        if(outfile>=0) {
          while(1) {
            int c=read(infile,buff,sizeof(buff));
            if(c>0) {
              int u=Process(buff,c);
              if(c!=u) printf("bummer, count!=0\n");
              }
            else {
              if(c<0) printf("read failed: %s\n",strerror(errno));
              break;
              }
            }
          close(outfile);
          Statistics();
          }
        else printf("open failed: %s: %s\n",outName,strerror(errno));
        }
      else printf("access failed: %s: %s\n",outName,strerror(errno));
      }
    else printf("file exists: %s\n",outName);
    close(infile);
    }
  else printf("open failed: %s: %s\n",inName,strerror(errno));
}

int cRmStream::Output(const uchar *data, int len)
{
  int c=write(outfile,data,len);
  if(c<0) printf("write failed: %s\n",strerror(errno));
  return c;
}

// --- MAIN --------------------------------------------------------------------

void usage(void)
{
  printf("Usage: rmstream -d str [-o outfile] infile\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  printf("This is %s %s (%s), (C) 2005 Stefan Hülswitt\n"
         "Released under the GNU Public License\n",
         "rmstream",PRG_VERSION,__DATE__);

  cRmStream gi;
  char *inName=0, *outName=0;
  int c, num=0;
  while((c=getopt(argc,argv,"d:o:"))!=-1) {
    switch(c) {
      case 'd':
        {
        int type=strtol(optarg,0,0);
        gi.Skip(type);
        printf("skipping stream 0x%x\n",type);
        num++;
        break;
        }
      case 'o':
        outName=optarg;
        break;
      case '?':
        usage();
      }
    }

  if(optind<argc) inName=argv[optind];
  else usage();

  if(num<1) usage();
  
  if(outName==0)
    asprintf(&outName,"%s.new",inName);

  gi.Work(inName,outName);
  return 0;
}
