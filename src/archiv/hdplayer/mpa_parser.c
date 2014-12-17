// mpa_parser.c

#include "mpa_parser.h"

#include <stdio.h>
    // FIXME: remove

#define LAYER_II_SAMPLE_COUNT 1152

static int layer2_bitrate_table[16] = {     0,
                                        32000,
                                        48000,
                                        56000,
                                        64000,
                                        80000,
                                        96000,
                                       112000,
                                       128000,
                                       160000,
                                       192000,
                                       224000,
                                       256000,
                                       320000,
                                       384000,
                                            0};

static int samplerate_table[4] = {44100, 48000, 32000, 0};
static int channel_table[4]= {2,2,2,1};

abuf_t *mpa_parser_get_frame(mpa_parser_t *self)
{
    while (audio_parser_find_byte(&self->aparser,
                                  0xFF,  // First 8 bit of MPA syncword
                                  4))
    {
        uint8_t const *p = self->aparser.abuf->data.buf;

        if ((p[1] & 0xF8) == 0xF8) // Last 4 Bit of MPA syncword + ID
        {
            int layer = p[1] & 0x06;
            int bitrate_index = p[2] >> 4;
            int samplerate = samplerate_table[(p[2] >> 2) & 0x03];
	    int channels=channel_table[(p[3]>>6)&3];

            if (bitrate_index == 0)
            {
                printf("MPEG Audio free format bitrate not supported!\n");
            }

            switch (layer)
            {
            case 0x06: // Layer I
                printf("MPEG-1 Audio Layer I not supported!\n");
                break;
            case 0x04: // Layer II
                {
                    int bitrate = layer2_bitrate_table[bitrate_index];
                    if (bitrate && samplerate)
                    {
                        int est_frame_size = 144 * bitrate / samplerate;
                        if (est_frame_size + 4 <= ABUFFER_CAPACITY)
                        {
                            // If we get here, the frame seems to be valid and must
                            // be est_frame_size or est_frame_size + 1 bytes long.

                            if (audio_parser_fill(&self->aparser, est_frame_size + 4))
                            {
                                self->aparser.abuf->samplerate = samplerate;
                                self->aparser.abuf->media_format = FORMAT_MPA;
                                self->aparser.abuf->sample_count = LAYER_II_SAMPLE_COUNT;
				self->aparser.abuf->channels = channels;
                                // Look for the start of the next frame

                                if (p[est_frame_size] == 0xFF &&
                                    (p[est_frame_size + 1] & 0xF8) == 0xF8)
                                {
                                    return audio_parser_get_frame_slice(&self->aparser, est_frame_size);
                                }
                                else if (p[est_frame_size + 1] == 0xFF &&
                                         (p[est_frame_size + 2] & 0xF8) == 0xF8)
                                {
                                    return audio_parser_get_frame_slice(&self->aparser, est_frame_size + 1);
                                }

                                // Here: Error, continue to search...
                            }
                            else
                            {
                                // Not enough data for this round.
                                return NULL;
                            }
                        }
                    }
                }
                break;
            case 0x02: // Layer III
                printf("MPEG-1 Audio Layer III not supported!\n");
                break;
            }
        }

        audio_parser_skip(&self->aparser, 1);
    }
    return NULL;
}
