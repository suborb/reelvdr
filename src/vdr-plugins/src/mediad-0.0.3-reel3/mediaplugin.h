#ifndef __MEDIIAPLUGIN_H
#define __MEDIIAPLUGIN_H

#include <vector>
#include <vdr/config.h>
#include <vdr/interface.h>
#include <vdr/plugin.h>
#include <vdr/menu.h>
#include <vdr/debugmacros.h>
#include <time.h>
#include <stdlib.h>
#include <vdr/dvdIndex.h>

#include "status.h"
#include "mediad.h"
#include "hal.h"

using std::vector;

enum searchMode {
	Or,
	And
};

enum searchType {
	State,
	Find,
	MediaType
};

// structure used to define a search pattern
class pattern {
public:
	pattern(searchType sMethod, mediaType mType,  const char* description, bool caseSens = false)
		: caseSensitive(caseSens), searchMethod(sMethod), mediaT(mType)
	{
		searchPattern = NULL;
		mediaT = mType;
		patternDescription = strdup(description);
	}
	pattern(searchType sMethod, const char* sPattern, const char* description, bool caseSens = false)
		: caseSensitive(caseSens), searchMethod(sMethod)
	{
		searchPattern = strdup(sPattern);
		patternDescription = strdup(description);
	}
	pattern(const pattern& pat) 
	{ 
		if(pat.searchPattern)
			searchPattern = strdup(pat.searchPattern);
		else
			searchPattern = NULL;
		if(pat.patternDescription)
			patternDescription = strdup(pat.patternDescription);
		else
			patternDescription = NULL;
		searchMethod = pat.searchMethod;
		mediaT = pat.mediaT;
		caseSensitive = pat.caseSensitive;
	}
	pattern& operator=(pattern const& pat)
	{
		if(pat.searchPattern)
			searchPattern = strdup(pat.searchPattern);
		else
			searchPattern = NULL;
		if(pat.patternDescription)
			patternDescription = strdup(pat.patternDescription);
		else
			patternDescription = NULL;
		searchMethod = pat.searchMethod;
		mediaT = pat.mediaT;
		caseSensitive = pat.caseSensitive;
		return *this;
	}
	~pattern()
	{
		if(searchPattern)
			free(searchPattern);
		if(patternDescription)
			free(patternDescription);
	}
protected:
	char *searchPattern;
	char *patternDescription;
	bool caseSensitive;
	searchType searchMethod;
	mediaType mediaT;
	friend class mediaplugin;
};

typedef vector<pattern> patterns;

class mediaplugin : public cListObject
{
public:
	mediaplugin(const char* pluginName);
	mediaplugin(const mediaplugin& plugin);
	mediaplugin& operator=(mediaplugin const& plugin);
	virtual ~mediaplugin();

	virtual bool matches() { return _found; }
    virtual bool matches2(std::vector<mediaplugin *> matchedPlugins) {return false;}
	virtual bool searchPattern();
	cOsdObject *execute() 
	{ 
		//printf("Executing plugin %s\n", _pluginName?_pluginName:"Unknown" );
		// dvdswitch plugin copies DVD to HDD, so no need to remember the DVD
		if (strcmp( _pluginName, "dvdswitch" )==0 )  // TODO checked ripit should also be excluded
		{ 
			//HERE;printf("Removing entry from database\n");
			// remove dvd from database
			 RemoveFromDataBase();
		} 
		else
        {
			Write2DataBase();
        } 
		preStartUmount(); 
		return startPlugin(); 
	}
	virtual void preStartUmount();	
	virtual cOsdObject* startPlugin();	
	void addPattern(const pattern searchPattern)
	{
		_patterns.push_back(searchPattern);
	}
	bool needsUmount() { return _needsUmount; }
	bool needsDirectory() { return _needsDirectory; }
	const char* getDescription() { 
    //printf("-------------const char* getDescription(), _pluginDescription = %d---------\n", (int) _pluginDescription);
    return _pluginDescription; }
	const char* getName() { return _pluginName; }
	void reset() { /*_found = false;*/ _pluginDescription = NULL; };
	static void setCurrentDevice(device* dev);
	static void setMountCmd(const char* mount);
	static void setStatus(cMediadStatus* status);
	void setSearchMode(searchMode sMode) { _sMode = sMode;}
	//void setAutoStart(bool autostart) { _doAutoStart = autostart;}
	void setAutoStart(int autostart) { _doAutoStart = autostart;}
	void setActive(bool activated) { _activated = activated;}
	int* isActiveSetup();
	bool isActive() { return _activated; }
	int* isAutoStartSetup();
	//bool isAutoStart() { return _doAutoStart; }
	int isAutoStart() { return _doAutoStart; }
	virtual bool checkPluginAvailable(bool ignoreActivation = false); 
	virtual mediaplugin* clone() = 0;

	// DataBase 
	void Write2DataBase(); // writes DVD info to database
	void RemoveFromDataBase(); // removes media (DVD) info from database

protected:
	int TestType(const char *dir, const char *file, bool caseSensitive = false);
	virtual bool checkPluginAvailable(const cPlugin* plugin);

	static char* _st_curDirectory;
	static char* _st_mountCmd;
	static char* _st_tmpCdrom;
	static device* _st_curDevice;
	static cMediadStatus* _st_status;
	static mediaType _st_curMediaType;
	static int _st_refCount;
	char* _pluginName;
	// never free it as it is only a refernce
	const char* _pluginDescription;
	bool _needsUmount;
	bool _needsDirectory;
	patterns _patterns;
	bool _found;
	int _doAutoStart;
	int _activated;
	searchMode _sMode;
private:
	void strToUpper(char *lower);
};
#endif
