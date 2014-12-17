/****************************************************************************
 * DESCRIPTION:
 *            
 *
 * $Id: mysocket.h,v 1.2 2006/08/15 20:35:22 ralf Exp $
 *
 * Contact:    ranga@vdrtools.de
 *
 * Copyright (C) 2005 by Ralf Dotzert
 ****************************************************************************/
#ifndef SOCKET_H
#define SOCKET_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>



/**
@author Ralf Dotzert
*/

class cHttp
{
  private:
    static void  encode(const char* input,size_t l,std::string& output, bool add_crlf);
    static void  encode(const std::string& str_in, std::string& str_out, bool add_crlf);
    
  public:
    static bool  SendHttpRequest( char * host, int port, char * url, char *login, char *passwd );
};

class cMySocket{

private:
    int                 _socket;
    bool                _connected;
    struct  sockaddr_in _server;
    std::string         _recBuf;

public:
  cMySocket();

  ~cMySocket();
    
    bool Connect(char* host, int port );
    void Disconnect();
    bool Receive( int sec);
    bool Send(const char * command );
    std::string GetRecBuf() {return _recBuf;};
};

#endif
