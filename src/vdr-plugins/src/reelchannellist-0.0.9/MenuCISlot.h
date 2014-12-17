#ifndef __MENU_CI_SLOT__H__
#define __MENU_CI_SLOT__H__

#include <vdr/menu.h>
#include "MenuEditChannel.h"

class cOsdMenuCISlot : public cOsdMenu
{
    private:
        cString filterParam;
        bool isFavourites;
        int caids[MAXCAIDS+1];
        cChannel *channel;
    public:
        /* filterParam is a string that is a folder/bouquet name in case isFav= true
           else it is the provider-name
         */
        cOsdMenuCISlot(cString filterParam, bool isFav=false, cChannel *channel_=NULL);

        // initialize caids to reflect the current state
        // eg. if all channels in folder are assigned to "upper slot" then init
        // caids to upper slot
        void InitCaIds();

        void Set();
        eOSState ProcessKey(eKeys key);
};


#endif
