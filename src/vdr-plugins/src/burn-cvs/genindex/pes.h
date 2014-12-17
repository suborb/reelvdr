/*
 * pes.h:
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 */

#ifndef __PES_H
#define __PES_H

#include <vdr/thread.h>
#include <vdr/tools.h>

#define PES_MIN_SIZE 4    // min. number of bytes to identify a packet
#define PES_HDR_SIZE 6    // length of PES header
#define PES_EXT_SIZE 3    // length of PES extension

#define NUM_RULESETS 4    // number of rule sets
#define NUM_RULES    256  // how many rules in every set

class cFrame;
class cRingBufferFrame;

class cPES : public cMutex {
protected:
  enum eRule { prPass, prSkip, prAct1, prAct2, prAct3, prAct4, prAct5, prAct6, prAct7, prAct8 };
private:
  eRule rules[NUM_RULESETS][NUM_RULES], *currRules, currRule, defaultRule;
  int currNum;
  // Statistics
  unsigned int seen[256];
  int skipped, zeros;
  long long totalBytes, totalSkipped, totalZeros;
  //
  enum eMode { pmNewSync, pmFastSync, pmSync, pmGetHeader, pmHeaderOk, pmPayload,
               pmRingGet, pmRingDrop, pmDataPut, pmDataReady, pmOutput };
  eMode mode, nextMode;
  uchar hbuff[PES_HDR_SIZE+PES_EXT_SIZE+256];
  uchar type;
  int have, need, old;
  bool unsavedHeader, outputHeader, redirect;
  //
  cFrame *frame;
  const uchar *outData;
  int outCount;
  //
  bool ValidRuleset(const int num);
  void Skip(uchar *data, int count=1);
  int Return(int used, int len);
  int HeaderSize(uchar *head, int len);
  int PacketSize(uchar *head, int len);
protected:
  bool SOP;        // true if we process the start of packet
  int headerSize;  // size of the header including additional header data
  uchar *header;   // the actual header
  int mpegType;    // gives type of packet 1=mpeg1 2=mpeg2
  int payloadSize; // number of data bytes in the packet
  //
  cRingBufferFrame *rb;
  //
  // Rule Management
  void UseRuleset(int num);
  int CurrentRuleset();
  void SetDefaultRule(eRule ru, const int num=0);
  void SetRule(int type, eRule ru, const int num=0);
  void SetRuleR(int ltype, int htype, eRule ru, const int num=0);
  void Reset();
  // Misc
  unsigned int Seen(int type) const;
  void ClearSeen();
  void Statistics();
  void ModifyPaketSize(int mod);
  // Data Processing
  int Process(const uchar *data, int len, int pid);
  void Redirect(eRule ru);
  void Clear();
  virtual int Output(const uchar *data, int len) { return len; }
  virtual int Action1(int type, int pid, uchar *data, int len) { return len; }
  virtual int Action2(int type, int pid, uchar *data, int len) { return len; }
  virtual int Action3(int type, int pid, uchar *data, int len) { return len; }
  virtual int Action4(int type, int pid, uchar *data, int len) { return len; }
  virtual int Action5(int type, int pid, uchar *data, int len) { return len; }
  virtual int Action6(int type, int pid, uchar *data, int len) { return len; }
  virtual int Action7(int type, int pid, uchar *data, int len) { return len; }
  virtual int Action8(int type, int pid, uchar *data, int len) { return len; }
  virtual void Skipped(uchar *data, int len) {}
public:
  cPES(eRule ru=prPass);
  virtual ~cPES();
  };

#endif
