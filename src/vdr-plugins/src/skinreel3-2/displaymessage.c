
#include "displaymessage.h"
#include "reel.h"
#include "logo.h"
#include "config.h"

/**
 * ReelSkinDisplayMessage
 */

ReelSkinDisplayMessage::ReelSkinDisplayMessage(void)
{
    struct ReelOsdSize OsdSize;
    ReelConfig.GetOsdSize(&OsdSize);

    font = ReelConfig.GetFont(FONT_MESSAGE);

    messageBarLength = int(OsdSize.w * .7);
    int const lineHeight = font->Height();

#define BarImageHeight 55
    x0 = 0;
    x1 = x0 + 20; //Roundness;
    x3 = x0 + messageBarLength;
    x2 = x3 - 20; //Roundness;

    y0 = 0;
    y2 = y0 + BarImageHeight;

    y1 = (y2 + y0 - lineHeight) / 2; // yText

#ifdef REELVDR
    osd = cOsdProvider::NewTrueColorOsd(OsdSize.x + OsdSize.w/2 - (x3 - x0)/2, OsdSize.y + OsdSize.h - y2, Setup.OSDRandom);
#else
    osd = cOsdProvider::NewTrueColorOsd(OsdSize.x + OsdSize.w/2 - (x3 - x0)/2, OsdSize.y + OsdSize.h - y2, 0);
#endif
    tArea Areas[] = { { x0, y0, x3 - 1, y2 - 1, 32 } };
    osd->SetAreas(Areas, sizeof(Areas) / sizeof(tArea));
    SetImagePaths(osd);
}

ReelSkinDisplayMessage::~ReelSkinDisplayMessage()
{
    delete osd;
}

void ReelSkinDisplayMessage::SetMessage(eMessageType Type, const char *Text)
{
    tColor  colorfg = 0xFFFFFFFF;
    int imgMessageIcon;

   switch(Type)
    {
        case mtStatus:
            imgMessageIcon = imgIconMessageInfo;
            break;
        case mtWarning:
            imgMessageIcon = imgIconMessageWarning;
            break;
        case mtError:
            imgMessageIcon = imgIconMessageError;
            break;
        default:
            imgMessageIcon = imgIconMessageNeutral;
            break;
    }
    colorfg = 0xFFFFFFFF;
    // Draw Message Bar
    DrawUnbufferedImage(osd, "message_bar_left.png", x0,y0,false);
    DrawUnbufferedImage(osd, "message_bar_x.png", x1, y0, false, x2 - x1, 1);
    DrawUnbufferedImage(osd, "message_bar_right.png", x2,y0,false);

    osd->DrawImage(imgMessageIcon, x1,y0+15, true);
#define messageIconW 30
    //display message
    osd->DrawText(x1 + messageIconW, y1, Text, colorfg, clrTransparent, font, x2 - x1 - messageIconW , 0, taCenter);
}

void ReelSkinDisplayMessage::Flush()
{
    if(osd)
       osd->Flush();
}

