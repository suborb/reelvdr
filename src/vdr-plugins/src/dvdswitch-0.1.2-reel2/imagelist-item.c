#include "imagelist-item.h"

cImageListItem::cImageListItem(char *lname, char *sname, eFileInfo type, const char *value, bool hide)
{
  LName = NULL;
  SName = NULL;
  Value = 0;
  SString = NULL;

  Edit(lname, sname, type, value, hide);

  Debug();
}

cImageListItem::~ cImageListItem(void)
{
  free(LName);
  free(SName);
  free(Value);
  free(SString);
}

void cImageListItem::Edit(char *lname, char *sname, eFileInfo type, const char *value, bool hide)
{
  MYDEBUG("New/Edit ImageListItem");
  Debug();

  free(LName);
  free(SName);
  free(Value);

  LName = lname ? strdup(lname) : NULL;
  SName = sname ? strdup(sname) : NULL;

  if(type == tFile && value && value[0] != '.')
    asprintf(&Value, ".%s", value);
  else
    Value = value ? strdup(value) : NULL;

  fType = type;
  HideExt = hide;

  MakeSetupString();

  Debug();
}

void cImageListItem::Debug(void)
{
  MYDEBUG("Items:");
  MYDEBUG("  LongName: %s", LName);
  MYDEBUG("  ShortName: %s", SName);
  MYDEBUG("  FileType: %i", (int)fType);
  MYDEBUG("  Value: %s", Value);
  MYDEBUG("  Hide: %s", HideExt ? "TRUE" : "FALSE");
  MYDEBUG("  SaveString: %s", SString);
}
