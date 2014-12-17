#include <algorithm>
#include <string>

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include <vdr/videodir.h>
#include <vdr/menuitems.h>
#include <vdr/debug.h>
#include "SelectFile.h"
#include "tools.h"
#include "mymenusetup.h"

#define DIR_SELECTOR trNOOP("Directory Selector")

int FileSystemAvailableSpaceMB(const std::string&path) // MB
{
    struct statvfs fsstat;
    if(statvfs(path.c_str(),&fsstat)) return -1; // Error

        return (int)(fsstat.f_bavail/(1024.0*1024.0/fsstat.f_bsize));
}

int RecordingsSizeMB(std::string& path) // recordings under the path 'path'
{
    int recMB = 0;
    for(cRecording *rec=Recordings.First();rec;rec=Recordings.Next(rec))
    {
        if(!strncmp(path.c_str(),rec->FileName(),path.length()))
            recMB += DirSizeMB(rec->FileName());
    }

    return recMB;
}

std::string AddIcon(std::string path, char icon)
{
    std::string str_with_icon = " \t"; // 1 space (replaced with icon char) and a tab character
    str_with_icon.at(0) = icon; // icon
    return str_with_icon + path;
}

bool InSameFileSystem(const std::string& path1, const std::string& path2)
{
    struct stat stat1,stat2;
    stat(path1.c_str(),&stat1);
    stat(path2.c_str(),&stat2);

    return stat1.st_dev==stat2.st_dev;
}
// return the current directory name give the full path; path = "/media/hd" returns "hd"
std::string CurrentDir(std::string path)
{
    return path.substr(path.find_last_of("/")+1);
}
std::string AddFolderIcon(std::string path)
{
    return AddIcon(path, char(130));
}
std::string AddFolderUpIcon(std::string path)
{
    return std::string("\x80 \t") + path;
    //return AddIcon(path, char(128)); // avoid this, extra ".." is drawn by skinreel3
}
std::string RemoveIcon(std::string path)
{
    if (path.length() < 2 ) return path;

    //handle FolderUp Icon seperately
    if (path.at(0) == char(128)) return path.substr(3) ;

    return path.substr(2, std::string::npos);
}
std::string GetEnding(const std::string name)
{
    return name.substr(name.find_last_of(".") + 1);
}


std::string ParentDir(const std::string& path)
{
    size_t index = path.find_last_of("/");
    return index <= 0? "/" : path.substr(0, path.find_last_of("/"));
}

bool IsFile(const std::string &entry)
{
    struct stat64 statBuf;
    stat64(entry.c_str(), &statBuf);
    if(S_ISREG(statBuf.st_mode)) //regular file
    {
        return true;
    }
    return false;
}

bool IsDir(const std::string &entry)
{
    struct stat64 statBuf;
    stat64(entry.c_str(), &statBuf);
    if(S_ISDIR(statBuf.st_mode)) //regular file
    {
        return true;
    }
    return false;
}

bool IsRecDir(const std::string &entry)
{
    std::string ext = GetEnding(entry);
    return ext == "rec" || ext == "del";
}
// loop thro' the dir and if it contains even one non-".rec" directory or
// if the directory has no subdirectories
// then return true: it is a real subdir not a recording's dir
bool IsRealRecSubDir(const std::string &entry)
{
    DIR *dp;
    struct dirent *dirp;
    std::string dirEntry;
    bool hasSubDir = false;

    dp = opendir(entry.c_str());

    if (!dp) return false; // could not open dir

    while((dirp = readdir(dp)) != NULL)
    {
        dirEntry = dirp->d_name;

        // exclude all hidden files
        if ( dirEntry.empty() || dirEntry[0]=='.' ) continue;

        if (IsDir(entry + "/" + dirEntry))
        {
            hasSubDir = true;
            if ( !IsRecDir(dirEntry)) { //found one non-recording sub-dir
                closedir(dp);
                return true;
            }
        }
    }

    closedir(dp);
    return !hasSubDir; // if "entry" has no subdir then show it
}
std::string GetLastNTokens(std::string &WorkString, char token, int num=1)
{
    int i = WorkString.length();

    for ( ; i > 0 ; --i)
    {
       if ( WorkString[i] == token ) --num;
       if ( num <= 0 ) break;
    }

    return WorkString.substr(i,std::string::npos);
}

std::string AddSubDir(const std::string& Parent, const std::string& subDir)
{
    if( Parent.length() && Parent.at(Parent.length()-1) == '/') return Parent + subDir;

    return Parent + std::string("/") + subDir;
}

std::string ReplaceUnderscores(std::string dir)
{
    // replace '_' to ' ' in currDir
    for (int i=0; i<dir.size(); ++i)
        if (dir[i]=='_') dir[i] = ' ';
    
    return dir;
}

#define osDir osUser1
class cDirItem : public cOsdItem
{
private:
    std::string path;
    std::string name; // display name
public:
    cDirItem(std::string Path, std::string Name="", eOSState state=osDir, bool selectable=true) 
        : path(Path), name(Name), cOsdItem("", state, selectable)
    {
        SetName();
        Set();
    }
    
    /**
     * @brief SetName given 'path' set its display name (with its icon),
     * if the display name was not already set
     */
    
    void SetName()
    {
        // add folder up icon
        if (name == ".." || path == "..")
        {
            name = AddFolderUpIcon(name.empty()?"..":name);
            return;
        }
        
        // if name is !empty, use it
        if (!name.empty()) return;
        
        std::string currDir = CurrentDir(path);
        
        if (currDir.empty()) 
        { 
            name = "[unknown]";
            return ;
        }
        
        currDir = ReplaceUnderscores(currDir);
        
        name = AddFolderIcon(currDir);
    }
    
    void Set()
    {
        SetText(name.c_str());
    }
    
    std::string DirPath() { return path; }
};
        

cSelectfileMenu::~cSelectfileMenu() {}

//cSelectfileMenu::cSelectfileMenu(myMenuRecordingsItem * item, std::string Base, bool showdirsonly) : cOsdMenu(tr("Directory Selector"))
cSelectfileMenu::cSelectfileMenu(std::string oldDir, std::string Base, bool showdirsonly) : cOsdMenu(tr(DIR_SELECTOR),4)
{
    ShowDirsOnly = showdirsonly;
    BaseDir = Base;
    pwd = BaseDir;
    OldDir = oldDir;

    size_t index = OldDir.find(".rec");

    // if .rec is at the end of OldDir string the it is a recording
    if (index != std::string::npos && index + 4 == OldDir.length())
    {
        dirSuffix = GetLastNTokens(OldDir, '/', 2);
        moveRecording = true;
    }
    else // give is a directory
    {
        dirSuffix = GetLastNTokens(OldDir, '/', 1);
        moveRecording = false;
    }

    Set();
}

bool stringCompareCaseInsensitive( const std::string &left, const std::string &right ){
   for( std::string::const_iterator lit = left.begin(), rit = right.begin(); lit != left.end() && rit != right.end(); ++lit, ++rit )
      if( tolower( *lit ) < tolower( *rit ) )
         return true;
      else if( tolower( *lit ) > tolower( *rit ) )
         return false;
   if( left.size() < right.size() )
      return true;
   return false;
}


bool CompareAscDesc(const std::string &left, const std::string &right) {
    if(mysetup.DescendSorting)
        return stringCompareCaseInsensitive(right, left);
    else
        return stringCompareCaseInsensitive(left, right);
}

// show contents of 'pwd'
void cSelectfileMenu::Set()
{
   Clear();

    DIR *dp;
    struct dirent *dirp;
    std::string dirEntry;

    dp = opendir(pwd.c_str());

    // could not open Dir 'pwd'
    if ( !dp )
    {
        PRINTF("\033[0;91mCould not open \033[0m'%s'\n",pwd.c_str());
        return;
    }

     std::string titleStr;
     /*get the current directory name*/
     std::string c_dir = CurrentDir(pwd);
     c_dir = ReplaceUnderscores(c_dir);
    titleStr = std::string(tr(DIR_SELECTOR)) + std::string(": ") + c_dir;
    SetTitle(titleStr.c_str());

//     /* show pwd */
//     Add(new cOsdItem( pwd.c_str(), osUnknown, false));
//     Add(new cOsdItem( "", osUnknown, false));

    // Add parent dir '..'
    if (pwd != "/" && pwd != "/media")
    {
        //Add(new cOsdItem( AddFolderUpIcon("..").c_str(), osUser1, true));
        Add(new cDirItem("..", tr("up")));
        Add(new cOsdItem( "", osUnknown, false)); //empty line
    }

    std::vector<std::string> entries; // needed to able to sort the entries

    // Add sub-directories
    while((dirp = readdir(dp)) != NULL)
    {
        dirEntry = dirp->d_name;

        // exclude all hidden files
        if ( dirEntry.empty() || dirEntry[0]=='.' || dirEntry == "LiveBuffer" ) continue;
    // exclude all files ending with ".rec" & ".del"
    if ( IsRecDir(dirEntry) ) continue;

        if (IsDir(pwd + "/" + dirEntry))
        {
            // Sub-Directory
        if (IsRealRecSubDir(pwd + "/" + dirEntry))
            entries.push_back(dirEntry);
        }
        else if (!ShowDirsOnly && IsFile(pwd + "/" + dirEntry))
        {
            // File
        }
    }
    closedir(dp);

    std::sort(entries.begin(), entries.end(), CompareAscDesc);

    for (unsigned int i = 0; i < entries.size(); i++)
#if 1
        //Add(new cOsdItem(AddFolderIcon(entries.at(i)).c_str(), osUser1, true));
        Add(new cDirItem(entries.at(i)));
#else
    {
        char tmp[256];
        strcpy(tmp, entries.at(i).c_str());
        char *name = ExchangeChars(tmp,false);
        Add(new cOsdItem(AddFolderIcon(name).c_str(), osUser1, true));
    }
#endif

    if(pwd != VideoDirectory)
    {
        SetHelp(tr("Cancel"), tr("Move"), tr("Rec Dir"), tr("Button$Create"));
    }
    else
    {
         SetHelp(tr("Cancel"), tr("Move"), NULL, tr("Button$Create"));
    }
    Display();
}

eOSState cSelectfileMenu::ProcessKey(eKeys key)
{
    eOSState state = cOsdMenu::ProcessKey(key);

    if (HasSubMenu())
    {
        if (state == osUser2) // reload page
        {
            CloseSubMenu();
            Set();
        }

        return state;
    }

    // no Sub-Menus
    switch(key)
    {
        case kOk:
        {
            if (state == osUser1) // Directory Item
                {
                    std::string dir;
                    cDirItem *currItem = dynamic_cast<cDirItem* >(Get(Current()));
                    if (!currItem || currItem->DirPath().empty()) // empty item OR empty text
                        break;

                    dir = currItem->DirPath();
                    //dir = RemoveIcon(dir);

                    // parent dir
                    if (dir == "..")
                    {
                        pwd = ParentDir(pwd);
                    }
                    else // open sub-dir
                    {
                        if (pwd == "/") // avoid two '/' in the beginning
                            pwd = pwd  + dir;
                        else
                            pwd = pwd + std::string("/") + dir;
                    }

                    Set();
                }
        }
            break;

        case kGreen:
            {
                PRINTF("OldDir : '%s'\n", OldDir.c_str());
                std::string newDir="";
                cDirItem *currItem = dynamic_cast<cDirItem* >(Get(Current()));
                if (currItem && !currItem->DirPath().empty())
                {
                    newDir =  currItem->DirPath();
                    PRINTF("Selected : '%s'\n", RemoveIcon(newDir).c_str());
                    newDir = AddSubDir(pwd, RemoveIcon(newDir));
                    newDir = pwd;

                    // check if moving into its own subdirectory!
                    if (newDir.find(OldDir) == 0)
                    {
                        Skins.Message(mtError, tr("Cannot move into its own sub folder"));
                        return osContinue;
                    }

                    // Check for write permission in destination-directory
                    if (access(newDir.c_str(), W_OK) != 0)
                    {
                        Skins.Message(mtError, tr("Cannot write into directory"));
                        return osContinue;
                    }

                    if (InSameFileSystem(OldDir, newDir))
                    {
                        newDir += dirSuffix;
                        PRINTF("dirSuffix : '%s'\n", dirSuffix.c_str());
                        PRINTF("newDir : '%s'\n", newDir.c_str());
                        if (moveRecording)
                        {
                            MoveRename(OldDir.c_str(), newDir.c_str(), Recordings.First(), true);
                            Skins.Message(mtInfo, tr("Moving Recording"));
                        }
                        else // moving directories
                        {
                            MoveRename(OldDir.c_str(), newDir.c_str(),NULL, true);
                            Skins.Message(mtInfo, tr("Moving Directories"));
                        }
                    }
                    else /* in different filesystems*/
                    {
                        // move recordings via Cutter queue Thread
                        if (MoveUsingThread(OldDir, newDir) <= 0)
                        {
                            Skins.Message(mtError, tr("Move failed. Not enough free space"));
                            return osContinue;
                        }

                    }

                    return osUser3; // close every menu and go back to recording's list menu
                } // if (Get())
            }// kGreen
            break;

        case kYellow: // Reset, get back to videodir
            pwd = VideoDirectory;
            Set();
            break;

        case kBlue: // Create a sub dir
        /*
            if ( mkdir( AddSubDir(pwd, "NewDir").c_str(), 0777) == 0)
                Skins.Message(mtInfo, tr("Created"));
            else
                Skins.Message(mtError, tr("Failed"));
            Set();*/
            AddSubMenu(new cCreateDirMenu(pwd));
            return osContinue;
            break;

        case kRed: // Cancel
            return osBack;

        case kBack:
            /** TB: Ibo wants it like that: */
            return osBack;
            /*exit if pwd is /media/hd/recordings || /media */
#if 0
            if (pwd == VideoDirectory || pwd == "/media") return osBack;

            pwd = ParentDir(pwd);
            Set();
            break;
#endif

        default:
            break;
    }

    return osContinue;
}

int cSelectfileMenu::MoveUsingThread(std::string srcDir, std::string destDir)
{
    int freeSpaceMB = FileSystemAvailableSpaceMB(destDir);

    cThreadLock RecordingsLock(&Recordings);
    int recordingsSizeMB = RecordingsSizeMB(srcDir);
    int count_moved = 0;

    // not enough space in disk
    if (recordingsSizeMB > freeSpaceMB)
    {
        return count_moved;
    }

    PRINTF("\nFreeSpace : %i MB\n", freeSpaceMB);
    PRINTF("Recs Size MB : %i MB\n", recordingsSizeMB);

    PRINTF("'%s' ->\n", srcDir.c_str());
    PRINTF("'%s'\n\n",destDir.c_str());
    if (moveRecording) // move one recording
    {
        std::string dest = destDir + dirSuffix;
        PRINTF("Src1: \033[0;91m'%s'\033[0m\n", srcDir.c_str());
        PRINTF("Dest1: \033[0;92m'%s'\033[0m\n\n",dest.c_str());
        MoveCutterThread->AddToMoveList( srcDir.c_str(), dest.c_str() );
        ++count_moved;
    }
    else // move a directory containing one or more recordings
    {
        for(cRecording *rec=Recordings.First();rec;rec=Recordings.Next(rec))
        {
            if(!strncmp(srcDir.c_str(),rec->FileName(),srcDir.length()))
            {
                //char *buf=ExchangeChars(strdup(srcDir.c_str()+strlen(VideoDirectory)+1),false);
                char *buf=strdup(srcDir.c_str()+strlen(VideoDirectory)+1);


                if(strcmp(rec->Name(),buf))
                {
                    char *buf2;
                    //asprintf(&buf2,"%s%s",destDir.c_str(),rec->FileName()+ srcDir.length());
                    char *posSlash = (char *) strchr((const char*)buf, '/');
                    asprintf(&buf2,"%s/%s%s",destDir.c_str(),posSlash ? posSlash+1 : buf,rec->FileName()+ srcDir.length());

                    MoveCutterThread->AddToMoveList(rec->FileName(),buf2);
                    PRINTF("Src2: \033[0;91m'%s'\033[0m\n", rec->FileName());
                    PRINTF("Dest2: \033[0;92m'%s'\033[0m\n\n", buf2);
                    ++count_moved;
                    free(buf2);
                }
                free(buf);
            }
        }
    }
        return count_moved;

}

cCopyFileMenu::~cCopyFileMenu() {}

cCopyFileMenu::cCopyFileMenu(std::string oldDir) : cOsdMenu(tr(DIR_SELECTOR),4)
{
    BaseDir = "/media";
    pwd = BaseDir;

    selectionMenuNeeded = false;
    int cntrHDs = 0;
    int firstHD = -9;
    struct stat fstat_media;
    if(stat("/media", &fstat_media) == 0) {

        int i;
        struct statvfs fsstat;
        struct stat fstat;
        char path[12] = "/media/hd/";
        if (stat(path, &fstat) == 0) { // path exists
            if (fstat.st_dev != fstat_media.st_dev && statvfs(path, &fsstat) == 0) { // not on the same FS
                firstHD = -1;
                cntrHDs++;
            }
        }
        for (i = 0; i < 10; i++) {
            path[9] = i+48;
            path[10] = '/';
            path[11] = '\0';
            if (stat(path, &fstat) == 0) { // path exists
                if (fstat.st_dev != fstat_media.st_dev && statvfs(path, &fsstat) == 0) { // not on the same FS
                    if (firstHD == -9)
                        firstHD = i;
                    cntrHDs++;
                }
            }
        }
    }

    OldDir = oldDir;

    size_t index = OldDir.find(".rec");

    // if .rec is at the end of OldDir string the it is a recording
    if (index != std::string::npos && index + 4 == OldDir.length()) {
        dirSuffix = GetLastNTokens(OldDir, '/', 2);
        copyRecording = true;
    } else { // give is a directory
        dirSuffix = GetLastNTokens(OldDir, '/', 1);
        copyRecording = false;
    }

    if (cntrHDs != 0) {
        if (cntrHDs == 1) {
            if (firstHD == -1)
                CopyUsingThread(OldDir, "/media/hd/recordings");
            else {
                std::string path = "/media/hd";
                path += firstHD+48;
                path += "/recordings";
                CopyUsingThread(OldDir, path.c_str());
            }
        } else
            selectionMenuNeeded = true;
    }
    Set();
}

// show contents of 'pwd'
void cCopyFileMenu::Set()
{
   Clear();


    if (!selectionMenuNeeded) {
        Add(new cOsdItem(tr("Copying started")));
        return;
    }

    DIR *dp;
    struct dirent *dirp;
    std::string dirEntry;

    dp = opendir(pwd.c_str());

    // could not open Dir 'pwd'
    if ( !dp ) {
        printf("\033[0;91mCould not open \033[0m'%s'\n",pwd.c_str());
        return;
    }

    std::string titleStr;
    /*get the current directory name*/
    std::string c_dir = CurrentDir(pwd);
    titleStr = std::string(tr(DIR_SELECTOR)) + std::string(": ") + c_dir;
    SetTitle(titleStr.c_str());

//     /* show pwd */
//     Add(new cOsdItem( pwd.c_str(), osUnknown, false));
//     Add(new cOsdItem( "", osUnknown, false));

    // Add parent dir '..'
    if (pwd != "/" && pwd != "/media")
        Add(new cOsdItem( AddFolderUpIcon("..").c_str(), osUser1, true));

    std::vector<std::string> entries; // needed to able to sort the entries

    // Add sub-directories
    while((dirp = readdir(dp)) != NULL) {
        dirEntry = dirp->d_name;

        // exclude all hidden files
        if ( dirEntry.empty() || dirEntry[0]=='.' || dirEntry == "LiveBuffer" ) continue;
        // exclude all files ending with ".rec" & ".del"
        if ( IsRecDir(dirEntry) ) continue;

        if (IsDir(pwd + "/" + dirEntry)) {
            // Sub-Directory
            if (IsRealRecSubDir(pwd + "/" + dirEntry))
                entries.push_back(dirEntry);
        }
    }
    closedir(dp);

    std::sort(entries.begin(), entries.end(), stringCompareCaseInsensitive);

    for (unsigned int i = 0; i < entries.size(); i++)
#if 1
        Add(new cOsdItem(AddFolderIcon(entries.at(i)).c_str(), osUser1, true));
#else
    {
        char tmp[256];
        strcpy(tmp, entries.at(i).c_str());
        char *name = ExchangeChars(tmp,false);
        Add(new cOsdItem(AddFolderIcon(name).c_str(), osUser1, true));
    }
#endif
    if(pwd != VideoDirectory)
    {
        SetHelp(tr("Cancel"), tr("Move"), tr("Rec Dir"), tr("Button$Create"));
    }
    else
    {
         SetHelp(tr("Cancel"), tr("Move"), NULL, tr("Button$Create"));
    }
    Display();
}

eOSState cCopyFileMenu::ProcessKey(eKeys key) {
    eOSState state = cOsdMenu::ProcessKey(key);

    if (HasSubMenu()) {
        if (state == osUser2) { // reload page
            CloseSubMenu();
            Set();
        }

        return state;
    }


    if (!selectionMenuNeeded && key != kNone) {
        pwd = VideoDirectory;
        return osBack;
    }

    // no Sub-Menus
    switch(key) {
        case kOk:
        {
            if (state == osUser1) { // Directory Item
                    std::string dir;
                    cOsdItem *currItem = Get(Current());
                    if (!currItem || ! currItem->Text()) // empty item OR empty text
                        break;

                    dir = currItem->Text();
                    dir = RemoveIcon(dir);

                    // parent dir
                    if (dir == "..") {
                        pwd = ParentDir(pwd);
                    } else { // open sub-dir
                        if (pwd == "/") // avoid two '/' in the beginning
                            pwd = pwd  + dir;
                        else
                            pwd = pwd + std::string("/") + dir;
                    }

                    Set();
                }
        }
        break;

        case kGreen:
            {
                printf("OldDir : '%s'\n", OldDir.c_str());
                std::string newDir="";
                if (Get(Current()) && Get(Current())->Text()) {
                    newDir =  Get(Current())->Text();
                    printf("Selected : '%s'\n", RemoveIcon(newDir).c_str());
                    newDir = AddSubDir(pwd, RemoveIcon(newDir));
                    newDir = pwd;

                    // check if copying into its own subdirectory!
                    if (newDir.find(OldDir) == 0) {
                        Skins.Message(mtError, tr("Cannot copy into its own sub folder"));
                        return osContinue;
                    }

                    // Check for write permission in destination-directory
                    if (access(newDir.c_str(), W_OK) != 0) {
                        Skins.Message(mtError, tr("Cannot write into directory"));
                        return osContinue;
                    }

                    // copy recordings via Cutter queue Thread
                    if (CopyUsingThread(OldDir, newDir) <= 0) {
                        Skins.Message(mtError, tr("Copy failed. Not enough free space"));
                        return osContinue;
                    }
                    return osUser3; // close every menu and go back to recording's list menu
                } // if (Get())
            }// kGreen
            break;

        case kYellow: // Reset, get back to videodir
            pwd = VideoDirectory;
            Set();
            break;

        case kBlue: // Create a sub dir
            AddSubMenu(new cCreateDirMenu(pwd));
            return osContinue;
            break;

        case kRed: // Cancel
            return osBack;

        case kBack:
            /** TB: Ibo wants it like that: */
            return osBack;
            /*exit if pwd is /media/hd/recordings || /media */
#if 0
            if (pwd == VideoDirectory || pwd == "/media") return osBack;

            pwd = ParentDir(pwd);
            Set();
            break;
#endif

        default:
            break;
    }

    return osContinue;
}

int cCopyFileMenu::CopyUsingThread(std::string srcDir, std::string destDir) {
    int freeSpaceMB = FileSystemAvailableSpaceMB(destDir);

    cThreadLock RecordingsLock(&Recordings);
    int recordingsSizeMB = RecordingsSizeMB(srcDir);
    //int recordingsSizeMB = 0;
    int count_copied = 0;

    // not enough space in disk
    if (recordingsSizeMB > freeSpaceMB) {
        return count_copied;
    }

    printf("\nFreeSpace : %i MB\n", freeSpaceMB);
    printf("Recs Size MB : %i MB\n", recordingsSizeMB);

    printf("'%s' ->\n", srcDir.c_str());
    printf("'%s'\n\n", destDir.c_str());
    if (copyRecording) { // move one recording
        std::string dest = destDir + dirSuffix;
        printf("Src1: \033[0;91m'%s'\033[0m\n", srcDir.c_str());
        printf("Dest1: \033[0;92m'%s'\033[0m\n\n", dest.c_str());
        MoveCutterThread->AddToCopyList( srcDir.c_str(), dest.c_str() );
        ++count_copied;
    } else { // move a directory containing one or more recordings
        for(cRecording *rec = Recordings.First(); rec; rec = Recordings.Next(rec)) {
            if(!strncmp(srcDir.c_str(), rec->FileName(), srcDir.length()))
            {
                //char *buf=ExchangeChars(strdup(srcDir.c_str()+strlen(VideoDirectory)+1),false);
                char *buf = strdup(srcDir.c_str() + strlen(VideoDirectory)+1);

                if(strcmp(rec->Name(), buf)) {
                    char *buf2;
                    //asprintf(&buf2,"%s%s",destDir.c_str(),rec->FileName()+ srcDir.length());
                    char *posSlash = (char *) strchr((const char*)buf, '/');
                    asprintf(&buf2, "%s/%s%s", destDir.c_str(), posSlash ? posSlash + 1 : buf, rec->FileName() + srcDir.length());

                    MoveCutterThread->AddToCopyList(rec->FileName(), buf2);
                    printf("Src2: \033[0;91m'%s'\033[0m\n", rec->FileName());
                    printf("Dest2: \033[0;92m'%s'\033[0m\n\n", buf2);
                    ++count_copied;
                    free(buf2);
                }
                free(buf);
            }
        }
    }
   return count_copied;
}


cCreateDirMenu::cCreateDirMenu(const std::string& base) : cOsdMenu(tr("Create directory"),15) {
    baseDirName = base;
    strncpy(newDirName, tr("New"), 63);
    Set();
}

void cCreateDirMenu::Set() {
    Clear();
    Add(new cOsdItem(tr("To create a new directory press 'OK':"), osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));

    Add(new cMenuEditStrItem(tr("Directory Name"), newDirName, 63, trVDR(FileNameChars)));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem(tr("Note:"), osUnknown, false));
    Add(new cOsdItem(tr("Directory will only be shown with"), osUnknown, false));
    Add(new cOsdItem(tr("recordings inside."), osUnknown, false));

    Display();
    SetHelp(tr("Cancel"));

}

eOSState cCreateDirMenu::ProcessKey(eKeys key) {
    eOSState state = cOsdMenu::ProcessKey(key);

    if (state == osUnknown) {
        switch (key) {
            case kRed: // cancel
                return osBack;

            //case kOk:
            case kOk: // create
                if ( mkdir( AddSubDir(baseDirName, newDirName).c_str(), 0777) == 0) {
                    Skins.Message(mtInfo, tr("Created"));
                    return osUser2;
                } else {
                    Skins.Message(mtError, tr("Failed"));
                    return osBack;
                }
                break;

            default:
                return osContinue;
        } // switch
    } else {
        if (key == kOk) { // out of edit mode
            SetHelp(tr("Cancel"));
        }
    }

    return state;
}

cCompressRecordingMenu::cCompressRecordingMenu(std::string cmd, std::string dir) : cOsdMenu(trVDR("Compress recording"), 30), Cmd(cmd), Dir(dir), RecInfo(dir.c_str()) {
	if(!RecInfo.Read()) printf("Failed to read recording info for \"%s\"\n", dir.c_str());
	std::string base = Dir.substr(0, dir.rfind('-'));
	for(int i=0; i<10; i++) {
		Target = cString::sprintf("%s-%d.rec", base.c_str(), i);
		struct stat st;
		if(stat(Target.c_str(), &st) != 0)
			break;
	} // for
/*
printf("-------cCompressRecordingMenu::cCompressRecordingMenu %s HD %d CMP %d Event %p (%s)\n", dir.c_str(), RecInfo.IsHD(), RecInfo.IsCompressed(), RecInfo.GetEvent(), cmd.c_str());
if(RecInfo.GetEvent()) {
	const cComponents *c = RecInfo.GetEvent()->Components();
	if(c) {
		for(int i=0; i < c->NumComponents();i++) {
			printf("->%s\n", (const char *)c->Component(i)->ToString());
		}
	} // if
} // if
*/
	HighCompression  = true;
	GetAdditionalRecInfo();
	Set();
} // cCompressRecordingMenu::cCompressRecordingMenu

cCompressRecordingMenu::~cCompressRecordingMenu() {
} // cCompressRecordingMenu::~cCompressRecordingMenu

eOSState cCompressRecordingMenu::ProcessKey(eKeys key) {
    eOSState state = cOsdMenu::ProcessKey(key);

    if (state == osUnknown) {
        switch (key) {
            case kOk:
                SystemExec(cString::sprintf("%s %s %s %lld '%s' '%s'", Cmd.c_str(), 
                           HighCompression ? "HIGH" : "LOW", 
                           RecInfo.IsHD() ? "HD" : "SD", 
                           duration ? (size*8 / duration) : 0, 
                           Dir.c_str(), Target.c_str()), true);
                return osBack;
            default:
                ;
        } // switch
    } // if
    return state;
} // cCompressRecordingMenu::ProcessKey

void cCompressRecordingMenu::Set() {
    Clear();
    Add(new cOsdItem(tr("Compression options"), osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));
    Add(new cMenuEditBoolItem(tr("Compression"), &HighCompression, tr("High"), tr("Low")));
//    Add(new cOsdItem("", osUnknown, false));
//    Add(new cOsdItem(tr("Audio tracks"), osUnknown, false));
//    Add(new cOsdItem("", osUnknown, false));
    // Add(new cOsdItem("", osUnknown, false));
} // cCompressRecordingMenu::Set

void cCompressRecordingMenu::GetAdditionalRecInfo() {
	duration = 0;
	size     = 0;
	int fps = RecInfo.FramesPerSecond();
	if(fps <= 0) fps = 25;
	int frames = cIndexFile::GetLength(Dir.c_str(), false); // We currently don't support pes recordings
	if(frames > 0) duration = frames/fps;
	struct stat st;
	int fpos = 1;
	while((fpos <= 0xFFFFF) && (stat(cString::sprintf("%s/%05d.ts", Dir.c_str(), fpos++), &st) == 0))
		size += st.st_size;
	int bps = duration ? size*8 / duration : 0;
	printf("duration %ds (%dmin) size %lldm bps: %d\n", duration, duration/60, size/(1024*1024), bps);
} // GetAdditionalRecInfo
