#include <vdr/tools.h>
#include "dvdlist.h"
#include "setup.h"
#include "helpers.h"
#include "imagelist.h"
#include "menu.h"
#include "tools.h"
#include <vdr/dvdIndex.h>


// --- cDVDListThread ------------------------------------------------------------
cDVDListThread::cDVDListThread()
: cThread("DVDSwitch")
{
  menu = NULL;
  RunningMenu = false;
  SelectFirst = NULL;
  dir = NULL;
  FirstSelectable = -1;
}

bool cDVDListThread::BuildDisp(cMainMenu *menu, void (cMainMenu::*cb) (int), char *dir)
{
  if(!Active()) {
    this->menu = menu;
    RunningMenu = true;
    SelectFirst = cb;
    this->dir = dir;
// Workaround for Avantgarde
#ifdef RBLITE
    Start();
#else
    Action();
#endif
    return true;
  }
  return false;
}

void cDVDListThread::Add(cMainMenuItem *item)
{
  mutex.Lock();
  if(menu)
    menu->Add(item);
  mutex.Unlock();
}
void cDVDListThread::BuildDisp0(void)
{
  if (refreshList) {
   delete(DVDList);
   DVDList = new cDVDList(&RunningMenu);
   }
  cMainMenuItem *mItem = NULL;
  MYDEBUG("Bilde Menu nach DisplayMode 0");

  if(DVDSwitchSetup.DisplayDVDDevice)
  {
    MYDEBUG("Füge Eintrag für das DVD-Device hinzu");
    Add(new cMainMenuItem(iDevice));
    if(!MainMenuOptions.getLastSelectItemName() && MainMenuOptions.LastSelectItemType() == iDevice)
      FirstSelectable = 0;
  }
 
  
 if (refreshList)  DVDList->Create(dir,
                     ImageList.GetExtensions(),
                     ImageList.GetDirContains(),
                     (eFileList)DVDSwitchSetup.SortMode,
                     true);
  cImageListItem *iItem = ImageList.First();
  while(iItem) { printf(" -- %s \"%s\" : %d \n", iItem->GetSName(), iItem->GetValue(), iItem->GetFType()); iItem = ImageList.Next(iItem); }
  printf(" \n ** %s \n",ImageList.GetExtensions());
  printf(" \n ** %s \n",ImageList.GetDirContains());

  MYDEBUG("DVDList erstellt");
  cDVDListItem *item = DVDList->First();
  while(menu && item)
  {
    Add(new cMainMenuItem(iDVD, item->FileName()));
    if(MainMenuOptions.getLastSelectItemName() &&
       !strcasecmp(item->FileName(), MainMenuOptions.getLastSelectItemName()))
    {
      mItem = (cMainMenuItem*) menu->Last();
      FirstSelectable = mItem->Index();
      mItem = NULL;
    }
    item = DVDList->Next(item);
  }

  if(menu && menu->Count() && FirstSelectable < 0)
    FirstSelectable = 0;

  //delete(DVDList);
}

void cDVDListThread::BuildDisp1(void)
{
  
  cImageListItem *iItem = NULL;
  if (refreshList){ 
   delete DVDList; 
   DVDList = new cDVDList(&RunningMenu);
   }
  cDVDListItem *dItem = NULL;
  cDirList *DirList = NULL;
  cFileListItem *fItem = NULL;
  cMainMenuItem *mItem = NULL;

  switch(DVDSwitchSetup.CategorieType)
  {
	  case 0: // Image-Type
		  if(DVDSwitchSetup.DisplayDVDDevice)
		  {

			  MYDEBUG("Füge Eintrag für das DVD-Device hinzu");
			  Add(new cMainMenuItem(iCat, dir));
			  Add(new cMainMenuItem(iDevice));
			  if(!MainMenuOptions.getLastSelectItemName() && MainMenuOptions.LastSelectItemType() == iDevice)
				  FirstSelectable = 1;
		  }
		  iItem = ImageList.First();
		  while(menu && iItem)
		  {
			  if( DVDList->Create(dir, iItem->GetFType() == tFile ? iItem->GetValue() : NULL,
						  iItem->GetFType() == tDir ? iItem->GetValue() : NULL,
						  (eFileList)DVDSwitchSetup.SortMode,
						  true) &&
					  (DVDList->Count() || !DVDSwitchSetup.HideEmptyDirs))
			  {
				  Add(new cMainMenuItem(iCat, iItem->GetSName()));
				  dItem = DVDList->First();
				  while(menu && dItem)
				  {
					  Add(new cMainMenuItem(iDVD, dItem->FileName()));
					  if(MainMenuOptions.getLastSelectItemName() &&
							  !strcasecmp(dItem->FileName(), MainMenuOptions.getLastSelectItemName()))
					  {
						  mItem = (cMainMenuItem*) menu->Last();
						  FirstSelectable = mItem->Index();
						  mItem = NULL;
					  }
					  dItem = DVDList->Next(dItem);
				  }
			  }
			  iItem = ImageList.Next(iItem);
		  }
		  break;
    case 1: // Directories
      if(DVDList->Create(dir,
                         ImageList.GetExtensions(),
                         ImageList.GetDirContains(),
                         (eFileList)DVDSwitchSetup.SortMode,
                         false) &&
         (DVDList->Count() || !DVDSwitchSetup.HideEmptyDirs || DVDSwitchSetup.DisplayDVDDevice))
      {
        Add(new cMainMenuItem(iCat, dir));
        if(DVDSwitchSetup.DisplayDVDDevice)
        {
          MYDEBUG("Füge Eintrag für das DVD-Device hinzu");
          Add(new cMainMenuItem(iDevice));
          if(!MainMenuOptions.getLastSelectItemName() && MainMenuOptions.LastSelectItemType() == iDevice)
            FirstSelectable = 1;
        }
        dItem = DVDList->First();
        while(menu && dItem)
        {
          Add(new cMainMenuItem(iDVD, dItem->FileName()));
          if(MainMenuOptions.getLastSelectItemName() &&
             !strcasecmp(dItem->FileName(), MainMenuOptions.getLastSelectItemName()))
          {
            mItem = (cMainMenuItem*) menu->Last();
            FirstSelectable = mItem->Index();
            mItem = NULL;
          }
          dItem = DVDList->Next(dItem);
        }
      }
      DirList = new cDirList();
      if(DirList->Load(dir, true))
      {
        fItem = DirList->First();
        while(menu && fItem)
        {
          if(!DirList->DirIsIn(fItem, ImageList.GetDirContains()) &&
              DVDList->Create(fItem->Value(),
                             ImageList.GetExtensions(),
                             ImageList.GetDirContains(),
                             (eFileList)DVDSwitchSetup.SortMode,
                             false) &&
             (DVDList->Count() || !DVDSwitchSetup.HideEmptyDirs))
          {
            Add(new cMainMenuItem(iCat, fItem->Value()));
            dItem = DVDList->First();
            while(menu && dItem)
            {
              Add(new cMainMenuItem(iDVD, dItem->FileName()));
              if(MainMenuOptions.getLastSelectItemName() &&
                 !strcasecmp(dItem->FileName(), MainMenuOptions.getLastSelectItemName()))
              {
                mItem = (cMainMenuItem*) menu->Last();
                FirstSelectable = mItem->Index();
                mItem = NULL;
              }
              dItem = DVDList->Next(dItem);
            }
          }
          fItem = DirList->Next(fItem);
        }
      }
      DELETENULL(DirList);
      break;
    case 2: // FileType
      if(DVDSwitchSetup.DisplayDVDDevice)
      {
        MYDEBUG("Füge Eintrag für das DVD-Device hinzu");
        Add(new cMainMenuItem(iCat, dir));
        Add(new cMainMenuItem(iDevice));
        if(!MainMenuOptions.getLastSelectItemName() && MainMenuOptions.LastSelectItemType() == iDevice)
          FirstSelectable = 1;
      }
      if(DVDList->Create(dir,
         ImageList.GetExtensions(),
         NULL,
         (eFileList)DVDSwitchSetup.SortMode,
         true) &&
         (DVDList->Count() || !DVDSwitchSetup.HideEmptyDirs))
      {
        Add(new cMainMenuItem(iCat, (char*)tr("Image-File")));
        dItem = DVDList->First();
        while(menu && dItem)
        {
          Add(new cMainMenuItem(iDVD, dItem->FileName()));
          if(MainMenuOptions.getLastSelectItemName() &&
             !strcasecmp(dItem->FileName(), MainMenuOptions.getLastSelectItemName()))
          {
            mItem = (cMainMenuItem*) menu->Last();
            FirstSelectable = mItem->Index();
            mItem = NULL;
          }
          dItem = DVDList->Next(dItem);
        }
      }
      if( DVDList->Create(dir,
         NULL,
         ImageList.GetDirContains(),
         (eFileList)DVDSwitchSetup.SortMode,
         true) &&
         (DVDList->Count() || !DVDSwitchSetup.HideEmptyDirs))
      {
        Add(new cMainMenuItem(iCat, (char*)tr("Image-Directory")));
        dItem = DVDList->First();
        while(menu && dItem)
        {
          Add(new cMainMenuItem(iDVD, dItem->FileName()));
          if(MainMenuOptions.getLastSelectItemName() &&
             !strcasecmp(dItem->FileName(), MainMenuOptions.getLastSelectItemName()))
          {
            mItem = (cMainMenuItem*) menu->Last();
            FirstSelectable = mItem->Index();
            mItem = NULL;
          }
          dItem = DVDList->Next(dItem);
        }
      }
      break;
    default:
      break;
  }

  MYDEBUG("Ermittle FirstSelectable");
  if (menu)
  {
  mItem = (cMainMenuItem*) menu->First();
  if(FirstSelectable < 0)
  {
    while(mItem && !mItem->Selectable())
      mItem = (cMainMenuItem*) menu->Next(mItem);
    if(mItem)
      FirstSelectable = mItem->Index();
  }
}
  //delete(DVDList);
  delete(DirList);
}

void cDVDListThread::BuildDisp2(void)
{
	cMainMenuItem *mItem = NULL;
	if (refreshList)
	{
		delete (DVDList);
		DVDList = new cDVDList(&RunningMenu);
	}
	cDVDListItem *dItem = NULL;

	cDirHandling *DirHand = new cDirHandling(menu, &MainMenuOptions);
	FirstSelectable = DirHand->Build(dir, !DVDSwitchSetup.HideEmptyDirs);
	delete(DirHand);

	if(DVDSwitchSetup.DisplayDVDDevice && !strcasecmp(dir, MainMenuOptions.ImageDir()))
	{
		MYDEBUG("Füge Eintrag für das DVD-Device hinzu");
		Add(new cMainMenuItem(iDevice));
		if(!MainMenuOptions.getLastSelectItemName() && MainMenuOptions.LastSelectItemType() == iDevice)
		{
			mItem = (cMainMenuItem*) menu->Last();
			FirstSelectable = mItem->Index();
		}
	}

	if(DVDList->Create(dir, ImageList.GetExtensions(), ImageList.GetDirContains(),
				(eFileList)DVDSwitchSetup.SortMode, false))
	{
		dItem = DVDList->First();
		while(menu && dItem)
		{
			Add(new cMainMenuItem(iDVD, dItem->FileName()));
			if(MainMenuOptions.getLastSelectItemName() &&
					!strcasecmp(dItem->FileName(), MainMenuOptions.getLastSelectItemName()))
			{
				mItem = (cMainMenuItem*) menu->Last();
				FirstSelectable = mItem->Index();
				mItem = NULL;
			}
			dItem = DVDList->Next(dItem);
		}
	}

	//delete(DVDList);

	if(menu)
	{
		mItem = (cMainMenuItem*) menu->First();
		if(FirstSelectable < 0)
			FirstSelectable = 0;
	}
}

void cDVDListThread::Action(void)
{
  switch(DVDSwitchSetup.DisplayMode)
  {
    case 0:
      BuildDisp0();
      break;
    case 1:
    refreshList = true;
      BuildDisp1();
      break;
    case 2:
    refreshList = true;
      BuildDisp2();
      break;
  }

  /// add Database items here
  cMainMenuItem *item;
  cMediaDataBase db( DVD_DB_FILENAME );
  cDataBaseEntry dbEntry;
  db.Rewind(); // go to first entry

  if(db.Size()>0) 
  { 
	  item = new cMainMenuItem( iCat, tr("Previously seen DVDs") ) ;
	  item->SetSelectable(false);
	  Add( item );

	  // add entries to OSD
	  while(db.Next(dbEntry))
	  {
		  item = new cMainMenuItem( iDataBase, dbEntry.VolName().c_str() ) ;
		  Add( item );
	  }
  }
  printf("(%s:%d) db size: %ld\n", __FILE__,__LINE__, db.Size() );

  printf("%s DisplayMode %d\n",__PRETTY_FUNCTION__ , DVDSwitchSetup.DisplayMode);
  if (menu) menu->SetStatus(NULL); // clear status message "searching for CD/DVD images"
  refreshList = false;

  mutex.Lock();
  if(menu && SelectFirst) {
    mutex.Unlock();
    (menu->*SelectFirst)(FirstSelectable);
  }
  else
    mutex.Unlock();
}

void cDVDListThread::SetCB(cMainMenu *menu, void (cMainMenu::*cb) (int))
{
    mutex.Lock();
    SelectFirst = cb;
    this->menu = menu;
    RunningMenu = true;
    mutex.Unlock();
}

// --- cDVDList ------------------------------------------------------------------

bool cDVDList::Create(char *dir, char *exts, char *dirs, eFileList smode, bool sub)
{
  MYDEBUG("DVDList: %s, %s", exts, dirs);

  Clear();
  FREENULL(DVDExts);
  FREENULL(DVDDirs);

  DVDExts = exts ? strdup(exts) : NULL;
  DVDDirs = dirs ? strdup(dirs) : NULL;

  if(!DVDExts && !DVDDirs)
    return false;

  return Load(dir, smode, sub);
}

bool cDVDList::Load(char *dir, eFileList smode, bool sub)
{
  MYDEBUG("DVDList: Load");
  bool ret = false;
  int i = 0;

  cFileInfo *fInfo = NULL;
  cFileList *fList = new cFileList();

  fList->OptExclude("^lost\\+found$"); // lost+found Dir
  fList->OptExclude("^\\."); // hidden Files
  fList->OptExclude("\\.sdel$"); // del Files
  fList->OptExclude("\\.smove$"); // move Files

  if(DVDExts)
  {
    cTokenizer *token = new cTokenizer(DVDExts, "@");
    char *extexp = NULL;
    for(i = 1; i <= token->Count(); i++)
    {
      asprintf(&extexp, "%s$", token->GetToken(i));
      fList->OptInclude(extexp, tFile);
      FREENULL(extexp);
    }
    delete(token);
  }

  if(DVDDirs)
    fList->OptInclude(".*", tDir);

  fList->OptSort(smode, true);

  ret = fList->Load(dir, sub);

  cFileListItem *fItem = fList->First();
  while(*RunningMenu && fItem)
  {
    fInfo = new cFileInfo(fItem->Value());
    if(fInfo->Type() == tFile || (fInfo->Type() == tDir && fList->DirIsIn(fItem, DVDDirs)))
      Add(new cDVDListItem(fItem->Value()));
    DELETENULL(fInfo);
    fItem = fList->Next(fItem);
  }

  delete(fList);
  delete(fInfo);

  return ret;
}
