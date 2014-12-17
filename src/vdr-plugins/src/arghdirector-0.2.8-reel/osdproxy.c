#include <vdr/tools.h>

#include <iostream>
#include "osdproxy.h"

cOsdProxy::cOsdProxy(OSDCREATEFUNC osdMenuCreateFunction, KEYHANDLERFUNC keyHandler, RELEASEFUNC releaseFunc,const char* title, cOsdProxyData* data,eKeys osdKey, bool directShow)
{
  mOsdMenuCreateFunc = osdMenuCreateFunction;
  mData = data;
  mTitle = strdup(title);
  mOsdMenu = NULL;
  mOsdKey = osdKey;
  mKeyHandlerFunc = keyHandler;
  mReleaseFunc = releaseFunc;
  doEnd = false;

  if (directShow)
    ShowOsd(true);
}

cOsdProxy::~cOsdProxy()
{
  if (mOsdMenu)
    ShowOsd(false);
  if (mTitle)
    free(mTitle);

  if (mReleaseFunc)
    mReleaseFunc();

  DELETENULL(mData);  
}

void cOsdProxy::Show(void)
{
  if (mOsdMenu)
    mOsdMenu->Display();
}

void cOsdProxy::ShowOsd(bool show)
{
  if ((!show) && (mOsdMenu))
    DELETENULL(mOsdMenu);
  else if ((show) && (!mOsdMenu))
    mOsdMenu = mOsdMenuCreateFunc(mTitle,mData);
  std::cout << "RR: " << mOsdMenu << std::endl;

  if (mOsdMenu==NULL) {
    doEnd = true;
    }
}

eOSState cOsdProxy::ProcessKey(eKeys Key)
{
  if(doEnd) {
    Skins.Message(mtInfo, tr("No Multifeed options"));
    return osBack;
  }
  if (mOsdMenu)
  {
    eOSState retState = mOsdMenu->ProcessKey(Key);
    if (retState == osEnd)
    {
      ShowOsd(false);
      return osEnd; // return osContinue;
    }
    return retState;
  }
  else if (Key == mOsdKey)
  {
    ShowOsd(true);
    return osContinue;
  }
  else if (Key == kBack)
    //return osEnd;
    return osBack;
  else if (mKeyHandlerFunc)
    return mKeyHandlerFunc(Key);
  else
    return osContinue;
}
