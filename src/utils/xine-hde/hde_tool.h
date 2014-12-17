/***************************************************************************
 *   Copyright (C) 2008 by Reel Multimedia Vertriebs GmbH                  *
 *                         E-Mail  : info @ reel-multimedia.com            *
 *                         Internet: www.reel-multimedia.com               *
 *                                                                         *
 *   This code is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef HAVE_HDE_TOOL_H
#define HAVE_HDE_TOOL_H

#include <xine_internal.h>
#include <assert.h>

//*****************************************************************************
// libavcodec - parser stuff

typedef struct AVRational {
	uint32_t num;
	uint32_t den;
} AVRational;

//the following defines may change, don't expect compatibility if you use them
#define MB_TYPE_INTRA4x4   0x0001
#define MB_TYPE_INTRA16x16 0x0002 //FIXME h264 specific
#define MB_TYPE_INTRA_PCM  0x0004 //FIXME h264 specific
#define MB_TYPE_16x16      0x0008
#define MB_TYPE_16x8       0x0010
#define MB_TYPE_8x16       0x0020
#define MB_TYPE_8x8        0x0040
#define MB_TYPE_INTERLACED 0x0080
#define MB_TYPE_DIRECT2     0x0100 //FIXME
#define MB_TYPE_ACPRED     0x0200
#define MB_TYPE_GMC        0x0400
#define MB_TYPE_SKIP       0x0800
#define MB_TYPE_P0L0       0x1000
#define MB_TYPE_P1L0       0x2000
#define MB_TYPE_P0L1       0x4000
#define MB_TYPE_P1L1       0x8000
#define MB_TYPE_L0         (MB_TYPE_P0L0 | MB_TYPE_P1L0)
#define MB_TYPE_L1         (MB_TYPE_P0L1 | MB_TYPE_P1L1)
#define MB_TYPE_L0L1       (MB_TYPE_L0   | MB_TYPE_L1)
#define MB_TYPE_QUANT      0x00010000
#define MB_TYPE_CBP        0x00020000

#define FF_I_TYPE 1 // Intra
#define FF_P_TYPE 2 // Predicted
#define FF_B_TYPE 3 // Bi-dir predicted
#define FF_S_TYPE 4 // S(GMC)-VOP MPEG4
#define FF_SI_TYPE 5
#define FF_SP_TYPE 6

#define I_TYPE FF_I_TYPE  ///< Intra
#define P_TYPE FF_P_TYPE  ///< Predicted
#define B_TYPE FF_B_TYPE  ///< Bi-dir predicted
#define S_TYPE FF_S_TYPE  ///< S(GMC)-VOP MPEG4
#define SI_TYPE FF_SI_TYPE  ///< Switching Intra
#define SP_TYPE FF_SP_TYPE  ///< Switching Predicted

extern void av_log(void *avcl, int level, const char *fmt, ...);
#define AV_LOG_ERROR 1
#define AV_LOG_INFO  2
#define AV_LOG_DEBUG 3

#define ROUNDED_DIV(a,b) (((a)>0 ? (a) + ((b)>>1) : (a) - ((b)>>1))/(b))

#ifdef __GNUC__
#define DECLARE_ALIGNED_8(t,v)       t v __attribute__ ((aligned (8)))
#else
#define DECLARE_ALIGNED_8(t,v)      __declspec(align(8)) t v
#endif
#define STRIDE_ALIGN 8

#ifndef av_always_inline
#if defined(__GNUC__) && (__GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ > 0)
#    define av_always_inline __attribute__((always_inline)) inline
#else
#    define av_always_inline inline
#endif
#endif
#include "h264data.h"
#include "bswap.h"
#include "bitstream.h"

static inline int ff_get_fourcc(const char *s){
#ifdef HAVE_AV_CONFIG_H
	assert( strlen(s)==4 );
#endif
	return (s[0]) + (s[1]<<8) + (s[2]<<16) + (s[3]<<24);
}

#define FF_INPUT_BUFFER_PADDING_SIZE 8
#define AV_NOPTS_VALUE          INT64_C(0x8000000000000000)

enum CodecID {
	CODEC_ID_NONE,
	CODEC_ID_MPEG1VIDEO,
	CODEC_ID_MPEG2VIDEO, /* prefered ID for MPEG Video 1 or 2 decoding */
	CODEC_ID_MPEG2VIDEO_XVMC,
	CODEC_ID_H261,
	CODEC_ID_H263,
	CODEC_ID_RV10,
	CODEC_ID_RV20,
	CODEC_ID_MJPEG,
	CODEC_ID_MJPEGB,
	CODEC_ID_LJPEG,
	CODEC_ID_SP5X,
	CODEC_ID_JPEGLS,
	CODEC_ID_MPEG4,
	CODEC_ID_RAWVIDEO,
	CODEC_ID_MSMPEG4V1,
	CODEC_ID_MSMPEG4V2,
	CODEC_ID_MSMPEG4V3,
	CODEC_ID_WMV1,
	CODEC_ID_WMV2,
	CODEC_ID_H263P,
	CODEC_ID_H263I,
	CODEC_ID_FLV1,
	CODEC_ID_SVQ1,
	CODEC_ID_SVQ3,
	CODEC_ID_DVVIDEO,
	CODEC_ID_HUFFYUV,
	CODEC_ID_CYUV,
	CODEC_ID_H264,
	CODEC_ID_INDEO3,
	CODEC_ID_VP3,
	CODEC_ID_THEORA,
	CODEC_ID_ASV1,
	CODEC_ID_ASV2,
	CODEC_ID_FFV1,
	CODEC_ID_4XM,
	CODEC_ID_VCR1,
	CODEC_ID_CLJR,
	CODEC_ID_MDEC,
	CODEC_ID_ROQ,
	CODEC_ID_INTERPLAY_VIDEO,
	CODEC_ID_XAN_WC3,
	CODEC_ID_XAN_WC4,
	CODEC_ID_RPZA,
	CODEC_ID_CINEPAK,
	CODEC_ID_WS_VQA,
	CODEC_ID_MSRLE,
	CODEC_ID_MSVIDEO1,
	CODEC_ID_IDCIN,
	CODEC_ID_8BPS,
	CODEC_ID_SMC,
	CODEC_ID_FLIC,
	CODEC_ID_TRUEMOTION1,
	CODEC_ID_VMDVIDEO,
	CODEC_ID_MSZH,
	CODEC_ID_ZLIB,
	CODEC_ID_QTRLE,
	CODEC_ID_SNOW,
	CODEC_ID_TSCC,
	CODEC_ID_ULTI,
	CODEC_ID_QDRAW,
	CODEC_ID_VIXL,
	CODEC_ID_QPEG,
	CODEC_ID_XVID,
	CODEC_ID_PNG,
	CODEC_ID_PPM,
	CODEC_ID_PBM,
	CODEC_ID_PGM,
	CODEC_ID_PGMYUV,
	CODEC_ID_PAM,
	CODEC_ID_FFVHUFF,
	CODEC_ID_RV30,
	CODEC_ID_RV40,
	CODEC_ID_VC1,
	CODEC_ID_WMV3,
	CODEC_ID_LOCO,
	CODEC_ID_WNV1,
	CODEC_ID_AASC,
	CODEC_ID_INDEO2,
	CODEC_ID_FRAPS,
	CODEC_ID_TRUEMOTION2,
	CODEC_ID_BMP,
	CODEC_ID_CSCD,
	CODEC_ID_MMVIDEO,
	CODEC_ID_ZMBV,
	CODEC_ID_AVS,
	CODEC_ID_SMACKVIDEO,
	CODEC_ID_NUV,
	CODEC_ID_KMVC,
	CODEC_ID_FLASHSV,
	CODEC_ID_CAVS,
	CODEC_ID_JPEG2000,
	CODEC_ID_VMNC,
	CODEC_ID_VP5,
	CODEC_ID_VP6,
	CODEC_ID_VP6F,
	CODEC_ID_TARGA,
	CODEC_ID_DSICINVIDEO,
	CODEC_ID_TIERTEXSEQVIDEO,
	CODEC_ID_TIFF,
	CODEC_ID_GIF,
	CODEC_ID_FFH264,
		
	/* various pcm "codecs" */
	CODEC_ID_PCM_S16LE= 0x10000,
	CODEC_ID_PCM_S16BE,
	CODEC_ID_PCM_U16LE,
	CODEC_ID_PCM_U16BE,
	CODEC_ID_PCM_S8,
	CODEC_ID_PCM_U8,
	CODEC_ID_PCM_MULAW,
	CODEC_ID_PCM_ALAW,
	CODEC_ID_PCM_S32LE,
	CODEC_ID_PCM_S32BE,
	CODEC_ID_PCM_U32LE,
	CODEC_ID_PCM_U32BE,
	CODEC_ID_PCM_S24LE,
	CODEC_ID_PCM_S24BE,
	CODEC_ID_PCM_U24LE,
	CODEC_ID_PCM_U24BE,
	CODEC_ID_PCM_S24DAUD,
		
	/* various adpcm codecs */
	CODEC_ID_ADPCM_IMA_QT= 0x11000,
	CODEC_ID_ADPCM_IMA_WAV,
	CODEC_ID_ADPCM_IMA_DK3,
	CODEC_ID_ADPCM_IMA_DK4,
	CODEC_ID_ADPCM_IMA_WS,
	CODEC_ID_ADPCM_IMA_SMJPEG,
	CODEC_ID_ADPCM_MS,
	CODEC_ID_ADPCM_4XM,
	CODEC_ID_ADPCM_XA,
	CODEC_ID_ADPCM_ADX,
	CODEC_ID_ADPCM_EA,
	CODEC_ID_ADPCM_G726,
	CODEC_ID_ADPCM_CT,
	CODEC_ID_ADPCM_SWF,
	CODEC_ID_ADPCM_YAMAHA,
	CODEC_ID_ADPCM_SBPRO_4,
	CODEC_ID_ADPCM_SBPRO_3,
	CODEC_ID_ADPCM_SBPRO_2,
		
	/* AMR */
	CODEC_ID_AMR_NB= 0x12000,
	CODEC_ID_AMR_WB,
		
	/* RealAudio codecs*/
	CODEC_ID_RA_144= 0x13000,
	CODEC_ID_RA_288,
		
	/* various DPCM codecs */
	CODEC_ID_ROQ_DPCM= 0x14000,
	CODEC_ID_INTERPLAY_DPCM,
	CODEC_ID_XAN_DPCM,
	CODEC_ID_SOL_DPCM,
		
	CODEC_ID_MP2= 0x15000,
	CODEC_ID_MP3, /* prefered ID for MPEG Audio layer 1, 2 or3 decoding */
	CODEC_ID_AAC,
#if LIBAVCODEC_VERSION_INT < ((52<<16)+(0<<8)+0)
	CODEC_ID_MPEG4AAC,
#endif
	CODEC_ID_AC3,
	CODEC_ID_DTS,
	CODEC_ID_VORBIS,
	CODEC_ID_DVAUDIO,
	CODEC_ID_WMAV1,
	CODEC_ID_WMAV2,
	CODEC_ID_MACE3,
	CODEC_ID_MACE6,
	CODEC_ID_VMDAUDIO,
	CODEC_ID_SONIC,
	CODEC_ID_SONIC_LS,
	CODEC_ID_FLAC,
	CODEC_ID_MP3ADU,
	CODEC_ID_MP3ON4,
	CODEC_ID_SHORTEN,
	CODEC_ID_ALAC,
	CODEC_ID_WESTWOOD_SND1,
	CODEC_ID_GSM,
	CODEC_ID_QDM2,
	CODEC_ID_COOK,
	CODEC_ID_TRUESPEECH,
	CODEC_ID_TTA,
	CODEC_ID_SMACKAUDIO,
	CODEC_ID_QCELP,
	CODEC_ID_WAVPACK,
	CODEC_ID_DSICINAUDIO,
	CODEC_ID_IMC,
	CODEC_ID_MUSEPACK7,
		
	/* subtitle codecs */
	CODEC_ID_DVD_SUBTITLE= 0x17000,
	CODEC_ID_DVB_SUBTITLE,
		
	CODEC_ID_MPEG2TS= 0x20000, /* _FAKE_ codec to indicate a raw MPEG2 transport
	stream (only used by libavformat) */
};

typedef struct ParseContext{
	uint8_t *buffer;
	int index;
	int last_index;
	unsigned int buffer_size;
	uint32_t state;             ///< contains the last few bytes in MSB order
	int frame_start_found;
	int overread;               ///< the number of bytes which where irreversibly read from the next frame
	int overread_index;         ///< the index into ParseContext.buffer of the overreaded bytes
} ParseContext;

typedef struct Picture{
	int64_t pts;
	int key_frame;
	int reference;
	int qscale_type;
#define FF_QSCALE_TYPE_MPEG1 0
#define FF_QSCALE_TYPE_MPEG2 1
#define FF_QSCALE_TYPE_H264  2
	int pict_type;
	int frame_num;              ///< h264 frame_num
	int field_poc[2];           ///< h264 top/bottom POC
} Picture;

typedef short DCTELEM;
typedef struct MpegEncContext {
	struct AVCodecContext *avctx;
	int width, height;///< picture size. must be a multiple of 16
	Picture current_picture;    ///< buffer to store the decompressed current picture
	Picture *current_picture_ptr;  ///< pointer to the current picture
	int pict_type;              ///< I_TYPE, P_TYPE, B_TYPE, ...
	int codec_tag;             ///< internal codec_tag upper case converted from avctx codec_tag
	int divx_version;
	int xvid_build;
	int divx_build;
	int divx_packed;
	int lavc_build;
	int low_delay;                   ///< no reordering needed / has no b-frames
	int vol_control_parameters;      ///< does the stream contain the low_delay flag, used to workaround buggy encoders
	int flags;        ///< AVCodecContext.flags (HQ, MV4, ...)
	int flags2;
	int partitioned_frame;           ///< is current frame partitioned
	int data_partitioning;           ///< data partitioning flag from header
	int time_increment_bits;        ///< number of bits to represent the fractional part of time
	int last_time_base;
	int time_base;                  ///< time in seconds of last I,P,S Frame
	int64_t time;                   ///< time of current frame
	int workaround_bugs;       ///< workaround bugs in encoders which cannot be detected automatically
	int64_t last_non_b_time;
	uint16_t pp_time;               ///< time distance between the last 2 p,s,i frames
	uint16_t pb_time;               ///< time distance between the last b and p,s,i frame
	uint16_t pp_field_time;
	uint16_t pb_field_time;         ///< like above, just for interlaced
	int t_frame;                       ///< time distance of first I -> B, used for interlaced b frames
	int progressive_sequence;
	int shape;
	int vol_sprite_usage;
	int no_rounding;  /**< apply no rounding to motion compensation (MPEG4, msmpeg4, ...) for b-frames rounding mode is allways 0 */
	int intra_dc_threshold;          ///< QP above whch the ac VLC should be used for intra dc
	int top_field_first;
	int alternate_scan;
	int sprite_brightness_change;
	int qscale;                 ///< QP
	int chroma_qscale;          ///< chroma QP
	int quant_precision;
	int f_code;                 ///< forward MV resolution
	int b_code;                 ///< backward MV resolution for B Frames (mpeg4)
	int quarter_sample;              ///< 1->qpel, 0->half pel ME/MC
	int resync_marker;               ///< could this stream contain resync markers
	int num_sprite_warping_points;
	int sprite_warping_accuracy;
	int vo_type;
	int scalability;
	int enhancement_type;
	int picture_number;       //FIXME remove, unclear definition
	int h_edge_pos, v_edge_pos;///< horizontal / vertical position of the right/bottom edge (pixel replication)
	int aspect_ratio_info; //FIXME remove
	int progressive_frame;
	int interlaced_dct;
	int sprite_width;
	int sprite_height;
	int sprite_left;
	int sprite_top;
	int low_latency_sprite;
	int mpeg_quant;
	int rvlc;                        ///< reversible vlc
	int new_pred;
	int reduced_res_vop;
	int hierachy_type;
	int dropable;
	int mb_width, mb_height;   ///< number of MBs horizontally & vertically
	int picture_structure;
	/* picture type */
#define PICT_TOP_FIELD     1
#define PICT_BOTTOM_FIELD  2
#define PICT_FRAME         3
	int mb_num;                ///< number of MBs of a picture
	int mb_x, mb_y;
	int resync_mb_x;                 ///< x position of last resync marker
	int resync_mb_y;                 ///< y position of last resync marker
	int context_initialized;
	int hurry_up;     /**< when set to 1 during decoding, b frames will be skipped when set to 2 idct/dequant will be skipped too */
	ParseContext parse_context;
	GetBitContext gb;
} MpegEncContext;

int MPV_common_init (MpegEncContext *s);
void MPV_common_end (MpegEncContext *s);

static const uint8_t mpeg4_dc_threshold[8]={
	99, 13, 15, 17, 19, 21, 23, 0
};

enum AVDiscard{
//we leave some space between them for extensions (drop some keyframes for intra only or drop just some bidir frames)
	AVDISCARD_NONE   =-16, ///< discard nothing
 AVDISCARD_DEFAULT=  0, ///< discard useless packets like 0 size packets in avi
 AVDISCARD_NONREF =  8, ///< discard all non reference
 AVDISCARD_BIDIR  = 16, ///< discard all bidirectional frames
 AVDISCARD_NONKEY = 32, ///< discard all frames except keyframes
 AVDISCARD_ALL    = 48, ///< discard all
};

#define MAX_PICTURE_COUNT 32

#define CODEC_FLAG_GLOBAL_HEADER  0x00400000 ///< place global headers in extradata instead of every keyframe
#define CODEC_FLAG_LOW_DELAY      0x00080000 ///< force low delay
#define CODEC_FLAG_EMU_EDGE 0x4000///< don't draw edges
#define CODEC_FLAG2_LOCAL_HEADER  0x00000008 ///< place global headers at every keyframe instead of in extradata
#define CODEC_FLAG2_FAST          0x00000001 ///< allow non spec compliant speedup tricks

#define FF_ASPECT_EXTENDED 15

#define FF_BUG_AUTODETECT       1  ///< autodetection
#define FF_BUG_OLD_MSMPEG4      2
#define FF_BUG_XVID_ILACE       4
#define FF_BUG_UMP4             8
#define FF_BUG_NO_PADDING       16
#define FF_BUG_AMV              32
#define FF_BUG_AC_VLC           0  ///< will be removed, libavcodec can now handle these non compliant files by default
#define FF_BUG_QPEL_CHROMA      64
#define FF_BUG_STD_QPEL         128
#define FF_BUG_QPEL_CHROMA2     256
#define FF_BUG_DIRECT_BLOCKSIZE 512
#define FF_BUG_EDGE             1024
#define FF_BUG_HPEL_CHROMA      2048
#define FF_BUG_DC_CLIP          4096
#define FF_BUG_MS               8192 ///< workaround various bugs in microsofts broken decoders

#define RECT_SHAPE       0
#define BIN_SHAPE        1
#define BIN_ONLY_SHAPE   2
#define GRAY_SHAPE       3

#define STATIC_SPRITE 1
#define GMC_SPRITE 2

typedef struct AVCodecContext {
	int bit_rate;
	int flags;
	enum CodecID codec_id;
	int sub_id;
	int flags2;
	int codec_tag;
	uint8_t *extradata;
	int extradata_size;
	int coded_width, coded_height;
	int lowres;
	int width;
	int height;
	AVRational time_base;
	AVRational sample_aspect_ratio;
	int aspect_ratio_idx;
	int debug;
#define FF_DEBUG_PICT_INFO 1
#define FF_DEBUG_RC        2
#define FF_DEBUG_BITSTREAM 4
#define FF_DEBUG_MB_TYPE   8
#define FF_DEBUG_QP        16
#define FF_DEBUG_MV        32
#define FF_DEBUG_DCT_COEFF 0x00000040
#define FF_DEBUG_SKIP      0x00000080
#define FF_DEBUG_STARTCODE 0x00000100
#define FF_DEBUG_PTS       0x00000200
#define FF_DEBUG_ER        0x00000400
#define FF_DEBUG_MMCO      0x00000800
#define FF_DEBUG_BUGS      0x00001000
#define FF_DEBUG_VIS_QP    0x00002000
#define FF_DEBUG_VIS_MB_TYPE 0x00004000
	int has_b_frames;
	enum AVDiscard skip_frame;
	enum AVDiscard skip_loop_filter;
} AVCodecContext;

typedef struct AVCodecParserContext {
	void *priv_data;
	struct AVCodecParser *parser;
	int64_t frame_offset; /* offset of the current frame */
	int64_t cur_offset; /* current offset (incremented by each av_parser_parse()) */
	int64_t last_frame_offset; /* offset of the last frame */
	/* video info */
	int pict_type; /* XXX: put it back in AVCodecContext */
	int pict_structure;
	int repeat_pict; /* XXX: put it back in AVCodecContext */
	int64_t pts;     /* pts of the current frame */
	int64_t dts;     /* dts of the current frame */
	/* private data */
	int64_t last_pts;
	int64_t last_dts;
	int fetch_timestamp;
//#define AV_PARSER_PTS_NB 4
#define AV_PARSER_PTS_NB 16
	int cur_frame_start_index;
	int64_t cur_frame_offset[AV_PARSER_PTS_NB];
	int64_t cur_frame_pts[AV_PARSER_PTS_NB];
	int64_t cur_frame_dts[AV_PARSER_PTS_NB];
	int flags;
#define PARSER_FLAG_COMPLETE_FRAMES           0x0001
	int64_t curr_pts;     /* pts of the current frame */
	int64_t curr_dts;     /* dts of the current frame */
	int delay_pts;
} AVCodecParserContext;

typedef struct AVCodecParser {
	int codec_ids[5]; /* several codec IDs are permitted */
	int priv_data_size;
	int (*parser_init)(AVCodecParserContext *s);
	int (*parser_parse)(AVCodecParserContext *s,
	AVCodecContext *avctx,
	uint8_t **poutbuf, int *poutbuf_size,
	const uint8_t *buf, int buf_size);
	void (*parser_close)(AVCodecParserContext *s);
	int (*split)(AVCodecContext *avctx, const uint8_t *buf, int buf_size);
	struct AVCodecParser *next;
} AVCodecParser;

void hde_av_register_codec_parser(AVCodecParser *parser);
AVCodecParserContext *hde_av_parser_init(int codec_id);
int hde_av_parser_parse(AVCodecParserContext *s,
					AVCodecContext *avctx,
					uint8_t **poutbuf, int *poutbuf_size,
					const uint8_t *buf, int buf_size,
					int64_t pts, int64_t dts);
void hde_av_parser_close(AVCodecParserContext *s);

#define FRAME_SKIPPED 100 ///< return value for header parsers if frame is not coded
#define VOS_STARTCODE        0x1B0
#define USER_DATA_STARTCODE  0x1B2
#define GOP_STARTCODE        0x1B3
#define VISUAL_OBJ_STARTCODE 0x1B5
#define VOP_STARTCODE        0x1B6

#define IS_3IV1 0

//int ff_mpeg4_decode_picture_header(MpegEncContext * s, GetBitContext *gb);
void myavcodec_set_dimensions(AVCodecContext *s, int width, int height);
int myavcodec_check_dimensions(AVCodecContext *s, unsigned int width, unsigned int height);
#ifdef CONFIG_MPEGVIDEO_PARSER
extern AVCodecParser hde_mpegvideo_parser;
#else
#error CONFIG_MPEGVIDEO_PARSER not defined!
#endif
#ifdef CONFIG_MPEG4VIDEO_PARSER
extern AVCodecParser hde_mpeg4video_parser;
#else
#error CONFIG_MPEG4VIDEO_PARSER not defined!
#endif
#ifdef CONFIG_H264_PARSER
AVCodecParser hde_h264_parser;
#else
#error CONFIG_H264_PARSER not defined!
#endif

//*****************************************************************************

#define BUFFER_PADDING 8

#define SEQ_END_CODE            0x000001b7
#define SEQ_START_CODE          0x000001b3
#define GOP_START_CODE          0x000001b8
#define PICTURE_START_CODE      0x00000100
#define SLICE_MIN_START_CODE    0x00000101
#define SLICE_MAX_START_CODE    0x000001af
#define EXT_START_CODE          0x000001b5
#define USER_START_CODE         0x000001b2

#define I_FRAME 1
#define P_FRAME 2
#define B_FRAME 3
#define D_FRAME 4
#define S_FRAME 4
#define SI_FRAME 5
#define SP_FRAME 6

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFABS(a) ((a) >= 0 ? (a) : (-a))
static const AVRational ff_frame_rate_tab[] = {
    {0, 0},
    {24000, 1001},
    {24, 1},
    {25, 1},
    {30000, 1001},
    {30, 1},
    {50, 1},
    {60000, 1001},
    {60, 1},
    {15, 1},
    {5, 1},
    {10, 1},
    {12, 1},
    {15, 1},
    {0, 0},
    {0, 0}};

static const AVRational ff_aspect_ratio_tab[] = {
    {0, 0},
    {1, 1},
    {6735, 10000},
    {7031, 10000},
    {7615, 10000},
    {8055, 10000},
    {8437, 10000},
    {8935, 10000},
    {9375, 10000},
    {9815, 10000},
    {10255, 10000},
    {10695, 10000},
    {11250, 10000},
    {11575, 10000},
    {12015, 10000},
    {0, 0}};

static const int mpa_bitrate2_tab[] = {-1, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, -1};
static const int mpa_bitrate3_tab[] = {-1, 32, 40, 48, 56, 64, 80,  96, 112, 128, 160, 192, 224, 256, 320, -1};
static const int mpa_bitrate_tab[2][3][15] = {
	{{0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
	 {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384 },
	 {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 }},
	{{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
	 {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
	 {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}}
};
static const int mpa_samplerate_tab[] = {44100, 48000, 32000, -1};
		
static const int aac_samplerate_tab[16] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0};
static const int aac_channels[8] = {0, 1, 2, 3, 4, 5, 6, 8};

static const int ac3_bitrate_tab[] = { 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640};
static const int ac3_channels[8] = { 2, 1, 2, 3, 3, 4, 4, 5 };
static const unsigned char ac3_halfrate[12] ={ 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3 };
static const unsigned char ac3_lfeon[8] ={ 0x10, 0x10, 0x04, 0x04, 0x04, 0x01, 0x04, 0x01 };

static const int dts_bitrate_tab[] = { 32000, 56000, 64000, 96000, 112000, 128000, 192000, 224000, 256000, 320000, 384000,
                                       448000, 512000, 576000, 640000, 768000, 896000, 1024000, 1152000, 1280000, 1344000,
                                       1408000, 1411200, 1472000, 1536000, 1920000, 2048000, 3072000, 3840000, 1/*open*/, 2/*variable*/, 3/*lossless*/};
static const int dts_samplerate_tab[] = {0, 8000, 16000, 32000, 0, 0, 11025, 22050, 44100, 0, 0, 12000, 24000, 48000, 96000, 192000};
static const uint8_t dts_channel_tab[] = { 1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6, 6, 7, 8, 8};

#define MPA_HEADER_SIZE 4
#define AC3_HEADER_SIZE 7
#define DTS_HEADER_SIZE 20
#define AAC_HEADER_SIZE 9

#define A52_DOLBY 10
#define A52_LFE 16

#define MAX_DTS_FRAME    8191
#define DTS_CHANNEL_MASK 0xF
        
typedef struct hde_frame_parser_s {
    int frame_start_found;
    int state;
} hde_frame_parser_t;

typedef struct hde_frame_buffer_s {
    uint8_t *data;
    int pos;
    unsigned int size;
} hde_frame_buffer_t;

static inline int64_t GetCurrentTime(void) {
    struct timeval now;
    gettimeofday ( &now, NULL );
    return now.tv_sec * 1000000LL + now.tv_usec;
} // GetCurrentTime

void *av_realloc(void *ptr, unsigned int size);
void *av_fast_realloc(void *ptr, unsigned int *size, unsigned int min_size);
const uint8_t *hde_ff_find_start_code ( const uint8_t * p, const uint8_t *end, uint32_t * state );
int mpeg_find_frame_end ( hde_frame_parser_t *this, const uint8_t *buf, int buf_size );

int parse_mpa(uint8_t *data, int32_t size, int64_t pts);

void reduce_factors(uint32_t *a, uint32_t *b);
void dump_memory ( unsigned char *d, int len );

void *av_malloc(unsigned int size);
void *av_mallocz(unsigned int size);
void av_free(void *ptr);

int av_reduce(int *dst_nom, int *dst_den, int64_t num, int64_t den, int64_t max);

extern const uint8_t ff_log2_tab[256];
static inline int av_log2(unsigned int v)
{
	int n;

	n = 0;
	if (v & 0xffff0000) {
		v >>= 16;
		n += 16;
	}
	if (v & 0xff00) {
		v >>= 8;
		n += 8;
	}
	n += ff_log2_tab[v];

	return n;
}

#endif // HAVE_HDE_TOOL_H
