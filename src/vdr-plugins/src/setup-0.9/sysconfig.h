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

#include <assert.h>
#include <string>
#include <map>
#include "util.h"

#ifndef SYSCONFIG_H
#define SYSCONFIG_H

//#define mapConstIter_t std::map<std::string, char *>::const_iterator
//#define MAP_ITER std::map<std::string, char *>::iterator

typedef std::map < std::string, char *>::const_iterator mapConstIter_t;
typedef std::map < std::string, char *>::iterator mapIter_t;


class cSysConfig
{
  public:
    static void Create();
    static void Destroy();
    static cSysConfig & GetInstance();

    bool Load(const char *fname);
    bool Save();
    void SetVariable(const char *key, const char *value);
    const char * GetVariable(const char *key) const;
    const char * FileName() const;
  private:
    cSysConfig();
    ~cSysConfig();
    cSysConfig(const cSysConfig &);
    cSysConfig & operator=(const cSysConfig &);

    static cSysConfig * instance_;
    std::map < std::string, char *> sysMap_;
    std::string fileName_;

    const char * ReadLine(int fd);
    void AddLine(const char *line);
    void AddVariable(const char *key, char *value);
};

// ---------  inline definitions --------------------------------

inline cSysConfig::cSysConfig()
{
}

inline cSysConfig &cSysConfig::GetInstance()
{
    //assert(instance_);
    if(!instance_)
        Create();
    return *instance_;
}

inline void cSysConfig::Create()
{
    //assert(!instance_);
    if (instance_)
        Destroy();
    instance_ = new cSysConfig;
}

inline void cSysConfig::Destroy()
{
    //assert(instance_);
    if(instance_)
        delete instance_;
    instance_ = NULL;
}

inline const char * cSysConfig::FileName() const
{
    return fileName_.c_str();
}

inline void cSysConfig::AddVariable(const char *key, char *value)
{
    if (key)
       sysMap_.insert(std::map < std::string, char *>::value_type(key, value));
}

inline void cSysConfig::SetVariable(const char *key, const char *value)
{
    //dsyslog(DBG_PREFIX " cSysConfig::SetVariable@%s: %s ",key ,value);
    if (!key)return;
    if (sysMap_[key])
       delete [] sysMap_[key];

    sysMap_[key] = Util::Strdupnew(value);
}

inline const char *cSysConfig::GetVariable(const char *key) const
{
    if (!key) return NULL;
    
    mapConstIter_t iter = sysMap_.find(key);

    if (iter != sysMap_.end())
        return iter->second;

  //  printf (" cSysConfig::GetVariable(%s) not found. \n", key);
    return NULL;
}

#endif
