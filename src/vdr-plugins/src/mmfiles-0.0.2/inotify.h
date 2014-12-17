
#ifndef __MMFILES_INOTIFY_H
#define __MMFILES_INOTIFY_H

#include <sys/inotify.h>
#include <errno.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h> 
#include <dirent.h>
#include "scanDir.h"
/* size of the event structure, not counting name */
#define EVENT_SIZE  (sizeof (struct inotify_event))
/* reasonable guess as to size of 1024 events */
#define BUF_LEN        (1024 * (EVENT_SIZE + 16) )



std::vector <std::string> GetSubDirs( const std::string &baseDir);


struct cWatch
{
    std::string path_;
    int wd_;

    public:
    cWatch(const std::string&path="", const int wd=-1): path_(path), wd_(wd) { }
    std::string Path() const { return path_;}
    int WatchHandle() const { return wd_; }
};




class cInotify
{
    int fd; // inotify handle
    std::vector <cWatch> Watches; // std::vector of directories being monitored
    bool DeleteWatchFromList(const int wd);

    public:
    cInotify();
    ~cInotify();
    bool WatchDir(const std::string& path, bool recursive=true);
    bool RemoveWatch(const int wd);
    std::string Wd2Path(const int wd);
    void InotifyEvents();

};


#endif

