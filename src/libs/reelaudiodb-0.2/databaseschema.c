#include "databaseschema.h"
#include <stddef.h>

const DB_column databaseSchema [] = {
    { "path"       , "VARCHAR(512) PRIMARY KEY NOT NULL"    }, // abs. path including filename
    { "available"  , "VARCHAR(2)"      }, // if the file is available

    // file info
    { "filename"   , "VARCHAR(255)"    },      // filename excluding the path
    { "sha1"       , "VARCHAR(40)"     }, // in hex
    { "size"       , "VARCHAR(10)"},
    { "mtime"      , "VARCHAR(10)"},
    { "mimetype"   , "VARCHAR(10)"},

    // media properties
    { "bitrate"    , "VARCHAR(10)"},
    { "samplerate" , "VARCHAR(10)"},
    { "length"     , "VARCHAR(10)"},
    { "format"     , "VARCHAR(10)"},
    { "codec"      , "VARCHAR(10)"},

    // metadata
    { "title"      , "VARCHAR(255) DEFAULT ' '"},
    { "artist"     , "VARCHAR(64) DEFAULT ' '"},
    { "album"      , "VARCHAR(255) DEFAULT ' '"},
    { "genre"      , "VARCHAR(64) DEFAULT ' '"},
    { "year"       , "VARCHAR(4)"},
    { "comment"    , "VARCHAR(255)"},

    // flags
    { "metadata_needs_update", "VARCHAR(2) DEFAULT \"1\""},
    { NULL         , NULL              }
};


