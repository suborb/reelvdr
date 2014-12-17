#ifndef TOOLS_H
#define TOOLS_H

#include "setup.h"

#define RIPIT_LOG "/tmp/ripit.log"

void AbortEncoding(void);
void StartEncoding(const char *Device);
/* returns true if ripping is currently running */
bool IsRipRunning(void);
/* return true if lame is installed */
bool IsLameInstalled(void);

#endif
