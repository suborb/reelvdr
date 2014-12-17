/****************************************************************************
 *                       Copyright Reel Multimedia 2008                     *
 *                            All Rights Reserved                           *
 ****************************************************************************/

#ifndef trNOOP
#define trNOOP(x) x
#endif

//                xx API Version
#define RBM_API 0x01
#define RBM_API_VERSION(x) ((x>>24)&0xFF)
#define RBM_API_BUILD(x)   (x&0xFFFFFF)

#define FBIO_WAITFORVSYNC         0x4621
#define FBIO_STARTDISPLAY         0x4622
#define FBIO_STOPDISPLAY          0x4623
#define FBIO_CHANGEOPACITY        0x4624
#define FBIO_CHANGEOUTPUTFORMAT   0x4625
#define FBIO_GETFBRESOLUTION      0x4626

typedef struct _cnxt_rbm_resolution{
	u_int32 uWidth;
	u_int32 uHeight;
}cnxt_rbm_resolution;

#define RBMIO_SET_DMX_MODE      0x5000
#define RBMIO_GET_DMX_MODE      0x5001
#define RBMIO_SET_AV_MODE       0x5002
#define RBMIO_GET_AV_MODE       0x5003
#define RBMIO_SET_TV_MODE       0x5004
#define RBMIO_GET_TV_MODE       0x5005
#define RBMIO_FLUSH             0x5010
#define RBMIO_STOP              0x5011
#define RBMIO_TRICK             0x5012
#define RBMIO_PAUSE             0x5013
#define RBMIO_RESUME            0x5014
#define RBMIO_STILL             0x5015
#define RBMIO_GET_STATE         0x5020
#define RBMIO_GET_AFC           0x5021
#define RBMIO_SET_AUDIO_CHANNEL 0x5022
#define RBMIO_GET_AUDIO_CHANNEL 0x5023
#define RBMIO_SET_AUDIO_VOLUME  0x5024
#define RBMIO_GET_AUDIO_VOLUME  0x5025
#define RBMIO_GET_STC           0x5026
#define RBMIO_GET_VERSION       0x5030
#define RBMIO_SET_DMX_PIDS      0x5100

typedef	u_int32 rbm_av_version;

#define RBMIO_MAX_PIDS 32
typedef struct _rbm_dmx_pids{
	u_int32 pid_count;
	u_int32 pid[RBMIO_MAX_PIDS];
}rbm_dmx_pids;

#define RBMIO_FLUSH_CLEAR   0
#define RBMIO_FLUSH_WAIT    1
typedef struct _rbm_av_flush {
	u_int32 mode;
	int32   timeout; // ms - only used if RBMIO_FLUSH_WAIT 
}rbm_av_flush;

#define RBMIO_TRICK_I_FRAME (1L<<31)
typedef	u_int32 rbm_av_trick;

typedef struct _rbm_av_still {
	u_int32 uSize;
	char *pData;
} rbm_av_still;

#define RBM_DMX_MODE_NONE        0
#define RBM_DMX_MODE_TS          1
#define RBM_DMX_MODE_PES         2
#define RBM_DMX_MODE_COUNT       3
#define RBM_DMX_MODE(x) ( \
	(x==RBM_DMX_MODE_NONE)? trNOOP("None") : \
	(x==RBM_DMX_MODE_TS  )? trNOOP("TS")   : \
	(x==RBM_DMX_MODE_PES )? trNOOP("PES")  : \
	trNOOP("Unknown") )
typedef	u_int32 rbm_dmx_mode;

#define RBM_STATE_NONE           0
#define RBM_STATE_INIT           1
#define RBM_STATE_RUN            2
#define RBM_STATE_PAUSE          3
#define RBM_STATE_TRICK          4
#define RBM_STATE(x) ( \
	(x==RBM_STATE_NONE )? trNOOP("None")  : \
	(x==RBM_STATE_INIT )? trNOOP("Init")  : \
	(x==RBM_STATE_RUN  )? trNOOP("Play")  : \
	(x==RBM_STATE_PAUSE)? trNOOP("Pause") : \
	(x==RBM_STATE_TRICK)? trNOOP("Trick") : \
	trNOOP("Unknown") )
typedef	u_int32 rbm_state;

typedef u_int32 rbm_afc;

typedef struct _rbm_stc{
	u_int32 high;
	u_int32 low;
}rbm_stc;

#define RBM_AUDIO_CHANNEL_STEREO 0
#define RBM_AUDIO_CHANNEL_LEFT   1
#define RBM_AUDIO_CHANNEL_RIGHT  2
#define RBM_AUDIO_CHANNEL_COUNT  3
#define RBM_AUDIO_CHANNEL(x) ( \
	(x==RBM_AUDIO_CHANNEL_STEREO)? trNOOP("Stereo") : \
	(x==RBM_AUDIO_CHANNEL_LEFT  )? trNOOP("Left")   : \
	(x==RBM_AUDIO_CHANNEL_RIGHT )? trNOOP("Right")  : \
	trNOOP("Unknown") )
typedef	u_int32 rbm_audio_channel;
#define RBM_AUDIO_VOLUME_MIN (-120000)
typedef	int32 rbm_audio_volume;

#define RBM_VIDEO_FORMAT_NONE    0
#define RBM_VIDEO_FORMAT_MPEG2   1
#define RBM_VIDEO_FORMAT_H264    2
#define RBM_VIDEO_FORMAT_JPEG    3
#define RBM_VIDEO_FORMAT_COUNT   6
#define RBM_VIDEO_FORMAT(x) ( \
	(x==RBM_VIDEO_FORMAT_NONE )? trNOOP("None") : \
	(x==RBM_VIDEO_FORMAT_MPEG2)? trNOOP("Mpg2") : \
	(x==RBM_VIDEO_FORMAT_H264 )? trNOOP("h264") : \
	(x==RBM_VIDEO_FORMAT_JPEG )? trNOOP("jpeg") : \
	trNOOP("Unknown") )

#define RBM_AUDIO_FORMAT_NONE    0
#define RBM_AUDIO_FORMAT_MPA     1
#define RBM_AUDIO_FORMAT_AC3     2
#define RBM_AUDIO_FORMAT_MP3     3
#define RBM_AUDIO_FORMAT_COUNT   4
#define RBM_AUDIO_FORMAT(x) ( \
	(x==RBM_AUDIO_FORMAT_NONE)? trNOOP("None") : \
	(x==RBM_AUDIO_FORMAT_MPA )? "Mpa"  : \
	(x==RBM_AUDIO_FORMAT_AC3 )? "Dolby D"  : \
	(x==RBM_AUDIO_FORMAT_MP3 )? "Mp3"  : \
	trNOOP("Unknown") )

typedef struct _rbm_av_mode {
	u_int32 uVideoFormat;
	u_int32 uStillFormat;
	u_int32 uAudioFormat;
	u_int32 uPCRPid;
	u_int32 uVideoPid;
	u_int32 uAudioPid;
	u_int32 dummy1;
	u_int32 dummy2;
	u_int32 dummy3;
} rbm_av_mode;

#define RBM_OUTPUT_MODE_NONE        0
#define RBM_OUTPUT_MODE_NTSC_M      1
#define RBM_OUTPUT_MODE_NTSC_JAPAN  2
#define RBM_OUTPUT_MODE_NTSC_443    3
#define RBM_OUTPUT_MODE_PAL_B_ITALY 4
#define RBM_OUTPUT_MODE_PAL_B_WEUR  5
#define RBM_OUTPUT_MODE_PAL_B_AUS   6
#define RBM_OUTPUT_MODE_PAL_B_NZ    7
#define RBM_OUTPUT_MODE_PAL_I       8
#define RBM_OUTPUT_MODE_SECAM_L     9
#define RBM_OUTPUT_MODE_SECAM_D     10
#define RBM_OUTPUT_MODE_PAL_D       11
#define RBM_OUTPUT_MODE_PAL_G       12
#define RBM_OUTPUT_MODE_PAL_H       13
#define RBM_OUTPUT_MODE_PAL_M       14
#define RBM_OUTPUT_MODE_PAL_60      15
#define RBM_OUTPUT_MODE_PAL_N       16
#define RBM_OUTPUT_MODE_PAL_NC      17
#define RBM_OUTPUT_MODE_NTSC_BT470  18
#define RBM_OUTPUT_MODE_480P        19
#define RBM_OUTPUT_MODE_576P        20
#define RBM_OUTPUT_MODE_720P_50     21
#define RBM_OUTPUT_MODE_720P_59     22
#define RBM_OUTPUT_MODE_720P_60     23
#define RBM_OUTPUT_MODE_1080I_50    24
#define RBM_OUTPUT_MODE_1080I_59    25
#define RBM_OUTPUT_MODE_1080I_60    26
#define RBM_OUTPUT_MODE_1080P_23    27
#define RBM_OUTPUT_MODE_1080P_24    28
#define RBM_OUTPUT_MODE_1080P_25    29
#define RBM_OUTPUT_MODE_1080P_29    30
#define RBM_OUTPUT_MODE_1080P_30    31
#define RBM_OUTPUT_MODE_SOURCE      32
#define RBM_OUTPUT_MODE_COUNT       33
#define RBM_OUTPUT_MODE(x) ( \
	(x==RBM_OUTPUT_MODE_NONE        )? trNOOP("None")       : \
	(x==RBM_OUTPUT_MODE_NTSC_M      )? trNOOP("NTSC")       : \
	(x==RBM_OUTPUT_MODE_NTSC_JAPAN  )? trNOOP("Ntsc Jap")   : \
	(x==RBM_OUTPUT_MODE_NTSC_443    )? trNOOP("Ntsc 443")   : \
	(x==RBM_OUTPUT_MODE_PAL_B_ITALY )? trNOOP("Pal B IT")   : \
	(x==RBM_OUTPUT_MODE_PAL_B_WEUR  )? trNOOP("PAL")        : \
	(x==RBM_OUTPUT_MODE_PAL_B_AUS   )? trNOOP("Pal B Aus")  : \
	(x==RBM_OUTPUT_MODE_PAL_B_NZ    )? trNOOP("Pal B NZ")   : \
	(x==RBM_OUTPUT_MODE_PAL_I       )? trNOOP("Pal I")      : \
	(x==RBM_OUTPUT_MODE_SECAM_L     )? trNOOP("SECAM")      : \
	(x==RBM_OUTPUT_MODE_SECAM_D     )? trNOOP("Secam D")    : \
	(x==RBM_OUTPUT_MODE_PAL_D       )? trNOOP("Pal D")      : \
	(x==RBM_OUTPUT_MODE_PAL_G       )? trNOOP("Pal G")      : \
	(x==RBM_OUTPUT_MODE_PAL_H       )? trNOOP("Pal H")      : \
	(x==RBM_OUTPUT_MODE_PAL_M       )? trNOOP("Pal M")      : \
	(x==RBM_OUTPUT_MODE_PAL_60      )? trNOOP("Pal 60")     : \
	(x==RBM_OUTPUT_MODE_PAL_N       )? trNOOP("Pal N")      : \
	(x==RBM_OUTPUT_MODE_PAL_NC      )? trNOOP("Pal NC")     : \
	(x==RBM_OUTPUT_MODE_NTSC_BT470  )? trNOOP("Ntsc BT470") : \
	(x==RBM_OUTPUT_MODE_480P        )? trNOOP("480p60")     : \
	(x==RBM_OUTPUT_MODE_576P        )? trNOOP("576p50")     : \
	(x==RBM_OUTPUT_MODE_720P_50     )? trNOOP("720p50")     : \
	(x==RBM_OUTPUT_MODE_720P_59     )? trNOOP("720p59")     : \
	(x==RBM_OUTPUT_MODE_720P_60     )? trNOOP("720p60")     : \
	(x==RBM_OUTPUT_MODE_1080I_50    )? trNOOP("1080i50")    : \
	(x==RBM_OUTPUT_MODE_1080I_59    )? trNOOP("1080i59")    : \
	(x==RBM_OUTPUT_MODE_1080I_60    )? trNOOP("1080i60")    : \
	(x==RBM_OUTPUT_MODE_1080P_23    )? trNOOP("1080p23")    : \
	(x==RBM_OUTPUT_MODE_1080P_24    )? trNOOP("1080p24")    : \
	(x==RBM_OUTPUT_MODE_1080P_25    )? trNOOP("1080p25")    : \
	(x==RBM_OUTPUT_MODE_1080P_29    )? trNOOP("1080p29")    : \
	(x==RBM_OUTPUT_MODE_1080P_30    )? trNOOP("1080p30")    : \
	(x==RBM_OUTPUT_MODE_SOURCE      )? trNOOP("Source")     : \
	trNOOP("Unknown") )

#define RBM_ASPECT_NONE   0
#define RBM_ASPECT_1_1    1
#define RBM_ASPECT_4_3    2
#define RBM_ASPECT_14_9   3
#define RBM_ASPECT_16_9   4
#define RBM_ASPECT_20_9   5
#define RBM_ASPECT_SOURCE 6
#define RBM_ASPECT_COUNT  7
#define RBM_ASPECT(x) ( \
	(x==RBM_ASPECT_NONE  )? trNOOP("None") : \
	(x==RBM_ASPECT_1_1   )? trNOOP("1:1")  : \
	(x==RBM_ASPECT_4_3   )? trNOOP("4:3")  : \
	(x==RBM_ASPECT_14_9  )? trNOOP("14:9") : \
	(x==RBM_ASPECT_16_9  )? trNOOP("16:9") : \
	(x==RBM_ASPECT_20_9  )? trNOOP("20:9") : \
	(x==RBM_ASPECT_SOURCE)? trNOOP("Source") : \
	trNOOP("Unknown") )

#define RBM_ARMODE_NONE       0
#define RBM_ARMODE_PANSCAN    1
#define RBM_ARMODE_LETTERBOX  2
#define RBM_ARMODE_NL_X       3
#define RBM_ARMODE_NL_Y       4
#define RBM_ARMODE_NL_XY      5
#define RBM_ARMODE_COUNT      6
#define RBM_ARMODE(x) ( \
	(x==RBM_ARMODE_NONE       )? trNOOP("Fill to Screen") : \
	(x==RBM_ARMODE_PANSCAN    )? trNOOP("Crop to Fill")   : \
	(x==RBM_ARMODE_LETTERBOX  )? trNOOP("Fill to Aspect") : \
	(x==RBM_ARMODE_NL_X       )? trNOOP("NL X")      : \
	(x==RBM_ARMODE_NL_Y       )? trNOOP("NL Y")      : \
	(x==RBM_ARMODE_NL_XY      )? trNOOP("NL XY")     : \
	trNOOP("Unknown") )

#define RBM_HDMIMODE_NONE    0
#define RBM_HDMIMODE_DVI     1
#define RBM_HDMIMODE_HDMI    2
#define RBM_HDMIMODE_PCM     3
#define RBM_HDMIMODE_COUNT   4
#define RBM_HDMIMODE(x) ( \
	(x==RBM_HDMIMODE_NONE)? trNOOP("None") : \
	(x==RBM_HDMIMODE_DVI )? trNOOP("DVI")  : \
	(x==RBM_HDMIMODE_HDMI)? trNOOP("HDMI") : \
	(x==RBM_HDMIMODE_PCM )? trNOOP("HDMI (PCM)") : \
	trNOOP("Unknown") )

#define RBM_SCARTMODE_NONE     0
#define RBM_SCARTMODE_ALL      1
#define RBM_SCARTMODE_CVBS     2
#define RBM_SCARTMODE_CVBS_DLY 3
#define RBM_SCARTMODE_RGB      4
#define RBM_SCARTMODE_YC       5
#define RBM_SCARTMODE_YPRPB    6
#define RBM_SCARTMODE_COUNT    7
#define RBM_SCARTMODE(x) ( \
	(x==RBM_SCARTMODE_NONE    )? trNOOP("None")     : \
	(x==RBM_SCARTMODE_ALL     )? trNOOP("All")      : \
	(x==RBM_SCARTMODE_CVBS    )? trNOOP("CVBS")     : \
	(x==RBM_SCARTMODE_CVBS_DLY)? trNOOP("CVBS DLY") : \
	(x==RBM_SCARTMODE_RGB     )? trNOOP("RGB")      : \
	(x==RBM_SCARTMODE_YC      )? trNOOP("YC")       : \
	(x==RBM_SCARTMODE_YPRPB   )? trNOOP("YPrPb")    : \
	trNOOP("Unknown") )

#define RBM_MPEG2_MODE_NONE  0
#define RBM_MPEG2_MODE_DB    1
#define RBM_MPEG2_MODE_DB_DR 2
#define RBM_MPEG2_MODE_COUNT 3
#define RBM_MPEG2_MODE(x) ( \
	(x==RBM_MPEG2_MODE_NONE  )? trNOOP("None")             : \
	(x==RBM_MPEG2_MODE_DB    )? trNOOP("Deblock")          : \
	(x==RBM_MPEG2_MODE_DB_DR )? trNOOP("Deblock & dering") : \
	trNOOP("Unknown") )

typedef struct _rbm_tv_mode {
	u_int32 uHDOutputMode;
	u_int32 uHDAspect;
	u_int32 uHDARMode;
	u_int32 uSDOutputMode;
	u_int32 uSDAspect;
	u_int32 uSDARMode;
	u_int32 uHDMIMode;
	u_int32 uSCARTMode;
	u_int32 uMPEG2Mode;
	int32   iMPAOffset;
	int32   iAC3Offset;
} rbm_tv_mode;

#define RBM_AV_HEAD_MARKER 0x87654321
typedef struct _rbm_av_head {
	u_int32 uMarker;
	u_int32 uStreamID;
	u_int64 uPts;
} rbm_av_head;
