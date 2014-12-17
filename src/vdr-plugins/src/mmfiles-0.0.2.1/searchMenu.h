#ifndef MMFILES_SEARCH_MENU__H
#define MMFILES_SEARCH_MENU__H


#include <vdr/osdbase.h>
#include <vdr/menuitems.h>

#include <time.h>

//const char [][] = { "Text search", "Date search", "duration search" };



class cSearchMenu: public cOsdMenu
{
    private:
        time_t date_;
        int length_;

        int search_mode_;
        const char *SearchModeText[6];
        
        void ClearParams();
    public:
        static char text_[256];
        cSearchMenu();
        void Set();
        eOSState ProcessKey(eKeys);
};


#endif
