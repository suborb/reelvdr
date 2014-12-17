/*
 * Remote plugin for the Video Disk Recorder
 *
 * remotetcp.h: tcp/telnet remote control
 *
 * See the README file for copyright information and how to reach the author.
 */


#ifndef __PLUGIN_REMOTETCP_H
#define __PLUGIN_REMOTETCP_H


#include <vdr/svdrp.h>
#include "remote.h"
#include "ttystatus.h"


/****************************************************************************/
class cTcpRemote : protected cRemoteDevTty
/****************************************************************************/
{
private:
  cTtyStatus *cstatus;
  cSocket *csock;

protected:
  virtual uint64_t getKey(void);

public:
  cTcpRemote(const char *name, int f, char *d);
  virtual ~cTcpRemote();
};


#endif // __PLUGIN_REMOTETCP_H
