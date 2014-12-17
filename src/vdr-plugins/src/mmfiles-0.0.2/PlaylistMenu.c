#include "PlaylistMenu.h"
#include "CacheTools.h"
#include "fileInfoMenu.h"
#include "recordingInfoMenu.h"
#include "filesMenu.h"

#include <vdr/interface.h>

cPlaylistMenu::cPlaylistMenu(cPlaylist *pl) : cOsdMenu(tr("Playlist"),2 ,35) // SetCols the same as cFilesMenu:filesMenu.c
{
    plPtr = pl;
    currentElt = 0;
    ShowPlaylist();
}

void cPlaylistMenu::ShowPlaylist()
{
    if(!plPtr) return;
    Clear();

    cCacheItem *item=NULL;
    unsigned int i = 0;

    for(; i < plPtr->Size(); ++i)
    {
        item = plPtr->At(i);
        if (item)
            Add(new cFileOsdItem(item));
    }

    // empty playlist
    if (plPtr->Size() <= 0)
        Add(new cOsdItem(tr("Press GREEN to add files"),osUnknown, false));

    // already seen the changes
    plPtr->ResetChanged();

    SetCurrentElt();
    SetHelpButtons();
    Display();
}

void cPlaylistMenu::SetCurrentElt()
{
    int count = Count();
    
    printf("Count %i; CurrentElt %i\n",count, currentElt);
    if (count<=0) return ; //empty OSD

    cOsdItem *item;

    if (currentElt < count)
        item = Get(currentElt);
    else
        item = Get(currentElt = count-1);
    
    do{
        if (!item) break;

        if (item->Selectable()) 
        { 
            SetCurrent(item);
            break;
        }
        else if (currentElt < count-1) 
            item = Get(++currentElt);
        else break;
        
    }while(1);
}

void cPlaylistMenu::SetHelpButtons()
{
    if (plPtr->Size() <= 0)
        SetHelp(NULL, tr("Add"));
    else
        SetHelp(tr("Unselect"),tr("Add"), tr("Info"), tr("Clear"));
}

eOSState cPlaylistMenu::ProcessKey(eKeys key)
{
    eOSState state = cOsdMenu::ProcessKey(key);

    currentElt = Current();

    if (state == osContinue && (key == kRed || key == (kRed | k_Repeat)))
    {
        ShowPlaylist();
        cFilesMenu::RefreshMenu = true;
    }

    if(state==osUnknown)
    {
        switch(key)
        {
            case kGreen | k_Repeat:
            case kGreen: // Go back to adding files to playlist
                state = osUser2;
                break;

            case kYellow | k_Repeat:
            case kYellow: // file info
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
                state = osContinue;
                break;

            case kBlue | k_Repeat:
            case kBlue: // clear the playlist
                {
                    bool confirm = Interface->Confirm(tr("Clear Playlist ?"));

                    if (confirm)
                    {
                        plPtr->Clear();
                        ShowPlaylist();
                        cFilesMenu::RefreshMenu = true;

                        Skins.Message(mtInfo, tr("Playlist Cleared"));
                    }

                    state = osContinue;
                    break;
                }

            default:
                break;
        }
    }

    if(plPtr->HasChanged()) ShowPlaylist();

    return state;
}
