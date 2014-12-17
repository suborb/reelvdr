/*
 * See the README file for copyright information and how to reach the author.
 */

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vdr/plugin.h>
#include <vdr/tools.h> // cTimeMs
#include <vdr/videodir.h>
#include <vdr/recording.h>
#include <vdr/cutter.h>
#include "tools.h"
#include "mymenusetup.h"
#include "mymenurecordings.h"  //Behni
#include "../bgprocess/service.h"

using namespace std;
extern bool VfatFileSytem;

#define CONFIGFILE "/extrecmenu.sort.conf"
#define BUFFERSIZE 32768 // 20972 // (2*1024*1024)/100

SortList *mySortList;
WorkerThread *MoveCutterThread;

string myStrEscape(string S,const char *Chars)
{
  int i=0;
  while(Chars[i]!=0)
  {
    string::size_type j=0;
    while((j=S.find(Chars[i],j))!=string::npos)
    {
      S.insert(j,1,'\\');
      j+=2;
     }
     i++;
  }
  return S;
}

string myStrReplace(string S,char C1,char C2)
{
  string::size_type i=0;
  while((i=S.find(C1,i))!=string::npos)
  {
    S.replace(i,1,1,C2);
    i++;
  }
  return S;
}

// --- SortList ---------------------------------------------------------------
void SortList::ReadConfigFile()
{
  string configfile(cPlugin::ConfigDirectory());
  configfile+=CONFIGFILE;

  ifstream in(configfile.c_str());
  if(in)
  {
    string buf;
    while(!in.eof())
    {
      getline(in,buf);
      if(buf.length()>0)
        Add(new SortListItem(buf));
    }
  }
}

void SortList::WriteConfigFile()
{
 string configfile(cPlugin::ConfigDirectory());
 configfile+=CONFIGFILE;

 ofstream outfile(configfile.c_str());

 for(SortListItem *item=First();item;item=Next(item))
  outfile << item->Path() << endl;
}

bool SortList::Find(string Path)
{
 for(SortListItem *item=First();item;item=Next(item))
 {
  if(item->Path()==Path)
   return true;
 }
 return false;
}

// --- MoveRename -------------------------------------------------------------
// creates the necassery directories and renames the given old name to the new name
bool MoveRename(const char *OldName,const char *NewName,cRecording *Recording,bool Move)
{
  char *buf=NULL;

  if(!strcmp(OldName,NewName))
    return true;

  if(Recording)
  {
    isyslog("[extrecmenu] moving %s to %s",OldName,NewName);

    if(!MakeDirs(NewName,true))
    {
      Skins.Message(mtError,tr("Creating directories failed!"));
      return false;
    }

    if(rename(OldName,NewName)==-1)
    {
      remove(NewName); // remove created directory
      if(rename(OldName,NewName)==-1) // Next try: rename on NetClient fails on cifs if directory NewName exists
      {
        Skins.Message(mtError,tr("Rename/Move failed!"));
        esyslog("[extrecmenu] MoveRename() recording - %s(\"%s\" \"%s\") - %s",Move?"move":"rename", OldName, NewName, strerror(errno));
        return false;
      }
    }

    cThreadLock RecordingsLock(&Recordings);
    Recordings.DelByName(OldName);
    Recordings.AddByName(NewName);

    // set user command for '-r'-option of VDR
    asprintf(&buf,"%s \"%s\"",Move?"move":"rename",*strescape(OldName,"'\\\"$"));
    cRecordingUserCommand::InvokeCommand(buf,NewName);
    free(buf);
  }
  else
  {
    // is the new path within the old?
    asprintf(&buf,"%s/",OldName); // we have to append a / to make sure that we search for a directory
    if(!strncmp(buf,NewName,strlen(buf)))
    {
      Skins.Message(mtError,tr("Moving into own sub-directory not allowed!"));
      free(buf);
      return false;
    }
    free(buf);

    myRecList *list=new myRecList();
    for(cRecording *recording=Recordings.First();recording;recording=Recordings.Next(recording))
      list->Add(new myRecListItem(recording));

    myRecListItem *item=list->First();
    while(item)
    {
      if(!strncmp(OldName,item->recording->FileName(),strlen(OldName)))
      {
        buf=strdup(OldName+strlen(VideoDirectory)+1);
        buf=ExchangeChars(buf,false);

        if(strcmp(item->recording->Name(),buf))
        {
          free(buf);
          asprintf(&buf,"%s%s",NewName,item->recording->FileName()+strlen(OldName));
          if(!MakeDirs(buf,true))
          {
            Skins.Message(mtError,tr("Creating directories failed!"));
            free(buf);
            delete list;
            return false;
          }
          if(MoveRename(item->recording->FileName(),buf,item->recording,Move)==false)
          {
            free(buf);
            delete list;
            return false;
          }
        }
        free(buf);
      }
      item=list->Next(item);
    }
    delete list;
  }
  return true;
}

// --- myRecListItem ----------------------------------------------------------
bool myRecListItem::SortByName=false;

myRecListItem::myRecListItem(cRecording *Recording)
{
  recording=Recording;
  filename=strdup(recording->FileName());
}

myRecListItem::~myRecListItem()
{
  free(filename);
}

#if 0
char *myRecListItem::StripEpisodeName(char *s)
{
  char *t=s,*s1=NULL,*s2=NULL;
  while(*t)
  {
    if(*t=='/')
    {
      if(s1)
      {
        if(s2)
          s1=s2;
        s2=t;
      }
      else
        s1=t;
    }
    t++;
  }
  if(mysetup.DescendSorting)
  {
    if(SortByName)
      *s1=1;
    else
      *(s2+1)=255;
  }
  else
    *s1=255;

  if(s1 && s2 && !SortByName)
    memmove(s1+1,s2,t-s2+1);

  return s;
}

int myRecListItem::Compare(const cListObject &ListObject)const
{
  myRecListItem *item=(myRecListItem*)&ListObject;

  char *s1 = NULL;
  char *s2 = NULL;
  stringstream infobuffer1;
  stringstream infobuffer2;
  if (SortByName) {
      const cRecordingInfo *info = recording->Info();

      bool s1IsDir = (strchr(filename+strlen(VideoDirectory)+1, '/') != strrchr(filename+strlen(VideoDirectory)+1, '/'));
      bool s2IsDir = (strchr(item->filename+strlen(VideoDirectory)+1, '/') != strrchr(item->filename+strlen(VideoDirectory)+1, '/'));

      if (s1IsDir != s2IsDir) {
        if (s1IsDir)
            return -1;
        else
            return 1;
      }

      if (myMenuRecordings::infotitle && s1IsDir && s2IsDir && info && info->Title()) {
        s1 = StripEpisodeName(strdup(filename+strlen(VideoDirectory)));
        *(strchr(s1, 255)+1)='\0';
        infobuffer1 << s1;
        infobuffer1 << strdup(info->Title());
        if (info->ShortText()) {
          infobuffer1 << " - ";
          infobuffer1 << info->ShortText();
        }
        s1=strdup(infobuffer1.str().c_str());
        strreplace(s1,' ','_');
      } else {
        s1 = StripEpisodeName(strdup(filename+strlen(VideoDirectory)));

        /** ignore the '%': doesn't matter if a recording is cut or not... */
        char *ptr = strchr(s1, 255);
        while (*(ptr+1)=='%')
          memmove(ptr+1, ptr+2, strlen(ptr+2));
      }
      if (myMenuRecordings::infotitle && s1IsDir && s2IsDir && item && item->recording && item->recording->Info() && item->recording->Info()->Title()) {
        s2 = StripEpisodeName(strdup(item->filename+strlen(VideoDirectory)));
        *(strchr(s2, 255)+1)='\0';
        infobuffer2 << s2;
        infobuffer2 << strdup(item->recording->Info()->Title());
        if (item->recording->Info()->ShortText()) {
          infobuffer2 << " - ";
          infobuffer2 << item->recording->Info()->ShortText();
        }
        s2=strdup(infobuffer2.str().c_str());
        strreplace(s2,' ','_');
      } else {
        s2 = StripEpisodeName(strdup(item->filename+strlen(VideoDirectory)));

        /** ignore the '%': doesn't matter if a recording is cut or not... */
        char *ptr = strchr(s2, 255);
        while (*(ptr+1)=='%')
          memmove(ptr+1, ptr+2, strlen(ptr+2));
      }

  } else {
      s1 = StripEpisodeName(strdup(filename+strlen(VideoDirectory)));
      s2 = StripEpisodeName(strdup(item->filename+strlen(VideoDirectory)));
  }

  int compare;
  if(mysetup.DescendSorting)
    compare=strcasecmp(s2,s1);
  else
    compare=strcasecmp(s1,s2);

  free(s1);
  free(s2);

  return compare;
}

#else

char *myRecListItem::StripEpisodeName(char *s)
{
  char *t = s, *s1 = NULL, *s2 = NULL;
  while (*t) {
        if (*t == '/') {
           if (s1) {
              if (s2)
                 s1 = s2;
              s2 = t;
              }
           else
              s1 = t;
           }
        t++;
        }
  if(s1) *s1=mysetup.DescendSorting?1:255;
  if (s1 && s2 && !SortByName)
     memmove(s1 + 1, s2, t - s2 + 1);
  return s;
}

int myRecListItem::Compare(const cListObject &ListObject)const
{
  myRecListItem *item=(myRecListItem*)&ListObject;

  char *s1 = NULL;
  char *s2 = NULL;
  std::stringstream infobuffer1;
  std::stringstream infobuffer2;
  if (SortByName) {
      const cRecordingInfo *info = recording->Info();

      bool s1IsDir = (strchr(filename+strlen(VideoDirectory)+1, '/') != strrchr(filename+strlen(VideoDirectory)+1, '/'));
      bool s2IsDir = (strchr(item->filename+strlen(VideoDirectory)+1, '/') != strrchr(item->filename+strlen(VideoDirectory)+1, '/'));

      if (s1IsDir != s2IsDir) {
        if (s1IsDir)
            return -1;
        else
            return 1;
      }

      if (myMenuRecordings::infotitle && info && info->Title()) {
        char *t = StripEpisodeName(strdup(filename+strlen(VideoDirectory)));
        *(strchr(t, mysetup.DescendSorting?1:255)+1)='\0';
        asprintf(&s1, "%s%s-%s", t, info->Title(), info->ShortText());
        strreplace(s1,' ','_');
        free(t);
      } else {
        s1 = StripEpisodeName(strdup(filename+strlen(VideoDirectory)));
        /** ignore the '%': doesn't matter if a recording is cut or not... */
        char *ptr = strchr(s1, mysetup.DescendSorting?1:255);
        while (*(ptr+1)=='%')
          memmove(ptr+1, ptr+2, strlen(ptr+2));
      }
      if (myMenuRecordings::infotitle && item && item->recording && item->recording->Info() && item->recording->Info()->Title()) {
        char *t = StripEpisodeName(strdup(item->filename+strlen(VideoDirectory)));
        *(strchr(t, mysetup.DescendSorting?1:255)+1)='\0';
        asprintf(&s2, "%s%s-%s", t, item->recording->Info()->Title(), item->recording->Info()->ShortText());
        strreplace(s2,' ','_');
        free(t);
      } else {
        s2 = StripEpisodeName(strdup(item->filename+strlen(VideoDirectory)));
        /** ignore the '%': doesn't matter if a recording is cut or not... */
        char *ptr = strchr(s2, mysetup.DescendSorting?1:255);
        while (*(ptr+1)=='%')
          memmove(ptr+1, ptr+2, strlen(ptr+2));
      }
  } else {
      s1 = StripEpisodeName(strdup(filename+strlen(VideoDirectory)));
      s2 = StripEpisodeName(strdup(item->filename+strlen(VideoDirectory)));
  }

  int compare;
  if(mysetup.DescendSorting)
    compare=strcasecmp(s2,s1);
  else
    compare=strcasecmp(s1,s2);

  free(s1);
  free(s2);

  return compare;
}

#endif

// --- myRecList --------------------------------------------------------------
void myRecList::Sort(bool SortByName)
{
 myRecListItem::SortByName=SortByName;
 cListBase::Sort();
}

// --- WorkerThread -----------------------------------------------------------
WorkerThread::WorkerThread():cThread("extrecmenu worker thread")
{
  cancelmove=cancelcut=false;
  CutterQueue=new CutterList();
  MoveBetweenFileSystemsList=new MoveList();
  CopyBetweenFileSystemsList=new CopyList();

  Start();
}

WorkerThread::~WorkerThread()
{
  Cancel(3);

  delete CutterQueue;
  delete MoveBetweenFileSystemsList;
  delete CopyBetweenFileSystemsList;
}

const char *WorkerThread::Working()
{
  if(CutterQueue->First()!=NULL)
    return tr("Cutter queue not empty");

  if(MoveBetweenFileSystemsList->First()!=NULL)
    return tr("Move recordings in progress");

  return NULL;
}

void WorkerThread::Action()
{
  CutterListItem *cutteritem=NULL;
  MoveListItem *moveitem=NULL;
  CopyListItem *copyitem = NULL;
  SetPriority(19);

  while(Running())
  {
    if((cutteritem=CutterQueue->First())!=NULL)
    {
      cutteritem->SetCutInProgress();

      // create filename for edited recording, check for recordings with this name, if exists -> delete recording
      // (based upon VDR's code (cutter.c))
      cRecording rec(cutteritem->FileName().c_str());

      /* TB: adoption from cuttime-patch */
      cMarks FromMarks;
#if APIVERSNUM >= 10703
      if (FromMarks.Load(cutteritem->FileName().c_str(), rec.FramesPerSecond(), rec.IsPesRecording())) {
#else
      if (FromMarks.Load(cutteritem->FileName().c_str())) {
#endif
         cMark *First=FromMarks.First();
          if (First)
#if APIVERSNUM >= 10721
             rec.SetStartTime(rec.Start() + ((First->Position() / rec.FramesPerSecond() +30)/60)*60);
#elif APIVERSNUM >= 10703
             rec.SetStartTime(rec.start+((First->position/rec.FramesPerSecond()+30)/60)*60);
#else
             rec.SetStartTime(rec.start+((First->position/FRAMESPERSEC+30)/60)*60);
#endif
      }

      const char *editedfilename=rec.PrefixFileName('%');
      if(editedfilename && RemoveVideoFile(editedfilename) && MakeDirs(editedfilename,true))
      {
        char *s=strdup(editedfilename);
        char *e=strrchr(s,'.');
        if(e)
        {
          if(!strcmp(e,".rec"))
          {
            strcpy(e,".del");
            RemoveVideoFile(s);
          }
        }
        free(s);
        rec.WriteInfo(); // don't know why, but VDR also does it
        Recordings.AddByName(editedfilename);
        cutteritem->SetNewFileName(editedfilename);

        Cut(cutteritem->FileName(),editedfilename, cutteritem->GetStartTime(), cutteritem->DisplayName());
      }
      else
        Skins.QueueMessage(mtError,tr("Can't start editing process!"));

      time_t startTime = cutteritem->GetStartTime();
      NotifyBgProcess(tr("Cutting"), cutteritem->DisplayName().c_str(), startTime, 101);
      CutterQueue->Del(cutteritem);

      Recordings.ChangeState();
    }

    if((moveitem=MoveBetweenFileSystemsList->First())!=NULL)
    {
      moveitem->SetMoveInProgress();
      if(Move(moveitem->From(),moveitem->To()))
        MoveBetweenFileSystemsList->Del(moveitem);
      else
        // error occured -> empty move queue
        MoveBetweenFileSystemsList->Clear();

      Recordings.ChangeState();
    }

    if((copyitem = CopyBetweenFileSystemsList->First()) != NULL)
    {
      copyitem->SetCopyInProgress();
      if(Copy(copyitem->From(), copyitem->To()))
        CopyBetweenFileSystemsList->Del(copyitem);
      else
        // error occured -> empty move queue
        CopyBetweenFileSystemsList->Clear();

      Recordings.ChangeState();
    }

    usleep(100000);
  }
}

void WorkerThread::AddToCutterQueue(std::string Path, std::string Title)
{
  time_t now = time(NULL);
  //printf("Title: %s\n", Title.c_str());
  NotifyBgProcess(tr("Cutting"), Title.c_str(), now, 0);
  CutterQueue->Add(new CutterListItem(Path, Title, now));
}

bool WorkerThread::IsCutting(string Path)
{
  for(CutterListItem *item=CutterQueue->First();item;item=CutterQueue->Next(item))
  {
    if(Path==item->FileName() || Path==item->NewFileName())
      return true;
  }
  return false;
}

void WorkerThread::CancelCut(string Path)
{
  for(CutterListItem *item=CutterQueue->First();item;item=CutterQueue->Next(item))
  {
    if(item->FileName()==Path || item->NewFileName()==Path)
    {
      if(item->GetCutInProgress())
        cancelcut=true;
      else {
        time_t startTime = item->GetStartTime();
        NotifyBgProcess(tr("Cutting"), item->DisplayName().c_str(), startTime, 101);
        CutterQueue->Del(item);
      }
      return;
    }
  }
}

#ifndef CUTTER_MAX_BANDWIDTH
#ifdef RBLITE
#  define CUTTER_MAX_BANDWIDTH MEGABYTE(3) // 10 MB/s
#else
#  define CUTTER_MAX_BANDWIDTH MEGABYTE(20) // 10 MB/s
#endif
#endif
#ifndef CUTTER_REL_BANDWIDTH
#  define CUTTER_REL_BANDWIDTH 75 // %
#endif
#ifndef CUTTER_PRIORITY
#  define CUTTER_PRIORITY sched_get_priority_min(SCHED_OTHER)
#endif
#define CUTTER_TIMESLICE   100   // ms

void WorkerThread::CheckTS(cUnbufferedFile *replayFile)
{
   uchar * pp = NULL;
   pp = MALLOC(uchar, 2*TS_SIZE);
   if ( pp != NULL) {
      if ( replayFile->Read( pp, 2*TS_SIZE ) == 2*TS_SIZE ) {
         if ( (*pp == 0x47 && *(pp+1) == 0x40) && (*(pp+TS_SIZE) == 0x47 && *(pp+TS_SIZE+1) == 0x40)) {
            printf("cCuttingThread::CheckTS(): TS recognized\n");
            PATPMT = pp;
         } else {
            printf("cCuttingThread::CheckTS(): PES recognized\n");
            PATPMT = NULL;
            free(pp);
         }
      }
      if (PATPMT == NULL)
         replayFile->Seek(0,SEEK_SET);
   }
}

// this based mainly upon VDR's code (cutter.c)
void WorkerThread::Cut(string From,string To, time_t startTime, string displayname)
{
  cUnbufferedFile *fromfile=NULL,*tofile=NULL;
  cFileName *fromfilename=NULL,*tofilename=NULL;
  cIndexFile *fromindex=NULL,*toindex=NULL;
  cMarks frommarks,tomarks;
  cMark *mark, *markNext;
  const char *error=NULL;
#if VDRVERSNUM > 10703 // TODO find correct version
  uchar buffer[MAXFRAMESIZE];
  uint16_t filenumber;
  bool picturetype;
  int length,index,currentfilenumber=0,filesize=0,lastiframe=0;
  off_t fileoffset;
#else
  uchar filenumber,picturetype,buffer[MAXFRAMESIZE];
  int fileoffset,length,index,currentfilenumber=0,filesize=0,lastiframe=0;
#endif

  bool lastmark=false,cutin=true;
  int nrMarks = 0, nrFramesToCut = 0, nrFramesCut = 0;
  {
    sched_param tmp;
    tmp.sched_priority = CUTTER_PRIORITY;
    if(pthread_setschedparam(pthread_self(), SCHED_OTHER, &tmp) != 0)
      printf("WorkerThread::Cut: cant set priority, error: %s\n", strerror(errno));
  }

  int bytes = 0;
  int __attribute__((unused)) burst_size = CUTTER_MAX_BANDWIDTH * CUTTER_TIMESLICE / 1000; // max bytes/timeslice
  cTimeMs __attribute__((unused)) t;
#if APIVERSNUM >= 10703
  cRecording rec(From.c_str());
  bool isPes = rec.IsPesRecording();
  if(frommarks.Load(From.c_str(), rec.FramesPerSecond(), isPes) && frommarks.Count())
  {
    fromfilename=new cFileName(From.c_str(),false,true, isPes);
    tofilename=new cFileName(To.c_str(),true,false, isPes);
    fromindex=new cIndexFile(From.c_str(),false, isPes);
    toindex=new cIndexFile(To.c_str(),true, isPes);
    tomarks.Load(To.c_str(), rec.FramesPerSecond(), isPes);
    toindex->StoreResume(0);  // set new recording as seen
  }
#else
  if(frommarks.Load(From.c_str()) && frommarks.Count())
  {
    fromfilename=new cFileName(From.c_str(),false,true);
    tofilename=new cFileName(To.c_str(),true,false);
    fromindex=new cIndexFile(From.c_str(),false);
    toindex=new cIndexFile(To.c_str(),true);
    tomarks.Load(To.c_str());
  }
#endif
  else
  {
    esyslog("[extrecmenu] no editing marks found for %s",From.c_str());
    return;
  }


  /* TB: count marks */
  mark = frommarks.First();
  while (mark) {
    nrMarks++;
    mark = frommarks.Next(mark);
  }

  /* TB: calcutate number of frames to cut */
  if (nrMarks>=2) {
      mark = frommarks.First();
      markNext = frommarks.Next(mark);
#if APIVERSNUM < 10721
      nrFramesToCut += markNext->position - mark->position;
#else
      nrFramesToCut += markNext->Position() - mark->Position();
#endif
      nrMarks-=2;

      while (nrMarks>=2) {
        mark = frommarks.Next(markNext);
        markNext = frommarks.Next(mark);
#if APIVERSNUM < 10721
        nrFramesToCut += markNext->position - mark->position;
#else
        nrFramesToCut += markNext->Position() - mark->Position();
#endif
        nrMarks-=2;
      }
  }
  if (nrMarks) { /* one cutting-mark left -> up to the end */
    mark = frommarks.Last();
#if APIVERSNUM < 10721
    nrFramesToCut += fromindex->Last() - mark->position;
#else
    nrFramesToCut += fromindex->Last() - mark->Position();
#endif
  }

  if((mark=frommarks.First())!=NULL)
  {
    if(!(fromfile=fromfilename->Open()) || !(tofile=tofilename->Open()))
      return;
    CheckTS(fromfile); //TB
    fromfile->SetReadAhead(MEGABYTE(20));
#if APIVERSNUM < 10721
    index=mark->position;
#else
    index=mark->Position();
#endif
    mark=frommarks.Next(mark);
    tomarks.Add(0);
    tomarks.Save();
  }
  else
  {
    esyslog("[extrecmenu] no editing marks found for %s",From.c_str());
    return;
  }

  isyslog("[extecmenu] editing %s",From.c_str());
  while(fromindex->Get(index++,&filenumber,&fileoffset,&picturetype,&length) && Running() && !cancelcut)
  {
    /* TB: notify bg-process-plugin */
    nrFramesCut++;
    if(nrFramesToCut && nrFramesCut && nrFramesCut%250==0) {
      NotifyBgProcess(tr("Cutting"), displayname.c_str(), startTime, nrFramesCut*100/nrFramesToCut);
    }

    AssertFreeDiskSpace(-1);

    if(filenumber!=currentfilenumber)
    {
      fromfile=fromfilename->SetOffset(filenumber,fileoffset);
      fromfile->SetReadAhead(MEGABYTE(20));
      currentfilenumber=filenumber;
    }
    if(fromfile)
    {
      int len=ReadFrame(fromfile,buffer,length,sizeof(buffer));
      if(len<0)
      {
        error="ReadFrame";
        break;
      }
      if(len!=length)
      {
        currentfilenumber=0;
        length=len;
      }
    }
    else
    {
      error="fromfile";
      break;
    }
#if VDRVERSNUM >= 10710 // TODO right version number
#define I_FRAME 1
#endif

    if(picturetype==I_FRAME || PATPMT != NULL)
    {
      if(lastmark)
        break;
              if (filesize == 0) {
                 if (PATPMT != NULL) {
                    if (tofile->Write(PATPMT, 2*TS_SIZE) < 0) // Add PATPMT to start of every file
                       break;
                    else
                       filesize+=TS_SIZE*2;
                    }
                 }
      if(filesize > MEGABYTE(Setup.MaxVideoFileSize))
      {
        tofile=tofilename->NextFile();
        if(!tofile)
        {
          error="tofile 1";
          break;
        }
        filesize=0;
                 if (PATPMT != NULL) {
                    if (tofile->Write(PATPMT, 2*TS_SIZE) < 0) // Add PATPMT to start of every file
                       break;
                    else
                       filesize+=TS_SIZE*2;
                    }
      }
      lastiframe=0;
      if(cutin)
      {
        cRemux::SetBrokenLink(buffer,length);
        cutin=false;
      }
    }
    if(tofile->Write(buffer,length)<0)
    {
      error="safe_write";
      break;
    }
    if(!toindex->Write(picturetype,tofilename->Number(),filesize))
    {
      error="toindex";
      break;
    }
    filesize+=length;
    if(!lastiframe)
      lastiframe=toindex->Last();

#if APIVERSNUM < 10721
    if(mark && index >= mark->position)
#else
    if(mark && index >= mark->Position())
#endif
    {
      mark=frommarks.Next(mark);
      tomarks.Add(lastiframe);
      if(mark)
        tomarks.Add(toindex->Last()+1);
      tomarks.Save();
      if(mark)
      {
#if APIVERSNUM < 10721
        index=mark->position;
#else
        index=mark->Position();
#endif
        mark=frommarks.Next(mark);
        currentfilenumber=0;
        cutin=true;
        if(Setup.SplitEditedFiles)
        {
          tofile=tofilename->NextFile();
          if(!tofile)
          {
            error="tofile 2";
            break;
          }
          filesize=0;
        }
      }
      else
        lastmark=true;

       bytes += length;
       if(bytes >= burst_size) {
         int elapsed = t.Elapsed();
         int sleep = 0;

#if CUTTER_REL_BANDWIDTH > 0 &&  CUTTER_REL_BANDWIDTH < 100
         // stay under max. relative bandwidth

         sleep = (elapsed * 100 / CUTTER_REL_BANDWIDTH) - elapsed;
         //if(sleep<=0 && elapsed<=2) sleep = 1;
         //if(sleep) esyslog("cutter: relative bandwidth limit, sleep %d ms (chunk %dk / %dms)", sleep, burst_size/1024, CUTTER_TIMESLICE);
#endif
         // stay under max. absolute bandwidth
         if(elapsed < CUTTER_TIMESLICE) {
           sleep = std::max(CUTTER_TIMESLICE - elapsed, sleep);
           //if(sleep) esyslog("cutter: absolute bandwidth limit, sleep %d ms (chunk %dk / %dms)", sleep, burst_size/1024, CUTTER_TIMESLICE);
         }

         if(sleep>0)
           cCondWait::SleepMs(sleep);
         t.Set();
         bytes = 0;
       }

    }
  }
  if(!Running() || cancelcut || error)
  {
    if(error)
      esyslog("[extrecmenu] ERROR: '%s' during editing process",error);
    else
      isyslog("[extrecmenu] editing process canceled, deleting edited recording");

    cancelcut=false;
    RemoveVideoFile(To.c_str());
    Recordings.DelByName(To.c_str());
  }
  else
  {
    isyslog("[extrecmenu] editing process ended");
    cRecordingUserCommand::InvokeCommand(RUC_EDITEDRECORDING,To.c_str());
  }
  Recordings.TouchUpdate();
  delete fromfilename;
  delete tofilename;
  delete fromindex;
  delete toindex;
  if( PATPMT != NULL )
     free(PATPMT);
}

bool WorkerThread::IsMoving(string Path)
{
  for(MoveListItem *item=MoveBetweenFileSystemsList->First();item;item=MoveBetweenFileSystemsList->Next(item))
  {
    if(Path==item->From() && !item->GetMoveCanceled())
      return true;
  }
  return false;
}

bool WorkerThread::IsCopying(string Path) {
  for(CopyListItem *item=CopyBetweenFileSystemsList->First();item;item=CopyBetweenFileSystemsList->Next(item)) {
    if(Path==item->From() && !item->GetCopyCanceled())
      return true;
  }
  return false;
}

void WorkerThread::CancelMove(string Path)
{
  for(MoveListItem *item=MoveBetweenFileSystemsList->First();item;item=MoveBetweenFileSystemsList->Next(item))
  {
    if(Path==item->From())
    {
      if(item->GetMoveInProgress())
      {
        cancelmove=true;
        item->SetMoveCanceled();
      }
      else
        MoveBetweenFileSystemsList->Del(item);

      return;
    }
  }
}

void WorkerThread::CancelCopy(string Path) {
  for(CopyListItem *item=CopyBetweenFileSystemsList->First();item;item=CopyBetweenFileSystemsList->Next(item)) {
    if(Path==item->From()) {
      if(item->GetCopyInProgress()) {
        cancelmove=true;
        item->SetCopyCanceled();
      } else
        CopyBetweenFileSystemsList->Del(item);
      return;
    }
  }
}

void WorkerThread::AddToMoveList(string From,string To)
{
  MoveBetweenFileSystemsList->Add(new MoveListItem(From,To));
  Recordings.ChangeState();
}

void WorkerThread::AddToCopyList(string From,string To)
{
  CopyBetweenFileSystemsList->Add(new CopyListItem(From,To));
  Recordings.ChangeState();
}

bool WorkerThread::Move(string From,string To)
{
  int blocks=0;
  if(!MakeDirs(To.c_str(),true))
  {
    Skins.QueueMessage(mtError,tr("Creating directories failed!"));
    return false;
  }
  SetPriority(19);

  isyslog("[extrecmenu] moving '%s' to '%s'",From.c_str(),To.c_str());

  DIR *dir=NULL;
  struct dirent *entry;
  int infile=-1,outfile=-1;

  if((dir=opendir(From.c_str()))!=NULL)
  {
    bool ok=true;
    time_t start_time = time(NULL);
    long long total_size = du_ignore_hidden_dirs(From.c_str());
    long long written_size = 0;
    int percent = 0;
    cTimeMs notify_timeout;

    NotifyBgProcess(tr("Moving"), tr("started"), start_time, 0);
    // copy each file in this dir, except sub-dirs
    while((entry=readdir(dir))!=NULL)
    {
      string from,to;
      from=From+"/"+entry->d_name;
      to=To+"/"+entry->d_name;

      AssertFreeDiskSpace(-1);

      struct stat st;
      if(stat(from.c_str(),&st)==0)
      {
        if(S_ISREG(st.st_mode))
        {
          isyslog("[extrecmenu] moving '%s'",entry->d_name);

          ssize_t sz,sz_read=1,sz_write;
          if(stat(from.c_str(),&st)==0 && (infile=open(from.c_str(),O_RDONLY))!=-1 && (outfile=open(to.c_str(),O_WRONLY|O_CREAT|O_EXCL,st.st_mode))!=-1)
          {
            char buf[BUFFERSIZE];
            while(sz_read>0 && (sz_read=read(infile,buf,BUFFERSIZE))>0)
            {
              AssertFreeDiskSpace(-1);

          if (notify_timeout.TimedOut()) // notify every 2 sec
          {
              if (total_size)
              {
                  percent = (int) (written_size * 100.0/ total_size);
                  std::cout<<written_size << "/" << total_size << "% = "<< percent<<std::endl;

                  if(percent < 0) percent = 0;
                  else if (percent > 100) percent = 100;
              }
              else
                  percent = 0;

	      cRecording *rec = new cRecording(From.c_str());
	      NotifyBgProcess(tr("Moving"), rec->Info()->Title(), start_time, percent);
	      delete rec;
              notify_timeout.Set(2000); //2 secs before notifying
          } // timeout

              sz_write=0;
              do
              {
                if(cancelmove || !Running())
                {
                  cancelmove=false;

                  close(infile);
                  close(outfile);
                  closedir(dir);

                  NotifyBgProcess(tr("Moving"), tr("Cancelled"), start_time, 0);
                  isyslog("[extrecmenu] moving canceled");

                  RemoveVideoFile(To.c_str());

                  return true;
                }

                if((sz=write(outfile,buf+sz_write,sz_read-sz_write))<0)
                {
                  close(infile);
                  close(outfile);
                  closedir(dir);

                  Skins.Message(mtError,tr("Rename/Move failed!"));
                  esyslog("[extrecmenu] WorkerThread::Move() - write() - %s",strerror(errno));
                  return false;
                }
                sz_write+=sz;
              }
              while(sz_write<sz_read);
              blocks++;
	      switch(mysetup.LimitBandwidth) {
	      case 0:  // 16MB/s
		      if ((blocks&3)==0)
			      cCondWait::SleepMs(5);
		      break;
	      case 1:  // 7MB/s
		      cCondWait::SleepMs(5);
		      break;
	      case 2:  // 3MB/s
		      cCondWait::SleepMs(15);
		      break;
	      case 3:  // no limit
		      if ((blocks&31)==0)
			      cCondWait::SleepMs(5);
		      break;
	      }

	      written_size += sz_write;
            }
            close(infile);
            close(outfile);
          }
          else
          {
            ok=false;
            break;
          }
        }
      }
      else
      {
        ok=false;
        break;
      }
    }

    NotifyBgProcess(tr("Moving"), ok?tr("Completed"):tr("Failed"), start_time, ok?110:0);

    if(ok)
    {
      closedir(dir);

      cThreadLock RecordingsLock(&Recordings);
      cRecording rec(From.c_str());
      rec.Delete();
      Recordings.DelByName(From.c_str());
      Recordings.AddByName(To.c_str());

      string cmdstring="move \"";
      cmdstring+=myStrEscape(From,"'\\\"$");
      cmdstring+="\"";
      cRecordingUserCommand::InvokeCommand(cmdstring.c_str(),To.c_str());

      Recordings.TouchUpdate();

      return true;
    }
  }
  if(dir)
    closedir(dir);
  if(infile!=-1)
    close(infile);
  if(outfile!=-1)
    close(outfile);

  Skins.QueueMessage(mtError,tr("Rename/Move failed!"));
  esyslog("[extrecmenu] WorkerThread::Move() - %s",strerror(errno));
  return false;
}

bool WorkerThread::Copy(string From,string To) {
  if(!MakeDirs(To.c_str(),true)) {
    Skins.QueueMessage(mtError,tr("Creating directories failed!"));
    return false;
  }
  SetPriority(19);

  isyslog("[extrecmenu] copying '%s' to '%s'",From.c_str(),To.c_str());

  DIR *dir = NULL;
  struct dirent *entry;
  int infile = -1, outfile = -1;

  if((dir = opendir(From.c_str())) != NULL) {
    bool ok = true;
    time_t start_time = time(NULL);
    long long total_size = du_ignore_hidden_dirs(From.c_str());
    long long written_size = 0;
    int percent = 0;
    cTimeMs notify_timeout;

    NotifyBgProcess(tr("Copying"), tr("started"), start_time, 0);
    // copy each file in this dir, except sub-dirs
    while((entry = readdir(dir)) != NULL) {
      string from , to;
      from= From + "/" + entry->d_name;
      to= To + "/" + entry->d_name;

      AssertFreeDiskSpace(-1);

      struct stat st;
      if(stat(from.c_str(),&st)==0) {
        if(S_ISREG(st.st_mode)) {
          isyslog("[extrecmenu] copying '%s'",entry->d_name);

          ssize_t sz, sz_read = 1, sz_write;
          if(stat(from.c_str(),&st)==0 && (infile=open(from.c_str(),O_RDONLY))!=-1 && (outfile=open(to.c_str(),O_WRONLY|O_CREAT|O_EXCL,st.st_mode))!=-1) {
            char buf[BUFFERSIZE];
            while(sz_read>0 && (sz_read=read(infile,buf,BUFFERSIZE))>0) {
              AssertFreeDiskSpace(-1);

              if (notify_timeout.TimedOut()) { // notify every 2 sec
                  if (total_size) {
                      percent = (int) (written_size * 100.0/ total_size);
                      std::cout<<written_size << "/" << total_size << "% = "<< percent<<std::endl;

                      if(percent < 0) percent = 0;
                      else if (percent > 100) percent = 100;
                  } else
                      percent = 0;

                  NotifyBgProcess(tr("Copying"), tr("copying recording"), start_time, percent);
                  notify_timeout.Set(2000); //2 secs before notifying
              } // timeout

              sz_write = 0;
              do {
                if(cancelcopy || !Running()) {
                  printf("XXXX: cc: %i r: %i\n", cancelcopy, Running());
                  cancelcopy = false;

                  close(infile);
                  close(outfile);
                  closedir(dir);

                  NotifyBgProcess(tr("Copying"), tr("Cancelled"), start_time, 0);
                  isyslog("[extrecmenu] copying canceled");

                  RemoveVideoFile(To.c_str());

                  return true;
                }

                if((sz = write(outfile,buf+sz_write,sz_read-sz_write)) < 0) {
                  close(infile);
                  close(outfile);
                  closedir(dir);

                  Skins.Message(mtError,tr("Copy failed!"));
                  esyslog("[extrecmenu] WorkerThread::Copy() - write() - %s",strerror(errno));
                  return false;
                }
                sz_write += sz;
              } while(sz_write<sz_read);

              if(mysetup.LimitBandwidth)
                usleep(10);

              written_size += sz_write;
              cCondWait::SleepMs(5);
            }
            close(infile);
            close(outfile);
          } else {
            ok = false;
            break;
          }
        }
      } else {
        ok = false;
        break;
      }
    }

    NotifyBgProcess(tr("Copying"), ok ? tr("Completed") : tr("Failed"), start_time, ok ? 110 : 0);

    if(ok) {
      closedir(dir);

#if 0
      cThreadLock RecordingsLock(&Recordings);
      Recordings.AddByName(To.c_str());

/*
      string cmdstring="move \"";
      cmdstring+=myStrEscape(From,"'\\\"$");
      cmdstring+="\"";
      cRecordingUserCommand::InvokeCommand(cmdstring.c_str(),To.c_str());
*/

#endif

      Recordings.TouchUpdate();

      return true;
    }
  }
  if(dir)
    closedir(dir);
  if(infile != -1)
    close(infile);
  if(outfile != -1)
    close(outfile);

  Skins.QueueMessage(mtError,tr("Copy failed!"));
  esyslog("[extrecmenu] WorkerThread::Copy() - %s",strerror(errno));
  return false;
}

