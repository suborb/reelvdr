#ifndef _MMFILES_SETUP_H
#define _MMFILES_SETUP_H

#include "mmfiles.h"
#include <vdr/menuitems.h>
#include <vector>

class cMMfilesSetup : public cMenuSetupPage 
{
    protected:
        std::vector<char *> DirList;
        virtual void Store();
        unsigned int NumDirs;
    public:
        cMMfilesSetup();
        eOSState ProcessKey(eKeys key);
        void Set();
        void LoadDirList();
};

#endif
