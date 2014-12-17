#ifndef __MMFILES_CLASSIFY_H
#define  __MMFILES_CLASSIFY_H

#include <string>
#include <vector>

enum File_t { eUnknownFile, eAudioFile, eVideoFile, eNotShownFile, 
    eEmptyDir,    eNormalDir, eVdrRecDir, eDvdDir,        eNotShownDir };

void AddDirContentsToCache(const std::string& path, const std::vector<std::string>& files,
        const std::vector<std::string>& subdirs) ;

#endif

