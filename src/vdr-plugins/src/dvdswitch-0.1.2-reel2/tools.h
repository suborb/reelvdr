#ifndef __TOOLS_DVDSWITCH_H
#define __TOOLS_DVDSWITCH_H

#include "helpers.h"

class cDirList : public cFileList
{
  public:
    cDirList(void);
};

class cFileDelThread : public cThread
{
  private:
    char *File;
    bool Ok;

    bool RightCheck(char *value)
    {
      bool ret = false;
      if(value)
      {
        cFileInfo *info = new cFileInfo(value);
        ret = info->isWriteable();
        DELETENULL(info);
      }
      return ret;
    }
  protected:
    virtual void Action(void)
    {
      if(File)
        cFileCMD::Del(File);
      delete(this);
    };
  public:
    cFileDelThread(char *file)
    {
      File = NULL;
      Ok = false;
      
      if(!RightCheck(file))
        OSD_ERRMSG(tr("no Rights to delete"));
      else
      {
        Ok = true;
        if(file)
        {
          asprintf(&File, "%s.sdel", file);
          cFileCMD::Rn(file, File);
        }
      }
    }
    ~cFileDelThread(void) { free(File); }
    bool OK(void) { return Ok; };
};

class cFileMoveThread : public cThread
{
  private:
    char *FileName;
    char *File;
    char *Dest;
    bool Ok;

    bool RightCheck(char *value)
    {
      bool ret = false;
      if(value)
      {
        cFileInfo *info = new cFileInfo(value);
        ret = info->isWriteable();
        DELETENULL(info);
      }
      return ret;
    }
  protected:
    virtual void Action(void)
    {
      if(FileName && File && Dest)
      {
        char *buffer = NULL;
        asprintf(&buffer, "%s/%s", Dest, FileName);
        cFileCMD::Rn(File, buffer);
        free(buffer);
      }
      delete(this);
    };
  public:
    cFileMoveThread(char *file, char *dest)
    {
      FileName = NULL;
      File = NULL;
      Dest = NULL;
      Ok = false;

      if(!RightCheck(file) || !RightCheck(dest))
        OSD_ERRMSG(tr("no Rights to move"));
      else
      {
        Ok = true;
        if(file)
        {
          cFileInfo *info = new cFileInfo(file);
          FileName = strdup(info->FileName());
          DELETENULL(info);
          asprintf(&File, "%s.smove", file);
          cFileCMD::Rn(file, File);
        }
        Dest = dest ? strdup(dest) : NULL;
      }
    }
    ~cFileMoveThread(void)
    {
      free(FileName);
      free(File);
      free(Dest);
    }
    bool OK(void) { return Ok; };
};

#endif // __TOOLS_DVDSWITCH_H
