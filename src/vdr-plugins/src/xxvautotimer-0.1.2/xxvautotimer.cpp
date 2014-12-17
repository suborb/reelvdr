/*
 * xxvautotimer.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: xxvautotimer.cpp,v 1.11 2006/08/15 20:35:22 ralf Exp $
 */

#include <getopt.h>
#include <vdr/plugin.h>
#include "setup.h"
#include "menu.h"
#include "i18n.h"
#include "inifile.h"

using namespace std;

static const char *VERSION        = "0.1.2";
static const char *DESCRIPTION    = "Autotimer for XXV";
static const char *MAINMENUENTRY  = "Edit XXV Autotimers";

char    plugin_name[MaxFileName]  = "XXVAutoTimer";
class cPluginXxvautotimer : public cPlugin {
private:
  // Add any member variables or functions you may need here.
  bool readXXVConfigFile(char *fname);
  string xxvParam(string src, string key, string defaultValue = t_Str(""));
public:
  cPluginXxvautotimer(void);
  virtual ~cPluginXxvautotimer();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual const char *MainMenuEntry(void) { return tr(MAINMENUENTRY); }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  };

cPluginXxvautotimer::cPluginXxvautotimer(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginXxvautotimer::~cPluginXxvautotimer()
{
  // Clean up after yourself!
}

const char *cPluginXxvautotimer::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return ("-f path     --xxvconfigfile    path to xxv configuration!)\n");
}

bool cPluginXxvautotimer::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  int c;
  struct option long_options[] = {
    { "xxvconfigfile",                required_argument,       NULL, 'f' },
    { NULL }
  };

  for (c = 1; c < argc; c++) {
    dsyslog("%s: Parameter%d=%s", plugin_name, c, argv[c]);
  }


  while ((c = getopt_long(argc, argv, "f:", long_options, NULL)) != -1) {
    switch(c)
    {
      case 'f': if( optarg)
                {
                    readXXVConfigFile(optarg);
                }
                break;
      default:  dsyslog("%s: unknown Parameter%d=%s", plugin_name, c, argv[c]);
                break;
    }
  }

  return true;
}

bool cPluginXxvautotimer::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  RegisterI18n(Phrases);
  return true;
}

bool cPluginXxvautotimer::Start(void)
{
  // Start any background activities the plugin shall perform.
  return true;
}

void cPluginXxvautotimer::Stop(void)
{
  // Stop any background activities the plugin shall perform.
}

void cPluginXxvautotimer::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

cOsdObject *cPluginXxvautotimer::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  return new cXxvAutotimerMenu;
}

cMenuSetupPage *cPluginXxvautotimer::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return new cXxvAutotimerSetupPage;
}

bool cPluginXxvautotimer::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  if( ! autotimerSetup.useXXVConfigFile ||
        strcmp(Name, autotimerSetup.strXXV020 )==0)
    return autotimerSetup.SetupParse(Name, Value);
  else
    return(true);

}


bool cPluginXxvautotimer::readXXVConfigFile(char *fname)
{
  bool result = false;
  CIniFile inifile;
  if(!inifile.Load(t_Str(fname)))
    dsyslog("%s: can not read config file:%s\n", plugin_name, fname);
  else
  {   
      t_Str  dbStr  = inifile.GetString(t_Str("DSN"),      t_Str("General"));
      int    telnetPort= inifile.GetInt(t_Str("Port"),     t_Str("TELNET"));
      t_Str  user   = inifile.GetString(t_Str("USR"),      t_Str("General"));
      t_Str  passwd = inifile.GetString(t_Str("PWD"),      t_Str("General"));
      int  lifetime = inifile.GetInt   (t_Str("Lifetime"), t_Str("TIMERS"));
      int  priority = inifile.GetInt   (t_Str("Priority"), t_Str("TIMERS"));
      int  prevMinutes = inifile.GetInt (t_Str("prevminutes"), t_Str("TIMERS"));
      int  afterMinutes = inifile.GetInt(t_Str("afterminutes"), t_Str("TIMERS"));
      string database = xxvParam(dbStr, t_Str("database")).c_str();
      string host     = xxvParam(dbStr, t_Str("host"),  t_Str("localhost") ).c_str();
      
      int dbPort      = atoi(xxvParam(dbStr, t_Str("port"),  t_Str("3304")).c_str());

      strncpy(autotimerSetup.database, database.c_str(), sizeof(autotimerSetup.database));
      strncpy(autotimerSetup.user,     user.c_str(),     sizeof(autotimerSetup.user));
      strncpy(autotimerSetup.passwd,   passwd.c_str(),   sizeof(autotimerSetup.passwd));
      strncpy(autotimerSetup.host,     host.c_str(),     sizeof(autotimerSetup.host));
      autotimerSetup.dbPort          = dbPort;
      autotimerSetup.telnetPort      = telnetPort;
      autotimerSetup.defaultLifetime = lifetime;
      autotimerSetup.defaultPriority = priority;
      autotimerSetup.defaultPrevMinutes  = prevMinutes;
      autotimerSetup.defaultAfterMinutes = afterMinutes;
      autotimerSetup.useXXVConfigFile = true;

      result=true;
  }
  return(result);
}


string cPluginXxvautotimer::xxvParam(string src, string key, string defaultValue )
{
  static   string result;
  unsigned int p1 = src.find(key+"=");
  unsigned int p2 = 0;

  if( p1 !=string::npos)
  {
    p1 = p1+key.length()+1;
    p2 = src.substr(p1,src.length()).find(';');
  }
  if(p2 == string::npos)
    p2= src.length();

  if(p1 != string::npos)
     result=src.substr(p1,p2);
  else
    result=defaultValue;
  return(result);
}


//holds setup configuration
cXxvAutotimerSetup  autotimerSetup;

VDRPLUGINCREATOR(cPluginXxvautotimer); // Don't touch this!
