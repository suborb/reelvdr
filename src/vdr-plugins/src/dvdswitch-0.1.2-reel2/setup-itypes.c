#include <vdr/plugin.h>
#include "setup-itypes.h"

cMenuSetupDSITypes::cMenuSetupDSITypes(bool select, int* retindex, char *buffer)
  : cOsdMenu(tr("Imagetypes"))
{
  MYDEBUG("Ermitlle ersten Setup-Eintrag");
  cSetupLine *item = Setup.First();
  Select = select;
  RetIndex = retindex;
  Buffer = buffer;

  if(item)
    MYDEBUG("OK");
  
  while(item)
  {
    if(item->Plugin() &&
       !strcasecmp(item->Plugin(), "dvdswitch") &&
       !strcasecmp(item->Name(), "ImageTypes"))
      break;
    item = Setup.Next(item);
  }
  
  SetupLine = item;

  Set();
}


void cMenuSetupDSITypes::Set(void)
{
  int current = Current();

  Clear();

  cImageListItem *item = ImageList.First();
  
  while(item)
  {
    Add(new cOsdItem(item->GetLName()));
    item = ImageList.Next(item);
  }

  SetCurrent(Get(current));

  Display();
  SetHelp();
}

void cMenuSetupDSITypes::SetHelp(void)
{
  cOsdItem *item = Get(Current());
  if(item)
  {
    if(Select)
      cOsdMenu::SetHelp(NULL, tr("New"), NULL , tr("Select"));
    else
      cOsdMenu::SetHelp(tr("Edit"), tr("New"), tr("Delete") , NULL);
  }
  else
    cOsdMenu::SetHelp(NULL, tr("New"), NULL , NULL);
}

eOSState cMenuSetupDSITypes::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  cImageListItem *item = NULL;

  if (!HasSubMenu())
  {
    switch(Key)
    {
      case kRed:
        state = AddSubMenu(new cMenuSetupDSITypesItem(Current()));
        break;
      case kGreen:
        state = AddSubMenu(new cMenuSetupDSITypesItem(ImageList.Count()));
        break;
      case kYellow:
        if(Interface->Confirm(tr("really delete Entry?")))
        {
          item = ImageList.Get(Current());
          ImageList.Del(item);
          Set();
          Display();
        }
        break;
      case kBlue:
        item = ImageList.Get(Current());
        if(Select && item)
        {
          strn0cpy((char*)Buffer, item->GetLName(), MaxFileName);
          *RetIndex = item->Index();
          return osBack;
        }
        break;
      case kBack:
      case kOk:
        item = ImageList.Get(Current());
        if(Select && item)
        {
          strn0cpy((char*)Buffer, item->GetLName(), MaxFileName);
          *RetIndex = item->Index();
          return osBack;
        }
        if(state == osContinue)
        {
          Set();
          Display();
        }
        else
        {
          if(SetupLine)
          {
            MYDEBUG("Lösche SetupLine");
            Setup.Del(SetupLine);
          }
          MYDEBUG("Hole SetupString");
          if(ImageList.GetSetupString())
            SetupLine = new cSetupLine("ImageTypes", ImageList.GetSetupString(), "dvdswitch");
          else
            SetupLine = new cSetupLine("ImageTypes", "", "dvdswitch");
          MYDEBUG("neue SetupLine erstellt");
          Setup.Add(SetupLine);
          MYDEBUG("neue SetupLine hinzugefügt");
          return osBack;
        }
        break;
      default:
        break;
    }
  }

  return state;
}

// --- cMenuSetupDSITypesItem -----------------------------------

cMenuSetupDSITypesItem::cMenuSetupDSITypesItem(int itemindex)
  : cOsdMenu(tr("New"), 22)
{
  if(itemindex < ImageList.Count())
  {
    SetTitle(tr("Edit"));
    Item = ImageList.Get(itemindex);
    strn0cpy(LongName, Item->GetLName(), 50);
    strn0cpy(ShortName, Item->GetSName(), 20);
    FileType = (int)Item->GetFType() - 1;
    strn0cpy(Extension, Item->GetValue(), 20);
    HideExtension = Item->IsHide();
  } else
  {
    Item = NULL;
    strcpy(LongName, "\0");
    strcpy(ShortName, "\0");
    FileType = 1;
    strcpy(Extension, "\0");
    HideExtension = 1;
  }

  FileTypes[0] = tr("Directory");
  FileTypes[1] = tr("File");

  Set();
}

void cMenuSetupDSITypesItem::Set(void)
{
  int current = Current();

  Clear();

  Add(new cMenuEditStrItem(tr("Description:"),
      LongName,
      50,
      " abcdefghijklmnopqrstuvwxyz0123456789-.#~'/()[]"));
  Add(new cMenuEditStrItem(tr("Type-Title:"),
      ShortName,
      20,
      " abcdefghijklmnopqrstuvwxyz0123456789-.#~'/()[]"));
  Add(new cMenuEditStraItem(tr("Image Type"), &FileType, 2, FileTypes));
  Add(new cMenuEditStrItem(!FileType ? tr("Directory contains:") : tr("File Extension:"),
      Extension,
      20,
      "abcdefghijklmnopqrstuvwxyz0123456789-_.#~"));
  if(FileType)
    Add(new cMenuEditBoolItem(tr("Hide Extension"), &HideExtension));

  SetCurrent(Get(current));
  Display();
}

eOSState cMenuSetupDSITypesItem::ProcessKey(eKeys Key)
{
  int tmpFileType = FileType;
  
  eOSState state = cOsdMenu::ProcessKey(Key);

  if(tmpFileType != FileType)
  {
    Set();
    Display();
  }

  if(state == osUnknown)
  {
    switch(Key)
    {
      case kOk:
        if(isempty(LongName))
        {
          OSD_WARNMSG(tr("'Description' must not empty!"));
          return osContinue;
        }
        if(isempty(ShortName))
        {
          OSD_WARNMSG(tr("'Type-Title' must not empty!"));
          return osContinue;
        }
        if(isempty(Extension))
        {
          if(!FileType)
            OSD_WARNMSG(tr("'Directory contains' must not empty!"));
          else
            OSD_WARNMSG(tr("'File Extension' must not empty!"));
          return osContinue;
        }
        if(!Item)
          ImageList.Add(new cImageListItem(LongName, ShortName, (eFileInfo)(FileType + 1), Extension, HideExtension));
        else
          Item->Edit(LongName, ShortName, (eFileInfo)(FileType + 1), Extension, HideExtension);
        state = osBack;
        break;
      default:
        break;
    }
  }

  return state;
}
