#ifndef _INCLUDE_PLAYER_DVB_H
#define _INCLUDE_PLAYER_DVB_H

typedef struct
{
	int vpid,apid;
	int fd_video, fd_audio, fd_dvr;
	int fd_dv, fd_da;
	int tmp_vcomp,tmp_acomp,tmp_acomp1;
	int vcomp,acomp;
	int framerate;
	int active;
	int output;
	int freeze;
	int trick;
	int apid_bak;
	int audio_delay;
	int audio_dropped;
} dvb_player_t;


typedef struct {
	unsigned int v_pes_frames;
	unsigned int v_displayed_frames;
	unsigned int v_decoded_frames;
	unsigned int v_dropped_frames;
	unsigned int v_invalid_pts;
	unsigned int a_pes_frames;
	unsigned int a_recv_frames;
	unsigned int a_late_frames;
	unsigned int a_dropped_frames;
	unsigned int a_sent_frames;
	unsigned int a_bytes;
} dvb_player_stats_t;

dvb_player_t* new_dvb_player(int output);
void stop_dvb_player(dvb_player_t *dvbp);
void del_dvb_player(dvb_player_t *dvbp);
void dvb_set_ts_data(dvb_player_t *dvbp, int vpid, int apid, int fr);
void dvb_write_ts(dvb_player_t *dvbp, const uint8_t *data, void* kmem, int size);
void dvb_set_es_data(dvb_player_t *dvbp, int vcomp, int acomp);
void dvb_write_es(dvb_player_t *dvbp, uint64_t pts, const uint8_t *data, int size);
void dvb_freeze(dvb_player_t *dvbp);
void dvb_stats(dvb_player_t *dvbp, dvb_player_stats_t *stats);
#endif
