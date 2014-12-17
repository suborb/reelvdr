/*
 * See the README file for copyright information and how to reach the author.
 *
 * This code is directly taken from VDR with some changes by me to work with this plugin
 */

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/statvfs.h>

#include <vdr/config.h>
//#include <vdr/debug.h>
#include <vdr/interface.h>
#include <vdr/menu.h>
#include <vdr/plugin.h>
#include "mymenucommands.h"
#include "SelectFile.h"
#include "MenuSearchRecordings.h"

myMoveCommand myMoveCommand::instance;
myDeleteCommand myDeleteCommand::instance;
mySetTitleCommand mySetTitleCommand::instance;
myRenameCommand myRenameCommand::instance;
myBurnCommand myBurnCommand::instance;
myMarkNewCommand myMarkNewCommand::instance;
//myDetailsCommand myDetailsCommand::instance;

std::vector<bgcommand> bgcommandlist;

#if VDRVERSNUM >= 10713
myMenuCommands::myMenuCommands(const char *Title, cList<cNestedItem> *Commands, cRecording *Recording, std::vector<myCommands *> mycommands, std::string &cmdMsg): commandMsg(cmdMsg), cOsdMenu(Title)
#else
myMenuCommands::myMenuCommands(const char *Title,cCommands *Commands,cRecording *Recording, 
                std::vector<myCommands *> mycommands, std::string &cmdMsg):commandMsg(cmdMsg), cOsdMenu(Title)
#endif
{
 SetHasHotkeys();
 commands=Commands;
 recording = Recording;
 mycommands_ = mycommands;


#if VDRVERSNUM >= 10713
 parameters = cString::sprintf("\"%s\"", Recording->FileName());
 //printf("parameters:'%s' '%s'\n", *parameters, Recording->FileName());
 result = NULL;
#else
 asprintf(&parameters,"\"%s\"",Recording->FileName());
#endif
 AddCommands();
 Display();
}

#if VDRVERSNUM >= 10713
myMenuCommands::myMenuCommands(const char *Title, cList<cNestedItem> *Commands, std::string path_, std::vector<myCommands *> mycommands, std::string &cmdMsg): commandMsg(cmdMsg),cOsdMenu(Title)
#else
myMenuCommands::myMenuCommands(const char *Title,cCommands *Commands, std::string path_, 
                std::vector<myCommands *> mycommands, std::string &cmdMsg):commandMsg(cmdMsg), cOsdMenu(Title)
#endif
{
 SetHasHotkeys();
 commands=Commands;
 path = path_;
 mycommands_ = mycommands;
#if VDRVERSNUM >= 10713
 parameters=cString::sprintf( "\"%s\"", path.c_str());
 //printf("parameters:'%s' '%s'\n", *parameters, path.c_str());
 result = NULL;
#else
 asprintf(&parameters,"\"%s\"",path.c_str());
#endif

 AddCommands();
 Display();
}

myMenuCommands::~myMenuCommands()
{
#if VDRVERSNUM >= 10713
 if(result) free(result);
#else
 free(parameters);
#endif
}

#if VDRVERSNUM >= 10713
bool myMenuCommands::Parse(const char *s)
{
 const char *p=strchr(s,':');
 if(p) {
  int l=p-s;
  if(l>0) {
   char t[l+1];
   stripspace(strn0cpy(t,s,l+1));
   l=strlen(t);
   if(l>1&&t[l-1]=='?') {
    t[l-1]=0;
    confirm=true;
   }
   else
    confirm=false;
   title=t;
   command=skipspace(p+1);
   return true;
  }
 }
 return false;
}
#endif

bool hasHD() {
    int cntrHDs = 0;
    struct stat fstat_media;
    if(stat("/media", &fstat_media) == 0) {

        int i;
        struct statvfs fsstat;
        struct stat fstat;
        char path[12] = "/media/hd/";
        if (stat(path, &fstat) == 0) { // path exists
            if (fstat.st_dev != fstat_media.st_dev && statvfs(path, &fsstat) == 0) // not on the same FS
                cntrHDs++;
        }
        for (i = 0; i < 10; i++) {
            path[9] = i+48;
            path[10] = '/';
            path[11] = '\0';
            if (stat(path, &fstat) == 0) { // path exists
                if (fstat.st_dev != fstat_media.st_dev && statvfs(path, &fsstat) == 0) // not on the same FS
                    cntrHDs++;
            }
        }
    }
    return (cntrHDs != 0);
}

/** TB: filter the commands - only add those to the menu that make sense */
#if VDRVERSNUM >= 10713
void myMenuCommands::AddCommands()
{
  std::string buffer;
  bool isProtected = false;
  if(path.size())
    buffer = path;
  else
    buffer = recording->FileName();
  buffer += "/protection.fsk";
  isProtected = !access(buffer.c_str(),R_OK);

#if REELVDR && APIVERSNUM >= 10718
  if(path.size())
    buffer = path;
  else
    buffer = recording->FileName();
  buffer += "/preview_vdr.png";
  bool hasPreviewImage = !access(buffer.c_str(),R_OK);
#endif /* REELVDR && APIVERSNUM >= 10718 */

  for(uint i = 0; i < mycommands_.size(); ++i)
  {
      Add(new cOsdItem(hk(tr(mycommands_[i]->Title().c_str()))));
  }

  for(cNestedItem *command=commands->First();command;command=commands->Next(command)) {
      const char* s = command->Text();
     // printf("commands->Text(): '%s' \n", s);
      if (command->SubItems())
          Add(new cOsdItem(hk(cString::sprintf("%s...", s))));
      else if(Parse(s))
      {
          //       Add(new cOsdItem(hk(trVDR(title))));

          if(strcmp(title, "Mark Commercials") == 0) {
              if (!recording->IsHD() || access("/usr/sbin/markad", X_OK) == 0 )
                  Add(new cOsdItem(hk(trVDR(title))));
          } else if(strcmp(title, "Add Child Lock") == 0) {
              if(!isProtected)
                  Add(new cOsdItem(hk(trVDR(title))));
          } else if(strcmp(title, "Remove Child Lock") == 0) {
              if(isProtected)
                  Add(new cOsdItem(hk(trVDR(title))));
          } else if(strcmp(title, "Mark as New") == 0) {
              if(!recording->IsNew())
                  Add(new cOsdItem(hk(trVDR(title))));
          } else if(strcmp(title, "Compress recording") == 0) {
              struct stat st;
              if(!recording->IsCompressed() && cPluginManager::GetPlugin("reelice")/*(0==stat(cString::sprintf("/usr/sbin/%s", (const char *)this->command), &st))*/) {
                  if(stat(cString::sprintf("%s/info.txt", recording->FileName()), &st)==0)
                      Add(new cOsdItem(hk(trVDR(title))));
                  else isyslog("Rejected old recording \"%s\" for compression", recording->FileName());
              }
          } else if(strcmp(title, "Remove Archive Mark") == 0) {
              bool isArchived = false;
              buffer = recording->FileName();
              buffer += "/dvd.vdr";
              isArchived = !access(buffer.c_str(),R_OK);
              if(isArchived)
                  Add(new cOsdItem(hk(trVDR(title))));
          } else if(strcmp(title, "Remove Crop Mark") == 0) {
              cMarks marks;
#if APIVERSNUM >= 10703
              marks.Load(recording->FileName(), recording->FramesPerSecond(),
                         recording->IsPesRecording());
#else
              marks.Load(recording->FileName());
#endif

              if(marks.Count())
                  Add(new cOsdItem(hk(trVDR(title))));
          } else if(strcmp(title, "Remove Archived Recording") == 0) {
              int isArchivedAndOnDisc = 0;
              buffer = recording->FileName();
              buffer += "/dvd.vdr";
              isArchivedAndOnDisc = !access(buffer.c_str(),R_OK);
              buffer = recording->FileName();
              buffer += "/001.vdr";
              isArchivedAndOnDisc += !access(buffer.c_str(),R_OK);
              if(isArchivedAndOnDisc == 2)
                  Add(new cOsdItem(hk(trVDR(title))));
          } else if(strcmp(title, "Restore Archived Recording") == 0) {
              if(SystemExec("mount.sh status") == 0)
                  Add(new cOsdItem(hk(trVDR(title))));
          } else if(strcmp(title, "Convert to MPEG (TS)") == 0) {
              // obsolete if(!recording->IsHD() && recording->IsPesRecording())
                  Add(new cOsdItem(hk(trVDR(title))));
          } else if (strcmp(title, "Generate preview") == 0) {
#if REELVDR && APIVERSNUM >= 10718
              if (!hasPreviewImage)
                  Add(new cOsdItem(hk(trVDR(title))));
#endif /* REELVDR && APIVERSNUM >= 10718 */
          } else if (strcmp(title, "Remove preview") == 0) {
#if REELVDR && APIVERSNUM >= 10718
              if (hasPreviewImage)
                  Add(new cOsdItem(hk(trVDR(title))));
#endif /* REELVDR && APIVERSNUM >= 10718 */
          } else
              Add(new cOsdItem(hk(trVDR(title))));
      }// Parse(s)
      else esyslog("Parse unsuccessful on '%s'", s);

}

    //if (hasHD() && ((path.size() && !InSameFileSystem(path.c_str(), "/media/reel/recordings")) || (recording && !InSameFileSystem(recording->FileName(), "/media/reel/recordings"))))
    if (hasHD() && (path.size() && !InSameFileSystem(path.c_str(), "/media/reel/recordings")) )
        Add(new cOsdItem(hk(trVDR("Copy to local harddisc"))));

    SetHelp(NULL, NULL, NULL, tr("Search"));
}
#else
void myMenuCommands::AddCommands()
{
  std::string buffer;
  bool isProtected = false;
  if(path.size())
    buffer = path;
  else
    buffer = recording->FileName();
  buffer += "/protection.fsk";
  isProtected = !access(buffer.c_str(),R_OK);
  for(uint i = 0; i < mycommands_.size(); ++i)
  {
      Add(new cOsdItem(hk(tr(mycommands_[i]->Title().c_str()))));
  }

  for(cCommand *command=commands->First();command;command=commands->Next(command)) {
     if(strcmp(command->Title(), "Mark Commercials") == 0) {
       if(!recording->IsHD() && !recording->IsTS())
          Add(new cOsdItem(hk(trVDR(command->Title()))));
     } else if(strcmp(command->Title(), "Add Child Lock") == 0) {
       if(!isProtected)
          Add(new cOsdItem(hk(trVDR(command->Title()))));
     } else if(strcmp(command->Title(), "Remove Child Lock") == 0) {
       if(isProtected)
          Add(new cOsdItem(hk(trVDR(command->Title()))));
     } else if(strcmp(command->Title(), "Mark as New") == 0) {
       if(!recording->IsNew())
          Add(new cOsdItem(hk(trVDR(command->Title()))));
     } else if(strcmp(command->Title(), "Remove Archive Mark") == 0) {
       bool isArchived = false;
       buffer = recording->FileName();
       buffer += "/dvd.vdr";
       isArchived = !access(buffer.c_str(),R_OK);
       if(isArchived)
         Add(new cOsdItem(hk(trVDR(command->Title()))));
     } else if(strcmp(command->Title(), "Remove Crop Mark") == 0) {
       cMarks marks;
       marks.Load(recording->FileName());
       if(marks.Count())
          Add(new cOsdItem(hk(trVDR(command->Title()))));
     } else if(strcmp(command->Title(), "Remove Archived Recording") == 0) {
       int isArchivedAndOnDisc = 0;
       buffer = recording->FileName();
       buffer += "/dvd.vdr";
       isArchivedAndOnDisc = !access(buffer.c_str(),R_OK);
       buffer = recording->FileName();
       buffer += "/001.vdr";
       isArchivedAndOnDisc += !access(buffer.c_str(),R_OK);
       if(isArchivedAndOnDisc == 2)
         Add(new cOsdItem(hk(trVDR(command->Title()))));
     } else if(strcmp(command->Title(), "Restore Archived Recording") == 0) {
       if(SystemExec("mount.sh status") == 0)
          Add(new cOsdItem(hk(trVDR(command->Title()))));
     } else if(strcmp(command->Title(), "Convert to MPEG (PS)") == 0) {
       if(!recording->IsHD() && !recording->IsTS())
          Add(new cOsdItem(hk(trVDR(command->Title()))));
     } else
       Add(new cOsdItem(hk(trVDR(command->Title()))));
 }

    //if (hasHD() && ((path.size() && !InSameFileSystem(path.c_str(), "/media/reel/recordings")) || (recording && !InSameFileSystem(recording->FileName(), "/media/reel/recordings"))))
    if (hasHD())
        Add(new cOsdItem(hk(trVDR("Copy to local harddisc"))));
}
#endif

#if VDRVERSNUM >= 10713

eOSState myMenuCommands::Execute()
{
    /* Get the selected command */

    //cNestedItem *Command=commands->Get(Current());

    int i = 0;
    cNestedItem *Command;
    const char * s = NULL;
    for(Command=commands->First();Command;Command=commands->Next(Command)) {
        s = Command->Text();
        Parse(s);
      //  printf("'%s' -- '%s' '%s'\n", s, *title, *command);
        // compare with osd text on screen
         if(strcmp(trVDR(title), Get(Current())->Text()+5) == 0)
            break;

         i++;
    }
    //printf("Execute() : searching for '%s' : i=%d\n", Get(Current())->Text()+5, i );

    if (strcmp(trVDR("Compress recording"), Get(Current())->Text()+5) == 0) {
       if (parameters[0] == '"' && parameters[strlen(parameters)-1] == '"') { // strip off those quotation-marks
           std::string param = &parameters[1];
           param[param.length()-1] = 0;
           return AddSubMenu(new cCompressRecordingMenu(*command, param));
       } else
        return AddSubMenu(new cCompressRecordingMenu(*command, std::string(*parameters)));
    } else if(Command && (strcmp(trVDR(title), Get(Current())->Text()+5) == 0)) {
        //printf("Execute() : got '%s'\n", Command->Text());
#ifdef CMDSUBMENUVERSNUM
        if(Command->SubItems())
            return AddSubMenu(new myMenuCommands(Title(),Command->SubItems(),parameters));
#endif

        if(Parse(Command->Text())) {
            //printf("Parsed '%s' : '%s' '%s'\n", Command->Text(), *title, *command);
            //printf("Confirm? %d\n Params: '%s'\n", confirm, *parameters);

            if(!confirm || Interface->Confirm(cString::sprintf("%s?",trVDR(title))) ) {
                Skins.Message(mtStatus, cString::sprintf("%s...",trVDR(title)));

                //printf("in confirm before result %x\n", result);
                if(result)
                    free(result);
                result=NULL;

               // printf("in confirm before isempty\n");

                cString cmdbuf;
                if(!isempty(parameters))
                    cmdbuf=cString::sprintf("%s %s", *command, *parameters);
		
		
		/*if(std::string(command) == "noadcall.sh")
		{
		  bgcommand cmd = {*command, *parameters};
		  bgcommandlist.push_back(cmd);
		  printf("bl++++++++++a############################################\n");
		}  */

              //  printf("cmdbuf '%s'\n", *cmdbuf);
                const char *cmd= *cmdbuf? *cmdbuf : *command;
                dsyslog("executing command '%s'",cmd);

                cPipe p;
                if(p.Open(cmd,"r")) {
                    int l=0;
                    int c;

                    while((c=fgetc(p))!=EOF) {
                        if(l%20==0)
                            result=(char *)realloc(result,l+21);
                        result[l++]=char(c);
                    } // while

                    if(result)
                        result[l]=0;
                    p.Close();
                } // if p.Open()
                else
                    esyslog("ERROR: can't open pipe for command '%s'",cmd);

                Skins.Message(mtStatus,NULL);
                if(result)
		{
                   //return AddSubMenu(new cMenuText(trVDR(title),result,fontFix));
		   commandMsg = result;
		   return osBack;
		}
                return osEnd;
            } // confirm | Interface

        } // Parse(Command->Text())

    } // if (Command)
    else if (strcmp(trVDR("Copy to local harddisc"), Get(Current())->Text()+5) == 0) {
       /* // if (parameters[0] == '"' && parameters[strlen(parameters)-1] == '"') { // strip off those quotation-marks
       //    parameters[strlen(parameters)-1] = '\0';
           return AddSubMenu(new cCopyFileMenu(parameters+1));
        } elsei*/
        return AddSubMenu(new cCopyFileMenu(std::string(*parameters)));
       } // copy to HDD
    else
    {
       for(uint i = 0; i < mycommands_.size(); ++i)
       {
          //printf("------mycommands_[i]->Title().c_str()= %s-------\n", mycommands_[i]->Title().c_str());
          //printf("------ Get(Current())->Text()+5= %s-------\n",  Get(Current())->Text()+5);
          if(strcmp(tr(mycommands_[i]->Title().c_str()), Get(Current())->Text()+5) == 0)
          {
              printf("Executing\n");
             return  mycommands_[i]->Execute();
          }
       }
    }
    return osContinue;

} // Execute()

#else

eOSState myMenuCommands::Execute()
{
 int i = 0;
 cCommand *command;
 for(command=commands->First();command;command=commands->Next(command)) {
      if(strcmp(trVDR(command->Title()), Get(Current())->Text()+5) == 0)
         break;
      i++;
 }

 if(command && (strcmp(trVDR(command->Title()), Get(Current())->Text()+5) == 0))
 {
  char *buffer=NULL;
  bool confirmed=true;
#ifdef CMDSUBMENUVERSNUM
     if (command->hasChilds()) {
        AddSubMenu(new myMenuCommands(trVDR(command->Title()), command->getChilds(), parameters));
        return osContinue;
        }
#endif
  if(command->Confirm()) {
   asprintf(&buffer,"%s?", trVDR(command->Title()));
   confirmed=Interface->Confirm(buffer);
   free(buffer);
  }
  if(confirmed)
  {
   asprintf(&buffer, "%s...", trVDR(command->Title()));
   Skins.Message(mtStatus,buffer);
   free(buffer);
   const char *Result=command->Execute(parameters);
   Skins.Message(mtStatus, NULL);
   if(Result)
    return AddSubMenu(new cMenuText(trVDR(command->Title()),Result,fontFix));
   return osEnd;
  }
 }
 else if (strcmp(trVDR("Copy to local harddisc"), Get(Current())->Text()+5) == 0) {
     if (parameters[0] == '"' && parameters[strlen(parameters)-1] == '"') { // strip off those quotation-marks
        parameters[strlen(parameters)-1] = '\0';
        return AddSubMenu(new cCopyFileMenu(parameters+1));
     } else
        return AddSubMenu(new cCopyFileMenu(parameters));
 }
 else
 {
    for(uint i = 0; i < mycommands_.size(); ++i)
    {
       //printf("------mycommands_[i]->Title().c_str()= %s-------\n", mycommands_[i]->Title().c_str());
       //printf("------ Get(Current())->Text()+5= %s-------\n",  Get(Current())->Text()+5);
       if(strcmp(tr(mycommands_[i]->Title().c_str()), Get(Current())->Text()+5) == 0)
       {
          return  mycommands_[i]->Execute();
       }
    }
 }
 return osContinue;
}
#endif

eOSState myMenuCommands::ProcessKey(eKeys Key)
{
 eOSState state=cOsdMenu::ProcessKey(Key);

 if(state==osUnknown)
 {
  switch(Key)
  {
   case kRed:
   case kGreen:
   case kYellow: return osContinue;
   case kBlue: return AddSubMenu(new cMenuSearchRecordings);
   case kOk:   return Execute(); break;
   default:    state = osUnknown;
  }
 }
 return state;
}
