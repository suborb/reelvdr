// mpa_parser_types.h

#ifndef MPA_PARSER_TYPES_H_INCLUDED
#define MPA_PARSER_TYPES_H_INCLUDED

#include "audio_parser_types.h"

typedef struct mpa_parser_s mpa_parser_t;

struct mpa_parser_s
{
    audio_parser_t aparser;

    size_t frame_size;
};

#endif // MPA_PARSER_TYPES_H_INCLUDED
