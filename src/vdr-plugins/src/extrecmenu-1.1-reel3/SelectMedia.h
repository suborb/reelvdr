#ifndef SELECTEMEDIA_H
#define SELECTEMEDIA_H

#include <vdr/osdbase.h>
#include <string>

class cSelectMediaMenu: public cOsdMenu
{
    private:
        std::string srcDir;

    public:
        cSelectMediaMenu(std::string);
        eOSState ProcessKey(eKeys);
        void Set();
};

#endif
