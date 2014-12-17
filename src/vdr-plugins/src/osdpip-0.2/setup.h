/*
 * OSD Picture in Picture plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */

#ifndef VDR_OSDPIP_SETUP_H
#define VDR_OSDPIP_SETUP_H

#include <vdr/plugin.h>

const int kInfoTopLeft     = 0;
const int kInfoTopRight    = 1;
const int kInfoBottomRight = 2;
const int kInfoBottomLeft  = 3;

struct pos
{
    int xpos;
    int ypos;
};

extern pos pipPositions[4];

struct cOsdPipSetup
{
    cOsdPipSetup(void);

    bool SetupParse(const char *Name, const char *Value);

    int PipPosition;
    int ShowInfo;
    int InfoWidth;
    int InfoPosition;
};

extern cOsdPipSetup OsdPipSetup;

class cOsdPipSetupPage:public cMenuSetupPage
{
  private:
    cOsdPipSetup m_NewOsdPipSetup;

  protected:
    virtual void Store(void);

  public:
      cOsdPipSetupPage(void);
      virtual ~ cOsdPipSetupPage();
};

#endif // VDR_OSDPIP_SETUP_H
