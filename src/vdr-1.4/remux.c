/*
 * remux.c: A streaming MPEG2/h.264 remultiplexer
 *
 * This code is a CPU efficient but still compatible replacement for 
 * vdr's original remux.c and is used in the vdr implementation of the Reelbox
 * 
 * The remuxer can automatically detect h.264 and outputs the data
 * unchanged as TS.
 *
 * Parts were adopted from the original remux.c, done
 * by Reinhard Nissl <rnissl@gmx.de> and Klaus.Schmidinger@cadsoft.de
 *
 * (c) 2006 by Georg Acher (acher at baycom dot de)
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "channels.h"
#include "debug.h"
#include "thread.h"
#include "tools.h"
#include <sys/time.h>
#include <time.h>
#include "device.h"

#include "remux.h"
#include "libsi/util.h" // for crc

//typedef unsigned long long uint64;

#define ENABLE_AC3_REPACKER

#define ENABLE_TS_MODE

#define MPEG_FRAME_TYPE(x) (((x) >> 3) & 0x07)
//--------------------------------------------------------------------------
// Debug stuff

uint64 Now(void)
{
  struct timeval t;
  if (gettimeofday(&t, NULL) == 0)
     return (uint64(t.tv_sec)) * 1000 + t.tv_usec / 1000;
  return 0;
}
//--------------------------------------------------------------------------
void xdump(const uchar *d, int l) 
{
	d+=l;
	PRINTF("%02x: %02x %02x %02x %02x : %02x %02x %02x %02x : %02x %02x %02x %02x : %02x %02x %02x %02x ",
	       l,d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7],d[8],d[9],d[10],d[11],d[12],d[13],d[14],d[15]);
	d+=16;
	PRINTF("%02x %02x %02x %02x : %02x %02x %02x %02x : %02x %02x %02x %02x : %02x %02x %02x %02x\n",
	       d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7],d[8],d[9],d[10],d[11],d[12],d[13],d[14],d[15]);
}
//--------------------------------------------------------------------------
unsigned int getpts(uchar *pesPacket) 
{
	return ((pesPacket[0] & 0x06) << 29) |
               ( pesPacket[1]         << 22) |
               ((pesPacket[2] & 0xFE) << 14) |
               ( pesPacket[3] <<  7) |
               ( pesPacket[4]         >>  1);
                   
}
//--------------------------------------------------------------------------
// Taken from original remux.c
//--------------------------------------------------------------------------
ePesHeader AnalyzePesHeader(const uchar *Data, int Count, int &PesPayloadOffset, bool *ContinuationHeader)
{
  if (Count < 7)
     return phNeedMoreData; // too short

  if ((Data[6] & 0xC0) == 0x80) { // MPEG 2
     if (Count < 9)
        return phNeedMoreData; // too short

     PesPayloadOffset = 6 + 3 + Data[8];
     if (Count < PesPayloadOffset)
        return phNeedMoreData; // too short

     if (ContinuationHeader)
        *ContinuationHeader = ((Data[6] == 0x80) && !Data[7] && !Data[8]);

     return phMPEG2; // MPEG 2
     }

  // check for MPEG 1 ...
  PesPayloadOffset = 6;

  // skip up to 16 stuffing bytes
  for (int i = 0; i < 16; i++) {
      if (Data[PesPayloadOffset] != 0xFF)
         break;

      if (Count <= ++PesPayloadOffset)
         return phNeedMoreData; // too short
      }

  // skip STD_buffer_scale/size
  if ((Data[PesPayloadOffset] & 0xC0) == 0x40) {
     PesPayloadOffset += 2;

     if (Count <= PesPayloadOffset)
        return phNeedMoreData; // too short
     }

  if (ContinuationHeader)
     *ContinuationHeader = false;

  if ((Data[PesPayloadOffset] & 0xF0) == 0x20) {
     // skip PTS only
     PesPayloadOffset += 5;
     }
  else if ((Data[PesPayloadOffset] & 0xF0) == 0x30) {
     // skip PTS and DTS
     PesPayloadOffset += 10;
     }
  else if (Data[PesPayloadOffset] == 0x0F) {
     // continuation header
     PesPayloadOffset++;

     if (ContinuationHeader)
        *ContinuationHeader = true;
     }
  else
     return phInvalid; // unknown

  if (Count < PesPayloadOffset)
     return phNeedMoreData; // too short

  return phMPEG1; // MPEG 1
}
//--------------------------------------------------------------------------
void cRemux::SetBrokenLink(uchar *Data, int Length)
{
  int PesPayloadOffset = 0;
  if (AnalyzePesHeader(Data, Length, PesPayloadOffset) >= phMPEG1 && (Data[3] & 0xF0) == VIDEO_STREAM_S) {
     for (int i = PesPayloadOffset; i < Length - 7; i++) {
         if (Data[i] == 0 && Data[i + 1] == 0 && Data[i + 2] == 1 && Data[i + 3] == 0xB8) {
            if (!(Data[i + 7] & 0x40)) // set flag only if GOP is not closed
               Data[i + 7] |= 0x20;
            return;
            }
         }
     dsyslog("SetBrokenLink: no GOP header found in video packet");
     }
  else
     dsyslog("SetBrokenLink: no video packet in frame");
}
//--------------------------------------------------------------------------
int cRemux::GetPacketLength(const uchar *Data, int Count, int Offset)
{
  // Returns the length of the packet starting at Offset, or -1 if Count is
  // too small to contain the entire packet.
  int Length = (Offset + 5 < Count) ? (Data[Offset + 4] << 8) + Data[Offset + 5] + 6 : -1;
  if (Length > 0 && Offset + Length <= Count)
     return Length;
  return -1;
}

//--------------------------------------------------------------------------
// Using tuned subroutine in tools.c for FindPacketHeader
//--------------------------------------------------------------------------
int cRemux::ScanVideoPacket(const uchar *Data, int Count, int Offset, uchar &PictureType, int &StreamFormat)
{
	// Scans the video packet starting at Offset and returns its length.
	// If the return value is -1 the packet was not completely in the buffer.
	int Length = GetPacketLength(Data, Count, Offset);
	StreamFormat = SF_UNKNOWN;
	if (Length > 0) {
		int PesPayloadOffset = 0;
		if (AnalyzePesHeader(Data + Offset, Length, PesPayloadOffset) >= phMPEG1) {
			const uchar *p = Data + Offset + PesPayloadOffset + 0*2;
			const uchar *pLimit = Data + Offset + Length - 3;

			while (p < pLimit) {
				int x;
				x=FindPacketHeader(p, 0, pLimit - p);
				if (x!=-1) {           // found 0x000001
					p+=x+2;
					switch (p[1]) {
					case SC_PICTURE: PictureType = (p[3] >> 3) & 0x07;
						StreamFormat = SF_MPEG2;
						return Length;
					case 6: // TB: needed for ARD FestivalHD
					case 9: // Access Unit Delimiter AUD in h.264
						StreamFormat = SF_H264;
						if (p[2]==0x10)
                                                        PictureType=I_FRAME;
                                                else
                                                        PictureType=B_FRAME;
                                                return 0;
					}
				}
				else
					break;
			}
		}

		PictureType = NO_PICTURE;
		return Length;
	}
	return -1;
}
//--------------------------------------------------------------------------
// like ScanVideoPacket, but searches in TS packets
// Hack: Looks only at two TS packets
int cRemux::ScanVideoPacketTS(const uchar *Data, int Count, uchar &PictureType, int &StreamFormat)
{
	uchar buffer[2*TS_SIZE];
	int l=0;

	for(int n=0;n<Count && n<2*TS_SIZE; n+=TS_SIZE) {
		int offset;
		int adapfield=Data[3]&0x30;
		
		if (adapfield==0 || adapfield==0x20)
			continue;

		if (adapfield==0x30) {
			offset=5+Data[4];
			if (offset>TS_SIZE)
				continue;
		}
		else
			offset=4;

		memcpy(buffer+l, Data, TS_SIZE-offset);
		l+=TS_SIZE-offset;

		Data+=TS_SIZE;
	}

	const uchar *p = buffer;
	const uchar *pLimit = buffer +l - 3;
	
	while (p < pLimit) {
		int x;
		x=FindPacketHeader(p, 0, pLimit - p);
		if (x!=-1) {           // found 0x000001
			p+=x+2;
			switch (p[1]) {
			case SC_PICTURE: PictureType = (p[3] >> 3) & 0x07;
				StreamFormat = SF_MPEG2;
				return 0;
			case 6: // TB: needed for ARD FestivalHD
			case 9: // Access Unit Delimiter AUD in h.264
				StreamFormat = SF_H264;
				if (p[2]==0x10)
					PictureType=I_FRAME;
				else
					PictureType=B_FRAME;
				return 0;
			}
		}
		else
			break;
	}		
	return -1;
}
//--------------------------------------------------------------------------
// Look for 00 00 01 00
int findPictureStartCode(const uchar *Data, int Count, uchar &PictureType)
{
	int n=0;
	while(n<Count) {
		int x;
		x=FindPacketHeader(Data, 0, Count - n); // returns position of first 00
		if (x!=-1) {
			Data+=x;
			n+=x;
			if (n+6>=Count)
				return -1;
			switch (Data[3]) {
			case SC_PICTURE: 
				PictureType = MPEG_FRAME_TYPE(Data[5]);
				return n; 
			}
			n+=3;
			Data+=3;
			Count-=3;
		}
		else
			break;
	}
	return -1;
}
//--------------------------------------------------------------------------
// Packetized Buffer
//--------------------------------------------------------------------------

typedef struct {
	int offset;
	int size;
	int realSize;  // incl. alignment
	int flags;
	uint64 timestamp;
} posData;

class cPacketBuffer {
	int dataSize;
	int posSize;
	uchar *dataBuffer;
	posData *posBuffer;
	int rp,wp;      
	posData *posRead,*posWrite;
	int posReadNum;
	int invalidate;
	int putTimeout,getTimeout;

	int FindSpace(int size);
	
	uchar* GetStartSub(int *readp, int timeout, int *size, int *flags, uint64 *timestamp);
public:
	cPacketBuffer(int Size, int Packets);
	~cPacketBuffer();
	uchar* PutStart(int size);
	void   PutEnd(int size, int flags, uint64 timestamp);
	uchar* GetStart(int *size, int *flags, uint64 *timestamp);
	uchar* GetStartMultiple(int *size, int *flags, uint64 *timestamp);
	void   GetEnd(void);
	void   GetEndMultiple(void);
	void   Invalidate(void);
	void SetTimeouts(int PutTimeout, int GetTimeout);
};
//--------------------------------------------------------------------------
cPacketBuffer::cPacketBuffer(int Size, int Packets)
{
	if (Packets==0)
		Packets=Size/2048;

	// Make Packets a power of 2 to avoid expensive modulo
	int n=1;
	for(int i=0;i<16;i++) {
		if (n>=Packets) {
			Packets=n;
			break;
		}
		n<<=1;
	}

	dataBuffer=(uchar*)malloc(Size);
	memset(dataBuffer,0, Size);
	posBuffer=(posData*)malloc(Packets*sizeof(posData));

	posSize=Packets;
	dataSize=Size;
	memset(posBuffer,0, Packets*sizeof(posData));
	rp=wp=0;
	posRead=NULL;
	posWrite=NULL;
	posReadNum=0;
	invalidate=0;
	putTimeout=getTimeout=0;
}
//--------------------------------------------------------------------------
cPacketBuffer::~cPacketBuffer(void)
{
	free(dataBuffer);
	free(posBuffer);
}
//--------------------------------------------------------------------------
int cPacketBuffer::FindSpace(int size)
{
	int wpm=(wp-1)&(posSize-1);
	posData *pr,*pw;

	if (wpm<0)
		wpm+=posSize;

	if (rp==wp) {
		if (size>dataSize)
			return -1;
		return 0;
	}
	pr=posBuffer+rp;
	pw=posBuffer+wpm;

	if (pr->offset <= pw->offset) {
		if (pw->offset+pw->realSize+size < dataSize) {
			return pw->offset+pw->realSize;
		}
		if (size <pr->offset)
			return 0;
		return -1;
	}
	else {
		if (pw->offset+pw->realSize+size<dataSize)
			return pw->offset+pw->realSize;
		return -1;
	}
	return -1;
}
//--------------------------------------------------------------------------
uchar* cPacketBuffer::PutStart(int size)
{
	uint64 starttime=0;
	int offset;
	int nwp;
	int rsize;
	
//	rsize= (size+15)&~15;
	rsize=size;
	while(true) {
		offset=FindSpace(rsize);
		if (offset!=-1)
			break;
		if (putTimeout && !starttime)
			starttime=Now();
		if (!putTimeout || (Now()-starttime)>(uint64)(putTimeout)) {
			return NULL;
		}
		usleep(5*1000);
	}	
	nwp=(wp)&(posSize-1);

	posWrite=posBuffer+nwp;
	posWrite->offset=offset;
	posWrite->realSize=rsize;

//	PRINTF("PUTSTART wp %i, start %x\n",nwp, offset);
	return dataBuffer+offset;
}
//--------------------------------------------------------------------------
void cPacketBuffer::PutEnd(int size, int flags, uint64 timestamp)
{
	if (!posWrite)
		return;

	if (size>posWrite->realSize)
		size=posWrite->realSize;

	posWrite->size=size;
	posWrite->flags=flags;
	posWrite->timestamp=timestamp;
	wp=(wp+1)&(posSize-1);
}
//--------------------------------------------------------------------------
uchar* cPacketBuffer::GetStartSub(int *readp, int timeout, int *size, int *flags, uint64 *timestamp)
{
	uint64 starttime=0;

	if (*readp==wp && timeout)
		starttime=Now();
//	PRINTF("GET rp %i wp %i\n",readp,wp);
	while(*readp==wp) {
		if (!timeout || (Now()-starttime) > (uint64)(timeout))
			return 0;
		usleep(20*1000);
	}
#if 0	
	if (readp>posSize) {
		// Fixme sync
		return 0;
	}
#endif
	posRead=posBuffer+*readp;
	
	if (flags)
		*flags=posRead->flags;
	if (size)
		*size=posRead->size;
	if (timestamp)
		*timestamp=posRead->timestamp;
//	PRINTF("GET rp %i, offset %x\n",readp,posRead->offset);
	return dataBuffer+posRead->offset;	
}
//--------------------------------------------------------------------------
uchar* cPacketBuffer::GetStart(int *size, int *flags, uint64 *timestamp)
{
	if (posRead) {
#if 1
		if (flags)
	                *flags=posRead->flags;
		if (size)
			*size=posRead->size;
		return dataBuffer+posRead->offset;
#else
		GetEnd();
#endif		
	}

	if (invalidate) {
		rp=wp;
		invalidate=0;
		return 0;
	}
	posReadNum=1;
	return GetStartSub(&rp,getTimeout,size,flags,timestamp);
}
//--------------------------------------------------------------------------
void cPacketBuffer::GetEnd(void)
{
	if (!posRead)
		return;
	rp=(rp+posReadNum)&(posSize-1);
	posRead=NULL;
	posReadNum=0;
}
//--------------------------------------------------------------------------
// Try to get multiple PES at once
uchar* cPacketBuffer::GetStartMultiple(int *size, int *flags, uint64 *timestamp)
{
	uchar *buf,*lastbuf,*startbuf;
	int sz,fl;
	int readp,packets;
	int totalsize;
	int startflags;
	int timeout=getTimeout;
	uint64 tsp, starttsp;
#if 0
	if (posRead)
		GetEnd();
#endif	
//PRINTF("fill %d    \r", (wp>=rp) ? wp-rp : posSize-rp+wp);
	if (invalidate) {
		rp=wp;
		invalidate=0;
		return 0;
	}
	readp=rp;
	startbuf=NULL;
	lastbuf=NULL;
	totalsize=0;
	packets=0;
	startflags=0;
	starttsp=0;
	while(1) {
		sz=0;
		buf=GetStartSub(&readp,timeout,&sz,&fl,&tsp);
//		PRINTF("GOT %x %i\n",buf,sz);
		if (!startbuf) {
			if (!buf)
				return NULL;
			startbuf=buf;
			startflags=fl;
			starttsp=tsp;
		}
		else {
			if (	lastbuf+sz != buf ||     // Buffer wraparound or no buffer
				fl!=0 ) {                // packet start
				if (size)
					*size=totalsize;
				if (flags)
					*flags=startflags;
				if (timestamp)
					*timestamp=starttsp;
				posReadNum=packets;
				return startbuf;
			}
		}
		readp=(readp+1)&(posSize-1);
		packets++;
		totalsize+=sz;
		lastbuf=buf;
		timeout=0;
	}
	return NULL;
}
//--------------------------------------------------------------------------
void cPacketBuffer::SetTimeouts(int PutTimeout, int GetTimeout)
{
	putTimeout=PutTimeout;
	getTimeout=GetTimeout;
}
//--------------------------------------------------------------------------
void cPacketBuffer::Invalidate(void)
{
	invalidate=1;
}

//--------------------------------------------------------------------------
// cRepackerFast
//--------------------------------------------------------------------------

//Old value has problems with live-buffer (special ard) #define MAX_PACKET_SIZE 2048
//#define MAX_PACKET_SIZE 0x10000
#define MAX_PACKET_SIZE 2048
#define MAX_TS_PACKET_SIZE (64*TS_SIZE)

class cRepacker {
protected:
	int maxPacketSize;
	int rewriteCid;
	int subStreamId;
	int synced;
	int pid;

public:
	cPacketBuffer *packetBuffer;
	cRepacker(void) {};
	virtual ~cRepacker(void) {};
	// Puts data into buffer, assemble PES packets
	virtual int Put(const uchar *Data, int Count, int Flag, uint64) {return 0;};
	// Puts raw TS data into buffer, packetized only by flag
	virtual int PutRaw(const uchar *Data, int Count, int Flag, uint64) {return 0;};
	void SetMaxPacketSize(int MaxPacketSize) { maxPacketSize = MaxPacketSize; }
	inline int GetPid(void) {return pid;}
};

class cRepackerFast: public cRepacker {
protected:
	int len;
	int packermode;
	int flagmode;
	int startFlag;
	void FlushBuffer(int, uint64 timestamp);
	void insertHeader(int);
	uchar *buffer;
	int writeData(const uchar *Data, int Count, uint64 timestamp);
	int handleStartPicture(const uchar *Data, int Count, int Flag, uint64 timestamp);
public:
	cRepackerFast(void) {};
	virtual ~cRepackerFast(void);
	cRepackerFast(int Pid, int RewriteCid, int SubStreamId, int MaxPacketSize=MAX_PACKET_SIZE);
	virtual int Put(const uchar *Data, int Count, int Flag, uint64);
	virtual int PutRaw(const uchar *Data, int Count, int Flag, uint64);
};
//--------------------------------------------------------------------------

cRepackerFast::cRepackerFast(int Pid, int RewriteCid, int SubStreamId, int MaxPacketSize)
{	
#ifdef ALWAYS_TS
	PRINTF("repack ALWAYS_TS\n");
#else
	PRINTF("repack %x %x\n",Pid,RewriteCid);
#endif
	rewriteCid=RewriteCid;
	subStreamId=SubStreamId;
	synced=0;
	len=0;
	maxPacketSize=MaxPacketSize;
	pid=Pid;
	packermode=0;
	flagmode=0;

#ifdef ALWAYS_TS
	packetBuffer = new cPacketBuffer(10*1024*1024, 2048); // only one buffer for raw ts
#else
	if (rewriteCid==0xe0)
		packetBuffer = new cPacketBuffer(4096*1024, 2048); // >1s for 20Mbit/s
	else
		packetBuffer = new cPacketBuffer(128*2024, 2048);
#endif
	packetBuffer->SetTimeouts(20,0);
	buffer=packetBuffer->PutStart(maxPacketSize);
}

//--------------------------------------------------------------------------
cRepackerFast::~cRepackerFast(void)
{
	delete packetBuffer;
}
//--------------------------------------------------------------------------
void cRepackerFast::FlushBuffer(int flag, uint64 timestamp)
{
	if (len && buffer) {
		// Restore header
		buffer[0]=0;
		buffer[1]=0;
		buffer[2]=0x01;
		buffer[3]=rewriteCid;
		buffer[4]=(len-6)>>8;
		buffer[5]=(len-6)&255;

#if 0
		if (1) 
		{
			if (rewriteCid==0xe0) {
				PRINTF("%i %i\n",len,flag);
			}
		}		
#endif
		packetBuffer->PutEnd(len,flag, timestamp);
	}
	buffer=packetBuffer->PutStart(maxPacketSize+16); // Allow some assembly headroom
	len=0;
	synced=1;		
}
//--------------------------------------------------------------------------
void cRepackerFast::insertHeader(int flag)
{
	if (!buffer)
		return;
	// Default values 
	buffer[6]=0x80;
	buffer[7]=0x00;
	buffer[8]=0x00;
	len=9;

	if ( subStreamId) {
		//PRINTF("\033[0;31m insert subStreamId \033[0m %#x\n",subStreamId);
		buffer[9] = subStreamId;
		buffer[10] = 0x01;
		buffer[11] = 0x00;
                buffer[12] = 0x01;// startFlag?0x00:0x01;
		len = 13;
	}
}
//--------------------------------------------------------------------------
int cRepackerFast::writeData(const uchar *Data, int Count, uint64 timestamp)
{
	if (!buffer) {
		synced=0;
		packermode=0;
		return 0;
	}
	if (len+Count>maxPacketSize) {
		int ll;
		ll=maxPacketSize-len;
		memcpy(buffer+len,Data,ll);
		len+=ll;
		FlushBuffer(startFlag, timestamp);
		startFlag=0;
		if (!buffer) {
			synced=0;
			packermode=0;
			return 0;
		}
		insertHeader(0);
		Data+=ll;
		Count-=ll;
	}
	memcpy(buffer+len,Data,Count);
	len+=Count;
	return 0;
}
//--------------------------------------------------------------------------
/*
  Find start of picture markers and cut out single pictures into PES packets

  Packermode optimization: A lot of broadcasters start a new PES
  packet not only with I-frames, but also with B and P-frames. If this
  gets detected, the manual search for the picture start code is
  omitted.

*/

#define PACKETMODEOPT 25

int cRepackerFast::handleStartPicture(const uchar *Data, int Count, int Flag, uint64 timestamp)
{
	int x=0;
	uchar pt;
	uchar backup[6];

	// Check if picture start code expands over TS packet boundaries

	if (len>=9+6) { // +9-> Don't scan own header or part of it
		if (buffer[len-5]==0) {
			if (buffer[len-6]==0 && buffer[len-4]==1 && buffer[len-3]==SC_PICTURE)
				x=6;
			else if (buffer[len-4]==0 && buffer[len-3]==1 && buffer[len-2]==SC_PICTURE)
				x=5;
		}
		if (!x && buffer[len-3]==0) {
			if (buffer[len-4]==0 && buffer[len-2]==1 && buffer[len-1]==SC_PICTURE)
				x=4;
			else if (buffer[len-2]==0 && buffer[len-1]==1 && Data[0]==SC_PICTURE && Count>=1 )
				x=3;					
		}
		if (!x && buffer[len-1]==0) {
			if (buffer[len-2]==0 && Data[0]==1 && Data[1]==SC_PICTURE && Count>=2)
				x=2;
			else if (Data[0]==0 && Data[1]==1 && Data[2]==SC_PICTURE && Count>=3)
				x=1;					
		}
		
		// Flush until marker, shorten PES packet
		if (x) {
			int n;
//			PRINTF("###### FOUND %i, len %i  %02x %02x %02x %02x\n",x,len,Data[0],Data[1],Data[2],Data[3]);
			if (flagmode==0) { // Do not split first frame in flagged PES packet

				// Save marker data
				for(n=0;n<x;n++)
					backup[n]=buffer[len-x+n];
				
				len=len-x; // shorten

				FlushBuffer(startFlag, timestamp);
				startFlag=16; // Fixme opt
				insertHeader(0);
				if (!buffer) {
					synced=0;
					packermode=0;
					return 0;
				}
				
				// Restore marker for new packet
				for(n=0;n<x;n++)
					buffer[len+n]=backup[n];
				
				len+=x; 
			}
			else {
//				PRINTF("Split ignored 1\n");
				flagmode=0; 
			}
		}
		x=0;
	}

	// Check for complete start-of-picture(s) in TS packet
       
	while (Count>=6) {
		int y;
		y=findPictureStartCode(Data, Count, pt); 
		if (y==-1) 
			return x;

//		PRINTF("## %p %03i %03i,len %i %03i %i | %02x %02x %02x %02x %02x %02x\n",Data,y,pt,len,Count,(int)timestamp,Data[y],Data[y+1],Data[y+2],Data[y+3],Data[y+4],Data[y+5]);		
		if (Flag && (pt==B_FRAME)) {
			packermode++;
			if (packermode>=PACKETMODEOPT) 
				PRINTF("Enabled packer optimization\n");
		}
		else if (packermode>0)
			packermode--;

		if (flagmode==0) {

			if (y)
				writeData(Data, y, timestamp); // copy until marker
			
//			PRINTF("Cut at %i\n",len);
			FlushBuffer(startFlag, timestamp);
			startFlag=16;  // Fixme opt
			insertHeader(0);
			x+=y; // bytes skipped		
		}
		else {
			flagmode=0;
//			PRINTF("Split ignored 2\n");
		}


		y+=6; // Skip marker
		Data+=y;
		Count-=y;

	}
	return x; // return already handled bytes
}
//--------------------------------------------------------------------------

int cRepackerFast::Put(const uchar *Data, int Count, int Flag, uint64 timestamp)
{
	if (Flag) {

		if (rewriteCid==0xe0) {
//			PRINTF("###################### FLAG %i %i %p ######################\n",Flag,Count,Data);
#if 0
		xdump(Data,0);
		xdump(Data,32);
		xdump(Data,64);
		xdump(Data,96);
		xdump(Data,128);
		xdump(Data,160);
#endif
		}
		FlushBuffer(startFlag, timestamp);
		startFlag=1;
		flagmode=1;
	}

	if (!buffer) {
		synced=0;
		packermode=0;
		return 1;
	}
	if(Flag && (Data[0] || Data[1] || (Data[2] != 0x01))) {
		// No valid pes header -> Drop till next start flag (possible false decrypted)
		// isyslog("Dropping data for pid 0x%02x till next start flag due to bad pes header", rewriteCid);
		synced=0;
		packermode=0;
		return 1;
	} // if

	if (synced) {

		// split up if start-of-picture markers are detected
#if 1
		if (rewriteCid==0xe0 && (!synced || packermode<PACKETMODEOPT)) {
			int pos;
			pos=handleStartPicture(Data, Count, Flag, timestamp);
			Data+=pos;
			Count-=pos;
//			PRINTF("POS %i, count %i len %i  %02x %02x %02x %02x %02x %02x\n",
//			       pos,Count,len,Data[0],Data[1],Data[2],Data[3],Data[4],Data[5]);
		}
#endif
		if (Count) {
			writeData(Data, Count, timestamp);
//			if (rewriteCid==0xe0)PRINTF("Now %i\n",len);
		}
	}	    
	return 0;
}
//--------------------------------------------------------------------------

int cRepackerFast::PutRaw(const uchar *Data, int Count, int Flag, uint64 timestamp) 
{

	if ((Flag&&len) || (len+Count) > maxPacketSize) {
		packetBuffer->PutEnd(len, startFlag, timestamp);
		maxPacketSize=MAX_TS_PACKET_SIZE;
		buffer=packetBuffer->PutStart(maxPacketSize);
		startFlag=Flag;
		len=0;
	}

	if (!buffer) {
		return 1;
	}
	memcpy(buffer, Data, Count);

	buffer+=Count;
	len+=Count;

	return 0;
}

//--------------------------------------------------------------------------
// cRepackerAC3Fast
//--------------------------------------------------------------------------
// Purpose: Split one PES packet up into multiple PES with one AC3-frame each.
// Add substream id to each PES header. Stuff additional header data
// before the first AC3 frame

#ifdef ENABLE_AC3_REPACKER

#define ASSEMBLY_SIZE (65536+16)
#define PART_BUF_LEN 8192
class cRepackerAC3Fast: public cRepacker {
protected:
	uchar assembly_buffer[ASSEMBLY_SIZE]; // should be big enough for full PES packet
	int assembly_len;
	int additional_header_pos;
	int additional_header_len;
	static int frame_size_tab[];
	uchar part_buffer[PART_BUF_LEN];
	int startFlag;
	int part_len;
	int part_framesize;
	uchar *buffer; // for RawPut
	int FlushBuffer(int, uint64 );
	int GenAC3Packet(uchar* data, int frame_size, int first, uint64 timestamp);
public:
	cRepackerAC3Fast(int Pid, int RewriteCid, int SubStreamId, int MaxPacketSize=MAX_PACKET_SIZE);
	virtual ~cRepackerAC3Fast(void);
	virtual int Put(const uchar *Data, int Count, int Flag, uint64);
	virtual int PutRaw(const uchar *Data, int Count, int Flag, uint64);
};

// taken from old remux.c
// Size in words
int cRepackerAC3Fast::frame_size_tab[] = {
  // fs = 48 kHz
    64,   64,   80,   80,   96,   96,  112,  112,  128,  128,  160,  160,  192,  192,  224,  224,
   256,  256,  320,  320,  384,  384,  448,  448,  512,  512,  640,  640,  768,  768,  896,  896,
  1024, 1024, 1152, 1152, 1280, 1280,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  // fs = 44.1 kHz
    69,   70,   87,   88,  104,  105,  121,  122,  139,  140,  174,  175,  208,  209,  243,  244,
   278,  279,  348,  349,  417,  418,  487,  488,  557,  558,  696,  697,  835,  836,  975,  976,
  1114, 1115, 1253, 1254, 1393, 1394,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  // fs = 32 kHz
    96,   96,  120,  120,  144,  144,  168,  168,  192,  192,  240,  240,  288,  288,  336,  336,
   384,  384,  480,  480,  576,  576,  672,  672,  768,  768,  960,  960, 1152, 1152, 1344, 1344,
  1536, 1536, 1728, 1728, 1920, 1920,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  //
     0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  };

//--------------------------------------------------------------------------

cRepackerAC3Fast::cRepackerAC3Fast(int Pid, int RewriteCid, int SubStreamId, int MaxPacketSize)
{	
	PRINTF("repack-AC3 %x %x\n",Pid,RewriteCid);
	rewriteCid=RewriteCid;
	subStreamId=SubStreamId;
	synced=0;
	assembly_len=0;
	part_len=0;
	part_framesize=0;
	maxPacketSize=MaxPacketSize;
	pid=Pid;
	packetBuffer = new cPacketBuffer(512*1024, 2048);

	startFlag = 0;
	
	packetBuffer->SetTimeouts(20,0);
	buffer=NULL;
}
//--------------------------------------------------------------------------
cRepackerAC3Fast::~cRepackerAC3Fast(void)
{
	delete packetBuffer;
}
//--------------------------------------------------------------------------
int cRepackerAC3Fast::GenAC3Packet(uchar* data, int frame_size, int first, uint64 timestamp)
{
	uchar *buffer;
	int len,data_pos;
	
	// Get packet buffer
	len=13+additional_header_len+frame_size;
	buffer=packetBuffer->PutStart(len);
	
	if (!buffer) {
		synced=0;
		assembly_len=0;
		return 1;  // buffer overflow, ignore remaining data
	}
	
	// Header
	buffer[0]=0;
	buffer[1]=0;
	buffer[2]=1;
	buffer[3]=rewriteCid;
	buffer[4]=(len-6)>>8;
	buffer[5]=(len-6)&255;
	
	// Always add header if set - and not allways if first
	if (/*first*/additional_header_len) {
		buffer[6]=assembly_buffer[6];
		buffer[7]=assembly_buffer[7] | 1;
		buffer[8]=assembly_buffer[8];
	}
	else {
		buffer[6]=0x80;
		buffer[7]=0x00;
		buffer[8]=0x00;
	}
	data_pos=9;
	
	// Copy additional Header
	// Always add header if set - and not only if also first
	if (/*first && */additional_header_len) {
		memcpy(buffer+9,assembly_buffer+additional_header_pos,additional_header_len);
		data_pos+=additional_header_len;
	}
	
	// Add Substream Header
	buffer[data_pos++]=subStreamId;
	buffer[data_pos++]=1; // 1 AC frame in the packet
	buffer[data_pos++]=0;
	buffer[data_pos++]=1; // No continuation frame
	
	// Copy Data
	memcpy(buffer+data_pos,data,frame_size);
	
	// Push packet
	packetBuffer->PutEnd(len,first, timestamp);
	return 0;
}
//--------------------------------------------------------------------------
int cRepackerAC3Fast::FlushBuffer(int Flag, uint64 timestamp)
{
	int first=1;
	int remaining_length=assembly_len;
	int full_length;
	uchar *data;
	int frame_size=0;

	if (assembly_len<9)
		goto finish;
	
	// Fixme Check length?
	full_length=(assembly_buffer[6]<<8)|assembly_buffer[7];

	// Find PES header
	additional_header_len=assembly_buffer[8];
	additional_header_pos=9;

	if (assembly_buffer[0]==0x00 && assembly_buffer[1]==0x00 && assembly_buffer[2]==0x01 && 
	    (additional_header_len+10 < assembly_len))
	{			
		data=assembly_buffer+additional_header_pos+additional_header_len;
		remaining_length-=additional_header_pos+additional_header_len;
		
		// Loop through all AC frames
		while(remaining_length>4) {

			// Check Magic
			if (data[0]==0x0b && data[1]==0x77) {
				// Check Framesize
				frame_size=2*frame_size_tab[data[4]];
//				PRINTF("%i %i\n",frame_size,remaining_length);
				if (!frame_size)
					break;
				if (frame_size>remaining_length) { // Not enough data -> copy to part buffer
					if (part_len+remaining_length>PART_BUF_LEN)
						return 1;
//					PRINTF("Copy %i to part buf\n",remaining_length);
					memcpy(part_buffer, data, remaining_length);
					part_len=remaining_length;
					if (part_len>4) 
						part_framesize=2*frame_size_tab[part_buffer[4]];
					else
						part_framesize=0;
					break;
				}
				
				if (GenAC3Packet(data, frame_size, first, timestamp))
					return 1;

			}
			else {
				// It makes no sense to search for the magic...
				// except for Pro7/Sat1 Austria... which don't start with the magic synced
				int found_pos=0;
				for(int n=0;n<remaining_length;n++) {
					if (data[n]==0x0b && data[n+1]==0x77) {
//						PRINTF("Found magix at %i\n",n);
						// Not the expected frame size, go on searching

						if (part_len) {
							if (part_framesize && part_len+n!=part_framesize) {
//								PRINTF("Ignoring it\n");
								continue;
							}
							memcpy(part_buffer+part_len,data,n);
							part_len+=n;
							frame_size=2*frame_size_tab[part_buffer[4]];
							if (GenAC3Packet(part_buffer, frame_size, 0, timestamp))
								return 1;
//							PRINTF("XXX %i %i %i\n",frame_size,part_len,n);
							part_len=0;
						}
						found_pos=n;
						break;
					}
				}
				if (found_pos) {
					data+=found_pos;
					remaining_length-=found_pos;
					continue;
				}
				break; // really no magic
			}
			data+=frame_size;
			remaining_length-=frame_size;
			additional_header_len=0;
			first=0;
		}
	}
finish:
	synced=1;		
       	assembly_len=0;
	return 0;
}
//--------------------------------------------------------------------------
int cRepackerAC3Fast::Put(const uchar *Data, int Count, int Flag, uint64 timestamp)
{
	if (Flag) {
		// New PES packet starts
		if (FlushBuffer(0, timestamp))
			return 1;  // Timeout
	}
	
	if (assembly_len+Count>ASSEMBLY_SIZE) {
		FlushBuffer(0, timestamp); // flush complete packets up to now
		return 0;
	}

	if (synced) {
		memcpy(assembly_buffer+assembly_len,Data,Count); 
		assembly_len+=Count;
	}
	return 0;
}
//--------------------------------------------------------------------------

int cRepackerAC3Fast::PutRaw(const uchar *Data, int Count, int Flag, uint64 timestamp)
{
	if (!buffer) 
		buffer=packetBuffer->PutStart(maxPacketSize);

	if (Flag || part_len+Count>maxPacketSize) {
		packetBuffer->PutEnd(part_len, 0, timestamp);
		maxPacketSize=MAX_TS_PACKET_SIZE;
		buffer=packetBuffer->PutStart(maxPacketSize);
		part_len=0;
	}

	if (!buffer) {
		return 1;
	}
	memcpy(buffer, Data, Count);
	buffer+=Count;
	part_len+=Count;

	return 0;
}

#endif

//--------------------------------------------------------------------------
// cRemux
//--------------------------------------------------------------------------

cRemux::cRemux(int VPid, int PPid, const int *APids, const int *DPids, 
			   const int *SPids, bool ExitOnFailure, enum eRemuxMode Rmode, bool SyncEarly)
{
	exitOnFailure = ExitOnFailure;
	isRadio = VPid == 0 || VPid == 1 || VPid == 0x1FFF;
	numUPTerrors = 0;
	synced = false;
	syncEarly = SyncEarly;
	skipped = 0;
	numTracks = 0;
	resultSkipped = 0;
	timestamp=0;
	rmode=Rmode;
	tsindex=0;
	cryptedPacketCount = 0;
	restartCITries = 0;
	
	tsmode=rAuto;
	sfmode=SF_UNKNOWN;
	tsmode_valid=0;
	last_ts_state=0;
#ifdef ALWAYS_TS
	tsmode=rTS;
	rmode=rTS;
#else
	if (rmode==rPES) {
		tsmode=rPES;
	}
	if (rmode==rTS) {
		tsmode=rTS;
	}
#endif

	vpid=0;
	for(int n=0;n<16;n++) {
		apids[n]=0;
		dpids[n]=0;
		spids[n]=0;
	}
	
	for(int n=0;n<MAXTRACKS;n++) {
		repacker[n]=NULL;
		rp_data[n]=NULL;
		rp_count[n]=0;
		rp_flags[n]=0;
		rp_ts[n]=0;
	}

	lastGet=-1;

	epid=ppid=PPid;
#ifdef ALWAYS_TS
	repacker[numTracks++] = new cRepackerFast(0x00, 0x00, 0x00);
	vpid=VPid;
#else
	if (VPid) {
		repacker[numTracks++] = new cRepackerFast(VPid, 0xE0, 0x00);
		vpid=VPid;
		if(epid==vpid) epid=0;
	}
#endif
	
	if (APids) {
		int n = 0;
		while (*APids && numTracks < MAXTRACKS && n < MAXAPIDS) {
			if (n<16)
				apids[n]=*APids;
			if(epid==*APids) epid=0;
#ifdef ALWAYS_TS
			APids++; 
			n++;
#else
			repacker[numTracks++] = new cRepackerFast(*APids++, 0xC0 + n++, 0x00);
#endif
		}
	}
#ifdef ENABLE_AC3_REPACKER
	if (DPids) {
		int n = 0;
		while (*DPids && numTracks < MAXTRACKS && n < MAXDPIDS) {
			if (n<16)
				dpids[n]=*DPids;
			if(epid==*DPids) epid=0;
#ifdef ALWAYS_TS
			DPids++; 
			n++;
#else
			repacker[numTracks++] = new cRepackerAC3Fast(*DPids++, 0xbd, 0x80 + n++, 65535+6);
#endif
		}
	}
#endif
	if (SPids) {
		int n = 0;
		while (*SPids && numTracks < MAXTRACKS && n < MAXSPIDS) {
			if (n<16)
				spids[n]=*SPids;
			if(epid==*SPids) epid=0;
#ifdef ALWAYS_TS
			SPids++; 
			n++;
#else
			repacker[numTracks++] = new cRepackerFast(*SPids++, 0xbd, 0x20 + n++);
#endif
		}
	}
	
	SetTimeouts(0,1000);
}
//--------------------------------------------------------------------------
void cRemux::SetTimeouts(int PutTimeout, int GetTimeout)
{
	for(int i=0;i<numTracks;i++)
		repacker[i]->packetBuffer->SetTimeouts(PutTimeout,0);
	getTimeout=GetTimeout;
}
//--------------------------------------------------------------------------
cRemux::~cRemux(void)
{
	for(int i=0;i<numTracks;i++)
		delete(repacker[i]);
	PRINTF("Destroy remux\n");
}
//--------------------------------------------------------------------------
#define SEARCH_DATA0(x) (!Data[x])
#define SEARCH_DATA1(x) (!Data[x])
#define SEARCH_DATA2(x) (Data[x]==1)
#define SEARCH_DATA3(x) (!Data[x])

int parse_mpeg2_picture_start_code(const uchar *Data, int *state) {
	if(Data[0] != 0x47) {
//		PRINTF("Invalid TS Packet\n");
		return 0;
	} // if
	if((Data[3] & 0x10) != 0x10) return 0; // No payload
	int pos=4;
	if(Data[3] & 0x20) // adaption field present
		pos += Data[4]+1; // add adaption field length
	switch(*state) {
		case 3: if(SEARCH_DATA3(pos)) goto found;
			break;
		case 2: if(SEARCH_DATA2(pos) && SEARCH_DATA3(pos+1)) goto found;
			// no break;
		case 1: if(SEARCH_DATA1(pos) && SEARCH_DATA2(pos+1) && SEARCH_DATA3(pos+2)) goto found;
	} // switch
	while(pos<(TS_SIZE-3)) {
		if(SEARCH_DATA0(pos) && SEARCH_DATA1(pos+1) && SEARCH_DATA2(pos+2) && SEARCH_DATA3(pos+3)) goto found;
		pos++;
	} // while
	if(SEARCH_DATA0(pos) && SEARCH_DATA1(pos+1) && SEARCH_DATA2(pos+2)) 
		*state=3;
	else if(SEARCH_DATA0(pos+1) && SEARCH_DATA1(pos+2))
		*state=2;
	else if(SEARCH_DATA0(pos+2))
		*state=1;
	else
		*state=0;
	return 0;
found:
//	if(*state)
//		PRINTF("Found wrapped %d %d\n", Data[pos+3-*state], (Data[pos+5] >> 3) & 0x07);
//	else if(pos >= TS_SIZE-5)
//		PRINTF("Found end %d\n", Data[pos+3]);
//	else
//		PRINTF("Found %d %d\n", Data[pos+3], (Data[pos+5] >> 3) & 0x07);
	*state=0;
	return 1;
} // parse_mpeg2_picture_start_code

#define SEARCH_DATA3_H264(x) (Data[x]==9)
int parse_h264_access_unit_delimiter(const uchar *Data, int *state, int start) { // 00 00 01 00
	if(Data[0] != 0x47) {
//		PRINTF("Invalid TS Packet\n");
		return 0;
	} // if
	if((Data[3] & 0x10) != 0x10) return 0; // No payload
	int pos=4;
	if(Data[3] & 0x20) // adaption field present
		pos += Data[4]+1; // add adaption field length
	switch(*state) {
		case 3: if(SEARCH_DATA3_H264(pos)) goto found_wrapped;
			break;
		case 2: if(SEARCH_DATA2(pos) && SEARCH_DATA3_H264(pos+1)) goto found_wrapped;
			// no break;
		case 1: if(SEARCH_DATA1(pos) && SEARCH_DATA2(pos+1) && SEARCH_DATA3_H264(pos+2)) goto found_wrapped;
	} // switch
	while(pos<(TS_SIZE-5)) {
		if(SEARCH_DATA0(pos) && SEARCH_DATA1(pos+1) && SEARCH_DATA2(pos+2) && SEARCH_DATA3_H264(pos+3)) goto found;
		pos++;
	} // while
	if(SEARCH_DATA0(pos) && SEARCH_DATA1(pos+1) && SEARCH_DATA2(pos+2)) 
		*state=3;
	else if(SEARCH_DATA0(pos+1) && SEARCH_DATA1(pos+2))
		*state=2;
	else if(SEARCH_DATA0(pos+2))
		*state=1;
	else
		*state=0;
	return 0;
found_wrapped:
//	PRINTF("Found wrapped %d %d\n", Data[pos+3-*state], Data[pos+4]);
	*state=0;
	return 1;
found:
//	if(pos > TS_SIZE-5)
//		PRINTF("Found end %d\n", Data[pos+3]);
//	else if(!start)
//		PRINTF("Found %d %d without start!\n", Data[pos+3], Data[pos+4]);
	*state=0;
	return 1;
} // parse_h264_access_unit_delimiter

int cRemux::Put(const uchar *Data, int Count)
{
	int adapfield;
	int pid;
	int pes_start;
	unsigned int offset;

	Count=TS_SIZE*(Count/TS_SIZE);
	for (int i = 0; i < Count; i += TS_SIZE, Data+=TS_SIZE) {

// already filtered out by dvb-api
#if 0
		if (Data[1]&0x80) // Error
			continue;
#endif

#define MAX_CRYPTED_PACKETS 10000
		if (Data[3]&0xc0){ // Scrambled 
			cryptedPacketCount++;
			if(restartCITries == 0 && cryptedPacketCount > MAX_CRYPTED_PACKETS/2) {
				PRINTF("ERROR: got crypted packets - restarting decryption\n");
				if(cDevice::GetDevice(0) && cDevice::GetDevice(0)->CiHandler())
                                        cDevice::GetDevice(0)->CiHandler()->StartDecrypting();
				restartCITries++;
				continue;
			}
			if(cryptedPacketCount >= (restartCITries ? MAX_CRYPTED_PACKETS*restartCITries*restartCITries : MAX_CRYPTED_PACKETS)) { /* TB: some kind of backoff */
				PRINTF("ERROR: got %i crypted packets - restarting decryption\n", cryptedPacketCount);
				if(cDevice::GetDevice(0) && cDevice::GetDevice(0)->CiHandler())
					cDevice::GetDevice(0)->CiHandler()->StartDecrypting();
				restartCITries++;
			}
			continue;
                }

		cryptedPacketCount = 0;
		restartCITries = 0;

		adapfield=Data[3]&0x30;

// Don't ignore packets without payload in ts mode - it could be a pcr stream
#ifndef ALWAYS_TS
#ifdef ENABLE_TS_MODE
		if ((adapfield==0 || adapfield==0x20) && !(tsmode_valid>0 && (tsmode==rTS)))
			continue;
#else
		if (adapfield==0 || adapfield==0x20)
			continue;
#endif
#endif

		if (adapfield==0x30) {
			offset=5+Data[4];
			if (offset>TS_SIZE)
				continue;
		}
		else
			offset=4;
			
		pes_start=Data[1]&0x40; // remember if this TS started a PES
		pid=((Data[1]&0x1f)<<8)|Data[2];

#ifdef ALWAYS_TS
		if(vpid && (pid!=vpid)) pes_start=0;
		if((pid==vpid) && (sfmode != SF_H264) && (adapfield&0x10)) {
			int last_state = last_ts_state;
			if(pes_start) {
				last_ts_state = 0;
				if(last_state) {
					if (repacker[0]->PutRaw(last_ts, TS_SIZE, 0, timestamp++)) {
						PRINTF("CLEAR PID ALWAYS_TS %x\n",pid);
						Clear();
					}
				} // if
			} else {
				pes_start = parse_mpeg2_picture_start_code(Data, &last_ts_state);
				if(last_state) {
					if (repacker[0]->PutRaw(last_ts, TS_SIZE, pes_start,timestamp++)) {
						PRINTF("CLEAR PID ALWAYS_TS %x\n",pid);
						Clear();
					}
					pes_start = 0;
				} // if
				if(last_ts_state) {
					memcpy(last_ts, Data, TS_SIZE);
					continue;
				} // if
			} // if
//		} else if((pid==vpid) && (sfmode == SF_H264)) {
//			parse_h264_access_unit_delimiter(Data, &last_ts_state, pes_start);
		} // if
		if (repacker[0]->PutRaw(Data, TS_SIZE, pes_start,timestamp++)) {
			PRINTF("CLEAR PID ALWAYS_TS %x\n",pid);
			Clear();
		}
#else
		for (int t = 0; t < numTracks; t++) {
			if (repacker[t]->GetPid() == pid) {
#ifdef ENABLE_TS_MODE
				// Do we need raw TS?
				if (tsmode_valid>0 && (tsmode==rTS)) {
					if (tsmode_valid==1) {
						PRINTF("READER %i %i\n",tsmode,sfmode);
						Clear();
						tsmode_valid=2;
					}
					if (repacker[t]->PutRaw(Data, TS_SIZE, pes_start,timestamp++)) {
						PRINTF("CLEAR PID %x\n",pid);
						Clear();
					}
				}
				else 
#endif				
				{
					if (repacker[t]->Put(Data+offset, TS_SIZE-offset, pes_start,timestamp++)) 
						Clear();
				}
				break;
#ifdef ENABLE_TS_MODE
			} else if ((tsmode_valid>0) && (tsmode==rTS) && epid && (epid == pid) && (repacker[t]->GetPid() == vpid)) {
				if (repacker[t]->PutRaw(Data, TS_SIZE, pes_start,timestamp++)) {
					PRINTF("CLEAR PID %x\n",pid);
					Clear();
				}
			}
#endif				
		}
#endif
	}
	
	return Count;
}
//--------------------------------------------------------------------------
#ifdef DEBUG_TIMES
extern struct timeval switchTime;
#endif

uchar* cRemux::Get(int &Count, uchar *PictureType, int mode, int *start)
{
	uint64 starttime=0;
	uchar *resultData=NULL;
	uchar pt= NO_PICTURE;
	int sf = SF_UNKNOWN;	
	int flags=0;
	int resultCount=0;
	int data_available=0;

	if (PictureType)
		*PictureType = NO_PICTURE;

	Count=0;

	while(1) 
	{
		for(int i=0;i<numTracks;i++) {
			if (!rp_data[i]) {
				if (mode)
					rp_data[i]=repacker[i]->packetBuffer->GetStartMultiple(&rp_count[i],&rp_flags[i], &rp_ts[i]);
				else
					rp_data[i]=repacker[i]->packetBuffer->GetStart(&rp_count[i],&rp_flags[i], &rp_ts[i]);
			}
			if (rp_data[i] && !rp_count[i]) // Forget empty packets
				rp_data[i]=NULL;
			if (rp_data[i])
				data_available=1;

		}

		if (data_available)
			break;
		
		if (!getTimeout) {
//			usleep(1000);
			return NULL;
		}

		if (!starttime)
			starttime=Now();

		usleep(10*1000); // Busy waiting: costs less than one would expect...
		if (Now()-starttime > (uint64)(getTimeout)) {
			return NULL;
		}
	}


#ifdef ALWAYS_TS
	if (!rp_data[0])
		return NULL;
	resultData=rp_data[0];
	flags=rp_flags[0];
	resultCount=rp_count[0];
	lastGet=0;
	if(flags) { // Video stream
		if(isRadio) {
			if(!synced) {
				PRINTF("Radio mode\n");
				synced=1;
			} // if
			if(PictureType) // For livebuffer to handle "frames"...
				*PictureType = I_FRAME;
		} else {
			if(!h264parser.GetTSFrame(resultData, resultCount, vpid, sf, pt)) {
				PRINTF("GetTSFrame: Invalid data! Dropping %d bytes.\n", resultCount);
				Del(resultCount);
				return NULL;
			} // if
			if(sf!=SF_UNKNOWN) {
				if(sf!=sfmode) PRINTF("New stream type %d->%d\n", sfmode, sf);
				sfmode=sf;
			} // if
			if((sfmode != SF_UNKNOWN) && (pt != NO_PICTURE) && !synced) {
				PRINTF("synced\n");
				synced=1;
//} else if((sfmode != SF_UNKNOWN) && !synced) {
//PRINTF("not synced: %d\n", pt);
			} // if
//PRINTF("Found %d size %d (%d)\n", pt, resultCount, mode);
			if(PictureType)
				*PictureType = pt;
		} // if
	} // if
	if(!synced) {
		Del(0);
		return NULL;
	} // if
	Count=resultCount;
	return resultData;
#else
	// Find oldest data in the queues
	int n=-1;
	if (!synced) {
		if (!rp_data[0])
			return NULL;
		n=0;
	}
	else {		
		uint64 min_ts=0xffffffffffffffffLL;
		for(int i=0;i<numTracks;i++) {
			if (rp_data[i]) {
				if (rp_ts[i]<min_ts) {
					n=i;
					min_ts=rp_ts[i];
				}
			}
		}
	}

	resultData=rp_data[n];
	flags=rp_flags[n];
	resultCount=rp_count[n];
	lastGet=n;

	if (n==-1)
		return NULL;
#ifdef ENABLE_TS_MODE
	if (tsmode_valid==2 && (tsmode==rTS)) {
#ifdef USE_H264_PARSER
		uchar pt = NO_PICTURE;
		if(n==0) {
			if(flags) { // Video stream
				if(!h264parser.GetTSFrame(resultData, resultCount, vpid, sf, pt)) {
					PRINTF("GetTSFrame: Invalid data! Dropping %d bytes.\n", resultCount);
					Del(resultCount);
					return NULL;
				} // if
				if(pt==I_FRAME) synced=1;
		    	} // if
		} // if
		if (!synced) {
			Del(0);
			return NULL;
		}
		if(PictureType)
			*PictureType = pt;
		if (start)
			*start=flags;
		Count=resultCount;
		return resultData;
#else
		if (n==0 && flags) { 
			/* Assumption/HACK: only a flagged PES packet starts a new frame
				and frame type fits in the first packet
			*/
			if (PictureType) {
				int l=ScanVideoPacketTS(resultData, (resultCount>2*TS_SIZE?2*TS_SIZE:resultCount), pt, sf);
				// UGLY HACK: If no frame found, fake an I-FRAME
				if (pt==NO_PICTURE)
					pt=I_FRAME;
				*PictureType = pt;
			}
		}
		Count=resultCount;
		synced=1; // FIXME: Start sync at I-Frame!
		return resultData;
#endif
	}
#endif
	if (resultData && flags) { // Only a flagged packet can start a sequence/picture
//		PRINTF("GOT %i %p\n",lastGet,data);
		uchar StreamType = resultData[3];
		if (VIDEO_STREAM_S <= StreamType && StreamType <= VIDEO_STREAM_E) {
			int l=ScanVideoPacket(resultData, resultCount, 0, pt, sf);

#ifdef ENABLE_TS_MODE
			if (!tsmode_valid) { // Find type (MPEG2/h264)
				if (sf!=SF_UNKNOWN) {
#ifdef DEBUG_TIMES
                                       struct timeval now;
                                       gettimeofday(&now, NULL);
                                       float secs = ((float)((1000000 * now.tv_sec + now.tv_usec) - (switchTime.tv_sec * 1000000 + switchTime.tv_usec))) / 1000000;
                                       PRINTF("\n\n======================= DETECTED %i : time since channelswitch-start: secs: %f\n\n",sf, secs);
#else
                                       DDD("======================= DETECTED %i",sf);
#endif
					sfmode=sf;
#ifndef ALWAYS_TS
                                        rmode = rAuto; // TB: hack to force rmode to rAuto
#endif
					switch(rmode) {
					case rAuto:
						if (sf==SF_MPEG2) {
							tsmode=rPES;
							tsmode_valid=1;
						}
						if (sf==SF_H264) {
							tsmode=rTS;
							Clear(); // Throw away already packed PES buffers
							tsmode_valid=1;
							return NULL;
						}
						break;
					case rPES:
						tsmode=rPES;
						tsmode_valid=1;
						break;
					case rTS:
						tsmode=rTS;
						Clear();
						tsmode_valid=1;
						return NULL;
					}
				}
				else {
				//	return NULL; // Wait until detection // TB: slows down channelswitching on crypted channels
                                }
			}
#endif

#if 0
			if (!synced) {
				PRINTF("PT %i %i\n",pt,flags);
				xdump(resultData,0);
				xdump(resultData,32);
			}
#endif
			if (!synced) {
				if (pt == I_FRAME || syncEarly) {
					synced=true;
					DDD("REMUX SYNCED");
					if (pt == I_FRAME) // syncEarly: it's ok but there is no need to call SetBrokenLink()
					   SetBrokenLink(resultData, l);
					else 
					PRINTF("video: synced early\n");
				}
			}
		}
		else if (isRadio || (!synced && syncEarly)) {
		  pt=I_FRAME;
		  synced=true;
		  if (!isRadio) 
              PRINTF("audio: synced early\n");	
		}
	    else if (!synced && PictureType && isRadio) {
		   lastGet=-1; // Hold back audio to get faster AV-sync
		}
    }

	if (!synced) {
		Del(0);
		return NULL;
	}

	if (PictureType)
		*PictureType = pt;
	if (start)
		*start=flags;
	Count=resultCount;

	return resultData;
#endif
}
//--------------------------------------------------------------------------
// Count is ignored due to packet structure
void cRemux::Del(int Count)
{
	if (lastGet!=-1 && repacker[lastGet]) {
		repacker[lastGet]->packetBuffer->GetEnd();
		rp_data[lastGet]=NULL;
	}
}
//--------------------------------------------------------------------------
void cRemux::Clear(void)
{
//	PRINTF("CLEAR\n");
#ifdef USE_H264_PARSER
	h264parser.Reset();
#endif
	for(int i=0;i<numTracks;i++) {
		repacker[i]->packetBuffer->GetEnd();
		rp_data[i]=NULL;
		repacker[i]->packetBuffer->Invalidate();
	}
	synced=0;
	last_ts_state=0;
}
//--------------------------------------------------------------------------
// taken from streamdev
static uchar tspid0[TS_SIZE] = { 
        0x47, 0x40, 0x00, 0x10, 0x00, 0x00, 0xb0, 0x0d, 
        0x80, 0x08, 0xc3, 0x00, 0x00, 0x00, 0x84, 0xe0, 
        0x84, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff};
//--------------------------------------------------------------------------
int cRemux::makeStreamType(uchar *data, int type, int pid)
{
	switch (type) {
	case 0: // MPEG2-Video
		data[0]=0x02;
		data[1]=0xe0|((pid>>8)&0x1f);
		data[2]=pid&0xff;
		data[3]=0xf0;
		data[4]=0x00;
		return 5;
	case 1: // H264-Video
		data[0]=0x1b;
		data[1]=0xe0|((pid>>8)&0x1f);
		data[2]=pid&0xff;
		data[3]=0xf0;
		data[4]=0x00;
		return 5;
	case 2: // MPEG-Audio
		data[0]=0x04;
		data[1]=0xe0|((pid>>8)&0x1f);
		data[2]=pid&0xff;
		data[3]=0xf0;
		data[4]=0x00;
		return 5;
	case 3: // AC3
		data[0]=0x06;
		data[1]=0xe0|((pid>>8)&0x1f);
		data[2]=pid&0xff;
		data[3]=0xf0;
		data[4]=0x03;
		data[5]=0x6a;
		data[6]=0x01;
		data[7]=0x00;
		return 8;
	case 4: // Subpicture
		data[0]=0x01;
		data[1]=0x03;
		data[2]=0xe0|((pid>>8)&0xf);
		data[3]=pid&0xff;
		data[4]=0xf0;
		data[5]=0x00;
		return 6;
		
	}
	
	// VIDEO 02 e0 65 f0 06 11 01 fe 52 01 01
        //           PPPP             
	// H264  1b e0 ff f0 00
	//       1b e3 ff f0 00
	//           PPPP
	// Audio 04 e0 66 f0 09 0a 04 64 65 75 01 52 01 02 05 e8 19 f0
	//       04 e0 66 f0 00
        //           PPPP
	// Dolby 06 e0 6a f0 0c 0a 04 64 65 75 01 52 01 11 6a 01 00
	//       06 e4 03 f0 03 6a 01 00
	//           PPPP

	return 0;
}
//--------------------------------------------------------------------------
int cRemux::GetPATPMT(uchar *data, int maxlen)
{
	int len,n;
	int crc;

	if (maxlen<TS_SIZE)
		return 0;
	memcpy(data,tspid0,TS_SIZE);
	data[3]|=tsindex&0xf;
	crc=SI::CRC32::crc32 ((const char*)data + 5, data[7]-1, 0xffffffff);
	data[17]=crc>>24;
	data[18]=crc>>16;
	data[19]=crc>>8;
	data[20]=crc;

	data+=TS_SIZE;
	memset(data,255,TS_SIZE);
	
	data[0]=0x47;
	data[1]=0x40;
	data[2]=0x84;
	data[3]=0x10 | (tsindex&0xf);
	data[4]=0x00;
	data[5]=0x02;
	data[6]=0xb0;
	// 7 length
	data[8]=0x00; // Programm number 0x0084
	data[9]=0x84;
	data[10]=0xc3;
	data[11]=0x00;
	data[12]=0x00;
	int pcr = ppid;
	if(!pcr) pcr = vpid;
	data[13]=0xe0 | (pcr>>8); // PCR
	data[14]=(pcr);
	data[15]=0xf0;
	data[16]=00;
	
	len=17;

	if (sfmode==SF_H264)
		len+=makeStreamType(data+len, 1, vpid);
	else
		len+=makeStreamType(data+len, 0, vpid);

	for(n=0;n<16;n++) {
		if (!apids[n])
			break;
		len+=makeStreamType(data+len, 2, apids[n]);
	}

	for(n=0;n<16;n++) {
		if (!dpids[n])
			break;
		len+=makeStreamType(data+len, 3, dpids[n]);
	}
       
	for(n=0;n<16;n++) {
		if (!spids[n])
			break;
		len+=makeStreamType(data+len, 4, spids[n]);
	}

	data[7]=len-8+4;
	crc=SI::CRC32::crc32 ((const char*)data+5, data[7]-1, 0xffffffff);
	data[len]=crc>>24;
	data[len+1]=crc>>16;
	data[len+2]=crc>>8;
	data[len+3]=crc;

	tsindex++;
	return 2*TS_SIZE;
}
//--------------------------------------------------------------------------
