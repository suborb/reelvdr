/*
 * config.h
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: config.h,v 1.1 2004/11/30 19:52:14 schmitzj Exp $
 *
 */

#ifndef TVONSCREEN_CONFIG_H
#define TVONSCREEN_CONFIG_H

#include <vdr/plugin.h>
#include <vdr/config.h>
#include <vdr/menuitems.h>

class timelineConfig
{
public:
	int ignorePrimary;

	timelineConfig(void);
	~timelineConfig();
	bool SetupParse(const char *Name, const char *Value);
	bool ProcessArgs(int argc, char *argv[]);
	const char *CommandLineHelp(void);
};

extern timelineConfig timelineCfg;

class timelineConfigPage : public cMenuSetupPage
{
private:
	timelineConfig m_NewConfig;
	
protected:
	virtual void Store(void);

public:
	timelineConfigPage(void);
	virtual ~timelineConfigPage();
};

#endif 


