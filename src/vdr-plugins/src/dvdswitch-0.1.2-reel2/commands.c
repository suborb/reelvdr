#include <unistd.h>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

#include <vdr/debug.h>
#include <vdr/tools.h>

#include "commands.h"
#include "setup.h"
#include "tools.h"
#include "menu-item.h"
#include "imagelist.h"
#include "xineplugin.h"
#include "setup-itypes.h"

#include "../bgprocess/service.h"
#include "../xinemediaplayer/services.h"

cStatusThread::cStatusThread(int m, std::string filename, std::string desc) {
	   mode = (m==BurnMode?BurnMode:ReadMode);
	   logFileName = filename;
	   thread_desc = desc;
           if(mode==ReadMode)
	     PluginName = tr("Reading DVD");
           else
	     PluginName = tr("Writing DVD");
}

void cStatusThread::StopStatusThread(int waitTime) {
   NotifyBgProcess(PluginName, thread_desc, startTime, 110); // thread done!
   Cancel(waitTime);
}

void cStatusThread::Action() {
   startTime = time(NULL);
   int percentage_done=0;
   while(Running()) {
      switch(mode) {
          case ReadMode: percentage_done = StatusRead(); break;
          case BurnMode: percentage_done = StatusBurn(); break;
        }
   	    MYDEBUG(" percent %d mode%d\n",percentage_done, mode);
        if (percentage_done > 100)
               StopStatusThread(0);
   	    else
            NotifyBgProcess(PluginName, thread_desc, startTime, percentage_done);

   	    sleep(2);
    }
}

int cStatusThread::StatusBurn() {
     float percentage_done = 0;
     std::string line;
     std::ifstream ifile;

     ifile.open(logFileName.c_str(), std::ios::in);
     while(ifile.good()) {
         getline(ifile,line);
         if (line == "DONE")
            return 1000;  // over
         if (line.find("done, estimate finish") != std::string::npos)
             sscanf(line.c_str()," %f", &percentage_done);
   }
   return (int) percentage_done;
}

int cStatusThread::StatusRead() { // reads the logFile till the end and returns the status
      long long diskSize;
	  std::ifstream ifile;
	  std::string line, part_str;
	  int count = 0;
	  long long total_size_written = 0;
	  long long last_written_size = 0;
	  int last_pos = 0;
	  int curr_pos = 0;
	  unsigned int pos_tmp;
	  count = 0;

	  ifile.open (logFileName.c_str(), std::ios::in);
	  while (ifile.good ()) {
	    getline (ifile, line);
        if (line == "DONE") return 1000;  // over

	    pos_tmp = line.find ("Vobs size: ");
	    if (pos_tmp != std::string::npos) {
	      sscanf (line.substr (pos_tmp + 11).c_str (), " %lld", &diskSize);
	      diskSize *= 1024*1024; // Vobcopy reports disk size in MB
	    } // END if "Vobs size"


	    if (line.find ("written") != std::string::npos) {
	      long long total_size, done_size;
	      char size_str[5], tmp_str[32];
	      while (curr_pos != (int)std::string::npos) {
	        curr_pos = line.find (0x0d, last_pos + 1);      // '^M'
	        if (curr_pos == (int)std::string::npos)
	          break;
	        part_str = line.substr (last_pos, curr_pos - 1);
	        if (!sscanf (part_str.c_str (), " %lld %s %s %lld %s %s", &done_size, size_str, tmp_str, &total_size, size_str, tmp_str)) break;                // error or EOF
	        {
	          if (size_str[0] == 'k' || size_str[0] == 'K') // asdiskSizee KB
	            done_size *= 1024;
	          if (size_str[0] == 'm' || size_str[0] == 'M') // asdiskSizee MB
	            done_size *= 1024 * 1024;
	          if (size_str[0] == 'g' || size_str[0] == 'G') // asdiskSizee GB
	            done_size *= 1024 * 1024 * 1024;

	          last_written_size = done_size;
	        }
	        last_pos = curr_pos;
	      }
	      last_pos = curr_pos = 0;
	    } // END if written

    	total_size_written += last_written_size;
	    last_written_size = 0;
	  } // END while !eof
	  ifile.close();
	  //cout <<  total_size_written << " bytes written  " << total_size_written * 100.0 / diskSize << "% done" << endl;

      return  (int)(total_size_written * 100.0 / diskSize);

}

void cCMDImageReadThread::Action(void) {
  MYDEBUG("ReadThread gestartet");
  if(File && Dir && FileType != tNone) {
    MYDEBUG("Aktion wird ausgefuehrt");
    char *cmd = NULL;
    char *output = NULL;
    char *mountpoint = NULL;
    int step = 0;
    cTokenizer *token = NULL;
    //char buffer[MaxFileName];
    char *buffer = NULL;
    //MYDEBUG("vor realpath '%s'", DVDSwitchSetup.DVDLinkOrg);

    if (DVDSwitchSetup.DVDLinkOrg == NULL)
    {
	esyslog("DVDSwitchSetup.DVDLinkOrg is empty. Cannot continue");
	return;
    }
    if (realpath(DVDSwitchSetup.DVDLinkOrg, buffer)== NULL)
        buffer[0] = '\0';

    while(!mountpoint && step < 2) {
      step++;
      asprintf(&cmd, "cat /etc/fstab | grep -e \"%s\" | grep -e \"^[^#]\"", step == 1 ? DVDSwitchSetup.DVDLinkOrg : buffer);
      MYDEBUG("Command: %s", cmd);

      FILE *p = popen(cmd, "r");
      if(p) {
#if VDRVERSNUM >= 10318
        cReadLine rl;
        output = rl.Read(p);
#else
        output = readline(p);
#endif
        pclose(p);
      }
      token = new cTokenizer(output, " ");
      if(token->Count() > 1)
        mountpoint = strdup(stripspace(token->GetToken(2)));
      DELETENULL(token);
    }
    FREENULL(cmd);
    asprintf(&cmd, "'%s' '%s' '%s' '%s' '%s' '%s'",
              DVDSwitchSetup.DVDReadScript,
              Dir, // /media/hd/movies
              File, // leer
              buffer, (mountpoint==NULL) ? " ":mountpoint,
              (FileType == tFile) ? "IMAGE" : "DIR");
    MYDEBUG("Aufruf: %s", cmd);
    // Start thread here
    int rc = SystemExec(cmd);
    cStatusThread* read_status = new cStatusThread(ReadMode,"/tmp/vobcopy_1.1.0.log",tr("Copying DVD"));
    read_status->Start();
    MYDEBUG("Aufruf: %s DONE with %d", cmd, rc);
    MYDEBUG("Rueckgabe Aufruf: %i", rc);
    FREENULL(cmd);
    FREENULL(mountpoint);
    FREENULL(buffer);
  }
  delete(this);
};

cCMDImageReadThread::cCMDImageReadThread(char *file, char *dir, int imgtype) {
  MYDEBUG("ReadThread created");
  File = NULL;
  Dir = NULL;
  FileType = tNone;

  MYDEBUG ( "  File: %s dir : %s  imgType %d " , file, dir ,imgtype);
  cImageListItem *item = ImageList.Get(imgtype);
  if(imgtype >= 0 && file && dir) {
    if(item->GetFType() == tFile) {
      asprintf(&File, "%s%s", file, item->GetValue());
      MYDEBUG ( "  File: %s itemValue %s  ", file, item->GetValue());
    } else {
      File = strdup(file);
      MYDEBUG ( "  File: %s itemValue %s  ", file);
    }
    Dir = strdup(dir);
    FileType = item->GetFType();
  }
}

cCMDImageReadThread::~cCMDImageReadThread(void) {
  free(File);
  free(Dir);
}

void cCMDImageBurnThread::Action(void) {
  MYDEBUG("BurnThread gestartet");
  if(File && FileType != tNone) {
    MYDEBUG("Aktion wird ausgefuehrt");
    char *cmd;
    asprintf(&cmd,
              "'%s' '%s' '%s'",
              DVDSwitchSetup.DVDWriteScript,
              File,
              FileType == tFile ? "IMAGE" : "DIR");
    MYDEBUG("Aufruf: %s", &cmd);
    int rc = SystemExec(cmd);
    cStatusThread* burn_status = new cStatusThread(BurnMode,"/var/log/dvdswitch_burn.log","Writing DVD");
    burn_status->Start();
    MYDEBUG("Rueckgabe Aufruf: %i", rc);
    FREENULL(cmd);
  }
  delete(this);
};

cCMDImageBurnThread::cCMDImageBurnThread(char *file, eFileInfo type) {
  MYDEBUG("BurnThread created");
  File = NULL;
  FileType = tNone;

  if(file && type != tNone) {
    File = strdup(file);
    FileType = type;
  }
}

cCMDImageBurnThread::~cCMDImageBurnThread(void) {
  free(File);
}

// --- cCMD ----------------------------------------------------------

eOSState cCMD::Play(cMainMenuItem *item)
{
  MYDEBUG(" --------- %s  item: %s \n", __PRETTY_FUNCTION__,item->FileName());
  if(item)
    MYDEBUG("Play Image %s, %i", item->FileName(), item->Type());
  else
    MYDEBUG("Play Image: Kein Image angegeben");

  if(!item || item->Type() == iDevice)
    cXinePlugin::Start();
  else if (item && item->Type() == iDVD) {
    cXinePlugin::Start(item->FileName());
  }

  if (item->Type() == iDataBase )
  {
	  std::string str= tr("Please insert DVD: ");
	  //TODO stop mediaD from detecting the inserted DVD.
	  str += item->FileName();
	  std::cout<<str<<std::endl;
	  bool flag =Interface->Confirm( str.c_str() );
	  std::cout<<str<<" "<<flag<<std::endl;
	  if( flag )
	  {

		  ///TODO check if DVD/media inserted is the correct one.
		  /// item->FileName() == Vobcopy -I ?

		  //cXinePlugin::Start(); // play the DVD

		  std::vector<std::string> playlistEntries; //empty list
		  Xinemediaplayer_Xine_Play_mrl xinePlayData;
          xinePlayData.mrl      = "dvd://"; 
          xinePlayData.instance = -1;
          xinePlayData.playlist = false;
          xinePlayData.playlistEntries = playlistEntries;

		  cPluginManager::CallAllServices("Xine Play mrl", &xinePlayData);
	  }

  }
  return osEnd;
}

eOSState cCMD::Eject(bool close)
{
  char *cmd = NULL;
  asprintf(&cmd, "eject %s %s", close ? "-t" : "", DVDSwitchSetup.DVDLinkOrg);
  MYDEBUG("Eject: %i - %s", close, cmd);
  int rc = SystemExec(cmd);
  MYDEBUG("Eject-Rückgabe: %i", rc);
  free(cmd);

  return osContinue;
}

// --- cCMDMenu -------------------------------------------------------------

cCMDMenu::cCMDMenu(cMainMenuItem *item, cMainMenu *osdobject)
  : cOsdMenu(tr("Commands"))
{
  iItem = item;
  OsdObject = osdobject;
  cOsdItem *mItem = NULL;

  SetHasHotkeys();

  if (iItem)
     MYDEBUG ("cMainMenu  FileName %s ItemType %d ", iItem->FileName(), iItem->Type());
  else
     MYDEBUG ("cMainMenu  no valid Item");

  //Add(new cOsdItem(hk(tr("Directory management"))));

  Add(new cOsdItem(hk(tr("Burn image"))));
  if(!iItem || (iItem && iItem->Type() != iDVD))
  {
	  mItem = Last();
	  mItem->SetSelectable(false);
  }

  Add(new cOsdItem(hk(tr("Rename image"))));
  if(!iItem || (iItem && iItem->Type() != iDVD))
  {
    mItem = Last();
    mItem->SetSelectable(false);
  }

  Add(new cOsdItem(hk(tr("Open DVD-tray"))));
#ifdef RBLITE
  Add(new cOsdItem(hk(tr("Close DVD-tray"))));
#endif
  /*Add(new cOsdItem(tr("Play")));
  if(!iItem
     || (iItem && iItem->Type() != iDVD && iItem->Type() != iDevice && iItem->Type()
         != iDataBase))
  {
    mItem = Last();
    mItem->SetSelectable(false);
  }
  */

  Add(new cOsdItem(hk(tr("Delete cache"))));
  Add(new cOsdItem(hk(tr("Format DVD-RW"))));

 /* Add(new cOsdItem(tr("Move image")));
  if(!iItem || (iItem && iItem->Type() != iDVD))
  {
    mItem = Last();
    mItem->SetSelectable(false);
  }
*/
/*
  Add(new cOsdItem(tr("Delete Image")));
  if(!iItem || (iItem && iItem->Type() != iDVD))
  {
    mItem = Last();
    mItem->SetSelectable(false);
  }
  Add(new cOsdItem(tr("Burn Image")));
  if(!iItem || (iItem && iItem->Type() != iDVD))
  {
    mItem = Last();
    mItem->SetSelectable(false);
  }
  Add(new cOsdItem(tr("Read DVD")));
*/
  Display();
}


eOSState cCMDMenu::ProcessKey(eKeys Key)
{
  eOSState ret = osUnknown;

  if(!HasSubMenu())
  {
    cCMDImage *CMDImage = NULL;
    cOsdItem *item = NULL;
    eCMDs cmd = cmdNone;
    std::string scmd;

	//int current = Current();  // remove

    switch(Key)
    {
      case kOk:
        item = Get(Current());
        if(strstr(item->Text(),tr("Directory management")))
          cmd = cmdDirManage;
	if(strstr(item->Text(),tr("Burn image")))
	  cmd = cmdImgBurn;
        if(strstr(item->Text(),tr("Open DVD-tray")))
          cmd = cmdDVDopen;
        if(strstr(item->Text(),tr("Close DVD-tray")))
          cmd = cmdDVDclose;
/*
        if(strstr(item->Text(),tr("Play")))
          cmd = cmdImgPlay;
*/
        if(strstr(item->Text(),tr("Rename image")))
          cmd = cmdImgRename;

        if(strstr(item->Text(),tr("Delete cache")))
          cmd = cmdDeleteCache;

        if(strstr(item->Text(),tr("Format DVD-RW")))
          cmd = cmdDVDRWFormat;
/*
        if(strstr(item->Text(),tr("Image move")))
          cmd = cmdImgMove;
        if(strstr(item->Text(),tr("Image delete")))
          cmd = cmdImgDelete;
        if(strstr(item->Text(),tr("Image burn")))
          cmd = cmdImgBurn;
        if(strstr(item->Text(),tr("DVD reading")))
          cmd = cmdImgRead;
*/
        switch(cmd)
        {
          case cmdDirManage:
            return AddSubMenu(new cCMDDir(OsdObject));
            break;
	   case cmdImgBurn:
	     CMDImage = new cCMDImage(OsdObject);
	     ret = CMDImage->Burn(iItem->FileName());
	     DELETENULL(CMDImage);
	     return ret;
	     break;
          case cmdDVDopen:
            return cCMD::Eject(false);
            break;
          case cmdDVDclose:
            return cCMD::Eject(true);
            break;
          case cmdImgPlay:
            return cCMD::Play(iItem);
            break;
          case cmdImgRename:
            OsdObject->SetState(mmsImgRename);
            item = OsdObject->Get(OsdObject->Current());
            item->ProcessKey(kRight);
            return osBack;
            break;
          case cmdImgMove:
            return AddSubMenu(new cCMDMove(iItem->FileName(), OsdObject, false));
            break;
          case cmdImgDelete:
            CMDImage = new cCMDImage(OsdObject);
            ret = CMDImage->Delete(iItem->FileName());
            DELETENULL(CMDImage);
            return ret;
            break;
          case cmdDeleteCache:
            scmd = "/usr/bin/sudo rm -f /etc/vdr/DVD.db";
            system(scmd.c_str());
            Skins.Message(mtInfo,tr("Success..."));
            break;
          case cmdImgRead:
            /* MARKUS */
            //DvdRead();
            //return osBack;
            return  AddSubMenu(new cCMDImageRead);
            break;
          case cmdDVDRWFormat:
            SystemExec("/usr/sbin/dvd-rw-format.sh");
            ////Skins.Message(mtInfo, tr("Formating started"));
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }

  return cOsdMenu::ProcessKey(Key);
}
// MARK DvdRead
/*

void cCMDMenu::DvdRead()
{
  MYDEBUG("lese DVD");

  cFileInfo *info = new cFileInfo(DVDSwitchSetup.DVDReadScript);

  if(!info->isExists())
  {
    MYDEBUG("Readscript existiert nicht");
    DELETENULL(info);
    OSD_ERRMSG(tr("Specified Readscript not exist!"));
    cRemote::Put(kBack);
  }
  else if(!info->isExecutable())
  {
    MYDEBUG("Readscript l�sst sich nicht ausf�hren");
    DELETENULL(info);
    OSD_ERRMSG(tr("Cannot execute Readscript!"));
    cRemote::Put(kBack);
  }
  else // script exists && is executable
  {
    DELETENULL(info);
    //strcpy(File, "\0"); //
    strcpy(Dir, "\0");  //
    //strcpy(ImgTypeTxt, "\0");
    ImgType = tNone;

    if(Interface->Confirm(tr("Now Read?")))
    {
          char *buffer = NULL;
          if(isempty(Dir))
          {
            buffer = strdup(DVDSwitchSetup.ImageDir);
            MYDEBUG(" buffer 1 -> %s ",buffer);
          }
          else
          {
            if(DVDSwitchSetup.ImageDir[strlen(DVDSwitchSetup.ImageDir)-1] == '/')
              asprintf(&buffer, "%s%s", DVDSwitchSetup.ImageDir, Dir);
            else
              asprintf(&buffer, "%s/%s", DVDSwitchSetup.ImageDir, Dir);

            MYDEBUG(" buffer 2  -> %s ",buffer);
          }
          if(buffer[strlen(buffer)-1] == '/')
            buffer[strlen(buffer)-1] = '\0';
          MYDEBUG("ReadThread wird gestartet");
          cCMDImageReadThread *read = new cCMDImageReadThread(Dir, buffer, ImgType);
          FREENULL(buffer);
          read->Start();

    }
  }
}


*/ //MARK  END

// --- cCMDDir ------------------------------------------------------------

cCMDDir::cCMDDir(cMainMenu *osdobject, bool select, char *buffer)
:cOsdMenu(tr("Directory Management"), 2, 2)
{

  MYDEBUG("Verzeichnis Management Args: cMainMenu %s, select  %s,  buffer %s ", osdobject?"valid":"null", select?"yes":"no", buffer );

  ImageDir(DVDSwitchSetup.ImageDir);
  CurrentDir(DVDSwitchSetup.ImageDir); // XXXX
  ParentDir(DVDSwitchSetup.ImageDir);

  State = csNone;
  OsdObject = osdobject;
  Select = select;
  Buffer = buffer;
  SetDir();

  if(Select)
    SetTitle(tr("Select Directory"));

  Build(DVDSwitchSetup.ImageDir);
  cMainMenuItem *mItem = (cMainMenuItem*)Get(Current());
  if(mItem)
    LastSelDir(mItem->FileName());
}

void cCMDDir::SetHelp(void)
{
  cMainMenuItem *item = NULL;
  switch(State)
  {
    case csNone:
      item = (cMainMenuItem*)Get(Current());
      if(Count() && item && item->Type() == iDir)
        if(Select)
          cOsdMenu::SetHelp(NULL, tr("New"), NULL , tr("Select"));
        else
          cOsdMenu::SetHelp(tr("Rename"), tr("New"), tr("Delete"), tr("Move"));
      else
        cOsdMenu::SetHelp(NULL, tr("New"), NULL , NULL);
      break;
    default:
      cOsdMenu::SetHelp(NULL, NULL, NULL , NULL);
      break;
  }
}

void cCMDDir::Build(char *dir)
{
  MYDEBUG("Erstelle Verzeichnisliste %s", dir);
  if(!dir)
    dir = CurrentDir();

  Clear();
  if(State == csNone)
  {
    SetCols(0);
    SetTitle(tr("Edit Directories"));
  }

  cDirHandling *DirHand = new cDirHandling(this, this);
  SetCurrent(Get(DirHand->Build(dir, true)));
  delete(DirHand);

  Display();
  SetHelp();
}

eOSState cCMDDir::ProcessKey(eKeys Key)
{
  cMainMenuItem *mItem = NULL;
  cDirHandling *DirHand = NULL;
  cFileInfo *info = NULL;

  if(!HasSubMenu())
  {
    switch(State)
    {
      case csNone:
        mItem = (cMainMenuItem*)Get(Current());
        switch(Key)
        {
          case kUp:
          case kUp|k_Repeat:
          case kDown:
          case kDown|k_Repeat:
            cOsdMenu::ProcessKey(Key);
            SetHelp();
            mItem = (cMainMenuItem*)Get(Current());
            if(mItem)
              LastSelDir(mItem->FileName());
            return osContinue;
            break;
          case kOk:
            {
               if (!mItem)
                return osBack;
            info = new cFileInfo(mItem->FileName());
            if(!info->isExecutable() || !info->isReadable())
            {
              MYDEBUG("Verzeichniswechsel: Keine Rechte");
              DELETENULL(info);
              OSD_ERRMSG(tr("no Rights!"));
              return osContinue;
              break;
            }
            DELETENULL(info);
            DirHand = new cDirHandling(this, this);
            DirHand->ProcessKey(mItem);
            delete(DirHand);
            Build();
            }
            break;
          case kRed:
            if(mItem->Type() == iDir)
              return Edit(mItem);
            break;
          case kGreen:
            return New();
            break;
          case kYellow:
            if(mItem->Type() == iDir && Interface->Confirm(tr("really delete?")))
            {
              MYDEBUG("Verzeichnis l�schen");
              if(cFileCMD::DirIsEmpty(mItem->FileName()) ||
                 (!cFileCMD::DirIsEmpty(mItem->FileName()) && Interface->Confirm(tr("Directory not empty. however delete?"))))
              {
                cFileDelThread *del = new cFileDelThread(mItem->FileName());
                if(del && del->OK())
                {
                  MYDEBUG("Verzeichnis l�schen gestartet");
                  del->Start();
                  Build();
                  OsdObject->SetState(mmsReInit);
                }
                else
                  DELETENULL(del);
              }
              return osContinue;
            }
            break;
          case kBlue:
            if(mItem->Type() == iDir)
            {
              if(Select)
              {
                MYDEBUG("Verzeichnis gew�hlt: %s", mItem->FileName());
                info = new cFileInfo(mItem->FileName());
                if(!info->isExecutable() || !info->isReadable())
                {
                  MYDEBUG("Verzeichnis: Keine Rechte");
                  OSD_ERRMSG(tr("no rights!"));
                }
                else
                {
                  char *seldir = mItem->FileName();
                  for (unsigned int i = 0; i < strlen(DVDSwitchSetup.ImageDir); i++)
                    seldir++;
                  if(seldir[0] == '/')
                    seldir++;
                  strn0cpy((char*)Buffer, seldir, MaxFileName);
                  cRemote::Put(kBack);
                }
                DELETENULL(info);
                return osContinue;
              }
              else
                return AddSubMenu(new cCMDMove(mItem->FileName(), OsdObject));
            }
            break;
          default:
            break;
        }
        break;
      case csDirNew:
        return New(Key);
        break;
      case csDirEdit:
        return Edit(Key);
        break;
      default:
        break;
    }
  }

  eOSState ret = cOsdMenu::ProcessKey(Key);

  if(!HasSubMenu() && Key == kBlue && ret == osContinue)
  {
    if((cMainMenuItem*)Get(Current() + 1))
      mItem = (cMainMenuItem*)Get(Current() + 1);
    else
      mItem = (cMainMenuItem*)Get(Current() - 1);
    if(mItem)
      LastSelDir(mItem->FileName());
    Build();
  }

  return ret;
}

eOSState cCMDDir::New(void)
{
  MYDEBUG("Verzeichnis: Neu");
  cFileInfo *info = new cFileInfo(CurrentDir());
  if(!info->isWriteable())
  {
    MYDEBUG("Verzeichnis: Neu: Keine Berechtigung in %s", CurrentDir());
    OSD_ERRMSG(tr("no rights to create"));
  }
  else
  {
    State = csDirNew;
    SetCols(5);
    SetTitle(CurrentDir());
    SetDir();

    cMainMenuItem *mItem = (cMainMenuItem*)First();
    Ins(new cMenuEditStrItem(tr("New"), Dir, MaxFileName, trVDR(FileNameChars)),
        true,
        mItem);
    while(mItem)
    {
      mItem->SetSelectable(false);
      mItem = (cMainMenuItem*)Next(mItem);
    }
    Display();
    cOsdMenu::ProcessKey(kRight);
  }

  return osContinue;
}

eOSState cCMDDir::New(eKeys Key)
{
  switch(Key)
  {
    case kOk:
      if(!isempty(Dir))
      {
        char *buffer = NULL;
        asprintf(&buffer, "%s/%s", CurrentDir(), stripspace(Dir));
        MYDEBUG("Verzeichnis: Neu: Anlegen: %s", buffer);
        cFileInfo *info = new cFileInfo(buffer);
        if(info->isExists())
        {
          MYDEBUG("Verzeichnis existiert bereits");
          OSD_WARNMSG(tr("Directory exists"));
          FREENULL(buffer);
          DELETENULL(info);
          return osContinue;
        }
        if(cFileCMD::Mkdir(buffer))
        {
          MYDEBUG("Verzeichnis anlegen erfolgreich");
          LastSelDir(buffer);
          if(!Select)
            OsdObject->SetState(mmsReInit);
        }
        FREENULL(buffer);
        DELETENULL(info);
      }
    case kBack:
      State = csNone;
      Build();
      return osContinue;
      break;
    default:
      break;
  }

  return cOsdMenu::ProcessKey(Key);
}

eOSState cCMDDir::Edit(cMainMenuItem *mItem)
{
  MYDEBUG("Verzeichnis: Edit: %s", mItem->FileName());
  cFileInfo *info = new cFileInfo(mItem->FileName());
  if(!info->isWriteable())
  {
    DELETENULL(info);
    info = new cFileInfo(CurrentDir());
    if(!info->isWriteable())
    {
      MYDEBUG("Verzeichnis: Edit: Keine Rechte");
      OSD_ERRMSG(tr("no rights to rename"));
      DELETENULL(info);
      return osContinue;
    }
  }
  DELETENULL(info);
  info = new cFileInfo(mItem->FileName());
  SetDir(info->FileName());
  DELETENULL(info);
  State = csDirEdit;
  SetCols(11);
  SetTitle(CurrentDir());

  cMainMenuItem *dmItem = NULL;
  mItem = (cMainMenuItem*)First();
  while(mItem)
  {
    if(!strcasecmp(mItem->FileName(), LastSelDir()))
    {
      MYDEBUG("Verzeichnis: Edit: Item gefunden: %s", mItem->FileName());
      Ins(new cMenuEditStrItem(tr("Rename"), Dir, MaxFileName, trVDR(FileNameChars)),
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
  cOsdMenu::ProcessKey(kRight);

  return osContinue;
}

eOSState cCMDDir::Edit(eKeys Key)
{
  switch(Key)
  {
    case kOk:
      if(!isempty(Dir))
      {
        char *buffer = NULL;
        asprintf(&buffer, "%s/%s", CurrentDir(), stripspace(Dir));
        MYDEBUG("Verzeichnis: Edit: OK: %s", buffer);
        cFileInfo *info = new cFileInfo(buffer);
        if(info->isExists())
        {
          MYDEBUG("Verzeichnis: Edit: Existiert schon");
          OSD_WARNMSG(tr("Directory exists"));
          FREENULL(buffer);
          DELETENULL(info);
          return osUnknown;
        }
        if(cFileCMD::Rn(LastSelDir(), buffer))
        {
          MYDEBUG("Verzeichnis: Edit: Rename OK");
          LastSelDir(buffer);
          OsdObject->SetState(mmsReInit);
        }
        FREENULL(buffer);
        DELETENULL(info);
      }
    case kBack:
      State = csNone;
      Build();
      return osContinue;
      break;
    default:
      break;
  }

  return cOsdMenu::ProcessKey(Key);
}

// --- cCMDMove -------------------------------------------------------------

cCMDMove::cCMDMove(char *file, cMainMenu *osdobject, bool dir, bool direct)
  : cOsdMenu(tr("Move"))
{
  MYDEBUG("Verzeichnis: Move: %s");
  File = file ? strdup(file) : NULL;
  OsdObject = osdobject;
  Dir = dir;
  Direct = direct;

  ImageDir(DVDSwitchSetup.ImageDir);
  CurrentDir(DVDSwitchSetup.ImageDir);
  ParentDir(DVDSwitchSetup.ImageDir);


  Build(DVDSwitchSetup.ImageDir);
}

void cCMDMove::SetHelp(void)
{
  cOsdMenu::SetHelp(NULL, NULL, NULL , tr("Insert"));
}

void cCMDMove::Build(char *dir)
{
  MYDEBUG("Verzeichnis: Move: Erstelle Verzeichnisliste: %s", dir);
  if(!dir)
    dir = CurrentDir();

  Clear();

  cDirHandling *DirHand = new cDirHandling(this, this);
  SetCurrent(Get(DirHand->Build(dir, true)));
  delete(DirHand);

  if(Count())
  {
    cMainMenuItem *item = (cMainMenuItem*)First();
    while(item)
    {
      if(!strcasecmp(item->FileName(), File))
      {
        Del(item->Index());
        break;
      }
      item = (cMainMenuItem*)Next(item);
    }
  }

  Display();
  SetHelp();
}

eOSState cCMDMove::ProcessKey(eKeys Key)
{
  cMainMenuItem *mItem = (cMainMenuItem*)Get(Current());
  cDirHandling *DirHand = NULL;
  cFileMoveThread *move = NULL;

  switch(Key)
  {
    case kUp:
    case kUp|k_Repeat:
    case kDown:
    case kDown|k_Repeat:
      cOsdMenu::ProcessKey(Key);
      SetHelp();
      mItem = (cMainMenuItem*)Get(Current());
      if(mItem)
        LastSelDir(mItem->FileName());
      return osContinue;
      break;
    case kOk:
      DirHand = new cDirHandling(this, this);
      DirHand->ProcessKey(mItem);
      DELETENULL(DirHand);
      Build();
      break;
    case kBlue:
      MYDEBUG("Verzeichnis: Move: Verschiede in: %s", CurrentDir());
      move = new cFileMoveThread(File, CurrentDir());
      if(move->OK())
      {
        MYDEBUG("Verzeichnis: Move: Erfolgreich");
        move->Start();
        cCondWait::SleepMs(1 * 500);
        OsdObject->SetState(mmsReInit);
      }
      else
        DELETENULL(move);
      if(!Dir && !Direct)
        cRemote::Put(kBack);
      return osBack;
      break;
    default:
      break;
  }

  return cOsdMenu::ProcessKey(Key);
}

// --- cCMDImage ------------------------------------------------------------

cCMDImage::cCMDImage(cMainMenu *osdobject)
{
  MYDEBUG("CMDImage");
  File = NULL;
  strcpy(NewFile, "\0");
  OsdObject = osdobject;
}

cCMDImage::~cCMDImage(void)
{
  if(File)
    free(File);
}

char* cCMDImage::Rename(char *file)
{
  MYDEBUG("CMDImage Rename");
  if(file)
  {
    FREENULL(File);
    File = strdup(file);
  }

  return File;
}

eOSState cCMDImage::Delete(char *file)
{
  MYDEBUG("l�sche DVD '%s'", file);
  if(file)
  {
    if(Interface->Confirm(tr("really delete?")))
    {
      cFileDelThread *del = new cFileDelThread(file);
      if(del->OK())
      {
        MYDEBUG("l�schen OK");
        del->Start();
        OsdObject->SetState(mmsReInitCur);
      }
      else
        DELETENULL(del);
    }
  }

  return osBack;
}

eOSState cCMDImage::Burn(char *file)
{
  MYDEBUG("DVD Schreiben '%s'", file);

  cFileInfo *info = new cFileInfo(DVDSwitchSetup.DVDWriteScript);

  if(!info->isExists())
  {
    MYDEBUG("Writescript existiert nicht");
    DELETENULL(info);
    OSD_ERRMSG(tr("Specified Writescript not exist!"));
    return osContinue;
  }
  if(!info->isExecutable())
  {
    MYDEBUG("Writescript l�sst sich nicht ausf�hren");
    DELETENULL(info);
    OSD_ERRMSG(tr("Cannot execute Writescript!"));
    return osContinue;
  }

  DELETENULL(info);

  info = new cFileInfo(file);

  if(Interface->Confirm(tr("Burn Now?")))
  {
    MYDEBUG("Starte Burn-Thread");
    cCMDImageBurnThread *burn = new cCMDImageBurnThread(file, info->Type());
    burn->Start();
    cRemote::CallPlugin("bgprocess");
  }

  return osContinue;
}

// --- cCmdDVDRead ------------------------------------------------------------

cCMDImageRead::cCMDImageRead(void)
:cOsdMenu(tr("Read DVD"), 14)
{
  MYDEBUG("lese DVD");

//  Add (new cOsdItem(tr("Press Ok to read DVD"), osUnknown, false));
// Add (new cOsdItem(tr("Osd ends when while copying"), osUnknown, false));
  cFileInfo *info = new cFileInfo(DVDSwitchSetup.DVDReadScript);

  if(!info->isExists())
  {
    MYDEBUG("Readscript existiert nicht");
    DELETENULL(info);
    OSD_ERRMSG(tr("Specified Readscript not exist!"));
    cRemote::Put(kBack);
  }
  else if(!info->isExecutable())
  {
    MYDEBUG("Readscript l�sst sich nicht ausf�hren");
    DELETENULL(info);
    OSD_ERRMSG(tr("Cannot execute Readscript!"));
    cRemote::Put(kBack);
  }
  else // script exists && is executable
  {
    DELETENULL(info);
    //strcpy(File, "\0"); //
    strcpy(Dir, "\0");  //
    //strcpy(ImgTypeTxt, "\0");
    ImgType = tNone;

    if(Interface->Confirm(tr("Now Read?")))
    {
          char *buffer = NULL;
          if(isempty(Dir))
          {
            buffer = strdup(DVDSwitchSetup.ImageDir);
            MYDEBUG(" buffer 1 -> %s ",buffer);
          }
          else
          {
            if(DVDSwitchSetup.ImageDir[strlen(DVDSwitchSetup.ImageDir)-1] == '/')
              asprintf(&buffer, "%s%s", DVDSwitchSetup.ImageDir, Dir);
            else
              asprintf(&buffer, "%s/%s", DVDSwitchSetup.ImageDir, Dir);

            MYDEBUG(" buffer 2  -> %s ",buffer);
          }
          if(buffer[strlen(buffer)-1] == '/')
            buffer[strlen(buffer)-1] = '\0';
          MYDEBUG("ReadThread wird gestartet");
          cCMDImageReadThread *read = new cCMDImageReadThread(Dir, buffer, ImgType);
          FREENULL(buffer);
          read->Start();
          //XXX
          cRemote::Put(kMenu);
	  cRemote::CallPlugin("bgprocess");

    }
  }
}


cCMDImageRead::~cCMDImageRead(void)
{
  MYDEBUG("Menu \"Lese DVD\" beendet");
}

