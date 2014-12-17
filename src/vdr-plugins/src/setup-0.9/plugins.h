/*********************************************************
 * DESCRIPTION:
 *             Header File
 *
 * $Id: plugins.h,v 1.4 2005/06/26 10:08:59 ralf Exp $
 *
 * Contact:    ranga@teddycats.de
 *
 * Copyright (C) 2004 by Ralf Dotzert
 *********************************************************/

#ifndef PLUGINS_H
#define PLUGINS_H

#include <assert.h>
#include <vdr/tools.h>
#include <tinyxml/tinyxml.h>
#include <string>
#include <vector>
#include <deque>

/**
@author Ralf Dotzert
@author Markus Hahn
*/

extern const char yesStr;
extern const char noStr;

enum ePluginFlags
{
    pfNone = 0x0000,
    pfActive = 0x0001,
    pfProtect = 0x0002,
    pfInSystem = 0x0004,
    pfHidden = 0x0008
};

// ---- Class cSetupPlugin -----------------------------------------

class cSetupPlugin
{
  public:
    cSetupPlugin(std::string Name, std::string Info, std::string Parameter,
                 int Flags);
    cSetupPlugin(const cSetupPlugin& p);
    ~cSetupPlugin();

    const char *GetPluginName() const;
    const char *GetPluginInfo() const;
    const char *GetPluginParam() const;
    /// beware! PluginMenu  cast const away and write to parameter_!
    void SetFlag(int flag);
    void ClrFlag(int flag);
    int Flags() const;
    bool HasFlag(int Flags) const;
    const int *Active() const;
    const int *Protected() const;
  private:
    std::string name_;
    std::string info_;
    char parameter_[80];
    int flags_;
    int active_;
    int protected_;
};


// ---- Class cSetupPlugins -----------------------------------------

class cSetupPlugins             //: private std::vector<cSetupPlugin>
{
  public:
    static void Create();
    static void Create(std::string SysconfigKey);
    static void Destroy();
    static cSetupPlugins & GetInstance();

    void Init(TiXmlNode * node, const std::string LibPath,
              std::string SysConfigKey);
    ///> Set XML values 
    bool LoadPlugins();
    ///< Saves sysConfigParamStr_ set value update sysconfig
    void Save();
    TiXmlElement *SaveXML();
      std::vector < cSetupPlugin > v_plugins_;

      std::string SysConfValue();
    const cSetupPlugin *GetPluginByName(std::string Name);
    void Sort();
      cSetupPlugin & Get(int index);

    int Size();
    //void Clear();
    //const char *GetActivePlugins() const;
  private:
    static cSetupPlugins *instance_;
    ///< singelton

      cSetupPlugins();
      cSetupPlugins(std::string SysconfigKey);
     ~cSetupPlugins();

      cSetupPlugins(const cSetupPlugins &);
      cSetupPlugins & operator=(const cSetupPlugins &);
    ///< private '=' operator prevent copying

    bool ReadDir();
    bool ParseXML();

    bool IsValidPlugin(std::string name);
    void AddPlugin(std::string name, std::string parameter, std::string info,
                   int flags);

    ///< Is plugin running in vdr, already?
    bool IsPluginLoaded(const std::string PluginName);

    TiXmlNode *xmlNode_;
      std::vector < std::string > availablePlugins_;
      std::string libPath_;
      std::string sysConfigKey_;
      std::string sysConfigParamStr_;

    bool changed;               //static ??

};

// --- inline definitions  ------------------------------------------------------------

// --- Class cSetupPlugins ---------------------------------------------------------------

inline cSetupPlugins &
cSetupPlugins::GetInstance()
{
    assert(instance_);
    return *instance_;
}

inline void
cSetupPlugins::Create()
{
    //dsyslog (DBG " cSetupPlugins::Create() ");
    if (!instance_)
        instance_ = new cSetupPlugins;
}

inline void
cSetupPlugins::Destroy()
{
    //dsyslog (DBG " cSetupPlugins::Destroy() ");
    assert(instance_);
    delete instance_;
    instance_ = NULL;
}

inline int
cSetupPlugins::Size()
{
    return v_plugins_.size();
}

/*
void cSetupPlugins::Clear()
{
   v_plugins_.clear();
}
*/

inline cSetupPlugin &
cSetupPlugins::Get(int index)
{
    return v_plugins_.at(index);
}

// --- Class cSetupPlugin ---------------------------------------------------------------

inline cSetupPlugin::cSetupPlugin(std::string Name, std::string Info, std::string Parameter, int Flag):
name_(Name), info_(Info), flags_(Flag)
{
    active_ = !HasFlag(pfActive);
    protected_ = !HasFlag(pfProtect);
    snprintf(parameter_, 80, "%s", Parameter.c_str());
}

inline cSetupPlugin::~cSetupPlugin()
{
}

inline const char *
cSetupPlugin::GetPluginName() const
{
    return name_.c_str();
}

inline const char *cSetupPlugin::GetPluginInfo() const
{
    return info_.c_str();
}

inline const char *cSetupPlugin::GetPluginParam() const
{
    return &parameter_[0];
}


inline void cSetupPlugin::SetFlag(int flag)
{
    flags_ |= flag;
}

inline void
cSetupPlugin::ClrFlag(int flag)
{
    flags_ &= ~flag;
}

inline int
cSetupPlugin::Flags() const
{
    return flags_;
}

inline bool cSetupPlugin::HasFlag(int Flags)const
{
    return (flags_ & Flags) == Flags;
}

inline const int * cSetupPlugin::Active() const
{
    return &active_;
}

inline const int *cSetupPlugin::Protected() const
{
    return &protected_;
}

#endif
