
#ifndef CFILESMENU_H
#define CFILESMENU_H

#include <vdr/osdbase.h>
#include <vdr/tools.h>

#include <string>

#include "cache.h"
#include "Enums.h"
#include "ExpertMenu.h"

/*
 * displays cFileOsdItem on OSD
 */
class cFilesMenu : public cOsdMenu
{
	private:
                friend class cExpertMenu;

		static int browse_mode;

                static int meta_data_mode;
	
		// present working directory
		static std::string pwdStr;

                // last selected string : kOk
                static std::string lastSelectedStr;
	
public:
    static bool RefreshMenu;
    cFilesMenu();
    ~cFilesMenu();
    
    void Set();
	void Set_NormalMode();
	void Set_DirMode();
	void Set_GenreMode();

    static void ResetBrowseMode();
	
    eOSState ProcessKey(eKeys key);
    eOSState ProcessKey_NormalMode(eKeys key);  // shows all files
    eOSState ProcessKey_DirMode(eKeys key);     // browse mode
    eOSState ProcessKey_GenreMode(eKeys key);   // Genre mode
};

extern std::string currentlySelectedFilePath;
extern int         currentlySelectedFileType;  
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


/*
 * cFileOsdItem handles kOk and starts cacheItem->Play()
 * Also, it sets the appropriate 'title/text' for the media-file that it shows
 */
class cFileOsdItem : public cOsdItem
{
    friend class cFilesMenu;
    cCacheItem *cacheItem_; // not to be deleted by this class
    public:
        cFileOsdItem(cCacheItem*);
        ~cFileOsdItem();
        void Set();
        eOSState ProcessKey(eKeys key);
};
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool AddRecToVdr();

#endif

