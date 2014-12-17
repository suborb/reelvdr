#ifndef __IMAGELIST_DVDSWITCH_H
#define __IMAGELIST_DVDSWITCH_H

#include <vdr/tools.h>
//#include <vdr/i18n.h>
#include "imagelist-item.h"

class cImageList : public cList<cImageListItem>
{
  private:
    cStringValue SString;
    cStringValue Ext;
    cStringValue HideExt;
    cStringValue DirIn;
    cStringValue Short;

    bool Setup;

    void AddItem(char *item);
  public:
    cImageList(void);
    ~cImageList(void);

    void Init(void);
    void AddSetup(char *value);

    char *GetSetupString(void);
    char *GetExtensions(void);
    char *GetHideExtensions(void);
    char *GetDirContains(void);
    char *GetShortName(char *file);
    bool IsHide(char *ext);
};

extern cImageList ImageList;

#endif //__IMAGELIST_DVDSWITCH_H
