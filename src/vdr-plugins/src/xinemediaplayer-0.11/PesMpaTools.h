#ifndef PES_MPA_TOOLS_H
#define PES_MPA_TOOLS_H

#include <vdr/tools.h> //uchar 



static const int mpa_bitrate_table[2][3][15] = {
	    {{0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
     {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384 },
     {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 }},
     {{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
     {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
     {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}}
};

static const int mpa_samplerate_table[] = {44100, 48000, 32000, -1};

typedef struct mpa_frame_s
{
	size_t len;
	int layer;
	int id;

	int lsf;  // low freq sampling
	int mp25; // mpeg 2.5
    
	/*bit rate index*/
	int bri;
	int bitrate;
    
	/*sample rate index*/
	int sri;
	int samplerate; 

	int samples_per_frame;

	int padding;
	int no_protection;

//DL	float duration;
    
} mpa_frame_t;


int AddPesAudioHeader(uchar* Data, size_t len, uint64_t pts);
int find_valid_mpa_frame(const uchar* data, size_t len, mpa_frame_t* frame);

#endif //PES_MPA_TOOLS_H	
