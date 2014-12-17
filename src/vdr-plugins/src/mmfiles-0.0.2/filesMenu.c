#include "filesMenu.h"
#include "Playlist.h"
#include "PlaylistTools.h"
#include "PlaylistMenu.h"

#include "ExpertMenu.h"

#include "cache.h"
#include "CacheItems.h"
#include "CacheTools.h"

#include "classify.h"
#include "search.h"

#include "DefaultParams.h"

#include "searchMenu.h"
#include <time.h>

#include "fileInfoMenu.h"
#include "recordingInfoMenu.h"

#include <vdr/videodir.h>

std::string currentlySelectedFilePath = "";
int         currentlySelectedFileType = -1 ;  

bool cFilesMenu::RefreshMenu = false;

cString AddIconToString(const std::string& str, char icon)
{
    return cString::sprintf("%c %s", icon, str.c_str());
}

cString AddDirIcon(const std::string& dirStr)
{
    if (dirStr == "..") return AddIconToString(dirStr, char(128)); // folder up
    else return AddIconToString(dirStr, char(130));
}

std::string RemoveIcon(const std::string& str)
{
   char c2; // second character of string 
   c2 = str.at(1);

   if (c2 == ' ') return str.substr(2,std::string::npos);

   return str;
}

// Holds the result of search etc. and 
// this is the list that is displayed on OSD
//cCacheList listToDisplay;

static cCacheItem *lastSelected=NULL;

//int cFilesMenu::browse_mode = eNormalMode;
int cFilesMenu::browse_mode = eGenreMode;
int cFilesMenu::meta_data_mode = meta_genre;
std::string cFilesMenu::pwdStr = "/";
std::string cFilesMenu::lastSelectedStr = "";

void cFilesMenu::ResetBrowseMode()
{
    //browse_mode = eGenreMode;
    browse_mode = cDefaultParams::Instance().BrowseMode();
}

cFilesMenu::cFilesMenu() : cOsdMenu(tr(tr("Multimedia Files")), 2,35)
{
    // catch Cache changes
    if (cSearchCache::Instance().HasCacheChanged()) {cSearchCache::Instance().SetParamChanged(); cSearchCache::Instance().RunSearch();}

    Set();
}

cFilesMenu::~cFilesMenu()
{
}

eOSState cFilesMenu::ProcessKey_NormalMode(eKeys key)
{
    eOSState state = cOsdMenu::ProcessKey(key);

    if (HasSubMenu() && state == osBack)
    {
        CloseSubMenu();
        return osContinue;
    }else
        if (state != osUnknown) return state;

    static bool  sortByName_order = true; // ascending / desc
    static bool  sortByLength_order = true; // ascending / desc
    static bool  sortByDate_order = true; // ascending / desc
    switch(key)
    {
        // sort by name
        case k0: 
            cSearchCache::Instance().SortByName( sortByName_order = !sortByName_order); // press 0 again inverts order
            SetStatus( sortByName_order? tr("Sorting by Name asc..."):  tr("Sorting by Name desc..."));
            return osContinue;

        // sort by length
        case k1:
            cSearchCache::Instance().SortByLength( sortByLength_order = !sortByLength_order);
            SetStatus( sortByLength_order? tr("Sorting by Length asc..."):  tr("Sorting by Length desc..."));
            return osContinue;

        // sort by date 
        case k2:
            cSearchCache::Instance().SortByDate( sortByDate_order = !sortByDate_order);
            SetStatus( sortByDate_order? tr("Sorting by Date asc..."):  tr("Sorting by Date desc..."));
            return osContinue;

        case k3:
            cSearchCache::Instance().SetSearchPath("/media/hd/recordings/Neu_folder/");
            cSearchCache::Instance().RunSearch();
            return osContinue;
		
        case k9:
            browse_mode = eDirBrowseMode;
            Set();
            return osContinue;

        case k8:
        {
            std::list<std::string> glist = cSearchCache::Instance().MetaDataList(meta_genre);
            std::list<std::string>::iterator it = glist.begin();
            for (;it != glist.end(); ++it)
                printf("'%s' \n", it->c_str());

            return osContinue;
        }



        // search subMenu
        case kGreen:
            AddSubMenu(new cSearchMenu);
            return osContinue;

        case kYellow:
            if (currentlySelectedFileType == eVdrRecDir)
            {
                std::string baseDir = VdrRecBaseDir(currentlySelectedFilePath);
                cRecording *rec = new cRecording(currentlySelectedFilePath.c_str(), baseDir.empty()?NULL:baseDir.c_str());

                AddSubMenu(new cRecordingInfoMenu(rec,false,true));

                delete rec;
            }
            else
                AddSubMenu(new cFileInfoMenu(currentlySelectedFilePath));

            currentlySelectedFilePath = "";
            currentlySelectedFileType = -1;
            return osContinue;
        
        // show all
        case kBlue:
            AddSubMenu(new cExpertMenu() );
            return osContinue;

            if ( cSearchCache::Instance().CacheSize() < Cache.size())
            {
                cSearchCache::Instance().ResetSearchResult();
                cSearchCache::Instance().RunSearch();
                SetStatus(tr("Showing all ..."));
            }
            return osContinue;

        default:
            break;
    }

    // catch Cache changes
    if (cSearchCache::Instance().HasCacheChanged()) {cSearchCache::Instance().SetParamChanged(); cSearchCache::Instance().RunSearch();}
    // redraw OSD  if list has changed
    const cCache& searchResult = cSearchCache::Instance().SearchResult();
    if (searchResult.HasChanged()) Set();

    return state;
}

void cFilesMenu::Set_DirMode()
{
	
	// Clear OSD
	Clear();
	SetStatus(NULL);
        //SetHelp(NULL);
	
	//SetCols(0);
	cCache& searchResult = cSearchCache::Instance().SearchResult();
	std::list<cCacheItem*>::iterator it;
	it = searchResult.cacheList_.begin();
        
        std::string gcd = cSearchCache::Instance().GCD(); // greatest common directory

        if (!StartsWith(pwdStr, gcd)) pwdStr = gcd;
        if (cSearchCache::Instance().CacheSize() <= 0) pwdStr ="/";

	// tmp string
	std::string subDirStr;
	
	// dirs and files at this level
	std::list <std::string> subDirs;
	std::list <cCacheItem*> files;
	
	// Get all sub dirs, 
	for ( ; it != searchResult.cacheList_.end(); ++it)
	{
		subDirStr = GetSubDir( (*it)->FullPath(), pwdStr);
		
		// item is a file
		if (subDirStr.empty()) 
			files.push_back(*it);
		else
			// item does not lie in the pwdStr dir Tree! Should not happen
			if (subDirStr == "!");
		else
			// collect the subdir
			subDirs.push_back(subDirStr);
			
	} // for loop search cache
	cSearchCache::Instance().ResetChanged();
	
	// sort Dirs
	subDirs.sort();
	subDirs.unique();
		
	// first dir is .. 'prev dir'
        if (pwdStr != "/" && pwdStr != gcd)
            subDirs.push_front("..");
		
	
	/* display items :- dirs and files*/
	
        // present Directory
        std::string cwd = pwdStr; 
        if (pwdStr.length() > 45)
        {
            cwd = "...";
            cwd += pwdStr.substr( pwdStr.length()-40); // show last 40 chars
        }
        //Add ( new cOsdItem( cwd.c_str(), osUnknown, false  ));
        //Add ( new cOsdItem( "", osUnknown, false  )); // empty line
        std::string title = cDefaultParams::Instance().MenuTitle() + std::string(": ") + cwd;
        SetTitle(title.c_str());

	// dirs
	std::list<std::string>::iterator it1;
	it1 = subDirs.begin();
	for ( ; it1 != subDirs.end(); ++it1)
		Add( new cOsdItem(*AddDirIcon(it1->c_str()), osUser1, true)); // returns osUser1 on kOk
	
	// files 
	it = files.begin();
	for ( ; it != files.end(); ++it)
		Add( new cFileOsdItem(*it));
	
        /*
        if ( cSearchCache::Instance().CacheSize() < Cache.size())
            SetHelp(NULL, tr("Search"),NULL, tr("Show All"));
        else
            SetHelp(NULL, tr("Search"),NULL, NULL); */
        Display();

        cSearchCache::Instance().ResetChanged();
	
}

void cFilesMenu::Set_GenreMode()
{
    Clear();

    std::string title = cDefaultParams::Instance().MenuTitle() + std::string(" - ");
    // Set title according to meta data mode
    switch(meta_data_mode)
    {
        default: meta_data_mode = meta_genre;
        case meta_genre:
            title = title + tr("Genres");
            break;

        case meta_artist:
            title = title + tr("Artists");
            break;

        case meta_album:
            title = title + tr("Albums");
            break;
    } // switch

    SetTitle(title.c_str());
    SetStatus(NULL);

    int meta_data_type = meta_data_mode;

    cSearchCache& searchCache = cSearchCache::Instance();
    //std::list<std::string> glist = Cache.FullMetaDataList(meta_genre);
    std::list<std::string> glist = cSearchCache::Instance().SearchResult().FullMetaDataList(meta_data_type);
    
    std::list<std::string>::iterator it = glist.begin();
    std::string osdStr;

    for (;it != glist.end(); ++it)
    {
        osdStr = AddIconToString(*it, char(136));

        Add( new cOsdItem( osdStr.c_str() ), osdStr == lastSelectedStr?true:false);
        //
        // Show cache Items if 
        if ( searchCache.SearchType() == st_metadata && searchCache.MetaDataType() == meta_data_type && searchCache.MetaDataStr() == it->c_str())
        {
            std::list<cCacheItem*>::iterator item = searchCache.SearchResult().cacheList_.begin();

            for ( ; item!= searchCache.SearchResult().cacheList_.end(); ++item)
                Add( new cFileOsdItem(*item), *item == lastSelected?true:false );


            if (searchCache.CacheSize()>0)
                Add( new cOsdItem("",osUnknown, false));
        }
    }

    cSearchCache::Instance().ResetChanged();

//    SetHelp(NULL,tr("Search"), NULL, tr("Show All")); 
    Display();
}

void cFilesMenu::Set()
{
	switch(browse_mode)
	{
		
		case eDirBrowseMode:
			return Set_DirMode();

	        case eGenreMode:
                        return Set_GenreMode();

		default: // fall through
		case eNormalMode:
			return Set_NormalMode();
		
	}
	
}

eOSState cFilesMenu::ProcessKey_GenreMode(eKeys key)
{
    if (key == kBack)
    {
        int fileType = cSearchCache::Instance().FileType();
        unsigned int oldCacheSize = cSearchCache::Instance().CacheSize();

        cSearchCache::Instance().ResetSearchResult();
        cSearchCache::Instance().SetFileType(fileType); // remember the old file type
        cSearchCache::Instance().RunSearch();

        if (cSearchCache::Instance().CacheSize() == oldCacheSize) return osBack;

        Set();
        return osContinue;
    }

    eOSState state = cOsdMenu::ProcessKey(key);

    if (state == osUnknown)
        switch(key)
        {
            
            case k8: // change meta data mode
                switch(meta_data_mode)
                {
                    default:
                    case meta_genre:
                        meta_data_mode = meta_artist;
                        break;

                    case meta_artist:
                        meta_data_mode = meta_album;
                        break;

                    case meta_album:
                        meta_data_mode = meta_genre;
                        break;
                }
                Set();
                return osContinue;
                break;

            case k9:
                browse_mode = eNormalMode;
                Set();
                return osContinue;
                break;

                // search subMenu
            case kGreen:
                AddSubMenu(new cSearchMenu);
                return osContinue;

            case kYellow:
                AddSubMenu(new cFileInfoMenu(currentlySelectedFilePath));
                currentlySelectedFilePath = "";
                return osContinue;

                // show all
            case kBlue:
                AddSubMenu(new cExpertMenu() );
                return osContinue;

                if ( cSearchCache::Instance().CacheSize() < Cache.size())
                {
                    cSearchCache::Instance().ResetSearchInSearch();
                    cSearchCache::Instance().ResetSearchResult();
                    cSearchCache::Instance().RunSearch();
                    SetStatus(tr("Showing all ..."));
                }
                return osContinue;

            case kOk:
                printf("GOT '%s'\n",Get(Current())->Text());
                lastSelectedStr = Get(Current())->Text();
                cSearchCache::Instance().SetSearchMetaData( RemoveIcon(lastSelectedStr), meta_data_mode);
                cSearchCache::Instance().RunSearch();
                //browse_mode = eDirBrowseMode;
                Set();
                // start genre search
                return osContinue;
                break;

            default:
                break;
        }

    // catch Cache changes
    if (cSearchCache::Instance().HasCacheChanged()) {cSearchCache::Instance().SetParamChanged(); cSearchCache::Instance().RunSearch();}
    // redraw OSD  if list has changed
    const cCache& searchResult = cSearchCache::Instance().SearchResult();
    if (searchResult.HasChanged()) Set();

    return state;
}


eOSState cFilesMenu::ProcessKey_DirMode(eKeys key)
{
    if (key == kBack)
    {
        std::string oldPwdStr = pwdStr;
        pwdStr = ParentDir(pwdStr);
        Set();
        if (pwdStr == oldPwdStr) return osBack;
        return osContinue;
    }

	eOSState state = cOsdMenu::ProcessKey(key);
	
	// a Dir Item was selected & kOked
	if (state == osUser1)
	{
		std::string nextDir = Get(Current())->Text();
                nextDir = RemoveIcon(nextDir);
		
		// prev dir
		if (nextDir == "..")
			pwdStr = ParentDir(pwdStr);
		else
			pwdStr = pwdStr + nextDir;
		
		Set();
		
		return osContinue;
	}
	
        if (state == osUnknown)
            switch(key)
            {
                case k9:
                    browse_mode = eGenreMode;
                    Set();
                    return osContinue;

                    // search subMenu
                case kGreen:
                    AddSubMenu(new cSearchMenu);
                    return osContinue;

                case kYellow:
                    AddSubMenu(new cFileInfoMenu(currentlySelectedFilePath));
                    currentlySelectedFilePath = "";
                    return osContinue;

                    // show all
                case kBlue:
                    AddSubMenu(new cExpertMenu() );
                    return osContinue;

                    if ( cSearchCache::Instance().CacheSize() < Cache.size())
                    {
                        cSearchCache::Instance().ResetSearchResult();
                        cSearchCache::Instance().RunSearch();
                        SetStatus(tr("Showing all ..."));
                    }
                    return osContinue;
                default:
                    break;
            }

        // catch Cache changes
        if (cSearchCache::Instance().HasCacheChanged()) {cSearchCache::Instance().SetParamChanged(); cSearchCache::Instance().RunSearch();}
        // redraw OSD  if list has changed
        const cCache& searchResult = cSearchCache::Instance().SearchResult();
        if (searchResult.HasChanged()) Set();
	
	return state;
}

eOSState cFilesMenu::ProcessKey(eKeys key)
{
        
    if (HasSubMenu())
    {
        cOsdMenu::ProcessKey(key);
        return osContinue;
    }

	eOSState state;

	switch(browse_mode)
	{
		case eDirBrowseMode:
			state = ProcessKey_DirMode(key);
			break;

                case eGenreMode:
			state = ProcessKey_GenreMode(key);
			break;
		
		default: // fall through
		case eNormalMode:
			state = ProcessKey_NormalMode(key);
			break;
	}
	        // Set Info button here?
        cOsdItem* item = Get(Current());
        if (item && !HasSubMenu())
        {
            std::string text = item->Text();
            if (!text.empty() && text.at(0) == '>' ) // Tick
                SetHelp(tr("Unselect"), tr("Search"), tr("Info"), tr("Expert"));
            else if (text.size()>2 && (text.at(2) == char(139) || text.at(2) == char(129) || text.at(2) == char(140) || text.at(2) == char(141)))
                SetHelp(tr("Select"), tr("Search"), tr("Info"), tr("Expert"));
            else
                SetHelp(NULL, tr("Search"), NULL, tr("Expert"));
        }
        
        //kRed
        if (state == osContinue && (key == kRed || key == (kRed|k_Repeat)) && !HasSubMenu() )
        {
            //Set();

            // Last osd Item, then stay there
            /*
               int current = Current();
               if ( current < Count()-1 ) ++current;
               printf("%i (%i)\n", current, Count());
               cOsdItem *item = Get(current);
               if (item->Selectable())
               {
               DisplayCurrent(false);
               SetCurrent(item);
               DisplayCurrent(true);
               */
            cOsdMenu::ProcessKey(kDown); // go to next item
            item = Get(Current());

         if(item)
         {
             std::string text = item->Text();
            if (!text.empty() && text.at(0) == '>' ) // Tick
                SetHelp(tr("Unselect"), tr("Search"), tr("Info"), tr("Expert"));
            //if (!text.empty() && (text.at(0) == char(139) || text.at(0) == char(129) || text.at(0) == char(140) || text.at(0) == char(141)))
            else if (text.size()>2 && (text.at(2) == char(139) || text.at(2) == char(129) || text.at(2) == char(140) || text.at(2) == char(141)))
                SetHelp(tr("Select"), tr("Search"), tr("Info"), tr("Expert"));
            else
                SetHelp(NULL, tr("Search"), NULL, tr("Expert"));
        }
            //} // if

            //Display();
        }

        if(!HasSubMenu() && RefreshMenu)
        {
            Set();
            RefreshMenu = false;
        }
      
   return state;
}

void cFilesMenu::Set_NormalMode()
{
    printf("%s\n", __PRETTY_FUNCTION__);

    Clear();
    SetStatus(NULL);
    std::string title = cDefaultParams::Instance().MenuTitle() + std::string(" - ") + tr("List Mode");
    SetTitle(title.c_str());

    // get appropriate cachelist
    std::list<cCacheItem*>::iterator it;

    //iterate through the list
    cFileOsdItem *currentlySelected = NULL;
    cFileOsdItem *tmpItem = NULL;
    cCache& searchResult = cSearchCache::Instance().SearchResult();
    for( it = searchResult.cacheList_.begin(); it != searchResult.cacheList_.end(); ++it)
    {
        tmpItem = new cFileOsdItem(*it);
        Add(tmpItem );

        if (!currentlySelected && *it == lastSelected) 
            currentlySelected = tmpItem;
    }

    /* Set Current element */
    
    // current element not found
    if (currentlySelected == NULL && Count()>0 ) 
        SetCurrent( Get(0) );
    // current element found
    if (Count() > 0) 
        SetCurrent(currentlySelected);

	// search found nothing
    if (Count()<=0)
    {
        Add(new cOsdItem(tr("No files found"),osUnknown, false));
        if ( cSearchCache::Instance().CacheSize() < Cache.size())
        {
            Add(new cOsdItem(" ",osUnknown, false));
            Add(new cOsdItem(tr("Press BLUE to show all files "),osUnknown, false));
        }
    }

/*
    if ( cSearchCache::Instance().CacheSize() < Cache.size())
        SetHelp(NULL, tr("Search"),NULL, tr("Show All"));
    else
        SetHelp(NULL, tr("Search"),NULL, NULL); */
    Display();

    cSearchCache::Instance().ResetChanged();
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
    std::string str ;
    if (Cache.Find(cacheItem_))
        str = cacheItem_->GetDisplayString();
    else str = "\tDeleted";
    SetText (str.c_str());
}

eOSState cFileOsdItem::ProcessKey(eKeys key)
{
    eOSState state = cOsdItem::ProcessKey(key);
    // remember last selected
    lastSelected = cacheItem_;
    switch(key)
    {
        case kRed|k_Repeat:
        case kRed:
            /*
            // Add to Playlist
            if (!cacheItem_->InPlaylist())
                CurrentPlaylist.Add(cacheItem_);
            else // Remove from playlist if already present
                CurrentPlaylist.Remove(cacheItem_);
            */
            TogglePlaylistItem(cacheItem_);
            Set();

            return osContinue;
            break;

        case kOk:
            // if CurrentPlaylist.Size() > 0 play it
            if (CurrentPlaylist.Size())
            {
                if (CallXineWithPlaylist())
                    return osEnd;
                else
                {
                    Skins.Message(mtError, tr("Could not play Playlist"));
                    return osContinue;
                }
            }

            // Play
            if (cacheItem_->Play())
            {
                /*if (0 && cacheItem_->Type() == eVdrRecDir)
                    return osReplay;
                else*/
                    return osEnd;
            }
            else
            return osContinue;

        case kYellow:
            currentlySelectedFilePath = cacheItem_ -> FullPath(); 
            currentlySelectedFileType = cacheItem_ -> Type();
            // no osContinue
            break;

        default:
            break;
    }
    return state;
}

bool AddRecToVdr()
{

    /* Check for last call of this function
     * Needed since Recordings.Refresh() is called twice in quick succession!
     */
    static time_t lastCallTime = time(NULL);
    if ( time(NULL) - lastCallTime < 10) { printf("last call less than 10 secs ago. RETURN\n"); return false; }
    lastCallTime = time(NULL);

    // Add all vdr recordings to Vdr's Recordings instance
    std::list<cCacheItem*>::iterator it = dirList.cacheList_.begin();
    
    std::string filename;
    std::string baseDir;

    cRecording *recording;

    for ( ; it != dirList.cacheList_.end(); ++it)
    {
        if ( (*it)->Type() == eVdrRecDir )
        {
            filename = (*it)->FullPath();

            // Recordings in VideoDirectory are 
            // handled by extrecmenu. Skip.
            if (VideoDirectory && StartsWith(filename, VideoDirectory)) continue;

            baseDir = VdrRecBaseDir(filename);

            recording = new cRecording(filename.c_str(), baseDir.empty()?(*it)->Path().c_str(): baseDir.c_str() );
            Recordings.Add(recording);
            Recordings.ChangeState();

        }
    } // for
 return true;
}
