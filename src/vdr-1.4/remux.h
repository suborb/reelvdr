/*
 * remux.h: A streaming MPEG2 remultiplexer
 *
 * Rewrite for Reelbox
 *
 * $Id: remux.h 1.16 2006/03/25 12:27:30 kls Exp $
 */

#ifndef __REMUX_H
#define __REMUX_H

#include <time.h>
#include <linux/dvb/dmx.h>
#include "ringbuffer.h"
//#include "tools.h"
#include "h264parser.h"

enum ePesHeader {
  phNeedMoreData = -1,
  phInvalid = 0,
  phMPEG1 = 1,
  phMPEG2 = 2
  };

ePesHeader AnalyzePesHeader(const uchar *Data, int Count, int &PesPayloadOffset, bool *ContinuationHeader = NULL);

enum eRemuxMode {
  rAuto, // auto detect, MPEG2->PES, h.264->TS
  rPES,  // Force PES
  rTS    // Force TS
};
#define TS_SIZE 188
  
// Picture types:
#define NO_PICTURE 0
#define I_FRAME    1
#define P_FRAME    2
#define B_FRAME    3

#define VIDEO_STREAM_S   0xE0
#define VIDEO_STREAM_E   0xEF

#define MAXTRACKS 64

class cRepacker;

class cRemux {
private:
  bool exitOnFailure;
  bool isRadio;
  int numUPTerrors;
  bool synced;
  bool syncEarly;
  int skipped;
  int getTimeout;
  int lastGet;
  int cryptedPacketCount;
  int restartCITries;
  cRepacker *repacker[MAXTRACKS];
  uchar *rp_data[MAXTRACKS];
  int rp_count[MAXTRACKS];
  int rp_flags[MAXTRACKS];
  uint64 rp_ts[MAXTRACKS];
  uint64 timestamp;
  int numTracks;
  int resultSkipped;
  inline int GetPid(const uchar *Data) 
	  {return ((Data[0] & 0xf) << 8) | (Data[1] & 0xff);};
	  
  // RMM extensions
  enum eRemuxMode rmode;
  int tsmode;
  int tsmode_valid;
  int sfmode;
  
  uchar patpmt[2*TS_SIZE];
  int patpmt_valid;
  int tsindex;
  int vpid;
  int ppid;
  int epid;
  int apids[16];
  int dpids[16];
  int spids[16];
  H264::cParser h264parser;
  int last_ts_state;
  uchar last_ts[TS_SIZE];
public:
  cRemux(int VPid, int PPid, const int *APids, const int *DPids, const int *SPids, bool ExitOnFailure = false, 
                   enum eRemuxMode Rmode=rPES, bool SyncEarly = false);
       ///< Creates a new remuxer for the given PIDs. VPid is the video PID, while
       ///< APids, DPids and SPids are pointers to zero terminated lists of audio,
       ///< dolby and subtitle PIDs (the pointers may be NULL if there is no such
       ///< PID). If ExitOnFailure is true, the remuxer will initiate an "emergency
       ///< exit" in case of problems with the data stream. SyncEarly causes cRemux
       ///< to sync as soon as a video or audio frame is seen.
  ~cRemux();
  void SetTimeouts(int PutTimeout, int GetTimeout);
       ///< By default cRemux assumes that Put() and Get() are called from different
       ///< threads, and uses a timeout in the Get() function in case there is no
       ///< data available. SetTimeouts() can be used to modify these timeouts.
       ///< Especially if Put() and Get() are called from the same thread, setting
       ///< both timeouts to 0 is recommended.
  int Put(const uchar *Data, int Count);
       ///< Puts at most Count bytes of Data into the remuxer.
       ///< \return Returns the number of bytes actually consumed from Data.
  uchar *Get(int &Count, uchar *PictureType = NULL, int mode=0, int *start=NULL);
       ///< Gets all currently available data from the remuxer.
       ///< \return Count contains the number of bytes the result points to, and
       ///< PictureType (if not NULL) will contain one of NO_PICTURE, I_FRAME, P_FRAME
       ///< or B_FRAME. mode=1 for multiple PES return.
		  
  void Del(int Count);
       ///< Deletes Count bytes from the remuxer. Count must be the number returned
       ///< from a previous call to Get(). Several calls to Del() with fractions of
       ///< a previously returned Count may be made, but the total sum of all Count
       ///< values must be exactly what the previous Get() has returned.
  void Clear(void);
       ///< Clears the remuxer of all data it might still contain, keeping the PID
       ///< settings as they are.
  static void SetBrokenLink(uchar *Data, int Length);
  static int GetPacketLength(const uchar *Data, int Count, int Offset);
  static int ScanVideoPacket(const uchar *Data, int Count, int Offset, uchar &PictureType, int &StreamFormat);
  static int ScanVideoPacketTS(const uchar *Data, int Count, uchar &PictureType, int &StreamFormat);

  // RMM extension: Mixed TS/PES handling
  int TSmode(void) { return tsmode;}
  int SFmode(void) { return sfmode;}
  int makeStreamType(uchar *data, int type, int pid);
  int GetPATPMT(uchar *data, int maxlen);

  };

// Start codes:
#define SC_SEQUENCE 0xB3  // "sequence header code"
#define SC_GROUP    0xB8  // "group start code"
#define SC_PICTURE  0x00  // "picture start code"

// Stream formats
#define SF_UNKNOWN 0
#define SF_MPEG2 1
#define SF_H264 2

#endif // __REMUX_H
