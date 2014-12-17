/*
 * OSD Picture in Picture plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */

#ifndef PESASSEMBLER_H
#define PESASSEMBLER_H

#define  uint32_t unsigned int

class cPesAssembler
{
  private:
    uchar * data;
    uint32_t tag;
    int length;
    int size;
    bool Realloc(int Size);
  public:
      cPesAssembler(void);
     ~cPesAssembler();
    int ExpectedLength(void)
    {
        return PacketSize(data);
    }
    static int PacketSize(const uchar * data);
    int Length(void)
    {
        return length;
    }
    const uchar *Data(void)
    {
        return data;
    }                           // only valid if Length() >= 4
    void Reset(void);
    void Put(uchar c);
    void Put(const uchar * Data, int Length);
    bool IsPes(void);
};

#endif //PESASSEMBLER_H
