/*
 * audio_parse.h
 *        
 *
 * Copyright (C) 2003 Marcus Metzler <mocm@metzlerbros.de>
 *                    Metzler Brothers Systementwicklung GbR
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * General Public License for more details.
 *
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef _AUDIO_PARSE_H_
#define _AUDIO_PARSE_H_

#include <linux/string.h>
#include <stdint.h>
#include <sys/types.h>
#include "types.h"

enum {
	NONE=0, AC3, MPEG_AUDIO, AAC, LPCM, DTS, UNKNOWN
};

//#define MPEG12MIN  5
#define MPEG12MIN  9
#define AC3MIN     7
#define AACMIN     9
#define MPA_MONO   3
#define A52_LFE   16
#define A52_DOLBY 10

#define NOPTS    (0x8000000000000000ULL)
#define MAXPTS   (0x00000001FFFFFFFFULL)
#define SYNCSIZE 20

#define AUDIO_ONLY_DEFAULT_FRAMERATE_DIV 25

struct audio_frame_s;

typedef int (*audio_callback)(struct audio_frame_s *, int, int, uint64_t);
typedef int (*audio_err_callback)(struct audio_frame_s *, int, int);
typedef int (*audio_config)(struct audio_frame_s *, int force);

typedef
struct audio_frame_s{
	int type;
	int sub_type;
	int set;

	int layer;
	int padding;
	int sample_rate;
	int mpg25;
	int lsf;
	uint32_t bit_rate;
	uint32_t framesize;
	uint8_t  nb_channels;
	uint32_t frequency;
	uint32_t bpf;

	audio_callback callback;
	audio_err_callback err_callback;
	uint8_t *mainbuf;
	int mainsize;
	uint64_t lastpts;
	uint64_t curpts;
	uint64_t nextpts;
	void *priv;
	audio_config config;
	u32 error;
	int framep;
	uint8_t *pcmbuf;
	int pcmpos;
	int pcmsize;
	int extra;
} audio_frame_t;


int audio_parse_ac3(uint8_t *buf, int len, audio_frame_t *af, int pusi, uint64_t pts, u32 error);
int audio_parse(uint8_t *buf, int len, audio_frame_t *af, int pusi, uint64_t pts, u32 error);
extern uint64_t audio_parse_base_pts;

#endif /*_AUDIO_PARSE_H_*/
