#include "CacheTools.h"
#include "Enums.h"
#include <vdr/plugin.h>


std::string VdrRecBaseDir(const std::string& filename)
{
    // find '/recordings/' directory
    // else BaseDir is the parent
    size_t idx = filename.find("/recordings/");
    if (idx != std::string::npos) return filename.substr(0,idx+strlen("/recordings"));

    return std::string();

}



////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

/* 
 * creates appropriate cCacheItem
 * and therefore gets added to the correct cache list
 */
bool AddToCache(std::string full_path, int type)
{

    std::string name     = GetLast(full_path);
    std::string parent   = RemoveName(full_path);

    switch( type )
    {
        case eVdrRecDir:
        case eDvdDir:
            new cDirItem(parent, name, type); // donot hold the pointer
            break;

        case eAudioFile:
        case eVideoFile:
        case ePictureFile:
            new cFileItem(parent, name, type);
            break;

        default:
            // uknown type
            new cCacheItem(parent, name, eUnknownFile);
            break;

    }

    return true;
}


bool RemoveFromCache(const std::string& full_path)
{
    return Cache.RemoveItem(full_path);
}

bool CallXinemediaplayer(const std::vector<std::string>& playlist)
{

    if (playlist.size()<=0) return false;

    Xinemediaplayer_Xine_Play_mrl xinePlayData =
    {
        playlist[0].c_str(),
	-1,
        true,
        playlist
    };

    Xinemediaplayer_Set_Xine_Mode xine_mode = {
        Xinemediaplayer_Mode_mmfiles // calls mmfiles back
    }
    cPluginManager::CallAllServices("Set Xine Mode", &xine_mode);
    return cPluginManager::CallAllServices("Xine Play mrl", &xinePlayData);
}
