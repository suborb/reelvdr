/*************************************************************** -*- c++ -*-
 *       Copyright (c) 2003,2004 by Marcel Wiesweg                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "storage.h"

int Storage::doCleanUp() {
   DIR *top=opendir(root);
   int pagesDeleted=0;
   if (top) {
      struct dirent *chandir, path;
      struct stat chandirstat;
      char fullPath[PATH_MAX];
      while ( (!readdir_r(top, &path, &chandir) && chandir != NULL) ) {
         if (strcmp(chandir->d_name, "..")==0 || strcmp(chandir->d_name, ".")==0)
            continue;
         snprintf(fullPath, PATH_MAX, "%s/%s", root, chandir->d_name);
         if (stat(fullPath, &chandirstat)==0) {
            if (S_ISDIR(chandirstat.st_mode)) {
               pagesDeleted+=cleanSubDir(fullPath);
            }
         }
      }
      closedir(top);
   } else {
      esyslog("OSD-Teletext: Error opening teletext storage directory \"%s\": %s", root, strerror(errno));
   }
   return pagesDeleted;
}

int Storage::cleanSubDir(const char *dir) {
   static bool reportedError=false; //avoid filling up syslog
   DIR *d=opendir(dir);
   bool hadError=false;
   int bytesDeleted=0;
   if (d) {
      struct dirent *txtfile, path;
      struct stat txtfilestat;
      char fullPath[PATH_MAX];
      int filesize;
      while ( (!readdir_r(d, &path, &txtfile) && txtfile != NULL) ) {
         int len=strlen(txtfile->d_name);
         //check that the file end with .vtx to avoid accidents and disasters
         if (strcmp(txtfile->d_name+len-4, ".vtx")==0) {
            snprintf(fullPath, PATH_MAX, "%s/%s", dir, txtfile->d_name);
            stat(fullPath, &txtfilestat);
            filesize=actualFileSize(txtfilestat.st_size);
            int ret=unlink(fullPath);
            if (ret==0)
               bytesDeleted+=filesize;
            else
               hadError=ret;
         }
      }
      closedir(d);
      rmdir(dir);
   } else {
      if (!reportedError) {
         esyslog("OSD-Teletext: Error opening teletext storage subdirectory \"%s\": %s", dir, strerror(errno));
         reportedError=true;
      }
   }

   if (hadError && !reportedError) {
      esyslog("OSD-Teletext: Error removing teletext storage subdirectory \"%s\": %s", dir, strerror(hadError));
      reportedError=true;
   }
   return bytesDeleted;
}

Storage::Storage()
  : byteCount(0), failedFreeSpace(false)
{
}

Storage::~Storage() {
}

void Storage::freeSpace() {
   //there might be a situation where only the current directory is left and
   //occupies the whole space. We cannot delete anything. Don't waste time scanning.
   if (failedFreeSpace)
      return;

   //printf("freeSpace()\n");
   time_t min=time(0);
   char minDir[PATH_MAX];
   char fullPath[PATH_MAX];
   DIR *top=opendir(getRootDir());
   if (top) {
      int haveDir=0;
      struct dirent *chandir, path;
      struct stat chandirstat;
      while ( (!readdir_r(top, &path, &chandir) && chandir != NULL) ) {
         if (strcmp(chandir->d_name, "..")==0 || strcmp(chandir->d_name, ".")==0)
            continue;
         snprintf(fullPath, PATH_MAX, "%s/%s", getRootDir(), chandir->d_name);
         if (stat(fullPath, &chandirstat)==0) {
            if (S_ISDIR(chandirstat.st_mode)) {
               if (chandirstat.st_ctime < min && strcmp(fullPath, currentDir)) {
                  min=chandirstat.st_ctime;
                  strcpy(minDir, fullPath);
                  haveDir++;
               }
            }
         }
      }
      closedir(top);

      //if haveDir, only current directory present, which must not be deleted
      if (haveDir>=2)
         byteCount-=cleanSubDir(minDir);
      else
         failedFreeSpace=true;
   }
}

bool Storage::exists(const char* file) {
   struct stat s;
   return (stat(file, &s)==0);
}

void Storage::getFilename(char *buffer, int bufLength, PageID page) {
   snprintf(buffer, bufLength, "%s/%s/%03x_%02x.vtx", getRootDir(),
            *page.channel.ToString(), page.page, page.subPage);
}

void Storage::prepareDirectory(tChannelID chan) {
   currentDir = cString::sprintf("%s/%s", root, *chan.ToString());
   if (!MakeDirs(currentDir, 1)) {
      esyslog("OSD-Teletext: Error preparing directory for channel \"%s\"",
              *chan.ToString());
      return;
   }
   failedFreeSpace=false;
}
