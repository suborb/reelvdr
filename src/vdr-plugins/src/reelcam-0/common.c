#include "common.h"
#include "stdio.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DEV_DVB_ADAPTER "/dev/dvb/adapter"
#ifndef PATH_MAX
#define PATH_MAX        512
#endif

static const char *DvbName(const char *Name, int n)
{
  static char buffer[PATH_MAX];
  snprintf(buffer, sizeof(buffer), "%s%d/%s%d", DEV_DVB_ADAPTER, n, Name, 0);
  return buffer;
}

int DvbOpen(const char *Name, int n, int Mode)
{
  const char *FileName=DvbName(Name,n);
  int fd=open(FileName,Mode); // TODO: check for error
  return fd;
}

