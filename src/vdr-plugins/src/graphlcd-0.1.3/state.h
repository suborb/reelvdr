/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  state.h  -  status monitor class
 *
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online de>
 **/

#ifndef _GRAPHLCD_STATE_H_
#define _GRAPHLCD_STATE_H_

#include <string.h>

#include <vdr/status.h>


struct tChannelState
{
	tChannelID id;
	int number;
	std::string str;
	std::string strTmp;
};

struct tEventState
{
	time_t presentTime;
	std::string presentTitle;
	std::string presentSubtitle;
	time_t followingTime;
	std::string followingTitle;
	std::string followingSubtitle;
};

enum eReplayMode
{
	eReplayNormal,
	eReplayMusic,
	eReplayDVD,
	eReplayFile,
	eReplayImage,
	eReplayAudioCD
};

struct tReplayState
{
	std::string name;
#ifdef USE_LIVEBUFFER
	std::string filename;
#endif
	std::string loopmode;
	cControl * control;
	eReplayMode mode;
	int current;
	int currentLast;
	int total;
	int totalLast;
};

struct tCardState
{
	int recordingCount;
	std::string recordingName;
};

struct tOsdState
{
	std::string currentItem;
	std::vector <std::string> items;
	std::string title;
	std::string colorButton[4];
	std::string textItem;
	std::string message;
	int currentItemIndex;
};

struct tVolumeState
{
	int value;
	unsigned long long lastChange;
};

class cGraphLCDState : public cStatus
{
private:
	bool first;
	bool tickUsed;

	cMutex mutex;

	tChannelState channel;
	tEventState event;
	tReplayState replay;
	tCardState card[MAXDEVICES];
	tOsdState osd;
	tVolumeState volume;

	void SetChannel(int ChannelNumber);
	void GetProgramme();
protected:
	virtual void ChannelSwitch(const cDevice *Device, int ChannelNumber);
#if VDRVERSNUM < 10338
	virtual void Recording(const cDevice *Device, const char *Name);
	virtual void Replaying(const cControl *Control, const char *Name);
#else
#if VDRVERSNUM > 10700
	virtual void Recording(const cDevice *Device, const char *Name, const char *FileName, bool On);
#else
	virtual void Recording(const cDevice *Device, const char *Name, const char *FileName, bool On, int ChannelNumber);
#endif
	virtual void Replaying(const cControl *Control, const char *Name, const char *FileName, bool On);
#endif
	virtual void SetVolume(int Volume, bool Absolute);
	virtual void Tick();
#if VDRVERSNUM < 10716
	virtual void OsdClear(bool doNotShow = false);
#else
	virtual void OsdClear();
#endif
	virtual void OsdTitle(const char *Title);
	virtual void OsdStatusMessage(const char *Message);
	virtual void OsdHelpKeys(const char *Red, const char *Green, const char *Yellow, const char *Blue);
	virtual void OsdItem(const char *Text, int Index);
	virtual void OsdCurrentItem(const char *Text);
	virtual void OsdTextItem(const char *Text, bool Scroll);
	virtual void OsdChannel(const char *Text);
	virtual void OsdProgramme(time_t PresentTime, const char *PresentTitle, const char *PresentSubtitle, time_t FollowingTime, const char *FollowingTitle, const char *FollowingSubtitle);

public:
	cGraphLCDState();
	virtual ~cGraphLCDState();

	tChannelState GetChannelState();
	tEventState GetEventState();
	tReplayState GetReplayState();
	tCardState GetCardState(int number);
	tOsdState GetOsdState();
	tVolumeState GetVolumeState();
};

#endif
