/***************************************************************************
                          svdrpc.cpp  -  description
                             -------------------
    begin                : Sat Aug  5 13:32:09 MEST 2000
    copyright            : (C) 2000 by Guido
    email                : gfiala@s.netic.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//#define _GNU_SOURCE

#include "svdrpc.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include "tools.h"
#include <time.h>

//begin TB
#include <libintl.h>
#include <sstream>
#define tr(s)  dgettext("noad", s)
//end TB

static time_t startTime = 0;

// --- cSocket ---------------------------------------------------------------

cSocket::~cSocket()
{
  Close();
}

void cSocket::Close(void)
{
  if (sock >= 0) {
     close(sock);
     sock = -1;
     }
}

bool cSocket::Open(int Port)
{
  dsyslog(LOG_INFO, "cSocket::Open, port = %d",Port );
  port = Port;
  sock = -1;
  if (sock < 0) {
     // create socket:
     sock = socket(PF_INET, SOCK_STREAM, 0);
     if (sock < 0) {
        perror("socket");
        port = 0;
        return false;
        }
     // make it non-blocking:
     int oldflags = fcntl(sock, F_GETFL, 0);
     if (oldflags < 0) {
        perror("fcntl-GET");
        return false;
        }
     oldflags |= O_NONBLOCK;
     if (fcntl(sock, F_SETFL, oldflags) < 0) {
        perror("fcntl-SET");
        return false;
        }
     //ToDo: make Socket-keep-alive...
  }
  dsyslog(LOG_INFO, "cSocket::Open ok" );
  return true;
}

int cSocket::Connect(void)
{
  dsyslog(LOG_INFO, "cSocket::Connect" );
  if (Open(port)) {
     struct sockaddr_in name;
     name.sin_family = AF_INET;
     name.sin_port = htons(port);
     name.sin_addr.s_addr = 0x0100007f;
     uint size = sizeof(name);
     errno=0;
     int result = connect(sock, (struct sockaddr *)&name, size);
     if ((result == -1) && (errno==EINPROGRESS))//yes, thats like spec!
     {
        return sock;
     }
     else
     {
        perror("connect");
     }
  }
  return -1;
}

// --- cSVDRPC ----------------------------------------------------------------

bool cSVDRPC::Connected()
{
	if(filedes==-1)
		return false;
	else
		return true;
}

cSVDRPC::~cSVDRPC()
{
  Close();
}

void cSVDRPC::Open(int Port)
{
  dsyslog(LOG_INFO, "cSVDRPC::Open port is %d",Port );
  name[0]=0;
  if (socket.Open(Port))
    filedes = socket.Connect();
  else
    filedes = -1;
  outstandingReply=1;
  ReadReply();//lese Greeting...
}

void cSVDRPC::Close(void)
{
  dsyslog(LOG_INFO, "cSVDRPC::Close" );
  close(filedes);
	socket.Close();
  filedes=-1;
}

bool cSVDRPC::Send(const char *s, int length)
{
  dsyslog(LOG_INFO, "cSVDRPC::Send %s %d",s,length );
	int ret=0;
  if(filedes>0)
  {
    if (length < 0) length = strlen(s);
		outstandingReply++;
    {
      int wbytes=write(filedes, s, length);
      if (wbytes == length)
        ret=true;
      else if (wbytes < 0)
			{
        //perror("svdrpc-write:");
				Close();//reopen connection in the case something went wrong:
			  //if (socket.Open())
			  //  filedes = socket.Connect();
			  //else
			  filedes = -1;
			  outstandingReply=1;
			  ReadReply();//lese Greeting...
			  ret=false;
			}
      else
      {
        fprintf(stderr, "Wrote %d bytes instead of %d\n", wbytes, length);
        ret=false;
      }
    }
  }
  else
  {
  	ret=false;
  }
  return ret;
}

bool cSVDRPC::ReadReply()
{
  dsyslog(LOG_INFO, "cSVDRPC::ReadReply" );
  int n=0,i=0,rbytes=0,size=MAXCMDBUFFER-1;
  buf[0]=0;
  if (filedes >= 0)
  {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(filedes, &set);
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 200;
    do
    {
      select(1, &set, NULL, NULL, &timeout);
      n = read(filedes, buf + rbytes, 1);
      rbytes += n;
      if (rbytes == size)
         break;
    }
    while ((n>=0) && (buf[rbytes-1]!='\n') && (++i<MAXCMDBUFFER));
    if(rbytes>0)
    {
      buf[rbytes]=0;
      if((outstandingReply>0)&&(buf[rbytes-1]=='\n'))outstandingReply--;
      return true;
    }
    else
    {
      buf[0]=0;
    }
  }
  return false;
}

bool cSVDRPC::ReadCompleteReply()
{
  int i = 0;
  while( i < 100 && outstandingReply > 0 )
  {
    usleep(10000);
    ReadReply();
    i++;
  }
  return( outstandingReply <= 0 );
}

bool cSVDRPC::CmdQuit()
{
  dsyslog(LOG_INFO, "cSVDRPC::CmdQuit" );
  char *Option=NULL;
  asprintf(&Option,"Quit\r\n");
  bool result=Send(Option);
  ReadReply();
  delete Option;
  return result;
}

void VDRMessage(const char *s)
{
  cSVDRPC *svdrpc=new cSVDRPC();
  svdrpc->Open(2001);
  usleep(10000);
  svdrpc->ReadCompleteReply();
  svdrpc->Send(s);
  usleep(10000);
  svdrpc->ReadCompleteReply();
  svdrpc->CmdQuit();
  usleep(10000);
  svdrpc->ReadCompleteReply();
  delete svdrpc;
}

char *noadMsg(const char *msg, const char *filename)
{
  char *baseName = NULL;
  char *cp = NULL;
  char *fname = NULL;
  char *vend = NULL;
  
  asprintf(&fname, "%s", filename);
  if(fname[strlen(fname) - 1] == '/')
    fname[strlen(fname) - 1] = '\0';
  vend = strrchr(fname,'/');
  if(vend) {
    *vend = '\0';
    vend = strrchr(fname,'/');
  }
  
  if( vend && vend[1] == '_')
  {
    *vend = '\0';
    vend = strrchr(fname,'/');
  }

  if( vend && strchr(vend+1, '/'))
    vend = strchr(vend+1,'/');
  if( vend )
    asprintf(&baseName,"mesg %s '%s'",msg,vend+1);
  else
    asprintf(&baseName,"mesg %s '%s'",msg, filename);
  if( baseName[strlen(baseName)-1] == '/' )
    baseName[strlen(baseName)-1] = '\0';
  vend = strrchr(baseName, '/');
  if( vend )
    *vend = '\0';
  asprintf(&cp,"%s\r\n",baseName);

  VDRMessage(cp);

  free(baseName);
  free(cp);
  free(fname);
}

char *noadMsg2(const char *msg, const char *filename)
{
  char *baseName = NULL;
  char *cp = NULL;
  char *fname = NULL;
  char *vend = NULL;
  
  asprintf(&fname, "%s", filename);
  if(fname[strlen(fname) - 1] == '/')
    fname[strlen(fname) - 1] = '\0';
  vend = strrchr(fname,'/');
  if(vend) {
    *vend = '\0';
    vend = strrchr(fname,'/');
  }
  
  if( vend && vend[1] == '_')
  {
    *vend = '\0';
    vend = strrchr(fname,'/');
  }

  if( vend && strchr(vend+1, '/'))
    vend = strchr(vend+1,'/');
  if( vend )
    asprintf(&baseName,"%s '%s'",msg,vend+1);
  else
    asprintf(&baseName,"%s '%s'",msg, filename);
  if( baseName[strlen(baseName)-1] == '/' )
    baseName[strlen(baseName)-1] = '\0';
  vend = strrchr(baseName, '/');
  if( vend )
    *vend = '\0';
  asprintf(&cp,"%s\r\n",baseName);

  VDRMessage(cp);

  free(baseName);
  free(cp);
  free(fname);
}

void noadMessage(const char *filename, int percentage)
{
  std::stringstream buf;
  buf << "plug bgprocess process " << tr("\"Mark Commercials\"") << " " << startTime << " " << percentage << " ";
  noadMsg2(buf.str().c_str(), filename);
}

void noadStartMessage( const char *filename)
{
  //noadMsg("starting noad for ", filename);
  std::stringstream buf;
  startTime = time(NULL);
  buf << "plug bgprocess process " << tr("\"Mark Commercials\"") << " " << startTime << " 1 ";
  noadMsg2(buf.str().c_str(), filename);
}

void noadEndMessage( const char *filename)
{
  noadMsg(tr("noad done for "), filename);
  std::stringstream buf;
  buf << "plug bgprocess process " << tr("\"Mark Commercials\"") << " " << startTime << " 101 ";
  noadMsg2(buf.str().c_str(), filename);
}
