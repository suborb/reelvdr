#ifndef _OSDPROXY_H
#define _OSDPROXY_H 1

#include <vdr/osdbase.h>

class cOsdMenu;

class cOsdProxyData
{
public:
  cOsdProxyData() {};
  virtual ~cOsdProxyData() {};
};

typedef cOsdMenu* (*OSDCREATEFUNC)(const char*, cOsdProxyData* data);
typedef eOSState (*KEYHANDLERFUNC)(eKeys Key);
typedef void (*RELEASEFUNC) (void);

class cOsdProxy : public cOsdObject
{
private:
  cOsdProxyData* mData;
  eKeys mOsdKey;
  char* mTitle;
  bool doEnd;
  cOsdMenu* mOsdMenu;
  OSDCREATEFUNC mOsdMenuCreateFunc;
  KEYHANDLERFUNC mKeyHandlerFunc;
  RELEASEFUNC mReleaseFunc;
public:
  cOsdProxy(OSDCREATEFUNC osdMenuCreateFunction, KEYHANDLERFUNC keyHandler, RELEASEFUNC releaseFunc, const char* title, cOsdProxyData* data, eKeys osdKey, bool directShow);
  virtual ~cOsdProxy();

  virtual eOSState ProcessKey(eKeys Key);
  virtual void Show(void);

  void ShowOsd(bool show);
};

#endif // #ifndef _OSDPROXY_H
