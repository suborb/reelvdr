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

    bool Load(const char *fname);
    bool Save();
    void SetVariable(const char *key, const char *value);
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

// ---------  inline definitions --------------------------------

inline cSysConfig_vdr::cSysConfig_vdr()
{
}

inline cSysConfig_vdr &cSysConfig_vdr::GetInstance()
{
    if (!instance_) Create();
    return *instance_;
}

inline void
cSysConfig_vdr::Create()
{
    if (!instance_)
        instance_ = new cSysConfig_vdr;
}

inline void cSysConfig_vdr::Destroy()
{
    if (instance_)
    delete instance_;
    instance_ = NULL;
}

inline const char * cSysConfig_vdr::FileName() const
{
    return fileName_.c_str();
}

inline void cSysConfig_vdr::AddVariable(const char *key, char *value)
{
    if (key)
       sysMap_.insert(std::map < std::string, char *>::value_type(key, value));
}

inline void cSysConfig_vdr::SetVariable(const char *key, const char *value)
{
    //dsyslog(DBG_PREFIX " cSysConfig_vdr::SetVariable@%s: %s ",key ,value);
    if (sysMap_[key])
       delete [] sysMap_[key];

    //sysMap_[key] = Util::Strdupnew(value);
    sysMap_[key] = strdup(value);
}

inline const char *cSysConfig_vdr::GetVariable(const char *key) const
{
    if (!key) return NULL;
    
    mapConstIter_t iter = sysMap_.find(key);

    if (iter != sysMap_.end())
        return iter->second;

    //printf (" cSysConfig_vdr::GetVariable(%s) not found. \n", key);
    return NULL;
}

#endif
