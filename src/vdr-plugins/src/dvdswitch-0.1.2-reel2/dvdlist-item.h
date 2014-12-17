#ifndef __DVDLISTITEM_DVDSWITCH_H
#define __DVDLISTITEM_DVDSWITCH_H

#include <vdr/tools.h>
//#include <vdr/i18n.h>

class cDVDListItem : public cListObject
{
  private:
    char *File;

  public:
    cDVDListItem(const char* file);
    ~cDVDListItem(void);

    char* FileName(void) { return File; };
};

#endif // __DVDLISTITEM_DVDSWITCH_H
