#ifndef BGPROCESS_H
#define BGPROCESS_H

class cPluginBgprocess;

class cBgProcessThread: public cThread
{
  friend class cPluginBgprocess;
  friend class BgProcessMenu;
  cPluginBgprocess *plugin_;

  public:
  cBgProcessThread(){};
  void Init(cPluginBgprocess *plugin);
  /*override*/ ~cBgProcessThread(){};
  /*override*/ void Action();
  bool PidExists(int pid);
};

class BgProcessMenu : public cOsdMenu
{
    enum { showCompleted, showRunning };
    int showMode;
    cBgProcessThread *bgProcessThread_;
  public:
    BgProcessMenu(cBgProcessThread *bgProcessThread);
    void ShowBgProcesses();
    void ShowCompleted();
    eOSState ProcessKey(eKeys k);
};

class cPluginBgprocess : public cPlugin
{
  private:
    bool GetProcessProgress(BgProcessData_t *pData);
    bool AddProcess(BgProcessData_t pData);
    bool RemoveProcess(BgProcessData_t pData);
    bool HasProcessFromCaller(BgProcessCallerInfo_t *pData);
    string GenKey(BgProcessData_t pData);
    cBgProcessThread bgProcessThread;
  public:
    cPluginBgprocess(void);
    virtual ~ cPluginBgprocess();
    virtual const char *Version(void);
    virtual const char *Description(void);
    virtual const char *CommandLineHelp(void);
    virtual bool ProcessArgs(int argc, char *argv[]);
    virtual bool Initialize(void);
    virtual bool Start(void);
    virtual void Stop(void);
    virtual void Housekeeping(void);
    virtual void MainThreadHook(void);
    virtual cString Active(void);
    virtual const char *MainMenuEntry(void);
    virtual cOsdObject *MainMenuAction(void);
    virtual cMenuSetupPage *SetupMenu(void);
    virtual bool SetupParse(const char *Name, const char *Value);
    virtual bool HasSetupOptions(void);
    virtual bool Service(const char *Id, void *Data = NULL);
    virtual const char **SVDRPHelpPages(void);
    virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
};

#endif

