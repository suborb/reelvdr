// audio_parser.c

#include "audio_parser.h"

#include <stdio.h>
    // FIXME: remove

void audio_parser_open(audio_parser_t *self,
                       audio_player_t *audio_player)
{
    self->audio_player = audio_player;

    self->abuf     = get_abuffer(audio_player);
    self->fullness = 0;

    self->input_data      = NULL;
    self->input_data_size = 0;
}

void audio_parser_close(audio_parser_t *self)
{
    if (self->audio_player)
    {
        put_abuffer(self->audio_player, self->abuf);

        self->audio_player = NULL;

        self->abuf     = NULL;
        self->fullness = 0;

        self->input_data      = NULL;
        self->input_data_size = 0;
    }
}

inline void audio_parser_fill2(audio_parser_t *self,
                               size_t          fullness)
{
    size_t copy_size;

    AUDIO_PARSER_ASSERT(self->fullness < fullness);
    AUDIO_PARSER_ASSERT(self->input_data_size);

    copy_size = fullness - self->fullness; // > 0

    if (copy_size > self->input_data_size)
    {
        copy_size = self->input_data_size; // > 0
    }

    memcpy(self->abuf->data.buf + self->fullness,
           self->input_data,
           copy_size);

    self->fullness        += copy_size;
    self->input_data      += copy_size;
    self->input_data_size -= copy_size;
}

bool audio_parser_fill(audio_parser_t *self,
                       size_t          fullness)
{
    // Fill feeded data into abuf up to fullness.

    if (self->fullness < fullness)
    {
        if (self->input_data_size)
        {
            // Input data available, fill in.
            audio_parser_fill2(self, fullness);
        }
    }
    else
    {
        return true;
    }

    AUDIO_PARSER_ASSERT(self->fullness <= fullness);
    return self->fullness >= fullness;
}

inline bool audio_parser_find_byte2(audio_parser_t *self,
                                    uint8_t         value)
{
    size_t fullness = self->fullness;

    for (;;)
    {
        uint8_t       *s = self->abuf->data.buf;
        uint8_t const *e = s + self->fullness;
        uint8_t const *p;
        size_t         remaining;

        for (p = s ; p != e; ++p)
        {
            if (*p == value)
            {
                break;
            }
        }

        remaining = e - p;
        if (remaining)
        {
            // Found.
            memmove(s, p, remaining);
            self->fullness = remaining;
            return audio_parser_fill(self, fullness);
        }
        self->fullness = 0;

        if (!audio_parser_fill(self, fullness))
        {
            return false;
        }
    }
}

void audio_parser_skip(audio_parser_t *self,
                       size_t          skip)
{
    uint8_t       *s = self->abuf->data.buf;
    uint8_t const *e = s + skip;

    self->fullness -= skip;

    memmove(s, e, self->fullness);
}

bool audio_parser_find_byte(audio_parser_t *self,
                            uint8_t         value,
                            size_t          fullness)
{
    // Advance to next byte with given value in stream and
    // fill abuf to fullness.
    if (!audio_parser_fill(self, fullness))
    {
        return false;
    }

    if (*self->abuf->data.buf == value) // Shortcut.
    {
        return true;
    }

    return audio_parser_find_byte2(self, value);
}
