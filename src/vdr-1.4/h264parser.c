/*
* h264parser.c: a minimalistic H.264 video stream parser
*
* See the main source file 'vdr.c' for copyright information and
* how to reach the author.
*
* The code was originally written by Reinhard Nissl <rnissl@gmx.de>,
* and adapted to the VDR coding style by Klaus.Schmidinger@cadsoft.de.
* and modified to work with reelvdr remux by Dirk Leber
*/

#include "debug.h"
#include "tools.h"
#include "h264parser.h"

namespace H264
{
  // --- cContext ------------------------------------------------------------
    void dump(const char *p, const uchar *d, int l, int m = 0) 
    {
        int i;
        if(!m) m = l;
        PRINTF("%s [%d/%d]:\n", p, m, l);
        for (i=0; (i<l) && (i<m); i++) {
            PRINTF("%2.2x ", d[i]);
            if(i % 32 == 31)
                PRINTF("\n");
            else if(i % 8 == 7)
                PRINTF("- ");
        } // for
        PRINTF("\n");
    }

  int cContext::GetFramesPerSec(void) const
  {
    const cSequenceParameterSet *SPS = ActiveSPS();
    const cSliceHeader *SH = CurrentSlice();
    if (!SH || !SPS->timing_info_present_flag || !SPS->time_scale || !SPS->num_units_in_tick)
       return -1;
    uint32_t DeltaTfiDivisor;
    if (SPS->pic_struct_present_flag) {
       if (!SPS->pic_timing_sei.Defined())
          return -1;
       switch (SPS->pic_timing_sei.pic_struct) {
         case 1:
         case 2:
              DeltaTfiDivisor = 1;
              break;
         case 0:
         case 3:
         case 4:
              DeltaTfiDivisor = 2;
              break;
         case 5:
         case 6:
              DeltaTfiDivisor = 3;
              break;
         case 7:
              DeltaTfiDivisor = 4;
              break;
         case 8:
              DeltaTfiDivisor = 6;
              break;
         default:
              return -1;
         }
       }
    else if (!SH->field_pic_flag)
       DeltaTfiDivisor = 2;
    else
       DeltaTfiDivisor = 1;

    double FPS = (double)SPS->time_scale / SPS->num_units_in_tick / DeltaTfiDivisor / (SH->field_pic_flag ? 2 : 1);
    int FramesPerSec = (int)FPS;
    if ((FPS - FramesPerSec) >= 0.5)
       FramesPerSec++;
    return FramesPerSec;
  }

  // --- cSimpleBuffer -------------------------------------------------------

  cSimpleBuffer::cSimpleBuffer(int Size)
  {
    size = Size;
    data = new uchar[size];
    avail = 0;
    gotten = 0;
  }

  cSimpleBuffer::~cSimpleBuffer()
  {
    delete []data;
  }

  int cSimpleBuffer::Put(const uchar *Data, int Count)
  {
    if (Count < 0) {
       if (avail + Count < 0)
          Count = 0 - avail;
       if (avail + Count < gotten)
          Count = gotten - avail;
       avail += Count;
       return Count;
    }
    if (avail + Count > size)
       Count = size - avail;
    memcpy(data + avail, Data, Count);
    avail += Count;
    return Count;
  }

  uchar *cSimpleBuffer::Get(int &Count)
  {
    Count = gotten = avail;
    return data;
  }

  void cSimpleBuffer::Del(int Count)
  {
    if (Count < 0)
       return;
    if (Count > gotten) {
       esyslog("ERROR: invalid Count in H264::cSimpleBuffer::Del: %d (limited to %d)", Count, gotten);
       Count = gotten;
       }
    if (Count < avail)
       memmove(data, data + Count, avail - Count);
    avail -= Count;
    gotten = 0;
  }

  void cSimpleBuffer::Clear(void)
  {
    avail = gotten = 0;
  }

  // --- cParser -------------------------------------------------------------

  cParser::cParser(bool OmitPicTiming)
    : nalUnitDataBuffer(1000)
  {
    // the above buffer size of 1000 bytes wont hold a complete NAL unit but
    // should be sufficient for the relevant part used for parsing.
    omitPicTiming = OmitPicTiming; // only necessary to determine frames per second
    Reset();
  }

  void cParser::Reset(void)
  {
    context = cContext();
    nalUnitDataBuffer.Clear();
    syncing = true;
    last_i_frame = false;
    skip_frames=0;
  }

  void cParser::ParseSequenceParameterSet(uint8_t *Data, int Count)
  {
    cSequenceParameterSet SPS;

    cBitReader br(Data + 1, Count - 1);
    uint32_t profile_idc = br.u(8);
    /* uint32_t constraint_set0_flag = */ br.u(1);
    /* uint32_t constraint_set1_flag = */ br.u(1);
    /* uint32_t constraint_set2_flag = */ br.u(1);
    /* uint32_t constraint_set3_flag = */ br.u(1);
    /* uint32_t reserved_zero_4bits = */ br.u(4);
    /* uint32_t level_idc = */ br.u(8);
    SPS.seq_parameter_set_id = br.ue();
    if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 144) {
       uint32_t chroma_format_idc = br.ue();
       if (chroma_format_idc == 3) {
          /* uint32_t residual_colour_transform_flag = */ br.u(1);
          }
       /* uint32_t bit_depth_luma_minus8 = */ br.ue();
       /* uint32_t bit_depth_chroma_minus8 = */ br.ue();
       /* uint32_t qpprime_y_zero_transform_bypass_flag = */ br.u(1);
       uint32_t seq_scaling_matrix_present_flag = br.u(1);
       if (seq_scaling_matrix_present_flag) {
          for (int i = 0; i < 8; i++) {
              uint32_t seq_scaling_list_present_flag = br.u(1);
              if (seq_scaling_list_present_flag) {
                 int sizeOfScalingList = (i < 6) ? 16 : 64;
                 int lastScale = 8;
                 int nextScale = 8;
                 for (int j = 0; j < sizeOfScalingList; j++) {
                     if (nextScale != 0) {
                        int32_t delta_scale = br.se();
                        nextScale = (lastScale + delta_scale + 256) % 256;
                        }
                     lastScale = (nextScale == 0) ? lastScale : nextScale;
                     }
                 }
              }
          }
       }
    SPS.log2_max_frame_num_minus4(br.ue());
    SPS.pic_order_cnt_type = br.ue();
    if (SPS.pic_order_cnt_type == 0)
       SPS.log2_max_pic_order_cnt_lsb_minus4(br.ue());
    else if (SPS.pic_order_cnt_type == 1) {
       SPS.delta_pic_order_always_zero_flag = br.u(1);
       /* int32_t offset_for_non_ref_pic = */ br.se();
       /* int32_t offset_for_top_to_bottom_field = */ br.se();
       uint32_t num_ref_frames_in_pic_order_cnt_cycle = br.ue();
       for (uint32_t i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
           /* int32_t offset_for_ref_frame = */ br.se();
           }
       }
    /* uint32_t num_ref_frames = */ br.ue();
    /* uint32_t gaps_in_frame_num_value_allowed_flag = */ br.u(1);
    /* uint32_t pic_width_in_mbs_minus1 = */ br.ue();
    /* uint32_t pic_height_in_map_units_minus1 = */ br.ue();
    SPS.frame_mbs_only_flag = br.u(1);

    if (!omitPicTiming) {
       if (!SPS.frame_mbs_only_flag) {
          /* uint32_t mb_adaptive_frame_field_flag = */ br.u(1);
          }
       /* uint32_t direct_8x8_inference_flag = */ br.u(1);
       uint32_t frame_cropping_flag = br.u(1);
       if (frame_cropping_flag) {
          /* uint32_t frame_crop_left_offset = */ br.ue();
          /* uint32_t frame_crop_right_offset = */ br.ue();
          /* uint32_t frame_crop_top_offset = */ br.ue();
          /* uint32_t frame_crop_bottom_offset = */ br.ue();
          }
       uint32_t vui_parameters_present_flag = br.u(1);
       if (vui_parameters_present_flag) {
          uint32_t aspect_ratio_info_present_flag = br.u(1);
          if (aspect_ratio_info_present_flag) {
             uint32_t aspect_ratio_idc = br.u(8);
             const uint32_t Extended_SAR = 255;
             if (aspect_ratio_idc == Extended_SAR) {
                /* uint32_t sar_width = */ br.u(16);
                /* uint32_t sar_height = */ br.u(16);
                }
             }
          uint32_t overscan_info_present_flag = br.u(1);
          if (overscan_info_present_flag) {
             /* uint32_t overscan_appropriate_flag = */ br.u(1);
             }
          uint32_t video_signal_type_present_flag = br.u(1);
          if (video_signal_type_present_flag) {
             /* uint32_t video_format = */ br.u(3);
             /* uint32_t video_full_range_flag = */ br.u(1);
             uint32_t colour_description_present_flag = br.u(1);
             if (colour_description_present_flag) {
                /* uint32_t colour_primaries = */ br.u(8);
                /* uint32_t transfer_characteristics = */ br.u(8);
                /* uint32_t matrix_coefficients = */ br.u(8);
                }
             }
          uint32_t chroma_loc_info_present_flag = br.u(1);
          if (chroma_loc_info_present_flag) {
             /* uint32_t chroma_sample_loc_type_top_field = */ br.ue();
             /* uint32_t chroma_sample_loc_type_bottom_field = */ br.ue();
             }
          SPS.timing_info_present_flag = br.u(1);
          if (SPS.timing_info_present_flag) {
             SPS.num_units_in_tick = br.u(32);
             SPS.time_scale = br.u(32);
             SPS.fixed_frame_rate_flag = br.u(1);
             }
          SPS.nal_hrd_parameters_present_flag = br.u(1);
          if (SPS.nal_hrd_parameters_present_flag)
             hrd_parameters(SPS, br);
          SPS.vcl_hrd_parameters_present_flag = br.u(1);
          if (SPS.vcl_hrd_parameters_present_flag)
             hrd_parameters(SPS, br);
          if (SPS.nal_hrd_parameters_present_flag || SPS.vcl_hrd_parameters_present_flag) {
             /* uint32_t low_delay_hrd_flag = */ br.u(1);
             }
          SPS.pic_struct_present_flag = br.u(1);
          }
       }

    context.Define(SPS);
  }

  void cParser::hrd_parameters(cSequenceParameterSet &SPS, cBitReader &br)
  {
    uint32_t cpb_cnt_minus1 = br.ue();
    /* uint32_t bit_rate_scale = */ br.u(4);
    /* uint32_t cpb_size_scale = */ br.u(4);
    for (uint32_t i = 0; i <= cpb_cnt_minus1; i++) {
        /* uint32_t bit_rate_value_minus1 = */ br.ue();
        /* uint32_t cpb_size_value_minus1 = */ br.ue();
        /* uint32_t cbr_flag = */ br.u(1);
        }
    /* uint32_t initial_cpb_removal_delay_length_minus1 = */ br.u(5);
    SPS.cpb_removal_delay_length_minus1(br.u(5));
    SPS.dpb_output_delay_length_minus1(br.u(5));
    /* uint32_t time_offset_length = */ br.u(5);
  }

  void cParser::ParsePictureParameterSet(uint8_t *Data, int Count)
  {
    cPictureParameterSet PPS;

    cBitReader br(Data + 1, Count - 1);
    PPS.pic_parameter_set_id = br.ue();
    PPS.seq_parameter_set_id = br.ue();
    /* uint32_t entropy_coding_mode_flag = */ br.u(1);
    PPS.pic_order_present_flag = br.u(1);

    context.Define(PPS);
  }

void cParser::ParseSlice(uint8_t *Data, int Count) {
    cSliceHeader SH;
    cBitReader br(Data + 1, Count - 1);
    SH.nal_ref_idc(Data[0] >> 5);
    SH.nal_unit_type(Data[0] & 0x1F);
/* uint32_t first_mb_in_slice = */ br.ue();
    SH.slice_type = br.ue();
    SH.pic_parameter_set_id = br.ue();

    context.ActivatePPS(SH.pic_parameter_set_id);
    const cSequenceParameterSet *SPS = context.ActiveSPS();

    SH.frame_num = br.u(SPS->log2_max_frame_num());
    if (!SPS->frame_mbs_only_flag) {
        SH.field_pic_flag = br.u(1);
        if (SH.field_pic_flag)
            SH.bottom_field_flag = br.u(1);
    }
    if (SH.nal_unit_type() == 5)
        SH.idr_pic_id = br.ue();
    if (SPS->pic_order_cnt_type == 0) {
        SH.pic_order_cnt_lsb = br.u(SPS->log2_max_pic_order_cnt_lsb());
        const cPictureParameterSet *PPS = context.ActivePPS();
        if (PPS->pic_order_present_flag && !SH.field_pic_flag)
        SH.delta_pic_order_cnt_bottom = br.se();
    }
    if (SPS->pic_order_cnt_type == 1 && !SPS->delta_pic_order_always_zero_flag) {
        SH.delta_pic_order_cnt[0] = br.se();
        const cPictureParameterSet *PPS = context.ActivePPS();
        if (PPS->pic_order_present_flag && !SH.field_pic_flag)
            SH.delta_pic_order_cnt[1] = br.se();
    }
    context.Define(SH);
}

  void cParser::ParseSEI(uint8_t *Data, int Count)
  {
    // currently only used to determine frames per second
    if (omitPicTiming)
       return;
    cBitReader br(Data + 1, Count - 1);
    do
      sei_message(br);
    while (br.GetBytesAvail());
  }

  void cParser::sei_message(cBitReader &br)
  {
    uint32_t payloadType = 0;
    while (1) {
          uint32_t last_payload_type_byte = br.u(8);
          payloadType += last_payload_type_byte;
          if (last_payload_type_byte != 0xFF)
             break;
          }
    uint32_t payloadSize = 0;
    while (1) {
          uint32_t last_payload_size_byte = br.u(8);
          payloadSize += last_payload_size_byte;
          if (last_payload_size_byte != 0xFF)
             break;
          }
    sei_payload(payloadType, payloadSize, br);
  }

  void cParser::sei_payload(uint32_t payloadType, uint32_t payloadSize, cBitReader &br)
  {
    const cBitReader::cBookMark BookMark = br.BookMark();
    switch (payloadType) {
      case 0:
           buffering_period(payloadSize, br);
           break;
      case 1:
           pic_timing(payloadSize, br);
           break;
      }
    // instead of dealing with trailing bits in each message
    // go back to start of message and skip it completely
    br.BookMark(BookMark);
    reserved_sei_message(payloadSize, br);
  }

  void cParser::buffering_period(uint32_t payloadSize, cBitReader &br)
  {
    uint32_t seq_parameter_set_id = br.ue();

    context.ActivateSPS(seq_parameter_set_id);
  }

  void cParser::pic_timing(uint32_t payloadSize, cBitReader &br)
  {
    cPictureTiming PT;

    const cSequenceParameterSet *SPS = context.ActiveSPS();
    if (!SPS)
       return;
    uint32_t CpbDpbDelaysPresentFlag = SPS->nal_hrd_parameters_present_flag || SPS->vcl_hrd_parameters_present_flag;
    if (CpbDpbDelaysPresentFlag) {
       /* uint32_t cpb_removal_delay = */ br.u(SPS->cpb_removal_delay_length());
       /* uint32_t dpb_output_delay = */ br.u(SPS->dpb_output_delay_length());
       }
    if (SPS->pic_struct_present_flag) {
       PT.pic_struct = br.u(4);
       }

    context.Define(PT);
  }

  void cParser::reserved_sei_message(uint32_t payloadSize, cBitReader &br)
  {
    for (uint32_t i = 0; i < payloadSize; i++) {
        /* uint32_t reserved_sei_message_payload_byte = */ br.u(8);
        }
  }

void cParser::PutNalUnitData(const uchar *Data, int Count) {
    int n = nalUnitDataBuffer.Put(Data, Count);
    // typically less than a complete NAL unit are needed for parsing the
    // relevant data, so simply ignore the overflow condition.
    if (false && n != Count)
        esyslog("ERROR: H264::cParser::PutNalUnitData(): NAL unit data buffer overflow");
}

  void cParser::Process()
  {
    // nalUnitDataBuffer contains the head of the current NAL unit -- let's parse it 
    int Count = 0;
    uchar *Data = nalUnitDataBuffer.Get(Count);
    if (Data && Count >= 4) {
       if (Data[0] == 0x00 && Data[1] == 0x00 && Data[2] == 0x01) {
          int nal_unit_type = Data[3] & 0x1F;
          try {
              switch (nal_unit_type) {
                case 1: // coded slice of a non-IDR picture
                case 2: // coded slice data partition A
                case 5: // coded slice of an IDR picture
                     ParseSlice(Data + 3, Count - 3);
                     break;
                case 6: // supplemental enhancement information (SEI)
                     ParseSEI(Data + 3, Count - 3);
                     break;
                case 7: // sequence parameter set
                     syncing = false; // from now on, we should get reliable results
                     ParseSequenceParameterSet(Data + 3, Count - 3);
                     break;
                case 8: // picture parameter set
                     ParsePictureParameterSet(Data + 3, Count - 3);
                     break;
                }
              }
          catch (cException *e) {
              if (!syncing) // suppress typical error messages while syncing
                 esyslog(e->Message());
              delete e;
              }
          }
       else if (!syncing)
          esyslog("ERROR: H264::cParser::Process(): NAL unit data buffer content is invalid");
       }
    else if (!syncing)
       esyslog("ERROR: H264::cParser::Process(): NAL unit data buffer content is too short");
    // reset the buffer for the next NAL unit
    nalUnitDataBuffer.Clear();
  }

#define MAX_TS_PAYLOAD (184*8)
#define TS_PACKET_SIZE 188
bool GetTSPayload(const uchar *Data, int PacketNr, int &StreamPID, uchar *Dest, int &Count) {
    int pos = PacketNr*TS_PACKET_SIZE;
    Count = 0;
    if(Data[PacketNr*TS_PACKET_SIZE] != 0x47) {
        //dump("cParser::PutTsData", &Data[pos], 188);
        PRINTF("Invalid sync byte 0x%x at packet %d offset %d\n", Data[pos], PacketNr, pos);
        return false;
    } // if
    if((Data[pos+3] & 0x10) != 0x10) {
//      PRINTF("No payload!\n");
        return true; // no payload
    } // if
    int PID = ((Data[pos+1]<<8)+Data[pos+2])&0x1FFF;
    if(!StreamPID) {
        StreamPID = PID;
        PRINTF("Setting pid %x %x!\n", PID, StreamPID);
    } else if(StreamPID != PID) {// Not wanted
//      PRINTF("Invalid pid %x %x!\n", PID, StreamPID);
        return true;
    } // if
    Count = pos+TS_PACKET_SIZE;
    if(Data[pos+3] & 0x20) // adaption field present
        pos += Data[pos+4]+1; // add adaption field length
    pos+=4;;
    Count -= pos;
    memcpy(Dest, &Data[pos], Count);
    return true;
} // GetTSPayload

bool GetTSData(uchar *Data, int MaxDataCount, int PacketNr, int MaxPackets, int &StreamPID, uchar *Dest, int &Count) {
    int Size;
    Count = 0;
    if(!GetTSPayload(Data, PacketNr, StreamPID, Dest, Size))
        return false;
    else if(!Size)
        return true;
    Count += Size;
    PacketNr++;
    while((PacketNr<MaxPackets) && (MaxDataCount >= Count+(TS_PACKET_SIZE-4))) {
        if(!GetTSPayload(Data, PacketNr, StreamPID, &Dest[Count], Size))
            return false;
        Count += Size;
        PacketNr++;
    } // while
    return true;
} // GetTSData

static const uint8_t slice_type_map[5] = {FRAME_P_TYPE, FRAME_B_TYPE, FRAME_I_TYPE, FRAME_SP_TYPE, FRAME_SI_TYPE};

//#define DEBUG_NALS
#ifdef DEBUG_NALS
int found=0;
#endif
bool cParser::GetTSFrame(uchar *Data, int DataCount, int PID, int &VideoFormat, uchar &PicType) {
    uchar buff[MAX_TS_PAYLOAD];
    int PacketCount = DataCount / TS_PACKET_SIZE;
    int Size;
    if(!GetTSData(Data, MAX_TS_PAYLOAD, 0, PacketCount, PID, buff, Size) || Size <= 0)
        return false;
    int Start = FindPacketHeader(buff, 0, Size);
    if(Start == -1)
        return false;
    while((Start>=0) && (Start<Size)) {
        uchar *BuffData = &buff[Start];
        int BuffCount = Size-Start;
        int mp2_format = (BuffData[5] >> 3) & 0x07;
        if(!BuffData[3] && mp2_format) { // mpeg2 picture start code
            //TODO: Check mpeg2 detection
            VideoFormat = VIDEO_FORMAT_MPEG2;
            PicType = mp2_format;
            return true;
        } // if
        int nal_unit_type = BuffData[3] & 0x1F;
        try {
            switch (nal_unit_type) {
                case 1: // coded slice of a non-IDR picture
                case 2: // coded slice data partition A
                case 5: {// coded slice of an IDR picture
                    ParseSlice(BuffData + 3, BuffCount - 3);
                    const H264::cSliceHeader *SH = Context().CurrentSlice();
                    if (SH) {
                        VideoFormat = VIDEO_FORMAT_H264;
                        PicType = FRAME_NO_TYPE;
                        if(SH->GetAccessUnitType() == cSliceHeader::TopField || SH->GetAccessUnitType() == cSliceHeader::Frame) {
                            int slice_type = SH->slice_type;
                            if ( slice_type <= 9 ) {
                                if ( slice_type > 4 )
                                    slice_type -= 5;
                                PicType = slice_type_map[slice_type];
                            } // if
                            int fps=Context().GetFramesPerSec();
                            if(SH->GetAccessUnitType() == cSliceHeader::Frame && (fps<0 || fps>30)) {
                                // Vdr expects 25 fps but we assume progressive is 50 fps but we want to keep i-frames
                                //TODO: Add variable framerate to vdr or always assume 50 fps (and signal Top&Bottom Fields)
                                //-> How to signal this with mpeg2/PES-Data?
                                skip_frames++;
                                if(PicType == FRAME_I_TYPE && skip_frames < 3) {
                                    // Prefer i-frames
                                    last_i_frame = true;
                                } else if (skip_frames>1) {
                                    if(!last_i_frame) { // But don't drop after i-frame or trickmode will get dirt from this frame
                                        PicType = FRAME_NO_TYPE;
                                        skip_frames-=2;
                                    } // if
                                    last_i_frame = false;   
                                } // if
                            } // if
                        } // if
                        if(show_format) {
                            PRINTF("h264parser found %s format with %d fps\n", (SH->GetAccessUnitType() == cSliceHeader::Frame)?"progressive":"interlaced", Context().GetFramesPerSec());
                            show_format = false;
                        } // if
#ifdef DEBUG_NALS
                        if(PicType != FRAME_NO_TYPE) {
                            PRINTF("===============================================\n");
                            found=0;
                        } // if
                        switch (PicType) {
                            case FRAME_I_TYPE: PRINTF("Found I-Frame %d %s at %d (%d fps)\n", SH->frame_num, (SH->GetAccessUnitType() == cSliceHeader::Frame)?"Progressive":"Interlaced", Start, Context().GetFramesPerSec()); break;
                            case FRAME_P_TYPE: PRINTF("Found P-Frame %d %s at %d (%d fps)\n", SH->frame_num, (SH->GetAccessUnitType() == cSliceHeader::Frame)?"Progressive":"Interlaced", Start, Context().GetFramesPerSec()); break;
                            case FRAME_B_TYPE: PRINTF("Found B-Frame %d %s at %d (%d fps)\n", SH->frame_num, (SH->GetAccessUnitType() == cSliceHeader::Frame)?"Progressive":"Interlaced", Start, Context().GetFramesPerSec()); break;
                            case FRAME_SI_TYPE: PRINTF("Found SI-Frame %d %s at %d (%d fps)\n", SH->frame_num, (SH->GetAccessUnitType() == cSliceHeader::Frame)?"Progressive":"Interlaced", Start, Context().GetFramesPerSec()); break;
                            case FRAME_SP_TYPE: PRINTF("Found SP-Frame %d %s at %d (%d fps)\n", SH->frame_num, (SH->GetAccessUnitType() == cSliceHeader::Frame)?"Progressive":"Interlaced", Start, Context().GetFramesPerSec()); break;
                        } // switch
                        if(PicType == FRAME_I_TYPE) {
                            found=1;
                            parse_nals(Data, DataCount, 0x2ff);
                        } // if
#endif
                        if(PicType != FRAME_NO_TYPE) return true; // We found a frame!
                    } // if
                    break;
                } // case
                case 6: // supplemental enhancement information (SEI)
                    try {
                        ParseSEI(BuffData + 3, BuffCount - 3);
                    } catch (cException *e) {
                        delete e;
                    } // ignore errors due to end of buffer
                    break;
                case 7: // sequence parameter set
                    if(syncing) {
                        syncing = false; // from now on, we should get reliable results
                        show_format = true;
                        PRINTF("h264parser synced!\n");
                    } // if
                    ParseSequenceParameterSet(BuffData + 3, BuffCount - 3);
                    break;
                case 8: // picture parameter set
                    ParsePictureParameterSet(BuffData + 3, BuffCount - 3);
                    break;
                case 9: { // access unit delimiter
                    const H264::cSliceHeader *SH = Context().CurrentSlice();
                    if (!SH && syncing) { // No interlace/progressive information yet -> get the hint from aud
                        VideoFormat = VIDEO_FORMAT_H264;
                        cBitReader br(BuffData + 4, BuffCount - 4);
                        int flag = br.u(3);
//                      PRINTF("AUD: %d -> %s at %d\n", flag, flag?"non I-Frame":"I-Frame", Start);
                        PicType = flag ? FRAME_B_TYPE : FRAME_I_TYPE;
                    } // if
                    break;
                } // case
                default:
                    break;
            } // switch
        } catch (cException *e) {
            if (!syncing) // suppress typical error messages while syncing
                esyslog(e->Message());
            delete e;
        } // try
        Start = FindPacketHeader(buff, Start+4, Size-Start-4);
    } // while
#ifdef DEBUG_NALS
    if(found) parse_nals(Data, DataCount, 0x2ff);
#endif
    return true;
} // cParser::GetTSFrame
  
#include <stdlib.h>
                 
void parse_nals(const uchar *Data, int DataCount, int StreamPID) {
    int Pos = 0;
    int Size=0;
    int Count=0;
    int PCount = DataCount/TS_PACKET_SIZE;
    int Last=0;
    uchar *Dest = (uchar *)malloc(DataCount);
    PRINTF("parse_nals %d p %d r %d\n", DataCount, PCount, DataCount%TS_PACKET_SIZE);
    for(Pos=0; Pos < PCount; Pos++) 
        if(GetTSPayload(Data, Pos, StreamPID, Dest+Size, Count))
            Size += Count;
        else {
            if(Pos) dump("Before:", &Data[Pos-1*TS_PACKET_SIZE], TS_PACKET_SIZE);
            dump("Error getting TSPayload", &Data[Pos*TS_PACKET_SIZE], DataCount-Pos*TS_PACKET_SIZE, 2*TS_PACKET_SIZE);
            free(Dest);
            return;
        } // if
    PRINTF("parse_nals %d->%d %d %d\n", DataCount, Pos*TS_PACKET_SIZE, Size, Pos);
    int Start=FindPacketHeader(Dest, 0, Size);
    while(-1 != Start) {
        Last = Start;
        //int nal_unit_type = Dest[Start+3] & 0x1F;
        //PRINTF("    %d: %d\n", Start, nal_unit_type);
        Start=FindPacketHeader(Dest, Start+4, Size-Start-4);
    } // while  
    free(Dest);
} // parse_nals

} /* namespace */
