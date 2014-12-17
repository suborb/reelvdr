/***************************************************************************
 *   Copyright (C) 2007 by Dirk Leber                                      *
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

#include "erfplayer.h"

#include <stdarg.h>
#include <syslog.h>

#include "channel.h"
#include <nmtk/platform.h>
#include <nmtk/event.h>
#include <nmtk/log.h>
#include <nmtk/pmsg.h>
#include <nmtk/nmtk.h>
#include <decoder.h>
 
void asf_parser_init(void);

#define INFO_INTERVAL 500
#define DEFAULT_ID ((unsigned long)0xffffffff)

static pthread_t gInfoThread = 0; 
static int gTerminate = 1;

static pthread_cond_t gEventSignal;
static pthread_mutex_t gEventLock;
static pthread_t gEventThread = 0;
static pthread_mutex_t gPlayerLock;
static int gDecoderPending = 0;
static int gClientPending  = 0;
static pthread_t gDispatcherThread = 0; 
static struct dispatcher  *gDispatcher = 0; 
static struct decypher    *gDecoder = 0; 
static struct nmtk_client *gClient = 0; 
static timer_id_t gInfoTimer = 0;
static int  gState = ERF_STATE_UNKNOWN;
static long gError = 0;
static long gFlags = 0;
static long long gReqPos = 0;
static int gReqSpeed = 100;
struct timeline gTimeline = {0,0,0}; 
static media_clock_t gDuration = 0;
static struct media_program *gProgram = 0;
static struct media_track *gVideo = 0;
static struct media_track *gAudio = 0;
static unsigned long gReqProg  = DEFAULT_ID;
static unsigned long gReqVideo = DEFAULT_ID;
static unsigned long gReqAudio = DEFAULT_ID;
static char gAudioLang[64] = {0};
static unsigned long gReqFlags = 0;
/***************************************************************************
	erf tool functions
 ***************************************************************************/

static char gStatusText[1024];
static const char *GetStateText(long fStatus) {
	switch (fStatus) {
		case DECYPHER_PLAYING            : return "DECYPHER_PLAYING"; 
		case DECYPHER_FLUSHED            : return "DECYPHER_FLUSHED"; 
		case DECYPHER_END_OF_STREAM      : return "DECYPHER_END_OF_STREAM"; 
		case DECYPHER_SYNCING            : return "DECYPHER_SYNCING"; 
		case DECYPHER_FORMAT_ERROR       : return "DECYPHER_FORMAT_ERROR"; 
		case DECYPHER_VIDEO_DEV_ERROR    : return "DECYPHER_VIDEO_DEV_ERROR"; 
		case DECYPHER_AUDIO_DEV_ERROR    : return "DECYPHER_AUDIO_DEV_ERROR"; 
		case NMTK_STATE_UNKNOWN          : return "NMTK_STATE_UNKNOWN"; 
		case NMTK_STATE_TYPE_NORMAL      : return "NMTK_STATE_TYPE_NORMAL"; 
		case NMTK_STATE_IDLE             : return "NMTK_STATE_IDLE"; 
		case NMTK_STATE_RUNNING          : return "NMTK_STATE_RUNNING"; 
		case NMTK_STATE_TYPE_PROGRESS    : return "NMTK_STATE_TYPE_PROGRESS"; 
		case NMTK_STATE_RESOLVING        : return "NMTK_STATE_RESOLVING"; 
		case NMTK_STATE_CONNECTING       : return "NMTK_STATE_CONNECTING";
		case NMTK_STATE_REQUESTING       : return "NMTK_STATE_REQUESTING"; 
		case NMTK_STATE_DESCRIBE         : return "NMTK_STATE_DESCRIBE"; 
		case NMTK_STATE_SETUP            : return "NMTK_STATE_SETUP"; 
		case NMTK_STATE_PLAY             : return "NMTK_STATE_PLAY"; 
		case NMTK_STATE_LOADING          : return "NMTK_STATE_LOADING"; 
		case NMTK_STATE_TYPE_GEN_ERROR   : return "NMTK_STATE_TYPE_GEN_ERROR"; 
		case NMTK_STATE_HOST_UNKNOWN     : return "NMTK_STATE_HOST_UNKNOWN";
		case NMTK_STATE_BAD_URI          : return "NMTK_STATE_BAD_URI"; 
		case NMTK_STATE_PROTO_ERROR      : return "NMTK_STATE_PROTO_ERROR"; 
		case NMTK_STATE_NOT_2XX          : return "NMTK_STATE_NOT_2XX";
		case NMTK_STATE_UNKNOWN_FORMAT   : return "NMTK_STATE_UNKNOWN_FORMAT";
		case NMTK_STATE_INVALID_SDP      : return "NMTK_STATE_INVALID_SDP"; 
		case NMTK_STATE_NO_MEDIA         : return "NMTK_STATE_NO_MEDIA"; 
		case NMTK_STATE_FORMAT_ERROR     : return "NMTK_STATE_FORMAT_ERROR"; 
		case NMTK_STATE_TRUNCATED        : return "NMTK_STATE_TRUNCATED"; 
		case NMTK_STATE_TYPE_SYS_ERROR   : return "NMTK_STATE_TYPE_SYS_ERROR"; 
		case NMTK_STATE_RESOLVER_ERROR   : return "NMTK_STATE_RESOLVER_ERROR"; 
		case NMTK_STATE_SOCKET_ERROR     : return "NMTK_STATE_SOCKET_ERROR"; 
		case NMTK_STATE_RTP_SOCKET_ERROR : return "NMTK_STATE_RTP_SOCKET_ERROR"; 
		case NMTK_STATE_FILE_OPEN_ERROR  : return "NMTK_STATE_FILE_OPEN_ERROR"; 
		case NMTK_STATE_FILE_READ_ERROR  : return "NMTK_STATE_FILE_READ_ERROR"; 
		default:
			sprintf(gStatusText, "Unknown: 0x%08lx", fStatus);
	} // switch			
	return gStatusText;
} // GetStateText

static long long GetPos() {
	media_clock_t lPos;
	if (gState != ERF_STATE_PLAYING || !gReqSpeed || !gTimeline.timescale)
		return gReqPos; 	
	if (!gDecoder || decypher_get_position(gDecoder, &lPos) < 0)
		return gReqPos;
	return gReqPos = lPos;
} // GetPos

static long long GetDuration() {
	if (!gTimeline.timescale)
		return -1; 	
	return gDuration;
} // GetDuration

static void SendState(user_data_t data) {
	hd_packet_erf_info_t lPacket;
	memset(&lPacket, 0, sizeof(lPacket));
	lPacket.header.type        = HD_PACKET_ERF_INFO;
	lPacket.header.packet_size = sizeof(lPacket);
	lPacket.state = gState;
	lPacket.error = gError;
	lPacket.speed = gReqSpeed;
	lPacket.flags = gFlags;
	lPacket.pos   = GetPos();
	lPacket.size  = GetDuration();

	if(!shm_send(&lPacket))
		syslog(LOG_WARNING, "erf: SendState FAILED");
	if (!gInfoTimer || !gDispatcher)
		return;
	time_ref lExpires;
	time_future(&lExpires, INFO_INTERVAL);
	dispatcher_mod_timer(gDispatcher, gInfoTimer, &lExpires);
} // SendState

static void ChangeState(int fNewState, long fNewError) {
	if (fNewState == gState && fNewError == gError)
		return;
	gState = fNewState;
	gError = fNewError;
	SendState(USER_DATA_NONE);
} // ChangeState

/***************************************************************************
	erf player functions
 ***************************************************************************/

static void *DispatcherThread(void *data) {
	dispatcher_run((struct dispatcher *)data);
	return NULL;
} // DispatcherThread

static void SeekStart() {
	if (!(gFlags & RA_SUPPORTED) || !gDecoder || !gClient) 
		return; 
	if (gState >= ERF_SEEK_PENDING && gState <= ERF_SEEK_READY)
		return;
	ChangeState(ERF_SEEK_PENDING, 0);
	media_clock_t lStart = gTimeline.start + gReqPos * gTimeline.timescale / 1000000;
	if (lStart >= gTimeline.end)
		lStart = gTimeline.end - 1;
	if (decypher_set_playback(gDecoder, gClient, lStart, gReqSpeed) < 0) {
		syslog(LOG_ERR, "erf: SeekStart decypher_set_playback FAILED %p %p %lld %d", gDecoder, gClient, lStart, gReqSpeed); 
	} else if (gReqSpeed != 0) { 
		return;
	} // if
	ChangeState(ERF_SEEK_READY, 0);
} // SeekStart

static void SeekDone() {
	if (!gDecoder || !gClient)
		return;
	media_clock_t lStart;
	nmtk_client_get_seek_result(gClient, &lStart);
	lStart = (lStart - gTimeline.start) * 1000000 / gTimeline.timescale;
	decypher_reset_position(gDecoder, lStart, lStart, gReqSpeed);
	ChangeState(ERF_SEEK_READY, 0);
	if (!gVideo && !gAudio)
		return;
	decypher_play(gDecoder);
	ChangeState(ERF_STATE_PLAYING, 0);
} // SeekDone

static void OpenStream() {
	if (!gDecoder || !gClient)
		return;
	hd_packet_erf_sinfo_t lPacket;
	struct media_program *lNewProgram = 0;
	struct media_program *lProgs[32]; 
	memset(&lPacket, 0, sizeof(lPacket));
    lPacket.header.type        = HD_PACKET_ERF_SINFO;
    lPacket.header.packet_size = sizeof(lPacket);
	
	int lProgCount = nmtk_client_get_progs(gClient, NULL, lProgs, 32);
	int i;
	for (i = 0; i < lProgCount; ++i) {
		nmtk_client_set_prog_act(gClient, lProgs[i], 0);
		if (lProgs[i]->start_time < 0)
			continue;
		if (gReqProg == DEFAULT_ID || lProgs[i]->id == gReqProg) {
			lNewProgram = lProgs[i];
			break;
		} // if
	} // for

	if(lNewProgram != gProgram) {
		gProgram = lNewProgram;
		if (gProgram->format == FORMAT_M2TS)
			decypher_set_offset_check(gDecoder, 1);
		else
			decypher_set_offset_check(gDecoder, 0);
		if (gFlags & RA_SUPPORTED)
			decypher_set_buffer_delay(gDecoder, 500);
		else if (gProgram->format == FORMAT_ASF)
			decypher_set_buffer_delay(gDecoder, 5000);
		else
			decypher_set_buffer_delay(gDecoder, 1000);
		if (gProgram->metadata_track)
			decypher_set_metadata_track(gDecoder, gProgram->metadata_track);
	} // if
	if (gProgram) {
		track_id_t lTid;
		struct sdp_track *lSt;
		struct media_track *lNewVideo = NULL;
		struct media_track *lNewAudio = NULL;

 		nmtk_client_set_prog_act(gClient, gProgram, 1);
		syslog(LOG_INFO, "erf: Found program list:");
		for (i = 0; (i < lProgCount) && (i < ERF_MAX_PROGS); ++i) {
			lPacket.prog[i].id     = lProgs[i]->id;
			lPacket.prog[i].format = lProgs[i]->format;
			lPacket.prog[i].flags  = ERF_SINFO_SET;
			if(gProgram == lProgs[i])
				 lPacket.prog[i].flags |= ERF_SINFO_PLAYING;
			if(lProgs[i]->ended)
				lPacket.prog[i].flags |= ERF_SINFO_ENDED;
			syslog(LOG_INFO, "erf: id %lu format 0x%08x %s %s", lProgs[i]->id, lProgs[i]->format, lProgs[i]->ended ? "(ended)" : "", gProgram == lProgs[i] ? "<- Playing" : "");
		} // for 
		for (i = 0; i < gProgram->track_count; ++i) {
			lTid = media_track_get_id(gProgram->tracks[i]);
			lSt = sdp_get_track_by_tid(gProgram->sdp, lTid);
			nmtk_client_set_track_act(gClient, gProgram->tracks[i], 0);
			switch (lSt->common_format & FORMAT_TYPE_MASK) {
				case FORMAT_TYPE_VIDEO:
					if (lNewVideo) 
						if (gReqVideo == DEFAULT_ID || lTid != gReqVideo)
							break;
					lNewVideo = gProgram->tracks[i];
					break;
				case FORMAT_TYPE_AUDIO:
					if (lNewAudio) {
						if (gReqAudio == DEFAULT_ID) {
							if (!gAudioLang[0] || !lSt->language[0])
								break;
// Todo: select best default audio format
							if (!lang_test(gAudioLang, lSt->language))
								break; 
						} else if (lTid != gReqAudio)
							break;
					} // if
					lNewAudio = gProgram->tracks[i];
					break;
				default:
					break;
			} // switch
		} // for
		if (lNewVideo)
			gVideo = lNewVideo;
		if (lNewAudio)
			gAudio = lNewAudio;
		if (gVideo)
			nmtk_client_set_track_act(gClient, gVideo, 1);
		if (gAudio)
			nmtk_client_set_track_act(gClient, gAudio, 1);
		if (lNewVideo) 
			decypher_set_video_track(gDecoder, gVideo, 0);
		if (lNewAudio) 
			decypher_set_audio_track(gDecoder, gAudio, 0);

		syslog(LOG_INFO, "erf: Found track list:");
		int lAudio = 0;
		int lVideo = 0;
		for (i = 0; i < gProgram->track_count; ++i) { 
			lTid = media_track_get_id(gProgram->tracks[i]);
			lSt = sdp_get_track_by_tid(gProgram->sdp, lTid);
			switch (lSt->common_format & FORMAT_TYPE_MASK) {
				case FORMAT_TYPE_VIDEO:
					if (lVideo < ERF_MAX_VIDEO) {
						lPacket.video[lVideo].id     = lTid;
						lPacket.video[lVideo].format = lSt->common_format;
						lPacket.video[lVideo].flags  = ERF_SINFO_SET; 
						if(gProgram->tracks[i] == gVideo)
							lPacket.video[lVideo].flags |= ERF_SINFO_PLAYING;
						strncpy(lPacket.video[lVideo].descr, lSt->language, sizeof(lPacket.video[lVideo].descr)-1);
						lVideo++; 
					} // if
					break;
				case FORMAT_TYPE_AUDIO:
					if (lAudio < ERF_MAX_AUDIO) {
						lPacket.audio[lAudio].id     = lTid;
						lPacket.audio[lAudio].format = lSt->common_format;
						lPacket.audio[lAudio].flags  = ERF_SINFO_SET;
						if(gProgram->tracks[i] == gAudio)
							lPacket.audio[lAudio].flags |= ERF_SINFO_PLAYING;
						strncpy(lPacket.audio[lAudio].descr, lSt->language, sizeof(lPacket.audio[lAudio].descr)-1);
						lAudio++; 
					} // if
					break;
				default: break;
			} // switch			
			syslog(LOG_INFO, "erf:      id %lu format %08x %s %s", lTid, lSt->common_format, lSt->language, (gProgram->tracks[i] == gVideo || gProgram->tracks[i] == gAudio) ? "<- Playing" : "");
		} // for
syslog(LOG_INFO, "shm_send");
		shm_send(&lPacket);
syslog(LOG_INFO, "shm_send DONE %d", gState);
		if(gState == ERF_SEEK_READY) {
			decypher_play(gDecoder);
			ChangeState(ERF_STATE_PLAYING, 0);
		} // if
	} // if
	nmtk_client_release_progs(gClient); 
} // OpenStream

static void CloseStream() {
	if(gDecoder)
		decypher_stop(gDecoder);
	gProgram = 0;
	gVideo   = 0;
	gAudio   = 0;
} // CloseStream
 
static void ClientNotify() {
	if (!gClient)
		return;
	long lState = nmtk_client_get_status(gClient).state;
	if (lState & NMTK_STATE_TYPE_FAILURE)
		ChangeState(ERF_STATE_DEMUX_ERROR, lState);
	if (!(lState & NMTK_STATE_TYPE_NORMAL))
		return; 
	if (gState == ERF_STATE_LOADING) {
		ra_flags_t lFlags = 0;
		if (nmtk_client_get_ra_info(gClient, &lFlags, &gTimeline) < 0)
			return; 		
		gFlags = lFlags;
		syslog(LOG_INFO, "erf: Media source is %s\n", gFlags & RA_SUPPORTED ? "seekable" : "not seekable"); 
		if (gTimeline.timescale) 
			syslog(LOG_INFO, "erf: Timeline received: scale: %llu start: %llu end: %llu", gTimeline.timescale, gTimeline.start, gTimeline.end);
		if (gFlags & RA_SUPPORTED) 
			SeekStart();
		else 
			ChangeState(ERF_SEEK_READY, 0);
		if (gTimeline.timescale > 0)
			gDuration = (gTimeline.end - gTimeline.start) * 1000000ULL / gTimeline.timescale; 	
	} else if (gState == ERF_SEEK_PENDING) {
		SeekDone(); 
	} // if
	OpenStream();
} // ClientNotify

static void DecoderNotify() {
	if (!gDecoder)
		return;
	long lState = decypher_get_status(gDecoder).state;
	if (lState & NMTK_STATE_TYPE_FAILURE) {
		CloseStream();
		ChangeState(ERF_STATE_DEC_ERROR, lState); 	
	} else if (lState == DECYPHER_END_OF_STREAM) {
		if (gFlags & RA_SUPPORTED) {
			CloseStream();
			ChangeState(ERF_STATE_FINISHED, 0);
		} else {
			ClientNotify();
		} // if
	} // if
} // DecoderNotify

static void *EventThread(void *data) {
//	syslog(LOG_INFO, "erf: EventThread INIT"); 
	while(!gTerminate) {
		pthread_mutex_lock(&gEventLock);
		while(!gTerminate) {
			if (gDecoderPending || gClientPending)
				break;
			pthread_cond_wait(&gEventSignal, &gEventLock);
		} // while
		pthread_mutex_unlock(&gEventLock);
		if(!gTerminate) {
			pthread_mutex_lock(&gPlayerLock); 
			if (gClientPending)
				ClientNotify();
			if(gDecoderPending)
				DecoderNotify();
			pthread_mutex_unlock(&gPlayerLock); 
		} // if
	} // while
//	syslog(LOG_INFO, "erf: EventThread DONE"); 
	return 0;
} // EventThread

static void EventNotifier(user_data_t data) {
	pthread_mutex_lock(&gEventLock); 
	gDecoderPending = gDecoder ? notifier_get_state(decypher_get_notifier(gDecoder))   : 0;
	gClientPending  = gClient  ? notifier_get_state(nmtk_client_get_notifier(gClient)) : 0;
	if(gDecoderPending || gClientPending)
		pthread_cond_broadcast(&gEventSignal);
	pthread_mutex_unlock(&gEventLock); 
} // EventNotifier

static void StopClient() {
	pthread_mutex_lock(&gPlayerLock); 
	CloseStream();
	if (gClient)
		nmtk_client_close(gClient);
	gClient = 0;
	pthread_mutex_lock(&gEventLock);
	memset(&gTimeline, 0, sizeof(gTimeline));
	ChangeState(ERF_STATE_STOPPED, 0);
	pthread_mutex_unlock(&gEventLock); 
	pthread_mutex_unlock(&gPlayerLock); 
} // StopClient

/***************************************************************************
	erf interface functions
 ***************************************************************************/

void erf_init(hd_packet_erf_init_t const *packet) {
//	syslog(LOG_INFO, "erf: erf_init INIT");
	if (gDispatcher)
		return;
	gTerminate = 0;
	if (!shm_init(ch_stream1_info,ch_stream1_data)) {
		syslog(LOG_ERR, "erf: ERROR: Failed to initialize shm");
		return;
	} // if
	pthread_mutex_init(&gPlayerLock, NULL); 
	pthread_mutex_init(&gEventLock, NULL);
	pthread_cond_init(&gEventSignal, NULL); 
	ChangeState(ERF_STATE_INIT, 0);
	nmtk_init(NMTK_VERBOSE); 
//	asf_parser_init();
	gDispatcher = dispatcher_new(); 
	if (!gDispatcher)
		goto error;
	time_ref lExpires;
	time_future(&lExpires, INFO_INTERVAL);
	gInfoTimer = dispatcher_add_timer(gDispatcher, &lExpires, SendState, USER_DATA_NONE, 0); 	
	if(pthread_create(&gDispatcherThread, NULL, DispatcherThread, gDispatcher)) 
		goto error;
	if(pthread_create(&gEventThread, NULL, EventThread, 0))
		goto error;
	gDecoder = decypher_open(gDispatcher, 0); 
	if (!gDecoder)
		goto error;
	notifier_set_callbacks(decypher_get_notifier(gDecoder), EventNotifier, NULL, USER_DATA_NONE);
//	syslog(LOG_INFO, "erf: erf_init DONE");
 	return;
error:
	erf_done(0);
} // erf_init

void erf_play(hd_packet_erf_play_t const *packet) {
//	syslog(LOG_INFO, "erf: erf_play INIT");
	if (!gDispatcher || !packet) {
		syslog(LOG_ERR, "erf: erf_play ERROR: not inited");
		return;
	} // if
	if (gClient) 
		StopClient();
	pthread_mutex_lock(&gPlayerLock); 
	gReqPos   = 0;
	gReqSpeed = 100;
	gReqFlags = packet->flags;
	strncpy(gAudioLang, packet->lang, sizeof(gAudioLang));
	syslog(LOG_INFO, "erf: playing %s", packet->file_name);
	gClient = nmtk_client_new((char *)packet->file_name, gDispatcher); 	 
	if (!gClient) {
		pthread_mutex_unlock(&gPlayerLock); 
		ChangeState(ERF_STATE_FINISHED, NMTK_STATE_BAD_URI);
		syslog(LOG_ERR, "erf: erf_play ERROR: failed to create client for %s", packet->file_name);
		return;
	} // if
	ChangeState(ERF_STATE_LOADING,0);
	notifier_set_callbacks(nmtk_client_get_notifier(gClient), EventNotifier, NULL, USER_DATA_NONE);
	nmtk_client_start(gClient);	
//	syslog(LOG_INFO, "erf: erf_play DONE");
	pthread_mutex_unlock(&gPlayerLock); 
} // erf_play

void erf_cmd(hd_packet_erf_cmd_t const *packet) {
//	syslog(LOG_INFO, "erf: erf_cmd INIT");
	if(!gDispatcher || !packet) {
		syslog(LOG_ERR, "erf: erf_cmd ERROR: not inited\n");
		return;
	} // if
	pthread_mutex_lock(&gPlayerLock); 
 	switch (packet->cmd) {
		case ERF_CMD_REL:
			gReqPos = GetPos(); 			
			if (gReqPos + packet->offset < 0) 
				gReqPos = 0;
			else
				gReqPos += packet->offset;
			gReqSpeed = packet->speed;
			SeekStart(); 
			break;
		case ERF_CMD_ABS:
			gReqPos   = packet->offset;
			gReqSpeed = packet->speed;
			SeekStart(); 
			break;
		case ERF_CMD_PROG:
			gReqProg  = packet->id;
			gReqSpeed = 100;
			gReqPos   = GetPos(); 
			CloseStream();
			if (gFlags & RA_SUPPORTED) 
				SeekStart();
			else 
				OpenStream(); 
			break;
			break;
		case ERF_CMD_VIDEO:
			gReqVideo = packet->id;
			gReqSpeed = 100;
			gReqPos   = GetPos(); 
			CloseStream();
			if (gFlags & RA_SUPPORTED) 
				SeekStart();
			else 
				OpenStream(); 
			break;
			break;
		case ERF_CMD_AUDIO:
			gReqAudio = packet->id;
			gReqSpeed = 100;
			gReqPos   = GetPos(); 
			CloseStream();
			if (gFlags & RA_SUPPORTED) 
				SeekStart();
			else 
				OpenStream(); 
			break;
		default:
			syslog(LOG_WARNING, "erf: erf_cmd ERROR: unknown cmd %d", packet->cmd);
			break;
	} // switch	
	pthread_mutex_unlock(&gPlayerLock); 
//	syslog(LOG_INFO, "erf: erf_cmd DONE");
} // erf_cmd

void erf_done(hd_packet_erf_done_t const *packet) {
//	syslog(LOG_INFO, "erf: erf_done INIT");
	StopClient();
	
	gTerminate = 1;
	if(gInfoThread)
		pthread_join(gInfoThread, NULL);
	gInfoThread = 0;

	if(gEventThread) 
		pthread_join(gEventThread, NULL);
	if(gDispatcher)
		dispatcher_exit(gDispatcher);
	if(gDispatcherThread) 
		pthread_join(gDispatcherThread, NULL);
	gDispatcherThread = 0;
	if(gDispatcher)
		dispatcher_del(gDispatcher);
	gDispatcher = 0;
	if(gDecoder)
		free(gDecoder);
	gDecoder = 0;
	ChangeState(ERF_STATE_DONE, 0);
	shm_done();
	pthread_cond_destroy(&gEventSignal);
	pthread_mutex_destroy(&gEventLock);
	pthread_mutex_destroy(&gPlayerLock);
//	syslog(LOG_INFO, "erf: erf_done DONE");
} // erf_done
