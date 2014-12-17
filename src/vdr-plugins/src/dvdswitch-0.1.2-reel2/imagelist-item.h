#ifndef __IMAGELISTITEM_DVDSWITCH_H
#define __IMAGELISTITEM_DVDSWITCH_H

#include <vdr/tools.h>
//#include <vdr/i18n.h>
#include "helpers.h"

class cImageListItem : public cListObject
{
  private:
    char *LName;
    char *SName;
    eFileInfo fType;
    char *Value;
    bool HideExt;
    char *SString;

    void MakeSetupString(void)
    {
      MYDEBUG("Neuer SetupString");
      FREENULL(SString);
      asprintf(&SString, "%s|%s|%i|%s|%i", LName, SName, (int)fType, Value, HideExt);
      MYDEBUG("...%s", SString);
    };

    void Debug(void);
  public:
    cImageListItem(char *lname, char *sname, eFileInfo type, const char *value, bool hide);
    ~cImageListItem(void);
    void Edit(char *lname, char *shortname, eFileInfo type, const char *value, bool hide);

    char *GetLName(void) { return LName; };
    char *GetSName(void) { return SName; };
    eFileInfo GetFType(void) { return fType; };
    char *GetValue(void) { return Value; };
    bool IsHide(void) { return HideExt; };
    char *SaveString(void) { return SString; };
};

#endif // __IMAGELISTITEM_DVDSWITCH_H
