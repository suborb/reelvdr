/*
 * h264parser.h: a minimalistic H.264 video stream parser
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 */

#ifndef __H264PARSER_H
#define __H264PARSER_H

namespace H264
{
  // --- cException ----------------------------------------------------------
#define FRAME_NO_TYPE 0
#define FRAME_I_TYPE  1
#define FRAME_P_TYPE  2
#define FRAME_B_TYPE  3
#define FRAME_SP_TYPE 4
#define FRAME_SI_TYPE 5
#define FRAME_TYPE_STR(x) (           \
	(FRAME_NO_TYPE == x) ? "NO" : \
	(FRAME_I_TYPE  == x) ? "I " : \
	(FRAME_P_TYPE  == x) ? "P " : \
	(FRAME_B_TYPE  == x) ? "B " : \
	(FRAME_SP_TYPE == x) ? "SP" : \
	(FRAME_SI_TYPE == x) ? "SI" : \
	"??")

#define VIDEO_FORMAT_UNKNOWN 0
#define VIDEO_FORMAT_MPEG2   1
#define VIDEO_FORMAT_H264    2
#define VIDEO_FORMAT_STR(x) (                     \
	(VIDEO_FORMAT_UNKNOWN == x) ? "UNKNOWN" : \
	(VIDEO_FORMAT_MPEG2   == x) ? "MPEG2"   : \
	(VIDEO_FORMAT_H264    == x) ? "H.264"   : \
	"?")

  class cException {
  private:
    cString message;
  public:
    cException(const cString &Message) { message = Message; }
    const cString &Message(void) const { return message; }
  };

  // --- cBitReader ----------------------------------------------------------

  class cBitReader {
  public:
    class cBookMark {
    private:
      uint8_t *data;
      int count;
      uint32_t bits;
      uint32_t bitsAvail;
      int countZeros;
      cBookMark(void) {}
      friend class cBitReader;
    };
  private:
    cBookMark bm;
    uint8_t NextByte(void);
    uint32_t ReadBits(uint32_t n);
  public:
    cBitReader(uint8_t *Data, int Count);
    uint32_t u(uint32_t n) { return ReadBits(n); } // read n bits as unsigned number
    uint32_t ue(void); // read Exp-Golomb coded unsigned number
    int32_t se(void); // read Exp-Golomb coded signed number
    uint32_t GetBitsAvail(void) { return (bm.bitsAvail & 0x07); }
    bool GetBytesAvail(void) { return (bm.count > 0); }
    const cBookMark BookMark(void) const { return bm; }
    void BookMark(const cBookMark &b) { bm = b; }
  };

  inline cBitReader::cBitReader(unsigned char *Data, int Count)
  {
    bm.data = Data;
    bm.count = Count;
    bm.bitsAvail = 0;
    bm.countZeros = 0;
  }

  inline uint8_t cBitReader::NextByte(void)
  {
    if (bm.count < 1) // there is no more data left in this NAL unit
       throw new cException("ERROR: H264::cBitReader::NextByte(): premature end of data");
    // detect 00 00 00, 00 00 01 and 00 00 03 and handle them
    if (*bm.data == 0x00) {
       if (bm.countZeros >= 3) // 00 00 00: the current NAL unit should have been terminated already before this sequence
          throw new cException("ERROR: H264::cBitReader::NextByte(): premature end of data");
       // increase the zero counter as we have a zero byte
       bm.countZeros++;
       }
    else {
       if (bm.countZeros >= 2) {
          if (*bm.data == 0x01) // 00 00 01: the current NAL unit should have been terminated already before this sequence
             throw new cException("ERROR: H264::cBitReader::NextByte(): premature end of data");
          if (*bm.data == 0x03) {
             // 00 00 03 xx: the emulation prevention byte 03 needs to be removed and xx must be returned
             if (bm.count < 2)
                throw new cException("ERROR: H264::cBitReader::NextByte(): premature end of data");
             // drop 03 and xx will be returned below
             bm.count--;
             bm.data++;
             }
          }
       // reset the zero counter as we had a non zero byte
       bm.countZeros = 0;
       }
    bm.count--;
    return *bm.data++;
  }

  inline uint32_t cBitReader::ReadBits(uint32_t n)
  {
    // fill the "shift register" bits with sufficient data
    while (n > bm.bitsAvail) {
          bm.bits <<= 8;
          bm.bits |= NextByte();
          bm.bitsAvail += 8;
          if (bm.bitsAvail > 24) { // a further turn will overflow bitbuffer
             if (n <= bm.bitsAvail)
                break; // service non overflowing request
             if (n <= 32) // split overflowing reads into concatenated reads 
                return (ReadBits(16) << 16) | ReadBits(n - 16);
             // cannot read more than 32 bits at once
             throw new cException("ERROR: H264::cBitReader::ReadBits(): bitbuffer overflow");
             }
          }
    // return n most significant bits
    bm.bitsAvail -= n;
    return (bm.bits >> bm.bitsAvail) & (((uint32_t)1 << n) - 1);
  }

  inline uint32_t cBitReader::ue(void)
  {
    // read and decode an Exp-Golomb coded unsigned number
    //
    // bitstring             resulting number
    //       1               0
    //     0 1 x             1 ... 2
    //   0 0 1 x y           3 ... 6
    // 0 0 0 1 x y z         7 ... 14
    // ...
    int LeadingZeroBits = 0;
    while (ReadBits(1) == 0)
          LeadingZeroBits++;
    if (LeadingZeroBits == 0)
       return 0;
    if (LeadingZeroBits >= 32)
       throw new cException("ERROR: H264::cBitReader::ue(): overflow");
    return ((uint32_t)1 << LeadingZeroBits) - 1 + ReadBits(LeadingZeroBits);
  }

  inline int32_t cBitReader::se(void)
  {
    // read and decode an Exp-Golomb coded signed number
    //
    // unsigned value       resulting signed value
    // 0                     0
    // 1                    +1
    // 2                    -1
    // 3                    +2
    // 4                    -2
    // ...
    uint32_t r = ue();
    if (r > 0xFFFFFFFE)
       throw new cException("ERROR: H264::cBitReader::se(): overflow");
    return (1 - 2 * (r & 1)) * ((r + 1) / 2);
  }

  // --- cPictureTiming ------------------------------------------------------

  class cPictureTiming {
  private:
    friend class cContext;
    bool defined;
  public:
    cPictureTiming(void) { memset(this, 0, sizeof (*this)); }
    bool Defined(void) const { return defined; }
    uint32_t pic_struct;
  };

  // --- cSequenceParameterSet -----------------------------------------------

  class cSequenceParameterSet {
  private:
    friend class cContext;
    bool defined;
    uint32_t log2MaxFrameNum;
    uint32_t log2MaxPicOrderCntLsb;
    uint32_t cpbRemovalDelayLength;
    uint32_t dpbOutputDelayLength;
  public:
    cSequenceParameterSet(void);
    bool Defined(void) { return defined; }
    void log2_max_frame_num_minus4(uint32_t Value) { log2MaxFrameNum = Value + 4; }
    uint32_t log2_max_frame_num_minus4(void) const { return log2MaxFrameNum - 4; }
    uint32_t log2_max_frame_num(void) const { return log2MaxFrameNum; }
    void log2_max_pic_order_cnt_lsb_minus4(uint32_t Value) { log2MaxPicOrderCntLsb = Value + 4; }
    uint32_t log2_max_pic_order_cnt_lsb_minus4(void) const { return log2MaxPicOrderCntLsb - 4; }
    uint32_t log2_max_pic_order_cnt_lsb(void) const { return log2MaxPicOrderCntLsb; }
    void cpb_removal_delay_length_minus1(uint32_t Value) { cpbRemovalDelayLength = Value + 1; }
    uint32_t cpb_removal_delay_length_minus1(void) const { return cpbRemovalDelayLength - 1; }
    uint32_t cpb_removal_delay_length(void) const { return cpbRemovalDelayLength; }
    void dpb_output_delay_length_minus1(uint32_t Value) { dpbOutputDelayLength = Value + 1; }
    uint32_t dpb_output_delay_length_minus1(void) const { return dpbOutputDelayLength - 1; }
    uint32_t dpb_output_delay_length(void) const { return dpbOutputDelayLength; }
    uint32_t seq_parameter_set_id;
    uint32_t pic_order_cnt_type;
    uint32_t delta_pic_order_always_zero_flag;
    uint32_t frame_mbs_only_flag;
    uint32_t timing_info_present_flag;
    uint32_t num_units_in_tick;
    uint32_t time_scale;
    uint32_t fixed_frame_rate_flag;
    uint32_t nal_hrd_parameters_present_flag;
    uint32_t vcl_hrd_parameters_present_flag;
    uint32_t pic_struct_present_flag;
    cPictureTiming pic_timing_sei;
  };

  inline cSequenceParameterSet::cSequenceParameterSet(void)
  {
    memset(this, 0, sizeof (*this));
    log2_max_frame_num_minus4(0);
    log2_max_pic_order_cnt_lsb_minus4(0);
    cpb_removal_delay_length_minus1(23);
    dpb_output_delay_length_minus1(23);
  }

  // --- cPictureParameterSet ------------------------------------------------

  class cPictureParameterSet {
  private:
    friend class cContext;
    bool defined;
  public:
    cPictureParameterSet(void) { memset(this, 0, sizeof (*this)); }
    bool Defined(void) { return defined; }
    uint32_t pic_parameter_set_id;
    uint32_t seq_parameter_set_id;
    uint32_t pic_order_present_flag;
  };

  // --- cSliceHeader --------------------------------------------------------

  class cSliceHeader {
  private:
    friend class cContext;
    bool defined;
    bool isFirstSliceOfCurrentAccessUnit;
    uint32_t picOrderCntType;
    uint32_t nalRefIdc;
    uint32_t nalUnitType;
  public:
    cSliceHeader(void) { memset(this, 0, sizeof (*this)); }
    bool Defined(void) const { return defined; }
    bool IsFirstSliceOfCurrentAccessUnit(void) const { return isFirstSliceOfCurrentAccessUnit; }
    void nal_ref_idc(uint32_t Value) { nalRefIdc = Value; }
    uint32_t nal_ref_idc(void) const { return nalRefIdc; }
    void nal_unit_type(uint32_t Value) { nalUnitType = Value; }
    uint32_t nal_unit_type(void) const { return nalUnitType; }
    uint32_t slice_type;
    uint32_t pic_parameter_set_id;
    uint32_t frame_num;
    uint32_t field_pic_flag;
    uint32_t bottom_field_flag;
    uint32_t idr_pic_id;
    uint32_t pic_order_cnt_lsb;
    int32_t delta_pic_order_cnt_bottom;
    int32_t delta_pic_order_cnt[2];
    enum eAccessUnitType {
      Frame = 0,
      TopField,
      BottomField
      };
    eAccessUnitType GetAccessUnitType() const { return (eAccessUnitType)(field_pic_flag + bottom_field_flag); }
  };
  #define ACCESS_UNIT_STR(x) (          \
  	(cSliceHeader::Frame       == x) ? "FRAME"  : \
  	(cSliceHeader::TopField    == x) ? "TOP"    : \
  	(cSliceHeader::BottomField == x) ? "BOTTOM" : \
  	"?")

  // --- cContext ------------------------------------------------------------

  class cContext {
  private:
    cSequenceParameterSet spsStore[32];
    cPictureParameterSet ppsStore[256];
    cSequenceParameterSet *sps; // active Sequence Parameter Set
    cPictureParameterSet *pps; // active Picture Parameter Set
    cSliceHeader sh;
  public:
    cContext(void) { sps = 0; pps = 0; }
    void Define(cSequenceParameterSet &SPS);
    void Define(cPictureParameterSet &PPS);
    void Define(cSliceHeader &SH);
    void Define(cPictureTiming &PT);
    void ActivateSPS(uint32_t ID);
    void ActivatePPS(uint32_t ID);
    const cSequenceParameterSet *ActiveSPS(void) const { return sps; }
    const cPictureParameterSet *ActivePPS(void) const { return pps; }
    const cSliceHeader *CurrentSlice(void) const { return sh.Defined() ? &sh : 0; }
    int GetFramesPerSec(void) const;
  };

  inline void cContext::ActivateSPS(uint32_t ID)
  {
	  if (ID >= (sizeof (spsStore) / sizeof (*spsStore)))
       throw new cException("ERROR: H264::cContext::ActivateSPS(): id out of range");
	  if (!spsStore[ID].Defined())
       throw new cException("ERROR: H264::cContext::ActivateSPS(): requested SPS is undefined");
	  sps = &spsStore[ID];
  }

  inline void cContext::ActivatePPS(uint32_t ID)
  {
    if (ID >= (sizeof (ppsStore) / sizeof (*ppsStore)))
       throw new cException("ERROR: H264::cContext::ActivatePPS(): id out of range");
    if (!ppsStore[ID].Defined())
       throw new cException("ERROR: H264::cContext::ActivatePPS(): requested PPS is undefined");
    pps = &ppsStore[ID];
    ActivateSPS(pps->seq_parameter_set_id);
  }

  inline void cContext::Define(cSequenceParameterSet &SPS)
  {
    if (SPS.seq_parameter_set_id >= (sizeof (spsStore) / sizeof (*spsStore)))
       throw new cException("ERROR: H264::cContext::DefineSPS(): id out of range");
    SPS.defined = true;
    spsStore[SPS.seq_parameter_set_id] = SPS;
  }

  inline void cContext::Define(cPictureParameterSet &PPS)
  {
    if (PPS.pic_parameter_set_id >= (sizeof (ppsStore) / sizeof (*ppsStore)))
       throw new cException("ERROR: H264::cContext::DefinePPS(): id out of range");
    PPS.defined = true;
    ppsStore[PPS.pic_parameter_set_id] = PPS;
  }

  inline void cContext::Define(cSliceHeader &SH)
  {
    SH.defined = true;
    SH.picOrderCntType = ActiveSPS()->pic_order_cnt_type;

    // ITU-T Rec. H.264 (03/2005): 7.4.1.2.4
    SH.isFirstSliceOfCurrentAccessUnit = !sh.Defined()
      || (sh.frame_num                  != SH.frame_num)
      || (sh.pic_parameter_set_id       != SH.pic_parameter_set_id)
      || (sh.field_pic_flag             != SH.field_pic_flag)
      || (sh.bottom_field_flag          != SH.bottom_field_flag)
      || (sh.nalRefIdc                  != SH.nalRefIdc
      && (sh.nalRefIdc == 0             || SH.nalRefIdc == 0))
      || (sh.picOrderCntType == 0       && SH.picOrderCntType == 0
      && (sh.pic_order_cnt_lsb          != SH.pic_order_cnt_lsb
      ||  sh.delta_pic_order_cnt_bottom != SH.delta_pic_order_cnt_bottom))
      || (sh.picOrderCntType == 1       && SH.picOrderCntType == 1
      && (sh.delta_pic_order_cnt[0]     != SH.delta_pic_order_cnt[0]
      ||  sh.delta_pic_order_cnt[1]     != SH.delta_pic_order_cnt[1]))
      || (sh.nalUnitType                != SH.nalUnitType
      && (sh.nalUnitType == 5           || SH.nalUnitType == 5))
      || (sh.nalUnitType == 5           && SH.nalUnitType == 5
      &&  sh.idr_pic_id                 != SH.idr_pic_id);
        
    sh = SH;
  }

  inline void cContext::Define(cPictureTiming &PT)
  {
    PT.defined = true;
    ((cSequenceParameterSet *)ActiveSPS())->pic_timing_sei = PT;
  }

  // --- cSimpleBuffer -------------------------------------------------------

  class cSimpleBuffer {
  private:
    uchar *data;
    int size;
    int avail;
    int gotten;
  public:
    cSimpleBuffer(int Size);
    ~cSimpleBuffer();
    int Size(void) { return size; }
    int Available(void) { return avail; }
    int Free(void) { return size - avail; }
    int Put(const uchar *Data, int Count);
    uchar *Get(int &Count);
    void Del(int Count);
    void Clear(void);
  };
  
  // --- cParser -------------------------------------------------------------

  class cParser {
  private:
    bool syncing;
    bool omitPicTiming;
    cContext context;
    cSimpleBuffer nalUnitDataBuffer;
    int skip_frames;
    bool last_i_frame;
    bool show_format;
    void hrd_parameters(cSequenceParameterSet &SPS, cBitReader &br);
    void ParseSequenceParameterSet(uint8_t *Data, int Count);
    void ParsePictureParameterSet(uint8_t *Data, int Count);
    void ParseSlice(uint8_t *Data, int Count);
    void reserved_sei_message(uint32_t payloadSize, cBitReader &br);
    void pic_timing(uint32_t payloadSize, cBitReader &br);
    void buffering_period(uint32_t payloadSize, cBitReader &br);
    void sei_payload(uint32_t payloadType, uint32_t payloadSize, cBitReader &br);
    void sei_message(cBitReader &br);
    void ParseSEI(uint8_t *Data, int Count);
  public:
    cParser(bool OmitPicTiming = false);
    const cContext &Context(void) const { return context; }
    void PutNalUnitData(const uchar *Data, int Count);
    void Reset(void);
    void Process(void);
	bool GetTSFrame(uchar *Data, int DataCount, int PID, int &VideoFormat, uchar &PicType);
  };
  void parse_nals(const uchar *Data, int DataCount, int StreamPID);
}

#endif // __H264PARSER_H
