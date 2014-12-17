#include "filesMenu.h"
#include "cache.h"

// Holds the result of search etc. and 
// this is the list that is displayed on OSD
//cCacheList listToDisplay;

cFilesMenu::cFilesMenu() : cOsdMenu(tr("Multimedia Files"))
{
    Set();
}


cFilesMenu::~cFilesMenu()
{
}

eOSState cFilesMenu::ProcessKey(eKeys key)
{
    eOSState state = cOsdMenu::ProcessKey(key);
    if (state != osUnknown) return state;

    // Has CacheList changed ?
    // Redraw Osd
    // Set();
    return state;
}

void cFilesMenu::Set()
{
    Clear();
    // get appropriate cachelist
    std::list<cCacheItem*>::iterator it;

   int i=1; 
    //iterate through the list
    for( it = Cache.cacheList_.begin(); it != Cache.cacheList_.end(); ++it)
    {
        Add(new cFileOsdItem (*it));
    }

    Display();
}

////////////////////////////////////////////////////////////////////////////////
// cFileOsdItem
////////////////////////////////////////////////////////////////////////////////

cFileOsdItem::cFileOsdItem(cCacheItem* cacheItem)
{
    cacheItem_ = cacheItem;
    Set();
}

cFileOsdItem::~cFileOsdItem()
{
}

void cFileOsdItem::Set()
{
    // Get the name of the Item
    SetText (cacheItem_->Name().c_str());
}

eOSState cFileOsdItem::ProcessKey(eKeys key)
{
    eOSState state = cOsdItem::ProcessKey(key);

    switch(key)
    {
        case kOk:
            // Play
            cacheItem_->Play();
            return osContinue;

        default:
            break;
    }
    return state;
}

