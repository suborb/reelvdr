#ifndef _MENUDIRECTOR_H
#define _MENUDIRECTOR_H 1

#include <vdr/channels.h>
#include <vdr/osdbase.h>

#include "osdproxy.h"

class cMenuDirectorData : public cOsdProxyData
{
public:
  int mHidePluginEntryInMainMenu;
  int mPortalmode;
  int mSwapKeys;
  int mSwapKeysInMenu;
  int mDisplayChannelInfo;
  int mSwitchChannelOnItemSelect;
  int mHidePluginAfterChannelSwitch;
  int mDisplayShortcutNumbers;
  int mDisplayEpgInfo;
  eKeys mHideButton;
//int mAutoStart;

  cMenuDirectorData();
  virtual ~cMenuDirectorData();
};

class cMenuDirectorItem : public cOsdItem
{
private:
  cChannel* mChannel;
public:
  cMenuDirectorItem(const char* text, cChannel* channel);
  virtual ~cMenuDirectorItem();

  cChannel* Channel();
};

class cMenuDirector : public cOsdMenu
{
private:
  static cMenuDirectorData* mData;
  static cChannel* mReferenceChannel;
  cChannel* mCurrentChannel;

  void DetermineReferenceChannel();
  bool SetLinkChannels(cChannel* selectedChannel);
  void AddChannelItem(cChannel* channel);
  void SelectCurrentChannel();
  cChannel* GetChannel(int Index);
  void SwitchToChannel(cChannel* channel);

  static void ChannelSwitchNonOsd(cChannel* channel);
  static cLinkChannels* GetLinkChannelsOutOfChannel(cChannel* channel)
  {
    if (!channel)
      return NULL;

    return (cLinkChannels*)channel->LinkChannels();
  };
  static cChannel* GetRefChannelOutOfChannel(cChannel* channel)
  {
    if (!channel)
      return NULL;

    return (cChannel*)channel->RefChannel();
  };

public:
  cMenuDirector(const char* title, cMenuDirectorData* data);
  virtual ~cMenuDirector();
  static cOsdMenu* Create(const char* title, cOsdProxyData* data);
  virtual  eOSState ProcessKey(eKeys Key);

  static void Release();
  static void ChannelSwitchNonOsd(int direction);
  static eOSState KeyHandlerFunction(eKeys Key);
  static bool hasChannels;
};

#endif
