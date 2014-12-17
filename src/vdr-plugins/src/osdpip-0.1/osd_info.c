#include "osd_info.h"
#include "setup.h"

#include <iostream>
#include <sstream>
#include <string>

#include <vdr/thread.h>
#include <vdr/device.h>

#define COLORBG clrTransparent

#define DIRECTCHANNELTIMEOUT 1
#define INFOTIMEOUT          5

cOsdInfoWindow::cOsdInfoWindow(cOsd * osd, unsigned int *palette, int x, int y, int pipPosX, int pipPosY):
m_Osd(osd), m_Palette(palette), m_InfoX(x), m_InfoY(y), m_pipPosX(pipPosX), m_pipPosY(pipPosY),
m_Shown(false), m_LastTime(0), m_Number(0), m_Group(-1),
m_WithInfo(false), m_Message(NULL), m_Channel(NULL), m_PipChannel(NULL)
{
#ifdef TRUECOLOR
    m_Bitmap = new cBitmap(720, 576, /*OsdPipSetup.InfoWidth, OsdPipSetup.ShowInfo * 30,*/ 32);
#else
    m_Bitmap = new cBitmap(720, 576, /*OsdPipSetup.InfoWidth, OsdPipSetup.ShowInfo * 30,*/ 4);
#endif
}

cOsdInfoWindow::~cOsdInfoWindow()
{
    delete m_Bitmap;
}

void cOsdInfoWindow::SetMessage(const char *message)
{
    m_Message = message;
}

void cOsdInfoWindow::SetChannel(const cChannel * channel, bool isPipChannel)
{
    std::cout << "PIP: " << isPipChannel << std::endl;
    if(isPipChannel)
      m_PipChannel = channel;
    else
      m_Channel = channel;
    m_WithInfo = true;
    m_Number = 0;
    m_Group = -1;
}

void cOsdInfoWindow::Show(bool pipChannel, bool dummy)
{
    std::cout << "cOsdInfoWindow::Show() pipChannel: " << pipChannel << std::endl;
    std::stringstream channel;
    std::string presentName;
    std::string presentTime;
    std::string followingName;
    std::string followingTime;
    //channel.width(100);

    const cChannel *tmp_channel;
    if(pipChannel && m_PipChannel)
       tmp_channel = m_PipChannel;
    else
       tmp_channel = m_Channel;

    if(!dummy) {
     if (m_Message) {
        std::cout << "cOsdInfoWindow::Show(), m_Message" << std::endl; 
        channel << m_Message;
        m_Message = NULL;
     } else if (tmp_channel) {
        std::cout << "cOsdInfoWindow::Show(), m_Channel" << std::endl;
        if (tmp_channel->GroupSep())
            channel << tmp_channel->Name();
        else
            channel << tmp_channel->Number() << (m_Number ? "-" : "") << " " << tmp_channel->Name();
        if (m_WithInfo) {
            std::cout << "cOsdInfoWindow::Show, Events!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
            const cEvent *present = NULL;
            const cEvent *following = NULL;
            cSchedulesLock schedulesLock;
            const cSchedules *schedules = cSchedules::Schedules(schedulesLock);
            if (schedules) {
                const cSchedule *schedule = schedules->GetSchedule(tmp_channel->GetChannelID());
                if (schedule) {
                    if ((present = schedule->GetPresentEvent()) != NULL) {
                        const char *presentTitle = present->Title();
                        if (!isempty(presentTitle)) {
                            presentTime = std::string(present->GetTimeString());
                            presentName = std::string(presentTitle);
                        }
                    }
                    if ((following = schedule->GetFollowingEvent()) != NULL) {
                        const char *followingTitle = following->Title();
                        if (!isempty(followingTitle)) {
                            followingTime = std::string(following->GetTimeString());
                            followingName = std::string(followingTitle);
                        }
                    }
                }
            }
        }
     } else if (m_Number) {
        std::cout << "cOsdInfoWindow::Show(), m_Number" << std::endl;
        channel << m_Number << "-";
     } else {
        std::cout << "cOsdInfoWindow::Show(), else" << std::endl;
        channel << tr("*** Invalid Channel ***");
     }
    }

    bool atBottom = false;

    m_Bitmap->DrawRectangle(0, 0, m_Bitmap->Width() - 1, m_Bitmap->Height() - 1, COLORBG);
    const cFont *font = cFont::GetFont(fontOsd);
    if(m_pipPosX < 100 && m_pipPosY < 100) { /* top left */
      if(pipChannel) {
	m_x = m_pipPosX;
        m_y = m_pipPosY + 200; 
      } else {
        m_x = 50;
	m_y = Setup.OSDHeight - 50; 
      }
    } else if (m_pipPosX >= 100 && m_pipPosY < 100) { /* top right */
      if(pipChannel) {
	m_x = m_pipPosX;
        m_y = m_pipPosY + 200; 
      } else {
        m_x = 50;
	m_y = Setup.OSDHeight - 50; 
      }
    } else if (m_pipPosX >= 100 && m_pipPosY >= 100) { /* bottom right */
      atBottom = true;
      if(pipChannel) {
	m_x = m_pipPosX;
        m_y = m_pipPosY - 40; 
      } else {
        m_x = 50;
	m_y = 50; 
      }
    } else { /* bottom left */
      atBottom = true;
      if(pipChannel) {
	m_x = m_pipPosX;
        m_y = m_pipPosY - 40; 
      } else {
        m_x = 50;
	m_y = 50; 
      }
    }

    int ty = m_y;
#define DATEW 60
    int textw = (pipChannel ? 115 : (m_Bitmap->Width() - DATEW - m_x));
    if(!dummy){
     if(pipChannel) {
      if(!atBottom) ty += 60;
      ty -= 60;
      if(atBottom && OsdPipSetup.ShowInfo == 2)
        ty += 30;
      else if (atBottom && OsdPipSetup.ShowInfo == 1)
        ty += 60;
      m_Bitmap->DrawText(m_x, ty, channel.str().c_str(), clrWhite, COLORBG, font, textw + DATEW, 29);
      if (OsdPipSetup.ShowInfo >= 2) {
          ty += 30;
          m_Bitmap->DrawText(m_x, ty, presentTime.c_str(), clrWhite, COLORBG, font, DATEW, 29);
          m_Bitmap->DrawText(m_x+DATEW, ty, presentName.c_str(), clrWhite, COLORBG, font, textw, 29);
      }
      if (OsdPipSetup.ShowInfo == 3) {
          ty += 30;
          m_Bitmap->DrawText(m_x, ty, followingTime.c_str(), clrWhite, COLORBG, font, DATEW, 29);
          m_Bitmap->DrawText(m_x + DATEW, ty, followingName.c_str(), clrWhite, COLORBG, font, textw, 29);
      }
    } else { 
      m_Bitmap->DrawText(m_x, ty, channel.str().c_str(), clrWhite, COLORBG, font, textw + DATEW, 29);
      if (OsdPipSetup.ShowInfo >= 2) {
          ty += 30;
          m_Bitmap->DrawText(m_x, ty, presentTime.c_str(), clrWhite, COLORBG, font, DATEW, 29);
          m_Bitmap->DrawText(m_x+DATEW, ty, presentName.c_str(), clrWhite, COLORBG, font, textw, 29);
      }
      if (OsdPipSetup.ShowInfo == 3) {
          ty += 30;
          m_Bitmap->DrawText(m_x, ty, followingTime.c_str(), clrWhite, COLORBG, font, DATEW, 29);
          m_Bitmap->DrawText(m_x + DATEW, ty, followingName.c_str(), clrWhite, COLORBG, font, textw, 29);
      }
     }
    }

    m_Osd->DrawBitmap(0, 0 /*m_InfoX, m_InfoY*/, *m_Bitmap);
    m_Osd->Flush();
    m_Shown = true;
    time(&m_LastTime);
}

void cOsdInfoWindow::Hide()
{
    m_Bitmap->DrawRectangle(0, 0, m_Bitmap->Width() - 1, m_Bitmap->Height() - 1, clrTransparent);
    m_Osd->DrawBitmap(0, 0 /*m_InfoX, m_InfoY*/, *m_Bitmap);
    m_Osd->Flush();
    m_Shown = false;
}

eOSState cOsdInfoWindow::ProcessKey(eKeys key)
{
    time_t curTime;

    switch (key)
    {
    case k0:
        if (m_Number == 0) {
            m_Group = -1;
            Hide();
            return osUnknown;
        }
    case k1...k9:
        if (m_Number >= 0) {
            m_Number = m_Number * 10 + key - k0;
            if (m_Number > 0) {
                cChannel *channel = Channels.GetByNumber(m_Number);
                m_Channel = channel;
                m_WithInfo = false;
                Show(false, (OsdPipSetup.ShowInfo==0));
                // Lets see if there can be any useful further input:
                int n = channel ? m_Number * 10 : 0;
                while (channel && (channel = Channels.Next(channel)) != NULL) {
                    if (!channel->GroupSep()) {
                        if (n <= channel->Number() && channel->Number() <= n + 9) {
                            n = 0;
                            break;
                        }
                        if (channel->Number() > n)
                            n *= 10;
                    }
                }
                if (n > 0) {
                    // This channel is the only one that fits the input, so let's take it right away:
                    int number = m_Number;
                    m_Number = 0;
                    m_Group = -1;
                    m_WithInfo = true;
                    Channels.SwitchTo(number);
                }
            }
        }
        return osContinue;
    case kLeft | k_Repeat:
    case kLeft:
    case kRight | k_Repeat:
    case kRight:
        m_WithInfo = false;
        if (m_Group < 0) {
            cChannel *channel = Channels.GetByNumber(cDevice::CurrentChannel());
            if (channel)
                m_Group = channel->Index();
        }
        if (m_Group >= 0) {
            int SaveGroup = m_Group;
            if (NORMALKEY(key) == kRight)
                m_Group = Channels.GetNextGroup(m_Group);
            else
                m_Group = Channels.GetPrevGroup(m_Group < 1 ? 1 : m_Group);
            if (m_Group < 0)
                m_Group = SaveGroup;
            cChannel *channel = Channels.Get(m_Group);
            if (channel) {
                m_Channel = channel;
                Show(false, (OsdPipSetup.ShowInfo==0));
                if (!channel->GroupSep())
                    m_Group = -1;
            }
        }
        return osContinue;
    case kUp | k_Repeat:
    case kUp:
    case kDown | k_Repeat:
    case kDown:
        //std::cout << "cOsdInfoWindow::ProcessKey, vor cDevice::SwitchChannel" << std::endl;
        if (cDevice::SwitchChannel(NORMALKEY(key) == kUp ? 1 : -1));
        SetChannel(Channels.GetByNumber(cDevice::CurrentChannel()), false);
        //std::cout << "cOsdInfoWindow::ProcessKey, nach cDevice::SwitchChannel" << std::endl;
        m_WithInfo = true;
        m_Number = 0;
        m_Group = -1;
        Show(false, (OsdPipSetup.ShowInfo==0));
        return osContinue;
        //case kChanUp|k_Repeat:
        //case kChanUp:
        //case kChanDn|k_Repeat:
        //case kChanDn:
        //    m_WithInfo = true;
        //    m_Number = 0;
        //    m_Group = -1;
        //      return osContinue;//osUnknown;
    case kNone:
        if (Shown()) {
            time(&curTime);
            if (m_Number && curTime - m_LastTime > DIRECTCHANNELTIMEOUT) {
                if (Channels.GetByNumber(m_Number)) {
                    int number = m_Number;
                    m_Number = 0;
                    m_Group = -1;
                    //std::cout << "cOsdInfoWindow::ProcessKey, vor Channels.SwitchTo" << std::endl;
                    /* TB: only switch the channel if we are in live-view-mode, not when replaying recordings */
                    if(cDevice::PrimaryDevice()->Transferring()) {
                        Channels.SwitchTo(number);
                    }
                } else {
                    m_Number = 0;
                    m_Group = -1;
                    m_Channel = NULL;
                    Show(false, (OsdPipSetup.ShowInfo==0));
                    m_Channel = Channels.Get(cDevice::CurrentChannel());
                    m_WithInfo = true;
                    return osContinue;
                }
                return osContinue;
            }
        }
        break;
    case kOk:
        if (Shown()) {
            if (m_Group >= 0) {
                int group = m_Group;
                m_Group = -1;
                m_Number = 0;
                Channels.SwitchTo(Channels.Get(Channels.GetNextNormal(group))->Number());
            } else {
                m_Group = -1;
                m_Number = 0;
                m_Channel = Channels.Get(cDevice::CurrentChannel());
                m_WithInfo = true;
                Hide();
            }
            return osContinue;
        }
        break;
    default:
        return osUnknown;
    }
    if (Shown()) {
        time(&curTime);
        if (curTime - m_LastTime >= INFOTIMEOUT) {
            m_Group = -1;
            m_Number = 0;
            Hide();
        }
        return osContinue;
    }
    return osContinue;
}
