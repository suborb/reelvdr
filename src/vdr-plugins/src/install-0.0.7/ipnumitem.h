/*********************************************************
 * copied from setup-plugin
 *********************************************************/

#ifndef IPNUMITEM_H
#define IPNUMITEM_H

#include <string>
#include <vector>

#include <vdr/menu.h>
#include <vdr/osdbase.h>
#include <vdr/menuitems.h>
#include <vdr/interface.h>
#include <vdr/submenu.h>
#include "util.h"

// ---- Class cMenuEditIpNumItem  ---------------------------------------------
class cMenuEditIpNumItem:public cMenuEditItem
{
  private:
    cMenuEditIpNumItem(const cMenuEditIpNumItem &); // just for save

      Util::Type val_type;
    int pos;
    int digit;
    char *value;                // comment
    unsigned char val[5];
    virtual void Set(void);

    void Init();
    unsigned char Validate(char *field);
  public:
      cMenuEditIpNumItem(const char *Name, char *Value);
     ~cMenuEditIpNumItem();
    bool InEditMode(void) const
    {
        return pos == 0;
    }
    virtual eOSState ProcessKey(eKeys Key);
};

#endif
