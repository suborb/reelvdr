#ifndef XINEOSDFUNCTION__H
#define XINEOSDFUNCTION__H

#include <vdr/osdbase.h>

class cXineOsdFunction: public cOsdMenu
{
private:
    int playlist_pos;
public:
    cXineOsdFunction(int pl_pos);

    void Set();
    eOSState ProcessKey(eKeys Key);

    void OnOk(const char* Text, bool &closeMenu);

}; // class cXineOsdFunction

#endif /*XINEOSDFUNCTION__H*/

