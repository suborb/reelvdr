#ifndef MMFILES_EXPERT_MENU_H
#define MMFILES_EXPERT_MENU_H

#include <vdr/osdbase.h>

class cExpertMenu : public cOsdMenu
{
    private:
        const char *ModeTexts[5];
        int mode;

        void MenuIdx2BrowseMode(int);
        int BrowseMode2MenuIdx();
    public:
        cExpertMenu();
        void ShowOptions();
        eOSState ProcessKey(eKeys);
};

#endif

