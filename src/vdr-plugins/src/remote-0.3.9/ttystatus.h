/*
 * Remote plugin for the Video Disk Recorder
 *
 * ttystatus.h: tty osd emulation
 *
 * See the README file for copyright information and how to reach the author.
 */


#ifndef __PLUGIN_TTYSTATUS_H
#define __PLUGIN_TTYSTATUS_H


#define WHITE_BLACK	1
#define YELLOW_BLACK	2
#define CYAN_BLACK	3
#define BLACK_CYAN	4
#define BLACK_RED	5
#define BLACK_GREEN	6
#define BLACK_YELLOW	7
#define WHITE_BLUE	8


#include <vdr/status.h>


#define clear_screen()   { set_color(WHITE_BLACK); print("\e[2J"); }
#define refresh()        
#define set_pos(y,x)     print("\e[%d;%dH", y+1, x+1);
#define print(fmt...)    { char buf[100]; snprintf(buf,100, fmt); display(buf); }
  

/****************************************************************************/
class cTtyStatus : public cStatus {
/****************************************************************************/
private:
  int fd;
  char **lineBuf;
  int numEntries;
  int lastIndex;
  int currIndex;
  int tabstop;

protected:
  virtual void display(const char *buf);
  virtual void display2(const char *buf);
  virtual void set_color(int col);
#if VDRVERSNUM <= 10337
  virtual void Replaying(const cControl *Control, const char *Name);
  virtual void Recording(const cDevice *Device, const char *Name);
#else
  virtual void Recording(const cDevice *Device,
                         const char *Name, const char *FileName, bool On, int ChannelNumber);
  virtual void Replaying(const cControl *Control,
                         const char *Name, const char *FileName, bool On);
#endif
  virtual void SetVolume(int Volume, bool Absolute);
  virtual void OsdClear(void);
  virtual void OsdTitle(const char *Title);
  virtual void OsdStatusMessage(const char *Message);
  virtual void OsdHelpKeys(const char *Red, const char *Green,
                           const char *Yellow, const char *Blue);
  virtual void OsdItem(const char *Text, int Index);
  virtual void OsdCurrentItem(const char *Text);
  virtual void OsdTextItem(const char *Text, bool Scroll);
  virtual void OsdChannel(const char *Text);
  virtual void OsdProgramme(time_t PresentTime, const char *PresentTitle,
                            const char *PresentSubtitle,
                            time_t FollowingTime, const char *FollowingTitle,
                            const char *FollowingSubtitle);

public:
  cTtyStatus(int fd);
  virtual ~cTtyStatus();
};


#endif // __PLUGIN_TTYSTATUS_H
