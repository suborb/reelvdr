#ifndef __DIRECTOROSD_H
#define __DIRECTOROSD_H

#include <vdr/plugin.h>
#include <vdr/status.h>
#include <vdr/tools.h>

class cDirectorOsd : public cThread, public cOsdObject {
private:
	const cFont *font;
	char *channelname;
	
	bool isOsdShowing;
  cOsd *osd;
  const cChannel *Channel;
  int themelike;
  int number;
  int lastChannel;
  int current;
  bool m_Active;
  tColor bgColor;
  tColor txtColor;
  tColor activeTxtColor;
  tColor borderColor;
  tColor bgClear;
  int last_gr_width;
  
  int portalMode;
  int swapKeys;
  int displayChannelInfo;
  time_t lastkeytime;
  int autostart;

protected:
	virtual void Action(void);
public:
  cDirectorOsd(int i_portalmode, int i_swapKeys, int i_displayChannelInfo, int i_themelike, int i_autostart);
  ~cDirectorOsd();
  virtual void Show(void);
  virtual eOSState ProcessKey(eKeys Key);
  
  void CursorUp();
  void CursorDown();
  void CursorLeft();
  void CursorRight();
  void CursorOK();
  void ChannelDown();
  void ChannelUp();

private:
	const cChannel* getChannel(int channelnumber);
	void SwitchtoChannel(int channelnumber);
	int getNumLinks();
	tColor toColor(int r, int g, int b, int t);
	void clearOsd(void);
	void drawOsd(void);
	void drawRect(int x, int y, int width, int height, tColor colorvalue);
};

#endif //__DIRECTOROSD_H
