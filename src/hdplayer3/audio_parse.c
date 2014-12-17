/*****************************************************************************
*
* Copyright (C) 2006-2007 Micronas
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 only, as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
* 02110-1301, USA
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************************/

#ifndef __KERNEL__
#include <stdio.h>
#include <string.h>
#else
#include <linux/kernel.h>
#endif
#include <syslog.h>
#include <stdlib.h>

#include "audio_parse.h"
#define AC3_HEADER_SIZE 7
#define DTS_HEADER_SIZE 20
#define MAX_DTS_FRAME    8191
#define DTS_CHANNEL_MASK 0xF

uint64_t audio_parse_base_pts = 0;
unsigned char ac3buffer[25*1024];
int ac3bufferlen =0;
unsigned char dtsbuffer[25*1024];
int dtsbufferlen =0;
u64 dts_pts=0;

const uint16_t bitrates[2][3][15] = {
    { {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
      {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384 },
      {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 } },
    { {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
      {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
      {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
    }
};

const uint16_t freqs[4] = { 44100, 48000, 32000, -1 };

static uint32_t samples[5] = { 384, 1152, 1152, 1536, 0};

unsigned int ac3_bitrates[32] =
    {32,40,48,56,64,80,96,112,128,160,192,224,256,320,384,448,512,576,640,
     0,0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t ac3half[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3,   0, 0, 0, 0};
uint32_t ac3_freq[4] = {480, 441, 320, 0};

static const int ac3_channels[8] = {
        2, 1, 2, 3, 3, 4, 4, 5
};

static int aac_sample_rates[16] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000, 7350,
    0,0,0
};

static int aac_channels[8] = {
	0, 1, 2, 3, 4, 5, 6, 8
};

static uint8_t lfeon[8] = {0x10, 0x10, 0x04, 0x04, 0x04, 0x01, 0x04, 0x01};

static uint64_t add_pts_audio(uint64_t pts, audio_frame_t *aframe)
{
	uint64_t newpts=0;
	uint32_t samp = 0;
	
	if (pts&NOPTS)
		return NOPTS;
	samp = samples[aframe->layer-1];
	if (aframe->type == AAC)
		samp = aframe->mpg25;
	else if (aframe->type == LPCM)
		return pts; // uses other calculation...

	if (!aframe->frequency) {
		aframe->frequency=48000;
		printf("audio_parse: Warning, frequency set to 0!\n");
	}
	newpts = (pts + (samp * 90000UL)/aframe->frequency);
	return (newpts&MAXPTS); //33 bits PTS
}

void calculate_mpg_framesize(audio_frame_t *af)
{
	int frame_size;

	frame_size = af->bit_rate/1000;
	if (!frame_size) return;
	switch(af->layer) {
	case 1:
		frame_size = (frame_size * 12000) / af->sample_rate;
		frame_size = (frame_size + af->padding) * 4;
		break;
	case 2:
		frame_size = (frame_size * 144000) / af->sample_rate;
		frame_size += af->padding;
		break;
	default:
	case 3:
		frame_size = (frame_size * 144000) / (af->sample_rate << af->lsf);
		frame_size += af->padding;
		break;
	}
	af->framesize = frame_size;
}

static int get_ac3_channels(uint8_t *headr)
{
	int flags, acmod, chans;

	acmod = headr[6] >> 5;
	flags = ((((headr[6] & 0xf8) == 0x50) ? A52_DOLBY : acmod) |
		 ((headr[6] & lfeon[acmod]) ? A52_LFE : 0));
	chans = ac3_channels[ flags & 7];
	if ( flags & A52_LFE)
		chans++;
	return chans;
}

static int parse_ac3(audio_frame_t *af, uint8_t *b) {
#if 1
	int frmsizecod = b[4] & 0x3f;
	if (b[0] != 0x0b || b[1] != 0x77 || frmsizecod >=38)
		return -1;
	int half = ac3half[(b[5]>>3)&0xF];
	int acmod = b[6] >> 5;
	int flags = (((b[6] & 0xf8) == 0x50) ? A52_DOLBY : acmod) | ((b[6] & lfeon[acmod]) ? A52_LFE : 0);
	int channels = ac3_channels[flags & 7];
	if(flags & A52_LFE)
		(channels)++;
	af->bit_rate = ac3_bitrates[frmsizecod>>1];
	switch(b[4] & 0xc0) {
		case 0:
			af->sample_rate = af->frequency = 48000 >> half;
			af->framesize = 4 * af->bit_rate;
			break;
		case 0x40:
			af->sample_rate = af->frequency = 44100 >> half;
			af->framesize = 2 * (320 * af->bit_rate / 147 + (frmsizecod & 1));
			break;
		case 0x80:
			af->sample_rate = af->frequency = 32000 >> half;
			af->framesize = 6 * af->bit_rate;
			break;
		default :
			return -1;
	} // switch
	af->bit_rate *= 1000;
	af->nb_channels = channels;
	af->layer = 4;  // 4 for AC3
	return 0;
#else // Old version
	uint8_t frame;
	int half = 0;
	int fr;
		
	if ((b[0]!=0x0b) || (b[1]!=0x77))
		return -1;
	af->layer = 4;  // 4 for AC3
	frame = (b[4]&0x3F);
	af->bit_rate = ac3_bitrates[frame>>1]*1000;
	if (!af->bit_rate )
		return -1;
	half = ac3half[15&(b[5] >> 3)];
	fr = (b[4] & 0xc0) >> 6;
	if (fr >2 )
		return -1;
	af->sample_rate = af->frequency = (ac3_freq[fr] *100) >> half;
		
	switch (b[4] & 0xc0) {
		case 0:
			af->framesize = 4 * af->bit_rate/1000;
			break;
			
		case 0x40:
			af->framesize = 2 * (320 * af->bit_rate / 
					147000 + (frame & 1));
			break;
			
		case 0x80:
			af->framesize = 6 * af->bit_rate/1000;
			break;
	}
		
	if (!af->framesize)
		return -1;
	af->nb_channels=get_ac3_channels(b);
	return 0;
#endif
} // parse_ac3

static const int dts_bitrate_tab[] = { 32000, 56000, 64000, 96000, 112000, 128000, 192000, 224000, 256000, 320000, 384000,
                                       448000, 512000, 576000, 640000, 768000, 896000, 1024000, 1152000, 1280000, 1344000,
                                                                              1408000, 1411200, 1472000, 1536000, 1920000, 2048000, 3072000, 3840000, 1/*open*/, 2/*variable*/, 3/*lossless*/};

static const int dts_samplerate_tab[] = {0, 8000, 16000, 32000, 0, 0, 11025, 22050, 44100, 0, 0, 12000, 24000, 48000, 96000, 192000};
static const uint8_t dts_channel_tab[] = { 1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6, 6, 7, 8, 8};



static int parse_dts(audio_frame_t *af, uint8_t *b) {

	if ((b[0]!=0x7F) && (b[1]!=0xFE) && (b[2]!=0x80) && (b[3]!=0x01)) return -1;
//syslog(LOG_WARNING,"parse DTS\n");

int word_mode = 0;
int be_mode = 0;
int i, j;
	if (b[0] == 0xff && b[1] == 0x1f && b[2] == 0x00 && b[3] == 0xe8 && (b[4] & 0xf0) == 0xf0 && b[5] == 0x07) {
            // 14 le
            } else if (b[0] == 0x1f && b[1] == 0xff && b[2] == 0xe8 && b[3] == 000 && b[4] == 0x07 && (b[5] % 0xf0) == 0xf0) {
            // 14 be
            be_mode = 1;
            } else if (b[0] == 0xfe && b[1] == 0x7f && b[2] == 0x01 && b[3] == 0x80) {
            // 16 le
            word_mode = 1;
            } else if (b[0] == 0x7f && b[1] == 0xfe && b[2] == 0x80 && b[3] == 0x01) {
            // 16 be
            word_mode = 1;
            be_mode = 1;
            } // if
uint32_t wdata[DTS_HEADER_SIZE/4];
for(i = 0, j = 0; i < sizeof(wdata)/sizeof(uint32_t); i++, j+=4) {
    if(be_mode)
	wdata[i] = b[j]<<24|b[j+1]<<16|b[j+2]<<8|b[j+3];
    else
	wdata[i] = b[j+2]<<24|b[j+3]<<16|b[j]<<8|b[j+1];
    if (!word_mode)
	wdata[i] = (wdata[i] & 0x00003fff) | ((wdata[i] & 0x3fff0000) >> 2);
} // for
int nblks   = (wdata[1]>>18)&0x7f;
int frm_len =(nblks+1)*32; // samples per frame
int frm_size = (wdata[1]>>4)&0x3fff;
frm_size = frm_size+1;
if (!word_mode)
    frm_size = frm_size * 8 / 14 * 2;
int chan     = (((wdata[1])&0xf)<<2)|((wdata[2]>>30)&0x3);
int sample   = (wdata[2]>>26)&0xf;
int bit      = (wdata[2]>>21)&0x1f;
int lfe      = (wdata[2]>>9)&0x3;
af->sample_rate = af->frequency =  dts_samplerate_tab[sample];
af->bit_rate    = dts_bitrate_tab[bit];
af->nb_channels   = dts_channel_tab[chan & DTS_CHANNEL_MASK];
if(lfe)
    (af->nb_channels)++;
if(frm_size < 80 || frm_size > MAX_DTS_FRAME)
return -1;

af->framesize = frm_size;
//*num_frames = frm_len;
af->extra = nblks+1;

//syslog(LOG_WARNING," srate %d brate %d ch %d frmsize %d extra %d \n",af->sample_rate,af->bit_rate,af->nb_channels,af->framesize,af->extra);
return 0;
}

static int parse_aac(audio_frame_t *af, uint8_t *b) {
	uint8_t sr=0;
	uint8_t rdb=0;
	uint8_t chan=0;
	if ((b[0]!=0xff) || ((b[1]&0xf6)!=0xf0))
		return -1;
	af->layer = 5; /* using 5 for AAC */
        uint8_t oti = ((b[2]&0xc0) >> 6)+1;
	sr = (b[2] & 0x3c) >> 2;
	if (!(af->frequency = aac_sample_rates[sr]))
		return -1;
	af->sample_rate = af->frequency;
	chan = ((b[2] & 0x01) << 2) | ((b[3] & 0xc0) >> 6);
	af->nb_channels = aac_channels[chan];
	af->framesize = ((b[3] & 0x03) << 11) | (b[4] << 3) | (b[5] >> 5);
	rdb = (b[6] & 0x03);
	af->mpg25 = 1024 * (rdb+1);
	af->bit_rate = af->framesize * 8 * af->frequency/(1024 *(1+rdb)) ;
	unsigned char c0=(oti<<3)|(sr>>1);
	unsigned char c1=(sr<<7)|(chan<<3);
	af->extra=(c0)<<8|(c1);
	int hlen = b[1]&0x01 ? 7:9;
//syslog(LOG_WARNING,"parse aac oti %d sr %d ch %d ex %d  \n",oti,sr,chan,af->extra);
//syslog(LOG_WARNING,"parse aac sr %d ch %d fs %d br %d \n",af->sample_rate,af->nb_channels,af->framesize,af->bit_rate);
	af->pcmpos = hlen;
	return 0;
} // parse_aac

static int parse_lpcm(audio_frame_t *af, uint8_t *b) {
	if ((b[0]&0xf8)!=0xa0)
			return -1;
	if(b[2] || b[3] != 4) // LPCM Header Len/Start of audio frame should be 4
			return -1;
	int sr = 0;
	int br = 0;
	switch((b[5]>>4)&3) {
		case 0: sr = 48000; break;
		case 1: sr = 96000; break;
		case 2: sr = 44100; break;
		case 3: sr = 32000; break;
	} // switch
	switch((b[5]>>6)&3) {
		case 0 : br = 16; break;
		case 1 : br = 20; break;
		case 2 : br = 24; break;
		default: 
			return -1;
	} // switch
	int ch = (b[5]&7)+1;
	af->bpf       = (af->frequency/AUDIO_ONLY_DEFAULT_FRAMERATE_DIV * af->bit_rate * af->nb_channels) / 8; 
	af->framesize = 0;
	if(LPCM != af->type || sr != af->frequency || br != af->bit_rate || ch != af->nb_channels) {
		printf("audio format changed: reset pcm buffer (%d->LPCM %d->%d %d->%d %d->%d)\n", af->type, af->frequency, sr, af->bit_rate, br, af->nb_channels, ch);
		af->pcmpos=0;
	} // if
	af->frequency   = af->sample_rate = sr;
	af->bit_rate    = br;
	af->nb_channels = ch;
	return 0;
} // parse_lpcm

static int parse_mpa(audio_frame_t *af, uint8_t *b) {
#if 1
	int id = (b[1] >> 3) & 3;
	int layer = 4 - ((b[1] >> 1) & 3);
	int bri = b[2]>>4;
	int sri = (b[2]>>2)&3;
	int pad = (b[2]>>1)&1;
	int chan = (b[3]>>6)&3;
	if (b[0] != 0xFF || (b[1] & 0xE0) != 0xe0 || layer > 3 || sri > 2 || !bri || bri >= 15 || id==1)
		return -1;
	af->lsf  = (id==0) || (id==2);
	af->mpg25 = (id==0);
	af->layer = layer;
	af->padding = pad;
	af->nb_channels = (chan == 3) ? 1 : 2;
	af->bit_rate = bitrates[af->lsf][layer-1][bri]*1000;
	af->frequency = af->sample_rate = freqs[sri] >> (af->lsf+af->mpg25);
	int num_frames = 0;
	switch(layer) {
		case 1 : num_frames = 384; break;
		case 2 : num_frames = 1152; break;
		default: num_frames = af->lsf ? 576 : 1152;
	} // switch
	af->framesize = ((num_frames / 8 * af->bit_rate) / af->sample_rate) + pad;
	return 0;
#else // Old version
	int fr =0;
	uint8_t mode;
	int sample_rate_index;
		
	if ((b[0]!=0xff) || ((b[1]&0xf0)!=0xf0))
		return -1;
	af->layer = 4 - ((b[1] & 0x06) >> 1);
	if (af->layer >3)
		return -1;
	if (b[1] & (1<<4)) {
		af->lsf = (b[1] & (1<<3)) ? 0 : 1;
		af->mpg25 = 0;
	} else {
		af->lsf = 1;
		af->mpg25 = 1;
	}
	sample_rate_index = (b[2] >> 2) & 3;
	if (sample_rate_index > 2)
		return -1;
	af->sample_rate = freqs[sample_rate_index] >> (af->lsf + af->mpg25);
	af->padding = (b[2] >> 1) & 1;
	af->bit_rate = bitrates[af->lsf][af->layer-1][(b[2] >> 4 )]*1000;
	fr = (b[2] & 0x0c ) >> 2;
	if (fr==3)
		return -1;
	af->frequency = freqs[fr];
	calculate_mpg_framesize(af);
	if (!af->framesize)
		return -1;
	mode = (b[3] >> 6) & 3;
	if ( mode == MPA_MONO){
		af->nb_channels = 1;
	} else {
		af->nb_channels = 2;
	}
	return 0;
#endif
} // parse_mpa

static int parse_header(audio_frame_t *af, uint8_t *b) 
{
	af->bpf=0;
	af->pcmpos=0;
	switch(af->type){
		case AC3       : return parse_ac3(af, b);
		case MPEG_AUDIO: return parse_mpa(af, b);
		case AAC       : return parse_aac(af, b);
		case LPCM      : return parse_lpcm(af, b);
		case DTS       : return parse_dts(af, b);
	} // switch
	return -1;
}

static int get_audio_type(uint8_t *b, audio_frame_t *af) {
	int prev = af->type;
	int type = NONE;
	int off  = 0;
	if ((b[0]==0x7F) && (b[1]==0xFE) && (b[2]==0x80) && (b[3]==0x01))
		type = DTS;
	else if((b[0]==0xFF) && ((b[1]&0xF0)==0xF0)) 
		type = MPEG_AUDIO; // May also be AAC
	else if((b[0]==0xFF) && ((b[1]&0xF6)==0xF0)) 
		type = AAC;
	else if ((b[0]&0xF0)==0xA0)
		type = LPCM;
	else if ((b[0]==0x0B) && (b[1]==0x77))
		type = AC3;
//	else if ((b[0]&0xF0)==0x80) {
//		type = DTS;
//		off = 4;
//	}
	static int last = -1;
	if((last != type) || (NONE == type)) {
//		printf("Audio detected on pusi %d [%d] (%02x %02x %02x %02x %02x %02x %02x %02x %02x)\n", type, prev, b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8]);
	}
	last = type;
	if(NONE == type)
		return af->type;
	// Verify the detected type
	switch(type){
		case DTS       : if(parse_dts (af, &b[off])) type = NONE; break;
		case MPEG_AUDIO: if(parse_mpa (af, b)) type = AAC;  else break;
		case AAC       : if(parse_aac (af, b)) type = NONE; break;
		case LPCM      : if(parse_lpcm(af, b)) type = NONE; break;
		case AC3       : if(parse_ac3 (af, b)) type = NONE; break;
		default        : type = NONE;
	} // switch
	if (NONE == type) {
//		printf("Audio fallback on pusi %d [%d] off %d (%02x %02x %02x %02x %02x %02x %02x %02x %02x)\n", last, prev, off, b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8]);
		return af->type;
	} // if
	return type;
} // get_audio_type


//remake ac3 && dts, for dvd
int audio_parse_ac3(u8 *inbuf, int inlen, audio_frame_t *af, 
		int pusi, u64 pts, u32 error)
{
u64 lastpts=0;
//if ((inbuf[0]&0xF0)==0x80 && inlen>10)
//    syslog(LOG_WARNING,"%d %X %X %X %X %X %X %X %X %X %X \n",inlen,inbuf[0],inbuf[1],inbuf[2],inbuf[3],inbuf[4],inbuf[5],inbuf[6],inbuf[7],inbuf[8],inbuf[9]);

if(((inbuf[0]&0xF0)==0x80)&& inbuf[0]<0x88 && (inbuf[1]*128-127) < inlen && (inbuf[2]<<8|inbuf[3]) < 3840){//detect DVD VOB AC3 format
//syslog(LOG_WARNING,"AC3 %X %X %X %X %X %d \n",inbuf[0],inbuf[1],inbuf[2],inbuf[3],inbuf[4],inlen);
    dtsbufferlen=0;
    if(ac3bufferlen+inlen-4>25*1024) {//reset if buffer overload
	ac3bufferlen=0;
	return 0;
    }
    memcpy(ac3buffer+ac3bufferlen,(unsigned char *)(inbuf+4),inlen-4);
    int a=0;
    ac3bufferlen+=inlen-4;
    while(a<ac3bufferlen-AC3_HEADER_SIZE){
	if(ac3buffer[a] == 0xb && ac3buffer[a+1]==0x77 && (ac3buffer[a+4] & 0x3f) < 38 && ac3buffer[a+5] < 0x60){ //new frame
	    if(parse_ac3(af, ac3buffer+a)) return 0;
	    int ac3_len=af->framesize;// = a52_syncinfo(ac3buffer+a, &fl, &sample_rate, &bit_rate);
	    if(ac3bufferlen<(ac3_len+a)) break;
	    if(!audio_parse(ac3buffer+a, ac3_len, af, pusi,  lastpts? (pts+=2880):pts, error)) return 0;
	    lastpts=pts;
	    memmove(ac3buffer,ac3buffer+a+ac3_len,ac3bufferlen-ac3_len-a);
	    ac3bufferlen-=a+ac3_len;
	    a=0;
	}else
	    a++;
    }
    return 1;
    } else if(((inbuf[0]&0xF0)==0x80)&& inbuf[0]>=0x88 && (inbuf[1]*128-127) < inlen && (inbuf[2]<<8|inbuf[3]) < 3840){//detect DVD VOB DTS format
//syslog(LOG_WARNING,"DTS %X %X %X %X %X %d \n",inbuf[0],inbuf[1],inbuf[2],inbuf[3],inbuf[4],inlen);
	ac3bufferlen=0;
	if(dtsbufferlen+inlen-4>25*1024) {//reset if buffer overload
    	    dtsbufferlen=0;
	    return 0;
	}
	memcpy(dtsbuffer+dtsbufferlen,(unsigned char *)(inbuf+4),inlen-4);
	int a=0;
	dtsbufferlen+=inlen-4;
//	syslog(LOG_WARNING,"-----dts \n");
//	syslog(LOG_WARNING,"%d %X %X %X %X %X %X %X %X %X %X \n",dtsbufferlen,inbuf[0],inbuf[1],inbuf[2],inbuf[3],inbuf[4],inbuf[5],inbuf[6],inbuf[7],inbuf[8],inbuf[9]);

	while(a<dtsbufferlen-DTS_HEADER_SIZE){
	    if (dtsbuffer[a]==0x7F && dtsbuffer[a+1]==0xFE && dtsbuffer[a+2]==0x80 && dtsbuffer[a+3]==0x01) {
		//if(parse_dts(af, inbuf+4)) return 0;
		if(parse_dts(af, dtsbuffer+a)) return 0;
		int dts_len = af->framesize;

		if(dtsbufferlen<(dts_len+a)) break;
		//if(dts_len>inlen)return 0;
		//syslog(LOG_WARNING,"dts_len %d srate %d \n",af->framesize,af->sample_rate);
		//memcpy(dtsbuffer,(unsigned char*)(inbuf+4),dts_len);
		//syslog(LOG_WARNING,"parse %d %X %X %X %X %X %X %X %X %X %X \n",dts_len,dtsbuffer[0],dtsbuffer[1],dtsbuffer[2],dtsbuffer[3],dtsbuffer[4],dtsbuffer[5],dtsbuffer[6],dtsbuffer[7],dtsbuffer[8],dtsbuffer[9]);
		//if(!audio_parse(inbuf+4, dts_len, af, pusi,  dts_pts?dts_pts+960:pts, error)) return 0;
		//syslog(LOG_WARNING,"parse %d %d %d %d \n",inlen,dts_len,pts&0xFFFFFFFF,(pts>>32)&0xFFFFFFFF);
		//if(!audio_parse(inbuf+4, dts_len, af, pusi,  pts, error)) return 0;
		//if(!dts_pts)dts_pts=pts;
		if(!audio_parse(dtsbuffer+a, dts_len, af, pusi,  lastpts? (pts+=960):pts, error)) return 0;
		lastpts=pts;
		memmove(dtsbuffer,dtsbuffer+a+dts_len,dtsbufferlen-dts_len-a);
		dtsbufferlen-=a+dts_len;
		a=0;
	    }else

	    a++;
	}

	return 1;
    }else {
//syslog(LOG_WARNING,"MPEG2 %X \n",inbuf[0]);
	ac3bufferlen=0;
	dtsbufferlen=0;
	return audio_parse(inbuf, inlen, af, pusi,  pts, error);
    }
    return 0;
}

//old parser
int audio_parse(u8 *inbuf, int inlen, audio_frame_t *af, 
		int pusi, u64 pts, u32 error)
{
	int framet = 24;
	int len;
	if (!(pts & NOPTS)) {
		if (!af->framep && (af->lastpts==pts))
			af->lastpts=pts;
		else
			af->nextpts=pts;
	}
	af->curpts=pts;
	if (error)
		af->error=error;
	while (inlen) {
		if (af->framep<framet) {
			len=framet-af->framep;
			if (len>inlen)
				len=inlen;
			memcpy(af->mainbuf+af->framep, inbuf, len);
			inbuf+=len;
			inlen-=len;
			af->framep+=len;
			continue;
		}
		if (af->framep==framet) {
			int last_type = af->type;
			/*if(pusi) allways check format since we changed the repacker in vdr/remux */
				af->type = get_audio_type(af->mainbuf, af);
			if(NONE == af->type) {
				af->framep=0;
				return 0;
			} // if
			pusi=0;
			if(last_type != af->type)
				af->pcmpos=0;
			if (parse_header(af, af->mainbuf)<0) {
				memmove(af->mainbuf, af->mainbuf+1, --af->framep);
				continue;
			} else {
				pusi=0;
				if(!af->framesize)
					af->framesize = inlen+framet;
				if (!(af->error&2) && af->config) {
					af->config(af, 0);
					af->set=1;
					if (af->framesize>af->mainsize)
						af->framesize=af->mainsize;
				}
			}
		}
		len=af->framesize-af->framep;
		if (len<0) {
			af->framep=0;
			continue;
		}
		if (len>inlen)
			len=inlen;
		memcpy(af->mainbuf+af->framep, inbuf, len);
		inlen-=len;
		inbuf+=len;
		af->framep+=len;
		if (af->framep==af->framesize) {
			if(af->bpf) {
				if(af->pcmpos + af->framesize > af->pcmsize) {
					af->pcmsize = af->pcmpos + af->framesize;
					af->pcmbuf = (uint8_t *)realloc(af->pcmbuf, af->pcmsize);
				} // if
				memcpy(&af->pcmbuf[af->pcmpos], &af->mainbuf[7], af->framesize);
				af->pcmpos += af->framesize-7;
				if(af->pcmpos >= af->bpf) {
					memcpy(af->mainbuf, af->pcmbuf, af->bpf);
					af->pcmpos -= af->bpf;
					memcpy(af->pcmbuf, &af->pcmbuf[af->bpf], af->pcmpos);
					if(af->lastpts == NOPTS)
						af->lastpts = 0;
					if (af->set && af->callback)
						af->callback(af, 0, af->bpf, af->lastpts);
					af->nextpts = af->lastpts+90000/AUDIO_ONLY_DEFAULT_FRAMERATE_DIV;
				} // if
			} else {
				if (af->set && af->callback)
					af->callback(af, af->pcmpos, af->framep-af->pcmpos, af->lastpts);
//				af->lastpts=NOPTS; Don't reset pts - needed for pts calculation (add_pts_audio)
			} // if
			if (!(af->nextpts&NOPTS)) {
				af->lastpts=af->nextpts;
				af->nextpts=NOPTS;
			} else
				af->lastpts = add_pts_audio(af->lastpts, af);
			af->framep=0;
			af->error=0;
		}
	} 
	return 1;
}
