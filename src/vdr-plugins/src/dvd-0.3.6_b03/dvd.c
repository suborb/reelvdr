/*
 * DVD Player plugin for VDR
 *
 * Copyright (C) 2001.2002 Andreas Schultz <aschultz@warp10.net>
 *
 * This code is distributed under the terms and conditions of the 
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

#ifdef HAVE_CONFIG_H 
#include "config.h"
#endif

#include "dvd.h"

// --- cPluginDvd ------------------------------------------------------------
cPluginDvd::cPluginDvd(void)
{
  // Initialize any member varaiables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginDvd::~cPluginDvd()
{
  // Clean up after yourself!
}

const char *cPluginDvd::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return   "  -C DEV,   --dvd=DEV      use DEV as the DVD device (default: /dev/dvd)\n";
}

bool cPluginDvd::ProcessArgs(int argc, char *argv[])
{
    // Implement command line argument processing here if applicable.
#ifndef __QNXNTO__
    static struct option long_options[] = {
	{ "dvd",      required_argument, NULL, 'C' },
	{ NULL }
    };
#endif
    
    int c;
#ifndef __QNXNTO__
    while ((c = getopt_long(argc, argv, "C:", long_options, NULL)) != -1) {
#else
    optind = 1;
    while ((c = getopt(argc, argv, "C:")) != -1) {
#endif
        switch (c) {
	    case 'C':
		fprintf(stderr, "arg: %s\n", optarg);
		cDVD::SetDeviceName(optarg);
		if (!cDVD::DriveExists()) {
		    fprintf(stderr, "vdr: DVD drive not found: %s\n", optarg);
		    return false;
		}
		break;
	    default:
		fprintf(stderr, "arg char: %c\n", c);
		return false;
	}
    }
    return true;
}

bool cPluginDvd::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  RegisterI18n(DvdPhrases);
  return true;
}

bool cPluginDvd::Start(void)
{
  // Start any background activities the plugin shall perform.
  return true;
}

void cPluginDvd::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

cOsdMenu *cPluginDvd::MainMenuAction(void)
{
    if (!cDvdPlayerControl::DVDActive())
      cControl::Launch(new cDvdPlayerControl);
    else {
    }
    
    return NULL;
}

cMenuSetupPage *cPluginDvd::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return new cMenuSetupDVD;
}

bool cPluginDvd::SetupParse(const char *Name, const char *Value)
{
    return DVDSetup.SetupParse(Name, Value);
}

cSetupLine *cPluginDvd::GetSetupLine(const char *Name, const char *Plugin)
{
  	for (cSetupLine *l = Setup.First(); l; l = Setup.Next(l)) {
	      if ((l->Plugin() == NULL) == (Plugin == NULL)) {
              if ((!Plugin || strcasecmp(l->Plugin(), Plugin) == 0) && strcasecmp(l->Name(), Name) == 0)
		    return l;
              }
        }
        return NULL;
}

VDRPLUGINCREATOR(cPluginDvd); // Don't touch this!





