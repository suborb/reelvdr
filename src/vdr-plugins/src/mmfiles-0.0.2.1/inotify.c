#include "inotify.h"
#include "cache.h"
#include "scanDir.h"

#include "CacheTools.h"

std::vector <std::string> GetSubDirs( const std::string &baseDir)
{
   DIR* dirp;
   struct dirent64 *dp;
   struct stat64 buf;

   std::vector <std::string> subDirs;
   if  ((dirp = opendir(baseDir.c_str())) == NULL)
    return subDirs;

    
    std::string path;
    std::string filename;

    while((dp = readdir64(dirp)) != NULL)
    {
        filename = dp->d_name;
        if ( IsExcludedFile(filename) ) continue;
        if (filename[0] == '.') continue; // exclude
     
        path = baseDir + "/" + filename;
        if ( lstat64(path.c_str(), &buf) == 0 &&  S_ISDIR(buf.st_mode))
        {
            if (filename == "PC Directory") // a fix for MediaTomb , search only this directory
            {    
                subDirs.clear();
                subDirs.push_back(filename);
                closedir(dirp);
                return subDirs;
            }
            subDirs.push_back(filename);
        }
    }

    closedir(dirp);
    return subDirs;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


bool cInotify::DeleteWatchFromList(const int wd)
{
    unsigned int i=0;

    for( ; i < Watches.size(); ++i)
        if ( Watches.at(i).WatchHandle() == wd) break;

    if ( i < Watches.size() ) // found
        Watches.erase( Watches.begin() + i);
    else
        return false;

    return true;
}


std::string cInotify::Wd2Path(const int wd)
{
    unsigned int i=0;

    for( ; i < Watches.size(); ++i)
        if ( Watches.at(i).WatchHandle() == wd) return Watches.at(i).Path();

    return std::string(); // not found
}




cInotify::cInotify()
{
    fd = inotify_init();
    if (fd < 0)
        perror ("inotify_init");
}




cInotify::~cInotify()
{
    int ret;

    ret = close (fd);
    if (ret)
        perror ("close");
}




bool cInotify::WatchDir( const std::string& path, bool recursive)
{
    //if ( !FileTools::IsDir(path) ) return false;

    //int wd = inotify_add_watch (fd, path.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE);
    int wd = inotify_add_watch (fd, path.c_str(),  IN_CREATE | IN_DELETE);

    if (wd < 0)
    { 
        perror ("inotify_add_watch"); 
        return false ; 
    }

    cWatch watch(path, wd);
    Watches.push_back(watch);
    //printf("Watching: %s\n", path.c_str());

    if (recursive)
    {
        // get subdirectories

        std::vector <std::string> dirs;
        dirs = GetSubDirs(path);
        for (unsigned int i=0; i<dirs.size(); ++i)
        {
                WatchDir( path + "/" + dirs.at(i), true);
        }

    } // recursive
    return true;
}




bool cInotify::RemoveWatch(const int wd)
{
    int ret;

    ret = inotify_rm_watch (fd, wd);
    if (ret)
        perror ("inotify_rm_watch");
    else
        {
            DeleteWatchFromList(wd);
            return true; // ret == 0 sucess
        }

    return false;
}




void cInotify::InotifyEvents()
{
    int i, len;
    char buf[BUF_LEN];
    printf("%i\t", Watches.size());
    //if (Watches.size() <= 0) WatchDir("/home/reel/av");
    //while(1)
    {
        //usleep(100);

        i = 0;
        len = read (fd, buf, BUF_LEN);
        //printf("len %i\n",len);

        if (len < 0) 
        {
            if (errno == EINTR) ;
            /* need to reissue system call */
            else
                perror ("read");
        } else if (!len) 
            /* BUF_LEN too small? */
            printf("BUF_LEN too small?\n");
        else
            while (i < len) 
            {

                struct inotify_event *event;
                event = (struct inotify_event *) &buf[i];

                //printf ("wd=%d mask=%u cookie=%u len=%u\n", event->wd, event->mask, event->cookie, event->len);

                //if (event->len)
                //    printf ("name=%s\n", event->name);

                i += EVENT_SIZE + event->len;

                // if event == create new dir
                // WatchDir(new dir, true)

                printf("%s/%s (%i) ", Wd2Path(event->wd).c_str(), event->len? event->name:"", event->wd );
                std::string full_path =  Wd2Path(event->wd);
                full_path += event->len? std::string("/") + event->name:"";

                // Watch has been auto-removed
                if (event->mask & IN_IGNORED)
                {
                    printf("Ignored");
                    //RemoveWatch(event->wd);
                    DeleteWatchFromList(event->wd);
                    RemoveFromCache( full_path ) ;

                    // Remove from Cache
                }
                if (event->mask & IN_DELETE_SELF)
                    printf("Deleted Self");
                if (event->mask & IN_MOVE_SELF)
                    printf("Moved Self");
                if ( event->mask & IN_DELETE)
                {
                    printf("Deleted");
                    RemoveFromCache( full_path ) ;
                    //if (event->mask & IN_ISDIR)
                    //    RemoveWatch(event->wd);
                    cScanDir scan;
                    scan.SetRecursive(false);
                    scan.ScanDirTree( Wd2Path(event->wd));
                }
                if ( event->mask & IN_CREATE)
                {
                    // Scan dir
                    // Scan since the dir could "become" a recoganised dir like VdrRec
                    cScanDir scan;
                    scan.ScanDirTree( Wd2Path(event->wd));


                    printf("Created");
                    if ( event->mask & IN_ISDIR) 
                        WatchDir( Wd2Path(event->wd) + "/" +  event->name);
                }
                if ( event->mask & IN_MODIFY)
                    printf("Modified");

                printf("  (%x)\n", event->mask);
            }

    }
}




