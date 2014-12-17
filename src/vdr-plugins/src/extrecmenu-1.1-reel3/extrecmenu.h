#include <vdr/plugin.h>
#include <vdr/tools.h>

static const char *VERSION        = "1.1-reel1";
static const char *DESCRIPTION    = trNOOP("Extended recordings menu");
static const char *MAINMENUENTRY  = trNOOP("Recordings");

// --- cPluginExtrecmenu ------------------------------------------------------
class cPluginExtrecmenu:public cPlugin
{
 private:
 public:
  cPluginExtrecmenu(void);
  virtual ~cPluginExtrecmenu();
  virtual const char *Version(void){return VERSION;}
  virtual const char *Description(void){return tr(DESCRIPTION);}
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc,char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual cString Active(void);
  virtual const char *MainMenuEntry(void){return tr(MAINMENUENTRY);}
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name,const char *Value);
  virtual bool Service(const char *Id,void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command,const char *Option,int &ReplyCode);
  static bool replayArchiveDVD;
};
