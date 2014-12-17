#ifndef INSTALL_H
#define INSTALL_H

#include "vdr/i18n.h"
#include "vdr/config.h"

static const char *MAINMENUENTRY = trNOOP("Installation Wizard");
#define MAX_LETTER_PER_LINE 48

#if VDRVERSNUM >= 10716
extern bool loopMode;
#endif

#endif
