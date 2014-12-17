/*
 * OSD Picture in Picture plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */

#include <vdr/config.h>

#include "setup.h"

const int kColorDepths = 4;
const int kSizes = 11;
const int kFrameModes = 3;
const int kFrameDrops = 4;
const int kShowInfoTypes = 4;
const int kInfoPositions = 4;

const char *ColorDepthItems[] = { NULL, NULL, NULL, NULL, NULL };       // initialized later
const char *ShowInfoItems[] = { NULL, NULL, NULL, NULL, NULL }; // initialized later
const char *InfoPositionItems[] = { NULL, NULL, NULL, NULL, NULL };     // initialized later
const char *FrameDropItems[] = { NULL, NULL, NULL, NULL, NULL };        // initialized later

//pos pipPositions[4] = { {70, 60}, {480, 60}, {70, 350}, {480, 350}, };
// TODO: position according to OSD settings (borders), see code in cOsdPipObject::ProcessImage()
pos pipPositions[4] = { {60, 50},     // Top left, definitions see setup.h
                        {490, 50},    // Top right
                        {490, 360},   // Bottom right
                        {60, 360},    // Bottom left
                      };

cOsdPipSetup OsdPipSetup;

cOsdPipSetup::cOsdPipSetup(void)
{
    PipPosition = kInfoTopRight;
    ShowInfo = 1;
    InfoWidth = 400;
    InfoPosition = kInfoBottomLeft;
}

bool cOsdPipSetup::SetupParse(const char *Name, const char *Value)
{
    if (strcmp(Name, "PipPosition") == 0)
        PipPosition = atoi(Value);
    else if (strcmp(Name, "ShowInfo") == 0)
        ShowInfo = atoi(Value);
    else if (strcmp(Name, "InfoWidth") == 0)
        InfoWidth = atoi(Value);
    else if (strcmp(Name, "InfoPosition") == 0)
        InfoPosition = atoi(Value);
    else
        return false;
    return true;
}

cOsdPipSetupPage::cOsdPipSetupPage(void)
{
    m_NewOsdPipSetup = OsdPipSetup;

    ShowInfoItems[0] = trVDR("no");
    ShowInfoItems[1] = tr("channel only");
    ShowInfoItems[2] = tr("simple");
    ShowInfoItems[3] = tr("complete");

    InfoPositionItems[0] = tr("top left");
    InfoPositionItems[1] = tr("top right");
    InfoPositionItems[2] = tr("bottom right");
    InfoPositionItems[3] = tr("bottom left");

    Add(new cMenuEditStraItem(tr("Show info window"), &m_NewOsdPipSetup.ShowInfo, kShowInfoTypes, ShowInfoItems));
/*
    Add(new cMenuEditIntItem(tr("Info window width"), &m_NewOsdPipSetup.InfoWidth, 200, 600));
    Add(new cMenuEditStraItem(tr("Info window position"), &m_NewOsdPipSetup.InfoPosition, kInfoPositions, InfoPositionItems));
*/
    Add(new cMenuEditStraItem(tr("Pip position"), &m_NewOsdPipSetup.PipPosition, kInfoPositions, InfoPositionItems));
}

cOsdPipSetupPage::~cOsdPipSetupPage()
{
    //std::cout << "-------------cOsdPipSetupPage::~cOsdPipSetupPage---------------------" << std::endl;
}

void cOsdPipSetupPage::Store(void)
{
    //std::cout << "---------------------cOsdPipSetupPage::Store---------------------" << std::endl;
    OsdPipSetup = m_NewOsdPipSetup;

    SetupStore("PipPosition", OsdPipSetup.PipPosition);
    SetupStore("ShowInfo", OsdPipSetup.ShowInfo);
//    SetupStore("InfoWidth", OsdPipSetup.InfoWidth);
    SetupStore("InfoPosition", OsdPipSetup.InfoPosition);
}
