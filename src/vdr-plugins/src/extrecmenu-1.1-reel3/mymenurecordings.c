/*
 * See the README file for copyright information and how to reach the author.
 */

#include <sys/statvfs.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <vdr/interface.h>
#include <vdr/videodir.h>
#include <vdr/status.h>
#include <vdr/plugin.h>
#include <vdr/cutter.h>
#include <vdr/menu.h>
#include "extrecmenu.h"

#include "myreplaycontrol.h"
#include "mymenurecordings.h"
#include "mymenusetup.h"
#include "mymenucommands.h"
#if !defined(REELVDR)
#include "patchfont.h"
#endif
#include "tools.h"
#include "SelectFile.h"
#include "SelectMedia.h"
#include "ExternalMedia.h"
#include "../bgprocess/service.h"
#include <time.h>


std::string RecordingsIcons(const cRecording *Recording) {

    if (!Recording) return "";

    /** Icons string **/
    char col[16] = {0};
    int i = 0;

    std::string buffer, filename;
    filename = Recording->FileName();

    buffer = filename + "/protection.fsk";
    bool isProtected = !access(buffer.c_str(), R_OK);

    buffer = filename + "/dvd.vdr";
    bool isdvd = !access(buffer.c_str(), R_OK);

    /* Icon in Column 1 */

    //New
    if(Recording->IsNew()) {
        col[i++] = char(132);
    }

    //Protected
    if(isProtected) {
        col[i++] = char(80);
    }

    /// 142, 143, 144, 145   HD, HDenc, newHD, newEnc

    //HD recording
    if(Recording->IsCompressed()){
        col[i++] = char(148);
    } else if(Recording->IsHD()){
        col[i++] = char(142);
    }

#if 0 /* do not show combined icons*/
    // New + HD
    if(Recording->IsNew() && Recording->IsHD()) {
        col[i] = char(144);
    }

    //New + protected
    if(Recording->IsNew() && isProtected) {
        col[i] = char(145);
    }

    //HD + protected
    if (Recording->IsHD() && isProtected) {
        col[i] = char(143);
    }

    // HD + New + protected
    if (Recording->IsHD() && Recording->IsNew() && isProtected) {
        col[i] = char(143); // TODO Change icon
    }

    /// the above 7 cases result in a single icon
    if (col[i] != 0) // icon was found, at least one case was true
        ++i;
#endif


    // recording being moved
    if(MoveCutterThread->IsMoving(filename))
        col[i++] = char(136);


    /* Icon in Column 2 */

    //scissor icon, a cut recording
    if(Recording->IsEdited())
        col[i++] = (char)133;

    //archived recording
    if(isdvd)
        col[i++] = char(129);

    // being cut OR currently being recorded
    if(MoveCutterThread->IsCutting(filename))
        col[i++] = '#';

    /// clear other icons if
    // recording
    if(cRecordControls::GetRecordControl(filename.c_str()))
    {
        col[i++] = '#';
    }

    return col;

} // Recordings Icons


using namespace std;
//static char lastReplayedFilename[1024]="";

#if 0
// --- myMenuRecordingInfo ----------------------------------------------------
// this class is mostly a copy of VDR's cMenuRecording class, I only adjusted
// the Display()-method
// ----------------------------------------------------------------------------
class myMenuRecordingInfo:public cOsdMenu
{
  private:
    const cRecording *recording;
    bool withButtons;
    eOSState Play();
    eOSState Rewind();
    bool bPicIsZoomed;

    bool bPictureAvailable;
    bool CheckForPicture(const cRecording *Recording);
  public:
    myMenuRecordingInfo(const cRecording *Recording,bool WithButtons = false);
    virtual void Display(void);
    virtual eOSState ProcessKey(eKeys Key);
};
#endif
bool myMenuRecordingInfo::CheckForPicture(const cRecording *Recording)
{
  string PngPath = Recording->FileName();
  PngPath += "/vdr.png";
  struct stat results;
  if ( stat ( PngPath.c_str(), &results ) == 0 )
    return(true);
  else
    return false;
}

myMenuRecordingInfo::myMenuRecordingInfo(const cRecording *Recording, bool WithButtons):cOsdMenu(tr("Recording info"))
{
  bPictureAvailable = CheckForPicture(Recording);

  recording=Recording;
  withButtons=WithButtons;
  if(withButtons)
      SetHelp(tr("Button$Play"), recording->IsNew() ? NULL : tr("Button$Rewind"), bPictureAvailable ? tr("Large picture") : NULL, /*tr("Button$Back")*/ NULL);
  Display();

  bPicIsZoomed = false;
}

void myMenuRecordingInfo::Display(void)
{
  cOsdMenu::Display();

  if(mysetup.UseVDRsRecInfoMenu)
  {
    DisplayMenu()->SetRecording(recording);

    if(recording->Info()->Description())
      cStatus::MsgOsdTextItem(recording->Info()->Description());
  }
  else
  {
    stringstream text;
#if APIVERSNUM < 10721
    text << *DateString(recording->start) << ", " << *TimeString(recording->start) << "\n\n";
#else
    text << *DateString(recording->Start()) << ", " << *TimeString(recording->Start()) << "\n\n";
#endif
    if(recording->Info()->Title())
    {
      text << recording->Info()->Title() << "\n\n";
      if(recording->Info()->Description())
        text << recording->Info()->Description() << "\n\n";
    }

    string recname=recording->Name();
    string::size_type i=recname.rfind('~');
    if(i!=string::npos)
      text << tr("Name") << ": " << recname.substr(i+1,recname.length()) << "\n"
           << tr("Path") << ": " << recname.substr(0,i) << "\n";
    else
      text << tr("Name") << ": " << recname << "\n";

    cChannel *chan=Channels.GetByChannelID(((cRecordingInfo*)recording->Info())->ChannelID());
    if(chan)
      text << tr("Channel") << ": " << *ChannelString(chan,0) << "\n";

    int recmb=DirSizeMB(recording->FileName());
    if(recmb<0)
      recmb=0;
    if(recmb > 1023)
      text << tr("Size") << ": " << setprecision(3) << recmb/1024.0 << " GB\n";
    else
      text << tr("Size") << ": " << recmb << " MB\n";
#if APIVERSNUM < 10721
    text << tr("Priority") << ": " << recording->priority << "\n";
    text << tr("Lifetime") << ": " << recording->lifetime << "\n";
#else
    text << tr("Priority") << ": " << recording->Priority() << "\n";
    text << tr("Lifetime") << ": " << recording->Lifetime() << "\n";
#endif
    DisplayMenu()->SetText(text.str().c_str(),false);
  }

  bPicIsZoomed = false;

}

eOSState myMenuRecordingInfo::ProcessKey(eKeys Key)
{
  switch(Key)
  {
    case kUp|k_Repeat:
    case kUp:
    case kDown|k_Repeat:
    case kDown:
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight: DisplayMenu()->Scroll(NORMALKEY(Key)==kUp||NORMALKEY(Key)==kLeft,NORMALKEY(Key)==kLeft||NORMALKEY(Key)==kRight);
                 cStatus::MsgOsdTextItem(NULL, NORMALKEY(Key) == kUp);
                 return osContinue;
      default: break;
  }

  eOSState state=cOsdMenu::ProcessKey(Key);
  if(state==osUnknown)
  {
    switch(Key)
    {
      case kRed:
          if (withButtons)
              Key=kOk;
      case kGreen:
          if (!withButtons || ( recording->IsNew() && Key == kGreen ))
              break;
          else
              cRemote::Put(Key,true);
          return osBack;
      case kYellow:
          {
              if(bPictureAvailable)
              {
                  if(bPicIsZoomed)
                  {
                      Display(); // Repaint -> Clear zoomed picture
                      bPicIsZoomed = false;
                  } else
                  {
                      // Construct Recording entry with information necessary to display Picture for recording
                      // Will be displayed in zoomed mode
                      cRecording *rec = new cRecording(recording->FileName(), recording->BaseDir());
                      rec->SetTitle("ZOOMPIC$", recording->Info()->Title());
                      DisplayMenu()->SetRecording(rec);

                      delete rec;
                      bPicIsZoomed = true;

                      return osContinue;
                  }
              }
              break;
          }
          //case kBlue:
          case kOk: return osBack;
      default: break;
    }
  }
  return state;
}

// --- myMenuRecordingsItem ---------------------------------------------------
myMenuRecordingsItem::myMenuRecordingsItem(cRecording *recording,const char *parent,int level)
{
  totalentries=newentries=0;
  isdvd=false;
  dirismoving=true;
  name=NULL;
  isProtected = false;
  Recording = recording;
  Level = level;
  Parent = parent;
  lastPercentage = 100;

  //printf("##########################myMenuRecordingsItem::myMenuRecordingsItem, Paren = %s\n", Parent);

#if VDRVERSNUM >= 10703
  isPesRecording = Recording->IsPesRecording();
#endif
#if REELVDR
#if APIVERSNUM < 10721
  latestRecordingStartTime = Recording->start;
#else
  latestRecordingStartTime = Recording->Start();
#endif
#endif

  filename=Recording->FileName();

  // get the level of this recording
  level=0;
  const char *s=Recording->Name();
  while(*++s)
  {
    if(*s=='~')
      level++;
  }

  // create the title of this item
  if(Level<level) // directory entry
  {
    s=Recording->Name();
    const char *p=s;
    while(*++s)
    {
      if(*s=='~')
      {
        if(Level--)
          p=s+1;
        else
          break;
      }
    }
    title=MALLOC(char,s-p+4);
    *title='\t';
    *(title+1)='\t';
    *(title+2)='\t';
    strn0cpy(title+3,p,s-p+1);
    name=strdup(title+3);
    uniqid=name;

   if (IsDirectory() && strlen(name)) {
       char *name2 = strdup(name);
       name2 = ExchangeChars(name2, true);
       if (std::string(filename).rfind(name2)) {
           std::string buffer = std::string(filename).erase(std::string(filename).rfind(name2)+strlen(name2));
           buffer += "/protection.fsk";
           isProtected = (access(buffer.c_str(),R_OK) == 0);
           if (isProtected) {
               free(title);
               title = (char*)malloc(strlen(name)+6);
               *(title+1) = *(title+2) = *(title+3) = '\t';
               *(title) = 'p';
               strn0cpy(title+5, name, strlen(name));
           }

       }
       free(name2);
   }
  }
  else
  {
    if(Level==level) // recording entry
    {
      string buffer;
      stringstream titlebuffer;
      stringstream idbuffer;

   // pin protection functionality
   //asprintf(&buffer, "%s/protection.fsk", filename);
   buffer = filename;
   buffer += "/protection.fsk";
   isProtected = (access(buffer.c_str(),R_OK) == 0);



      buffer=filename;
#if VDRVERSNUM > 10703
      if (isPesRecording)
        buffer+="/001.vdr";
      else
        buffer+="/00001.ts";
#else
      buffer+="/001.vdr";
#endif
      if(access(buffer.c_str(),R_OK))
      {
        buffer=filename;
        buffer+="/dvd.vdr";
        isdvd=!access(buffer.c_str(),R_OK);

        if(isdvd)
        {
            FILE *f;
            if((f=fopen(buffer.c_str(),"r"))!=NULL)
            {
            // get the dvd id
            if(fgets(dvdnr,sizeof(dvdnr),f))
            {
            char *p=strchr(dvdnr,'\n');
            if(p)
            *p=0;
            }
            // determine if the dvd is a video dvd
            char tmp[BUFSIZ];
            if(fgets(tmp,sizeof(tmp),f))
            isvideodvd=true;

            fclose(f);
            }
        }

      }

#if 0 // drawing two icons in one column
      // marker
      if(MoveCutterThread->IsMoving(filename))
        titlebuffer << char(136);else // donot show both icons
	   {
            if(Recording->IsNew() && !mysetup.PatchNew)
                titlebuffer << '*';
            if(Recording->IsNew()&&mysetup.PatchNew)
                titlebuffer << char(132);
            if(Recording->IsEdited()&&!Recording->IsNew())
                titlebuffer << char(133);
            if(Recording->IsEdited()&&Recording->IsNew())
                titlebuffer << char(133);
	   }
        // clear every other icons if DVD / protected

       if(MoveCutterThread->IsCutting(filename) || cRecordControls::GetRecordControl(filename))
        {  titlebuffer.str(""); titlebuffer << '#'; }
        else
       if(isdvd)
           { titlebuffer.str(""); titlebuffer<< char(129);}
           else
       if (isProtected)
           { titlebuffer.str(""); titlebuffer << char(80); } // P

      //titlebuffer << ' ';

      titlebuffer << '\t';
	  titlebuffer << ' ';
#endif

          /** Icons in two columns **/
          char col1[2] = {0}; // icon in column 1
          char col2[2] = {0}; // icon in column 2


          /* Icon in Column 1 */

          //New
          if(Recording->IsNew()) {
              col1[0] = char(132);
          }

          //Protected
          if(isProtected) {
              col1[0] = char(80);
          }

          /// 142, 143, 144, 145   HD, HDenc, newHD, newEnc

          //HD recording
          if(Recording->IsHD()){
              col1[0] = char(142);
          }

          // New + HD
          if(Recording->IsNew() && Recording->IsHD()) {
              col1[0] = char(144);
          }

          //New + protected
          if(Recording->IsNew() && isProtected) {
              col1[0] = char(145);
          }

          //HD + protected
          if (Recording->IsHD() && isProtected) {
              col1[0] = char(143);
          }

          // HD + New + protected
          if (Recording->IsHD() && Recording->IsNew() && isProtected) {
              col1[0] = char(143); // TODO Change icon
          }

          if (Recording->IsCompressed()) {
              col1[0] = char(148);
          }

          // recording being moved
          if(MoveCutterThread->IsMoving(filename))
              col1[0] = char(136);


          /* Icon in Column 2 */

          //scissor icon, a cut recording
          if(Recording->IsEdited())
              col2[0] = (char)133;

          //archived recording
          if(isdvd)
              col2[0] = char(129);

          // being cut OR currently being recorded
          if(MoveCutterThread->IsCutting(filename))
              col2[0] = '#';

          /// clear other icons if
          // recording
          if(cRecordControls::GetRecordControl(filename))
          {
              col1[0] = '#'; col2[0] = ' ';
          }


          //Set icons in buffer
          titlebuffer << string(col1) << '\t' << string(col2) << '\t' /*<< ' '*/;


      // date and time of recording
      struct tm tm_r;
#if APIVERSNUM < 10721
      struct tm *t=localtime_r(&Recording->start,&tm_r);
#else
      time_t start = Recording->Start();
      struct tm *t=localtime_r(&start, &tm_r);
#endif

      if(mysetup.ShowRecDate)
        titlebuffer << setw(2) << setfill('0') << t->tm_mday << '.'
                    << setw(2) << setfill('0') << t->tm_mon+1 << '.'
                    << setw(2) << setfill('0') << t->tm_year%100 << '\t';

      if(mysetup.ShowRecTime)
        titlebuffer << setw(2) << setfill('0') << t->tm_hour << ':'
                    << setw(2) << setfill('0') << t->tm_min << '\t';


      idbuffer << t->tm_mday << t->tm_mon << t->tm_year
               << t->tm_hour << t->tm_min;

      // recording length
      if(mysetup.ShowRecLength)
      {
// Behni TODO: Reihenfolge fuer length.vdr umstellen
        buffer=filename;
#if VDRVERSNUM > 10703
        if (isPesRecording)
          buffer+="/index.vdr";
        else
          buffer+="/index";
#else
        buffer+="/index.vdr";
#endif
        struct stat statbuf;
        if(!stat(buffer.c_str(),&statbuf))
        {
          ostringstream strbuf;
#if APIVERSNUM < 10700
          strbuf << setw(3) << setfill(' ') << right << (int)(statbuf.st_size/12000) << "'";
#else
          strbuf << setw(3) << (int)(statbuf.st_size/480/Recording->FramesPerSecond()) << "'";
#endif
#if 0
          titlebuffer << myStrReplace(strbuf.str(),' ',char(131)) << '\t'; // TB: Freetype doesn't like this char
#else
          titlebuffer << strbuf.str() << '\t';
#endif
        }
        else
        {
          // get recording length from file 'length.vdr'
          buffer=filename;
          buffer+="/length.vdr";

          ifstream in(buffer.c_str());
          if(in)
          {
            if(!in.eof())
              getline(in,buffer);

            buffer+="'";

            while(buffer.length()<=3)
#if 0
              buffer.insert(0,1, char(131));
#else
              buffer.insert(0,1, ' ');
#endif
            titlebuffer << buffer << '\t';

            in.close();
          }
          else
            titlebuffer << '\t';
        }
      }
      if(!mysetup.ShowRecDate && !mysetup.ShowRecTime && !mysetup.ShowRecLength)
        titlebuffer << '\t';

#if 0 // no longer need here; shown in the first column of icons
   if(Recording->IsHD()) {
    titlebuffer << char(134);
     titlebuffer << ' ';
   }
#endif
      // recording title

   if(myMenuRecordings::infotitle && Recording->Info()->Title()) {
     if(Recording->Info()->ShortText()
	     && Parent && strcmp(Recording->Info()->Title(), Parent)==0) {
       titlebuffer << Recording->Info()->ShortText();
       idbuffer << Recording->Info()->ShortText();
     }
     else {
       titlebuffer << Recording->Info()->Title();
       idbuffer << Recording->Info()->Title();
       if (Recording->Info()->ShortText()) {
         titlebuffer << std::string(" - ");
         idbuffer << std::string(" - ");
         titlebuffer << Recording->Info()->ShortText();
         idbuffer << Recording->Info()->ShortText();
       }
     }
   }
   else {
//      isyslog("no RecordingInfo, using Recording     Name: %s", Recording->Name());
//      isyslog("no RecordingInfo, using Recording FileName: %s", Recording->FileName());

      string s=Recording->Name();
      string::size_type i=s.rfind('~');
      if(i!=string::npos)
        s=s.substr(i+1,s.length()-i);
      while (!s.find('%'))  /// first char=% -> find=0 -> false
        s=s.substr(1,s.length()-1);
      titlebuffer << s;
      idbuffer << s;
   }
//   isyslog("no RecordingInfo, using Recording     Name: %s", Recording->Name());
//   isyslog("no RecordingInfo, using Recording FileName: %s", Recording->FileName());
      title=strdup(titlebuffer.str().c_str());
      idbuffer << Recording->FileName();
      uniqid=idbuffer.str();
    }
    else
    {
      if(Level>level) // any other
        title=strdup("");
    }
  }
  SetText(title);
}

myMenuRecordingsItem::~myMenuRecordingsItem()
{
  free(title);
  free(name);
}

#if REELVDR
void myMenuRecordingsItem::AddRecordingToDir(cRecording* recording) {

#if APIVERSNUM < 10721
    if (recording && latestRecordingStartTime < recording->start)
        latestRecordingStartTime = recording->start;
#else
    if (recording && latestRecordingStartTime < recording->Start())
        latestRecordingStartTime = recording->Start();
#endif

    IncrementCounter(recording->IsNew());
}

time_t myMenuRecordingsItem::LatestRecordingStartTime() const {
    return latestRecordingStartTime;
}

bool myMenuRecordingsItem::SetProgress(std::string processName, int percentage)
{
      //printf("myMenuRecordingsItem::SetProgress\n");
      if(lastPercentage != percentage)
      {
	lastPercentage= percentage;
	if(percentage < 100)
	{

	  std::string processDesc = /*std::string(Parent) +*/ GetRecording()->Name();
	  SetText(GetBgProcessString(GetRecording()->Name() , processName, percentage).c_str());
	  SetFresh(true);
	}
	else
	{
	  SetText(title);
	  SetFresh(true);
	}
	return true;
      }
      return false;
}

std::string myMenuRecordingsItem::GetBgProcessString(std::string processDesc, std::string processName, int percentage)
{
    std::ostringstream oss;
    char per_str[100 + 3];
    per_str[102] = '\0';
    per_str[0] = '[';
    per_str[101] = ']';
    for (int j = 0; j < 100; j++)
	per_str[1 + j] = ' ';
    for (int j = 0; j < percentage; j++)
	per_str[1 + j] = '|';

    // calc hour and minute
    //struct tm *time_tm = localtime(&iter->second.startTime);

    oss  << "\t"  << "\t" << per_str << "\t"  << processDesc;
    return oss.str();
}

#endif // REELVDR

void myMenuRecordingsItem::IncrementCounter(bool IsNew)
{
  totalentries++;
  if(IsNew)
    newentries++;

  char *buffer=NULL;
  char entries[8];
  snprintf(entries,sizeof(entries),"%3d",totalentries);
#if 0
  strreplace(entries,' ',char(131));
#endif

  if(mysetup.ShowNewRecs)
  {
    asprintf(&buffer, "%c\t%c\t%s/%d%s%s%s%s%s",
                     GetDirIsMoving()?char(134):char(130),
                     isProtected ? char(80) : ' ',
                     entries,
                     newentries,
                     (!mysetup.ShowRecDate&&!mysetup.ShowRecTime&&!mysetup.ShowRecLength)?"\t":"",
                     (mysetup.ShowRecDate||mysetup.ShowRecTime||mysetup.ShowRecLength)?"\t":"",
                     ((mysetup.ShowRecDate&&mysetup.ShowRecTime)||(mysetup.ShowRecTime&&mysetup.ShowRecLength)||(mysetup.ShowRecLength&&mysetup.ShowRecDate))?"\t":"",
                     (mysetup.ShowRecDate&&mysetup.ShowRecTime&&mysetup.ShowRecLength)?"\t":"",
                     name);
  }
  else
  {
    asprintf(&buffer, "%c\t%c\t%s%s%s%s%s%s",
                     GetDirIsMoving()?char(134):char(130),
                     isProtected ? char(80) : ' ',
                     entries,
                     (!mysetup.ShowRecDate&&!mysetup.ShowRecTime&&!mysetup.ShowRecLength)?"\t":"",
                     (mysetup.ShowRecDate||mysetup.ShowRecTime||mysetup.ShowRecLength)?"\t":"",
                     ((mysetup.ShowRecDate&&mysetup.ShowRecTime)||(mysetup.ShowRecTime&&mysetup.ShowRecLength)||(mysetup.ShowRecLength&&mysetup.ShowRecDate))?"\t":"",
                     (mysetup.ShowRecDate&&mysetup.ShowRecTime&&mysetup.ShowRecLength)?"\t":"",
                     name);
  }
  SetText(buffer,false);
}

// --- myMenuRecordings -------------------------------------------------------
#define MB_PER_MINUTE 25.75 // this is just an estimate!

bool myMenuRecordings::golastreplayed=false;
bool myMenuRecordings::wasdvd;
bool myMenuRecordings::infotitle=true; //Behni
dev_t myMenuRecordings::fsid=0;
int myMenuRecordings::freediskspace=0;

myMenuRecordings::myMenuRecordings(const char *Base,const char *Parent,int Level):cOsdMenu("")
{
 //printf("+++++Base = %s Parent = %s++++\n", Base, Parent);
#if !defined(REELVDR) && VDRVERSNUM < 10503
  // only called if plugin menu was opened
  if(Level==0 && mysetup.PatchFont)
  {
    // patch font
    if(Setup.UseSmallFont==2)
       PatchFont(fontSml);
    else
       PatchFont(fontOsd);
  }
#endif
  // GA: immediate visual feedback
  if (!Level)
  {
    cOsdMenu::SetTitle(tr("Opening Recordings..."));
    cOsdMenu::Display();
    Skins.Flush();
  }

  // For PIN protection
  isPinValid = false;
  getPin = false;
  nextKey = kNone;
  replayDVD = cPluginExtrecmenu::replayArchiveDVD;

#if !defined(REELVDR) && VDRVERSNUM < 10503
 // only called if plugin menu is opened
 if(Level==0)
 {
  // patch font
  if(!strcmp(Setup.OSDSkin, "Reel") || Setup.UseSmallFont==2)
   PatchFont(fontSml);
  else
   PatchFont(fontOsd);
 }
#endif

#if REELVDR && APIVERSNUM >= 10718
 mysetup.ShowRecLength = 0;
#endif /* REELVDR && APIVERSNUM >= 10718 */

#if APIVERSNUM < 10700
   // set tabs
   if(mysetup.ShowRecDate&&mysetup.ShowRecTime&&!mysetup.ShowRecLength) // recording date and time are shown, recording length not
     SetCols(2,2,8,6);
   else if(mysetup.ShowRecDate&&mysetup.ShowRecTime) // all details are shown
     SetCols(2,2,8,6,4);
   else if(mysetup.ShowRecDate&&!mysetup.ShowRecTime) // recording time is not shown
     SetCols(2,2,8,4);
   else if(!mysetup.ShowRecDate&&mysetup.ShowRecTime&&mysetup.ShowRecLength) // recording date is not shown
     SetCols(2,2,6,4);
   else // recording date and time are not shown; even if recording length should be not shown we must set two tabs because the details of the directories
   {
     if(mysetup.ShowNewRecs)
       SetCols(2,2,8,3);
     else
       SetCols(2,2,4,3);
   }
#else
   // set tabs
   if(mysetup.ShowRecDate&&mysetup.ShowRecTime&&!mysetup.ShowRecLength) // recording date and time are shown, recording length not
       SetCols(2,2,9,6);
   else if(mysetup.ShowRecDate&&mysetup.ShowRecTime) // all details are shown
       SetCols(2,2,9,6,4);
   else if(mysetup.ShowRecDate&&!mysetup.ShowRecTime) // recording time is not shown
       SetCols(2,2,9,4);
   else if(!mysetup.ShowRecDate&&mysetup.ShowRecTime&&mysetup.ShowRecLength) // recording date is not shown
       SetCols(2,2,6,4);
   else // recording date and time are not shown; even if recording length should be not shown we must set two tabs because the details of the directories
   {
       if(mysetup.ShowNewRecs)
           SetCols(2,2,9,3);
       else
           SetCols(2,2,4,3);
   }
#endif

  edit=false;
  parent=Parent;
  level=Level;
  helpkeys=-1;
  base=Base?strdup(Base):NULL;

  Recordings.StateChanged(recordingsstate);

#if REELVDR && APIVERSNUM >= 10718
  EnableSideNote(true);
#endif
  golastreplayed=mysetup.GoLastReplayed && myReplayControl::LastReplayed();
  Display(); //TB: don't do this twice - gets called from vdr.c, line 1316 via menu->Show()

  if(0 /*wasdvd&&!cControl::Control()*/) //TB: hack to make archive-dvds work again
  {
    char *cmd=NULL;
    asprintf(&cmd,"dvdarchive.sh umount \"%s\"",*strescape(myReplayControl::LastReplayed(),"'\\\"$"));
    isyslog("[extrecmenu] calling %s to unmount dvd",cmd);
    int result=SystemExec(cmd);
    if(result)
    {
      result=result/256;
      if(result==1)
        Skins.Message(mtError,tr("Error while mounting DVD!"));
    }
    isyslog("[extrecmenu] dvdarchive.sh returns %d",result);
    free(cmd);

    wasdvd=false;
  }

  Set();

  //if(myReplayControl::LastReplayed())
    Open();

  SetHelpKeys();

  // Flush queued keys to avoid unwanted actions
  // FIXME: Move to common functions
  for(int i=0;i<100;i++) {
    eKeys k=Interface->GetKey(0);
    if (k==kNone)
      break;
  }

}

myMenuRecordings::~myMenuRecordings()
{
  free(base);
#if VDRVERSNUM < 10503 && !defined(REELVDR)
  if(level==0)
  {
    if(Setup.UseSmallFont==2)
      cFont::SetFont(fontSml);
    else
      cFont::SetFont(fontOsd);
  }
#endif

 if (cPluginExtrecmenu::replayArchiveDVD && level == 1 && !myReplayControl::active)
 {
    cPluginExtrecmenu::replayArchiveDVD = false;
    //system("mount.sh unmount"); //TB: hack to make archive-DVDs work again
    //Recordings.Load();
  }

}

int myMenuRecordings::FreeMB()
{
  if(mysetup.FileSystemFreeMB)
  {
    string path=VideoDirectory;
    path+="/";
    char *tmpbase=base?ExchangeChars(strdup(base),true):NULL;
    if(base)
      path+=tmpbase;
    /*check for file also using old ExchangeChars() encoding, called OldExchangeChars()*/
#if APIVERSNUM < 10700
    string path_old=VideoDirectory;
    path_old += "/";
	if (base)
	{
		if (tmpbase) free(tmpbase);
		tmpbase = OldExchangeChars(strdup(base),true);
		path_old += tmpbase;
	}
#endif

    struct stat statdir;
#if APIVERSNUM < 10700
    if(!stat(path.c_str(),&statdir) || !stat(path_old.c_str(), &statdir) )
#else
    if(!stat(path.c_str(),&statdir))
#endif
    {
      if(statdir.st_dev!=fsid)
      {
        fsid=statdir.st_dev;

        struct statvfs fsstat;
#if APIVERSNUM < 10700
        if(!statvfs(path.c_str(),&fsstat) || !statvfs(path_old.c_str(), &fsstat) )
#else
        if(!statvfs(path.c_str(),&fsstat))
#endif
        {
          freediskspace=int(fsstat.f_bavail/(1024.0*1024.0/fsstat.f_bsize));

          for(cRecording *rec=DeletedRecordings.First();rec;rec=DeletedRecordings.Next(rec))
          {
            if(!stat(rec->FileName(),&statdir))
            {
              if(statdir.st_dev==fsid)
                freediskspace+=DirSizeMB(rec->FileName());
            }
          }
        }
        else
        {
          esyslog("[extrecmenu] error while getting filesystem size - statvfs (%s): %s",path.c_str(),strerror(errno));
          freediskspace=0;
	  fsid = 0; // so that FreeMB() is calculated the next time
        }
      }
    }
    else
    {
      esyslog("[extrecmenu] error while getting filesystem size - stat (%s): %s",path.c_str(),strerror(errno));
      freediskspace=0;
      fsid = 0; // so that FreeMB() is calculated the next time
    }
    free(tmpbase);
  }
  else
  {
    int freemb;
    VideoDiskSpace(&freemb);
    return freemb;
  }

  return freediskspace;
}

void myMenuRecordings::Title()
{
  int freemb=FreeMB();
  int minutes=int(double(freemb)/MB_PER_MINUTE);

  stringstream buffer;
/*  if(MoveCutterThread->IsMoveListEmpty())
    buffer << char(133);
  if(MoveCutterThread->IsCutterQueueEmpty())
    buffer << char(132);
 */
  if(MoveCutterThread->IsMoveListEmpty() || MoveCutterThread->IsCutterQueueEmpty())
    buffer << ' ';

  if(base)
    buffer << base;
  else
    buffer << tr("Recordings");

  buffer << " ("
         << minutes/60 << ":"
         << setw(2) << setfill('0') << minutes%60 << "h "
         << tr("free")
         << ")";

  cOsdMenu::SetTitle(buffer.str().c_str());
}

void myMenuRecordings::SetFreeSpaceTitle()
{
 int freemb;
 VideoDiskSpace(&freemb);
 int minutes=int(double(freemb)/MB_PER_MINUTE);
 char buffer[40];
 snprintf(buffer,sizeof(buffer),"%s - %2d:%02dh %s",tr("Recordings"),minutes/60,minutes%60,tr("free"));
 cOsdMenu::SetTitle(buffer);
}


void myMenuRecordings::SetHelpKeys()
{
  if(!HasSubMenu())
  {
    bool newrecording = false;
    static bool last_newrecording = false;
    myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
    if (!item) item = (myMenuRecordingsItem*)Get(0); //TB: at the first call Current() is -1
    int newhelpkeys=0;
    if(item)
    {
      if(item->IsDirectory())
      {
        if(item->GetDirIsMoving())
          newhelpkeys=6;
        else
        {;
          newhelpkeys=1;
        }
      }
      else
      {
        newhelpkeys=2;
        cRecording *recording=GetRecording(item);
        if(recording)
        {
          newrecording = recording->IsNew();
          if(mysetup.UseVDRsRecInfoMenu)
          {
            if(!recording->Info()->Title())
            {
              newhelpkeys=4;
              if(MoveCutterThread->IsMoving(recording->FileName()) || MoveCutterThread->IsCutting(recording->FileName()))
                newhelpkeys=5;
            }
          }
          if(MoveCutterThread->IsMoving(recording->FileName()) || MoveCutterThread->IsCutting(recording->FileName()))
            newhelpkeys=3;

          if (recording->Info()->Title())
             newhelpkeys = 3; // imitating old extrecmenu (0.12-reel3)
             /// XXX TODO handle cutter / move threads in set help and process keys
        }
      }
      if(newhelpkeys!=helpkeys || last_newrecording != newrecording)
      {
        last_newrecording = newrecording;
        static eKeys ikeys[] = { kInfo, kNone };
        switch(newhelpkeys)
        {
          /*case 0: SetHelp(NULL);break;
          case 1: SetHelp(tr("Button$Open"),NULL,tr("Button$Edit"));break;
          case 2: SetHelp(RecordingCommands.Count()?tr("Button$Functions"):tr("Button$Play"),tr("Button$Rewind"),tr("Button$Edit"),tr("Button$Info"));break;
          case 3: SetHelp(RecordingCommands.Count()?tr("Button$Functions"):tr("Button$Play"),tr("Button$Rewind"),tr("Button$Cancel"),tr("Button$Info"));break;
          case 4: SetHelp(RecordingCommands.Count()?tr("Button$Functions"):tr("Button$Play"),tr("Button$Rewind"),tr("Button$Edit"),NULL);break;
          case 5: SetHelp(RecordingCommands.Count()?tr("Button$Functions"):tr("Button$Play"),tr("Button$Rewind"),tr("Button$Cancel"),NULL);break;
          case 6: SetHelp(tr("Button$Open"),NULL,tr("Button$Cancel"));break;
          */

          /*
          // XXX old "Edit" version
             case 0: SetHelp(NULL);break;
             //case 1: SetHelp(replayDVD ? NULL : tr("Button$Edit"), NULL, NULL, tr("Button$Open")); break;
             case 1: SetHelp(replayDVD ? NULL : tr("Button$Edit"), NULL, NULL, tr("Button$Functions")); break;
             case 2: SetHelp(replayDVD ? NULL : tr("Button$Edit"), tr("Button$Rewind"), NULL, RecordingCommands.Count() && !replayDVD ? tr("Button$Functions") : tr("Button$Play")); break;
             case 3: SetHelp(replayDVD ? NULL : tr("Button$Edit"), tr("Button$Rewind"), tr("Button$Info"), RecordingCommands.Count() && !replayDVD ? tr("Button$Functions") : tr("Button$Play")); break;
             case 4:SetHelp(replayDVD ? NULL : tr("Button$Edit"), tr("Button$Rewind"), NULL, RecordingCommands.Count() && !replayDVD ? tr("Button$Functions") : tr("Button$Play")); break;
          */

           //printf("-----------  newrecording = %d-------------\n", newrecording);
           //printf("-----------  newhelpkeys = %d-------------\n", newhelpkeys);


          // XXX USE these
             case 0: SetHelp(NULL);break;
             //case 1: SetHelp(replayDVD ? NULL : tr("Button$Edit"), NULL, NULL, tr("Button$Open")); break;
             case 1: SetHelp((replayDVD || !::Setup.ExpertNavi) ? NULL : trVDR("Button$Delete"), NULL, NULL, tr("Button$Functions")); break;
             case 2: SetHelp((replayDVD || !::Setup.ExpertNavi) ? NULL : trVDR("Button$Delete"), newrecording ? NULL : trVDR("Button$Rewind"), NULL, RecordingCommands.Count() && !replayDVD ? trVDR("Button$Functions") : trVDR("Button$Play")); break;
#if REELVDR && APIVERSNUM >= 10718
             case 3: SetHelp((replayDVD || !::Setup.ExpertNavi) ? NULL : trVDR("Button$Delete"), newrecording ? NULL : trVDR("Button$Rewind"), NULL, RecordingCommands.Count() && !replayDVD ? trVDR("Button$Functions") : trVDR("Button$Play"), ikeys); break;
#else
             case 3: SetHelp((replayDVD || !::Setup.ExpertNavi) ? NULL : trVDR("Button$Delete"), newrecording ? NULL : trVDR("Button$Rewind"), trVDR("Button$Info"), RecordingCommands.Count() && !replayDVD ? trVDR("Button$Functions") : trVDR("Button$Play")); break;
#endif
             case 4: SetHelp((replayDVD || !::Setup.ExpertNavi) ? NULL : trVDR("Button$Delete"), newrecording ? NULL : trVDR("Button$Rewind"), NULL, RecordingCommands.Count() && !replayDVD ? trVDR("Button$Functions") : trVDR("Button$Play")); break;
             default:
             	//printf("Cannot handle yet %i newhelpkeys (%s:%i)\n", newhelpkeys, __FILE__, __LINE__);
             	break;

        }
      }
      helpkeys=newhelpkeys;
    }
  }
}

// create the menu list
void myMenuRecordings::Set(bool Refresh,char *current)
{
  const char *lastreplayed=current?current:myReplayControl::LastReplayed();

  cThreadLock RecordingsLock(&Recordings);

  if(Refresh && !current)
  {
    fsid=0;
    myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
    if(item)
    {
     cRecording *recording=Recordings.GetByName(item->FileName());
      if(recording)
        lastreplayed=recording->FileName();
    }
  }

  Clear();

  // create my own recordings list from VDR's
  myRecList *list=new myRecList();
  for(cRecording *recording=Recordings.First();recording;recording=Recordings.Next(recording))
  {
    list->Add(new myRecListItem(recording));
  }
  // sort my recordings list
  string path=VideoDirectory;
  path+="/";
  if(base)
    path+=base;
  list->Sort(mySortList->Find(path));

  char *lastitemtext=NULL;
  myMenuRecordingsItem *lastitem=NULL;
  for(myRecListItem *listitem=list->First();listitem;listitem=list->Next(listitem))
  {
    cRecording *recording=listitem->recording;
    if(!base||(strstr(listitem->recording->Name(),base)==listitem->recording->Name()&&listitem->recording->Name()[strlen(base)]=='~'))
    {
      myMenuRecordingsItem *recitem=new myMenuRecordingsItem(listitem->recording,parent,level);
#ifdef WITHPINPLUGIN
      bool hidepinprotectedrecs=false;
      cPlugin *pinplugin=cPluginManager::GetPlugin("pin");
      if(pinplugin)
        hidepinprotectedrecs=pinplugin->SetupParse("hideProtectedRecordings","1");

      if((*recitem->UniqID() && ((!lastitem || strcmp(recitem->UniqID(),lastitemtext)))) &&
         !(cStatus::MsgReplayProtected(GetRecording(recitem),recitem->Name(),base,recitem->IsDirectory())==true && hidepinprotectedrecs))
#else
      if(*recitem->UniqID() && ((!lastitem || strcmp(recitem->UniqID(),lastitemtext))))
#endif
      {
        /*int progress = GetBgProcess(listitem->recording->Name());
        if(progress < 100)
        {
            ShowBgProcess(listitem->recording->Name(), progress);
        }
        else
        {*/
            Add(recitem);
        //}
        lastitem=recitem;
        free(lastitemtext);
        lastitemtext=strdup(lastitem->UniqID());
      }
      else
        delete recitem;

      if(lastitem)
      {
        if(!MoveCutterThread->IsMoving(recording->FileName()))
          lastitem->SetDirIsMoving(false);

        if(lastitem->IsDirectory())
        {
#if  REELVDR
          // 'recording' is available in this directory (or its subdirs)
          // also increments count
          lastitem->AddRecordingToDir(recording);
#else
          lastitem->IncrementCounter(recording->IsNew());
#endif
        }
        if(lastreplayed && !strcmp(lastreplayed,recording->FileName()))
        {
          if(golastreplayed||Refresh)
          {
            SetCurrent(lastitem);
            if(recitem && !recitem->IsDirectory() && !cControl::Control() && !mysetup.GoLastReplayed)
              golastreplayed=false;
          }
          if(recitem&&!recitem->IsDirectory()&&recitem->IsDVD()&&!cControl::Control())
            cReplayControl::ClearLastReplayed(cReplayControl::LastReplayed());
        }
      }
    }
  }
  free(lastitemtext);
  delete list;

  Title();
  if(Refresh)
    Display();
}

void myMenuRecordings::Display() {

    cOsdMenu::Display();
    ShowRecordingDetailsInSideNote();
} // Display()


void myMenuRecordings::ShowRecordingDetailsInSideNote()
{
    // show Side note if applicable
    myMenuRecordingsItem *item= dynamic_cast<myMenuRecordingsItem*> (Get(Current()));

#if REELVDR && APIVERSNUM >= 10718

    bool hasSubMenu = HasSubMenu();

    if (!hasSubMenu &&  item) {


        // if item is recording, get the cRecording object
        if (!item->IsDirectory())
        {
            cRecording *rec = new cRecording(item->FileName());

            // if rec == NULL, Side note is cleared
            SideNote(rec);
            SideNoteIcons(RecordingsIcons(rec).c_str());

            delete rec;

        } // if
        else
        {
//#define USE_SNPRINTF 1 //RC: currently crashes, maybe str_list.Append() ?
#if USE_SNPRINTF
            char folder[25];
            char totalEntriesStr[12];
            char newEntriesStr[12];
#else
            char *totalEntriesStr, *newEntriesStr, *folder;
#endif
            char *str;

#if USE_SNPRINTF
            snprintf(folder, sizeof(folder), "FOLDER ICON");
            snprintf(totalEntriesStr, sizeof(totalEntriesStr), "%s: %d", tr("Total"), item->TotalEntries());
            snprintf(newEntriesStr,   sizeof(newEntriesStr), "%s: %d", tr("New"), item->NewEntries());
#else
            asprintf(&folder, "FOLDER ICON");
            asprintf(&totalEntriesStr, "%s: %d", tr("Total"), item->TotalEntries());
            asprintf(&newEntriesStr, "%s: %d", tr("New"), item->NewEntries());
#endif

            cStringList str_list;
            str_list.Append(folder);

            //folder name
#if USE_SNPRINTF
            snprintf(folder, sizeof(folder), "FOLDER NAME:%s", item->Name());
#else
            asprintf(&folder, "FOLDER NAME:%s", item->Name());
#endif
            str_list.Append(folder);

            //empty lines
            for (int i=0; i < 2; ++i)
            {
                asprintf(&str, "");
                str_list.Append(str);
            }

            asprintf(&str, tr("Recordings"));
            str_list.Append(str);
            str_list.Append(totalEntriesStr);
            str_list.Append(newEntriesStr);

            time_t start_time = item->LatestRecordingStartTime();

            if (start_time > 0)
            {
                //empty lines
                for (int i=0; i < 3; ++i)
                {
                    asprintf(&str, "");
                    str_list.Append(str);
                }

                asprintf(&str, "%s:", tr("Last recording"));
                str_list.Append(str);

                asprintf(&str, "%s", *DateString(start_time));
                str_list.Append(str);
            } // if

            SideNote(&str_list);
        } // else


    } // if !hasSubMenu && item

#endif /* REELVDR  && APIVERSNUM >= 10718 */

} // ShowRecordingDetailsInSideNote()

// returns the corresponding recording to an item
cRecording *myMenuRecordings::GetRecording(myMenuRecordingsItem *Item)
{
 cRecording *recording=Recordings.GetByName(Item->FileName());
 if(!recording)
  Skins.Message(mtError,tr("Error while accessing recording!"));
 return recording;
}

// opens a subdirectory
bool myMenuRecordings::Open()
{
 myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
 if(item&&item->IsDirectory())
 {
  const char *t=item->Name();
  char *buffer=NULL;
  if(base)
  {
   asprintf(&buffer,"%s~%s",base,t);
   t=buffer;
  }
  AddSubMenu(new myMenuRecordings(t,item->Name(),level+1));
  free(buffer);
  return true;
 }

 return false;
}

// plays a recording
eOSState myMenuRecordings::Play()
{
  char *msg=NULL;
  char *name=NULL;

  char path[MaxFileName];

  myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
  if(item)
  {
#ifdef WITHPINPLUGIN
    if(cStatus::MsgReplayProtected(GetRecording(item),item->Name(),base,item->IsDirectory())==true)
      {
  if (!isPinValid)
  {
   getPin = true;
   nextKey = kOk;
   SetStatus(tr("PIN code: ") ); // to avoid delay in display, as ProcessKey() is called once every ~1s
   return osContinue;
   }
   else isPinValid = false;
  }

#endif
    if(item->IsDirectory())
      Open();
    else
    {
      cRecording *recording=GetRecording(item);
      if(recording)
      {
        if(item->IsDVD())
        {
          bool isvideodvd=false;
          char dvdnr[BUFSIZ];
          char *buffer=NULL;
          FILE *f;

          asprintf(&buffer,"%s/dvd.vdr",recording->FileName());
          if((f=fopen(buffer,"r"))!=NULL)
          {
            // get the dvd id
            if(fgets(dvdnr,sizeof(dvdnr),f))
            {
              char *p=strchr(dvdnr,'\n');
              if(p)
                *p=0;
            }
            // determine if dvd is a video dvd
            char tmp[BUFSIZ];
            if(fgets(tmp,sizeof(dvdnr),f))
            isvideodvd=true;

            fclose(f);
          }
          free(buffer);

          asprintf(&msg,tr("Please insert DVD %s"),dvdnr);
          if(Interface->Confirm(msg))
          {
            free(msg);
            // recording is a video dvd
            if(isvideodvd)
            {
#ifndef RBLITE
        cPlugin *plugin=cPluginManager::GetPlugin("mediad");
#else
        cPlugin *plugin=cPluginManager::GetPlugin("vdrcd");
#endif

              if(plugin)
              {
                cOsdObject *osd=plugin->MainMenuAction();
                delete osd;
                osd=NULL;
                return osEnd;
              }
              else
              {
                Skins.Message(mtError,tr("DVD plugin is not installed!"));
                return osContinue;
              }
            }
            // recording is an archive dvd
            else
            {
              strcpy(path,recording->FileName());
              name=strrchr(path,'/')+1;
              asprintf(&msg,"dvdarchive.sh mount \"%s\" '%s'",*strescape(path,"'"),*strescape(name,"'\\\"$"));

              isyslog("[extrecmenu] calling %s to mount dvd",msg);
              int result=SystemExec(msg);
              isyslog("[extrecmenu] dvdarchive.sh returns %d",result);
              free(msg);
              if(result)
              {
                result=result/256;
                if(result==1)
                  Skins.Message(mtError,tr("Error while mounting DVD!"));
                else if(result==2)
                  Skins.Message(mtError,tr("No DVD in drive!"));
                else if(result==3)
                  Skins.Message(mtError,tr("Recording not found on DVD!"));
                else if(result==4)
                  Skins.Message(mtError,tr("Error while linking [0-9]*.vdr!"));
                else if(result==5)
                  Skins.Message(mtError,tr("sudo or mount --bind / umount error (vfat system)"));
                else if(result==127)
                  Skins.Message(mtError,tr("Script 'dvdarchive.sh' not found!"));
                return osContinue;
              }
              wasdvd=true;
            }
          }
          else
          {
            free(msg);
            return osContinue;
          }
        }
        golastreplayed=true;
// Behni revert r11143:
//        const cRecordingInfo *info = recording->Info();
//        if(info && info->Title())
//                myReplayControl::SetRecording(recording->FileName(),info->Title());
//         else
// Behni revert r11143
#ifdef REELVDR
#if VDRVERSNUM < 10728
        myReplayControl::SetRecording(recording->FileName(),recording->Name());
#else
        myReplayControl::SetRecording(recording->FileName());
#endif
        printf("PLAYING '%s' (%s) \n", recording->FileName(),recording->Name());
#else
        myReplayControl::SetRecording(recording->FileName(),recording->Title());
        printf("PLAYING '%s' (%s) \n", recording->FileName(),recording->Title());
#endif
        cControl::Shutdown();
        isyslog("[extrecmenu] starting replay of recording");
        cControl::Launch(new myReplayControl());
        return osEnd;
      }
    }
  }
  return osContinue;
}

// plays a recording from the beginning
eOSState myMenuRecordings::Rewind()
{
 if(HasSubMenu()||Count()==0)
  return osContinue;

 myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
 if(item&&!item->IsDirectory())
 {
  cDevice::PrimaryDevice()->StopReplay();
#if VDRVERSNUM >= 10703
  cResumeFile ResumeFile(item->FileName(), item->IsPesRecording());
#else
  cResumeFile ResumeFile(item->FileName());
#endif
  ResumeFile.Delete();
  return Play();
 }
 return osContinue;
}

void myMenuRecordings::GetAllRecordingsInDir(std::vector<cRecording *> &recordings, std::string base)
{
 for(cRecording *recording=Recordings.First();recording;recording=Recordings.Next(recording))
 {
    std::string name = recording->Name();
    if(name.compare(0, base.size(), base) == 0)
    {
        recordings.push_back(recording);
    }
 }
}

eOSState myMenuRecordings::DeleteDirectory()
{
  if(Interface->Confirm(tr("Delete directory?")))
  {
    myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
    std::vector<cRecording *> recordings;
    std::string dir = item->Name();
    if(base)
    {
        dir = std::string(base) + "~" + item->Name();
    }
    dir += "~"; // only delete this folder and not all starting with folder name
    GetAllRecordingsInDir(recordings, dir);
    for(int i = 0; i<recordings.size(); ++i)
    {
        //printf("delete %s\n",recordings[i]->Name());
        Delete(recordings[i]);
    }

    cOsdMenu::Del(Current());
    SetHelpKeys();
    Display();

    if(!Count())
    {
        return osBack;
    }
  }
  return osContinue;
}

void myMenuRecordings::Delete(cRecording *recording)
{
    if(!recording)
    {
      return;
    }
    cRecordControl *rc=cRecordControls::GetRecordControl(recording->FileName());
    if(rc)
    {
        if(Interface->Confirm(tr("Timer still recording - really delete?")))
        {
            cTimer *timer=rc->Timer();
            if(timer)
            {
                timer->Skip();
                cRecordControls::Process(time(NULL));
                if(timer->IsSingleEvent())
                {
                    isyslog("deleting timer %s",*timer->ToDescr());
                    if(!Timers.Del(timer))
                    Skins.Message(mtError, trVDR("Couldn't add timer"));
                }
                Timers.SetModified();
            }
        }
        else
        {
            return;
        }
    }
    if(recording->Delete())
    {
        cRecordingUserCommand::InvokeCommand("delete",recording->FileName());
        myReplayControl::ClearLastReplayed(recording->FileName());
        Recordings.DelByName(recording->FileName());
    }
    else
    {
        Skins.Message(mtError,tr("Error while deleting recording!"));
    }
}

// delete a recording
eOSState myMenuRecordings::Delete()
{
  if(HasSubMenu()||Count()==0)
    return osContinue;

  myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
  if(item && item->IsDirectory())
  {
    DeleteDirectory();
  }
  else
  {
    if(Interface->Confirm(tr("Delete recording?")))
    {
      cRecordControl *rc=cRecordControls::GetRecordControl(item->FileName());
      if(rc)
      {
        if(Interface->Confirm(tr("Timer still recording - really delete?")))
        {
          cTimer *timer=rc->Timer();
          if(timer)
          {
            timer->Skip();
            cRecordControls::Process(time(NULL));
            if(timer->IsSingleEvent())
            {
              isyslog("deleting timer %s",*timer->ToDescr());
              if(!Timers.Del(timer))
                Skins.Message(mtError, trVDR("Couldn't add timer"));
            }
            Timers.SetModified();
          }
        }
        else
          return osContinue;
      }
      cRecording *recording=GetRecording(item);
      if(recording)
      {
        if(recording->Delete())
        {
          cRecordingUserCommand::InvokeCommand("delete",item->FileName());
          myReplayControl::ClearLastReplayed(item->FileName());
          Recordings.DelByName(item->FileName());
          cOsdMenu::Del(Current());
          SetHelpKeys();
          Display();
          if(!Count())
            return osBack;
        }
        else
          Skins.Message(mtError,tr("Error while deleting recording!"));
      }
    }
  }
  return osContinue;
}
// change title of a recording
eOSState myMenuRecordings::SetTitle()
{
 if(HasSubMenu()||Count()==0)
  return osContinue;

 myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
 if(item)
 {
  if(!item->IsDirectory())
  {
   cRecording *recording=GetRecording(item);
   if(recording)
    return AddSubMenu(new myMenuRecordingTitle(this,recording));
  }
 }
 return osContinue;
}



// renames a recording
eOSState myMenuRecordings::Rename()
{
 if(HasSubMenu()||Count()==0)
  return osContinue;

 myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
 if(item)
 {
  if(item->IsDirectory())
   return AddSubMenu(new myMenuRenameRecording(NULL,base,item->Name()));
  else
  {
   cRecording *recording=GetRecording(item);
   if(recording)
    return AddSubMenu(new myMenuRenameRecording(recording,NULL,NULL));
  }
 }
 return osContinue;
}

// edit details of a recording
eOSState myMenuRecordings::Details()
{
 if(HasSubMenu()||Count()==0)
  return osContinue;

 myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
 if(item&&!item->IsDirectory())
 {
  cRecording *recording=GetRecording(item);
#if VDRVERSNUM >= 10703
    if(recording && recording->IsPesRecording())
#else
  if(recording)
#endif
   return AddSubMenu(new myMenuRecordingDetails(recording));
 }
 return osContinue;
}


// mark Recordingas new
eOSState myMenuRecordings::MarkAsNew()
{
 myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
 if(item && !item->IsDirectory())
 {
  cRecording *recording=GetRecording(item);
#if APIVERSNUM > 10700
  recording->ResetResumeToNew();
#endif
 }
 return osContinue;
}



// moves a recording
eOSState myMenuRecordings::MoveRec()
{
  if(HasSubMenu() || Count()==0)
    return osContinue;

  myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
  if(item)
  {
    myMenuMoveRecording::clearall=false;
    if(item->IsDirectory())
    {
        /*printf("%s/", VideoDirectory);
        if (base) printf("%s/",base);
        if (item->Name()) printf("%s/", item->Name());
        printf("\n");*/
        char buffer[256];
        if (base && item->Name())
        snprintf(buffer,255, "%s/%s/%s", VideoDirectory,base, item->Name());
        else if (base && !item->Name())
        snprintf(buffer,255, "%s/%s", VideoDirectory,base);
        else if (!base && item->Name())
        snprintf(buffer,255, "%s/%s", VideoDirectory, item->Name());
        //return AddSubMenu(new myMenuMoveRecording(NULL,base,item->Name()));
        char *s = strdup(buffer);
        s = ExchangeChars(s,false);
        s = ExchangeChars(s,true);
        strncpy(buffer, s, 255);
        buffer[255] =0;
        delete s;

        if ( ExtMediaCount() > 0 ) // external media available
            return AddSubMenu(new cSelectMediaMenu(buffer));
        else
        return AddSubMenu(new cSelectfileMenu(buffer, VideoDirectory));
    }
    else
    {
      cRecording *recording=GetRecording(item);
      if(recording)
     {
     /*
        const char *oldDir = NULL;
        printf("\nrecording filename: '%s'\n", oldDir = recording->FileName());
        const char *p = strrchr(oldDir, '/');
        char buffer[256];
        int len  = 255;
        if ( len > p - oldDir + 1) len = p - oldDir + 1;
        if (len < 0) len = 0;
        snprintf(buffer,len, "%s", oldDir);
        printf("\033[1;91m OldDir : '%s'\033[0m\n", buffer);*/
        //return AddSubMenu(new myMenuMoveRecording(recording,NULL,NULL));
         if ( ExtMediaCount() > 0 ) // external media available
             return AddSubMenu(new cSelectMediaMenu( recording->FileName()) );
         else
             return AddSubMenu(new cSelectfileMenu(recording->FileName(), VideoDirectory));
     }
    }
  }
  return osContinue;
}

// burns a recording
eOSState myMenuRecordings::BurnRec()
{
    myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
    if(item)
    {
        std::vector<cRecording *> burnRecordings;
        std::string name = "";
        if(item->IsDirectory())
        {
            name = item->Name();
            std::string dir = item->Name();
            if(base)
            {
                dir = std::string(base) + "~" + item->Name();
            }
            GetAllRecordingsInDir(burnRecordings, dir);
        }
        else
        {
            burnRecordings.push_back(GetRecording(item));
        }

        struct burnRecordingData
        {
            std::string name;
            std::vector<cRecording *> recordings;
            //cRecording *selectedRecording;
        } data =
        {
            name,
            burnRecordings
            //GetRecording(item)
        };
        cPluginManager::CallAllServices("Burn recordings", &data);
    }
    return osEnd;
}

// opens an info screen to a recording
eOSState myMenuRecordings::Info(void)
{
  if(HasSubMenu()||Count()==0)
    return osContinue;

  myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
  if(item&&!item->IsDirectory())
  {
    cRecording *recording=GetRecording(item);
    if(mysetup.UseVDRsRecInfoMenu && (!recording || (recording && !recording->Info()->Title())))
      return osContinue;
    else
      return AddSubMenu(new myMenuRecordingInfo(recording,true));
  }
  return osContinue;
}

// execute a command for a recording
eOSState myMenuRecordings::Commands(eKeys Key)
{
 if(HasSubMenu()||Count()==0)
  return osContinue;
 myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());

  // TODO check for pin protection!
#ifdef WITHPINPLUGIN
 if(item&&cStatus::MsgReplayProtected(GetRecording(item),item->Name(),base,item->IsDirectory())==true)
 {
	if(!isPinValid)
	{
	 getPin = true;
	 nextKey = (Key==kNone? kBlue:Key); // Commands() started with kBlue has Key = kNone, else k1...k9
	 SetStatus(tr("PIN code: ") ); // to avoid delay in display, as ProcessKey() is called once every ~1s
	 return osContinue;
	 }
	 else isPinValid = false;
 }

#endif


 if(item&&!item->IsDirectory())
 {
  cRecording *recording=GetRecording(item);
  if(recording)
  {
   myMenuCommands *menu;
   std::vector<myCommands *> mycommands;
   mycommands.push_back(myDeleteCommand::Instance());
   mycommands.push_back(myMoveCommand::Instance());
   mycommands.push_back(mySetTitleCommand::Instance());
   if(cPluginManager::GetPlugin("burn"))
   {
      mycommands.push_back(myBurnCommand::Instance());
   }
   if(!recording->IsNew())
   {
      mycommands.push_back(myMarkNewCommand::Instance());
   }
   //mycommands.push_back(myDetailsCommand::Instance());
   eOSState state=AddSubMenu(menu=new myMenuCommands(tr("Recording commands"),&RecordingCommands,recording, mycommands, commandMsg));

   if(Key!=kNone)
    state=menu->ProcessKey(Key);
   return state;
  }
 } else if(item&&item->IsDirectory()) {
  std::string path=item->FileName();
  char *name = strdup(item->Name());
  name = ExchangeChars(name, true);
  size_t index = path.rfind(name);
  free(name);
  if (index != std::string::npos)
      path = path.erase(index+strlen(item->Name()));


  if(path.size())
  {
    myMenuCommands *menu;
    static bool commandsDone = false;

#if VDRVERSNUM >= 10713
    static cList<cNestedItem> DirCommands;

    if (!commandsDone) {
        commandsDone = true;

        static cNestedItem command1("Add Child Lock  : fskprotect.sh protect");
        static cNestedItem command2("Remove Child Lock  : fskprotect.sh unprotect");

        DirCommands.Add(&command1);
        DirCommands.Add(&command2);

    } // commandsDone

#else

    static cCommands DirCommands;

   if(!commandsDone) {
       commandsDone = true;
       static cCommand command1;
       static cCommand command2;
       command1.Parse("Add Child Lock                  : fskprotect.sh protect");
       DirCommands.Add(&command1);
       command2.Parse("Remove Child Lock               : fskprotect.sh unprotect");
       DirCommands.Add(&command2);
   }
#endif

   std::vector<myCommands *> mycommands;
   mycommands.push_back(myDeleteCommand::Instance());
   mycommands.push_back(myMoveCommand::Instance());
   mycommands.push_back(myRenameCommand::Instance());
   if(cPluginManager::GetPlugin("burn"))
   {
      mycommands.push_back(myBurnCommand::Instance());
   }

   eOSState state=AddSubMenu(menu=new myMenuCommands(trVDR("Directory commands"),&DirCommands,path.c_str(), mycommands, commandMsg));

   if(Key!=kNone)
    state=menu->ProcessKey(Key);
   return state;
  }
} // if path.size()
 return osContinue;
}

// change sorting
eOSState myMenuRecordings::ChangeSorting()
{
  string path=VideoDirectory;
  path+="/";
  if(base)
    path+=base;

  for(SortListItem *item=mySortList->First();item;item=mySortList->Next(item))
  {
    if(path==item->Path())
    {
      mySortList->Del(item);
      mySortList->WriteConfigFile();
      Set(true);

      Skins.Message(mtInfo,tr("Sort by date"),1);

      return osContinue;
    }
  }
  mySortList->Add(new SortListItem(path));
  mySortList->WriteConfigFile();
  Set(true);

  Skins.Message(mtInfo,tr("Sort by name"),1);

  return osContinue;
}

// change title
eOSState myMenuRecordings::ChangeTitle()
{
    Set(true);
    if(myMenuRecordings::infotitle)
    {
        Skins.Message(mtInfo,tr("Show title"),1);
    }
    else
    {
        Skins.Message(mtInfo,tr("Show directory name"),1);
    }
    return osContinue;
}



#define MAX_PIN_LEN 5
class cGetPin
{

	char code[10]; // entered Pin
	int len;
	public:
	cGetPin()  { ClearPin(); }

	void ClearPin()    { len = 0; code[0] = 0; }
	const char* Code() { return code; }
	int Len(){ return len; }

	eOSState ProcessKey(eKeys Key)
	{
		eOSState state = osUnknown;
		switch (Key)
		{
			case kBack:
				{
					// cancel Getpin
					ClearPin();

					state = osUser1; 	// abort/cancel Pin and ask for no more
					break;
				}
			case kLeft:
				{
					if (len <= 0)
						break;

					len--;
					code[len] = 0;

					state = osContinue; 	// continue with getting PIN
					break;
				}

			case k0 ... k9:
				{
					if (len >= MAX_PIN_LEN)           // max 5 digits
						break; 		// state is osUnknown

					code[len++] = Key-k0+48;
					code[len] = 0;

					state = osContinue;
					break;
				}

			case kOk:
				{
					state = osEnd; 		// entring of PIN complete
					break;
				}

			default: return state; // osUnknown
		}
		return state;

	}

};




eOSState myMenuRecordings::ProcessKey(eKeys Key)
{
  eOSState state;

 if(getPin)
 {
	 isPinValid = false;
	 std::string msgStr = tr("PIN code: ");
	 SetStatus(msgStr.c_str());

	 static cGetPin GetPin;
	 eOSState getpinState = GetPin.ProcessKey(Key);
	 switch (getpinState)
	 {
		 case osEnd:
			 {				// got PIN, check for validity
				 getPin = false;
				 SetStatus(NULL);
				 cPlugin *plugin = cPluginManager::GetPlugin("pin");
				 if (plugin)
				 {
				 	const char *serviceCallString=NULL;
					switch(nextKey)
					{
						case k1 ... k9 :
						case kBlue: // Commands
							serviceCallString = "CheckPIN_and_ADD_OneTimeScriptAuth";
							break;
						default:
							serviceCallString = "CheckPIN";
							break;
					}

					 bool result = plugin->Service( serviceCallString ,(void*) GetPin.Code()); // valid PIN ?
					 if (result)
					 {
						 isPinValid = true;
						 cRemote::Put(nextKey); // process the key pressed again
					 }
					 else
					 {
						 // display pin invalid
						 Skins.Message(mtError, tr("Invalid PIN"), 1 );
					  }
				 }

				 GetPin.ClearPin();
				 break;
			 }
		 case osUser1: 		// PIN aborted
			 // clear message & display
			 GetPin.ClearPin();

			 getPin = false;
			 isPinValid = false;
			 Skins.Message(mtError, tr("PIN Aborted"), 1 );
             SetStatus(NULL); // clear 'PIN Code:', status message
			 /*SetStatus("Pin Aborted!");

			 sleep(1);
			 SetStatus(NULL);*/
			 break;

		 case osContinue: 		// still entering pin
			 //show *s

		 case osUnknown: 		// wait for next key
		 default: 			// do nothing
		 	msgStr.append(GetPin.Len(), '*') ;
			//Skins.Message(mtInfo, msgStr.c_str() );
			SetStatus(msgStr.c_str());
			break;
	 }

	return osContinue;

 }

  /*
  if(edit)
  {
    myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
    if(Key==kRed || Key==kGreen || Key==kYellow || (!item->IsDVD() && Key==kBlue) || Key==kBack)
    {
      edit=false;
      helpkeys=-1;
    }
    switch(Key)
    {
      case kRed: return Delete();
      case kGreen: return MoveRec();
      case kYellow: if(item->IsDirectory())
		   				return Rename();
	         		else
		   				return SetTitle();
		 		break;

      case kBlue: if(item&&!item->IsDVD())
                    return Details();
                  else
                    break;
      case kBack: SetHelpKeys();
      default: break;
    }
    state=osContinue;
  }
  */
  //else
  //{
    state=cOsdMenu::ProcessKey(Key);

    if(true)//(HasProcessFromCaller("extrecmenu"))
    {
      myMenuRecordingsItem *it;

      for (int i= 0; i < Count(); i++)
      {
	it = static_cast<myMenuRecordingsItem *>(Get(i));
	if(it->IsDirectory())
	  continue;

	  //printf("it->FileName() = %s\n", (std::string("\"") + it->FileName() + "\"").c_str());
	  //printf("it->GetRecording()->Name() = %s\n\n", it->GetRecording()->Name());
	  int percentage  = GetBgProcess(it->GetRecording()->Name(), "Mark Commercials");
	  /*if(percentage == 100)
	  {
	      //for(int i = 0; i< bgcommandlist.size();++i)
	      {
		std::string desc = std::string("\"") + it->FileName() + "\"";
		if(bgcommandlist[i].description == desc && bgcommandlist[i].command = "noadcall.sh")
		{
		  foundprocess = true;
		}
		printf("it->FileName() = %s\n", (std::string("\"") + it->FileName() + "\"").c_str());
		printf("it->GetRecording()->Name() = %s\n", it->GetRecording()->Name());
		printf("bgcommandlist[i].command =  %s\n", bgcommandlist[i].command.c_str());
		printf("bgcommandlist[i].description =  %s\n", bgcommandlist[i].description.c_str());
	      }
	  }
	  if(percentage!= 100)
	  {
	    //TODO
	  }
	  if(percentage!= 100)
	  {

	  }*/

	  if(it->SetProgress("Mark Commercials", percentage))
	  {
	    Display();
	  }
	//}

      }
    }

    static bool hadSubMenu = false;
    if (hadSubMenu && !HasSubMenu() && commandMsg != "") // when selectMedia is not necessary, moving files returns osUser3
    {
        Set(true);

        //remove line breaks
        for (int i =0; i<commandMsg.size(); i++)
        {
            if (commandMsg[i] =='\n')
            {
                commandMsg[i] = ' ';
            }
        }

        //printf("commandMsg.c_str()= %s\n", commandMsg.c_str());

        Skins.Message(mtInfo, commandMsg.c_str(), 3);
        commandMsg = "";
    }
    hadSubMenu = HasSubMenu();

    if (Key != kNone && state != osUnknown)
        ShowRecordingDetailsInSideNote();

    myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
    if (HasSubMenu() && state == osUser3) // when selectMedia is not necessary, moving files returns osUser3
    {
        CloseSubMenu(); // close earlier menu
	    Set(true);
        SetHelpKeys();  // but still in edit mode !
        return osContinue;
    }

    if (HasSubMenu() && state == osUser4) //move selected
    {
        CloseSubMenu(); // close earlier menu
        return MoveRec();
    }
    if (HasSubMenu() && state == osUser5) //delete selected
    {
        CloseSubMenu(); // close earlier menu
        return Delete();
    }
    if (HasSubMenu() && state == osUser6) //rename
    {
        CloseSubMenu(); // close earlier menu
        if(item && item->IsDirectory())
        {
           //printf("--------------return rename----------------\n");
           return Rename();
        }
    }
    if (HasSubMenu() && state == osUser7) //SetTitle
    {
        CloseSubMenu(); // close earlier menu
        if(item && !item->IsDirectory())
        {
            //printf("--------------return SetTitle()----------------\n");
            return SetTitle();
        }
    }
    if (HasSubMenu() && state == osUser8) //BurnRec
    {
        CloseSubMenu(); // close earlier menu
        if(item && !item->IsDirectory());
        return BurnRec();
    }
    if (HasSubMenu() && state == osUser8) //dvd Details()
    {
        CloseSubMenu(); // close earlier menu
#if VDRVERSNUM >= 10703
        if(item&&!item->IsDVD()&&item->IsPesRecording())
#else
        if(item &&!item->IsDVD())
#endif
        {
            return Details();
        }
    }
    if (HasSubMenu() && state == osUser9) //mark as new
    {
        CloseSubMenu();
	if(item && !item->IsDirectory())
	{
#if APIVERSNUM > 10700
            MarkAsNew();
#endif
        std::string path=item->FileName();
	    Set(true);

            /*char *name = strdup(item->Name());
	    name = ExchangeChars(name, true);
	    size_t index = path.rfind(name);
	    free(name);
	    if (index != std::string::npos)
	      path = path.erase(index+strlen(item->Name()));*/

	    std::string fn = path + "/resume.vdr";
            unlink(fn.c_str());
	    fn = path + "/resume";
            unlink(fn.c_str());
#if APIVERSNUM < 10700
            SystemExec("touch /media/reel/recordings/.update");
#endif
        }
    SetHelpKeys();
    return osContinue;
    }


    if (state==osUnknown || state < 0)
    {
      switch(Key)
      {
        case kOk: return Play();
        case kBlue: //printf("%d RecordingCommands\n", RecordingCommands.Count());
            if (RecordingCommands.Count()) return Commands();
                    break;
        case kGreen:
                    {
                        myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
                        if(item && !item->IsDirectory())
                        {
                            cRecording*rec=GetRecording(item);
                            if(!rec->IsNew())
                                return Rewind();
                            else
                                return Play();
                        }
                        return osContinue;
                    }
        case kRed:
                    {
                        if(!HasSubMenu())
                        {
                          myMenuRecordingsItem *item=(myMenuRecordingsItem*)Get(Current());
                          if(item)
                          {
                            if(item->IsDirectory())
                            {
                              if(item->GetDirIsMoving())
                              {
                                string path;
                                if(base)
                                {
                                  path=base;
                                  path+="~";
                                  path+=item->Name();
                                 }
                                 else
                                  path=item->Name();

                                if(Interface->Confirm(tr("Cancel moving?")))
                                {
                                  for(cRecording *rec=Recordings.First();rec;rec=Recordings.Next(rec))
                                  {
                                    if(!strncmp(path.c_str(),rec->Name(),path.length()))
                                      MoveCutterThread->CancelMove(rec->FileName());
                                  }
                                  Set(true);
                                }
                              }
                              else
                              {
                                //edit=true;
                                //SetHelp(NULL, tr("Button$Move"), tr("Button$Rename"));
                                if (::Setup.ExpertNavi)
                                    return Delete();
                                else
                                    return osContinue;
                              }
                            }
                            else
                            {
                              cRecording *rec=GetRecording(item);
                              if(rec)
                              {
#ifdef WITHPINPLUGIN
                                if(cStatus::MsgReplayProtected(rec,item->Name(),base,item->IsDirectory())==true)
					{
						if(!isPinValid)
						{
							getPin = true;
							nextKey = kRed;
							SetStatus(tr("PIN code: ") ); // to avoid delay in display, as ProcessKey() is called once every ~1s
							return osContinue;
						}
						else
							isPinValid = false;
					}
					//break;
#endif
                                if(MoveCutterThread->IsCutting(rec->FileName()))
                                {
                                  if(Interface->Confirm(tr("Cancel editing?")))
                                  {
                                    MoveCutterThread->CancelCut(rec->FileName());
                                    Set(true);
                                  }
                                }
                                else if(MoveCutterThread->IsMoving(rec->FileName()))
                                {
                                  if(Interface->Confirm(tr("Cancel moving?")))
                                  {
                                    MoveCutterThread->CancelMove(rec->FileName());
                                    Set(true);
                                  }
                                }
                                else if (::Setup.ExpertNavi)
                                {
                                  //edit=true;
                                  return Delete();
                                  //SetHelp(tr("Button$Rename"),tr("Button$Move"),tr("Button$Delete"),(!item->IsDVD())?tr("Details"):NULL);

                     /*
                     if(item->IsDirectory())
                     SetHelp(NULL,tr("Button$Move"),tr("Button$Rename"),NULL);
                         else
                     SetHelp(tr("Button$Delete"),tr("Button$Move"),tr("Button$Rename"),(!item->IsDVD())?tr("Details"):NULL);
                     */

                                }
                                else
				    return osContinue;
                              }
                            }
                          }
                        }
                      }
                      break;
#if REELVDR && APIVERSNUM >= 10718
        case kYellow: return osContinue;
                      //return Info();
#else
        case kYellow: //fall through
#endif
        case kInfo:   return Info();
        case kGreater:
                      {
                        infotitle = infotitle ? false : true;
                        return ChangeTitle();
                      }
        case k1...k9: return Commands(Key);
        case k0: return ChangeSorting();
        default: break;
      }
    //}
    if(Recordings.StateChanged(recordingsstate) || MoveCutterThread->IsCutterQueueEmpty())
      Set(true);

    if(!Count() && level>0)
      state=osBack;

    //if(!HasSubMenu() && Key!=kNone)
    //    SetHelpKeys();
  }
  //else
  //   	DDD("NOT osUnknown, %d", state);
  if (!HasSubMenu() &&
     (Key == kUp || Key == kDown || Key == (kUp|k_Repeat)
      || Key == (kDown|k_Repeat)
      || Key == (kLeft || Key == kRight)))
      SetHelpKeys();
  return state;
}


int myMenuRecordings::GetBgProcess(std::string processDesc, std::string processName)
{
  /*static long long duration1 = 0;
  static long long duration2 = 0;
  timespec time;
  timespec last_time;

  clock_gettime(CLOCK_REALTIME, &last_time);*/

  float percentage;
  uint pos = processDesc.find_last_of('~');
  if(pos == processDesc.npos)
  {
    pos= 0;
  }
  else
  {
    ++pos;
  }
  processDesc = processDesc.substr(pos);

  char *name = MALLOC(char, processDesc.size() + 1);
  strcpy(name, processDesc.c_str());
        name[processDesc.size()] = 0;
  name = ExchangeChars(name, true);

  //clock_gettime(CLOCK_REALTIME, &time);
  //duration1 += ((time.tv_sec - last_time.tv_sec) *1000000 + (time.tv_nsec - last_time.tv_nsec)/1000);


  //printf("+++++++myMenuRecordings::GetBgProcess3, processName=%s name= %s++++++\n", processName.c_str(), name);
  GetBgProcessProgress(processName, std::string("\'") + name + "\'", "extrecmenu", percentage);

  return percentage;
}
