// mpa_parser.h

#ifndef MPA_PARSER_H_INCLUDED
#define MPA_PARSER_H_INCLUDED

#include "audio_parser.h"
#include "mpa_parser_types.h"

abuf_t *mpa_parser_get_frame1(mpa_parser_t *self);

static inline void mpa_parser_open(mpa_parser_t   *self,
                                   audio_player_t *audio_player)
{
    audio_parser_open(&self->aparser, audio_player);
}

static inline void mpa_parser_close(mpa_parser_t *self)
{
    audio_parser_close(&self->aparser);
}

static inline void mpa_parser_feed(mpa_parser_t  *self,
                                   uint8_t const *data,
                                   size_t         size)
{
    audio_parser_feed(&self->aparser, data, size);
}

abuf_t *mpa_parser_get_frame(mpa_parser_t *self);

#endif // MPA_PARSER_H_INCLUDED
