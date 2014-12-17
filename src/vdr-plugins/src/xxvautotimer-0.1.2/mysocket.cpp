/****************************************************************************
 * DESCRIPTION:
 *            
 *
 * $Id: mysocket.cpp,v 1.2 2006/08/15 20:35:22 ralf Exp $
 *
 * Contact:    ranga@vdrtools.de
 *
 * Copyright (C) 2005 by Ralf Dotzert
 ****************************************************************************/
#include "mysocket.h"
#include "debug.h"


cMySocket::cMySocket()
{
  _connected = false;
}


cMySocket::~cMySocket()
{
  Disconnect();
}


// -------- Network Methods ---------------------

bool cMySocket::Connect(char* host, int port )
{
  _connected = false;
  struct hostent *rht;

   
  if ((_socket=socket(AF_INET, SOCK_STREAM, 0))<0)
  {
    LOGERR("create Socket failed. errno =%d\n", errno);
  }
  else
  {
    _connected =true;
    if( (rht = gethostbyname(host)) == NULL)
    {
      LOGERR("Unknown host: %s", host);
    }
    else
    {
      _server.sin_family = AF_INET;
      bcopy((char *)rht->h_addr,
            (char *)&_server.sin_addr,
            rht->h_length);
      _server.sin_port = htons(port);
      if(connect(_socket,(struct sockaddr*)&_server,sizeof(struct sockaddr))== 0)
         _connected =true;
    }
  }

  return(_connected);
}


void cMySocket::Disconnect( )
{
  if(_connected)
    close(_socket);

  _connected = false;
}



bool cMySocket::Receive(int sec)
{
  int l;
  struct timeval tv;
  fd_set fds;
  char   buf[512+1];

  if(_connected)
  {
    _recBuf ="";
    do
    {
      l= -1;
      // set timer
      tv.tv_sec = sec;
      tv.tv_usec = 0;
      FD_ZERO(&fds);
      FD_SET(_socket, &fds);
      if( select(_socket+1, &fds, NULL, NULL, &tv) ==-1)
      {
        LOGERR("error in Select %s\n", strerror(errno));
      }
      else
      if(FD_ISSET(_socket, &fds))
      {
        l = recv(_socket, buf, 512, 0);
        if( l == -1 )
        {
          LOGERR("Read returned -1: %s\n", strerror(errno));
        }
        else
        {
          buf[l]='\0';
          _recBuf += buf;
          sec=3; // reset timer
        }
      }
    }while( l>0 );
  }
  return(true);
}

bool cMySocket::Send(const char * command )
{
  bool result = true;
  LOGDBG("Send command=%s\n", command);
  if( send(_socket, command, strlen(command), 0) == -1)
  {
    LOGERR("Error in sending command =%s  errno=%d, %s\n", command, errno, strerror(errno) );
    result = false;
  }
  return(result);
}






/*-----------------------------------------------------------------*/


static char *bstr =
    "ABCDEFGHIJKLMNOPQ"
    "RSTUVWXYZabcdefgh"
    "ijklmnopqrstuvwxy"
    "z0123456789+/";

void cHttp::encode( const char * input, size_t l, std::string & output, bool add_crlf )
{
  size_t i = 0;
  size_t o = 0;

  output = "";
  while (i < l)
  {
    size_t remain = l - i;
    if (add_crlf && o && o % 76 == 0)
      output += "\n";
    switch (remain)
    {
      case 1:
        output += bstr[ ((input[i] >> 2) & 0x3f) ];
        output += bstr[ ((input[i] << 4) & 0x30) ];
        output += "==";
        break;
      case 2:
        output += bstr[ ((input[i] >> 2) & 0x3f) ];
        output += bstr[ ((input[i] << 4) & 0x30) + ((input[i + 1] >> 4) & 0x0f) ];
        output += bstr[ ((input[i + 1] << 2) & 0x3c) ];
        output += "=";
        break;
      default:
        output += bstr[ ((input[i] >> 2) & 0x3f) ];
        output += bstr[ ((input[i] << 4) & 0x30) + ((input[i + 1] >> 4) & 0x0f) ];
        output += bstr[ ((input[i + 1] << 2) & 0x3c) + ((input[i + 2] >> 6) & 0x03) ];
        output += bstr[ (input[i + 2] & 0x3f) ];
    }
    o += 4;
    i += 3;
  }
}

void cHttp::encode(const std::string& str_in, std::string& str_out, bool add_crlf)
{
  encode(str_in.c_str(), str_in.size(), str_out, add_crlf);
}


bool cHttp::SendHttpRequest( char * host, int port, char * url, char * login, char * passwd )
{
  bool result = true;
  cMySocket s;
  std::string  secretOut;
  std::string  secretIn = std::string(login) + ":" + std::string(passwd);
  std::string  req;

  encode(secretIn, secretOut, false);
  req = "GET " + std::string(url) + " HTTP/1.0\r\n" +
      "Host: " + std::string(host) + "\n" +
      "Authorization: Basic " + secretOut + "\r\n\r\n";

  if(s.Connect(host, port))
  {
    fprintf(stderr, "SendHttpRequest=%s\n",req.c_str()); fflush(stderr);
    if( s.Send(req.c_str()))
    {
      s.Receive(30);
      fprintf(stderr, "Received Buffer=%s\n", s.GetRecBuf().c_str()); fflush(stderr);
    }
    s.Disconnect();
  }
  else
    result=false;

  return(result);
}


