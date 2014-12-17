#ifdef REELVDR
/*********************************************************
 * DESCRIPTION:
 *             Header File
 *
 * $Id: sysconfig.h,v 1.3 2005/10/03 14:05:20 ralf Exp $
 *
 * Contact:    ranga@teddycats.de
 *
 * Copyright (C) 2004 by Ralf Dotzert
 *********************************************************/

#include <string>
#include <map>
//#include "debug.h"
//#include "util.h"

#ifndef SYSCONFIG_VDR_H
#define SYSCONFIG_VDR_H

//#define mapConstIter_t std::map<std::string, char *>::const_iterator
//#define MAP_ITER std::map<std::string, char *>::iterator


typedef std::map < std::string, char *>::const_iterator mapConstIter_t;
typedef std::map < std::string, char *>::iterator mapIter_t;


class cSysConfig_vdr
{
  public:
    static void Create();
    static void Destroy();
    static cSysConfig_vdr & GetInstance();

    bool Reload();
    bool Load(const char *fname);
    bool Save();
    void SetVariable(const char *key, const char *value);
    
     // increments 'changed' if sysconfig was modified
    void SetVariable(const char *key, const char *value, int& changed);
    
    const char * GetVariable(const char *key) const;
    const char * FileName() const;
  private:
    cSysConfig_vdr();
    ~cSysConfig_vdr();
    cSysConfig_vdr(const cSysConfig_vdr &);
    cSysConfig_vdr & operator=(const cSysConfig_vdr &);

    static cSysConfig_vdr * instance_;
    std::map < std::string, char *> sysMap_;
    std::string fileName_;

    const char * ReadLine(int fp);
    void AddLine(const char *line);
    void AddVariable(const char *key, char *value);
};



#endif
#endif /*REELVDR*/

