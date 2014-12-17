#include <vector>
#include <vdr/osdbase.h>
#include <string>

class cCommands;

//additional commands, singletons

struct bgcommand
{
 std::string command;
 std::string description;
};
  
extern std::vector<bgcommand> bgcommandlist;

class myCommands
{
    public:
    std::string Title(){return title;}
    virtual eOSState Execute(){return osContinue;}
    virtual ~myCommands(){};

    protected:
    std::string title;
    myCommands(){};
};

class myMoveCommand:public myCommands
{
    public:
    /*override*/ eOSState Execute(){return osUser4;}
    static myMoveCommand *Instance(){return &instance;}

    protected:
    static myMoveCommand instance;
    myMoveCommand(){title = trNOOP("Move");};
};

class myDeleteCommand:public myCommands
{
    public:
    /*override*/ eOSState Execute(){return osUser5;}
    static myDeleteCommand *Instance(){return &instance;}

    protected:
    static myDeleteCommand instance;
    myDeleteCommand(){title = trNOOP("Delete");};
};

class myRenameCommand:public myCommands
{
    public:
    /*override*/ eOSState Execute(){return osUser6;}
    static myRenameCommand *Instance(){return &instance;}

    protected:
    static myRenameCommand instance;
    myRenameCommand(){title = trNOOP("Rename");};
};

class mySetTitleCommand:public myCommands
{
    public:
    /*override*/ eOSState Execute(){return osUser7;}
    static mySetTitleCommand *Instance(){return &instance;}

    protected:
    static mySetTitleCommand instance;
    mySetTitleCommand(){title = trNOOP("Change title");};
};

class myBurnCommand:public myCommands
{
    public:
    /*override*/ eOSState Execute(){return osUser8;}
    static myBurnCommand *Instance(){return &instance;}

    protected:
    static myBurnCommand instance;
    myBurnCommand(){title = trNOOP("Burn recording");};
};

class myMarkNewCommand:public myCommands
{
    public:
    /*override*/ eOSState Execute(){return osUser9;}
    static myMarkNewCommand *Instance(){return &instance;}

    protected:
    static myMarkNewCommand instance;
    myMarkNewCommand(){title = trNOOP("Mark as new");};
};

/*
class myDetailsCommand:public myCommands
{
    public:
    / *override* / eOSState Execute(){return osUser9;}
    static myDetailsCommand *Instance(){return &instance;}

    protected:
    static myDetailsCommand instance;
    myDetailsCommand(){title = trNOOP("Edit Details");};
};
*/

class myMenuCommands:public cOsdMenu
{
 private:
#if VDRVERSNUM >= 10713
  cList<cNestedItem> *commands;
  cString parameters;
  cString title;
  cString command;
  bool confirm;
  char *result;
  bool Parse(const char *s);
#else
  cCommands *commands;
  char *parameters;
#endif

  std::string path;
  cRecording *recording;
  
  std::string &commandMsg;
  
  std::vector<myCommands *> mycommands_;
  eOSState Execute(void);
 public:
#if VDRVERSNUM >= 10713
  myMenuCommands(const char *Title, cList<cNestedItem>  *Commands, cRecording *recording, std::vector<myCommands *> mycommands, 
		 std::string &commandMsg);
  myMenuCommands(const char *Title, cList<cNestedItem>  *Commands, std::string path, std::vector<myCommands *> mycommands, std::string &cmdMsg);
#else
  myMenuCommands(const char *Title, cCommands *Commands, cRecording *recording, 
                 std::vector<myCommands *> mycommands, std::string &commandMsg);
  myMenuCommands(const char *Title, cCommands *Commands, std::string path, 
                 std::vector<myCommands *> mycommands, std::string &cmdMsg);
#endif
  virtual ~myMenuCommands();
  void AddCommands();
  virtual eOSState ProcessKey(eKeys Key);
};
