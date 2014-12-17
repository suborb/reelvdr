/*
 * config.c
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: config.c,v 1.1 2004/11/30 19:52:14 schmitzj Exp $
 *
 */

#include "config.h"

#include <getopt.h>
#include <unistd.h>
#include <vdr/menuitems.h>

timelineConfig timelineCfg;

timelineConfig::timelineConfig(void)
{
	ignorePrimary=true;
}
timelineConfig::~timelineConfig()
{
}

bool timelineConfig::SetupParse(const char *Name, const char *Value)
{
	if      (strcmp(Name,"ignorePrimary")==0) ignorePrimary = atoi(Value);
	else 
		return false;

	return true;
}
const char *timelineConfig::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
	return "";
}

bool timelineConfig::ProcessArgs(int argc, char *argv[])
{ 
	return true;
}

// ----------------------------------------------------------------------

timelineConfigPage::timelineConfigPage(void) : cMenuSetupPage()
{
	m_NewConfig = timelineCfg;

	Add(new cMenuEditBoolItem(tr("Ignore primary interface"),
			&m_NewConfig.ignorePrimary));
}

timelineConfigPage::~timelineConfigPage()
{
}

void timelineConfigPage::Store(void)
{
	SetupStore("ignorePrimary", m_NewConfig.ignorePrimary);

	timelineCfg = m_NewConfig;
}
