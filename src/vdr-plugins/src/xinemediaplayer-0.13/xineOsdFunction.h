#ifndef XINEOSDFUNCTION__H
#define XINEOSDFUNCTION__H

#include <vdr/osdbase.h>

class cXineOsdFunction: public cOsdMenu
{
private:
    int playlist_pos;
    int playlist_size; /* use it to decide which options to offer */
    int remember_playlist; /* if false/0, mediaplayer is always opened with empty playlist */
public:
    cXineOsdFunction(int pl_pos, int pl_size);

    void Set();
    eOSState ProcessKey(eKeys Key);

    void OnOk(const char* Text, bool &closeMenu);

}; // class cXineOsdFunction

#endif /*XINEOSDFUNCTION__H*/

