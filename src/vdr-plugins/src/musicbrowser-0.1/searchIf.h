#ifndef SEARCHIF_H
#define SEARCHIF_H

#include "userIf.h"
#include <vector>

//----------------------------------------------------------------
//-----------------cSearchIf--------------------------------------
//
//----------------------------------------------------------------

class cSearchIf : public cBrowserIf
{
public:
    /*override*/ void EnterState(cBrowserMenu *menu, cSearchDatabase *search, cBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue);
    static cSearchIf *Instance(){return &instance;}

protected:
    cSearchIf();

private:
    // string given to search the database := '*' + '<text>'+ '*'
    std::string searchText;
    char text[256]; // text input by user

    // Group by: Album/Artist/Genre/Title
    cStringList groupByStrs;
    int groupBy;

    static cSearchIf instance;

    std::vector<cOsdItem*> GetOsdItems(cBrowserMenu *menu);
    eOSState OkKey(cBrowserMenu *menu);
    eOSState AnyKey(cBrowserMenu *menu, eKeys key);

    /* search the database and add the appropriate osdItems to the given vector */
    bool AddAlbumOsdItems (cBrowserMenu*menu, std::vector<cOsdItem*>& osdItems);
    bool AddArtistOsdItems(cBrowserMenu*menu, std::vector<cOsdItem*>& osdItems);
    bool AddTitleOsdItems (cBrowserMenu*menu, std::vector<cOsdItem*>& osdItems);
    bool AddGenreOsdItems (cBrowserMenu*menu, std::vector<cOsdItem*>& osdItems);
};


#endif // SEARCHIF_H
