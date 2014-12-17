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

#include "ice_device.h"

#define DBG_FN	DBG_STDOUT
//DBG_NONE
#define DBG_SMD	DBG_STDOUT
#define DBG_GDL	DBG_STDOUT
//DBG_SYSLOG

//#define PRINT_AV_ACTION
#define FREE_DEV_AT_STARTUP
//#define FREE_CLOCK_AT_INIT
#define USE_CLOCK_SYNC

#define MAX_FOLLOW_WAIT_COUNT 8

#define MAX_HOLD_TIME_NUM 1
#define MAX_HOLD_TIME_DEN 1
#define INITIAL_VIDEO_DIFF 90000
#define INITIAL_AUDIO_DIFF 90000

#define DRIFT_AHEAD 1
#define DRIFT_BEHIND 200

#define IS_IPB_SPEED(x) ((x>0) && (x<=ISMD_NORMAL_PLAY_RATE*4))
#define IS_FORWARD_SPEED(x) (x>=0)

#define PORT_WRITE_RETRY_COUNT 100
#define PORT_WRITE_RETRY_SLEEP 10

#define CHUNK_SIZE(x) ( \
	(ICE_QUEUE_AUDIO==x) ? 8192  : \
	(ICE_QUEUE_VIDEO==x) ? 32768 : \
	(ICE_QUEUE_PIP  ==x) ? 32768 : \
	-1)

#define DEFAULT_AUDIO_CODEC ISMD_AUDIO_MEDIA_FMT_MPEG
#define DEFAULT_VIDEO_CODEC ISMD_CODEC_TYPE_MPEG2

#define ICE_DEFAULT_PIP_WIDTH  190
#define ICE_DEFAULT_PIP_HEIGHT 150

#ifdef PRINT_AV_ACTION
const char action_marker[]="-\\|/";
int action_video_data=0;
int action_video=0;
int action_audio_data=0;
int action_audio=0;
ismd_vidrend_stream_position_info_t g_vinfo;
ismd_audio_stream_position_info_t g_ainfo;
ismd_time_t g_rnd_time, g_time;
#define PRINT_ACTION { printf("\rV %c %c A %c %c                                            \r", action_marker[action_video%4], action_marker[action_video_data%4], action_marker[action_audio%4], action_marker[action_audio_data%4]); fflush(stdout); }
#endif

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////

static ismd_pts_t pts_val(uchar Check, const uchar *Data) {
	if((Data[0]>>4) != Check) {
		isyslog("pts_val: Invalid pts check %02x %02x %02x %02x %02x - %x", Data[0], Data[1], Data[2], Data[3], Data[4], Check);
		return ISMD_NO_PTS;
	} // if
	if(!(Data[0]&0x1) || !(Data[2]&0x1) || !(Data[4]&0x1)) {
		isyslog("pts_val: Invalid field check %02x %02x %02x %02x %02x", Data[0], Data[1], Data[2], Data[3], Data[4]);
		return ISMD_NO_PTS;
	} // if
	ismd_pts_t ret;
	ret =  (((ismd_pts_t)Data[0]) & 0x0E)<<(30-1);
	ret += (((ismd_pts_t)Data[1]) & 0xFF)<<22;
	ret += (((ismd_pts_t)Data[2]) & 0xFE)<<(15-1);
	ret += (((ismd_pts_t)Data[3]) & 0xFF)<<7;
	ret += (((ismd_pts_t)Data[4]) & 0xFE)>>1;
	return ret;
} // pts_val

typedef union {
	unsigned char attributes[256];
	ismd_es_buf_attr_t buf_attr;
} __attribute__((__may_alias__)) ismd_es_buf_attr_mapper_t;

static int write_chunk(ismd_port_handle_t Port, int ChunkSize, const uchar *Data, int Size, ismd_es_buf_attr_t *Attr) {
	ismd_result_t ret = ISMD_SUCCESS;
	ismd_buffer_handle_t buffer = ISMD_BUFFER_HANDLE_INVALID;
	ismd_buffer_descriptor_t buffer_desc;
	void *buff_ptr = NULL;
	ismd_es_buf_attr_mapper_t *buff_attr = NULL;
	SMDCHECK_IGN(ISMD_ERROR_NO_RESOURCES, ismd_buffer_alloc(ChunkSize, &buffer));
	if(ISMD_SUCCESS != ret) goto error;
	SMDCHECK(ismd_buffer_read_desc(buffer, &buffer_desc));
	if(ISMD_SUCCESS != ret) goto error;
	buff_ptr = OS_MAP_IO_TO_MEM_CACHE(buffer_desc.phys.base, buffer_desc.phys.size);
	if(!buff_ptr) goto error;
	buffer_desc.phys.level = (Size > buffer_desc.phys.size) ? buffer_desc.phys.size : Size;
	memcpy(buff_ptr, Data, buffer_desc.phys.level);
	OS_UNMAP_IO_FROM_MEM(buff_ptr, buffer_desc.phys.size);
#if 1
	buff_attr = (ismd_es_buf_attr_mapper_t *)&buffer_desc.attributes;
	if(!Attr) {
		buff_attr->buf_attr.local_pts      = ISMD_NO_PTS;
		buff_attr->buf_attr.original_pts   = ISMD_NO_PTS;
		buff_attr->buf_attr.original_dts   = ISMD_NO_DTS;
		buff_attr->buf_attr.discontinuity  = false;
		buff_attr->buf_attr.pvt_data_valid = false;
	} else
		buff_attr->buf_attr = *Attr;
#else
	if(!Attr) {
		((ismd_es_buf_attr_t *)buffer_desc.attributes)->local_pts      = ISMD_NO_PTS;
		((ismd_es_buf_attr_t *)buffer_desc.attributes)->original_pts   = ISMD_NO_PTS;
		((ismd_es_buf_attr_t *)buffer_desc.attributes)->original_dts   = ISMD_NO_DTS;
		((ismd_es_buf_attr_t *)buffer_desc.attributes)->discontinuity  = false;
		((ismd_es_buf_attr_t *)buffer_desc.attributes)->pvt_data_valid = false;
	} else
		*((ismd_es_buf_attr_t *)buffer_desc.attributes) = *Attr;
#endif
	SMDCHECK(ismd_buffer_update_desc(buffer, &buffer_desc));
	if(ISMD_SUCCESS != ret) goto error;
	SMDCHECK_IGN(ISMD_ERROR_NO_SPACE_AVAILABLE, ismd_port_write(Port, buffer));
	if(ISMD_SUCCESS != ret) goto error;
	return buffer_desc.phys.level;
error:
	if(NULL != buff_ptr) OS_UNMAP_IO_FROM_MEM(buff_ptr, buffer_desc.phys.size);
	if(ISMD_BUFFER_HANDLE_INVALID != buffer) SMDCHECK(ismd_buffer_dereference(buffer));
	return 0;
} // write_chunk

///////////////////////////////////////////////////////////////////////////////
// cICEDeviceSMD
///////////////////////////////////////////////////////////////////////////////

#ifdef FREE_CLOCK_AT_INIT
#define MAX_ISMD_CLOCKS 128
static bool ismd_clocks[MAX_ISMD_CLOCKS];

void InitIsmdClocks() {
	for(int i=0;i<MAX_ISMD_CLOCKS;i++)
		ismd_clocks[i] = true;
} // InitIsmdClocks

void RegisterIsmdClocks() {
	ismd_time_t t;
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>VALID clocks:");
	for(int i=0;i<MAX_ISMD_CLOCKS;i++) {
		if(ISMD_SUCCESS==ismd_clock_get_time(i, &t)) {
			printf(" %d", i);
			ismd_clocks[i] = true;
		} else ismd_clocks[i] = false;
	} // for
	printf("\n");
} // RegisterIsmdClocks

void FreeIsmdClocks() {
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>FREE clocks");
	for(int i=0;i<MAX_ISMD_CLOCKS;i++) {
		if(!ismd_clocks[i]) {
			if(ISMD_SUCCESS == ismd_clock_free(i))
				printf(" %d", i);
		}
		ismd_clocks[i] = true;
	} // for
	printf("\n");
} // FreeIsmdClocks
#endif

cICEDeviceSMD::cICEDeviceSMD(cPluginICE *Plugin)
              :cICEDevice(Plugin),
               playRate(ISMD_NORMAL_PLAY_RATE),
               aud_prc(AUDIO_INVALID_HANDLE),
               vid_codec(ISMD_CODEC_TYPE_INVALID),
               pip_codec(ISMD_CODEC_TYPE_INVALID),
               aud_codec(ISMD_AUDIO_MEDIA_FMT_INVALID),
               vid_out(ISMD_PORT_HANDLE_INVALID),
               pip_out(ISMD_PORT_HANDLE_INVALID),
               ppr_inp(ISMD_PORT_HANDLE_INVALID),
               ppr_out(ISMD_PORT_HANDLE_INVALID),
               rnd_inp(ISMD_PORT_HANDLE_INVALID),
               user_data(ISMD_PORT_HANDLE_INVALID),
               aud(ISMD_DEV_HANDLE_INVALID),
               vid(ISMD_DEV_HANDLE_INVALID),
               ppr(ISMD_DEV_HANDLE_INVALID),
               rnd(ISMD_DEV_HANDLE_INVALID),
               mon(ISMD_CLOCK_HANDLE_INVALID),
               pip(ISMD_DEV_HANDLE_INVALID),
               pip_rnd(ISMD_CLOCK_HANDLE_INVALID),
               pip_ppr(ISMD_DEV_HANDLE_INVALID),
               pip_mon(ISMD_CLOCK_HANDLE_INVALID),
               pip_ppr_inp(ISMD_PORT_HANDLE_INVALID),
               pip_ppr_out(ISMD_PORT_HANDLE_INVALID),
               pip_rnd_inp(ISMD_PORT_HANDLE_INVALID),
               pipe_state(ISMD_DEV_STATE_INVALID),
               base_time(0),
               paused_time(0),
               video_debug_thread(this),
               audio_debug_thread(this),
               clk_sync(ISMD_CLOCK_HANDLE_INVALID),
               clk(ISMD_CLOCK_HANDLE_INVALID),
               pip_clk(ISMD_CLOCK_HANDLE_INVALID),
               audio_underrun_event(ISMD_EVENT_HANDLE_INVALID),
               pes_buffer(NULL),
               pes_buffer_pos(0) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::cICEDeviceSMD");
	for(int i=0; i < EVENT_COUNT; i++)
		event_list[i] = ISMD_EVENT_HANDLE_INVALID;
	currVideoType=currAudioType=-1;
	vid_prop.codec_type=ISMD_CODEC_TYPE_INVALID;
	aud_prop.algo      =ISMD_AUDIO_MEDIA_FMT_INVALID;
#ifdef ICE_TRIGGER_AR_ON_SET_VIDEO_FORMAT
	SkipSetVideoFormat=1; // We expect one initial SetVideoFormat call
#endif
#ifdef FREE_DEV_AT_STARTUP
	ismd_result_t ret = ISMD_SUCCESS;
	for(int i=0;i<1024;i++) {
		SMDCHECK_IGN(ISMD_ERROR_INVALID_HANDLE, ismd_dev_close(i));
//		SMDCHECK_IGN(ISMD_ERROR_INVALID_HANDLE, ismd_clock_free(i));
	} // if
#endif /*FREE_DEV_AT_STARTUP*/
#ifdef FREE_CLOCK_AT_INIT
	InitIsmdClocks();
#endif /*FREE_DEV_AT_STARTUP*/
	CreatePipe();
	Start();
} // cICEDeviceSMD::cICEDeviceSMD

cICEDeviceSMD::~cICEDeviceSMD() {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::~cICEDeviceSMD");
	if(pes_buffer) delete[] pes_buffer;
	DestroyPipe();
} // cICEDeviceSMD::~cICEDeviceSMD

bool cICEDeviceSMD::SetPlayMode(ePlayMode PlayMode) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::SetPlayMode %d %s (was %d %s)", PlayMode, PLAY_MODE_STR(PlayMode), playMode, PLAY_MODE_STR(playMode));
	if(pmExtern_THIS_SHOULD_BE_AVOIDED != playMode) {
		SetPipe(ISMD_DEV_STATE_STOP);
		SetVideoCodec(DEFAULT_VIDEO_CODEC);
		TrickSpeed(0); //playRate = ISMD_NORMAL_PLAY_RATE;
	} // if
	currVideoType=currAudioType=-1;
	vid_prop.codec_type=ISMD_CODEC_TYPE_INVALID;
	aud_prop.algo      =ISMD_AUDIO_MEDIA_FMT_INVALID;
	switch(PlayMode) {
		case pmNone:           // audio/video from decoder
		case pmAudioVideo:     // audio/video from player
		case pmAudioOnly:      // audio only from player, video from decoder
		case pmAudioOnlyBlack: // audio only from player, no video (black screen)
		case pmVideoOnly:      // video only from player, audio from decoder
#ifdef FREE_CLOCK_AT_INIT
			FreeIsmdClocks();
#endif
			if(!CreatePipe()) return false;
			if(plugin && (pmExtern_THIS_SHOULD_BE_AVOIDED==playMode)) plugin->updateVideo(true); // recover from gstreamer settings
//Will be started on data receive			if(!SetPipe(ISMD_DEV_STATE_PLAY)) return false;
			break;
		case pmExtern_THIS_SHOULD_BE_AVOIDED:
			DestroyPipe();
#ifdef FREE_CLOCK_AT_INIT
			RegisterIsmdClocks();
#endif /*FREE_DEV_AT_STARTUP*/
			playMode = PlayMode;
			if(plugin) plugin->updateVideo(false);
			if(plugin) plugin->RemoveOutput(ICE_OUTPUT_HDMI); // prepare for gstreamer
			break;
		default:
			esyslog("cICEDeviceSMD::SetPlayMode invalid PlayMode %d", PlayMode);
			return false;
	} // switch
	return cICEDevice::SetPlayMode(PlayMode);
} // cICEDeviceSMD::SetPlayMode


void cICEDeviceSMD::HandleClockRecover(const uchar *Data, int Length) {
	ismd_result_t ret = ISMD_SUCCESS;
	int pos = 0;
	if((ISMD_CLOCK_HANDLE_INVALID == clk_sync) || (ISMD_CLOCK_HANDLE_INVALID == clk)) return;
	while((pos+TS_SIZE) <= Length){
		if(Data[pos+3] & 0x20) { // Check for adaption field
			if(PatPmtParser()->Ppid() == (((Data[pos+1] & 0x1F)<<8) | Data[pos+2])) {
				if((Data[pos+4] != 0) && (Data[pos+5]&0x10)) {
					ismd_time_t pcr = (((ismd_time_t)(Data[pos+6])) << 25) |
					                                 (Data[pos+7] << 17) |
					                                 (Data[pos+8] << 9) |
					                                 (Data[pos+9] << 1) |
					                                ((Data[pos+10] >> 7) & 0x1);
					ismd_time_t ct;
					SMDCHECK(ismd_clock_get_time(clk, &ct));
					if (ret == ISMD_SUCCESS) SMDCHECK(ismd_clock_sync_add_pair(clk_sync, pcr, ct));
					if (ret == ISMD_SUCCESS) {/*printf("%04x %09llx %09llx (%lld)\n", ((Data[pos+1] & 0x1F)<<8) | Data[pos+2], pcr, ct, pcr-ct);*/return;}
				} // if
			} // if
		} // if
		pos += TS_SIZE;
	} // while
} // cICEDeviceSMD::HandleClockRecover

int cICEDeviceSMD::PlayTsVideo(const uchar *Data, int Length) {
	queue[ICE_QUEUE_VIDEO].CheckTs(Data, Length, PatPmtParser()->Vpid());
	return cDevice::PlayTsVideo(Data, Length);
} // cICEDeviceSMD::PlayTsVideo

int cICEDeviceSMD::PlayTsAudio(const uchar *Data, int Length) {
	queue[ICE_QUEUE_AUDIO].CheckTs(Data, Length);
	return cDevice::PlayTsAudio(Data, Length);
} // cICEDeviceSMD::PlayTsAudio

int cICEDeviceSMD::PlayTs(const uchar *Data, int Length, bool VideoOnly) {
	if(NULL == Data) queue[ICE_QUEUE_VIDEO].new_seg = queue[ICE_QUEUE_AUDIO].new_seg = true;
	int ret = cDevice::PlayTs(Data, Length, VideoOnly);
	if(ret >= TS_SIZE) HandleClockRecover(Data, Length);
	return ret;
} // cICEDeviceSMD::PlayTs

int cICEDeviceSMD::PlayVideo(const uchar *Data, int Length) {
//	ICE_DBG(DBG_FN,"cICEDeviceSMD::PlayVideo %d", Length);
	if(PatPmtParser()->Vtype() != currVideoType) {
		ismd_codec_type_t codec = ISMD_CODEC_TYPE_INVALID;
		switch (PatPmtParser()->Vtype()) {
			case    0: //Default PlayVideo (without TS/PatPmtParser)
			case 0x01: //MPEG1
			case 0x02: //MPEG2
				codec = ISMD_CODEC_TYPE_MPEG2; 
				break;
			case 0x1B: //H264
				codec = ISMD_CODEC_TYPE_H264;
				break;
			case 0xEA: //VC1
				codec = ISMD_CODEC_TYPE_VC1;
				break;
			default:
				isyslog("Invalid video type 0x%x", PatPmtParser()->Vtype());
				return Length;
		} // switch
		if(!SetVideoCodec(codec)) return Length;
		currVideoType = PatPmtParser()->Vtype();
	} // if
	lastVideo = ::time(0);
	if(!PatPmtParser()->Vtype()) {
		// merge pes packets to bigger ones (old remux created ~2k pes packets)
		if(!pes_buffer) {
			pes_buffer = new uchar[CHUNK_SIZE(ICE_QUEUE_VIDEO)];
			pes_buffer_pos=0;
		} // if
#ifdef PRINT_AV_ACTION
		action_video++;
#endif
		if((pes_buffer_pos + Length > CHUNK_SIZE(ICE_QUEUE_VIDEO)) || (pes_buffer_pos && PesHasPts(Data)) || !Length) {
			int ret = WriteESData(ICE_QUEUE_VIDEO, pes_buffer_pos, pes_buffer);
#ifdef PRINT_AV_ACTION
			if(ret>0) action_video_data++;
			PRINT_ACTION;
#endif
			if(ret<=0) return 0;
			if(ret != pes_buffer_pos) printf("ERROR: cICEDeviceSMD::PlayVideo: Not all data written! %d / %d - dropping data...\n", ret, pes_buffer_pos);
			pes_buffer_pos = 0;
		} // if
		if(!Length) return 0;
		if(Length > 8) {
			if((Data[0]==0) && (Data[1]==0) && (Data[2]==1)) {
				int data_start = pes_buffer_pos ? 9 + Data[8] : 0;
				if(Length>data_start) {
					memcpy(&pes_buffer[pes_buffer_pos], &Data[data_start], Length-data_start);
					pes_buffer_pos += Length - data_start;
				} else printf("ERROR: cICEDeviceSMD::PlayVideo: No data for complete pes header! %d / %d - dropping data...\n", Length, data_start);
			} else printf("ERROR: cICEDeviceSMD::PlayVideo: Invalid pes header start code %02x %02x %02x - dropping data...\n", Data[0], Data[1], Data[2]);
		} else printf("ERROR: cICEDeviceSMD::PlayVideo: No data for pes header! %d - dropping data...\n", Length);
		return Length;
	} // if
	int ret = WriteESData(ICE_QUEUE_VIDEO, Length, Data);
#ifdef PRINT_AV_ACTION
	if(ret>0) action_video_data++;
	PRINT_ACTION;
	action_video++;
#endif
	return ret;
} // cICEDeviceSMD::PlayVideo

int cICEDeviceSMD::PlayAudio(const uchar *Data, int Length, uchar Id) {
//	ICE_DBG(DBG_FN,"cICEDeviceSMD::PlayAudio %d 0x%02x", Length, Id);
	if(Id!=currAudioType) {
		ismd_audio_format_t codec = ISMD_AUDIO_MEDIA_FMT_INVALID;
		switch(Id) {
			case 0xC0 ... 0xDF: {
				if(Length<9) return Length; // Not a complete pes packet
				int pos = 9+Data[8];
				while(pos < (Length-1)) {
					if((Data[pos] == 0xFF) && ((Data[pos+1] & 0xF0) == 0xF0)) {
						codec = (Data[pos+1] & 0x08) ? ISMD_AUDIO_MEDIA_FMT_MPEG : ISMD_AUDIO_MEDIA_FMT_AAC;
						break;
					} // if
					pos++;
				} // while
				if(ISMD_AUDIO_MEDIA_FMT_INVALID == codec) return Length; // Not detected. Try next pes packet...
				break;
			} // case
			case 0x80: // VDR 1.4 pes
			case 0xBD:
				codec = ISMD_AUDIO_MEDIA_FMT_DD;
				break;
			default:
				isyslog("Invalid stream id 0x%x", Id);
				return Length;
		} // switch
		if(!SetAudioCodec(codec)) return Length;
		currAudioType = Id;
	} // if
	lastAudio = ::time(0);
	int ret = WriteESData(ICE_QUEUE_AUDIO, Length, Data);
#ifdef PRINT_AV_ACTION
	if(ret>0) action_audio_data++;
	PRINT_ACTION;
	action_audio++;
#endif
	return ret;
} // cICEDeviceSMD::PlayAudio

int64_t cICEDeviceSMD::GetSTC(void) {
//	ICE_DBG(DBG_FN,"cICEDeviceSMD::GetSTC");
	ismd_result_t ret = ISMD_SUCCESS;
//	PRINT_QUEUE_STATUS(">",this);
	if(IS_IPB_SPEED(playRate)) {
		ismd_audio_stream_position_info_t ainfo;
		ainfo.segment_time=ISMD_NO_PTS;
		if(ISMD_DEV_HANDLE_INVALID!=aud) SMDCHECK_IGN(ISMD_ERROR_NO_DATA_AVAILABLE, ismd_audio_input_get_stream_position(aud_prc, aud, &ainfo));
		if((ISMD_SUCCESS==ret) && (ISMD_NO_PTS!=ainfo.segment_time)) return ainfo.segment_time;
	} // if
	ismd_vidrend_stream_position_info_t vinfo;
	vinfo.segment_time=ISMD_NO_PTS;
	if(ISMD_DEV_HANDLE_INVALID!=rnd) SMDCHECK_IGN(ISMD_ERROR_NO_DATA_AVAILABLE, ismd_vidrend_get_stream_position(rnd, &vinfo));
	if((ISMD_SUCCESS==ret) && (ISMD_NO_PTS!=vinfo.segment_time)) return vinfo.segment_time;
	if(IS_IPB_SPEED(playRate)) {
		if(ISMD_NO_PTS != queue[ICE_QUEUE_AUDIO].last_pts) return queue[ICE_QUEUE_AUDIO].last_pts;
	} // if
	if(ISMD_NO_PTS != queue[ICE_QUEUE_VIDEO].last_pts) return queue[ICE_QUEUE_VIDEO].last_pts;
	return cDevice::GetSTC();
} // cICEDeviceSMD::GetSTC

void cICEDeviceSMD::SetVideoDisplayFormat(eVideoDisplayFormat VideoDisplayFormat) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::SetVideoDisplayFormat %d %s", VideoDisplayFormat, VIDEO_DISPLAY_FORMAT_STR(VideoDisplayFormat));
	cDevice::SetVideoDisplayFormat(VideoDisplayFormat);
} // cICEDeviceSMD::SetVideoDisplayFormat

void cICEDeviceSMD::SetVideoFormat(bool VideoFormat16_9) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::SetVideoFormat %d", VideoFormat16_9);
#ifdef ICE_TRIGGER_AR_ON_SET_VIDEO_FORMAT
	// Ignore function description!!!
	if(SkipSetVideoFormat > 0) {
		SkipSetVideoFormat--;
		return;
	} // if
	if(!plugin) return;
	plugin->options.scale_mode = cICESetup::GetNextScaleMode(plugin->options.scale_mode);
	UpdateOptions();
	Skins.Message(mtInfo, tr(SCALE_MODE_STR(plugin->options.scale_mode)));
#endif //ICE_TRIGGER_AR_ON_SET_VIDEO_FORMAT
} // cICEDeviceSMD::SetVideoFormat

eVideoSystem cICEDeviceSMD::GetVideoSystem(void) {
//	ICE_DBG(DBG_FN,"cICEDeviceSMD::GetVideoSystem");
	if(plugin) return plugin->options.video_system;
	return cDevice::GetVideoSystem();
} // cICEDeviceSMD::GetVideoSystem

void cICEDeviceSMD::GetVideoSize(int &Width, int &Height, double &VideoAspect) {
//	ICE_DBG(DBG_FN,"cICEDeviceSMD::GetVideoSize");
	if(pmExtern_THIS_SHOULD_BE_AVOIDED == playMode) {
		Width  = 1920;
		Height = 1080;
		VideoAspect = 1.0;
	} else if(ISMD_CODEC_TYPE_INVALID!=vid_prop.codec_type) {
		Width  = vid_prop.coded_width;
		Height = vid_prop.coded_height;
		VideoAspect = ((double)vid_prop.sample_aspect_ratio.numerator) / vid_prop.sample_aspect_ratio.denominator;
	} else
		cDevice::GetVideoSize(Width, Height, VideoAspect);
} // cICEDeviceSMD::GetVideoSize

void cICEDeviceSMD::GetOsdSize(int &Width, int &Height, double &PixelAspect) {
//	ICE_DBG(DBG_FN,"cICEDeviceSMD::GetOsdSize");
	if(plugin) {
		gdl_uint32 width, height;
		plugin->GetOsdSize(width, height);
		Width       = width;
		Height      = height;
		PixelAspect = 1.0;
		return;
	} // if
	cDevice::GetOsdSize(Width, Height, PixelAspect);
} // cICEDeviceSMD::GetOsdSize

void cICEDeviceSMD::StillPicture(const uchar *Data, int Length) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::StillPicture %d", Length);
	if((Length > 1) && (Data[0] == 0x47)) {
		cDevice::StillPicture(Data, Length);
	} else if((Length>9) && (Data[0] == 0) && (Data[1] == 0) && (Data[2] == 1) && (Data[3] >= 0xE0) && (Data[3] <= 0xEF)) {
		uchar id = Data[3];
		uchar *p = (uchar *)malloc(Length);
		if(p) {
			memcpy(p, Data, Length);
			for(int i=0; i<Length-9; i++) {
				if((p[i]==0) && (p[i+1]==0) && (p[i+2]==1) && (p[i+3]==id)) {
					int hs = p[i+8]+9;
					memcpy(&p[i], &p[i+hs], Length-i-hs);
					Length -= hs;
				} // if
			} // for
			StillPicture(p, Length);
			free(p);
		} else esyslog("cICEDeviceSMD::StillPicture PES: out of memory %d", Length);
	} else if(Length > 0) {
//		ismd_result_t ret = ISMD_SUCCESS;
		if(playRate) SetPlayRate(0);
		for(int i=0; i<Length-4; i++) {
			if(!Data[0] && !Data[1] && (Data[2]==1) && (Data[3]==0xB3)) // MPEG2 sequence header
				SetVideoCodec(ISMD_CODEC_TYPE_MPEG2);
			//TODO: Detect other formats?
		} // for
		for(int i=0; i<12; i++) // write data multiple times to evict frame
			WriteESData(ICE_QUEUE_VIDEO, Length, Data, (ismd_pts_t)GetSTC());
//		SMDCHECK(ismd_viddec_evict_frame(vid)); // Destroys decoder
		lastBgPic = ::time(0);
	} // if
} // cICEDeviceSMD::StillPicture

bool cICEDeviceSMD::Poll(cPoller &Poller, int TimeoutMs) {
//	ICE_DBG(DBG_FN,"cICEDeviceSMD::Poll %d", TimeoutMs);
	ismd_result_t ret = ISMD_SUCCESS;
	ismd_event_t trigger = ISMD_EVENT_HANDLE_INVALID;
	ismd_event_t events[2];
	int event_count=0;
	switch(playMode) { // Look at playMode since Poll should only be called by player
		case pmAudioVideo:
			if(ISMD_EVENT_HANDLE_INVALID!=queue[ICE_QUEUE_VIDEO].event) events[event_count++] = queue[ICE_QUEUE_VIDEO].event;
			if(ISMD_EVENT_HANDLE_INVALID!=queue[ICE_QUEUE_AUDIO].event) events[event_count++] = queue[ICE_QUEUE_AUDIO].event;
			break;
		case pmAudioOnly:      // audio only from player, video from decoder
		case pmAudioOnlyBlack: // audio only from player, no video (black screen)
			if(ISMD_EVENT_HANDLE_INVALID!=queue[ICE_QUEUE_AUDIO].event) events[event_count++] = queue[ICE_QUEUE_AUDIO].event;
			break;
		case pmVideoOnly:
			if(ISMD_EVENT_HANDLE_INVALID!=queue[ICE_QUEUE_VIDEO].event) events[event_count++] = queue[ICE_QUEUE_VIDEO].event;
			break;
		case pmNone:
			break;
		default: // Should not happen
			cCondWait::SleepMs(1); // Keeps cpu load low
			return false;
	} // switch
	if(!event_count) { // Not working with events or not yet initialized
		cCondWait::SleepMs(1); // Keeps cpu load low
		return true;
	} // if
#ifdef WAIT_FOR_SINGLE_EVENT
	SMDCHECK_IGN(ISMD_ERROR_TIMEOUT, ismd_event_wait_multiple(events, event_count, TimeoutMs, &trigger));
	if(ISMD_SUCCESS==ret) {
		SMDCHECK(ismd_event_acknowledge(trigger));
	} else if(ISMD_ERROR_TIMEOUT==ret) {
		return false;
	} else {
		cCondWait::SleepMs(1); // Keeps cpu load low (with gdb ISMD_ERROR_NOT_DONE)
	}
#else
	for(int i=0; i<event_count; i++) {
		SMDCHECK_IGN(ISMD_ERROR_TIMEOUT, ismd_event_wait(events[i], TimeoutMs));
		if(ISMD_SUCCESS==ret) {
			SMDCHECK(ismd_event_acknowledge(events[i]));
		} else if(ISMD_ERROR_TIMEOUT==ret) {
			return false;
		} else cCondWait::SleepMs(1); // Keeps cpu load low (with gdb ISMD_ERROR_NOT_DONE)
	} // for
#endif
	return true;
//	return Poller.Poll(TimeoutMs);
} // cICEDeviceSMD::Poll

bool cICEDeviceSMD::HasIBPTrickSpeed(void) { 
	return IS_IPB_SPEED(playRate);
} // cICEDeviceSMD::HasIBPTrickSpeed

void cICEDeviceSMD::TrickSpeed(int Speed) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::TrickSpeed %d", Speed);
	int rate = ISMD_NORMAL_PLAY_RATE;
	switch(Speed) { // We need a patch to detect forward(speed>0)/backward(speed<0)
		// Fast
		case   6: rate = ISMD_NORMAL_PLAY_RATE* 2; break;
		case   3: rate = ISMD_NORMAL_PLAY_RATE* 4; break;
		case   1: rate = ISMD_NORMAL_PLAY_RATE* 8; break;
		case  -6: rate = ISMD_NORMAL_PLAY_RATE*-2; break;
		case  -3: rate = ISMD_NORMAL_PLAY_RATE*-4; break;
		case  -1: rate = ISMD_NORMAL_PLAY_RATE*-8; break;
		// Slow
		case   8: rate = ISMD_NORMAL_PLAY_RATE/ 8; break;
		case   4: rate = ISMD_NORMAL_PLAY_RATE/ 4; break;
		case   2: rate = ISMD_NORMAL_PLAY_RATE/ 2; break;
		case -63: rate = ISMD_NORMAL_PLAY_RATE/-8; break;
		case -48: rate = ISMD_NORMAL_PLAY_RATE/-4; break;
		case -24: rate = ISMD_NORMAL_PLAY_RATE/-2; break;
		//Normal
		case   0: rate = ISMD_NORMAL_PLAY_RATE; break;
		default : esyslog("cICEDeviceSMD::TrickSpeed: Invalid speed %d", Speed); return;
	} // switch
	SetPlayRate(rate);
} // cICEDeviceSMD::TrickSpeed

void cICEDeviceSMD::Clear(void) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::Clear (%s%s)", DEV_STATE_STR(pipe_state), IS_IPB_SPEED(playRate)?" IPB": " I only");
	cDevice::Clear();
#if 0
	SetPipe(ISMD_DEV_STATE_STOP);
	SetPipe(ISMD_DEV_STATE_PLAY);
#else
	if(!playRate) {
		ismd_result_t ret = ISMD_SUCCESS;
		if(ISMD_DEV_HANDLE_INVALID != aud) SMDCHECK(ismd_dev_flush(aud));
	} else if(IS_IPB_SPEED(playRate)) {
		SetPipe(ISMD_DEV_STATE_STOP);
		SetPipe(ISMD_DEV_STATE_PLAY);
	} else {
		ismd_result_t ret = ISMD_SUCCESS;
		if(ISMD_DEV_HANDLE_INVALID != vid) SMDCHECK(ismd_dev_flush(vid));
	} // if
#endif
} //  cICEDeviceSMD::Clear

void cICEDeviceSMD::Play(void) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::Play");
	cDevice::Play();
	if((ISMD_DEV_STATE_PAUSE==pipe_state) && (ISMD_NORMAL_PLAY_RATE==playRate))
		SetPipe(ISMD_DEV_STATE_PLAY);
	else TrickSpeed(0);
} // cICEDeviceSMD::Play

void cICEDeviceSMD::Freeze(void) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::Freeze");
	cDevice::Freeze();
	SetPipe(ISMD_DEV_STATE_PAUSE);
} // cICEDeviceSMD::Freeze

bool cICEDeviceSMD::Flush(int TimeoutMs) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::Flush %d", TimeoutMs);
	return true;
} // cICEDeviceSMD::Flush

int cICEDeviceSMD::GetAudioChannelDevice(void) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::GetAudioChannelDevice");
	return cDevice::GetAudioChannelDevice();
} // cICEDeviceSMD::GetAudioChannelDevice

void cICEDeviceSMD::SetAudioChannelDevice(int AudioChannel) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::SetAudioChannelDevice %d", AudioChannel);
} // cICEDeviceSMD::SetAudioChannelDevice

void cICEDeviceSMD::SetVolumeDevice(int Volume) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::SetVolumeDevice %d", Volume);
	ismd_result_t ret = ISMD_SUCCESS;
	ismd_audio_processor_t tmp_aud_prc = AUDIO_INVALID_HANDLE;
	if(Volume < 0        ) Volume = 0;
	if(Volume > MAXVOLUME) Volume = MAXVOLUME;
	if(pmExtern_THIS_SHOULD_BE_AVOIDED != playMode) {
		if(AUDIO_INVALID_HANDLE == aud_prc) SMDCHECK(ismd_audio_open_global_processor(&aud_prc));
		tmp_aud_prc = aud_prc;
	} else {
		SMDCHECK(ismd_audio_open_global_processor(&tmp_aud_prc));
	} // if
	if(MAXVOLUME==Volume) {
		SMDCHECK(ismd_audio_disable_master_volume(tmp_aud_prc));
		SMDCHECK(ismd_audio_mute(tmp_aud_prc, false));
	} else if (Volume) {
		ismd_audio_gain_value_t vol = log10(((double)Volume)/MAXVOLUME)*255;
		SMDCHECK(ismd_audio_set_master_volume(tmp_aud_prc, vol));
		SMDCHECK(ismd_audio_mute(tmp_aud_prc, false));
	} else
		SMDCHECK(ismd_audio_mute(tmp_aud_prc, true));
	if(pmExtern_THIS_SHOULD_BE_AVOIDED == playMode) {
		if(AUDIO_INVALID_HANDLE != tmp_aud_prc) SMDCHECK(ismd_audio_close_processor(tmp_aud_prc));
	} // if
	cDevice::SetVolumeDevice(Volume);
} // cICEDeviceSMD::SetVolumeDevice

void cICEDeviceSMD::SetAudioTrackDevice(eTrackType Type) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::SetAudioTrackDevice %d", Type);
	queue[ICE_QUEUE_AUDIO].discont=true;
} // cICEDeviceSMD::SetAudioTrackDevice

void cICEDeviceSMD::SetDigitalAudioDevice(bool On) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::SetDigitalAudioDevice %d", On);
	queue[ICE_QUEUE_AUDIO].discont=true;
//	ismd_result_t ret = ISMD_SUCCESS;
//	if(ISMD_DEV_HANDLE_INVALID != aud) SMDCHECK(ismd_dev_flush(aud));
#if 0
	ismd_result_t ret = ISMD_SUCCESS;
	ismd_audio_output_mode_t new_mode  = On ? ISMD_AUDIO_OUTPUT_PASSTHROUGH : ISMD_AUDIO_OUTPUT_PCM;
	ismd_audio_output_mode_t curr_mode = new_mode;
	SMDCHECK(ismd_audio_output_get_mode(aud_prc, aud_hdmi, &curr_mode));
	if(ISMD_SUCCESS!=ret) return;
	if(new_mode == curr_mode) return;
	SMDCHECK(ismd_audio_output_disable(aud_prc, aud_hdmi));
	SMDCHECK(ismd_audio_output_set_mode(aud_prc, aud_hdmi, new_mode));
	SMDCHECK(ismd_audio_output_enable(aud_prc, aud_hdmi));
	isyslog("Set hdmi output to %s", AUDIO_OUTPUT_STR(new_mode));
#endif
} // cICEDeviceSMD::SetDigitalAudioDevice

typedef union {
	unsigned char attributes[256];
	ismd_frame_attributes_t frame_attr;
} __attribute__((__may_alias__)) ismd_frame_attr_mapper_t;

#if 0
uchar *cICEDeviceSMD::GrabImage(int &Size, bool Jpeg, int Quality, int SizeX, int SizeY) {
	return Plugin ? Plugin->GrabImage(Size, Jpeg, Quality, SizeX, SizeY) : NULL;
	ismd_result_t ret = ISMD_SUCCESS;
	if(ISMD_DEV_HANDLE_INVALID == rnd) return NULL;
	ismd_buffer_handle_t buffer = ISMD_BUFFER_HANDLE_INVALID;
	ismd_buffer_descriptor_t desc;
	ismd_frame_attr_mapper_t *frame;
	SMDCHECK_CLEANUP(ismd_vidrend_get_current_frame(rnd, &buffer));
	SMDCHECK_CLEANUP(ismd_buffer_read_desc(buffer, &desc));
	if(ISMD_BUFFER_TYPE_VIDEO_FRAME != desc.buffer_type) goto cleanup;
	frame = (ismd_frame_attr_mapper_t *)&desc.attributes;
isyslog("cICEDeviceSMD::GrabImage: format %s %dx%d", PIXEL_FORMAT_STR(frame->frame_attr.pixel_format), frame->frame_attr.dest_size.width, frame->frame_attr.dest_size.height);
/*
	buff_ptr = OS_MAP_IO_TO_MEM_CACHE(buffer_desc.phys.base, buffer_desc.phys.size);
	if(!buff_ptr) goto error;
	buffer_desc.phys.level = (Size > buffer_desc.phys.size) ? buffer_desc.phys.size : Size;
	memcpy(buff_ptr, Data, buffer_desc.phys.level);
	OS_UNMAP_IO_FROM_MEM(buff_ptr, buffer_desc.phys.size);
*/
	if(ISMD_BUFFER_HANDLE_INVALID != buffer) SMDCHECK(ismd_buffer_dereference(buffer));
	return NULL;
cleanup:
	if(ISMD_BUFFER_HANDLE_INVALID != buffer) SMDCHECK(ismd_buffer_dereference(buffer));
	return NULL;
} // cICEDeviceSMD::GrabImage
#endif

void cICEDeviceSMD::Action() {
	ismd_result_t ret = ISMD_SUCCESS;
	ismd_event_t trigger = ISMD_EVENT_HANDLE_INVALID;
	while(Running()) {
		CheckBgPic();
		ismd_event_t events[ISMD_EVENT_LIST_MAX];
		int event_count=0;
		for(int i=0; i<EVENT_COUNT; i++) {
			if(ISMD_EVENT_HANDLE_INVALID!=event_list[i]) {
				if(event_count>=ISMD_EVENT_LIST_MAX) {
					isyslog("Monitored events exceeds maximum - \"%s\" not handled!", EVENT_STR(i));
					continue;
				} // if
				events[event_count++]=event_list[i];
			} // if
		} // if
		if(!event_count) {
			cCondWait::SleepMs(100);
			continue;
		} // if
		ret = ismd_event_wait_multiple(events, event_count, 100, &trigger);
		if(ISMD_SUCCESS==ret) {
			SMDCHECK(ismd_event_acknowledge(trigger));
			for(int i=0; i<EVENT_COUNT; i++) {
				if(event_list[i]!=trigger) continue;
				if (BUFMON_OVERRUN == i) {
					PRINT_QUEUE_STATUS("BUFMON_OVERRUN", this);
					Clear();
				} else if (VIDDEC_RESOLUTION_CHANGE == i) {
					ismd_viddec_stream_properties_t prop;
					bool panscan = false;
					SMDCHECK(ismd_viddec_get_stream_properties(vid, &prop));
					if(ISMD_SUCCESS==ret) vid_prop = prop;
					SMDCHECK(ismd_viddec_get_pan_scan_flag(vid, &panscan));
					isyslog("Resolution change %s %ld*%ld%s%.2f (%ld*%ld) %d:%d %s%s", 
						CODEC_TYPE_STR(prop.codec_type), prop.coded_width, prop.coded_height, prop.is_stream_interlaced?"i":"p", double(prop.frame_rate_num)/prop.frame_rate_den, 
						prop.display_width, prop.display_height, prop.sample_aspect_ratio.numerator, prop.sample_aspect_ratio.denominator,
						panscan?"PANSCAN ":"", VIDDEC_STREAM_FORMAT_STR(prop.stream_format));
					//TODO: Set plugin->options.video_system depending on frame rate and call plugin->updateVideo() if changed
					if(plugin && (ICE_TVMODE_SOURCE==plugin->options.tv_mode)) plugin->updateVideo();
				} else if((AUDIO_NOTIFY_SAMPLE_RATE_CHANGE == i) || (AUDIO_NOTIFY_CHANNEL_CONFIG_CHANGE == i)) {
					ismd_audio_stream_info_t prop;
					SMDCHECK_IGN(ISMD_ERROR_NO_DATA_AVAILABLE, ismd_audio_input_get_stream_info(aud_prc, aud, &prop));
					if(ISMD_SUCCESS==ret) aud_prop = prop;
					isyslog("%s br %d sr %d ss %d cc %d cn %d", AUDIO_MEDIA_FMT_STR(prop.algo), prop.bitrate, prop.sample_rate, prop.sample_size, prop.channel_config, prop.channel_count);
					if(plugin) plugin->updateAudio();
				} else if(VIDDEC_USER_DATA == i) {
					ismd_buffer_handle_t buffer = ISMD_BUFFER_HANDLE_INVALID;
					ismd_buffer_descriptor_t buf_desc;
					SMDCHECK_IGN(ISMD_ERROR_INVALID_HANDLE, ismd_port_read(user_data, &buffer));
					if (ISMD_SUCCESS!=ret) continue;
					SMDCHECK(ismd_buffer_read_desc(buffer, &buf_desc));
					if (ISMD_SUCCESS!=ret) continue;
					uint8_t *buf_ptr = (uint8_t *)OS_MAP_IO_TO_MEM_NOCACHE(buf_desc.phys.base,buf_desc.phys.size);
					//here is the spot to do stuff with the data
					//isyslog("Got %d(%d) of user data", buf_desc.phys.level, buf_desc.phys.size);
#if 0
					char data[1024*3+2];
					memset(&data, 0, sizeof(data));
					for(int i=0; (i<buf_desc.phys.level)&&(i*3<(int)sizeof(data)); i++)
						sprintf(&data[i*3], "%02X ", buf_ptr[i]);
					isyslog("Data: %s", data);
#endif
					OS_UNMAP_IO_FROM_MEM(buf_ptr, buf_desc.phys.size);
					//need to dereference this buffer as SMD driver created reference when it made the buffer so it can be freed
					SMDCHECK(ismd_buffer_dereference(buffer));
				} else if(AUDIO_NOTIFY_RENDER_UNDERRUN == i) {
#if 0
					if(ISMD_EVENT_HANDLE_INVALID != audio_underrun_event) {
						ismd_port_status_t astat;
						if((ISMD_SUCCESS==ismd_port_get_status(queue[ICE_QUEUE_AUDIO].port, &astat)) && !astat.cur_depth) // If queue is empty handle renderer underrun (silence) as underrun (Fix for PC25.0)
							SMDCHECK(ismd_event_strobe(audio_underrun_event));
					} // if
#else
					if(IS_IPB_SPEED(playRate)) {
						ismd_port_status_t astat, vstat;
						if((ISMD_SUCCESS==ismd_port_get_status(queue[ICE_QUEUE_AUDIO].port, &astat)) && !astat.cur_depth) {
							if ((ISMD_SUCCESS!=ismd_port_get_status(queue[ICE_QUEUE_VIDEO].port, &vstat)) || (vstat.cur_depth == vstat.max_depth)) {
								PRINT_QUEUE_STATUS("AUDIO_NOTIFY_RENDER_UNDERRUN", this);
								Clear();
							} // if
						} // if
					} // if
#endif
				} else isyslog("GOT EVENT %s", EVENT_STR(i));
				break;
			} // for
		} else if(ISMD_ERROR_TIMEOUT!=ret)
			cCondWait::SleepMs(100);
//		PRINT_VSTREAM_INFO(">", this);
	} // while
} // cICEDeviceSMD::Action

int cICEDeviceSMD::WriteESData(tICE_Queue Queue, int Size, const uchar *Data, ismd_pts_t stc) {
	cMutexLock lock(&writeLock);
//	ICE_DBG(DBG_FN,"cICEDeviceSMD::WriteESData %d %d", Port, Size);
	if(ISMD_PORT_HANDLE_INVALID==queue[Queue].port) return Size;
	ismd_result_t ret = ISMD_SUCCESS;
	ismd_port_status_t status;
	SMDCHECK(ismd_port_get_status(queue[Queue].port, &status));
	if((ISMD_SUCCESS != ret) || (status.cur_depth && (status.cur_depth + (Size / CHUNK_SIZE(Queue)) >= status.max_depth))) {
		cCondWait::SleepMs(10); // Don call port_get_status too often
		return 0; // We don't have enough buffers
	} // if
	int written = 0;
	ismd_es_buf_attr_t attr;
	attr.local_pts = attr.original_pts = ISMD_NO_PTS;
	attr.original_dts = ISMD_NO_DTS;
	attr.discontinuity = attr.pvt_data_valid = attr.au_start = false;
	attr.discontinuity = queue[Queue].discont;
	if(attr.discontinuity) printf("Discontinuity detected for %s\n", QUEUE_STR(Queue));
	queue[Queue].discont = false;
	ismd_es_buf_attr_t *attr_ptr = &attr;
	ismd_time_t last_pts = queue[Queue].last_pts;
	if((Size >= 9) && !Data[0] && !Data[1] && (Data[2]==1)) { // We have to strip pes header
		switch(Data[3]) {
			case 0xBD:
			case 0xC0 ... 0xDF:
			case 0xE0 ... 0xEF: {
				switch(Data[7]&0xC0) {
					case 0x80:
						if(Size >= 9+5) {
							if(Data[8]>=5) {
								queue[Queue].last_pts = attr.local_pts = attr.original_pts = pts_val(0x2, &Data[9]);
							} else
								isyslog("PES header to small %02x-%02x %d %d", Data[3], Data[7], Data[8], Size);
						} else
							isyslog("PES to small %02x %d", Data[3], Size);
						break;
					case 0xC0:
						if(Size >= 9+5) {
							if(Data[8]>=10) {
								queue[Queue].last_pts = attr.local_pts = attr.original_pts = pts_val(0x3, &Data[9]);
								attr.original_dts = pts_val(0x1, &Data[9+5]);
							} else
								isyslog("PES header to small %02x-%02x %d %d", Data[3], Data[7], Data[8], Size);
						} else
							isyslog("PES to small %02x %d", Data[3], Size);
						break;
					default:
						break;
				} // switch
				written = Data[8]+9;
				break;
			} // case
		} // switch
	} // if
	if(ISMD_NO_PTS != stc) attr.local_pts = attr.original_pts = stc;
	if(!playRate) queue[ICE_QUEUE_VIDEO].new_seg = queue[ICE_QUEUE_AUDIO].new_seg = false;
//if(queue[ICE_QUEUE_VIDEO].follow) {printf("Wait follow VIDEO %s %d                    \r", QUEUE_STR(Queue), queue[ICE_QUEUE_VIDEO].follow);}
//if(queue[ICE_QUEUE_AUDIO].follow) {printf("Wait follow AUDIO %s %d                    \r", QUEUE_STR(Queue), queue[ICE_QUEUE_AUDIO].follow);}
	if(ISMD_NO_PTS != last_pts && ISMD_NO_PTS != queue[Queue].last_pts && ISMD_NO_PTS != queue[Queue].base_pts) {
		int64_t last_delta = queue[Queue].last_pts - last_pts;
		if((last_delta > 90000) || (last_delta < -90000)) {
			ismd_time_t position = last_pts - queue[Queue].base_pts + queue[Queue].lin_start;
			isyslog("Handle %s pts jump (%lld) %s -> %lld\n", QUEUE_STR(Queue), last_delta, queue[Queue].follow ? "follow" : "set", queue[Queue].follow ? queue[Queue].lin_start : position);
			switch(Queue) {
				case ICE_QUEUE_VIDEO:
				case ICE_QUEUE_AUDIO:
					if(!queue[Queue].follow) {
						queue[ICE_QUEUE_VIDEO].lin_start = queue[ICE_QUEUE_AUDIO].lin_start = position;
						queue[ICE_QUEUE_VIDEO].base_pts  = queue[ICE_QUEUE_AUDIO].base_pts  = attr.original_pts;
						queue[ICE_QUEUE_VIDEO].follow    = queue[ICE_QUEUE_AUDIO].follow    = MAX_FOLLOW_WAIT_COUNT;
						queue[ICE_QUEUE_VIDEO].new_seg   = queue[ICE_QUEUE_AUDIO].new_seg   = false;
					} // if
					break;
				default:
					queue[Queue].lin_start = position;
					queue[Queue].base_pts  = attr.original_pts;
					queue[Queue].new_seg   = false;
			} // switch
			queue[Queue].write_seg = true;
			queue[Queue].follow    = 0;
		} else if (last_delta && queue[Queue].follow > 2) {
			queue[Queue].follow--;
		} else if (last_delta && queue[Queue].follow == 2) {
			queue[Queue].follow = 1;
			queue[Queue].write_seg = true; // Still wait for pts diff but force writinbg new seg
		} // if
	} // if
	if(queue[Queue].new_seg && (ISMD_NO_PTS != attr.original_pts)) {
		isyslog("Set new %s segment %09llx", QUEUE_STR(Queue), attr.original_pts);
		if(ICE_QUEUE_VIDEO==Queue) {
#if INITIAL_VIDEO_DIFF
			int pts_diff=0;
			if(ISMD_NORMAL_PLAY_RATE==playRate) {
				queue[Queue].new_seg = false;
				return 0;//Size; // Wait for initial video
			}
			queue[ICE_QUEUE_VIDEO].base_pts  = queue[ICE_QUEUE_AUDIO].base_pts  = attr.original_pts-pts_diff;
			queue[ICE_QUEUE_VIDEO].lin_start = queue[ICE_QUEUE_AUDIO].lin_start = 0;
			queue[ICE_QUEUE_VIDEO].write_seg = queue[ICE_QUEUE_AUDIO].write_seg = true;
			queue[ICE_QUEUE_VIDEO].new_seg   = queue[ICE_QUEUE_AUDIO].new_seg   = false;
#else
			queue[ICE_QUEUE_VIDEO].base_pts  = queue[ICE_QUEUE_AUDIO].base_pts  = attr.original_pts;
			queue[ICE_QUEUE_VIDEO].lin_start = queue[ICE_QUEUE_AUDIO].lin_start = 0;
			queue[ICE_QUEUE_VIDEO].write_seg = queue[ICE_QUEUE_AUDIO].write_seg = true;
			queue[ICE_QUEUE_VIDEO].new_seg   = false;
#endif
		} else if(ICE_QUEUE_AUDIO==Queue) {
			if(ISMD_NORMAL_PLAY_RATE==playRate) {
#if INITIAL_AUDIO_DIFF
				int pts_diff=0;
				if(ISMD_NORMAL_PLAY_RATE==playRate) pts_diff=INITIAL_AUDIO_DIFF;
				queue[ICE_QUEUE_VIDEO].base_pts  = queue[ICE_QUEUE_AUDIO].base_pts  = attr.original_pts-pts_diff;
				queue[ICE_QUEUE_VIDEO].lin_start = queue[ICE_QUEUE_AUDIO].lin_start = 0;
				queue[ICE_QUEUE_VIDEO].write_seg = queue[ICE_QUEUE_AUDIO].write_seg = true;
				queue[ICE_QUEUE_VIDEO].new_seg   = queue[ICE_QUEUE_AUDIO].new_seg   = false;
#else
				queue[ICE_QUEUE_VIDEO].base_pts  = queue[ICE_QUEUE_AUDIO].base_pts  = attr.original_pts;
				queue[ICE_QUEUE_VIDEO].lin_start = queue[ICE_QUEUE_AUDIO].lin_start = 0;
				queue[ICE_QUEUE_VIDEO].write_seg = queue[ICE_QUEUE_AUDIO].write_seg = true;
				queue[ICE_QUEUE_VIDEO].new_seg   = queue[ICE_QUEUE_AUDIO].new_seg   = false;
#endif
			} // if
		} else if(WriteNewSeg(Queue, attr.original_pts, ISMD_NO_PTS)) {
			queue[Queue].base_pts  = attr.original_pts;
			queue[Queue].lin_start = 0;
			queue[Queue].write_seg = true;
			queue[Queue].new_seg   = false;
		} // if
	} // if
	if(queue[Queue].write_seg)
		if(WriteNewSeg(Queue, queue[Queue].base_pts, ISMD_NO_PTS, queue[Queue].lin_start))
			queue[Queue].write_seg = false;
	int retry = PORT_WRITE_RETRY_COUNT;
	while(Size > written) {
		int len = write_chunk(queue[Queue].port, CHUNK_SIZE(Queue), &Data[written], Size-written, attr_ptr);
		if(!len) {
			if(attr_ptr) return 0;  // We haven't written something yet
			if(!retry--) break; // Give up
			cCondWait::SleepMs(PORT_WRITE_RETRY_SLEEP);
			continue;
		} // if
		written += len;
		attr_ptr = NULL;
		retry = PORT_WRITE_RETRY_COUNT;
	} // while
	return attr_ptr ? 0 : written;
} // cICEDeviceSMD::WriteESData

bool cICEDeviceSMD::WriteNewSeg(tICE_Queue Queue, ismd_time_t Start, ismd_time_t Stop, ismd_time_t position) {
//	ICE_DBG(DBG_FN,"cICEDeviceSMD::WriteNewSeg %d", Port);
	if(ISMD_PORT_HANDLE_INVALID==queue[Queue].port) return true; // true because there is nothing to set
	ismd_result_t ret = ISMD_SUCCESS;
	ismd_port_status_t status;
	int wait_counter = 100;
	ismd_buffer_handle_t buffer = ISMD_BUFFER_HANDLE_INVALID;
	ismd_newsegment_tag_t newsegment_data;
//	ismd_buffer_descriptor_t buffer_desc;
	SMDCHECK_CLEANUP(ismd_buffer_alloc(0, &buffer));
//	SMDCHECK_CLEANUP(ismd_buffer_read_desc(buffer, &buffer_desc));
	memset(&newsegment_data, 0, sizeof(newsegment_data));
	newsegment_data.linear_start = position;
	newsegment_data.start = Start;
	newsegment_data.stop  = Stop;
	newsegment_data.requested_rate = abs(playRate);
	newsegment_data.applied_rate = ISMD_NORMAL_PLAY_RATE;
	newsegment_data.rate_valid = true;
	newsegment_data.segment_position = ISMD_NO_PTS;//Start;
	SMDCHECK_CLEANUP(ismd_tag_set_newsegment_position(buffer, newsegment_data));
//	SMDCHECK_CLEANUP(ismd_buffer_update_desc(buffer, &buffer_desc));
	SMDCHECK(ismd_port_get_status(queue[Queue].port, &status));
	while((ISMD_SUCCESS==ret) && (status.cur_depth >= status.max_depth)) {
		SMDCHECK(wait_counter--?ISMD_SUCCESS:ISMD_ERROR_TIMEOUT);
		if(ISMD_SUCCESS!=ret) goto cleanup;
		cCondWait::SleepMs(1); // Keeps cpu load low
		SMDCHECK(ismd_port_get_status(queue[Queue].port, &status));
	} // while
	SMDCHECK_CLEANUP(ismd_port_write(queue[Queue].port, buffer));
	return true;
cleanup:
	if(ISMD_BUFFER_HANDLE_INVALID != buffer) SMDCHECK(ismd_buffer_dereference(buffer));
	return false;
} // cICEDeviceSMD::WriteNewSeg

bool cICEDeviceSMD::CreateAudioPipe(ismd_audio_format_t codec) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::CreateAudioPipe %d %s (%s)", codec, AUDIO_MEDIA_FMT_STR(codec), AUDIO_MEDIA_FMT_STR(aud_codec));
	ismd_result_t ret = ISMD_SUCCESS;
	if(AUDIO_INVALID_HANDLE == aud_prc) {
		SMDCHECK_CLEANUP(ismd_audio_open_global_processor(&aud_prc));
		SMDCHECK_CLEANUP(ismd_audio_set_clock_mode(aud_prc, ISMD_AUDIO_CLK_LOCKED/*ISMD_AUDIO_CLK_INDEPENDENT*/));
	} // if
	if(ISMD_DEV_HANDLE_INVALID == aud) {
		SMDCHECK_CLEANUP(ismd_audio_add_input_port(aud_prc, true, &aud, &queue[ICE_QUEUE_AUDIO].port));
		SMDCHECK_CLEANUP(ismd_audio_set_stream_timing_mode(aud, ISMD_TIMING_MODE_CONVERTED/*ISMD_TIMING_MODE_ORIGINAL*/));
		SMDCHECK_CLEANUP(ismd_dev_set_clock(aud, clk));
		queue[ICE_QUEUE_AUDIO].Attach();
		
		if(ISMD_DEV_HANDLE_INVALID != mon) SMDCHECK_CLEANUP(ismd_bufmon_add_renderer(mon, aud, &audio_underrun_event));
		SMDCHECK_CLEANUP(ismd_dev_set_underrun_event(aud, audio_underrun_event));
		SMDCHECK_CLEANUP(ismd_audio_input_set_data_format(aud_prc, aud, codec));
#if defined(DRIFT_AHEAD) && defined(DRIFT_BEHIND)
		SMDCHECK_CLEANUP(ismd_audio_input_set_timing_accuracy(aud_prc, aud, DRIFT_AHEAD, DRIFT_BEHIND));
#endif
		ismd_audio_input_pass_through_config_t aud_cfg;
		memset(&aud_cfg, 0, sizeof(aud_cfg));
		aud_cfg.is_pass_through = true;
		aud_cfg.dts_or_dolby_convert = false;
//		aud_cfg.supported_formats[aud_cfg.supported_format_count++] = ISMD_AUDIO_MEDIA_FMT_PCM;
//		aud_cfg.supported_formats[aud_cfg.supported_format_count++] = ISMD_AUDIO_MEDIA_FMT_DD;
		SMDCHECK(ismd_audio_input_set_as_primary(aud_prc, aud, aud_cfg));
		SMDCHECK_CLEANUP(ismd_audio_input_enable(aud_prc, aud));
//		SMDCHECK_CLEANUP(ismd_dev_set_state(aud, ISMD_DEV_STATE_PAUSE));

//		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_STREAM_BEGIN,          &audio_debug_thread.event_list[AUDIO_NOTIFY_STREAM_BEGIN]));
//		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_STREAM_END,            &audio_debug_thread.event_list[AUDIO_NOTIFY_STREAM_END]));
		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_RENDER_UNDERRUN,       &event_list[AUDIO_NOTIFY_RENDER_UNDERRUN]));
//		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_WATERMARK,             &audio_debug_thread.event_list[AUDIO_NOTIFY_WATERMARK]));
//		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_TAG_RECEIVED,          &audio_debug_thread.event_list[AUDIO_NOTIFY_TAG_RECEIVED]));
		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_SAMPLE_RATE_CHANGE,    &event_list[AUDIO_NOTIFY_SAMPLE_RATE_CHANGE]));
		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_CODEC_CHANGE,          &audio_debug_thread.event_list[AUDIO_NOTIFY_CODEC_CHANGE]));
// FEATURE_NOT_SUPPORTED		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_SAMPLE_SIZE_CHANGE,    &audio_debug_thread.event_list[AUDIO_NOTIFY_SAMPLE_SIZE_CHANGE]));
		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_CHANNEL_CONFIG_CHANGE, &event_list[AUDIO_NOTIFY_CHANNEL_CONFIG_CHANGE]));
		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_PTS_VALUE_EARLY,       &audio_debug_thread.event_list[AUDIO_NOTIFY_PTS_VALUE_EARLY]));
		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_PTS_VALUE_LATE,        &audio_debug_thread.event_list[AUDIO_NOTIFY_PTS_VALUE_LATE]));
		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_PTS_VALUE_RECOVERED,   &audio_debug_thread.event_list[AUDIO_NOTIFY_PTS_VALUE_RECOVERED]));
//		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_CORRUPT_FRAME,         &audio_debug_thread.event_list[AUDIO_NOTIFY_CORRUPT_FRAME]));
//		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_CAPTURE_OVERRUN,              &audio_debug_thread.event_list[AUDIO_CAPTURE_OVERRUN]));
//		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_CLIENT_ID,             &audio_debug_thread.event_list[AUDIO_NOTIFY_CLIENT_ID]));
//		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_SEGMENT_END,           &audio_debug_thread.event_list[AUDIO_NOTIFY_SEGMENT_END]));
//		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_SRC_STATUS_CHANGE,     &audio_debug_thread.event_list[AUDIO_NOTIFY_SRC_STATUS_CHANGE]));
//		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_DECODE_ERROR,          &audio_debug_thread.event_list[AUDIO_NOTIFY_DECODE_ERROR]));
		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_DECODE_SYNC_FOUND,     &audio_debug_thread.event_list[AUDIO_NOTIFY_DECODE_SYNC_FOUND]));
		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_DECODE_SYNC_LOST,      &audio_debug_thread.event_list[AUDIO_NOTIFY_DECODE_SYNC_LOST]));
//		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_INPUT_FULL,            &audio_debug_thread.event_list[AUDIO_NOTIFY_INPUT_FULL]));
//		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_INPUT_EMPTY,           &audio_debug_thread.event_list[AUDIO_NOTIFY_INPUT_EMPTY]));
//		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_INPUT_RECOVERED,       &audio_debug_thread.event_list[AUDIO_NOTIFY_INPUT_RECOVERED]));
//		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_SEGMENT_START,         &audio_debug_thread.event_list[AUDIO_NOTIFY_SEGMENT_START]));
//		SMDCHECK(ismd_audio_input_get_notification_event(aud_prc, aud, ISMD_AUDIO_NOTIFY_DATA_RENDERED,         &audio_debug_thread.event_list[AUDIO_NOTIFY_DATA_RENDERED]));
		if(ISMD_CLOCK_HANDLE_INVALID != clk_sync)SMDCHECK(ismd_audio_input_set_sync_clock(aud_prc, aud, clk_sync));
		aud_codec = codec;
		if(plugin) plugin->updateAudio();
	} // if
	return true;
cleanup:
	DestroyAudioPipe();
	return false;
} //  cICEDeviceSMD::CreateAudioPipe

void cICEDeviceSMD::DestroyAudioPipe() {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::DestroyAudioPipe");
	ismd_result_t ret = ISMD_SUCCESS;
	aud_codec = ISMD_AUDIO_MEDIA_FMT_INVALID;
	queue[ICE_QUEUE_AUDIO] = tICE_QueueData();
	if(ISMD_DEV_HANDLE_INVALID != aud) {
		audio_underrun_event = ISMD_EVENT_HANDLE_INVALID;
		if(ISMD_DEV_HANDLE_INVALID != mon) SMDCHECK(ismd_bufmon_remove_renderer(mon, aud));
		SMDCHECK(ismd_dev_close(aud));
	} // if
	aud = ISMD_DEV_HANDLE_INVALID;
	
//	if((AUDIO_INVALID_HANDLE != aud_hdmi) && (AUDIO_INVALID_HANDLE != aud_prc)) SMDCHECK(ismd_audio_remove_output(aud_prc, aud_hdmi));
//	aud_hdmi = AUDIO_INVALID_HANDLE;
	
	if(AUDIO_INVALID_HANDLE != aud_prc) SMDCHECK(ismd_audio_close_processor(aud_prc));
	aud_prc = AUDIO_INVALID_HANDLE;
} // cICEDeviceSMD::DestroyAudioPipe

bool cICEDeviceSMD::CreateVideoPipe(ismd_codec_type_t codec) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::CreateVideoPipe %d %s", codec, CODEC_TYPE_STR(codec));
	ismd_result_t ret = ISMD_SUCCESS;
	ismd_event_t underrun_event = ISMD_EVENT_HANDLE_INVALID;
	if(ISMD_DEV_HANDLE_INVALID == rnd) {
		SMDCHECK_CLEANUP(ismd_vidrend_open(&rnd));
		SMDCHECK_CLEANUP(ismd_vidrend_get_input_port(rnd, &rnd_inp));
		SMDCHECK_CLEANUP(ismd_dev_set_clock(rnd, clk));
		SMDCHECK_CLEANUP(ismd_vidrend_set_timing_mode(rnd, ISMD_TIMING_MODE_CONVERTED));
		if(ISMD_DEV_HANDLE_INVALID != mon) SMDCHECK_CLEANUP(ismd_bufmon_add_renderer(mon, rnd, &underrun_event));
		SMDCHECK_CLEANUP(ismd_dev_set_underrun_event(rnd, underrun_event));
#if (MAX_HOLD_TIME_NUM > MAX_HOLD_TIME_DEN)
		SMDCHECK_CLEANUP(ismd_vidrend_enable_max_hold_time(rnd, MAX_HOLD_TIME_NUM, MAX_HOLD_TIME_DEN));
#else
		SMDCHECK_CLEANUP(ismd_vidrend_disable_max_hold_time(rnd));
#endif
		SMDCHECK_CLEANUP(ismd_vidrend_disable_fixed_frame_rate(rnd));
		SMDCHECK_CLEANUP(ismd_vidrend_set_video_plane(rnd, ICE_VID_PLANE));
		SMDCHECK_CLEANUP(ismd_vidrend_set_interlaced_display_rate(ISMD_VIDREND_INTERLACED_RATE_50));
		SMDCHECK_CLEANUP(ismd_vidrend_set_flush_policy(rnd, ISMD_VIDREND_FLUSH_POLICY_DISPLAY_BLACK));
		SMDCHECK_CLEANUP(ismd_vidrend_set_stop_policy(rnd, ISMD_VIDREND_STOP_POLICY_DISPLAY_BLACK));
		SMDCHECK(ismd_vidrend_get_resolution_change_event(rnd, &event_list[VIDREND_EVENT_TYPE_RES_CHG]));
//		SMDCHECK(ismd_vidrend_get_error_event(rnd, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_ERROR]));
		SMDCHECK(ismd_vidrend_get_eos_event(rnd, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_EOS]));
		SMDCHECK(ismd_vidrend_get_client_id_seen_event(rnd, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_CLIENT_ID_SEEN]));
		SMDCHECK(ismd_vidrend_get_eoseg_event(rnd, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_EOSEG]));
		SMDCHECK(ismd_vidrend_get_soseg_event(rnd, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_SOSEG]));
		SMDCHECK(ismd_vidrend_get_underflow_event(rnd, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_UNDERFLOW]));
		SMDCHECK(ismd_vidrend_get_underflow_recovered_event(rnd, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_UNDERFLOW_RECOVERED]));
//		SMDCHECK(ismd_vidrend_get_vsync_event(rnd, ISMD_VIDREND_VSYNC_TYPE_FRAME, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_VSYNC_FRAME]));
//		SMDCHECK(ismd_vidrend_get_vsync_event(rnd, ISMD_VIDREND_VSYNC_TYPE_FIELD_TOP, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_VSYNC_FIELD_TOP]));
//		SMDCHECK(ismd_vidrend_get_vsync_event(rnd, ISMD_VIDREND_VSYNC_TYPE_FIELD_BOTTOM, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_VSYNC_FIELD_BOTTOM]));
//		SMDCHECK(ismd_vidrend_get_frame_flipped_event(rnd, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_FRAME_FLIPPED]));
	} // if
	if(ISMD_DEV_HANDLE_INVALID == ppr) {
		SMDCHECK_CLEANUP(ismd_vidpproc_open(&ppr));
		SMDCHECK_CLEANUP(ismd_vidpproc_get_input_port(ppr,&ppr_inp));
		SMDCHECK_CLEANUP(ismd_vidpproc_get_output_port(ppr,&ppr_out));
		SMDCHECK(ismd_vidpproc_get_input_src_resolution_event(ppr, &video_debug_thread.event_list[VIDPPROC_INPUT_SRC_RESOLUTION]));
		SMDCHECK(ismd_vidpproc_get_input_disp_resolution_event(ppr,&video_debug_thread.event_list[VIDPPROC_INPUT_DISP_RESOLUTION]));
		SMDCHECK(ismd_vidpproc_get_hw_hang_prevented_event(ppr,    &video_debug_thread.event_list[VIDPPROC_INPUT_HW_HANG_PREVENTED]));
		//Parameters will be set in UpdateOptions
		SMDCHECK_CLEANUP(ismd_port_connect(ppr_out, rnd_inp));
	} // if
//	if(ISMD_EVENT_HANDLE_INVALID == event_list[VIDDEC_USER_DATA]) {
//		SMDCHECK_CLEANUP(ismd_event_alloc(&event_list[VIDDEC_USER_DATA]));
//	} // if
	if(ISMD_DEV_HANDLE_INVALID == vid) {
		SMDCHECK_CLEANUP(ismd_viddec_open(codec, &vid));
		SMDCHECK(ismd_viddec_get_event(vid, ISMD_VIDDEC_RESOLUTION_CHANGE,   &event_list[VIDDEC_RESOLUTION_CHANGE]));
		SMDCHECK(ismd_viddec_get_event(vid, ISMD_VIDDEC_OUT_OF_MEMORY,       &video_debug_thread.event_list[VIDDEC_OUT_OF_MEMORY]));
		SMDCHECK(ismd_viddec_get_event(vid, ISMD_VIDDEC_BITSTREAM_ERROR,     &video_debug_thread.event_list[VIDDEC_BITSTREAM_ERROR]));
		SMDCHECK(ismd_viddec_get_event(vid, ISMD_VIDDEC_DROPPED_FRAMES,      &video_debug_thread.event_list[VIDDEC_DROPPED_FRAMES]));
//		SMDCHECK(ismd_viddec_get_event(vid, ISMD_VIDDEC_UNDERFLOW,           &video_debug_thread.event_list[VIDDEC_UNDERFLOW]));
//		SMDCHECK(ismd_viddec_get_event(vid, ISMD_VIDDEC_UNDERFLOW_RECOVERED, &video_debug_thread.event_list[VIDDEC_UNDERFLOW_RECOVERED]));
//		SMDCHECK(ismd_viddec_get_event(vid, ISMD_VIDDEC_EOS,                 &video_debug_thread.event_list[VIDDEC_EOS]));
//		SMDCHECK(ismd_viddec_get_event(vid, ISMD_VIDDEC_CLIENT_ID_SEEN,      &video_debug_thread.event_list[VIDDEC_CLIENT_ID_SEEN]));
//		SMDCHECK_CLEANUP(ismd_viddec_get_user_data_port(vid, &user_data));
//		SMDCHECK_CLEANUP(ismd_port_attach(user_data,event_list[VIDDEC_USER_DATA], ISMD_QUEUE_EVENT_NOT_EMPTY, ISMD_QUEUE_WATERMARK_NONE));

		SMDCHECK_CLEANUP(ismd_viddec_set_pts_interpolation_policy(vid, ISMD_VIDDEC_INTERPOLATE_MISSING_PTS, ISMD_NO_PTS));
		SMDCHECK_CLEANUP(ismd_viddec_set_frame_mask(vid, ISMD_VIDDEC_SKIP_NONE));
		SMDCHECK_CLEANUP(ismd_viddec_enable_output_scaling(vid, ISMD_VIDDEC_SCALE_NONE));
		SMDCHECK_CLEANUP(ismd_viddec_set_max_frames_to_decode(vid, ISMD_VIDDEC_ALL_FRAMES));
		SMDCHECK_CLEANUP(ismd_viddec_set_frame_error_policy(vid, (ISMD_CODEC_TYPE_H264 == codec) ? ISMD_VIDDEC_EMIT_ALL : ISMD_VIDDEC_EMIT_ERROR_CONCEAL_FRAMES));
//SMDCHECK_CLEANUP(ismd_viddec_set_seq_disp_ext_default_policy(vid, ISMD_VIDDEC_ISO13818_2));
		SMDCHECK_CLEANUP(ismd_viddec_get_output_port(vid, &vid_out));
		SMDCHECK_CLEANUP(ismd_port_connect(vid_out, ppr_inp));
		SMDCHECK_CLEANUP(ismd_viddec_get_input_port(vid, &queue[ICE_QUEUE_VIDEO].port));
//		if(ISMD_DEV_HANDLE_INVALID != mon) SMDCHECK_CLEANUP(ismd_bufmon_add_port_for_overrun_flush(mon, queue[ICE_QUEUE_VIDEO].port));
		queue[ICE_QUEUE_VIDEO].Attach();
		vid_codec = codec;
	} // if
	return true;
cleanup:
	DestroyVideoPipe();
	return false;
} // cICEDeviceSMD::CreateVideoPipe

void cICEDeviceSMD::DestroyVideoPipe() {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::DestroyVideoPipe");
	ismd_result_t ret = ISMD_SUCCESS;
	vid_codec = ISMD_CODEC_TYPE_INVALID;
//	if((ISMD_DEV_HANDLE_INVALID != mon) && (ISMD_PORT_HANDLE_INVALID != queue[ICE_QUEUE_VIDEO].port)) SMDCHECK(ismd_bufmon_remove_port_for_overrun_flush(mon, queue[ICE_QUEUE_VIDEO].port));
	queue[ICE_QUEUE_VIDEO] = tICE_QueueData();
	if(ISMD_DEV_HANDLE_INVALID != vid) SMDCHECK(ismd_dev_close(vid));
	vid = ISMD_DEV_HANDLE_INVALID;
	if(ISMD_EVENT_HANDLE_INVALID != event_list[VIDDEC_USER_DATA]) SMDCHECK(ismd_event_free(event_list[VIDDEC_USER_DATA]));
	event_list[VIDDEC_USER_DATA] = ISMD_EVENT_HANDLE_INVALID;
	
	if(ISMD_DEV_HANDLE_INVALID != ppr) SMDCHECK(ismd_dev_close(ppr));
	ppr = ISMD_DEV_HANDLE_INVALID;
	
	if(ISMD_DEV_HANDLE_INVALID != rnd) {
		if(ISMD_DEV_HANDLE_INVALID != mon) SMDCHECK(ismd_bufmon_remove_renderer(mon, rnd));
		SMDCHECK(ismd_dev_close(rnd));
	} // if
	rnd = ISMD_DEV_HANDLE_INVALID;
} // cICEDeviceSMD::DestroyVideoPipe

bool cICEDeviceSMD::CreatePipPipe(ismd_codec_type_t codec) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::CreatePipPipe %d %s", codec, CODEC_TYPE_STR(codec));
	ismd_result_t ret = ISMD_SUCCESS;
	ismd_event_t underrun_event = ISMD_EVENT_HANDLE_INVALID;
	if(ISMD_DEV_HANDLE_INVALID == pip_rnd) {
		SMDCHECK_CLEANUP(ismd_vidrend_open(&pip_rnd));
		SMDCHECK_CLEANUP(ismd_vidrend_get_input_port(pip_rnd, &pip_rnd_inp));
		SMDCHECK_CLEANUP(ismd_dev_set_clock(pip_rnd, pip_clk));
		SMDCHECK_CLEANUP(ismd_vidrend_set_timing_mode(pip_rnd, ISMD_TIMING_MODE_CONVERTED/*ISMD_TIMING_MODE_ORIGINAL*/));
		SMDCHECK_CLEANUP(ismd_bufmon_add_renderer(pip_mon, pip_rnd, &underrun_event));
		SMDCHECK_CLEANUP(ismd_dev_set_underrun_event(pip_rnd, underrun_event));
//#if (MAX_HOLD_TIME_NUM > MAX_HOLD_TIME_DEN)
//		SMDCHECK_CLEANUP(ismd_vidrend_enable_max_hold_time(pip_rnd, MAX_HOLD_TIME_NUM, MAX_HOLD_TIME_DEN));
//#else
		SMDCHECK_CLEANUP(ismd_vidrend_disable_max_hold_time(pip_rnd));
//#endif
		SMDCHECK_CLEANUP(ismd_vidrend_disable_fixed_frame_rate(pip_rnd));
		SMDCHECK_CLEANUP(ismd_vidrend_set_video_plane(pip_rnd, ICE_PIP_PLANE));
		SMDCHECK_CLEANUP(ismd_vidrend_set_interlaced_display_rate(ISMD_VIDREND_INTERLACED_RATE_50));
		SMDCHECK_CLEANUP(ismd_vidrend_set_flush_policy(pip_rnd,  ISMD_VIDREND_FLUSH_POLICY_REPEAT_FRAME/* ISMD_VIDREND_FLUSH_POLICY_DISPLAY_BLACK*/));
		SMDCHECK_CLEANUP(ismd_vidrend_set_stop_policy(pip_rnd, ISMD_VIDREND_STOP_POLICY_CLOSE_VIDEO_PLANE));
//		SMDCHECK(ismd_vidrend_get_resolution_change_event(pip_rnd, &event_list[VIDREND_EVENT_TYPE_RES_CHG]));
//		SMDCHECK(ismd_vidrend_get_error_event(pip_rnd, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_ERROR]));
//		SMDCHECK(ismd_vidrend_get_eos_event(pip_rnd, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_EOS]));
//		SMDCHECK(ismd_vidrend_get_client_id_seen_event(pip_rnd, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_CLIENT_ID_SEEN]));
//		SMDCHECK(ismd_vidrend_get_eoseg_event(pip_rnd, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_EOSEG]));
//		SMDCHECK(ismd_vidrend_get_soseg_event(pip_rnd, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_SOSEG]));
//		SMDCHECK(ismd_vidrend_get_underflow_event(pip_rnd, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_UNDERFLOW]));
//		SMDCHECK(ismd_vidrend_get_underflow_recovered_event(pip_rnd, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_UNDERFLOW_RECOVERED]));
//		SMDCHECK(ismd_vidrend_get_vsync_event(pip_rnd, ISMD_VIDREND_VSYNC_TYPE_FRAME, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_VSYNC_FRAME]));
//		SMDCHECK(ismd_vidrend_get_vsync_event(pip_rnd, ISMD_VIDREND_VSYNC_TYPE_FIELD_TOP, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_VSYNC_FIELD_TOP]));
//		SMDCHECK(ismd_vidrend_get_vsync_event(pip_rnd, ISMD_VIDREND_VSYNC_TYPE_FIELD_BOTTOM, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_VSYNC_FIELD_BOTTOM]));
//		SMDCHECK(ismd_vidrend_get_frame_flipped_event(pip_rnd, &video_debug_thread.event_list[VIDREND_EVENT_TYPE_FRAME_FLIPPED]));
	} // if
	if(ISMD_DEV_HANDLE_INVALID == pip_ppr) {
		SMDCHECK_CLEANUP(ismd_vidpproc_open(&pip_ppr));
		SMDCHECK_CLEANUP(ismd_vidpproc_get_input_port(pip_ppr,&pip_ppr_inp));
		SMDCHECK_CLEANUP(ismd_vidpproc_get_output_port(pip_ppr,&pip_ppr_out));
//		SMDCHECK(ismd_vidpproc_get_input_src_resolution_event(pip_ppr, &video_debug_thread.event_list[VIDPPROC_INPUT_SRC_RESOLUTION]));
//		SMDCHECK(ismd_vidpproc_get_input_disp_resolution_event(pip_ppr,&video_debug_thread.event_list[VIDPPROC_INPUT_DISP_RESOLUTION]));
//		SMDCHECK(ismd_vidpproc_get_hw_hang_prevented_event(pip_ppr,    &video_debug_thread.event_list[VIDPPROC_INPUT_HW_HANG_PREVENTED]));
		//Parameters will be set in UpdateOptions
		SMDCHECK_CLEANUP(ismd_port_connect(pip_ppr_out, pip_rnd_inp));
	} // if
//	if(ISMD_EVENT_HANDLE_INVALID == event_list[VIDDEC_USER_DATA]) {
//		SMDCHECK_CLEANUP(ismd_event_alloc(&event_list[VIDDEC_USER_DATA]));
//	} // if
	if(ISMD_DEV_HANDLE_INVALID == pip) {
		SMDCHECK_CLEANUP(ismd_viddec_open(codec, &pip));
//		SMDCHECK(ismd_viddec_get_event(vid, ISMD_VIDDEC_RESOLUTION_CHANGE,   &event_list[VIDDEC_RESOLUTION_CHANGE]));
//		SMDCHECK(ismd_viddec_get_event(vid, ISMD_VIDDEC_OUT_OF_MEMORY,       &video_debug_thread.event_list[VIDDEC_OUT_OF_MEMORY]));
//		SMDCHECK(ismd_viddec_get_event(vid, ISMD_VIDDEC_BITSTREAM_ERROR,     &video_debug_thread.event_list[VIDDEC_BITSTREAM_ERROR]));
//		SMDCHECK(ismd_viddec_get_event(vid, ISMD_VIDDEC_DROPPED_FRAMES,      &video_debug_thread.event_list[VIDDEC_DROPPED_FRAMES]));
////		SMDCHECK(ismd_viddec_get_event(vid, ISMD_VIDDEC_UNDERFLOW,           &video_debug_thread.event_list[VIDDEC_UNDERFLOW]));
////		SMDCHECK(ismd_viddec_get_event(vid, ISMD_VIDDEC_UNDERFLOW_RECOVERED, &video_debug_thread.event_list[VIDDEC_UNDERFLOW_RECOVERED]));
////		SMDCHECK(ismd_viddec_get_event(vid, ISMD_VIDDEC_EOS,                 &video_debug_thread.event_list[VIDDEC_EOS]));
////		SMDCHECK(ismd_viddec_get_event(vid, ISMD_VIDDEC_CLIENT_ID_SEEN,      &video_debug_thread.event_list[VIDDEC_CLIENT_ID_SEEN]));
//		SMDCHECK_CLEANUP(ismd_viddec_get_user_data_port(pip, &user_data));
//		SMDCHECK_CLEANUP(ismd_port_attach(user_data,event_list[VIDDEC_USER_DATA], ISMD_QUEUE_EVENT_NOT_EMPTY, ISMD_QUEUE_WATERMARK_NONE));

		SMDCHECK_CLEANUP(ismd_viddec_set_pts_interpolation_policy(pip, ISMD_VIDDEC_INTERPOLATE_MISSING_PTS, ISMD_NO_PTS));
		SMDCHECK_CLEANUP(ismd_viddec_set_frame_mask(pip, ISMD_VIDDEC_SKIP_NONE));
		SMDCHECK_CLEANUP(ismd_viddec_enable_output_scaling(pip, (ISMD_CODEC_TYPE_MPEG2==codec)?ISMD_VIDDEC_HALF_HORIZONTAL:ISMD_VIDDEC_QUARTER_HORIZONTAL));
		SMDCHECK_CLEANUP(ismd_viddec_set_max_frames_to_decode(pip, ISMD_VIDDEC_ALL_FRAMES));
		SMDCHECK_CLEANUP(ismd_viddec_set_frame_error_policy(pip, (ISMD_CODEC_TYPE_H264 == codec) ? ISMD_VIDDEC_EMIT_ALL : ISMD_VIDDEC_EMIT_ERROR_CONCEAL_FRAMES));
//SMDCHECK_CLEANUP(ismd_viddec_set_seq_disp_ext_default_policy(pip, ISMD_VIDDEC_ISO13818_2));
		SMDCHECK_CLEANUP(ismd_viddec_get_output_port(pip, &pip_out));
		SMDCHECK_CLEANUP(ismd_port_connect(pip_out, pip_ppr_inp));
		SMDCHECK_CLEANUP(ismd_viddec_get_input_port(pip, &queue[ICE_QUEUE_PIP].port));
		queue[ICE_QUEUE_PIP].Attach();
		pip_codec = codec;
	} // if
	return true;
cleanup:
	DestroyPipPipe();
	return false;
} // cICEDeviceSMD::CreatePipPipe

void cICEDeviceSMD::DestroyPipPipe() {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::DestroyPipPipe");
	ismd_result_t ret = ISMD_SUCCESS;
	vid_codec = ISMD_CODEC_TYPE_INVALID;
	queue[ICE_QUEUE_PIP] = tICE_QueueData();
	if(ISMD_DEV_HANDLE_INVALID != pip) SMDCHECK(ismd_dev_close(pip));
	pip = ISMD_DEV_HANDLE_INVALID;
	
	if(ISMD_DEV_HANDLE_INVALID != pip_ppr) SMDCHECK(ismd_dev_close(pip_ppr));
	pip_ppr = ISMD_DEV_HANDLE_INVALID;
	
	if(ISMD_DEV_HANDLE_INVALID != pip_rnd) {
		SMDCHECK(ismd_bufmon_remove_renderer(pip_mon, pip_rnd));
		SMDCHECK(ismd_dev_close(pip_rnd));
	} // if
	pip_rnd = ISMD_DEV_HANDLE_INVALID;
} // cICEDeviceSMD::DestroyPipPipe

bool cICEDeviceSMD::CreatePipe() {
	ismd_result_t ret = ISMD_SUCCESS;
//	SMDCHECK(ismd_clock_get_primary(&clk)); // What if that clock is from an other application?
	if(ISMD_CLOCK_HANDLE_INVALID == clk) {
		SMDCHECK_CLEANUP(ismd_clock_alloc_typed(ISMD_CLOCK_CLASS_MASTER, ISMD_CLOCK_DOMAIN_TYPE_AUDIO, &clk));
//		SMDCHECK_CLEANUP(ismd_clock_alloc(ISMD_CLOCK_TYPE_FIXED, &clk));
		SMDCHECK_CLEANUP(ismd_clock_make_primary(clk));
		SMDCHECK_CLEANUP(ismd_clock_set_vsync_pipe(clk, 0));
	} // if
	if(ISMD_CLOCK_HANDLE_INVALID != clk_sync) {
		SMDCHECK(ismd_clock_sync_close(clk_sync));
		clk_sync=ISMD_CLOCK_HANDLE_INVALID;
	} // if
#ifdef USE_CLOCK_SYNC
	if(ISMD_CLOCK_HANDLE_INVALID == clk_sync) {
		SMDCHECK_CLEANUP(ismd_clock_sync_open(&clk_sync));
		SMDCHECK_CLEANUP(ismd_clock_sync_set_clock(clk_sync, clk));
		SMDCHECK_CLEANUP(ismd_clock_sync_set_algorithm(clk_sync, THRESHOLD_MONITOR/*THRESHOLD_MONITOR, PID_FILTERING, PID_FILTERING_V1*/));
	} // if
#endif
	if(ISMD_CLOCK_HANDLE_INVALID == pip_clk) {
//		SMDCHECK_CLEANUP(ismd_clock_alloc_typed(ISMD_CLOCK_CLASS_MASTER, ISMD_CLOCK_DOMAIN_TYPE_VIDEO, &pip_clk));
//		SMDCHECK_CLEANUP(ismd_clock_alloc_typed(ISMD_CLOCK_CLASS_SLAVE, ISMD_CLOCK_DOMAIN_TYPE_VIDEO, &pip_clk));
		SMDCHECK_CLEANUP(ismd_clock_alloc(ISMD_CLOCK_TYPE_FIXED, &pip_clk));
//		SMDCHECK_CLEANUP(ismd_clock_make_primary(pip_clk));
		SMDCHECK_CLEANUP(ismd_clock_set_vsync_pipe(pip_clk, 0));
	} // if
	if(ISMD_EVENT_HANDLE_INVALID == event_list[BUFMON_UNDERRUN]) {
		SMDCHECK(ismd_event_alloc(&event_list[BUFMON_UNDERRUN]));
	} // if
	if(ISMD_EVENT_HANDLE_INVALID == event_list[BUFMON_OVERRUN]) {
		SMDCHECK(ismd_event_alloc(&event_list[BUFMON_OVERRUN]));
	} // if
	if(ISMD_EVENT_HANDLE_INVALID == event_list[BUFMON_CRITICAL]) {
		SMDCHECK(ismd_event_alloc(&event_list[BUFMON_CRITICAL]));
	} // if
	if(ISMD_DEV_HANDLE_INVALID == mon) {
		SMDCHECK_CLEANUP(ismd_bufmon_open(&mon));
		SMDCHECK_CLEANUP(ismd_dev_set_clock(mon, clk));
		if(ISMD_CLOCK_HANDLE_INVALID != clk_sync) SMDCHECK_CLEANUP(ismd_bufmon_set_clock_sync(mon, clk_sync));
		SMDCHECK_CLEANUP(ismd_bufmon_set_overrun_correction_amount (mon, 18000/*18000 default*/));
		SMDCHECK_CLEANUP(ismd_bufmon_set_underrun_correction_amount(mon, 18000/*18000 default*/));
		SMDCHECK(ismd_bufmon_set_underrun_event(mon, event_list[BUFMON_UNDERRUN]));
		SMDCHECK(ismd_bufmon_set_overrun_event(mon, event_list[BUFMON_OVERRUN]));
		SMDCHECK(ismd_bufmon_set_critical_error_event(mon, event_list[BUFMON_CRITICAL]));
	} // if
	if(ISMD_DEV_HANDLE_INVALID == pip_mon) {
		SMDCHECK_CLEANUP(ismd_bufmon_open(&pip_mon));
		SMDCHECK_CLEANUP(ismd_dev_set_clock(pip_mon, pip_clk));
		SMDCHECK_CLEANUP(ismd_bufmon_set_overrun_correction_amount (pip_mon, 18000/*18000 default*/));
		SMDCHECK_CLEANUP(ismd_bufmon_set_underrun_correction_amount(pip_mon, 18000/*18000 default*/));
//		SMDCHECK(ismd_bufmon_set_underrun_event(mon, event_list[BUFMON_UNDERRUN]));
//		SMDCHECK(ismd_bufmon_set_overrun_event(mon, event_list[BUFMON_OVERRUN]));
//		SMDCHECK(ismd_bufmon_set_critical_error_event(mon, event_list[BUFMON_CRITICAL]));
	} // if
	
	if(!CreateAudioPipe(DEFAULT_AUDIO_CODEC)) goto cleanup;
	if(!CreateVideoPipe(DEFAULT_VIDEO_CODEC)) goto cleanup;
	if(!CreatePipPipe  (DEFAULT_VIDEO_CODEC)) goto cleanup;
	UpdateOptions();
	return true;
cleanup:
	DestroyPipe();
	return false;
} // cICEDeviceSMD::CreatePipe

void cICEDeviceSMD::DestroyPipe() {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::DestroyPipe");
	ismd_result_t ret = ISMD_SUCCESS;
	DestroyAudioPipe();
	DestroyVideoPipe();
	DestroyPipPipe();
	if(ISMD_DEV_HANDLE_INVALID != mon) SMDCHECK(ismd_dev_close(mon));
	mon = ISMD_DEV_HANDLE_INVALID;
	if(ISMD_DEV_HANDLE_INVALID != pip_mon) SMDCHECK(ismd_dev_close(pip_mon));
	pip_mon = ISMD_DEV_HANDLE_INVALID;
	if(ISMD_CLOCK_HANDLE_INVALID != clk_sync) SMDCHECK(ismd_clock_sync_close(clk_sync));
	clk_sync=ISMD_CLOCK_HANDLE_INVALID;
	if(ISMD_CLOCK_HANDLE_INVALID != clk) SMDCHECK(ismd_clock_free(clk)); 
	clk = ISMD_CLOCK_HANDLE_INVALID;
	if(ISMD_CLOCK_HANDLE_INVALID != pip_clk) SMDCHECK(ismd_clock_free(pip_clk)); 
	pip_clk = ISMD_CLOCK_HANDLE_INVALID;
	pipe_state = ISMD_DEV_STATE_INVALID;
} // cICEDeviceSMD::DestroyPipe

bool cICEDeviceSMD::SetPipe(ismd_dev_state_t state) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::SetPipe %d %s", state, DEV_STATE_STR(state));
	ismd_result_t ret = ISMD_SUCCESS;
	if(ISMD_CLOCK_HANDLE_INVALID != clk) {
		if(ISMD_DEV_STATE_PLAY==state) {
			if(!paused_time) {
				SMDCHECK(ismd_clock_get_time(clk, &base_time));
				isyslog("Set new basetime %llx", base_time);
//SMDCHECK(ismd_dev_set_state(mon, ISMD_DEV_STATE_STOP));
				SMDCHECK(ismd_dev_set_state(aud, ISMD_DEV_STATE_STOP));
				SMDCHECK(ismd_dev_set_state(rnd, ISMD_DEV_STATE_STOP));
				queue[ICE_QUEUE_VIDEO].new_seg = queue[ICE_QUEUE_AUDIO].new_seg = true;
				queue[ICE_QUEUE_VIDEO].last_pts = queue[ICE_QUEUE_AUDIO].last_pts = ISMD_NO_PTS;
			} else {
				ismd_time_t curr_time;
				SMDCHECK(ismd_clock_get_time(clk, &curr_time));
				base_time += curr_time-paused_time;
				isyslog("Set paused basetime %llx", base_time);
			} // if
			if(ISMD_DEV_HANDLE_INVALID != aud) SMDCHECK(ismd_dev_set_stream_base_time(aud, base_time));
			if(ISMD_DEV_HANDLE_INVALID != rnd) SMDCHECK(ismd_dev_set_stream_base_time(rnd, base_time));
		} // if
		if(ISMD_DEV_STATE_PAUSE==state) {
			SMDCHECK(ismd_clock_get_time(clk, &paused_time));
		} else
			paused_time=0;
	} // if
	if(ISMD_DEV_HANDLE_INVALID != mon) SMDCHECK(ismd_dev_set_state(mon, (ISMD_NORMAL_PLAY_RATE==playRate)?state:ISMD_DEV_STATE_STOP));
	if(ISMD_DEV_HANDLE_INVALID != aud) SMDCHECK(ismd_dev_set_state(aud, state));
	if(ISMD_DEV_HANDLE_INVALID != vid) SMDCHECK(ismd_dev_set_state(vid, state));
	if(ISMD_DEV_HANDLE_INVALID != ppr) SMDCHECK(ismd_dev_set_state(ppr, state));
	if(ISMD_DEV_HANDLE_INVALID != rnd) SMDCHECK(ismd_dev_set_state(rnd, state));
	if(ISMD_DEV_STATE_STOP==state) {
		queue[ICE_QUEUE_VIDEO].follow = queue[ICE_QUEUE_AUDIO].follow = 0;
		pes_buffer_pos=0;
		if(ISMD_DEV_HANDLE_INVALID != aud) SMDCHECK(ismd_dev_flush(aud));
		if(ISMD_DEV_HANDLE_INVALID != vid) SMDCHECK(ismd_dev_flush(vid));
		if(ISMD_DEV_HANDLE_INVALID != ppr) SMDCHECK(ismd_dev_flush(ppr));
		if(ISMD_DEV_HANDLE_INVALID != rnd) SMDCHECK(ismd_dev_flush(rnd));
	} // if
	pipe_state = state;
	ICE_DBG(DBG_FN,"cICEDeviceSMD::SetPipe %d %s DONE", state, DEV_STATE_STR(state));
	return true;
} // cICEDeviceSMD::SetPipe

void cICEDeviceSMD::UpdateOptions() {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::UpdateOptions");
	if(!plugin) return;
	ismd_result_t ret = ISMD_SUCCESS;
	if(ISMD_DEV_HANDLE_INVALID != ppr) {
//		SMDCHECK(ismd_vidpproc_set_deinterlace_policy(ppr, plugin->options.deinterlace_mode));
		SMDCHECK(ismd_vidpproc_set_scaling_policy(ppr, plugin->options.scale_mode));
		SMDCHECK(ismd_vidpproc_set_scaling_policy(pip_ppr, plugin->options.scale_mode));
		if(INVALID_LEVEL!=plugin->options.deringing_level) {
			SMDCHECK(ismd_vidpproc_deringing_enable(ppr));
			SMDCHECK(ismd_vidpproc_set_deringing_level(ppr, plugin->options.deringing_level-MIN_DERINGNING_LEVEL+1));
		} else {
			SMDCHECK(ismd_vidpproc_deringing_disable(ppr));
		} // if
		if(INVALID_LEVEL!=plugin->options.gaussian_level) {
			SMDCHECK(ismd_vidpproc_gaussian_enable(ppr));
			SMDCHECK(ismd_vidpproc_set_gaussian_level(ppr, plugin->options.gaussian_level-MIN_GAUSSIAN_LEVEL));
		} else {
			SMDCHECK(ismd_vidpproc_gaussian_disable(ppr));
		} // if
		if(INVALID_LEVEL!=plugin->options.degrade_level) {
			SMDCHECK(ismd_vidpproc_set_degrade_factor(ppr, plugin->options.degrade_level-MIN_DEGRADE_LEVEL));
		} else {
			SMDCHECK(ismd_vidpproc_disable_degrade_factor(ppr));
		} // if
		if(plugin->options.pan_scan) {
			SMDCHECK(ismd_viddec_pan_scan_enable(vid));
			SMDCHECK(ismd_vidpproc_pan_scan_enable(ppr));
		} else {
			SMDCHECK(ismd_viddec_pan_scan_disable(vid));
			SMDCHECK(ismd_vidpproc_pan_scan_disable(ppr));
		} // if
		if(plugin->options.override_frame_polarity) {
			SMDCHECK(ismd_viddec_enable_override_frame_polarity(vid));
		} else {
			SMDCHECK(ismd_viddec_disable_override_frame_polarity(vid));
		} // if
//SMDCHECK(ismd_vidpproc_set_parameter(ppr, PAR_ENABLE_GEN4_ENHANCED_DI_ALGORITHM, 1));
#if 0
for(int i=PAR_FIRST; i< PAR_COUNT; i++) {
    int val;
    SMDCHECK(ismd_vidpproc_get_parameter(ppr, (ismd_vidpproc_param_t)i, &val));
    printf("ppr %s %d\n", PPROC_PAR_STR(i), val);
} // for
#endif
	} // if
} // cICEDeviceSMD::UpdateOptions

void cICEDeviceSMD::UpdateScreen(const gdl_tvmode_t &mode) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::UpdateScreenSize(%d,%d)", mode.width, mode.height);
	ismd_result_t ret = ISMD_SUCCESS;
	if(ISMD_DEV_HANDLE_INVALID != ppr) {
		int num = 1;
		int den = 1;
#if 0 
		// Seems not to be correct
		if(mode.height <= 480) { // NTSC
			num = 640;
			den = 702;
		} else if (Height <= 576) { // PAL
			num = 768; // 576/702 * 4/3 = 768/702 and not 720 = 16/15! 
			den = 702;
		} // if
#else
		if(mode.height <= 576) {
			num=4;
			den=3;
		} // if
#endif
//		SMDCHECK(ismd_vidpproc_set_display_scaling(ppr, Width, Height)); // HD=1/1, 576=16/15, 480=10/11
		SMDCHECK(ismd_vidpproc_set_dest_params2(ppr, mode.width, mode.height, num, den, 0, 0)); // HD=1/1, 576=16/15, 480=10/11
//		SMDCHECK(ismd_vidpproc_set_dest_params(ppr, Width, Height, num, den)); // HD=1/1, 576=16/15, 480=10/11
//		if(ISMD_DEV_HANDLE_INVALID != pip_ppr) SMDCHECK(ismd_vidpproc_set_dest_params2(pip_ppr, Width/4, Height/4, num, den, Width/8, Height/8));
		ismd_vidpproc_deinterlace_policy_t deint = plugin->options.deinterlace_mode;
		if(mode.interlaced) {
			switch(plugin->options.deinterlace_mode) {
				case ISMD_VIDPPROC_DI_POLICY_FILM:
				case ISMD_VIDPPROC_DI_POLICY_VIDEO:
				case ISMD_VIDPPROC_DI_POLICY_AUTO:
					deint = ISMD_VIDPPROC_DI_POLICY_NONE;
					break;
				default:
					break;
			} // switch
		} // if
		SMDCHECK(ismd_vidpproc_set_deinterlace_policy(ppr, deint));
		switch (mode.refresh) {
			case GDL_REFRESH_29_97 :
			case GDL_REFRESH_30    :
			case GDL_REFRESH_59_94 :
			case GDL_REFRESH_60    :
				SMDCHECK(ismd_vidrend_set_interlaced_display_rate(ISMD_VIDREND_INTERLACED_RATE_59_94));
				break;
			case GDL_REFRESH_25 :
			case GDL_REFRESH_50 :
			default             :
				SMDCHECK(ismd_vidrend_set_interlaced_display_rate(ISMD_VIDREND_INTERLACED_RATE_50));
				break;
		} // switch
	} // if
} //  cICEDeviceSMD::UpdateScreen

bool cICEDeviceSMD::SetVideoCodec(ismd_codec_type_t codec) {
	ismd_result_t ret = ISMD_SUCCESS;
	if(vid_codec==codec) goto success;
	isyslog("Change video to %d %s", codec, CODEC_TYPE_STR(codec));
	if(!SetPipe(ISMD_DEV_STATE_STOP)) return false;
	vid_codec=ISMD_CODEC_TYPE_INVALID;
	if(ISMD_DEV_HANDLE_INVALID != vid) SMDCHECK(ismd_dev_close(vid));
	vid = ISMD_DEV_HANDLE_INVALID;
	if(!CreateVideoPipe(codec)) return false;
success:
	if(ISMD_DEV_STATE_PLAY!=pipe_state)
		if(!SetPipe(ISMD_DEV_STATE_PLAY)) return false;
	return true;
} //  cICEDeviceSMD::SetVideoCodec

bool cICEDeviceSMD::SetPipCodec(ismd_codec_type_t codec) {
	ismd_result_t ret = ISMD_SUCCESS;
	if(pip_codec==codec) return true;
	isyslog("Change pip to %d %s", codec, CODEC_TYPE_STR(codec));
//	if(!SetPipe(ISMD_DEV_STATE_STOP)) return false;
	pip_codec=ISMD_CODEC_TYPE_INVALID;
	if(ISMD_DEV_HANDLE_INVALID != pip) SMDCHECK(ismd_dev_close(pip));
	pip = ISMD_DEV_HANDLE_INVALID;
	if(!CreatePipPipe(codec)) return false;
	return true;
} // cICEDeviceSMD::SetPipCodec

bool cICEDeviceSMD::SetAudioCodec(ismd_audio_format_t codec) {
#if 1
	ismd_result_t ret = ISMD_SUCCESS;
	if(aud_codec==codec) goto success;
	isyslog("Change audio to %d %s", codec, AUDIO_MEDIA_FMT_STR(codec));
	if(!SetPipe(ISMD_DEV_STATE_STOP)) return false;
	aud_codec = ISMD_AUDIO_MEDIA_FMT_INVALID;
	if(ISMD_DEV_HANDLE_INVALID != aud) {
		audio_underrun_event = ISMD_EVENT_HANDLE_INVALID;
		if(ISMD_DEV_HANDLE_INVALID != mon) SMDCHECK(ismd_bufmon_remove_renderer(mon, aud));
		SMDCHECK(ismd_dev_close(aud));
	} // if
	aud = ISMD_DEV_HANDLE_INVALID;
	if(!CreateAudioPipe(codec)) return false;
success:
	if(ISMD_DEV_STATE_PLAY!=pipe_state)
		if(!SetPipe(ISMD_DEV_STATE_PLAY)) return false;
	return true;
#else
	ismd_result_t ret = ISMD_SUCCESS;
	if(aud_codec==codec) goto success;
	isyslog("Change audio from %s to %s (%s)", AUDIO_MEDIA_FMT_STR(aud_codec), AUDIO_MEDIA_FMT_STR(codec), DEV_STATE_STR(pipe_state));
	SMDCHECK(ismd_dev_set_state(aud, ISMD_DEV_STATE_STOP));
	aud_codec = ISMD_AUDIO_MEDIA_FMT_INVALID;
	SMDCHECK(ismd_audio_input_set_data_format(aud_prc, aud, codec));
	if(ISMD_SUCCESS!=ret) return false;
	aud_codec = codec;
	queue[ICE_QUEUE_AUDIO].discont=true;
success:
	SMDCHECK(ismd_dev_set_state(aud, pipe_state));
	return true;
#endif
} // cICEDeviceSMD::SetAudioCodec

void cEventDebugThread::Action() {
	ismd_result_t ret = ISMD_SUCCESS;
	ismd_event_t trigger = ISMD_EVENT_HANDLE_INVALID;
	while(Running()) {
		ismd_event_t events[ISMD_EVENT_LIST_MAX];
		int event_count=0;
		for(int i=0; i<EVENT_COUNT; i++) {
			if(ISMD_EVENT_HANDLE_INVALID!=event_list[i]) {
				if(event_count>=ISMD_EVENT_LIST_MAX) {
					isyslog("Monitored events exceeds maximum - \"%s\" not handled!", EVENT_STR(i));
					continue;
				} // if
				events[event_count++]=event_list[i];
			} // if
		} // if
		if(!event_count) {
			cCondWait::SleepMs(100);
			continue;
		} // if
		ret = ismd_event_wait_multiple(events, event_count, 100, &trigger);
		if(ISMD_SUCCESS==ret) {
			SMDCHECK(ismd_event_acknowledge(trigger));
			bool found=false;
			for(int i=0; i<EVENT_COUNT; i++) {
				if(event_list[i]!=trigger) continue;
				found=true;
				if (device && ((AUDIO_NOTIFY_INPUT_EMPTY    ==i) || 
				               (AUDIO_NOTIFY_INPUT_RECOVERED==i) || 
				               (VIDREND_EVENT_TYPE_ERROR    ==i) || 
				               (VIDDEC_UNDERFLOW            ==i) || 
				               (VIDDEC_UNDERFLOW_RECOVERED  ==i) || 
				               (VIDREND_EVENT_TYPE_UNDERFLOW==i))) {
					PRINT_QUEUE_STATUS(EVENT_STR(i), device);
				} else if(VIDDEC_DROPPED_FRAMES == i) {
					PRINT_VSTREAM_INFO(EVENT_STR(i), device);
				} else isyslog("GOT EVENT %s", EVENT_STR(i));
				break;
			} // for
			if(!found) isyslog("GOT UNRECOGNIZED EVENT %d", trigger);
		} else if(ISMD_ERROR_TIMEOUT!=ret)
			cCondWait::SleepMs(100);
	} // while
	isyslog("!!!!!!!!!!!!!!!!!cEventDebugThread terminated!!!!!!!!!!!!!!!!!");
} // cEventDebugThread::Action

void cICEDeviceSMD::PlayPipVideo(const unsigned char *Data, int Length){
//	ICE_DBG(DBG_FN,"cICEDeviceSMD::PlayPipVideo %d\n", Length);
	// Format is checked in SetPipFormat
	WriteESData(ICE_QUEUE_PIP, Length, Data);
} // cICEDeviceSMD::PlayPipVideo

void cICEDeviceSMD::StartPip(bool start){ 
	ICE_DBG(DBG_FN,"cICEDeviceSMD::StartPip %d\n", start);
	ismd_result_t ret = ISMD_SUCCESS;
	if(start) {
		ismd_time_t base_time;
		SMDCHECK(ismd_clock_get_time(pip_clk, &base_time));
		if(ISMD_DEV_HANDLE_INVALID != pip_rnd) SMDCHECK(ismd_dev_set_stream_base_time(pip_rnd, base_time));
		queue[ICE_QUEUE_PIP].new_seg  = true;
		queue[ICE_QUEUE_PIP].last_pts = ISMD_NO_PTS;
	} // if
	if(ISMD_DEV_HANDLE_INVALID != pip_mon) SMDCHECK(ismd_dev_set_state(pip_mon, start ? ISMD_DEV_STATE_PLAY : ISMD_DEV_STATE_STOP));
	if(ISMD_DEV_HANDLE_INVALID != pip    ) SMDCHECK(ismd_dev_set_state(pip,     start ? ISMD_DEV_STATE_PLAY : ISMD_DEV_STATE_STOP));
	if(ISMD_DEV_HANDLE_INVALID != pip_ppr) SMDCHECK(ismd_dev_set_state(pip_ppr, start ? ISMD_DEV_STATE_PLAY : ISMD_DEV_STATE_STOP));
	if(ISMD_DEV_HANDLE_INVALID != pip_rnd) SMDCHECK(ismd_dev_set_state(pip_rnd, start ? ISMD_DEV_STATE_PLAY : ISMD_DEV_STATE_STOP));
	if(!start) {
		if(ISMD_DEV_HANDLE_INVALID != pip    ) SMDCHECK(ismd_dev_flush(pip));
		if(ISMD_DEV_HANDLE_INVALID != pip_ppr) SMDCHECK(ismd_dev_flush(pip_ppr));
		if(ISMD_DEV_HANDLE_INVALID != pip_rnd) SMDCHECK(ismd_dev_flush(pip_rnd));
	} // if
} // cICEDeviceSMD::StartPip

bool cICEDeviceSMD::IsPipChannel(const cChannel *channel, bool *ret) { 
	ICE_DBG(DBG_FN,"cICEDeviceSMD::SetPipDimensions %s\n", channel ? channel->Name() : "invalid channel");
	if(!ret) return true;
	*ret = false;
	if(!channel) return true;
	*ret = (channel->Vpid() != 0);
	return true;
} // cICEDeviceSMD::IsPipChannel

bool cICEDeviceSMD::SetPipVideoType(int type) {
//	ICE_DBG(DBG_FN,"cICEDeviceSMD::SetPipVideoType %d\n", type);
	if(type == currPipType) return true;
	ismd_codec_type_t codec = ISMD_CODEC_TYPE_INVALID;
	switch(type) {
		case 0x01: //MPEG1
		case 0x02: //MPEG2
			codec = ISMD_CODEC_TYPE_MPEG2; 
			break;
		case 0x1B: //H264
			codec = ISMD_CODEC_TYPE_H264;
			break;
		case 0xEA: //VC1
			codec = ISMD_CODEC_TYPE_VC1;
			break;
		default:
			isyslog("cICEDeviceSMD::SetPipVideoType: Invalid pip type 0x%x", type);
	} // switch
	StartPip(false);
	if(!SetPipCodec(codec)) return false;
	currPipType = type;
	StartPip(true);
	return true;
} // cICEDeviceSMD::SetPipVideoType

void cICEDeviceSMD::SetPipDimensions(const uint x, const uint y, const uint width, const uint height){ 
	ICE_DBG(DBG_FN,"cICEDeviceSMD::SetPipDimensions %d %d %d %d\n", x, y, width, height);
	int vid_w, vid_h, osd_w, osd_h;
	double vid_a, osd_a;
	GetVideoSize(vid_w, vid_h, vid_a);
	gdl_tvmode_t mode;
	if(plugin && plugin->GetCurrentTVMode(mode)) {
		vid_w = mode.width;
		vid_h = mode.height;
	} // if
	GetOsdSize(osd_w, osd_h, osd_a);

	int pip_x = (((x*vid_w)/osd_w)/2)*2;
	int pip_y = (((y*vid_h)/osd_h)/2)*2;
	int pip_w = ((((width  ? width  : ICE_DEFAULT_PIP_WIDTH ) *vid_w)/osd_w)/2)*2;
	int pip_h = ((((height ? height : ICE_DEFAULT_PIP_HEIGHT) *vid_h)/osd_h)/2)*2;

	ismd_result_t ret = ISMD_SUCCESS;
printf("cICEDeviceSMD::SetPipDimensions >>>>>>>>>>>>>> %d,%d %d*%d -> %d,%d %d*%d (%d %d) [%d %d]\n", x, y, width, height, pip_x, pip_y, pip_w, pip_h, vid_w, vid_h, osd_w, osd_h);
	int num = 1;
	int den = 1;
	if(vid_h <= 576) {
		num=4;
		den=3;
	} // if
	if(ISMD_DEV_HANDLE_INVALID != pip_ppr) SMDCHECK(ismd_vidpproc_set_dest_params2(pip_ppr, pip_w, pip_h, num, den, pip_x, pip_y));
} // cICEDeviceSMD::SetPipDimensions

void cICEDeviceSMD::ChannelSwitchInLiveMode(){ 
	ICE_DBG(DBG_FN,"cICEDeviceSMD::ChannelSwitchInLiveMode\n"); 
} // cICEDeviceSMD::ChannelSwitchInLiveMode

void cICEDeviceSMD::OverflowDetected() {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::OverflowDetected\n"); 
	if(ISMD_DEV_HANDLE_INVALID == mon) return;
	ismd_result_t ret = ISMD_SUCCESS;
	ismd_event_t event = ISMD_EVENT_HANDLE_INVALID;
	SMDCHECK(ismd_bufmon_get_push_source_overrun_event(mon, &event));
	if(ISMD_EVENT_HANDLE_INVALID != event) SMDCHECK(ismd_event_strobe(event));
//	if(ISMD_EVENT_HANDLE_INVALID != event) SMDCHECK(ismd_event_set(event));
} // cICEDeviceSMD::OverflowDetected

void cICEDeviceSMD::SetPlayRate(int Rate) {
	ICE_DBG(DBG_FN,"cICEDeviceSMD::SetPlayRate %d\n", Rate); 
	ismd_result_t ret = ISMD_SUCCESS;
	ismd_dev_state_t old_state = pipe_state;
	if(!Rate) { // Enter still frame mode
		if((ISMD_DEV_HANDLE_INVALID != mon) && (ISMD_DEV_HANDLE_INVALID != rnd)) SMDCHECK(ismd_bufmon_remove_renderer(mon, rnd));
		SMDCHECK(ismd_audio_set_clock_mode(aud_prc, /*ISMD_AUDIO_CLK_LOCKED*/ISMD_AUDIO_CLK_INDEPENDENT));
		if(ISMD_DEV_HANDLE_INVALID!=rnd) SMDCHECK(ismd_vidrend_enable_fixed_frame_rate(rnd, 90000/25));
		playRate = Rate;
		return;
	} else if(!playRate) { // Recover from still frame mode
		if((ISMD_DEV_HANDLE_INVALID != mon) && (ISMD_DEV_HANDLE_INVALID != rnd)) {
			ismd_event_t underrun_event = ISMD_EVENT_HANDLE_INVALID;
			SMDCHECK(ismd_bufmon_add_renderer(mon, rnd, &underrun_event));
			SMDCHECK(ismd_dev_set_underrun_event(rnd, underrun_event));
		}
		SMDCHECK(ismd_audio_set_clock_mode(aud_prc, ISMD_AUDIO_CLK_LOCKED/*ISMD_AUDIO_CLK_INDEPENDENT*/));
	} // if
	if((IS_IPB_SPEED(Rate) != IS_IPB_SPEED(playRate)) || (IS_FORWARD_SPEED(Rate) != IS_FORWARD_SPEED(playRate))) {
		SetPipe(ISMD_DEV_STATE_STOP);
	} else if (ISMD_DEV_STATE_STOP != old_state) {
		SetPipe(ISMD_DEV_STATE_PAUSE);
		ismd_vidrend_stream_position_info_t vinfo;
		SMDCHECK_IGN(ISMD_ERROR_NO_DATA_AVAILABLE, ismd_vidrend_get_stream_position(rnd, &vinfo));
		if(ISMD_SUCCESS == ret) {
			SMDCHECK(ismd_dev_set_play_rate(rnd, vinfo.linear_time, Rate));
			SMDCHECK(ismd_dev_set_play_rate(aud, vinfo.linear_time, Rate));
		} else
			SetPipe(ISMD_DEV_STATE_STOP);
	} // if
	if(ISMD_DEV_STATE_STOP==pipe_state) queue[ICE_QUEUE_VIDEO].new_seg = true;
	if(IS_IPB_SPEED(Rate)) {
//		queue[ICE_QUEUE_VIDEO].new_seg = queue[ICE_QUEUE_AUDIO].new_seg = true;
		if((Rate > 30000) && (ISMD_CODEC_TYPE_H264==vid_codec)) {
			if(ISMD_DEV_HANDLE_INVALID!=vid) SMDCHECK(ismd_viddec_set_frame_mask(vid, ISMD_VIDDEC_SKIP_FRAME_TYPE_B));
		} else {
			if(ISMD_DEV_HANDLE_INVALID!=vid) SMDCHECK(ismd_viddec_set_frame_mask(vid, ISMD_VIDDEC_SKIP_NONE));
		} // if
		if(ISMD_DEV_HANDLE_INVALID!=rnd) SMDCHECK(ismd_vidrend_disable_fixed_frame_rate(rnd));
		if(ISMD_DEV_HANDLE_INVALID!=aud) SMDCHECK(ismd_audio_input_enable(aud_prc, aud));
	} else {
//		SMDCHECK(ismd_viddec_set_frame_mask(vid, ISMD_VIDDEC_SKIP_NONE));
		if(ISMD_DEV_HANDLE_INVALID!=vid) SMDCHECK(ismd_viddec_set_frame_mask(vid, IS_IPB_SPEED(Rate) ? ISMD_VIDDEC_SKIP_NONE : ISMD_VIDDEC_SKIP_FRAME_TYPE_NON_REF | ISMD_VIDDEC_SKIP_FRAME_TYPE_P | ISMD_VIDDEC_SKIP_FRAME_TYPE_B));
		int vidrend_rate = (2 * 90000ll / 9 ) * ISMD_NORMAL_PLAY_RATE / (Rate?abs(Rate):ISMD_NORMAL_PLAY_RATE);
		if(ISMD_CODEC_TYPE_H264==vid_codec) vidrend_rate *= 2;
		if(ISMD_DEV_HANDLE_INVALID!=rnd) SMDCHECK(ismd_vidrend_enable_fixed_frame_rate(rnd, vidrend_rate));
		if(ISMD_DEV_HANDLE_INVALID!=aud) SMDCHECK(ismd_audio_input_disable(aud_prc, aud));
	} // if
	playRate = Rate;
	SetPipe(old_state<=ISMD_DEV_STATE_STOP?ISMD_DEV_STATE_STOP:ISMD_DEV_STATE_PLAY);
} // cICEDeviceSMD::PlayRate

