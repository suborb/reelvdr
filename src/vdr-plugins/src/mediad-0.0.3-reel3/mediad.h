#ifndef VDR_MEDIAD_H
#define VDR_MEDIAD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vdr/debug.h"
#include "devicesetup.h"

//#define DEBUG_MEDIAD

#if defined DEBUG
#   define DPRINT(x...) DDD(x)
#else
# define DPRINT(x...)
#endif

#define BUF_SIZ 4096
#define MAXFILESPERRECORDING 255
#define FINDCMD   "find %s -follow -type f -name '%s' 2> /dev/null"
#define FINDICMD   "find %s -follow -type f -iname '%s' 2> /dev/null"
#define FINDMOUNTPOINT   "cat /etc/mtab | grep  %s | awk '{ print $2}'"

#endif // VDR_MEDIAD_H
