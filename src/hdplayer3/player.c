// player.c

#include "player.h"

#include "channel.h"
#include "hdplayer.h"

//Start by dl
#include "erfplayer.h"
//End by dl

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <linux/videodev.h>
#include <linux/soundcard.h>
#include <syslog.h>
#include <stdarg.h>

#ifndef DISABLE_RPC
//#include <linux/go8xxx.h>
#include <linux/go8xxx-video.h>
//#include <linux/go8xxx-adec.h>
#endif /* DISABLE_RPC */

#include "demux.h"

#include <hdchannel.h>

extern hd_data_t volatile *hda;

//#define DEBUG
/*-------------------------------------------------------------------------*/
void hexdump(uint8_t *x, int len)
{
            int n;
            for(n=0;n<len;n+=16) {
                    printf("%p: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			   x,x[0],x[1],x[2],x[3],x[4],x[5],x[6],x[7],
			   x[8],x[9],x[10],x[11],x[12],x[13],x[14],x[15]);
                    x+=16;
            }
}
/*-------------------------------------------------------------------------*/
player_t *new_player(hd_channel_t *channel, hd_channel_t *info, int n_player)
{
	printf ("\033[0;41m new_player \033[0m\n");
	player_t *pl;

	pl = (player_t*)malloc(sizeof(player_t));
	memset(pl, 0, sizeof(player_t));

	pl->num=n_player;
	pl->channel = channel;
	pl->info = info;

	pl->video_generation = -1;
	pl->shm_player = &hda->player[n_player];

	pl->trickmode = 1;
	pl->trickspeed = 0;
	pl->pause = 0;

	pl->seq_width = 0;
	pl->seq_height = 0;
	pl->seq_aspect_w = 0;
	pl->seq_aspect_h = 0;

	pl->vdec=new_decoder(n_player);
	return pl;
}
/*-------------------------------------------------------------------------*/
void delete_player(player_t *pl)
{
    if(pl->vdec)
	delete_decoder(pl->vdec);
    free(pl);
}
/*-------------------------------------------------------------------------*/

static void put_packet_ts_data(player_t *pl, hd_packet_ts_data_t const *packet, void *kmem)
{
	unsigned int apid = packet->apid;
	unsigned int vpid = packet->vpid;
	uint8_t const *data = (uint8_t const *)packet + sizeof(hd_packet_ts_data_t);
	uint32_t data_size = packet->header.packet_size - sizeof(hd_packet_ts_data_t);
	int old_vpid = pl->vdec->video_id;
	int old_apid = pl->vdec->adec?pl->vdec->adec->audio_id:0;

	/* TB: if only the audio-pid changed, there is no need to reset the decoder */
        if ((apid<=0x1FFF && vpid<=0x1FFF && apid >=0 && vpid >=1 && apid != old_apid && vpid == old_vpid && packet->generation==pl->video_generation)){ 
	    printf("%i: old_apid: %i, apid: %i\n", pl->num, old_apid, apid);
            pl->vdec->adec->audio_id=apid;
	} else if ((apid<=0x1FFF && vpid<=0x1FFF && apid >=0 && vpid >=1 && (apid != old_apid ||vpid != old_vpid)) ||
	    packet->generation!=pl->video_generation)
	{
		printf("%i: old_apid: %i old_vpid: %i, apid: %i, vpid: %i\n", pl->num, old_apid, old_vpid, apid, vpid);
		int tm,ts;
		tm=pl->vdec->trickmode;
		ts=pl->vdec->trickspeed;
		decoder_setup(pl->vdec, vpid, apid);

		// Hack: generation changes with fast fwd/rev
		// Restore trickmodes
		if ((old_apid==apid && old_vpid==vpid)) {
			pl->vdec->trickmode=tm;
			pl->vdec->trickspeed=ts;
		}
	}
	if(data_size>0)		
		vdec_write_ts_block(pl->vdec, (u8 *)data, data_size);
	pl->video_generation=packet->generation;
}
/*-------------------------------------------------------------------------*/
static void put_packet_pes_data(player_t *pl, hd_packet_pes_data_t const *packet)
{
#if OLD_PACKET_PES
	uint8_t const *data = (uint8_t const *)packet + sizeof(hd_packet_pes_data_t);
	uint32_t data_size = packet->header.packet_size - sizeof(hd_packet_pes_data_t);
	int start=0;
	if (*data==0 && *(data+1)==0 && *(data+2)==1 && (*(data+7)&0x80) )
		start=1;
	*(char*)(data+4)=0;
	*(char*)(data+5)=0;

	if (packet->generation!=pl->video_generation){
		int tm,ts;
		tm=pl->vdec->trickmode;
		ts=pl->vdec->trickspeed;
		decoder_setup(pl->vdec, 0, 0);
		pl->vdec->trickmode=tm;
		pl->vdec->trickspeed=ts;

	}
#if 0
	printf("%i %i %i\n",data_size,start,packet->av);
	hexdump(data,32);
#endif
	if (pl->vdec->run_video_out && (data_size > (8-start))) {
		int hlen=*(data+8);
		if (packet->av) {
			if (start)
				vdec_write_pes(pl->vdec, (u8 *)data, data_size, start, TS_PES_VIDEO);
			else if (data_size-(9+hlen)>0) {			       
                            vdec_write_pes(pl->vdec, (u8 *)data+9+hlen, data_size-9-hlen, start, TS_PES_VIDEO);
			}
		}
		else {
		if (pl->vdec->pes[TS_PES_AUDIO].state == MPEGTS_SKIP) {
			set_default_framerate(pl->vdec);
			printf("%i: audio only:",pl->num);
			decoder_stc_init(pl->vdec, 0);
			audio_parse_base_pts = 0;
			start = 1;
			printf("force audio start!\n");
		} // if
		if (start)
                            vdec_write_pes(pl->vdec, (u8 *)data, data_size, start, TS_PES_AUDIO);
			else if (data_size-(9+hlen)>0)
                            vdec_write_pes(pl->vdec, (u8 *)data+9+hlen, data_size-9-hlen, start, TS_PES_AUDIO);
		}
	} else printf("%i: >>>>>>>>>>>>>> data_size = %d <<<<<<<<<<<<<<<<<<<\n", pl->num, data_size);

	pl->video_generation=packet->generation;
#else
	uint8_t const *data = (uint8_t const *)packet + sizeof(hd_packet_pes_data_t);
	uint32_t data_size = packet->header.packet_size - sizeof(hd_packet_pes_data_t);
	int pes_start=0;
	int pts_start=0;
	if (data_size>2 && *data==0 && *(data+1)==0 && *(data+2)==1)
		pes_start=1;
	if (pes_start && data_size>7 && (*(data+7)&0x80) )
		pts_start=1;
	*(char*)(data+4)=0;
	*(char*)(data+5)=0;
	if (packet->generation!=pl->video_generation){
		int tm,ts;
		tm=pl->vdec->trickmode;
		ts=pl->vdec->trickspeed;
		decoder_setup(pl->vdec, 0, 0);
		pl->vdec->trickmode=tm;
		pl->vdec->trickspeed=ts;
		pl->video_generation=packet->generation;
	}
	if (pl->vdec->run_video_out && (data_size > (8-pes_start))) {
		int hlen=*(data+8);
		if (packet->av) {
			int dec_start = 0;//pts_start;
			if(!pl->vdec->trickmode || pl->vdec->trickspeed) dec_start = pts_start;
			if (pl->vdec->pes[TS_PES_VIDEO].state == MPEGTS_SKIP && pes_start && !dec_start && (pl->vdec->video_mode == -1)) {
				int i;
				for(i=9+hlen; i<data_size-4; i++) {
					if(!data[i] && !data[i+1] && 1==data[i+2]) {
//printf("NAL %d (%d) @%d (%d)\n", 0x1F & data[i+3], data[i+3], i-hlen-9, i);
						if(!data[i+3] || ((data[i+3]&0x1f) == 8)) {// 8 = NAL PictureParameterSet, 0 = Mpeg2 PictureStartCode
							dec_start = 1;
							printf("----------Found %s @%d (%d) starting video decoder stream (%s)\n", data[i+3] ? "PictureParameterSet" : "PictureStartCode", i-hlen-9, i, pts_start ? "has pts" : "has NO pts");
							break;
						} // if
					} // if
				} // for
			} // if
			if (pl->vdec->pes[TS_PES_VIDEO].state == MPEGTS_SKIP && !dec_start) {
				printf("----------DROP VIDEO WAIT %s %d\n", pes_start?(pts_start?"PTS":"NO PTS"):"DOES NOT START WITH A PES HEADER!", data_size);
				return;
			}
			if (pts_start)
				vdec_write_pes(pl->vdec, (u8 *)data, data_size, pts_start, TS_PES_VIDEO);
			else if (data_size-(9+hlen)>0) {			       
                            vdec_write_pes(pl->vdec, (u8 *)data+9+hlen, data_size-9-hlen, pts_start, TS_PES_VIDEO);
			}
		} else {
			if (pl->vdec->pes[TS_PES_AUDIO].state == MPEGTS_SKIP && !pes_start) {
				printf("----------DROP AUDIO WAIT %s %d\n", pes_start?(pts_start?"PTS":"NO PTS"):"DOES NOT START WITH A PES HEADER!", data_size);
				return;
			}
			if (pl->vdec->pes[TS_PES_AUDIO].state == MPEGTS_SKIP && pes_start) {
				set_default_framerate(pl->vdec);
				printf("%i: audio only:",pl->num);
				decoder_stc_init(pl->vdec, 0);
				audio_parse_base_pts = 0;
				printf("force audio start!\n");
			} // if
			if (pes_start)
				vdec_write_pes(pl->vdec, (u8 *)data, data_size, pes_start, TS_PES_AUDIO);
			else if (data_size-(9+hlen)>0)
			    vdec_write_pes(pl->vdec, (u8 *)data+9+hlen, data_size-9-hlen, pes_start, TS_PES_AUDIO);
		}
	} else printf("%i: >>>>>>>>>>>>>> data_size = %d <<<<<<<<<<<<<<<<<<<\n", pl->num, data_size);
#endif
}
/*-------------------------------------------------------------------------*/
// Compatibility call
static void put_packet_es_data(player_t *pl, hd_packet_es_data_t const *packet)
{
	uint8_t const *data = (uint8_t const *)packet + sizeof(hd_packet_es_data_t);
	uint32_t data_size = packet->header.packet_size - sizeof(hd_packet_es_data_t);
	int start=0;

	if(pl->vdec->xine_mode) {
		if(packet->stream_id != 0xe0) // Only allow video data for audio only via xine
			return;
		if (!packet->still_frame) // Only allow still picture
			return;
	} else {
		if (packet->generation!=pl->video_generation){
			int tm,ts;
			tm=pl->vdec->trickmode;
			ts=pl->vdec->trickspeed;
			decoder_setup(pl->vdec, 0, 0);
			pl->vdec->trickmode=tm;
			pl->vdec->trickspeed=ts;
	
		}
	} // if
	if (*data==0 && *(data+1)==0 && *(data+2)==1)
		start=1;
	else if (*data==0 && *(data+1)==0 && *(data+2)==0 && *(data+3)==1)
		start=1;

//	if (start) printf("ES-Packet, len %i, type %x, time stamp %x, start %i\n", data_size,packet->stream_id,packet->timestamp, start);

	if (pl->vdec->run_video_out) {
		if (packet->stream_id == 0xe0) {			
			vdec_write_es(pl->vdec, data, data_size, start, packet->timestamp, 0, 0);
			if (packet->still_frame)
				pl->vdec->force_field=1; //WIS_FIELD_NONE
			else
				pl->vdec->force_field=0;
		}
		else if (packet->stream_id == 0xbd) {
			pl->vdec->adec->af.type = AC3;
			adec_push_es(pl->vdec, data, data_size, 0, packet->timestamp, 0);
		}
		else {
			pl->vdec->adec->af.type = MPEG_AUDIO;
			adec_push_es(pl->vdec, data, data_size, 0, packet->timestamp, 0);
		}
	}
	pl->video_generation=packet->generation;
}
/*-------------------------------------------------------------------------*/
// Compatibility call
static void put_packet_af_data(player_t *pl, hd_packet_af_data_t const *packet)
{
	//uint8_t const *data = (uint8_t const *)packet + sizeof(hd_packet_af_data_t);
	//uint32_t data_size = packet->header.packet_size - sizeof(hd_packet_af_data_t);
	// FIXME
}
/*-------------------------------------------------------------------------*/
// Compatibility call
static void put_packet_ac3_data_mpa(player_t *pl, hd_packet_af_data_t const *packet)
{
	// FIXME
}
/*-------------------------------------------------------------------------*/
static void packet_off(player_t *pl, hd_packet_off_t const *packet)
{
	printf("%i: Stop!\n",pl->num);	
	decoder_stop(pl->vdec);
        pl->video_generation=-1;
}

#ifndef DISABLE_RPC

#define DBG_RPC_CMD(format, args...)
//#define DBG_RPC_CMD(format, args...) syslog(LOG_INFO,format, ##args)
//#define DBG_RPC_CMD(format, args...) printf(format"\n", ##args)

#define DBG_RPC_CMD_RESPONSE(format, args...)
//#define DBG_RPC_CMD_RESPONSE(ret,format, args...) if(ret)syslog(LOG_INFO,format, ##args)
//#define DBG_RPC_CMD_RESPONSE(ret,format, args...) syslog(LOG_INFO,format, ##args)
//#define DBG_RPC_CMD_RESPONSE(ret,format, args...) printf(format"\n", ##args)

static char gCmdText[64];
char *get_cmd_text(unsigned int cmd) {
    switch (cmd) {
        case RPC_CMD_ACFG     : return "RPC_CMD_ACFG     ";
        case RPC_CMD_ADATA    : return "RPC_CMD_ADATA    ";
        case RPC_CMD_VCFG     : return "RPC_CMD_VCFG     ";
        case RPC_CMD_VDATA    : return "RPC_CMD_VDATA    ";
		default : sprintf(gCmdText, "Unknown Comand: %d", cmd);
    } // switch
    return gCmdText;
} // get_cmd_text

int rpc_send_response(player_t *pl, hd_packet_rpc_cmd_t *fResponse, unsigned int fCmd, unsigned int fParam1, unsigned int fParam2, unsigned int fSize) {
    DBG_RPC_CMD_RESPONSE(fParam1, "%s (%d %d %d) -> ret %d errno %d (%d)", get_cmd_text(fResponse->cmd), fResponse->param1, fResponse->param2, fResponse->data_size, fCmd, fParam1, fParam2);
    if(!pl->info || !fResponse) 
        return 0;
    fResponse->cmd       = fCmd; 
    fResponse->param1    = fParam1;
    fResponse->param2    = fParam2;
    fResponse->data_size = fSize;
    fResponse->header.packet_size = sizeof(hd_packet_rpc_cmd_t)+fResponse->data_size;
    void *lBuffer;
    int lBufferSize = hd_channel_write_start(pl->info, &lBuffer, fResponse->header.packet_size, 0);
    if (!lBufferSize)
        return 0;
    memcpy(lBuffer, fResponse, fResponse->header.packet_size);
    hd_channel_write_finish(pl->info, lBufferSize);
    return 1;
} // rpc_send_response

static void rpc_init(vdec_t *vdec) {
    vdec->xine_mode = 1;
    decoder_setup ( vdec,0, 0);
} // rpc_init

static void rpc_done(vdec_t *vdec) {
    vdec->xine_mode = 0;
    decoder_setup ( vdec,0, 0);
} // rpc_done

static void rpc_cmd(player_t *pl, hd_packet_rpc_cmd_t *cmd) {
    unsigned char *data = ((unsigned char *)cmd)+sizeof(hd_packet_rpc_cmd_t);
    errno = 0;
    switch (cmd->cmd) {
        case RPC_CMD_ACFG : {
            int ret = -1;
            DBG_RPC_CMD("RPC_CMD_ACFG %d %d %d", cmd->param1, cmd->param2, cmd->data_size);
            if(cmd->data_size >= sizeof(hd_adec_config_t))
                ret = adec_write_cfg(pl->vdec, (hd_adec_config_t *)data);
            else
                errno = EINVAL;
            if(!ret && pl->shm_player)
                pl->shm_player->adec_cfg = *((hd_adec_config_t *)data);
            if(!rpc_send_response(pl, cmd, ret, errno, 0, 0))
                syslog(LOG_WARNING, "RPC_CMD_ACFG: rpc_send_response FAILED");
            break;
        } // RPC_CMD_ACFG
        case RPC_CMD_ADATA :
            DBG_RPC_CMD("RPC_CMD_ADATA %d %d %d", cmd->param1, cmd->param2, cmd->data_size);
            adec_write_data(pl->vdec, ((uint64_t)cmd->param1)<<32|cmd->param2, data, cmd->data_size);
            DBG_RPC_CMD("RPC_CMD_ADATA DONE");
            break;
        case RPC_CMD_VCFG : {
            int ret = -1;
            DBG_RPC_CMD("RPC_CMD_VCFG %d %d %d", cmd->param1, cmd->param2, cmd->data_size);
            if(cmd->data_size >= sizeof(hd_vdec_config_t))
                ret = vdec_write_cfg(pl->vdec, (hd_vdec_config_t *)data);
            else
                errno = EINVAL;
            if(!ret && pl->shm_player)
                pl->shm_player->vdec_cfg = *((hd_vdec_config_t *)data);
            if(!rpc_send_response(pl, cmd, ret, errno, 0, 0))
                syslog(LOG_WARNING, "RPC_CMD_VCFG: rpc_send_response FAILED");
            break;
        } // RPC_CMD_VCFG
        case RPC_CMD_VDATA :
            DBG_RPC_CMD("RPC_CMD_VDATA %d %d %d", cmd->param1, cmd->param2, cmd->data_size);
            vdec_write_data(pl->vdec, ((uint64_t)cmd->param1)<<32|cmd->param2, data, cmd->data_size);
            DBG_RPC_CMD("RPC_CMD_VDATA DONE");
            break;
        default:
            syslog(LOG_WARNING, "rpc_cmd: unknown command %d", cmd->cmd);
    } // switch
} // rpc_cmd

int rpc_check_stillframe(vdec_t *vdec) {
	return vdec_check_stillframe(vdec);
} // rpc_check_stillframe

#endif /* DISABLE_RPC */

/*-------------------------------------------------------------------------*/
static enum {
    MODE_RMM = 0,
    MODE_ERF,
#ifndef DISABLE_RPC
    MODE_RPC,
#endif /* DISABLE_RPC */
} gState = MODE_RMM;

int last_seq;
int last_state = -1;
int run_player(player_t *pl)
{
	hd_packet_header_t const *packet_header;
	void *rcv_buffer;
	int packet_size=0;
	int result = 1;
	void *kmem;
	int tmp1,tmp2;

	if(last_state != gState) {
		syslog(LOG_INFO, "STATE CHANGED %d->%d", last_state, gState);
		last_state = gState;
	} // if
//        syslog(LOG_INFO, "Running....%p %d %p", pl, pl?pl->pause:0, pl?pl->shm_player:0);
	
	tmp1=pl->shm_player->pause;
	if (tmp1!=pl->pause) {
		if(pl->vdec)
			decoder_pause(pl->vdec,tmp1);
		pl->pause=tmp1;
	}

	tmp1=pl->shm_player->trickmode;
	tmp2=pl->shm_player->trickspeed;
	if (tmp1!=pl->trickmode || tmp2!=pl->trickspeed) {
		if(pl->vdec)
			decoder_trick(pl->vdec,  tmp2, tmp1);
		pl->trickmode=tmp1;
		pl->trickspeed=tmp2;
	}
//		Don't ignore all cmds because stream is paused...(special in rpc-mode)
//		if (pl->pause)
//			return 1;
	tmp1 = pl->shm_player->pts_shift;
	tmp2 = pl->shm_player->ac3_pts_shift;
	if(pl->vdec) {
		pl->vdec->pts_shift = 90000LL * tmp1 / 1000;
		pl->vdec->ac3_pts_shift = 90000LL * tmp2 / 1000;
	} // if
	if(pl && pl->vdec && hda && pl->vdec->xine_mode) {
		int64_t diff = (hda->player[0].hde_stc_offset_low&0xFFFFFFFF)+(((int64_t)hda->player[0].hde_stc_offset_high&0xFFFFFFFF)<<32);
		if(diff != pl->vdec->stc_diff) {
			printf("%i: adjust stc offset from 0x%llx to 0x%llx delta %lld\n", pl->num, pl->vdec->stc_diff, diff, pl->vdec->stc_diff-diff);
			pl->vdec->stc_diff = diff;
		} // if
	} // if
	if (hd_channel_read_start_kmem(pl->channel, &rcv_buffer, &kmem, &packet_size, 0)) {
		packet_header = (hd_packet_header_t *)rcv_buffer;
//                syslog(LOG_INFO, "Packet %d", packet_header->type);
		if(HD_PLAYER_MODE_LIVE == hda->active_player[pl->num]) {
//                if(MODE_RMM == gState) { // Use orig player
			// Keep old "pause=ignore anything" in vdr-mode
			if (pl->pause && packet_header->type != HD_PACKET_RPC_INIT)
				return 1;
			switch (packet_header->type)
			{
			case HD_PACKET_TS_DATA:
				put_packet_ts_data(pl, (hd_packet_ts_data_t const *)packet_header, kmem);
				break;
			case HD_PACKET_ES_DATA: 
				//printf("-----run_player:HD_PACKET_ES_DATA------------\n");
				put_packet_es_data(pl, (hd_packet_es_data_t const *)packet_header);
				break;
			case HD_PACKET_PES_DATA:
				put_packet_pes_data(pl, (hd_packet_pes_data_t const *)packet_header);
				break;

			case HD_PACKET_AF_DATA:
//				printf("-----run_player:HD_PACKET_AF_DATA------------\n");
				put_packet_af_data(pl, (hd_packet_af_data_t const *)packet_header);
				break;
			case HD_PACKET_AC3_DATA:
//				printf("-----run_player:HD_PACKET_AC3_DATA------------\n");
				put_packet_ac3_data_mpa(pl, (hd_packet_af_data_t const *)packet_header);
				break; 

			case HD_PACKET_CLEAR:
				syslog(LOG_INFO,"HD_PACKET_CLEAR");
//				decoder_setup(pl->vdec, 0, 0);
				break;

#ifndef DISABLE_RPC
			case HD_PACKET_RPC_INIT:
				syslog(LOG_INFO, "HD_PACKET_RPC_INIT");
				if(sizeof(hd_packet_rpc_init_t) != packet_size) {
					syslog(LOG_ERR, "ERROR: Illigal HD_PACKET_RPC_INIT packet size: %d (%d)", packet_size, sizeof(hd_packet_rpc_init_t));
					break;
				} // if
//                                gState = MODE_RPC;
				hda->active_player[pl->num] = HD_PLAYER_MODE_RPC;
/*
				if(pl->vdec) {
				decoder_stop(pl->vdec);
				delete_decoder(pl->vdec);
				} // if
				pl->vdec = 0;
*/
				rpc_init(pl->vdec);
				syslog(LOG_INFO, "HD_PACKET_RPC_INIT DONE");
#ifdef RPC_SHOW_DEFINES
				showDefines();
#endif /* RPC_SHOW_DEFINES */
				break;
#endif /* DISABLE_RPC */
			case HD_PACKET_OFF:
				packet_off(pl, (hd_packet_off_t const *)packet_header);
				break;
			case HD_PACKET_ERF_PLAY:
			case HD_PACKET_ERF_CMD:
			case HD_PACKET_ERF_DONE:
			case HD_PACKET_RPC_CMD:
			case HD_PACKET_RPC_DONE:
				syslog(LOG_WARNING, "WARN: Packet not supported in normal mode: %d", packet_header->type);
				break;
			default:
				syslog(LOG_WARNING, "WARN: Unknown packet type: %d", packet_header->type);
			}
		} else if (HD_PLAYER_MODE_ERF == hda->active_player[pl->num]) { // Use file player
#ifndef DISABLE_RPC
		} else if (HD_PLAYER_MODE_RPC == hda->active_player[pl->num]) { // Use direct calls
			switch (packet_header->type)
			{
			case HD_PACKET_RPC_CMD:
	//                syslog(LOG_INFO, "HD_PACKET_RPC_CMD");
				rpc_cmd(pl, (hd_packet_rpc_cmd_t *)packet_header);
				break;
			case HD_PACKET_RPC_DONE:
				syslog(LOG_INFO, "HD_PACKET_RPC_DONE");
				if(sizeof(hd_packet_erf_done_t) != packet_size) {
					syslog(LOG_ERR, "ERROR: Illigal HD_PACKET_RPC_DONE packet size: %d (%d)", packet_size, sizeof(hd_packet_rpc_done_t));
					break;
				} // if
				rpc_done(pl->vdec);
				//gState = MODE_RMM;
				hda->active_player[pl->num] = HD_PLAYER_MODE_LIVE;
				hda->osd_dont_touch&=~2; // Clear xine draw bit (prevent no fb osd if vdr crashes)
				if(!pl->vdec)
					pl->vdec = new_decoder(pl->num);
				syslog(LOG_INFO, "HD_PACKET_RPC_DONE DONE");
				break;
			case HD_PACKET_RPC_INIT:
				syslog(LOG_INFO, "HD_PACKET_RPC_INIT within rpc mode");
				rpc_done(pl->vdec);
				rpc_init(pl->vdec);
				break;
			case HD_PACKET_ES_DATA:
				put_packet_es_data(pl, (hd_packet_es_data_t const *)packet_header);
				break;
			case HD_PACKET_TS_DATA:
			case HD_PACKET_TRICKMODE:
			case HD_PACKET_AF_DATA:
			case HD_PACKET_OFF:
			case HD_PACKET_ERF_INIT:
			case HD_PACKET_ERF_PLAY:
			case HD_PACKET_ERF_CMD:
			case HD_PACKET_ERF_DONE:
				syslog(LOG_WARNING, "WARN: Packet not supported in rpc mode: %d", packet_header->type);
				break;
			default:
				syslog(LOG_WARNING, "WARN: Unknown packet type: %d", packet_header->type);
			} // switch
#endif /* DISABLE_RPC */
		} // if
//                syslog(LOG_INFO, "Packet %d PROCESSED", packet_header->type);
		hd_channel_read_finish(pl->channel);
		result = 2;
/*
		if(pl->vdec) {
			hda->player[0].hde_stc=pl->vdec->last_stc;
			hda->player[0].pts_offset=pl->vdec->pts_off;
			hda->player[0].decoder_frames = pl->vdec->decoder_frames;
		}
*/
//                syslog(LOG_INFO, "Packet %d DONE", packet_header->type);
	}
//        syslog(LOG_INFO, "Running....DONE %d %d %d", result, hda->hdp_terminate, hda->hdp_running);
	if(pl && pl->vdec && hda) {
		int n=pl->num;
		hda->player[n].hde_stc=pl->vdec->last_stc;
		hda->player[n].pts_offset=pl->vdec->pts_off;
		hda->player[n].decoder_frames = pl->vdec->decoder_frames;
		if(pl->vdec->xine_mode) {
			u64 stc = vdec_get_stc(pl->vdec);
			hda->player[n].hde_stc_base_low  = stc&0xFFFFFFFF; 
			hda->player[n].hde_stc_base_high = (stc>>32)&0xFFFFFFFF; 
		} else {
			u64 ostc = vdec_get_stc(pl->vdec);
			u64 stc = (((((ostc-pl->vdec->stc_diff)&0x1FFFFFFFFULL)-pl->vdec->pts_off)&0x1FFFFFFFFULL)-pl->vdec->pts_delay)&0x1FFFFFFFFULL;
			if(!pl->vdec->last_decoder_pts) {
				stc = -1; // Stream not valid
			} else if(-1 == pl->vdec->first_pts) {
				stc = pl->vdec->last_decoder_pts ? pl->vdec->last_decoder_pts : -1; // Adec not inited
			} else if(0 == pl->vdec->stc_init) {
				stc = pl->vdec->last_decoder_pts ? pl->vdec->last_decoder_pts : -1; // Vdec not inited
			} else if((stc > pl->vdec->last_decoder_pts) ? (stc-pl->vdec->last_decoder_pts)>900000 : (pl->vdec->last_decoder_pts-stc)>900000) {
				stc = pl->vdec->last_decoder_pts ? pl->vdec->last_decoder_pts : -1; // Mad stc value?
			} // if
			hda->player[n].hde_stc_base_low  = stc&0xFFFFFFFF; 
			hda->player[n].hde_stc_base_high = (stc>>32)&0xFFFFFFFF; 
#if 0
static unsigned long long ltime=0;
if(ltime != GetTime()/1000000LL) {
  printf("STC %lld(0x%llx) first %lld last %lld (%lld) ostc %lld(0x%llx) diff %lld off %lld delay %lld init %d\n", 
              stc, stc, pl->vdec->first_pts, pl->vdec->last_decoder_pts, stc-pl->vdec->last_decoder_pts, ostc, ostc, pl->vdec->stc_diff, pl->vdec->pts_off, pl->vdec->pts_delay, pl->vdec->stc_init);
  ltime = GetTime()/1000000LL;
} // if
#endif
		} // if
		hda->player[n].hde_last_pts=pl->vdec->last_decoder_pts;
	} // if
#ifndef DISABLE_RPC
	if(pl && pl->vdec)
		if(rpc_check_stillframe(pl->vdec))
			result = 2;
#endif
	return result;
}
