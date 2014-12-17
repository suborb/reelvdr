/*
 * OSD Picture in Picture plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */

#ifndef VDR_OSDPIP_OSD_INFO_H
#define VDR_OSDPIP_OSD_INFO_H

#include <sys/time.h>

#include <vdr/channels.h>
#include <vdr/osd.h>
#include <vdr/osdbase.h>


class cOsdInfoWindow
{
  private:
    cOsd * m_Osd;
    unsigned int *m_Palette;
    cBitmap *m_Bitmap;
    int m_InfoX;
    int m_InfoY;
    int m_pipPosX;
    int m_pipPosY;
    int m_x, m_y;
    bool m_Shown;
    time_t m_LastTime;

    int m_Number;
    int m_Group;
    bool m_WithInfo;
    const char *m_Message;
    const cChannel *m_Channel;
    const cChannel *m_PipChannel;
  public:
      cOsdInfoWindow(cOsd * osd, unsigned int *palette, int x, int y, int pipPosX, int pipPosY);
     ~cOsdInfoWindow();
    void SetMessage(const char *message);
    void SetChannel(const cChannel * channel, bool isPipChannel);
    void Show(bool pipChannel, bool dummy);
    void Hide();
    eOSState ProcessKey(eKeys key);

    bool Shown() const
    {
        return m_Shown;
    }
};

#endif
