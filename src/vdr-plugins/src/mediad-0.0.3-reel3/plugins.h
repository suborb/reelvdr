#ifndef __PLUGINS_H
#define __PLUGINS_H

#include "mediaplugin.h"

#define VDRSUBDIRARCH "2002-01-01.01.01.99."
#define VDRSUBDIR "2002-01-01.01.01.99.99.rec"

class vdrrecording : public mediaplugin
{
public:
    vdrrecording();
    virtual ~vdrrecording() {};
    virtual cOsdObject * startPlugin();
    virtual bool checkPluginAvailable(bool ignoreActivation = false); 
    virtual mediaplugin* clone();
    void TestDvdLink();
protected:
    virtual bool checkPluginAvailable(const cPlugin* plugin);
};

class dmharchivedvd : public mediaplugin
{
public:
    dmharchivedvd();
    virtual ~dmharchivedvd() {};
    
    virtual cOsdObject * startPlugin();
    virtual bool checkPluginAvailable(bool ignoreActivation = false); 
    virtual mediaplugin* clone();
protected:
    virtual bool checkPluginAvailable(const cPlugin* plugin);
};

class dvd : public mediaplugin
{
public:
    dvd();
    virtual ~dvd() {};
#if HAVE_XINEMEDIAPLAYER        
    virtual cOsdObject * startPlugin();
#endif
    virtual mediaplugin* clone();
};

class vcd : public mediaplugin
{
public:
    vcd();
    virtual ~vcd() {};
    virtual cOsdObject * startPlugin();
    virtual mediaplugin* clone();
};

class mplayer : public mediaplugin
{
public:
    mplayer();
    virtual ~mplayer() {};
    virtual cOsdObject * startPlugin();
    virtual mediaplugin* clone();
};

class mp3 : public mediaplugin
{
public:
    mp3();
    virtual ~mp3();
    virtual cOsdObject* startPlugin();
    virtual mediaplugin* clone();
};

class ripit : public mediaplugin
{
public:
    ripit();
    virtual ~ripit() {};
    virtual cOsdObject * startPlugin();
    virtual mediaplugin* clone();
    /*override*/ bool matches(); 
};

class photocd : public mediaplugin
{
public:
    photocd();
    virtual ~photocd() {};
    virtual mediaplugin* clone();
};

class cdda : public mediaplugin
{
public:
  cdda();
  virtual ~cdda() {};
    virtual mediaplugin* clone();
#if HAVE_XINEMEDIAPLAYER        
    virtual cOsdObject * startPlugin();
#endif
};

class burn : public mediaplugin
{
public:
    burn();
    virtual ~burn() {};
    virtual mediaplugin* clone();
};

class dvdswitch : public mediaplugin
{
public:
    dvdswitch();
    virtual ~dvdswitch() {};
    virtual cOsdObject * startPlugin();
    virtual bool matches();
    virtual mediaplugin* clone();
};

class filebrowser : public mediaplugin
{
public:
    filebrowser();
    /*override*/ ~filebrowser();
    /*override*/ mediaplugin* clone();
    /*override*/ bool matches(); 
    /*override*/ bool matches2(std::vector<mediaplugin *> matchedplugins);
    /*override*/ cOsdObject* startPlugin();
private:
    bool PluginMatches(std::string name, std::vector<mediaplugin *> matchedPlugins);
    bool GetDirEntries(const std::string &dir, std::vector<std::string>  &entries);
    std::string path_;
};


#endif
