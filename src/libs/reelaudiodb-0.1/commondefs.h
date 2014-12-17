#ifndef COMMONDEFS_H
#define COMMONDEFS_H

#include <string>
#include <queue>
#include "thread.h"

#define MAX_LINK_LEVEL 6

struct cPath
{
    std::string filename;  /* just the name of the file */
    std::string path;      /* absolute path including the filename */
    size_t linkLevel;      /* number of symlinks in the path; lets not have too many of them. High number indicates "looping" */
};

/**
 * @brief The cDirectoryQueue class
 * hold a list/queue of directories to be traversed
 */
class cDirectoryQueue : public cMutex
{
    std::queue<cPath> dirQ;
public:
    bool AddPath(const cPath& path)
    {
        cMutexLock mutexLock(this);
        dirQ.push(path);
        return true;
    }

    int Size()
    {
        cMutexLock mutexLock(this);
        return dirQ.size();
    }

    bool Empty()
    {
        cMutexLock mutexLock(this);
        return dirQ.empty();
    }

    cPath Pop()
    {
        cMutexLock mutexLock(this);
        cPath path = dirQ.front();
        dirQ.pop();

        return path;
    }

};


#endif // COMMONDEFS_H
