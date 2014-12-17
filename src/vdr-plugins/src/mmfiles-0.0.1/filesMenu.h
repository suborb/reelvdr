
#ifndef CFILESMENU_H
#define CFILESMENU_H

#include <vdr/osdbase.h>
#include <vdr/tools.h>

#include "cache.h"

/*
 * displays cFileOsdItem on OSD
 */
class cFilesMenu : public cOsdMenu
{
public:
    cFilesMenu();
    ~cFilesMenu();
    
    void Set();
    eOSState ProcessKey(eKeys key);

};
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


/*
 * cFileOsdItem handles kOk and starts cacheItem->Play()
 * Also, it sets the appropriate 'title/text' for the media-file that it shows
 */
class cFileOsdItem : public cOsdItem
{
    cCacheItem *cacheItem_; // not to be deleted by this class
    public:
        cFileOsdItem(cCacheItem*);
        ~cFileOsdItem();
        void Set();
        eOSState ProcessKey(eKeys key);
};
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


#endif

