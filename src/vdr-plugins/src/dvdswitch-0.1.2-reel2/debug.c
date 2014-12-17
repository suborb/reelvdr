#include <vdr/plugin.h>
#include <stdarg.h>
#include "debug.h"

// --- cDebugLog -------------------------------------------------------

cDebugLog DebugLog;

cDebugLog::cDebugLog(void)
{
  FileName = NULL;
  File = NULL;
}

cDebugLog::~ cDebugLog(void)
{
  Close();
  free(FileName);
}

bool cDebugLog::Open(void)
{
  File = fopen(FileName, "a");
  if(File)
  {
    MYDEBUG("---------");
    MYDEBUG("Neuer Log");
    MYDEBUG("---------");
    return true;
  }

  return false;
}

void cDebugLog::Close(void)
{
  if(File)
    fclose(File);
}

bool cDebugLog::SetLogFile(const char *filename)
{
  if(filename)
    FileName = strdup(filename);

  return Open();
}

void cDebugLog::WriteLine(const char *file, int line, const char *format, ...)
{
  if(File)
  {
    char *fmt;
    asprintf(&fmt, "DVDSWITCH(%s,%d): %s", file, line, format);
    va_list ap;
    va_start(ap, format);
    vfprintf(File, fmt, ap);
    va_end(ap);
    fprintf(File, "\n");
    fflush(File);
    free(fmt);
  }
}

void cDebugLog::End(void)
{
  Close();
}
