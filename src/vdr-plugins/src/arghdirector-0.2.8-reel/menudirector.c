#include "menudirector.h"

#include <math.h>
#include <iostream>

#include <vdr/device.h>
#include <vdr/epg.h>
#include <vdr/skins.h>
#include <vdr/tools.h>

bool cMenuDirector::hasChannels;

static const char* const ProgressBar[7] =
{
  "[      ]",
  "[|     ]",
  "[||    ]",
  "[|||   ]",
  "[||||  ]",
  "[||||| ]",
  "[||||||]"
};

cMenuDirectorData::cMenuDirectorData()
{
  mHidePluginEntryInMainMenu = 0;
  mPortalmode = 1;
  mSwapKeys = 1;
  mSwapKeysInMenu = 0;
  mDisplayChannelInfo = 0;
  mSwitchChannelOnItemSelect = 0;
  mHidePluginAfterChannelSwitch = 1;
  mDisplayShortcutNumbers = 1;
  mDisplayEpgInfo = 1;
  mHideButton = kOk;
//mAutoStart = 0;
};

cMenuDirectorData::~cMenuDirectorData()
{
}

cMenuDirectorItem::cMenuDirectorItem(const char* text, cChannel* channel) : cOsdItem(text)
{
  mChannel = channel;
}

cMenuDirectorItem::~cMenuDirectorItem()
{
}

cChannel* cMenuDirectorItem::Channel()
{
  return mChannel;
}

cChannel* cMenuDirector::mReferenceChannel = NULL;
cMenuDirectorData* cMenuDirector::mData = NULL;

cMenuDirector::cMenuDirector(const char* title, cMenuDirectorData* data) : cOsdMenu(title)
{
  mData = data;
  
//  mReferenceChannel = NULL;
  mCurrentChannel = NULL;

  DetermineReferenceChannel();

  if(!SetLinkChannels(mCurrentChannel))
     hasChannels = false;
  else
     hasChannels = true;

  if(hasChannels)
    Display();
}

cMenuDirector::~cMenuDirector()
{
}

cOsdMenu* cMenuDirector::Create(const char* title,cOsdProxyData* data)
{
  cOsdMenu* menu = new cMenuDirector(title, (cMenuDirectorData*)data);
  if(!cMenuDirector::hasChannels) {
    delete menu;
    return NULL;
  } else
    return menu;
}

void cMenuDirector::Release()
{
  mReferenceChannel = NULL;
  mData = NULL;
}

void cMenuDirector::ChannelSwitchNonOsd(cChannel* channel)
{
  cDevice::PrimaryDevice()->SwitchChannel(channel, true);
  if ((mData) && (mData->mDisplayChannelInfo == 1))
    Skins.Message(mtInfo,channel->Name());
}

void cMenuDirector::ChannelSwitchNonOsd(int direction)
{
  if (!mReferenceChannel)
    return;

  if ((mData) && (mData->mSwapKeys == 1))
    direction *= -1;

  if(GetLinkChannelsOutOfChannel(mReferenceChannel))
  {
    cChannel* currentChannel = Channels.GetByNumber(cDevice::PrimaryDevice()->CurrentChannel());
    cLinkChannels* linkChannels = GetLinkChannelsOutOfChannel(mReferenceChannel);

    if ((currentChannel == mReferenceChannel) && (direction == -1))
    {
      ChannelSwitchNonOsd(linkChannels->Last()->Channel());
      return;
    }
    if ((currentChannel == mReferenceChannel) && (direction == +1))
    {
      ChannelSwitchNonOsd(linkChannels->First()->Channel());
      return;
    }
    
    cChannel* prevChannel = mReferenceChannel;
    for (cLinkChannel* linkChannel = linkChannels->First(); linkChannel != NULL; linkChannel = linkChannels->Next(linkChannel))
    {
      if (linkChannel->Channel() == currentChannel)
      {
        if (direction == -1)
        {
          ChannelSwitchNonOsd(prevChannel);
          return;
        }
        else
        {
          if (linkChannels->Next(linkChannel) == NULL)
            ChannelSwitchNonOsd(mReferenceChannel);
          else
            ChannelSwitchNonOsd(linkChannels->Next(linkChannel)->Channel());
          return;
        }
      }

      prevChannel = linkChannel->Channel();
    }
  }
}

eOSState cMenuDirector::KeyHandlerFunction(eKeys Key)
{
  if (Key == kUp)
  {
    ChannelSwitchNonOsd(-1);
  }
  else if (Key == kDown)
  {
    ChannelSwitchNonOsd(+1);
  }

  return osContinue;
}

eOSState cMenuDirector::ProcessKey(eKeys Key)
{
  if ((mData->mSwapKeysInMenu == 1) && (Key == kUp))
    Key = kDown;
  else if ((mData->mSwapKeysInMenu == 1) && (Key == kDown))
    Key = kUp;
  
  cOsdMenu::ProcessKey(Key);

  if ((Key == kBack) || ((Key == mData->mHideButton) && (mData->mHideButton != kOk)))
    return osBack;

  cChannel* itemChannel = GetChannel(Current());
  if ((itemChannel != mCurrentChannel) && (itemChannel))
  {
    if ((mData->mSwitchChannelOnItemSelect == 1) || (Key == kOk))
    {
      SwitchToChannel(itemChannel);
      if (mData->mHidePluginAfterChannelSwitch == 1)
      {
        return osEnd;
      }
      return osContinue;
    }
  }
  else if ((itemChannel == mCurrentChannel) && (Key == kOk))
  {
    return osEnd;
  }
  return osContinue;
}

void cMenuDirector::SwitchToChannel(cChannel* channel)
{
  if (!channel)
    return;

  cDevice::PrimaryDevice()->SwitchChannel(channel, true);
  mCurrentChannel = channel;
}

void cMenuDirector::DetermineReferenceChannel()
{
  cChannel* currentChannel = Channels.GetByNumber(cDevice::PrimaryDevice()->CurrentChannel());

  if (!mReferenceChannel)
  {
    cChannel* referenceChannel = currentChannel;
	
    if(!GetLinkChannelsOutOfChannel(referenceChannel)) {
      referenceChannel = GetRefChannelOutOfChannel(referenceChannel);
    }
	
    mReferenceChannel = referenceChannel;
  }

  mCurrentChannel = currentChannel;
}

void cMenuDirector::AddChannelItem(cChannel* channel)
{
  if (mData->mDisplayShortcutNumbers == 1)
  {    
    char* buffer;
    int channelNo = cList<cOsdItem>::Count()+1;
    if (mData->mDisplayEpgInfo == 1)
      channelNo = channelNo/2+1;
    if (mData->mPortalmode == 1)
      channelNo--;

    if ((cList<cOsdItem>::First() == NULL) && (mData->mPortalmode == 1))
      asprintf(&buffer,"\t%s", channel->Name());
    else
      asprintf(&buffer,"%d\t%s", channelNo ,channel->Name());

    Add(new cMenuDirectorItem(hk(buffer),channel));
  }
  else
    Add(new cMenuDirectorItem(channel->Name(),channel));

  if (mData->mDisplayEpgInfo == 1)
  {
    const char* title = NULL;
    char* title2;
    cSchedulesLock schedulesLock;
    const cSchedules* schedules = cSchedules::Schedules(schedulesLock);
    const cSchedule* schedule = schedules->GetSchedule(channel->GetChannelID());
    const cEvent* event = NULL;
    int p = 0;

    if(schedule)
    { 
     event = schedule->GetPresentEvent();
     if (event) {
       if(event->Duration())
         p = (int)roundf((float)(time(0) - event->StartTime()) / (float)(event->Duration()) * 6.0);
       if      (p < 0)  p = 0;
       else if (p > 6)  p = 6;

       title = event->Title();
     }
    }

    if (title) {
      if(event)
         asprintf(&title2,(mData->mDisplayShortcutNumbers == 1)?"\t%s\t%s":"%s\t%s", ProgressBar[p], title);
      else
         asprintf(&title2,(mData->mDisplayShortcutNumbers == 1)?"\t%s":"%s", title);
    } else {
      if(event)
         asprintf(&title2,(mData->mDisplayShortcutNumbers == 1)?"\t%s\t%s":"%s\t%s", tr("No EPG available"), ProgressBar[p]);
      else
         asprintf(&title2,(mData->mDisplayShortcutNumbers == 1)?"\t%s":"%s", tr("No EPG available"));
    }

    Add(new cOsdItem(title2));
    cList<cOsdItem>::Last()->SetSelectable(false);
  }
}

bool cMenuDirector::SetLinkChannels(cChannel* selectedChannel)
{
  if (mReferenceChannel != NULL)
  {
    int noOfChannels = 1;
    if(GetLinkChannelsOutOfChannel(mReferenceChannel))
      noOfChannels += GetLinkChannelsOutOfChannel(mReferenceChannel)->Count();
      std::cout << "Nr: " << noOfChannels << std::endl;
    if(noOfChannels == 1)
       return false;
    SetCols((mData->mDisplayShortcutNumbers == 1)?(numdigits(noOfChannels) + 1):0, 6, 28);

    AddChannelItem(mReferenceChannel);

    if(GetLinkChannelsOutOfChannel(mReferenceChannel))
    {
      cLinkChannels* linkChannels = GetLinkChannelsOutOfChannel(mReferenceChannel);
      for (cLinkChannel* linkChannel = linkChannels->First(); linkChannel != NULL; linkChannel = linkChannels->Next(linkChannel))
      {
        AddChannelItem(linkChannel->Channel());
      }
    }
  }
  else
    return false;

  SelectCurrentChannel();
  return true;
}

void cMenuDirector::SelectCurrentChannel()
{
  for (cMenuDirectorItem* item = (cMenuDirectorItem*)First(); item != NULL; item = (cMenuDirectorItem*)Next(item))
  {
    if (item->Channel() == mCurrentChannel)
    {
      SetCurrent(item);
      break;
    }
  }
}

cChannel* cMenuDirector::GetChannel(int Index)
{
  cMenuDirectorItem* item = (cMenuDirectorItem*)Get(Index);
  return item ? (cChannel *)item->Channel() : NULL;
}
