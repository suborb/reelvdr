// ac3_parser_types.h

#ifndef AC3_PARSER_TYPES_H_INCLUDED
#define AC3_PARSER_TYPES_H_INCLUDED

#include "audio_parser_types.h"

typedef struct ac3_parser_s ac3_parser_t;

struct ac3_parser_s
{
    audio_parser_t aparser;

    struct abuf_s *(*get_frame)(ac3_parser_t *self); // State pattern.

    size_t frame_size;
};

#endif // AC3_PARSER_TYPES_H_INCLUDED
