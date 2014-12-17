#ifndef SELECTFILE_H
#define SELECTFILE_H

#include <vdr/tools.h>
#include <vdr/osdbase.h>

#include <string>

#include <sys/stat.h>
#include <sys/types.h> 
#include <dirent.h>
#include <errno.h>

#include "mymenurecordings.h"

bool InSameFileSystem(const std::string& path1, const std::string& path2);

/* 
 * Shows a Directory listing like filebrowser
 **/

 /*
  * showEmptyDir
  * showDirsOnly
  * SelectFileOnly
  * SelectDirOnly (?)
  */

class cSelectfileMenu: public cOsdMenu
{
    private:
        std::string pwd;
        std::string OldDir;
        std::string BaseDir; // do not go below this
        std::string dirSuffix;
        bool ShowDirsOnly;
        bool moveRecording; // false ==> moving directories

        // this contains either the directory / recording to be moved
        myMenuRecordingsItem * Item;
    public:
        eOSState ProcessKey(eKeys key);
        void Set(); // show dir contents
        //cSelectfileMenu(myMenuRecordingsItem*, std::string base = "/", bool ShowDirsOnly=true);
        cSelectfileMenu(std::string oldDir, std::string base = "/", bool ShowDirsOnly=true);
        ~cSelectfileMenu();
        int MoveUsingThread(std::string srcDir, std::string Destdir);
};

class cCopyFileMenu: public cOsdMenu
{
    private:
        std::string pwd;
        std::string OldDir;
        std::string BaseDir; // do not go below this
        std::string dirSuffix;
        bool copyRecording; // false ==> moving directories
        bool selectionMenuNeeded;

        // this contains either the directory / recording to be moved
        myMenuRecordingsItem * Item;
    public:
        eOSState ProcessKey(eKeys key);
        void Set(); // show dir contents
        cCopyFileMenu(std::string dir);
        ~cCopyFileMenu();
        int CopyUsingThread(std::string srcDir, std::string Destdir);
};

class cCreateDirMenu:public cOsdMenu
{
    private:
        char newDirName[64];
        std::string baseDirName;

    public:
        explicit cCreateDirMenu(const std::string& base );
        void Set();
        eOSState ProcessKey(eKeys);
};

class cCompressRecordingMenu: public cOsdMenu
{
    private:
        std::string Cmd;
        std::string Dir;
        std::string Target;
        cRecordingInfo RecInfo;
        int HighCompression;
        int ReduceResolution;
        uint32_t duration;
        uint64_t size;
    public:
        cCompressRecordingMenu(std::string cmd, std::string dir);
        ~cCompressRecordingMenu();
        eOSState ProcessKey(eKeys key);
        void Set();
        void GetAdditionalRecInfo();
};

#endif
