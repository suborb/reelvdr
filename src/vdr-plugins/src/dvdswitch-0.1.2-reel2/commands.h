#ifndef __COMMANDS_DVDSWITCH_H
#define __COMMANDS_DVDSWITCH_H

#include <vdr/plugin.h>
#include "menu.h"
#include "imagelist.h"
#include "setup.h"
#include <stdlib.h>
#include <string>

enum eCMDs
{
  cmdNone,
  cmdDirManage,
  cmdDVDopen,
  cmdDVDclose,
  cmdImgPlay,
  cmdImgRename,
  cmdImgMove,
  cmdImgDelete,
  cmdImgBurn,
  cmdImgRead,
  cmdCommands,
  cmdDeleteCache,
  cmdDVDRWFormat
};

enum eCMDState
{
  csNone,
  csDirNew,
  csDirEdit,
  csDirMove,
  csDirDel
};

class cCMD
{
  public:
    static eOSState Play(cMainMenuItem *item = NULL);
    static eOSState Eject(bool close = false);
};


class cCMDMenu : public cOsdMenu
{
  private:
    int ImgType;
    char Dir[MaxFileName];
    char File[MaxFileName];
    cMainMenuItem *iItem;
    cMainMenu *OsdObject;
  public:
    cCMDMenu(cMainMenuItem *item, cMainMenu *osdobject);
    void DvdRead();
    virtual eOSState ProcessKey(eKeys Key);
};

class cCMDDir : public cOsdMenu, private cDirHandlingOpt
{
  private:
    eCMDState State;
    char Dir[MaxFileName];
    cMainMenu *OsdObject;
    bool Select;
    char *Buffer;

    void Build(char *dir = NULL);
    void SetDir(char *value = NULL)
    {
      MYDEBUG (" cCMDDir %s ", value); 
      if(value)
        strn0cpy(Dir, value, MaxFileName);
      else
        strcpy(Dir, "\0");
    }

    eOSState New(void);
    eOSState New(eKeys Key);
    eOSState Edit(cMainMenuItem *mItem);
    eOSState Edit(eKeys Key);
  public:
    cCMDDir(cMainMenu *osdobject = NULL, bool select = false, char *buffer = NULL);

    virtual eOSState ProcessKey(eKeys Key);
    void SetHelp(void);
};

class cCMDMove : public cOsdMenu, private cDirHandlingOpt
{
  private:
    char *File;
    cMainMenu *OsdObject;
    bool Dir;
    bool Direct;

    void Build(char *dir = NULL);
  public:
    cCMDMove(char *file, cMainMenu *osdobject, bool dir = true, bool direct = false);
    ~cCMDMove(void) { free(File); }

    virtual eOSState ProcessKey(eKeys Key);
    void SetHelp(void);
};

class cCMDImage
{
  private:
    char *File;
    cMainMenu *OsdObject;
  public:
    char NewFile[MaxFileName];
    int tmpHideTypeCol;
    //int tmpHideImgSize

    cCMDImage(cMainMenu *osdobject = NULL);
    ~cCMDImage(void);

    char* Rename(char *file = NULL);
    eOSState Delete(char *file = NULL);
    eOSState Burn(char *file);
};

class cCMDImageRead : public cOsdMenu
{
  private:
    char File[MaxFileName];
    char Dir[MaxFileName];
    char ImgTypeTxt[MaxFileName];
    int ImgType;
  public:
    cCMDImageRead(void);
    ~cCMDImageRead(void);

    //void SetHelp(void);
    //virtual eOSState ProcessKey(eKeys Key);
};

/* Thread to read the output of vobcopy and growisofs 
 * and sends the percent completed to bgprocess
 * */
enum Mode { ReadMode, BurnMode};

class cStatusThread : public cThread
{
 private:
	 int mode; // ReadMode / BurnMode
	 std::string logFileName; // file to monitor
     std::string thread_desc; // what the thread is doing
     time_t startTime;
     std::string PluginName; 
 public:
	 cStatusThread(int m, std::string filename, std::string desc);
	 void StopStatusThread(int waitTime);
 protected:
	 virtual void Action();
     int StatusBurn();
	 int StatusRead(); // reads the logFile till the end and returns the status
};

class cCMDImageReadThread : public cThread
{
  private:
    char *File;
    char *Dir;
    eFileInfo FileType;
  protected:
    virtual void Action(void);
  public:
    cCMDImageReadThread(char *file, char *dir, int imgtype);
    ~cCMDImageReadThread(void);
};

class cCMDImageBurnThread : public cThread
{
  private:
    char *File;
    eFileInfo FileType;
  protected:
    virtual void Action(void);
  public:
    cCMDImageBurnThread(char *file, eFileInfo type);
    ~cCMDImageBurnThread(void);
};

#endif // __COMMANDS_DVDSWITCH_H
