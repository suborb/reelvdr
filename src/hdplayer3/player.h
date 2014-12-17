#ifndef _INCLUDE_PLAYER_H
#define _INCLUDE_PLAYER_H

#include "demux.h"

#include "channel.h"
#include <hdshm_user_structs.h>

extern hd_data_t volatile *hda;

struct player_s
{   
	int num;
	vdec_t *vdec;

	int last_width;
	int last_height;
	int last_aspect_w;
	int last_aspect_h;

	hd_channel_t *channel, *info;

	uint32_t video_generation;

	hd_player_t volatile *shm_player;

	int seq_width, seq_height;
	int seq_aspect_w, seq_aspect_h;

	int trickspeed, trickmode;
	int pause;
};

typedef struct player_s player_t;

player_t *new_player(hd_channel_t *channel, hd_channel_t *info, int n_player);
void delete_player(player_t *pl);
int run_player(player_t *pl);

#endif
