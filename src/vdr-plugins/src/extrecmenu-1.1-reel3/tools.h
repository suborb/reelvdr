std::string myStrReplace(std::string S,char C1,char C2);

class SortListItem:public cListObject
{
  private:
    std::string path;
  public:
    SortListItem(std::string Path){path=Path;};
    std::string Path(){return path;}
};

class SortList:public cList<SortListItem>
{
  public:
    void ReadConfigFile();
    void WriteConfigFile();
    bool Find(std::string Path);
};

extern SortList *mySortList;

bool MoveRename(const char *OldName,const char *NewName,cRecording *Recording,bool Move);

class myRecListItem:public cListObject
{
  friend class myRecList;
  private:
    static bool SortByName;
    char *filename;
    static char *StripEpisodeName(char *s);
  public:
    myRecListItem(cRecording *Recording);
    ~myRecListItem();
    virtual int Compare(const cListObject &ListObject)const;
    cRecording *recording;
};

class myRecList:public cList<myRecListItem>
{
  public:
    void Sort(bool SortByName);
};

// --- MoveListItem -----------------------------------------------------------
class MoveListItem:public cListObject
{
  private:
    bool moveinprogress;
    bool movecanceled;
    std::string from;
    std::string to;
  public:
    MoveListItem(std::string From,std::string To){from=From;to=To;moveinprogress=false;movecanceled=false;}
    std::string From() const { return from; }
    std::string To() const { return to; }
    void SetMoveInProgress() { moveinprogress=true; }
    bool GetMoveInProgress() const { return moveinprogress; }
    void SetMoveCanceled() { movecanceled=true; }
    bool GetMoveCanceled() const { return movecanceled; }
};

// --- MoveList ---------------------------------------------------------------
class MoveList:public cList<MoveListItem>
{
};

// --- CopyListItem -----------------------------------------------------------
class CopyListItem:public cListObject
{
  private:
    bool copyinprogress;
    bool copycanceled;
    std::string from;
    std::string to;
  public:
    CopyListItem(std::string From,std::string To){from=From;to=To;copyinprogress=false;copycanceled=false;}
    std::string From() const { return from; }
    std::string To() const { return to; }
    void SetCopyInProgress() { copyinprogress=true; }
    bool GetCopyInProgress() const { return copyinprogress; }
    void SetCopyCanceled() { copycanceled=true; }
    bool GetCopyCanceled() const { return copycanceled; }
};

// --- CopyList ---------------------------------------------------------------
class CopyList:public cList<CopyListItem>
{
};

// --- CutterListItem ---------------------------------------------------------
class CutterListItem:public cListObject
{
  private:
    bool cutinprogress;
    std::string filename;
    std::string newfilename;
    std::string displayname;
    time_t starttime;
  public:
    CutterListItem(std::string FileName, std::string DisplayName, time_t startTime){filename=FileName; displayname = DisplayName, cutinprogress=false; starttime = startTime;};
    void SetNewFileName(std::string NewFileName){newfilename=NewFileName;}
    std::string FileName() const {return filename;}
    std::string NewFileName() const {return newfilename;}
    std::string DisplayName() const { return displayname;}
    void SetCutInProgress(){cutinprogress=true;}
    bool GetCutInProgress() const {return cutinprogress;}
    time_t GetStartTime() const {return starttime;}
    void SetStartTime(time_t StartTime) { starttime = StartTime; }
};

// --- CutterList -------------------------------------------------------------
class CutterList:public cList<CutterListItem>
{
};

// --- WorkerThread -----------------------------------------------------------
class WorkerThread:public cThread
{
  private:
    bool cancelmove,cancelcut,cancelcopy;
    MoveList *MoveBetweenFileSystemsList;
    CopyList *CopyBetweenFileSystemsList;
    CutterList *CutterQueue;
    void Cut(std::string From,std::string To, time_t startTime, std::string displayname);
    bool Move(std::string From,std::string To);
    bool Copy(std::string From,std::string To);
    uchar *PATPMT;
  protected:
    virtual void Action();
  public:
    WorkerThread();
    ~WorkerThread();
    const char *Working();
    bool IsCutting(std::string Path);
    bool IsMoving(std::string Path);
    bool IsCopying(std::string Path);
    void CancelCut(std::string Path);
    void CancelMove(std::string Path);
    void CancelCopy(std::string Path);
    void AddToCutterQueue(std::string Path, std::string DisplayName);
    void AddToMoveList(std::string From,std::string To);
    void AddToCopyList(std::string From,std::string To);
    bool IsCutterQueueEmpty(){return CutterQueue->First();}
    bool IsMoveListEmpty(){return MoveBetweenFileSystemsList->First();}
    bool IsCopyListEmpty() { return CopyBetweenFileSystemsList->First(); }
    void CheckTS(cUnbufferedFile*);
};

extern WorkerThread *MoveCutterThread;

long long du_ignore_hidden_dirs(const char*); // see du.c
