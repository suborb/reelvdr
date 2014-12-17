// audio_parser.h

#ifndef AUDIO_PARSER_H_INCLUDED
#define AUDIO_PARSER_H_INCLUDED

#include "audio_parser_types.h"

#include "player_ll.h"

#include <string.h>

#define AUDIO_PARSER_ASSERT REEL_ASSERT

void audio_parser_open(audio_parser_t *self,
                       audio_player_t *audio_player);

void audio_parser_close(audio_parser_t *self);

static inline void audio_parser_feed(audio_parser_t *self,
                              uint8_t const  *input_data,
                              size_t          input_data_size)
{
    // All previous input data must have been consumed.
    AUDIO_PARSER_ASSERT(!self->input_data_size);

    self->input_data      = input_data;
    self->input_data_size = input_data_size;
}

static inline abuf_t *audio_parser_get_frame(audio_parser_t *self)
{
    abuf_t *abuf = self->abuf;

    abuf->length = self->fullness;

    self->abuf     = get_abuffer(self->audio_player);
    self->fullness = 0;

    return abuf;
}

static inline abuf_t *audio_parser_get_frame_slice(audio_parser_t *self, size_t size)
{
    abuf_t *abuf = self->abuf;
    abuf_t *abuf_new;
    size_t end_data=self->fullness;
    AUDIO_PARSER_ASSERT(size <= self->fullness);

    abuf->length = size;

    abuf_new = get_abuffer(self->audio_player);
    self->fullness -= size;
    memcpy(abuf_new->data.buf, abuf->data.buf+end_data-self->fullness, self->fullness);

    self->abuf = abuf_new;

    return abuf;
}

void audio_parser_skip(audio_parser_t *self,
                       size_t          skip);

bool audio_parser_find_byte(audio_parser_t *self,
                            uint8_t         value,
                            size_t          fullness);

bool audio_parser_fill(audio_parser_t *self,
                       size_t          fullness);

#endif // AUDIO_PARSER_H_INCLUDED
