// ac3_parser.h

#ifndef AC3_PARSER_H_INCLUDED
#define AC3_PARSER_H_INCLUDED

#include "audio_parser.h"
#include "ac3_parser_types.h"

abuf_t *ac3_parser_get_frame1(ac3_parser_t *self);

static inline void ac3_parser_open(ac3_parser_t   *self,
                                   audio_player_t *audio_player)
{
    audio_parser_open(&self->aparser, audio_player);
    self->get_frame = ac3_parser_get_frame1;
}

static inline void ac3_parser_close(ac3_parser_t *self)
{
    audio_parser_close(&self->aparser);
    self->get_frame = NULL;
}

static inline void ac3_parser_feed(ac3_parser_t  *self,
                            uint8_t const *data,
                            size_t         size)
{
    audio_parser_feed(&self->aparser, data, size);
}

static inline abuf_t *ac3_parser_get_frame(ac3_parser_t *self)
{
    return self->get_frame(self);
}

#endif // AC3_PARSER_H_INCLUDED
