// ac3_parser.c

#include "ac3_parser.h"

#define ABUFFER_MIN_FULLNESS                 128
#define AC3_DECODED_FRAME_SAMPLE_COUNT  (6 * 256)
#define AC3_NUM_FRMSIZECODS                   38

static int const ac3_fscod_table[3] = {48000, 44100, 32000};

struct ac3size_s
{
    short kbps;
    short word_count[3];
};

typedef struct ac3size_s ac3size_t;

static ac3size_t const ac3_frmsizecod_table[AC3_NUM_FRMSIZECODS] =
{
    { 32  ,{64   ,69   ,96   } },
    { 32  ,{64   ,70   ,96   } },
    { 40  ,{80   ,87   ,120  } },
    { 40  ,{80   ,88   ,120  } },
    { 48  ,{96   ,104  ,144  } },
    { 48  ,{96   ,105  ,144  } },
    { 56  ,{112  ,121  ,168  } },
    { 56  ,{112  ,122  ,168  } },
    { 64  ,{128  ,139  ,192  } },
    { 64  ,{128  ,140  ,192  } },
    { 80  ,{160  ,174  ,240  } },
    { 80  ,{160  ,175  ,240  } },
    { 96  ,{192  ,208  ,288  } },
    { 96  ,{192  ,209  ,288  } },
    { 112 ,{224  ,243  ,336  } },
    { 112 ,{224  ,244  ,336  } },
    { 128 ,{256  ,278  ,384  } },
    { 128 ,{256  ,279  ,384  } },
    { 160 ,{320  ,348  ,480  } },
    { 160 ,{320  ,349  ,480  } },
    { 192 ,{384  ,417  ,576  } },
    { 192 ,{384  ,418  ,576  } },
    { 224 ,{448  ,487  ,672  } },
    { 224 ,{448  ,488  ,672  } },
    { 256 ,{512  ,557  ,768  } },
    { 256 ,{512  ,558  ,768  } },
    { 320 ,{640  ,696  ,960  } },
    { 320 ,{640  ,697  ,960  } },
    { 384 ,{768  ,835  ,1152 } },
    { 384 ,{768  ,836  ,1152 } },
    { 448 ,{896  ,975  ,1344 } },
    { 448 ,{896  ,976  ,1344 } },
    { 512 ,{1024 ,1114 ,1536 } },
    { 512 ,{1024 ,1115 ,1536 } },
    { 576 ,{1152 ,1253 ,1728 } },
    { 576 ,{1152 ,1254 ,1728 } },
    { 640 ,{1280 ,1393 ,1920 } },
    { 640 ,{1280 ,1394 ,1920 } }
};

static inline bool parse(ac3_parser_t *self)
{
    // ac3-syncword 0x0B77
    while (audio_parser_find_byte(&self->aparser,
                                  0x0B,
                                  ABUFFER_MIN_FULLNESS))
    {
        uint8_t const *p = self->aparser.abuf->data.buf;

        if (p[1] == 0x77)
        {
            uint8_t fscod_frmsizecod = p[4];
            uint8_t fscod            = fscod_frmsizecod >> 6;
            uint8_t frmsizecod       = fscod_frmsizecod & 0x3F;

            if (fscod < 3
                && frmsizecod < AC3_NUM_FRMSIZECODS)
            {
                self->aparser.abuf->samplerate = ac3_fscod_table[fscod];
                self->frame_size               = ac3_frmsizecod_table[frmsizecod].word_count[fscod] * 2;
                return true;
            }
        }

        audio_parser_skip(&self->aparser, 1);
    }
    return false;
}

static abuf_t *ac3_parser_get_frame2(ac3_parser_t *self)
{
    if (audio_parser_fill(&self->aparser, self->frame_size))
    {
        self->get_frame = ac3_parser_get_frame1;
        self->aparser.abuf->sample_count = AC3_DECODED_FRAME_SAMPLE_COUNT;
        self->aparser.abuf->media_format = FORMAT_AC3;
        return audio_parser_get_frame(&self->aparser);
    }

    return NULL;
}

abuf_t *ac3_parser_get_frame1(ac3_parser_t *self)
{
    if (parse(self))
    {
        self->get_frame = ac3_parser_get_frame2;
        return ac3_parser_get_frame2(self);
    }
    return NULL;
}

