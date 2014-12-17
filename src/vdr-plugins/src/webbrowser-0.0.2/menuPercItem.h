#ifndef menuPercItem_h
#define menuPercItem_h

#include <vdr/menuitems.h>

class cMenuEditIntPercItem:public cMenuEditItem
{
  protected:
    int *value;
    int min, max, delta;
    const char *minString, *maxString;
    virtual void Set(void);
  public:
      cMenuEditIntPercItem(const char *Name, int *Value, int Min = 0, int Max = INT_MAX, const char *MinString = NULL, const char *MaxString = NULL, int Delta = 1);
    virtual eOSState ProcessKey(eKeys Key);
};


#endif
