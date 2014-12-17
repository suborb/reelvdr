#ifndef vlcclient_socket_h
#define vlcclient_socket_h

#include "common.h"
#include <string>
#include "remote.h"

enum eSocketErrors { csOk=1, csSendFailed=-1, csReturned404=-2, csReturnedNothing=-3, csConnectFailed=-4 };

class cClientSocket{
private:
  int SendVLCCommand(const char *Command);
public:
  /* get current position or length of currently playing movie */
  int GetVLCAnswer(bool mode); /* 0 == pos, 1 == len */
#if 0
  /* get stream info from VLC */
  const char* GetVLCInfo(void);
#endif
  /* add movie with name "Name" to VLC's playlist */
  int AddToVLCPlaylist(const char *Name);
  /* clear VLC's playlist */
  int ClearVLCPlaylist(void);
  /* get List of Recordings in the directory "BaseDir" from VLC */
  int LoadVLCRecordings(cRemoteRecordings &Recordings, const char *BaseDir);
  /* set the streaming options of VLC */
  int SetVLCSoutMode(void);
  /* VLC shall start streaming */
  int VLCStartStreaming(void);
  /* VLC shall stop streaming */
  int VLCStopStreaming(void);
  /* VLC shall pause streaming */
  int VLCPauseStreaming(void);
  /* VLC shall "Jump" in current movie */ 
  int VLCSeekInStream(const char *SeekString);
  /* VLC shall skip the given amount of seconds in the file */
  int VLCSkipSeconds(int Seconds);
  /* VLC shall jump the given position in seconds in the file */
  int VLCGoto(int Seconds);
};

extern class cClientSocket ClientSocket;

#endif

