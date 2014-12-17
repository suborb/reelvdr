
#ifndef DISPLAYMESSAGE_H
#define DISPLAYMESSAGE_H

#include "reel.h"

/**
 * ReelSkinDisplayMessage
 */

class ReelSkinDisplayMessage : public cSkinDisplayMessage { // TODO
    private:
        cOsd *osd;
        int x0, x1, x2, x3;
        int y0, y1, y2;
        int messageBarLength;
        cFont const *font;
    public:
        ReelSkinDisplayMessage();
        virtual ~ReelSkinDisplayMessage();
        virtual void SetMessage(eMessageType Type, const char *Text);
        virtual void Flush();
};

#endif

