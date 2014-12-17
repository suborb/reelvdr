#ifndef MYMENURECORDINGS_H
#define MYMENURECORDINGS_H

std::string RecordingsIcons(const cRecording *Recording);

// --- myMenuRecordingsItem ---------------------------------------------------
class myMenuRecordingsItem:public cOsdItem
{
  private:
    bool dirismoving;
    bool isdvd;
   	bool isvideodvd;
    bool isProtected;
#if REELVDR
    /* holds the start time of the latest recording in the subdirectory */
    time_t latestRecordingStartTime;
#endif
#if VDRVERSNUM >= 10703
    bool isPesRecording;
#endif
    char dvdnr[BUFSIZ];
    int level,isdirectory;
    int totalentries,newentries;
    char *title;
    char *name;
    const char *filename;
    std::string uniqid; // this is the unique name that identifies a recording
    cRecording *Recording;
    const char *Parent;
    int Level;
    int lastPercentage;
  public:
    myMenuRecordingsItem(cRecording *Recording,const char *Parent,int Level);
    ~myMenuRecordingsItem();
    cRecording *GetRecording(){return Recording;}
    const char *FileName(){return filename;}
    const char *Name(){return name;}
    bool IsDirectory(){return name!=NULL;}
    bool SetProgress(std::string processName, int percentage);
    std::string GetBgProcessString(std::string processDesc, std::string processName, int percentage);
    const char *GetParent(){ return Parent;}
#if REELVDR
    int TotalEntries() const {return totalentries;}
    int NewEntries() const { return newentries; }
    bool IsProtected() const {return isProtected; }

    /* call this function to say, recording is in this items directory
        or in one of its subdirectories*/
    void AddRecordingToDir(cRecording* recording);
    time_t LatestRecordingStartTime() const;
#endif /* REELVDR */

#if VDRVERSNUM >= 10703
    bool IsPesRecording(void) const { return isPesRecording; }
#endif
    void IncrementCounter(bool IsNew);
    bool IsDVD(){return isdvd;}
    void SetDirIsMoving(bool moving){dirismoving=moving;}
    bool GetDirIsMoving(){return dirismoving;}
    const char *UniqID(){return uniqid.length()?uniqid.c_str():"";}
};

// --- myMenuRecordings -------------------------------------------------------
class myMenuRecordings:public cOsdMenu
{
 private:
  bool edit;
  
 // needed to Check pin and resume with the "commands"
  bool getPin; // static ?
  bool isPinValid; // static ?
  eKeys nextKey; //


  
  
  static bool wasdvd;
#ifndef REELVDR
  static bool golastreplayed;
#endif
  static dev_t fsid;
  static int freediskspace;
  int level,helpkeys;
  int recordingsstate;
  const char *parent;
  char *base;
  
  std::string commandMsg;
  
  bool Open();
  void SetHelpKeys();
  
  void SetFreeSpaceTitle();
  bool replayDVD;

  void Title();
  cRecording *GetRecording(myMenuRecordingsItem *Item);
  eOSState Play();
  eOSState Rewind();
  eOSState Delete();
  eOSState SetTitle();
  eOSState Rename();
  eOSState MoveRec();
  eOSState Info();
  eOSState Details();
  eOSState BurnRec();
  eOSState MarkAsNew();
  eOSState Commands(eKeys Key=kNone);
  eOSState ChangeSorting();
  eOSState ChangeTitle();  //Behni
  void Delete(cRecording *recording);
  eOSState DeleteDirectory();
  void GetAllRecordingsInDir(std::vector<cRecording *> &recordings, std::string base);
  int FreeMB();
  int GetBgProcess(std::string processDesc, std::string processName);
 public:
#if REELVDR
   static bool golastreplayed;
#endif
  static bool infotitle;    //Behni
  myMenuRecordings(const char *Base=NULL,const char *Parent=NULL, int Level=0);
  ~myMenuRecordings();
  void Set(bool Refresh=false,char *current=NULL);
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Display();
  void ShowRecordingDetailsInSideNote();
};

// --- myMenuRecordingTitle --------------------------------------------------
class myMenuRecordingTitle:public cOsdMenu
{
 private:
  char title[MaxFileName];
  char shortText[MaxFileName];
  cRecording *recording;
  myMenuRecordings *menurecordings;
 public:
  myMenuRecordingTitle(myMenuRecordings *MenuRecordings,cRecording *Recording);
  ~myMenuRecordingTitle();
  virtual eOSState ProcessKey(eKeys Key);
};


// --- myMenuRenameRecording --------------------------------------------------
class myMenuRenameRecording:public cOsdMenu
{
 private:
  bool isdir;
  char *dirbase,*dirname;
  char name[MaxFileName];
  char path[MaxFileName];
  cRecording *recording;
  myMenuRecordings *menurecordings;
 public:
  myMenuRenameRecording(cRecording *Recording,const char *DirBase,const char *DirName);
  ~myMenuRenameRecording();
  virtual eOSState ProcessKey(eKeys Key);
};

// --- myMenuMoveRecording ----------------------------------------------------
class myMenuMoveRecording:public cOsdMenu
{
 private:
  int level;
  char *base;
  char *dirbase,*dirname;
  cRecording *recording;
  myMenuRecordings *menurecordings;
  void Set();
  eOSState Open();
  eOSState MoveRec();
  eOSState Create();
 public:
  myMenuMoveRecording(cRecording *Recording,const char *DirBase,const char *DirName,const char *Base=NULL,int Level=0);
  ~myMenuMoveRecording();
  virtual eOSState ProcessKey(eKeys Key);
  static bool clearall;
};

// --- myMenuRecordingDetails -------------------------------------------------
class myMenuRecordingDetails:public cOsdMenu
{
 private:
  int priority,lifetime;
  cRecording *recording;
  myMenuRecordings *menurecordings;
  const char *PriorityTexts[3];
  int tmpprio;
 public:
  myMenuRecordingDetails(cRecording *Recording);
  virtual eOSState ProcessKey(eKeys Key);
};

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
