// audio_parser_types.h

#ifndef AUDIO_PARSER_TYPES_H_INCLUDED
#define AUDIO_PARSER_TYPES_H_INCLUDED

#include "reel.h"

struct audio_parser_s
{
    struct audio_player_s *audio_player;

    struct abuf_s *abuf;
    size_t         fullness;

    uint8_t const *input_data;
    size_t         input_data_size;
};

typedef struct audio_parser_s audio_parser_t;

#endif // AUDIO_PARSER_TYPES_H_INCLUDED
