/**************************************************************************
*   Copyright (C) 2011 by Reel Multimedia                                 *
*                                                                         *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
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

#ifndef ICE_DEBUG_H_INCLUDED
#define ICE_DEBUG_H_INCLUDED

#define DBG_NONE   0
#define DBG_STDOUT 1
#define DBG_SYSLOG 2

#define ICE_DBG(level, text...) {  \
    if(DBG_STDOUT==level){         \
        printf(text);              \
        printf("\n");              \
    }else if(DBG_SYSLOG==level){   \
        printf("SYSLOG: ");        \
        printf(text);              \
        printf("\n");              \
        isyslog(text);             \
    }                              \
}

#define PLAY_MODE_STR(x) (                                          \
    (pmNone                         ==x)?trNOOP("None")           : \
    (pmAudioVideo                   ==x)?trNOOP("AudioVideo")     : \
    (pmAudioOnly                    ==x)?trNOOP("AudioOnly")      : \
    (pmAudioOnlyBlack               ==x)?trNOOP("AudioOnlyBlack") : \
    (pmVideoOnly                    ==x)?trNOOP("VideoOnly")      : \
    (pmExtern_THIS_SHOULD_BE_AVOIDED==x)?trNOOP("Extern")         : \
    trNOOP(""))

#define VIDEO_DISPLAY_FORMAT_STR(x) (             \
    (vdfPanAndScan  ==x)?trNOOP("PanAndScan")   : \
    (vdfLetterBox   ==x)?trNOOP("LetterBox")    : \
    (vdfCenterCutOut==x)?trNOOP("CenterCutOut") : \
    trNOOP(""))

#define DFBCHECK(x...) {                                                     \
    DFBResult err = x;                                                       \
    if (err != DFB_OK) {                                                     \
        if(DBG_DFB==DBG_STDOUT)                                              \
            printf( "DFB %s <%d>: %d=" #x "\n", __FILE__, __LINE__, err );   \
        else if(DBG_DFB==DBG_SYSLOG)                                         \
            isyslog( "DFB %s <%d>: %d=" #x, __FILE__, __LINE__, err );       \
    }                                                                        \
}

#define GDL_ERR_STR(x) (                                                   \
    (GDL_SUCCESS                   ==x)?("SUCCESS")                : \
    (GDL_ERR_INVAL                 ==x)?("INVAL")                  : \
    (GDL_ERR_BUSY                  ==x)?("BUSY")                   : \
    (GDL_ERR_DISPLAY               ==x)?("DISPLAY")                : \
    (GDL_ERR_SURFACE               ==x)?("SURFACE")                : \
    (GDL_ERR_COMMAND               ==x)?("COMMAND")                : \
    (GDL_ERR_NULL_ARG              ==x)?("NULL_ARG")               : \
    (GDL_ERR_NO_MEMORY             ==x)?("NO_MEMORY")              : \
    (GDL_ERR_FAILED                ==x)?("FAILED")                 : \
    (GDL_ERR_INTERNAL              ==x)?("INTERNAL")               : \
    (GDL_ERR_NOT_IMPL              ==x)?("NOT_IMPL")               : \
    (GDL_ERR_MAPPED                ==x)?("MAPPED")                 : \
    (GDL_ERR_NO_INIT               ==x)?("NO_INIT")                : \
    (GDL_ERR_NO_HW_SUPPORT         ==x)?("NO_HW_SUPPORT")          : \
    (GDL_ERR_INVAL_PF              ==x)?("INVAL_PF")               : \
    (GDL_ERR_INVAL_RECT            ==x)?("INVAL_RECT")             : \
    (GDL_ERR_ATTR_ID               ==x)?("ATTR_ID")                : \
    (GDL_ERR_ATTR_NO_SUPPORT       ==x)?("ATTR_NO_SUPPORT")        : \
    (GDL_ERR_ATTR_READONLY         ==x)?("ATTR_READONLY")          : \
    (GDL_ERR_ATTR_VALUE            ==x)?("ATTR_VALUE")             : \
    (GDL_ERR_PLANE_CONFLICT        ==x)?("PLANE_CONFLICT")         : \
    (GDL_ERR_DISPLAY_CONFLICT      ==x)?("DISPLAY_CONFLICT")       : \
    (GDL_ERR_TIMEOUT               ==x)?("TIMEOUT")                : \
    (GDL_ERR_MISSING_BEGIN         ==x)?("MISSING_BEGIN")          : \
    (GDL_ERR_PLANE_ID              ==x)?("PLANE_ID")               : \
    (GDL_ERR_INVAL_PTR             ==x)?("INVAL_PTR")              : \
    (GDL_ERR_INVAL_HEAP            ==x)?("INVAL_HEAP")             : \
    (GDL_ERR_HEAP_IN_USE           ==x)?("HEAP_IN_USE")            : \
    (GDL_ERR_INVAL_CALLBACK        ==x)?("INVAL_CALLBACK")         : \
    (GDL_ERR_SCALING_POLICY        ==x)?("SCALING_POLICY")         : \
    (GDL_ERR_INVAL_EVENT           ==x)?("INVAL_EVENT")            : \
    (GDL_ERR_INVAL_IOCTL           ==x)?("INVAL_IOCTL")            : \
    (GDL_ERR_SCHED_IN_ATOMIC       ==x)?("SCHED_IN_ATOMIC")        : \
    (GDL_ERR_MMAP                  ==x)?("MMAP")                   : \
    (GDL_ERR_HDCP                  ==x)?("HDCP")                   : \
    (GDL_ERR_CONFIG                ==x)?("CONFIG")                 : \
    (GDL_ERR_HDMI_AUDIO_PLAYBACK   ==x)?("HDMI_AUDIO_PLAYBACK")    : \
    (GDL_ERR_HDMI_AUDIO_BUFFER_FULL==x)?("HDMI_AUDIO_BUFFER_FULL") : \
    (GDL_ERR_PLANE_ORIGIN_ODD      ==x)?("PLANE_ORIGIN_ODD")       : \
    (GDL_ERR_PLANE_HEIGHT_ODD      ==x)?("PLANE_HEIGHT_ODD")       : \
    (GDL_ERR_HANDLE                ==x)?("HANDLE")                 : \
    (GDL_ERR_TVMODE_UNDEFINED      ==x)?("TVMODE_UNDEFINED")       : \
    (GDL_ERR_PREMULT_CONFLICT      ==x)?("PREMULT_CONFLICT")       : \
    (GDL_ERR_SUSPENDED             ==x)?("SUSPENDED")              : \
    (GDL_ERR_STEREO_PLANE          ==x)?("STEREO_PLANE")           : \
    (GDL_ERR_CE4100_3D_ORIGIN      ==x)?("CE4100_3D_ORIGIN")       : \
    (GDL_ERR_HDCP_KSV_INVALID      ==x)?("HDCP_KSV_INVALID")       : \
    (GDL_ERR_HDCP_KSV_REVOKED      ==x)?("HDCP_KSV_REVOKED")       : \
    (GDL_ERR_HDCP_NO_ACK           ==x)?("HDCP_NO_ACK")            : \
    (GDL_ERR_HDCP_LINK_INTEGRITY   ==x)?("HDCP_LINK_INTEGRITY")    : \
    trNOOP(""))

#define GDLCHECK(x...) {                                                                          \
    ret = x;                                                                                      \
    if (ret != GDL_SUCCESS) {                                                                     \
        if(DBG_GDL==DBG_STDOUT)                                                                   \
            printf( "GDL %s <%d>: %d %s=" #x "\n", __FILE__, __LINE__, ret, GDL_ERR_STR(ret) );   \
        else if(DBG_GDL==DBG_SYSLOG)                                                              \
            isyslog( "GDL %s <%d>: %d %s=" #x, __FILE__, __LINE__, ret, GDL_ERR_STR(ret) );       \
    }                                                                                             \
}

#define GDLCHECK_IGN(y,x...) {                                                                    \
    ret = x;                                                                                      \
    if ((GDL_SUCCESS != ret) && (y != ret)) {                                                     \
        if(DBG_GDL==DBG_STDOUT)                                                                   \
            printf( "GDL %s <%d>: %d %s=" #x "\n", __FILE__, __LINE__, ret, GDL_ERR_STR(ret) );   \
        else if(DBG_GDL==DBG_SYSLOG)                                                              \
            isyslog( "GDL %s <%d>: %d %s=" #x, __FILE__, __LINE__, ret, GDL_ERR_STR(ret) );       \
    }                                                                                             \
}

#define GDLSETPORT_BOOL(x,y,z) {             \
    gdl_ret_t ret = GDL_SUCCESS;             \
    gdl_boolean_t val = z;                   \
    GDLCHECK(gdl_port_set_attr(x, y, &val)); \
}

#define ISMD_ERR_STR(x) (                                                      \
    (ISMD_SUCCESS                      ==x)?("SUCCESS")                : \
    (ISMD_ERROR_FEATURE_NOT_IMPLEMENTED==x)?("FEATURE_NOT_IMPLEMENTED"): \
    (ISMD_ERROR_FEATURE_NOT_SUPPORTED  ==x)?("FEATURE_NOT_SUPPORTED")  : \
    (ISMD_ERROR_INVALID_VERBOSITY_LEVEL==x)?("INVALID_VERBOSITY_LEVEL"): \
    (ISMD_ERROR_INVALID_PARAMETER      ==x)?("INVALID_PARAMETER")      : \
    (ISMD_ERROR_INVALID_HANDLE         ==x)?("INVALID_HANDLE")         : \
    (ISMD_ERROR_NO_RESOURCES           ==x)?("NO_RESOURCES")           : \
    (ISMD_ERROR_INVALID_RESOURCE       ==x)?("INVALID_RESOURCE")       : \
    (ISMD_ERROR_INVALID_QUEUE_TYPE     ==x)?("INVALID_QUEUE_TYPE")     : \
    (ISMD_ERROR_NO_DATA_AVAILABLE      ==x)?("NO_DATA_AVAILABLE")      : \
    (ISMD_ERROR_NO_SPACE_AVAILABLE     ==x)?("NO_SPACE_AVAILABLE")     : \
    (ISMD_ERROR_TIMEOUT                ==x)?("TIMEOUT")                : \
    (ISMD_ERROR_EVENT_BUSY             ==x)?("EVENT_BUSY")             : \
    (ISMD_ERROR_OBJECT_DELETED         ==x)?("OBJECT_DELETED")         : \
    (ISMD_ERROR_ALREADY_INITIALIZED    ==x)?("ALREADY_INITIALIZED")    : \
    (ISMD_ERROR_IOCTL_FAILED           ==x)?("IOCTL_FAILED")           : \
    (ISMD_ERROR_INVALID_BUFFER_TYPE    ==x)?("INVALID_BUFFER_TYPE")    : \
    (ISMD_ERROR_INVALID_FRAME_TYPE     ==x)?("INVALID_FRAME_TYPE")     : \
    (ISMD_ERROR_QUEUE_BUSY             ==x)?("QUEUE_BUSY")             : \
    (ISMD_ERROR_NOT_FOUND              ==x)?("NOT_FOUND")              : \
    (ISMD_ERROR_OPERATION_FAILED       ==x)?("OPERATION_FAILED")       : \
    (ISMD_ERROR_PORT_BUSY              ==x)?("PORT_BUSY")              : \
    (ISMD_ERROR_NULL_POINTER           ==x)?("NULL_POINTER")           : \
    (ISMD_ERROR_INVALID_REQUEST        ==x)?("INVALID_REQUEST")        : \
    (ISMD_ERROR_OUT_OF_RANGE           ==x)?("OUT_OF_RANGE")           : \
    (ISMD_ERROR_NOT_DONE               ==x)?("NOT_DONE")               : \
    (ISMD_ERROR_SUSPENDED              ==x)?("SUSPENDED")              : \
    (ISMD_ERROR_UNSPECIFIED            ==x)?("UNSPECIFIED")            : \
    trNOOP(""))

#define SMDCHECK(x...) {                                                                          \
    ret = x;                                                                                      \
    if (ISMD_SUCCESS != ret) {                                                                    \
        if(DBG_SMD==DBG_STDOUT)                                                                   \
            printf( "SMD %s <%d>: %d %s=" #x "\n", __FILE__, __LINE__, ret, ISMD_ERR_STR(ret) );  \
        else if(DBG_SMD==DBG_SYSLOG)                                                              \
            isyslog( "SMD %s <%d>: %d %s=" #x, __FILE__, __LINE__, ret, ISMD_ERR_STR(ret) );      \
    }                                                                                             \
}

#define SMDCHECK_IGN(y,x...) {                                                                    \
    ret = x;                                                                                      \
    if ((ISMD_SUCCESS != ret) && (y != ret)) {                                                    \
        if(DBG_SMD==DBG_STDOUT)                                                                   \
            printf( "SMD %s <%d>: %d %s=" #x "\n", __FILE__, __LINE__, ret, ISMD_ERR_STR(ret) );  \
        else if(DBG_SMD==DBG_SYSLOG)                                                              \
            isyslog( "SMD %s <%d>: %d %s=" #x, __FILE__, __LINE__, ret, ISMD_ERR_STR(ret) );      \
    }                                                                                             \
}

#define SMDCHECK_CLEANUP(x...) { \
    SMDCHECK(x);                 \
    if (ISMD_SUCCESS != ret)     \
        goto cleanup;            \
}

#define DEV_STATE_STR(x) (                          \
    (ISMD_DEV_STATE_INVALID==x)?trNOOP("INVALID") : \
    (ISMD_DEV_STATE_STOP   ==x)?trNOOP("STOP")    : \
    (ISMD_DEV_STATE_PAUSE  ==x)?trNOOP("PAUSE")   : \
    (ISMD_DEV_STATE_PLAY   ==x)?trNOOP("PLAY")    : \
    trNOOP(""))

#define CODEC_TYPE_STR(x) (                          \
    (ISMD_CODEC_TYPE_INVALID==x)?("INVALID") : \
    (ISMD_CODEC_TYPE_MPEG2  ==x)?("MPEG2")   : \
    (ISMD_CODEC_TYPE_H264   ==x)?("H264")    : \
    (ISMD_CODEC_TYPE_VC1    ==x)?("VC1")     : \
    (ISMD_CODEC_TYPE_MPEG4  ==x)?("MPEG4")   : \
    trNOOP(""))

#define PIXEL_FORMAT_STR(x) (                         \
    (ISMD_PF_ARGB_32     ==x)?("ARGB_32")      :\
    (ISMD_PF_RGB_32      ==x)?("RGB_32")       :\
    (ISMD_PF_RGB_24      ==x)?("RGB_24")       :\
    (ISMD_PF_ARGB_16_1555==x)?("ARGB_16_1555") :\
    (ISMD_PF_ARGB_16_4444==x)?("ARGB_16_4444") :\
    (ISMD_PF_RGB_16      ==x)?("RGB_16")       :\
    (ISMD_PF_RGB_15      ==x)?("RGB_15")       :\
    (ISMD_PF_RGB_8       ==x)?("RGB_8")        :\
    (ISMD_PF_ARGB_8      ==x)?("ARGB_8")       :\
    (ISMD_PF_YUY2        ==x)?("YUY2")         :\
    (ISMD_PF_UYVY        ==x)?("UYVY")         :\
    (ISMD_PF_YVYU        ==x)?("YVYU")         :\
    (ISMD_PF_VYUY        ==x)?("VYUY")         :\
    (ISMD_PF_YV12        ==x)?("YV12")         :\
    (ISMD_PF_YVU9        ==x)?("YVU9")         :\
    (ISMD_PF_I420        ==x)?("I420")         :\
    (ISMD_PF_I422        ==x)?("I422")         :\
    (ISMD_PF_YV16        ==x)?("YV16")         :\
    (ISMD_PF_NV12        ==x)?("NV12")         :\
    (ISMD_PF_NV15        ==x)?("NV15")         :\
    (ISMD_PF_NV16        ==x)?("NV16")         :\
    (ISMD_PF_NV20        ==x)?("NV20")         :\
    (ISMD_PF_A8          ==x)?("A8")           :\
    trNOOP(""))

#define VIDDEC_STREAM_FORMAT_STR(x) (                                             \
    (VIDDEC_STREAM_FORMAT_2D                 ==x)?("2D")                  : \
    (VIDDEC_STREAM_FORMAT_3D_SIDEBYSIDE      ==x)?("3D_SIDEBYSIDE")       : \
    (VIDDEC_STREAM_FORMAT_3D_OVER_UNDER      ==x)?("3D_OVER_UNDER")       : \
    (VIDDEC_STREAM_FORMAT_3D_FRAME_SEQUENTIAL==x)?("3D_FRAME_SEQUENTIAL") : \
    (VIDDEC_STREAM_FORMAT_3D_ROW_INTERLEAVING==x)?("3D_ROW_INTERLEAVING") : \
    (VIDDEC_STREAM_FORMAT_3D_MVC             ==x)?("3D_MVC")              : \
    trNOOP(""))

#define HDMI_AUDIO_TO_MEDIA_FORMAT(x) (                                 \
    (GDL_HDMI_AUDIO_FORMAT_UNDEFINED==x)?ISMD_AUDIO_MEDIA_FMT_INVALID : \
    (GDL_HDMI_AUDIO_FORMAT_PCM      ==x)?ISMD_AUDIO_MEDIA_FMT_PCM     : \
    (GDL_HDMI_AUDIO_FORMAT_AC3      ==x)?ISMD_AUDIO_MEDIA_FMT_DD      : \
    (GDL_HDMI_AUDIO_FORMAT_MPEG1    ==x)?ISMD_AUDIO_MEDIA_FMT_MPEG    : \
    (GDL_HDMI_AUDIO_FORMAT_MP3      ==x)?ISMD_AUDIO_MEDIA_FMT_MPEG    : \
    (GDL_HDMI_AUDIO_FORMAT_MPEG2    ==x)?ISMD_AUDIO_MEDIA_FMT_MPEG    : \
    (GDL_HDMI_AUDIO_FORMAT_AAC      ==x)?ISMD_AUDIO_MEDIA_FMT_AAC     : \
    (GDL_HDMI_AUDIO_FORMAT_DTS      ==x)?ISMD_AUDIO_MEDIA_FMT_DTS     : \
    (GDL_HDMI_AUDIO_FORMAT_ATRAC    ==x)?ISMD_AUDIO_MEDIA_FMT_INVALID : \
    (GDL_HDMI_AUDIO_FORMAT_OBA      ==x)?ISMD_AUDIO_MEDIA_FMT_INVALID : \
    (GDL_HDMI_AUDIO_FORMAT_DDP      ==x)?ISMD_AUDIO_MEDIA_FMT_DD_PLUS : \
    (GDL_HDMI_AUDIO_FORMAT_DTSHD    ==x)?ISMD_AUDIO_MEDIA_FMT_DTS_HD  : \
    (GDL_HDMI_AUDIO_FORMAT_MLP      ==x)?ISMD_AUDIO_MEDIA_FMT_TRUE_HD : \
    (GDL_HDMI_AUDIO_FORMAT_DST      ==x)?ISMD_AUDIO_MEDIA_FMT_INVALID : \
    (GDL_HDMI_AUDIO_FORMAT_WMA_PRO  ==x)?ISMD_AUDIO_MEDIA_FMT_WM9     : \
    ISMD_AUDIO_MEDIA_FMT_INVALID)

#define AUDIO_MEDIA_FMT_STR(x) (                                                  \
    (ISMD_AUDIO_MEDIA_FMT_INVALID            ==x)? ("INVALID")             : \
    (ISMD_AUDIO_MEDIA_FMT_PCM                ==x)? ("PCM")        : \
    (ISMD_AUDIO_MEDIA_FMT_DVD_PCM            ==x)? ("DVD_PCM")             : \
    (ISMD_AUDIO_MEDIA_FMT_BLURAY_PCM         ==x)? ("BLUREAY_PCM")         : \
    (ISMD_AUDIO_MEDIA_FMT_MPEG               ==x)? ("MPEG")                : \
    (ISMD_AUDIO_MEDIA_FMT_AAC                ==x)? ("AAC")                 : \
    (ISMD_AUDIO_MEDIA_FMT_AAC_LOAS           ==x)? ("AAC_LOAS")            : \
    (ISMD_AUDIO_MEDIA_FMT_DD                 ==x)? ("Dolby Digital")       : \
    (ISMD_AUDIO_MEDIA_FMT_DD_PLUS            ==x)? ("Dolby Digital+")      : \
    (ISMD_AUDIO_MEDIA_FMT_TRUE_HD            ==x)? ("TRUE_HD")             : \
    (ISMD_AUDIO_MEDIA_FMT_DTS_HD             ==x)? ("DTS_HD")              : \
    (ISMD_AUDIO_MEDIA_FMT_DTS_HD_HRA         ==x)? ("DTS_HD_HRA")          : \
    (ISMD_AUDIO_MEDIA_FMT_DTS_HD_MA          ==x)? ("DTS_HD_MA")           : \
    (ISMD_AUDIO_MEDIA_FMT_DTS                ==x)? ("DTS")                 : \
    (ISMD_AUDIO_MEDIA_FMT_DTS_LBR            ==x)? ("DTS_LBR")             : \
    (ISMD_AUDIO_MEDIA_FMT_DTS_BC             ==x)? ("DTS_BC")              : \
    (ISMD_AUDIO_MEDIA_FMT_WM9                ==x)? ("WM9")                 : \
    trNOOP(""))
//    (ISMD_AUDIO_MEDIA_FMT_IEC61937_COMPRESSED==x)?trNOOP("IEC61937_COMPRESSED") :

#define HDMI_AUDIO_FORMAT_STR(x) (                             \
    (GDL_HDMI_AUDIO_FORMAT_UNDEFINED==x)?("UNDEFINED") : \
    (GDL_HDMI_AUDIO_FORMAT_PCM      ==x)?("analog (PCM)")       : \
    (GDL_HDMI_AUDIO_FORMAT_AC3      ==x)?("Dolby Digital")       : \
    (GDL_HDMI_AUDIO_FORMAT_MPEG1    ==x)?("MPEG1")     : \
    (GDL_HDMI_AUDIO_FORMAT_MP3      ==x)?("MP3")       : \
    (GDL_HDMI_AUDIO_FORMAT_MPEG2    ==x)?("MPEG2")     : \
    (GDL_HDMI_AUDIO_FORMAT_AAC      ==x)?("AAC")       : \
    (GDL_HDMI_AUDIO_FORMAT_DTS      ==x)?("dts")       : \
    (GDL_HDMI_AUDIO_FORMAT_ATRAC    ==x)?("ATRAC")     : \
    (GDL_HDMI_AUDIO_FORMAT_OBA      ==x)?("OBA")       : \
    (GDL_HDMI_AUDIO_FORMAT_DDP      ==x)?("DDP")       : \
    (GDL_HDMI_AUDIO_FORMAT_DTSHD    ==x)?("DTSHD")     : \
    (GDL_HDMI_AUDIO_FORMAT_MLP      ==x)?("MLP")       : \
    (GDL_HDMI_AUDIO_FORMAT_DST      ==x)?("DST")       : \
    (GDL_HDMI_AUDIO_FORMAT_WMA_PRO  ==x)?("WMA_PRO")   : \
    trNOOP(""))

#define HDMI_AUDIO_FS_STR(x) (                             \
    (GDL_HDMI_AUDIO_FS_32_KHZ    ==(x))?(   " 32") : \
    (GDL_HDMI_AUDIO_FS_44_1_KHZ  ==(x))?( " 44.1") : \
    (GDL_HDMI_AUDIO_FS_48_KHZ    ==(x))?(   " 48") : \
    (GDL_HDMI_AUDIO_FS_88_2_KHZ  ==(x))?( " 88.2") : \
    (GDL_HDMI_AUDIO_FS_96_KHZ    ==(x))?(   " 96") : \
    (GDL_HDMI_AUDIO_FS_176_4_KHZ ==(x))?(" 176.4") : \
    (GDL_HDMI_AUDIO_FS_192_KHZ   ==(x))?(  " 192") : \
    trNOOP(""))

#define HDMI_AUDIO_SS_STR(x) (                                \
    (GDL_HDMI_AUDIO_SS_16        ==(x))?(" 16")       : \
    (GDL_HDMI_AUDIO_SS_20        ==(x))?(" 20")       : \
    (GDL_HDMI_AUDIO_SS_24        ==(x))?(" 24")       : \
    trNOOP(""))

#define REFRESH_STR(x) (                              \
    (GDL_REFRESH_23_98       ==(x))?("23.98") : \
    (GDL_REFRESH_24          ==(x))?("24")    : \
    (GDL_REFRESH_25          ==(x))?("25")    : \
    (GDL_REFRESH_29_97       ==(x))?("29.97") : \
    (GDL_REFRESH_30          ==(x))?("30")    : \
    (GDL_REFRESH_50          ==(x))?("50")    : \
    (GDL_REFRESH_59_94       ==(x))?("59.94") : \
    (GDL_REFRESH_60          ==(x))?("60")    : \
    (GDL_REFRESH_48          ==(x))?("48")    : \
    (GDL_REFRESH_47_96       ==(x))?("47.96") : \
    (GDL_REFRESH_NONE        ==(x))?("NONE")  : \
    (GDL_REFRESH_USER_DEFINED==(x))?("USER")  : \
    trNOOP(""))

#define AUDIO_OUTPUT_STR(x) (                                              \
    (ISMD_AUDIO_OUTPUT_INVALID              ==x)?trNOOP("INVALID")       : \
    (ISMD_AUDIO_OUTPUT_PCM                  ==x)?trNOOP("analog (PCM)")  : \
    (ISMD_AUDIO_OUTPUT_PASSTHROUGH          ==x)?trNOOP("bitstream")     : \
    (ISMD_AUDIO_OUTPUT_ENCODED_DOLBY_DIGITAL==x)?trNOOP("Dolby Digital") : \
    (ISMD_AUDIO_OUTPUT_ENCODED_DTS          ==x)?trNOOP("dts")           : \
    trNOOP(""))

#define AUDIO_CHANNEL_STR(x) (                                 \
    (ISMD_AUDIO_CHAN_CONFIG_INVALID ==x)?trNOOP("INVALID")   : \
    (ISMD_AUDIO_STEREO              ==x)?trNOOP("Stereo")    : \
    (ISMD_AUDIO_DUAL_MONO           ==x)?trNOOP("Dual mono") : \
    (ISMD_AUDIO_5_1                 ==x)?trNOOP("5.1")       : \
    (ISMD_AUDIO_7_1                 ==x)?trNOOP("7.1")       : \
    trNOOP(""))

#define DEINTERLACE_MODE_STR(x) (                                           \
    (ISMD_VIDPPROC_DI_POLICY_NONE           ==x)?trNOOP("Only if scaling")           : \
    (ISMD_VIDPPROC_DI_POLICY_SPATIAL_ONLY   ==x)?trNOOP("Spatial only")   : \
    (ISMD_VIDPPROC_DI_POLICY_FILM           ==x)?trNOOP("Film")           : \
    (ISMD_VIDPPROC_DI_POLICY_VIDEO          ==x)?trNOOP("Video")          : \
    (ISMD_VIDPPROC_DI_POLICY_AUTO           ==x)?trNOOP("Auto")           : \
    (ISMD_VIDPPROC_DI_POLICY_TOP_FIELD_ONLY ==x)?trNOOP("Top field only") : \
    (ISMD_VIDPPROC_DI_POLICY_NEVER          ==x)?trNOOP("Never")          : \
    trNOOP(""))

#define SCALE_MODE_STR(x) (                                                             \
    (ISMD_VIDPPROC_SCALING_POLICY_SCALE_TO_FIT            ==x)?trNOOP("Scale to fit") : \
    (ISMD_VIDPPROC_SCALING_POLICY_NO_SCALING              ==x)?trNOOP("No scaling")   : \
    (ISMD_VIDPPROC_SCALING_POLICY_ZOOM_TO_FILL            ==x)?trNOOP("Zoom to fill") : \
    (ISMD_VIDPPROC_SCALING_POLICY_ZOOM_TO_FIT             ==x)?trNOOP("Standard")     : \
    (ISMD_VIDPPROC_SCALING_POLICY_NON_LINEAR_SCALE_TO_FIT ==x)?trNOOP("Non linear")   : \
    trNOOP("Unknown"))

#define EDID_MODE_STR(x) ( \
    (GDL_HDMI_USE_EDID_NONE==x)?trNOOP("None") : \
    (GDL_HDMI_USE_EDID_SAFE==x)?trNOOP("Safe") : \
    (GDL_HDMI_USE_EDID_REAL==x)?trNOOP("Real") : \
    trNOOP("Unknown"))

#define INVALID_LEVEL        0
#define MIN_DERINGNING_LEVEL (INVALID_LEVEL+1)
#define MAX_DERINGNING_LEVEL 100
#define MIN_GAUSSIAN_LEVEL   (INVALID_LEVEL+1)
#define MAX_GAUSSIAN_LEVEL   3
#define MIN_DEGRADE_LEVEL    (INVALID_LEVEL+1)
#define MAX_DEGRADE_LEVEL    100

enum tICE_TVMode {
    ICE_TVMODE_NONE = 0,
    ICE_TVMODE_PAL,
    ICE_TVMODE_NTSC,
    ICE_TVMODE_480I,
    ICE_TVMODE_480P,
    ICE_TVMODE_576I,
    ICE_TVMODE_576P,
    ICE_TVMODE_720P50,
    ICE_TVMODE_720P59_94,
    ICE_TVMODE_1080I50,
    ICE_TVMODE_1080I59_94,
    ICE_TVMODE_1080P23_98,
    ICE_TVMODE_1080P25,
    ICE_TVMODE_1080P29_97,
    ICE_TVMODE_1080P50,
    ICE_TVMODE_1080P59_94,
    ICE_TVMODE_SOURCE,
    ICE_TVMODE_NATIVE,
    ICE_TVMODE_COUNT
}; // tICE_TVMode

#define TVMODE_STR(x) (                                \
    (ICE_TVMODE_NONE       ==x)?trNOOP("None")       : \
    (ICE_TVMODE_PAL        ==x)?trNOOP("PAL")        : \
    (ICE_TVMODE_NTSC       ==x)?trNOOP("NTSC")       : \
    (ICE_TVMODE_480I       ==x)?trNOOP("480i")       : \
    (ICE_TVMODE_480P       ==x)?trNOOP("480p")       : \
    (ICE_TVMODE_576I       ==x)?trNOOP("576i")       : \
    (ICE_TVMODE_576P       ==x)?trNOOP("576p")       : \
    (ICE_TVMODE_720P50     ==x)?trNOOP("720p50")     : \
    (ICE_TVMODE_720P59_94  ==x)?trNOOP("720p60")     : \
    (ICE_TVMODE_1080I50    ==x)?trNOOP("1080i50")    : \
    (ICE_TVMODE_1080I59_94 ==x)?trNOOP("1080i60")    : \
    (ICE_TVMODE_1080P23_98 ==x)?trNOOP("1080p24")    : \
    (ICE_TVMODE_1080P25    ==x)?trNOOP("1080p25")    : \
    (ICE_TVMODE_1080P29_97 ==x)?trNOOP("1080p30")    : \
    (ICE_TVMODE_1080P50    ==x)?trNOOP("1080p50")    : \
    (ICE_TVMODE_1080P59_94 ==x)?trNOOP("1080p60")    : \
    (ICE_TVMODE_SOURCE     ==x)?trNOOP("Source")     : \
    (ICE_TVMODE_NATIVE     ==x)?trNOOP("Automatic")     : \
    trNOOP("Unknown"))

#define PPROC_PAR_STR(x) (                                                                     \
      (PAR_DI_POLICY                        ==x)?("DI_POLICY")                         : \
      (PAR_MADTH_DELTA                      ==x)?("MADTH_DELTA")                       : \
      (PAR_NRF_LEVEL                        ==x)?("NRF_LEVEL")                         : \
      (PAR_ENABLE_HSC_DERING                ==x)?("ENABLE_HSC_DERING")                 : \
      (PAR_ENABLE_VSC_DERING                ==x)?("ENABLE_VSC_DERING")                 : \
      (PAR_HSC_COEFF_IDXSHIFT_Y             ==x)?("HSC_COEFF_IDXSHIFT_Y")              : \
      (PAR_HSC_COEFF_IDXSHIFT_UV            ==x)?("HSC_COEFF_IDXSHIFT_UV")             : \
      (PAR_VSC_COEFF_IDXSHIFT_Y             ==x)?("VSC_COEFF_IDXSHIFT_Y")              : \
      (PAR_VSC_COEFF_IDXSHIFT_UV            ==x)?("VSC_COEFF_IDXSHIFT_UV")             : \
      (PAR_SCALER_TABLES_NUM_Y              ==x)?("SCALER_TABLES_NUM_Y")               : \
      (PAR_SCALER_TABLES_NUM_UV             ==x)?("SCALER_TABLES_NUM_UV")              : \
      (PAR_STREAM_PRIORITY                  ==x)?("STREAM_PRIORITY")                   : \
      (PAR_BYPASS_DEINTERLACE               ==x)?("BYPASS_DEINTERLACE")                : \
      (PAR_MAD_WEIGHT_POST                  ==x)?("MAD_WEIGHT_POST")                   : \
      (PAR_MAD_WEIGHT_PRE                   ==x)?("MAD_WEIGHT_PRE")                    : \
      (PAR_SAD_WEIGHT_POST                  ==x)?("SAD_WEIGHT_POST")                   : \
      (PAR_SAD_WEIGHT_PRE1                  ==x)?("SAD_WEIGHT_PRE1")                   : \
      (PAR_SAD_WEIGHT_PRE2                  ==x)?("SAD_WEIGHT_PRE2")                   : \
      (PAR_ENABLE_GEN4_ENHANCED_DI_ALGORITHM==x)?("ENABLE_GEN4_ENHANCED_DI_ALGORITHM") : \
      (PAR_BYPASS_AUTO_FMD_32322_23322      ==x)?("BYPASS_AUTO_FMD_32322_23322")       : \
      (PAR_BYPASS_AUTO_FMD_32               ==x)?("BYPASS_AUTO_FMD_32")                : \
      (PAR_BYPASS_AUTO_FMD_2224_2332        ==x)?("BYPASS_AUTO_FMD_2224_2332")         : \
      (PAR_BYPASS_AUTO_FMD_22               ==x)?("BYPASS_AUTO_FMD_22")                : \
      (PAR_BYPASS_AUTO_FMD_33               ==x)?("BYPASS_AUTO_FMD_33")                : \
      (PAR_BYPASS_AUTO_FMD_ETC              ==x)?("BYPASS_AUTO_FMD_ETC")               : \
      (PAR_DISABLE_MAD_MOTION_DETECTION     ==x)?("DISABLE_MAD_MOTION_DETECTION")      : \
      (FMD_CADENCE_CHANGED_EVENT            ==x)?("FMD_CADENCE_CHANGED_EVENT")         : \
      (FMD_CADENCE_TYPE                     ==x)?("FMD_CADENCE_TYPE")                  : \
      (HW_HANG_TYPE                         ==x)?("HW_HANG_TYPE")                      : \
      (PAR_FRAME_PACKING_FORMAT             ==x)?("FRAME_PACKING_FORMAT")              : \
      (PAR_RIGHT_SHIFT_FOR_RIGHT_VIEW       ==x)?("RIGHT_SHIFT_FOR_RIGHT_VIEW")        : \
      (PAR_DOWN_SHIFT_FOR_BOTTOM_VIEW       ==x)?("DOWN_SHIFT_FOR_BOTTOM_VIEW")        : \
      trNOOP("Unknown"))

enum tICE_Queue {
    ICE_QUEUE_AUDIO,
    ICE_QUEUE_VIDEO,
    ICE_QUEUE_PIP,
    ICE_QUEUE_COUNT
}; // tICE_Queue

#define QUEUE_STR(x) (                     \
    (ICE_QUEUE_AUDIO==x)?trNOOP("Audio") : \
    (ICE_QUEUE_VIDEO==x)?trNOOP("Video") : \
    (ICE_QUEUE_PIP  ==x)?trNOOP("PiP")   : \
    trNOOP("Unknown"))

enum tICE_Event {
    BUFMON_UNDERRUN,
    BUFMON_OVERRUN,
    BUFMON_CRITICAL,
    VIDDEC_OUT_OF_MEMORY,
    VIDDEC_RESOLUTION_CHANGE,
    VIDDEC_EOS,
    VIDDEC_BITSTREAM_ERROR,
    VIDDEC_DROPPED_FRAMES,
    VIDDEC_CLIENT_ID_SEEN,
    VIDDEC_UNDERFLOW,
    VIDDEC_UNDERFLOW_RECOVERED,
    VIDDEC_USER_DATA,
    VIDPPROC_INPUT_SRC_RESOLUTION,
    VIDPPROC_INPUT_DISP_RESOLUTION,
    VIDPPROC_INPUT_HW_HANG_PREVENTED,
    VIDREND_EVENT_TYPE_ERROR,
    VIDREND_EVENT_TYPE_VSYNC_FRAME,
    VIDREND_EVENT_TYPE_VSYNC_FIELD_TOP,
    VIDREND_EVENT_TYPE_VSYNC_FIELD_BOTTOM,
    VIDREND_EVENT_TYPE_RES_CHG,
    VIDREND_EVENT_TYPE_EOS,
    VIDREND_EVENT_TYPE_CLIENT_ID_SEEN,
    VIDREND_EVENT_TYPE_EOSEG,
    VIDREND_EVENT_TYPE_UNDERRUN,
    VIDREND_EVENT_TYPE_SOSEG,
    VIDREND_EVENT_TYPE_FRAME_FLIPPED,
    VIDREND_EVENT_TYPE_UNDERFLOW,
    VIDREND_EVENT_TYPE_UNDERFLOW_RECOVERED,
    AUDIO_NOTIFY_STREAM_BEGIN,
    AUDIO_NOTIFY_STREAM_END,
    AUDIO_NOTIFY_RENDER_UNDERRUN,
    AUDIO_NOTIFY_WATERMARK,
    AUDIO_NOTIFY_TAG_RECEIVED,
    AUDIO_NOTIFY_SAMPLE_RATE_CHANGE,
    AUDIO_NOTIFY_CODEC_CHANGE,
    AUDIO_NOTIFY_SAMPLE_SIZE_CHANGE,
    AUDIO_NOTIFY_CHANNEL_CONFIG_CHANGE,
    AUDIO_NOTIFY_PTS_VALUE_EARLY,
    AUDIO_NOTIFY_PTS_VALUE_LATE,
    AUDIO_NOTIFY_PTS_VALUE_RECOVERED,
    AUDIO_NOTIFY_CORRUPT_FRAME,
    AUDIO_CAPTURE_OVERRUN,
    AUDIO_NOTIFY_CLIENT_ID,
    AUDIO_NOTIFY_SEGMENT_END,
    AUDIO_NOTIFY_SRC_STATUS_CHANGE,
    AUDIO_NOTIFY_DECODE_ERROR,
    AUDIO_NOTIFY_DECODE_SYNC_FOUND,
    AUDIO_NOTIFY_DECODE_SYNC_LOST,
    AUDIO_NOTIFY_INPUT_FULL,
    AUDIO_NOTIFY_INPUT_EMPTY,
    AUDIO_NOTIFY_INPUT_RECOVERED,
    AUDIO_NOTIFY_SEGMENT_START,
    AUDIO_NOTIFY_DATA_RENDERED,
    EVENT_COUNT
}; // tICE_Event

#define EVENT_STR(x) (                                                                             \
    (BUFMON_UNDERRUN                       ==x)?("BUFMON_UNDERRUN")                        : \
    (BUFMON_OVERRUN                        ==x)?("BUFMON_OVERRUN")                         : \
    (BUFMON_CRITICAL                       ==x)?("BUFMON_CRITICAL")                        : \
    (VIDDEC_OUT_OF_MEMORY                  ==x)?("VIDDEC_OUT_OF_MEMORY")                   : \
    (VIDDEC_RESOLUTION_CHANGE              ==x)?("VIDDEC_RESOLUTION_CHANGE")               : \
    (VIDDEC_EOS                            ==x)?("VIDDEC_EOS")                             : \
    (VIDDEC_BITSTREAM_ERROR                ==x)?("VIDDEC_BITSTREAM_ERROR")                 : \
    (VIDDEC_DROPPED_FRAMES                 ==x)?("VIDDEC_DROPPED_FRAMES")                  : \
    (VIDDEC_CLIENT_ID_SEEN                 ==x)?("VIDDEC_CLIENT_ID_SEEN")                  : \
    (VIDDEC_UNDERFLOW                      ==x)?("VIDDEC_UNDERFLOW")                       : \
    (VIDDEC_UNDERFLOW_RECOVERED            ==x)?("VIDDEC_UNDERFLOW_RECOVERED")             : \
    (VIDDEC_USER_DATA                      ==x)?("VIDDEC_USER_DATA")                       : \
    (VIDPPROC_INPUT_SRC_RESOLUTION         ==x)?("VIDPPROC_INPUT_SRC_RESOLUTION")          : \
    (VIDPPROC_INPUT_DISP_RESOLUTION        ==x)?("VIDPPROC_INPUT_DISP_RESOLUTION")         : \
    (VIDPPROC_INPUT_HW_HANG_PREVENTED      ==x)?("VIDPPROC_INPUT_HW_HANG_PREVENTED")       : \
    (VIDREND_EVENT_TYPE_ERROR              ==x)?("VIDREND_EVENT_TYPE_ERROR")               : \
    (VIDREND_EVENT_TYPE_VSYNC_FRAME        ==x)?("VIDREND_EVENT_TYPE_VSYNC_FRAME")         : \
    (VIDREND_EVENT_TYPE_VSYNC_FIELD_TOP    ==x)?("VIDREND_EVENT_TYPE_VSYNC_FIELD_TOP")     : \
    (VIDREND_EVENT_TYPE_VSYNC_FIELD_BOTTOM ==x)?("VIDREND_EVENT_TYPE_VSYNC_FIELD_BOTTOM")  : \
    (VIDREND_EVENT_TYPE_RES_CHG            ==x)?("VIDREND_EVENT_TYPE_RES_CHG")             : \
    (VIDREND_EVENT_TYPE_EOS                ==x)?("VIDREND_EVENT_TYPE_EOS")                 : \
    (VIDREND_EVENT_TYPE_CLIENT_ID_SEEN     ==x)?("VIDREND_EVENT_TYPE_CLIENT_ID_SEEN")      : \
    (VIDREND_EVENT_TYPE_EOSEG              ==x)?("VIDREND_EVENT_TYPE_EOSEG")               : \
    (VIDREND_EVENT_TYPE_UNDERRUN           ==x)?("VIDREND_EVENT_TYPE_UNDERRUN")            : \
    (VIDREND_EVENT_TYPE_SOSEG              ==x)?("VIDREND_EVENT_TYPE_SOSEG")               : \
    (VIDREND_EVENT_TYPE_FRAME_FLIPPED      ==x)?("VIDREND_EVENT_TYPE_FRAME_FLIPPED")       : \
    (VIDREND_EVENT_TYPE_UNDERFLOW          ==x)?("VIDREND_EVENT_TYPE_UNDERFLOW")           : \
    (VIDREND_EVENT_TYPE_UNDERFLOW_RECOVERED==x)?("VIDREND_EVENT_TYPE_UNDERFLOW_RECOVERED") : \
    (AUDIO_NOTIFY_STREAM_BEGIN             ==x)?("AUDIO_NOTIFY_STREAM_BEGIN")              : \
    (AUDIO_NOTIFY_STREAM_END               ==x)?("AUDIO_NOTIFY_STREAM_END")                : \
    (AUDIO_NOTIFY_RENDER_UNDERRUN          ==x)?("AUDIO_NOTIFY_RENDER_UNDERRUN")           : \
    (AUDIO_NOTIFY_WATERMARK                ==x)?("AUDIO_NOTIFY_WATERMARK")                 : \
    (AUDIO_NOTIFY_TAG_RECEIVED             ==x)?("AUDIO_NOTIFY_TAG_RECEIVED")              : \
    (AUDIO_NOTIFY_SAMPLE_RATE_CHANGE       ==x)?("AUDIO_NOTIFY_SAMPLE_RATE_CHANGE")        : \
    (AUDIO_NOTIFY_CODEC_CHANGE             ==x)?("AUDIO_NOTIFY_CODEC_CHANGE")              : \
    (AUDIO_NOTIFY_SAMPLE_SIZE_CHANGE       ==x)?("AUDIO_NOTIFY_SAMPLE_SIZE_CHANGE")        : \
    (AUDIO_NOTIFY_CHANNEL_CONFIG_CHANGE    ==x)?("AUDIO_NOTIFY_CHANNEL_CONFIG_CHANGE")     : \
    (AUDIO_NOTIFY_PTS_VALUE_EARLY          ==x)?("AUDIO_NOTIFY_PTS_VALUE_EARLY")           : \
    (AUDIO_NOTIFY_PTS_VALUE_LATE           ==x)?("AUDIO_NOTIFY_PTS_VALUE_LATE")            : \
    (AUDIO_NOTIFY_PTS_VALUE_RECOVERED      ==x)?("AUDIO_NOTIFY_PTS_VALUE_RECOVERED")       : \
    (AUDIO_NOTIFY_CORRUPT_FRAME            ==x)?("AUDIO_NOTIFY_CORRUPT_FRAME")             : \
    (AUDIO_CAPTURE_OVERRUN                 ==x)?("AUDIO_CAPTURE_OVERRUN")                  : \
    (AUDIO_NOTIFY_CLIENT_ID                ==x)?("AUDIO_NOTIFY_CLIENT_ID")                 : \
    (AUDIO_NOTIFY_SEGMENT_END              ==x)?("AUDIO_NOTIFY_SEGMENT_END")               : \
    (AUDIO_NOTIFY_SRC_STATUS_CHANGE        ==x)?("AUDIO_NOTIFY_SRC_STATUS_CHANGE")         : \
    (AUDIO_NOTIFY_DECODE_ERROR             ==x)?("AUDIO_NOTIFY_DECODE_ERROR")              : \
    (AUDIO_NOTIFY_DECODE_SYNC_FOUND        ==x)?("AUDIO_NOTIFY_DECODE_SYNC_FOUND")         : \
    (AUDIO_NOTIFY_DECODE_SYNC_LOST         ==x)?("AUDIO_NOTIFY_DECODE_SYNC_LOST")          : \
    (AUDIO_NOTIFY_INPUT_FULL               ==x)?("AUDIO_NOTIFY_INPUT_FULL")                : \
    (AUDIO_NOTIFY_INPUT_EMPTY              ==x)?("AUDIO_NOTIFY_INPUT_EMPTY")               : \
    (AUDIO_NOTIFY_INPUT_RECOVERED          ==x)?("AUDIO_NOTIFY_INPUT_RECOVERED")           : \
    (AUDIO_NOTIFY_SEGMENT_START            ==x)?("AUDIO_NOTIFY_SEGMENT_START")             : \
    (AUDIO_NOTIFY_DATA_RENDERED            ==x)?("AUDIO_NOTIFY_DATA_RENDERED")             : \
    trNOOP("Unknown"))

#define PRINT_VSTREAM_INFO(y,x) { \
    static ismd_time_t last_time = 0; \
    ismd_time_t curr_time; \
    SMDCHECK(ismd_clock_get_time(x->clk, &curr_time)); \
    ismd_time_t diff_time = (curr_time>last_time) ? (curr_time-last_time) : (last_time-curr_time); \
    if(diff_time>90000) { \
        last_time=curr_time; \
        ismd_viddec_stream_statistics_t vsstat; \
        SMDCHECK(ismd_viddec_get_stream_statistics(x->vid, &vsstat)); \
        if(ISMD_SUCCESS==ret) \
            isyslog("%s %ld,%lld,%ld,%ld | %ld,%ld,%ld,%ld,%ld,%ld,%ld | %ld,%ld,%ld,%ld,%ld | %ld,%ld,%ld,%ld", y, \
                vsstat.current_bit_rate, vsstat.total_bytes, vsstat.frames_decoded, vsstat.frames_dropped, vsstat.error_frames, \
                vsstat.frames_I_decoded, vsstat.frames_P_decoded, vsstat.frames_B_decoded, vsstat.frames_H264_IDR_decoded, vsstat.frames_VC1_BI_decoded, vsstat.frames_MPEG4_SVOP_decoded, \
                vsstat.maximum_contiguous_frame_drops, vsstat.invalid_bitstream_syntax_errors, vsstat.sequence_count, vsstat.number_of_parsing_errors, vsstat.number_of_decoding_errors, \
                vsstat.unsupported_profile_errors, vsstat.unsupported_level_errors, vsstat.unsupported_feature_errors, vsstat.unsupported_resolution_errors); \
    } \
}

#define PRINT_QUEUE_STATUS(y,x) { \
    ismd_vidrend_stream_position_info_t vinfo; \
    memset(&vinfo, 0, sizeof(vinfo)); \
    SMDCHECK_IGN(ISMD_ERROR_NO_DATA_AVAILABLE, ismd_vidrend_get_stream_position(x->rnd, &vinfo)); \
    ismd_audio_stream_position_info_t ainfo; \
    memset(&ainfo, 0, sizeof(ainfo)); \
    SMDCHECK_IGN(ISMD_ERROR_NO_DATA_AVAILABLE, ismd_audio_input_get_stream_position(x->aud_prc, x->aud, &ainfo)); \
    ismd_port_status_t astat, vstat; \
    SMDCHECK(ismd_port_get_status(x->queue[ICE_QUEUE_AUDIO].port, &astat)); \
    SMDCHECK(ismd_port_get_status(x->queue[ICE_QUEUE_VIDEO].port, &vstat)); \
    isyslog("%s [%d/%d] a %llx (%llx_%llx_%llx) [%d/%d] v %llx (%llx_%llx_%llx)", y, \
        astat.cur_depth, astat.max_depth, x->queue[ICE_QUEUE_AUDIO].last_pts, ainfo.segment_time, ainfo.linear_time, ainfo.scaled_time, \
        vstat.cur_depth, vstat.max_depth, x->queue[ICE_QUEUE_VIDEO].last_pts, vinfo.segment_time, vinfo.linear_time, vinfo.scaled_time); \
}

#define PRINT_HDMI_INFO(x) { \
    gdl_hdmi_sink_info_t info; \
    if(GDL_SUCCESS==gdl_port_recv(GDL_PD_ID_HDMI, GDL_PD_RECV_HDMI_SINK_INFO, &info, sizeof(info))) \
        isyslog("%s-mode edid:%s device: %s", info.hdmi?"hdmi":"dvi",EDID_MODE_STR(x->options.edid_mode), info.name ); \
    gdl_tvmode_t m; \
    if(GDL_SUCCESS==gdl_port_recv(GDL_PD_ID_HDMI, GDL_PD_RECV_HDMI_NATIVE_MODE, &m, sizeof(m))) \
        isyslog("Video [Native] %dx%d%s%s", m.width, m.height, m.interlaced?"i":"p",REFRESH_STR(m.refresh)); \
    int idx=0; \
    while(GDL_SUCCESS==gdl_get_port_tvmode_by_index(GDL_PD_ID_HDMI, idx++, &m)) \
        isyslog("Video [%d] %dx%d%s%s", idx, m.width, m.height, m.interlaced?"i":"p",REFRESH_STR(m.refresh)); \
    gdl_hdmi_audio_ctrl_t ctrl; \
    ctrl.cmd_id = GDL_HDMI_AUDIO_GET_CAPS; \
    ctrl.data._get_caps.index = 0; \
    while(GDL_SUCCESS == gdl_port_recv(GDL_PD_ID_HDMI, GDL_PD_RECV_HDMI_AUDIO_CTRL, &ctrl, sizeof(ctrl))) { \
        isyslog("Audio [%d] %s c %d %s%s%s%s%s%s%s kHz %s%s%s Bits", ctrl.data._get_caps.index, \
                HDMI_AUDIO_FORMAT_STR(ctrl.data._get_caps.cap.format), \
                ctrl.data._get_caps.cap.max_channels, \
                HDMI_AUDIO_FS_STR(ctrl.data._get_caps.cap.fs&GDL_HDMI_AUDIO_FS_32_KHZ), \
                HDMI_AUDIO_FS_STR(ctrl.data._get_caps.cap.fs&GDL_HDMI_AUDIO_FS_44_1_KHZ), \
                HDMI_AUDIO_FS_STR(ctrl.data._get_caps.cap.fs&GDL_HDMI_AUDIO_FS_48_KHZ), \
                HDMI_AUDIO_FS_STR(ctrl.data._get_caps.cap.fs&GDL_HDMI_AUDIO_FS_88_2_KHZ), \
                HDMI_AUDIO_FS_STR(ctrl.data._get_caps.cap.fs&GDL_HDMI_AUDIO_FS_96_KHZ), \
                HDMI_AUDIO_FS_STR(ctrl.data._get_caps.cap.fs&GDL_HDMI_AUDIO_FS_176_4_KHZ), \
                HDMI_AUDIO_FS_STR(ctrl.data._get_caps.cap.fs&GDL_HDMI_AUDIO_FS_192_KHZ), \
                HDMI_AUDIO_SS_STR(ctrl.data._get_caps.cap.ss_bitrate&GDL_HDMI_AUDIO_SS_16), \
                HDMI_AUDIO_SS_STR(ctrl.data._get_caps.cap.ss_bitrate&GDL_HDMI_AUDIO_SS_20), \
                HDMI_AUDIO_SS_STR(ctrl.data._get_caps.cap.ss_bitrate&GDL_HDMI_AUDIO_SS_24)); \
        ctrl.data._get_caps.index++; \
    } \
}

#define AUDIO_HW_OUTPUT_STR(x) (                             \
    (GEN3_HW_OUTPUT_I2S0  ==x)?trNOOP("Audio [analog]") : \
    (GEN3_HW_OUTPUT_SPDIF ==x)?trNOOP("Audio [S/P-DIF]") : \
    (GEN3_HW_OUTPUT_HDMI  ==x)?trNOOP("Audio [HDMI]")   : \
    trNOOP("Audio [Unknown]"))

#define PRINT_AUDIO_OUTPUT_CFG(z,y,x) { \
    isyslog("%s %s %s %s %d Hz %d Bit Delay: %d ms", z, y, AUDIO_OUTPUT_STR((x)->out_mode), AUDIO_CHANNEL_STR((x)->ch_config), (x)->sample_rate, (x)->sample_size, (x)->stream_delay); \
}

#endif /*ICE_DEBUG_H_INCLUDED*/

