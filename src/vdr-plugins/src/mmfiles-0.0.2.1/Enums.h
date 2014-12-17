#ifndef PLUGINS_ENUMS_H
#define PLUGINS_ENUMS_H

// enumeration for meta data : id3 etc
enum {meta_genre, meta_artist, meta_album, meta_comment, meta_desc};

// file type
enum File_t { eUnknownFile, eAudioFile,ePictureFile, eVideoFile, eNotShownFile, 
    eEmptyDir, eNormalDir, eVdrRecDir, eDvdDir, eNotShownDir };

// Cache Type
enum { eMainCache=0, eAudioCache, eVideoCache,ePictureCache, eDvdCache, eVdrRecCache};

// Display Type: List, Directory or Genre mode
enum{ eNormalMode, eDirBrowseMode, eGenreMode };

// Search Type
enum {st_none=0, st_text, st_date, st_length, st_path, st_path_starts_with, st_metadata};


// Sort Type
enum { no_sort = 0, text_a2z_sort, text_z2a_sort, number_asc_sort, number_desc_sort, date_old_first_sort, date_new_first_sort};

#endif
