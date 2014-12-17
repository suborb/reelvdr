#ifndef __MYDEBUG_DVDSWITCH_H
#define __MYDEBUG_DVDSWITCH_H

#include <stdio.h>

#ifdef DEBUG
#define MYDEBUG(a...) DebugLog.WriteLine(__FILE__, __LINE__, a)
#else
#define MYDEBUG(a...)
#endif

class cDebugLog
{
  private:
    char *FileName;
    FILE *File;
    

    bool Open(void);
    void Close(void);
  public:
    cDebugLog(void);
    ~cDebugLog(void);

    bool SetLogFile(const char *filename);
    void WriteLine(const char *file, int line, const char *format, ...);
    void End(void);
};

extern cDebugLog DebugLog;

#endif // __MYDEBUG_DVDSWITCH_H
