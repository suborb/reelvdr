#ifndef vlcclient_h
#define vlcclient_h

static const char *VERSION        = "0.0.2";
static const char *DESCRIPTION    = "Client for a remote VLC";
static const char *MAINMENUENTRY  = "VideoLAN-Client";


class cPluginVlcClient : public cPlugin {
private:
  cRecordingsListThread *listThread;
public:
  cPluginVlcClient(void);
  virtual ~cPluginVlcClient();
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
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  };

#endif
