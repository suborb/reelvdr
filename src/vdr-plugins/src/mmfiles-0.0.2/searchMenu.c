#include "searchMenu.h"
#include "search.h"

char cSearchMenu::text_[256];
cSearchMenu::cSearchMenu():cOsdMenu(tr("Search"),14)
{
    ClearParams();

    SearchModeText[eMainCache] = tr("All");
    SearchModeText[eAudioCache] = tr("Audio");
    SearchModeText[eVideoCache] = tr("Video");
    SearchModeText[ePictureCache] = tr("Photo");
    SearchModeText[eDvdCache] = tr("DVD");
    SearchModeText[eVdrRecCache] = tr("Vdr Rec");


    Set();
}


void cSearchMenu::ClearParams()
{
//    text_[0] = 0;
    search_mode_ = cSearchCache::Instance().FileType();
}

void cSearchMenu::Set()
{
    Clear();
    //Add( new cMenuEditStraItem(tr("Search mode", &search_mode, ) )) 
    Add( new cMenuEditStrItem(tr("Search Text"), text_, sizeof(text_), "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890 " ) );
    
    Add( new cMenuEditStraItem(tr("Type"), &search_mode_, 4, SearchModeText));

    SetHelp(NULL,NULL,NULL, tr("Reset"));
    Display();
}

eOSState cSearchMenu::ProcessKey(eKeys key)
{
    eOSState state = cOsdMenu::ProcessKey(key);

    if (state == osUnknown)
        switch(key)
        {
            case kOk: // only if not inside EditItem
                cSearchCache::Instance().SetSearchString(text_);
                //cSearchCache::Instance().SetCacheType(search_mode_);
                cSearchCache::Instance().SetCacheType(eMainCache);
                cSearchCache::Instance().SetFileType(search_mode_);
                cSearchCache::Instance().RunSearch();
                cSearchCache::Instance().SetSearchInSearch();
                return osBack;
           
           // Reset Search params
           case kBlue:
                //ClearParams();
                text_[0]=0;
                search_mode_ = eMainCache;
                Set();
                return osContinue;

            default:
                return state;        
        }
    else // if  state == osContinue
    if (key == kOk || key == kBack) // out of Edit mode
    SetHelp(NULL,NULL,NULL, tr("Reset")); // Draw Help buttons

    return state;
}
