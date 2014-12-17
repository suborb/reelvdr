#include <vdr/plugin.h>
#include "menu.h"
#include "tools.h"
#include "setup.h"
//#include "dvdplugin.h"
#include "commands.h"
#include "imagelist.h"

 
/* try openning the dvd device '/dev/dvd' 
   and look for error 
 */
bool IsDvdInDrive1() {

    int fd = open("/dev/dvd", O_RDONLY);

    // error NO medium in drive
    if (fd < 0)
        return false;
    else {
        // DVD is in drive
        close(fd);
        return true;
    } // else

} // dvd_in_drive()



cDirHandlingOpt MainMenuOptions;
cDVDList* DVDList;


// --- static helper function -----------------------------------------

static bool switchOnMediad(bool val) {
      cPlugin *plugin=cPluginManager::GetPlugin("mediad");
      if(plugin) return plugin->Service("SwitchOn", &val);
      return false;
}

// --- cMainMenu ------------------------------------------------------------------------

cMainMenu::cMainMenu(cDVDListThread *listThread)
  : cOsdMenu(DVDSwitchSetup.MenuName, DVDSwitchSetup.CountTypCol, 10)
{
  thread = listThread;
  FirstSelectable = -1;
  CMDImg = NULL;
  State = mmsNone;

  if(!MainMenuOptions.ImageDir())
    Init();

  switchOnMediad(false);

  /* TB: TODO: find out why this message gets corrupted */
  //SetStatus(tr("Please wait. Searching for DVD images"));
  refreshList = true;
  Build(MainMenuOptions.CurrentDir());
}

cMainMenu::~cMainMenu(void)
{
  if(CMDImg)
    delete(CMDImg);
  switchOnMediad(true);
  thread->Stop();
}

void cMainMenu::Init(void)
{
  MainMenuOptions.ImageDir(DVDSwitchSetup.ImageDir);
  MainMenuOptions.CurrentDir(DVDSwitchSetup.ImageDir);
  MainMenuOptions.ParentDir(DVDSwitchSetup.ImageDir);
}

void cMainMenu::SetMenuTitle(void)
{
  char *title = NULL;

  if(DVDSwitchSetup.ViewFreeDiskSpace)
  {
    int mByte = FreeDiskSpaceMB(DVDSwitchSetup.ImageDir);
	double gByte = (double)((int)((double)mByte / 1024 * 10)) / 10;
	asprintf(&title,"%s - Disk: %4.1f GB %s",tr("My DVDs"), gByte, tr("free"));
  } else
    title = tr("My DVDs") ? strdup(tr("My DVDs")) : NULL;

  SetTitle(title);
  free(title);
}

void cMainMenu::Build(char *dir)
{
  void (cMainMenu::*p) (int);

  MYDEBUG("Build MainMenu von %s", dir);

  SetMenuTitle();
 /* ar
  if(!DVDSwitchSetup.HideImgSizeCol)
    SetCols(DVDSwitchSetup.CountTypCol, 10);
  else
 */
    SetCols(22, DVDSwitchSetup.CountTypCol-5, 10);

  FirstSelectable = -1;

  Clear();

  p = &cMainMenu::SelectFirst;
  thread->BuildDisp(this, p, dir);

  // empty menu
  if (Count() == 0) {
      if (IsDvdInDrive1()) { 
          Add(new cOsdItem(tr("DVD found in drive."),osUnknown, false));
          Add(new cOsdItem((""),osUnknown, false) );
          Add(new cOsdItem((""),osUnknown, false));
          Add(new cOsdItem((""),osUnknown, false));
          Add(new cOsdItem((""),osUnknown, false));
          Add(new cOsdItem((""),osUnknown, false));
          Add(new cOsdItem((""),osUnknown, false));
          Add(new cOsdItem((""),osUnknown, false));
          Add(new cOsdItem((""),osUnknown, false));
          Add(new cOsdItem((""),osUnknown, false));
          Add(new cOsdItem((""),osUnknown, false));
          Add(new cOsdItem(tr("Info:"),osUnknown, false));
          AddFloatingText(tr("Use the red button to create a archive."), MAXOSDTEXTWIDTH );
      } else {
          Add(new cOsdItem(tr("No DVD found in drive."),osUnknown, false));
          Add(new cOsdItem((""),osUnknown, false) );
          Add(new cOsdItem((""),osUnknown, false));
          Add(new cOsdItem((""),osUnknown, false));
          Add(new cOsdItem((""),osUnknown, false));
          Add(new cOsdItem((""),osUnknown, false));
          Add(new cOsdItem((""),osUnknown, false));
          Add(new cOsdItem((""),osUnknown, false));
          Add(new cOsdItem((""),osUnknown, false));
          Add(new cOsdItem((""),osUnknown, false));
          Add(new cOsdItem((""),osUnknown, false));
          Add(new cOsdItem(tr("Info:"),osUnknown, false));
          AddFloatingText(tr("Insert a DVD to create a archive"), MAXOSDTEXTWIDTH );
  }

}

}
void cMainMenu::SelectFirst(int first) {
  FirstSelectable = first;
  MYDEBUG("First Selectable ist: %i", FirstSelectable);

  if(FirstSelectable >= 0)
  {
    cMainMenuItem *mmItem = (cMainMenuItem*)Get(FirstSelectable);
    if(!mmItem) { Clear(); return;}
    SetCurrent(mmItem);
    MainMenuOptions.setLastSelectItemName(mmItem->FileName());
    MainMenuOptions.LastSelectItemType(mmItem->Type());
  }

  Display();
  SetHelp();
}

void cMainMenu::SetHelp(void)
{
  cMainMenuItem *mItem = (cMainMenuItem*)Get(Current());

  const char *kRed = NULL, *kGreen = NULL, *kYellow = NULL, *kBlue = NULL;

  switch((eCMDs)DVDSwitchSetup.kRed)
  {
    case cmdImgPlay:
      if(mItem && (mItem->Type() == iParent || mItem->Type() == iDir))
      {
         kRed = tr("to change");
         break;
      }
      if(mItem && !DVDSwitchSetup.HaveXinePlugin)
        break;
    case cmdImgRename:
    case cmdImgMove:
    case cmdImgDelete:
    case cmdImgBurn:
      if(!mItem)
        break;
      if(mItem && (mItem->Type() == iParent || mItem->Type() == iDir))
        break;
      if(mItem && (eCMDs)DVDSwitchSetup.kRed != cmdImgPlay && mItem->Type() != iDVD)
        break;
    case cmdDirManage:
    case cmdDVDopen:
    case cmdDVDclose:
    case cmdImgRead:
    case cmdDeleteCache:
    case cmdCommands:
      kBlue = DVDSwitchSetup.CommandsShortName[DVDSwitchSetup.kRed];
      break;
    default:
      break;
  }

  switch((eCMDs)DVDSwitchSetup.kGreen)
  {
    case cmdImgPlay:
      if(mItem && (mItem->Type() == iParent || mItem->Type() == iDir))
      {
        kGreen = tr("to change");
        break;
      }
      if(mItem && !DVDSwitchSetup.HaveXinePlugin)
        break;
    case cmdImgRename:
    case cmdImgMove:
    case cmdImgDelete:
    case cmdImgBurn:
      if(!mItem)
        break;
      if(mItem && (mItem->Type() == iParent || mItem->Type() == iDir))
        break;
      if(mItem && (eCMDs)DVDSwitchSetup.kGreen != cmdImgPlay && mItem->Type() != iDVD)
        break;
    case cmdDirManage:
    case cmdDVDopen:
    case cmdDVDclose:
    case cmdImgRead:
    case cmdDeleteCache:
    case cmdCommands:
      kGreen = DVDSwitchSetup.CommandsShortName[DVDSwitchSetup.kGreen];
      break;
    default:
      break;
  }

  switch((eCMDs)DVDSwitchSetup.kYellow)
  {
    case cmdImgPlay:
      if(mItem && (mItem->Type() == iParent || mItem->Type() == iDir))
      {
        kYellow = tr("to change");
        break;
      }
      if(mItem && !DVDSwitchSetup.HaveXinePlugin)
        break;
    case cmdImgRename:
    case cmdImgMove:
    case cmdImgDelete:
    case cmdImgBurn:
      if(!mItem)
        break;
      if(mItem && (mItem->Type() == iParent || mItem->Type() == iDir))
        break;
      if(mItem && (eCMDs)DVDSwitchSetup.kYellow != cmdImgPlay && mItem->Type() != iDVD)
        break;
    case cmdDirManage:
    case cmdDVDopen:
    case cmdDVDclose:
    case cmdImgRead:
    case cmdDeleteCache:
    case cmdCommands:
      kYellow = DVDSwitchSetup.CommandsShortName[DVDSwitchSetup.kYellow];
      break;
    default:
      break;
  }

  switch((eCMDs)DVDSwitchSetup.kBlue)
  {
    case cmdImgPlay:
      if(mItem && (mItem->Type() == iParent || mItem->Type() == iDir))
      {
        kBlue = tr("to change");
        break;
      }
      if(mItem && !DVDSwitchSetup.HaveXinePlugin)
        break;
    case cmdImgRename:
    case cmdImgMove:
    case cmdImgDelete:
    case cmdImgBurn:
      if(!mItem)
        break;
      if(mItem && (mItem->Type() == iParent || mItem->Type() == iDir))
        break;
      if(mItem && (eCMDs)DVDSwitchSetup.kBlue != cmdImgPlay && mItem->Type() != iDVD)
        break;
    case cmdDirManage:
    case cmdDVDopen:
    case cmdDVDclose:
    case cmdImgRead:
    case cmdDeleteCache:
    case cmdCommands:
      kRed = DVDSwitchSetup.CommandsShortName[DVDSwitchSetup.kBlue];
      break;
    default:
      break;
  }

  cOsdMenu::SetHelp(kRed, kGreen, kYellow, kBlue);
}

eOSState cMainMenu::ProcessKey(eKeys Key)
{
  eOSState ProcessKeyState = osUnknown;

  if(!HasSubMenu())
  {
    switch(State)
    {
      case mmsImgRename:
        ProcessKeyState = cOsdMenu::ProcessKey(Key); // Run ProcessKey first
        switch(Key)
        {
          case kBack:
            SetState(mmsNone);
            DVDSwitchSetup.HideTypeCol = CMDImg->tmpHideTypeCol;
            DELETENULL(CMDImg);
						refreshList = true;
            Build(MainMenuOptions.CurrentDir());
            Display();
            return osContinue;
            break;
          case kOk:
            MYDEBUG ( " COUNT %d", Count());
            if (!Count())
                return osContinue;

            SetState(mmsNone);
            DVDSwitchSetup.HideTypeCol = CMDImg->tmpHideTypeCol;
            if(!isempty(CMDImg->NewFile))
            {
              char *buffer = NULL;
              char *buffer2 = NULL;
              cFileInfo *info = new cFileInfo(CMDImg->Rename());
              if(ImageList.IsHide(info->Extension()))
              {
                buffer2 = strdup(info->Extension());
                asprintf(&buffer, "%s/%s%s", info->Path(), stripspace(CMDImg->NewFile), buffer2);
              }
              else
                asprintf(&buffer, "%s/%s", info->Path(), stripspace(CMDImg->NewFile));
              DELETENULL(info);
              info = new cFileInfo(buffer);
              if(!info->isExists())
              {
                if(cFileCMD::Rn(CMDImg->Rename(), buffer))
                  MainMenuOptions.setLastSelectItemName(buffer);
              }
              else
                OSD_WARNMSG(tr("File exists in Directory"));
              FREENULL(buffer);
              FREENULL(buffer2);
              DELETENULL(info);
            }
            DELETENULL(CMDImg);
            refreshList = true;
            Build(MainMenuOptions.CurrentDir());
            Display();
            return osContinue;
            break;
          default:
            break;
        }
        break;
      default:
        switch(Key)
        {
          case kUp:
          case kUp|k_Repeat:
          case kDown:
          case kDown|k_Repeat:
               if (Key != kUp || Key != (kUp|k_Repeat))
          case kRight:
          case kRight|k_Repeat:
          case kLeft:
          case kLeft|k_Repeat:
		       return MenuMove(Key);
		       break;
	  case k5: refreshList = true;  SetStatus(tr("Please wait. Searching for DVD images"));
		   Build(MainMenuOptions.CurrentDir());
		   Display(); break;
          case k1:
          case k2:
          case k3:
          case k4:
          case k6:
          case k7:
          case k8:
          case k9:
          case k0:
          case kRed:
          case kGreen:
          case kYellow:
          case kBlue:
          case kOk:
            return Commands(Key);
            break;
          default:
            break;
        }
    }
  }

  if(State!=mmsImgRename)
    ProcessKeyState = cOsdMenu::ProcessKey(Key);
  return ProcessKeyState;
}

eOSState cMainMenu::MenuMove(eKeys Key)
{
  eOSState state = osContinue;
  cMainMenuItem *mItem = NULL;
  int step = 0;

  if(DVDSwitchSetup.DisplayMode == 1)
  {
    switch(Key)
    {
      case kDown:
      case kDown|k_Repeat:
      case kUp:
      case kUp|k_Repeat:
        state = cOsdMenu::ProcessKey(Key);
        if (Count() && Current() == FirstSelectable)
        {

          SetCurrent(Get(0));
          Display();
          SetCurrent(Get(FirstSelectable));
          Display();
        }
        break;
      case kLeft:
      case kLeft|k_Repeat:
        if(Count() &&
          DVDSwitchSetup.JumpCatByKey)
        {
          mItem = (cMainMenuItem*)Get(Current());
          step = 0;
          while(step < 4)
          {
            while(mItem && step == 0)
            {
              if(mItem->Type() == iCat)
              {
                step++;
                break;
              }
              mItem = (cMainMenuItem*)Prev(mItem);
            }
            while(mItem && step == 1)
            {
              if(mItem->Type() != iCat)
              {
                step++;
                break;
              }
              mItem = (cMainMenuItem*)Prev(mItem);
            }
            while(mItem && step == 2)
            {
              if(mItem->Type() == iCat)
              {
                step++;
                break;
              }
              mItem = (cMainMenuItem*)Prev(mItem);
            }
#if VDRVERSNUM >= 10327
            if(!mItem && Setup.MenuScrollWrap)
            {
              mItem = (cMainMenuItem*)Last();
              if(mItem->Type() != iCat)
                step = 2;
              else
                step = 1;
            }
            else
#endif
              step = 4;
          }
          if(mItem)
          {
            SetCurrent(Next(mItem));
            Display();
          }
        }
        else
        {
          state = cOsdMenu::ProcessKey(Key);
        }
        if(Count() &&
           Current() == FirstSelectable)
        {
          SetCurrent(Get(0));
          Display();
          SetCurrent(Get(FirstSelectable));
          Display();
        }
        break;
      case kRight:
      case kRight|k_Repeat:
        if(Count() &&
           DVDSwitchSetup.JumpCatByKey)
        {
          mItem = (cMainMenuItem*)Get(Current());
          step = 0;
          while(step < 3)
          {
            while(mItem && step == 0)
            {
              if(mItem->Type() == iCat)
              {
                step++;
                break;
              }
              mItem = (cMainMenuItem*)Next(mItem);
            }
            while(mItem && step == 1)
            {
              if(mItem->Type() != iCat)
              {
                step++;
                break;
              }
              mItem = (cMainMenuItem*)Next(mItem);
            }
#if VDRVERSNUM >= 10327
            if(!mItem && Setup.MenuScrollWrap)
            {
              mItem = (cMainMenuItem*)First();
              step = 1;
            }
            else
#endif
              step = 3;
          }
          if(mItem)
          {
            SetCurrent(mItem);
            Display();
          }
        }
        else
        {
          state = cOsdMenu::ProcessKey(Key);
        }
        if(Count() &&
           Current() == FirstSelectable)
          {
            SetCurrent(Get(0));
            Display();
            SetCurrent(Get(FirstSelectable));
            Display();
          }
        break;
      default:
        state = cOsdMenu::ProcessKey(Key);
        break;
    }
  }
  else
  {
     state = cOsdMenu::ProcessKey(Key);
  }
  SetHelp();

  if (Get(Current()))
  {
     mItem = (cMainMenuItem*)Get(Current());
     MainMenuOptions.setLastSelectItemName(mItem->FileName());
    MainMenuOptions.LastSelectItemType(mItem->Type());
  }
  return state;
}


eOSState cMainMenu::SelectItem(void)
{
  cMainMenuItem *mItem = (cMainMenuItem*)Get(Current());
  cFileInfo *info = NULL;
  cDirHandling *DirHand = NULL;

  if (mItem)
  {
    switch(mItem->Type())
    {
      case iParent:
      case iDir:
        info = new cFileInfo(mItem->FileName());
        if(!info->isExecutable() || !info->isReadable())
        {
          DELETENULL(info);
          OSD_ERRMSG(tr("no Rights!"));
          return osContinue;
          break;
        }
        DELETENULL(info);
        DirHand = new cDirHandling(this, &MainMenuOptions);
        DirHand->ProcessKey(mItem);
        delete(DirHand);
        Build(MainMenuOptions.CurrentDir());
        Display();
        break;
      case iDVD:
      case iDevice:
      case iDataBase:
        return cCMD::Play(mItem);
        break;
      default:
        break;
    }
  }

  return osContinue;
}

eOSState cMainMenu::Commands(eKeys Key)
{
  eOSState state = osUnknown;
  cMainMenuItem *mItem = (cMainMenuItem*)Get(Current());
  eCMDs cmd = cmdNone;
  cCMDImage *CMDImage = NULL;

  if(!HasSubMenu())
  {
    switch(Key)
    {
      case k1:
        cmd = (eCMDs)DVDSwitchSetup.k1;
        break;
      case k2:
        cmd = (eCMDs)DVDSwitchSetup.k2;
        break;
      case k3:
        cmd = (eCMDs)DVDSwitchSetup.k3;
        break;
      case k4:
        cmd = (eCMDs)DVDSwitchSetup.k4;
        break;
      case k5:
        cmd = (eCMDs)DVDSwitchSetup.k5;
        break;
      case k6:
        cmd = (eCMDs)DVDSwitchSetup.k6;
        break;
      case k7:
        cmd = (eCMDs)DVDSwitchSetup.k7;
        break;
      case k8:
        cmd = (eCMDs)DVDSwitchSetup.k8;
        break;
      case k9:
        cmd = (eCMDs)DVDSwitchSetup.k9;
        break;
      case k0:
        cmd = (eCMDs)DVDSwitchSetup.k0;
        break;
      case kBlue:
        cmd = (eCMDs)DVDSwitchSetup.kRed;
        break;
      case kGreen:
        cmd = (eCMDs)DVDSwitchSetup.kGreen;
        break;
      case kYellow:
        cmd = (eCMDs)DVDSwitchSetup.kYellow;
        break;
      case kRed:
        cmd = (eCMDs)DVDSwitchSetup.kBlue;
        break;
      case kOk:
        cmd = (eCMDs)DVDSwitchSetup.kOk;
        break;
      default:
        break;
    }
    switch(cmd)
    {
      case cmdDirManage:
        return AddSubMenu(new cCMDDir(this));
        break;
      case cmdDVDopen:
        return cCMD::Eject(false);
        break;
      case cmdDVDclose:
        return cCMD::Eject(true);
        break;
      case cmdImgPlay:
        return SelectItem();
        break;
      case cmdImgRename:
        if(mItem && mItem->Type() == iDVD)
        {
          SetState(mmsImgRename);
          cOsdMenu::ProcessKey(kRight);
          return osContinue;
        }
        break;
      case cmdImgMove:
        return AddSubMenu(new cCMDMove(mItem->FileName(), this, false, true));
        break;
      case cmdImgDelete:
        if(mItem && mItem->Type() == iDVD)
        {
          CMDImage = new cCMDImage(this);
          state = CMDImage->Delete(mItem->FileName());
          DELETENULL(CMDImage);
	  refreshList = true;
          Build(MainMenuOptions.CurrentDir());
	  Display();
          return osContinue;
        }
        break;
      case cmdImgBurn:
        if(mItem && mItem->Type() == iDVD)
        {
          CMDImage = new cCMDImage(this);
          state = CMDImage->Burn(mItem->FileName());
          DELETENULL(CMDImage);
          return osContinue;
        }
        break;
      case cmdImgRead:
        {
        //cMainMenuItem *item = new cMainMenuItem(iDVD, "CREATE");
        //return AddSubMenu(new cCMDMenu(mItem, this));
        eOSState res =  AddSubMenu(new cCMDImageRead());
        refreshList = true;
        Build(MainMenuOptions.CurrentDir());
	Display();
	return res;
        break;
        }
      case cmdDeleteCache:
        return osContinue;
      case cmdCommands:
        return AddSubMenu(new cCMDMenu(mItem, this));
      default:
        break;
    }
  }

  return cOsdMenu::ProcessKey(Key);
}

char *cMainMenu::CreateOSDName(eMainMenuItem itype, char *file)
{
  cStringValue buffer;
  cStringValue tmpOSD;
  cFileInfo *info = NULL;
  char *reg = NULL;
  int i = 0;
  int len = 0;

  switch(itype)
  {
    case iCat:
      asprintf(&reg, "^%s", MainMenuOptions.ImageDir());
      if(!RegIMatch(file, reg))
        buffer = file;
      else
      {
        char *p = &file[strlen(MainMenuOptions.ImageDir())];
        if(p[0] == '/')
          p++;
        cTokenizer *token = new cTokenizer(p, "/");
        for (i = 1; i <= token->Count(); i++)
        {
          ChangeChars(token->GetToken(i), DVDSwitchSetup.ChangeCharsOSDName);
          buffer += token->GetToken(i);
          if(i != token->Count())
            buffer += DVDSwitchSetup.SubCatCutter;
        }
        DELETENULL(token);
      }
      FREENULL(reg);
      for (i = 0; i < DVDSwitchSetup.CharCountBeforeCat; i++)
        tmpOSD += &DVDSwitchSetup.CatLineChar;
      if(DVDSwitchSetup.SpacesBeforeAfterCat)
        tmpOSD += " ";
      if(isempty(&buffer))
        tmpOSD += (char*)tr("without Category");
      else
        tmpOSD += &buffer;
      if(DVDSwitchSetup.SpacesBeforeAfterCat)
        tmpOSD += " ";
      len = tmpOSD.len();
      for (i = 0; i < (119 - len); i++)
        tmpOSD += &DVDSwitchSetup.CatLineChar;
      break;
    case iParent:
#if 0
      buffer = "..";
#else
      buffer = "\x80\t";
      tmpOSD += &buffer;
      break;
#endif
    case iDir:
      if(!&buffer && file)
      {
        info = new cFileInfo(file);
        buffer = info->FileName();
      }
#if 0
      tmpOSD = "[";
      tmpOSD += &buffer;
      tmpOSD += "]";
#else
      tmpOSD = "\x82\t";
      tmpOSD += &buffer;
#endif
      break;
    case iDVD:
      info = new cFileInfo(file);
      if(ImageList.IsHide(info->Extension()))
        buffer = info->FileNameWithoutExt();
      else
        buffer = info->FileName();
      ChangeChars(&buffer, DVDSwitchSetup.ChangeCharsOSDName);

      tmpOSD += &buffer;
      tmpOSD += "\t";

      //      if(DVDSwitchSetup.HideImgSizeCol) AR
      {
        char *size = NULL;
        asprintf(&size, tr("(Size %03.1f GB)\t"), info->SizeGByte(1)); //AR
        tmpOSD += size;
        free(size);
      }

      if(DVDSwitchSetup.HideTypeCol)
      {
          tmpOSD += "\t";
          tmpOSD += ImageList.GetShortName(file);
      }
      //tmpOSD = &buffer;
      break;

    case iDevice:
      tmpOSD = "--> (";
      tmpOSD += (char*) tr("DVD-Drive");
      tmpOSD += ") <--";
      break;

    case iDataBase:
      tmpOSD  = file;
      tmpOSD += "\t";
      tmpOSD += "(";
      tmpOSD += tr("insert Media");
      tmpOSD += ")";

    default:
      break;
  }

  delete(info);
  if (reg) free(reg);

  return strdup(&tmpOSD);
}

void cMainMenu::SetState(eMainMenuState state)
{
  State = state;
  cMainMenuItem *mItem = NULL;
  cMainMenuItem *dmItem = NULL;
  cFileInfo *info = NULL;

  switch(State)
  {
    case mmsReInit:
      SetState(mmsNone);
      Init();
      Build(MainMenuOptions.CurrentDir());
      break;
    case mmsReInitCur:
      SetState(mmsNone);
      Build(MainMenuOptions.CurrentDir());
      break;
    case mmsImgRename:
      DELETENULL(CMDImg);
      mItem = (cMainMenuItem*)Get(Current());
      if(mItem && mItem->Type() != iDVD)
      {
        SetState(mmsNone);
        break;
      }
      info = new cFileInfo(mItem->FileName());
      if(!info->isWriteable())
      {
        DELETENULL(info);
        info = new cFileInfo(MainMenuOptions.CurrentDir());
        if(!info->isWriteable())
        {
          OSD_ERRMSG(tr("no Rights to rename"));
          SetState(mmsNone);
          DELETENULL(info);
          break;
        }
      }
      DELETENULL(info);
      CMDImg = new cCMDImage();
      CMDImg->Rename(mItem->FileName());
      CMDImg->tmpHideTypeCol = DVDSwitchSetup.HideTypeCol;
      if(DVDSwitchSetup.HideTypeCol)
      {
        DVDSwitchSetup.HideTypeCol = 0;
        Build(MainMenuOptions.CurrentDir());
      }
      SetCols(11);
      mItem = (cMainMenuItem*)First();
      while(mItem)
      {
        if(mItem->Index() == Current())
        {
          info = new cFileInfo(mItem->FileName());
          if(ImageList.IsHide(info->Extension()))
            strn0cpy(CMDImg->NewFile, info->FileNameWithoutExt(), MaxFileName);
          else
            strn0cpy(CMDImg->NewFile, info->FileName(), MaxFileName);
          DELETENULL(info);
          Ins(new cMenuEditStrItem(tr("Rename"), CMDImg->NewFile, MaxFileName, trVDR(FileNameChars)),
              true,
              mItem);
          dmItem = mItem;
        }
        mItem->SetSelectable(false);
        mItem = (cMainMenuItem*)Next(mItem);
      }
      if(dmItem)
        Del(dmItem->Index());
      Display();
      break;
    default:
      break;
  }
}

// --- cDirHandling -----------------------------------------

cDirHandling::cDirHandling(cOsdMenu *osdobject, cDirHandlingOpt *dirobject)
{
  OsdObject = osdobject;
  DirObject = dirobject;
}

int cDirHandling::Build(char *dir, bool emptydirs)
{
  int ret = -1;
  cMainMenuItem *mItem = NULL;

  cDirList *DirList = new cDirList();
  if(DirList->Load(dir, false))
  {
    if(DirObject->isParent(dir))
      OsdObject->Add(new cMainMenuItem(iParent, DirObject->ParentDir()));

    cFileListItem *fItem = DirList->First();
    while(fItem)
    {
      if(!DirList->DirIsIn(fItem, ImageList.GetDirContains()) &&
          (!DirList->DirIsEmpty(fItem) || emptydirs))
      {
        OsdObject->Add(new cMainMenuItem(iDir, fItem->Value()));
        mItem = (cMainMenuItem*)OsdObject->Last();
        if(mItem &&
           DirObject->getLastSelectItemName() &&
           !strcasecmp(mItem->FileName(), DirObject->getLastSelectItemName()))
          ret = mItem->Index();
        if(mItem && DirObject->isLastSel(mItem->FileName()))
          ret = mItem->Index();
      }
      fItem = DirList->Next(fItem);
    }
  }

  return ret;
}

void cDirHandling::ProcessKey(cMainMenuItem *mItem)
{
  cFileInfo *info = NULL;

  if (mItem)
  {
    switch(mItem->Type())
    {
      case iParent:
        DirObject->LastSelDir(DirObject->CurrentDir());
        DirObject->CurrentDir(DirObject->ParentDir());
        if(strcasecmp(DirObject->ImageDir(), DirObject->ParentDir()))
        {
          info = new cFileInfo(DirObject->ParentDir());
          DirObject->ParentDir(info->Path());
          DELETENULL(info);
        }
        break;
      case iDir:
        DirObject->ParentDir(DirObject->CurrentDir());
        DirObject->CurrentDir(mItem->FileName());
        break;
      default:
        break;
    }
  }
}
