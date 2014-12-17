/*
 * pes.c:
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 */

#include <stdlib.h>

#include "pes.h"
#include "ringbuffer.h"

#define DEBUG(x...) printf(x)
//#define DEBUG(x...)

//#define PD(x...) printf(x)
#define PD(x...)

// --- cPES --------------------------------------------------------------------

cPES::cPES(eRule ru)
{
  rb=new cRingBufferFrame(KILOBYTE(50));
  defaultRule=ru;
  Reset();
}

cPES::~cPES()
{
  delete rb;
}

void cPES::Reset(void)
{
  for(int i=0 ; i<NUM_RULESETS ; i++) SetDefaultRule(defaultRule,i);
  UseRuleset(0);
  ClearSeen();
  Clear();
}

void cPES::Clear(void)
{
  Lock();
  mode=pmNewSync; frame=0; mpegType=2;
  rb->Clear();
  Unlock();
}

bool cPES::ValidRuleset(const int num)
{
  if(num>=0 && num<NUM_RULESETS) return true;
  DEBUG("PES: illegal ruleset %d\n",num);
  return false;
}

void cPES::UseRuleset(const int num)
{
  if(ValidRuleset(num)) {
    currRules=rules[num];
    currNum=num;
    }
}

int cPES::CurrentRuleset(void)
{
  return currNum;
}

void cPES::SetDefaultRule(eRule ru, const int num)
{
  if(ValidRuleset(num)) 
    for(int i=0 ; i<NUM_RULES ; i++) rules[num][i]=ru;
}

void cPES::SetRule(uchar type, eRule ru, const int num)
{
  if(ValidRuleset(num)) rules[num][type]=ru;
}

void cPES::SetRuleR(uchar ltype, uchar htype, eRule ru, const int num)
{
  if(ValidRuleset(num)) {
    if(ltype<htype) for( ; ltype<=htype ; ltype++) rules[num][ltype]=ru;
    else DEBUG("PES: bad range %x-%x\n",ltype,htype);
    }
}

unsigned int cPES::Seen(uchar type) const
{
  return seen[type];
}

void cPES::ClearSeen(void)
{
  memset(seen,0,sizeof(seen));
  totalBytes=totalSkipped=totalZeros=0;
  skipped=zeros=0;
}

void cPES::Skip(uchar *data, int count)
{
  if(data) {
    Skipped(data,count);
    while(count>0) {
      skipped++;
      if(!*data++) zeros++;
      count--;
      }
    }
  else if(skipped) {
   totalSkipped+=skipped;
   if(skipped==zeros) totalZeros+=zeros;
   else DEBUG("PES: skipped %d bytes\n",skipped);
   skipped=zeros=0;
   }
}

void cPES::Statistics(void)
{
  if(totalBytes) {
    DEBUG("PES: Stats %lld bytes total, %lld skipped, %lld zero-gaps\n",
          totalBytes,totalSkipped,totalZeros);
    for(int type=0 ; type<=0xFF ; type++)
     if(seen[type])
       DEBUG("PES: Stats %02X: %d packets\n",type,seen[type]);
    }
}

void cPES::ModifyPaketSize(int mod)
{
  if(SOP) {
    int size=header[4]*256+header[5]+mod;
    header[4]=(size>>8)&0xFF;
    header[5]=(size   )&0xFF;
    }
  else DEBUG("PES: modify paket size called in middle of packet\n");
}

void cPES::Redirect(eRule ru)
{
  if(SOP) {
    currRule=ru;
    redirect=true;
    }
  else DEBUG("PES: redirect called in middle of packet\n");
}

int cPES::HeaderSize(uchar *head, int len)
{
  if(len<PES_MIN_SIZE) return -PES_MIN_SIZE;
  switch(head[3]) {
    default:
    // Program end
    case 0xB9:
    // Programm stream map
    case 0xBC:
    // video stream start codes
    case 0x00 ... 0xB8:
    // reserved
    case 0xF0 ... 0xFF:
      return PES_MIN_SIZE;

    // Pack header
    case 0xBA:
      if(len<5) return -5;
      switch(head[4]&0xC0) {
        default:
          DEBUG("PES: unknown mpegType in pack header (0x%02x)\n",head[4]);
          // fall through
        case 0x00:
          mpegType=1;
          return 12;
        case 0x40:
          mpegType=2;
          if(len<14) return -14;
          return 14+(head[13]&0x07); // add stuffing bytes
        }

    // System header
    case 0xBB:
      if(len<6) return -6;
      return 6+head[4]*256+head[5]; //XXX+ is there a difference between mpeg1 & mpeg2??

    // Padding stream
    case 0xBE:
    // Private stream2 (navigation data)
    case 0xBF:
      return 6; //XXX+ is there a difference between mpeg1 & mpeg2??

    // Private stream1
    case 0xBD:
    // all the rest (the real packets)
    case 0xC0 ... 0xCF:
    case 0xD0 ... 0xDF:
    case 0xE0 ... 0xEF:
      if(len<7) return -7;
      int index=6;
      while((head[index]&0xC0)==0xC0) { // skip stuffing bytes
        index++; if(index>=len) return -(index+1);
        }
      if((head[index]&0xC0)==0x80) { // mpeg2
        mpegType=2;
        index+=2; if(index>=len) return -(index+1);
        return index+1+head[index]; // mpeg2 header data bytes
        }
      mpegType=1;
      if((head[index]&0xC0)==0x40) { // mpeg1 buff size
        index+=2; if(index>=len) return -(index+1);
        }
      switch(head[index]&0x30) {
        case 0x30: index+=9; break; // mpeg1 pts&dts
        case 0x20: index+=4; break; // mpeg1 pts
        case 0x10: DEBUG("PES: bad pts/dts flags in MPEG1 header (0x%02x)\n",head[index]); break;
        }
      return index+1;
    }
}

int cPES::PacketSize(uchar *head, int len)
{
  switch(head[3]) {
    default:
    // video stream start codes
    case 0x00 ... 0xB8:
    // Program end
    case 0xB9:
    // Pack header
    case 0xBA:
    // System header
    case 0xBB:
    // Programm stream map
    case 0xBC:
    // reserved
    case 0xF0 ... 0xFF:
      return len; // packet size = header size

    // Private stream1
    case 0xBD:
    // Padding stream
    case 0xBE:
    // Private stream2 (navigation data)
    case 0xBF:
    // all the rest (the real packets)
    case 0xC0 ... 0xCF:
    case 0xD0 ... 0xDF:
    case 0xE0 ... 0xEF:
      return 6+head[4]*256+head[5];
    }
}

int cPES::Return(int used, int len)
{
  PD("PES: return used=%d len=%d mode=%d\n",used,len,mode);
  if(SOP && unsavedHeader && used>=len) {
    // if we are about to finish the current data packet and we have
    // an unsaved header inside, we must save the header to the buffer
    memcpy(hbuff,header,headerSize);
    header=hbuff; unsavedHeader=false;
    PD("PES: header saved\n");
    }
  if(used>len) {
    DEBUG("PES: BUG! used %d > len %d\n",used,len);
    used=len;
    }
  if(used>0) totalBytes+=used;
  Unlock(); // release lock from Process()
  return used;
}

int cPES::Process(const uchar *data, int len)
{
  Lock(); // lock is released in Return()
  PD("PES: enter data=%p len=%d mode=%d have=%d need=%d old=%d\n",
      data,len,mode,have,need,old);
  int used=0;
  while(used<len) {
    uchar *c=(uchar *)data+used;
    int rest=len-used, n;
    switch(mode) {
      case pmNewSync:
        PD("PES: new sync from %d, rest %d\n",used,rest);
        have=old=need=0;
        unsavedHeader=false; outputHeader=true; SOP=true; redirect=false;
        mode=pmFastSync;
        // fall through

      case pmFastSync:
        // a short cut for the most common case
        // if matched here, header isn't copied around
        if(rest>=PES_MIN_SIZE) {
          PD("PES: fastsync try used=%d: %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                used,c[0],c[1],c[2],c[3],c[4],c[5],c[6],c[7],c[8]);
          if(c[2]==0x01 && c[1]==0x00 && c[0]==0x00) {
            headerSize=HeaderSize(c,rest);
            if(headerSize>0 && rest>=headerSize) {
              // found a packet start :-)
              PD("PES: fastsync hit used=%d headerSize=%d rest=%d\n",used,headerSize,rest);
              header=c; unsavedHeader=true;
              used+=headerSize; mode=pmHeaderOk; continue;
              }
            }
          else if(c[2]!=0x00) { used+=3; Skip(c,3); continue; }
          else { used++; Skip(c); continue; }
          }
        // copy remaining bytes to buffer
        memcpy(hbuff,c,rest);
        have=old=rest; used+=rest;
        mode=pmSync;
        PD("PES: buffering started old=%d\n",old);
        break;

      case pmSync:
        PD("PES: slowsync have=%d old=%d\n",have,old);
        if(have<PES_MIN_SIZE && rest>=1) { hbuff[have++]=c[0]; used++; continue; }
        if(have>=PES_MIN_SIZE) {
          PD("PES: slowsync try used=%d: %02x %02x %02x %02x\n",
                used,hbuff[0],hbuff[1],hbuff[2],hbuff[3]);
          if(hbuff[0]==0x00 && hbuff[1]==0x00&& hbuff[2]==0x01) {
            need=abs(HeaderSize(hbuff,have));
            mode=pmGetHeader; continue;
            }
          // no sync found, move buffer one position ahead
          have--; Skip(hbuff);
          memmove(hbuff,hbuff+1,have);
          // if all bytes from previous data block used up, switch to FastSync
          if(!--old) {
            used=0; mode=pmFastSync;
            PD("PES: buffering ended\n");
            }
          continue;
          }
        break;

      case pmGetHeader:
        if(have<need) {
          n=min(need-have,rest);
          memcpy(hbuff+have,c,n);
          have+=n; used+=n;
          PD("PES: get header n=%d need=%d have=%d used=%d\n",n,need,have,used);
          continue;
          }
        if(have>=need) {
          need=abs(HeaderSize(hbuff,have));
          if(have<need) continue;
          // header data complete
          PD("PES: slowsync hit used=%d\n",used);
          if(have>need) DEBUG("PES: bug, buffered too much. have=%d need=%d\n",have,need);
          if(have>(int)sizeof(hbuff)) DEBUG("PES: bug, header buffer overflow. have=%d size=%d\n",have,sizeof(hbuff));
          headerSize=need;
          header=hbuff;
          mode=pmHeaderOk;
          }
        break;

      case pmHeaderOk:
        type=header[3]; seen[type]++; Skip(0);
        if(type<=0xB8) {
          // packet types 0x00-0xb8 are video stream start codes
          DEBUG("PES: invalid packet type 0x%02x, skipping\n",type);
          mode=pmNewSync;
          break;
          }
        payloadSize=PacketSize(header,headerSize)-headerSize;
        if(payloadSize<0) {
          DEBUG("PES: invalid payloadsize %d, skipping\n",payloadSize);
          mode=pmNewSync;
          break;
          }
        PD("PES: found sync at offset %d, type %02x, length %d, next expected %d\n",
              used-headerSize,type,headerSize+payloadSize,used+payloadSize);
        PD("PES: header type=%02x mpeg=%d header=%d payload=%d:",
              type,mpegType,headerSize,payloadSize);
        for(int i=0 ; i<headerSize ; i++) PD(" %02x",header[i]);
        PD("\n");
        currRule=currRules[type];
        have=need=0;
        mode=pmPayload;
        // fall through

      case pmPayload:
        n=min(payloadSize-have,rest);
        if(!n) {
          if(payloadSize==0) n=1;
          else break;
          }
        PD("PES: payload have=%d n=%d SOP=%d\n",have,n,SOP);
        switch(currRule) {
          default:
            DEBUG("PES: bug, unknown rule %d, assuming pass\n",currRule);
            // fall through
          case prPass: n=n; break;
          case prSkip: n=-n; break;
          case prAct1: n=Action1(type,c,n); break;
          case prAct2: n=Action2(type,c,n); break;
          case prAct3: n=Action3(type,c,n); break;
          case prAct4: n=Action4(type,c,n); break;
          case prAct5: n=Action5(type,c,n); break;
          case prAct6: n=Action6(type,c,n); break;
          case prAct7: n=Action7(type,c,n); break;
          case prAct8: n=Action8(type,c,n); break;
          }
        if(n==0) {
          if(redirect) { redirect=false; continue; }
          return Return(used,len);
          }
        need=n; SOP=false;
        mode=pmRingGet;
        // fall through

      case pmRingGet:
        frame=rb->Get();
        if(frame) {
          outCount=frame->Count();
          outData=(uchar *)frame->Data();
          PD("PES: ringbuffer got frame %p count=%d\n",frame,outCount);
          nextMode=pmRingDrop; mode=pmOutput; break;
          }
        mode=pmDataPut;
        // fall through

      case pmDataPut:
        if(need<0) {
          need=-need; outputHeader=false;
          mode=pmDataReady; continue;
          }
        if(outputHeader) {
          outData=header; outCount=headerSize; outputHeader=false;
          nextMode=pmDataPut;
          }
        else if(payloadSize) {
          outData=c; outCount=need;
          nextMode=pmDataReady;
          }
        else {
          mode=pmDataReady;
          continue;
          }
        mode=pmOutput;
        // fall through

      case pmOutput:
        for(;;) {
          PD("PES: output data=%p count=%d -> ",outData,outCount);
          n=Output(outData,outCount);
          PD("n=%d\n",n);
          if(n<0) return Return(-1,len);
          if(n==0) return Return(used,len);

          outCount-=n; outData+=n;
          if(outCount<=0) { mode=nextMode; break; }
          }
        break;

      case pmDataReady:
        if(payloadSize) { used+=need; have+=need; }
        PD("PES: data ready need=%d have=%d paySize=%d used=%d\n",
            need,have,payloadSize,used);
        if(have>=payloadSize) {
          PD("PES: packet finished\n");
          if(have>payloadSize) DEBUG("PES: payload exceeded, size=%d have=%d\n",payloadSize,have);
          mode=pmNewSync;
          }
        else mode=pmPayload;
        break;

      case pmRingDrop:
        PD("PES: ringbuffer drop %p\n",frame);
        rb->Drop(frame); frame=0;
        mode=pmRingGet;
        break;

      default:
        DEBUG("PES: bug, bad mode %d\n",mode);
        return Return(-1,len);
      }
    }
  PD("PES: leave\n");
  return Return(used,len);
}
