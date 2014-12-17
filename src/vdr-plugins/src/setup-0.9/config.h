/*********************************************************
 * DESCRIPTION:
 *             Header File
 *
 *Id: config.h,v 1.7 2005/10/03 14:05:20 ralf Exp $
 *
 * Contact:    ranga@teddycats.de
 *
 * Copyright (C) 2004 by Ralf Dotzert
 *********************************************************/

#ifndef SETUP_CONFIG_H
#define SETUP_CONFIG_H

#include <assert.h>
#include <string>
#include <vdr/submenu.h>

#include <tinyxml/tinystr.h>
#include <tinyxml/tinyxml.h>
#include "plugins.h"
#include <vdr/sysconfig_vdr.h>
#include "menus.h"

/**
@author Ralf Dozert and Markus Hahn
*/

// ----  Class cSetupConfig -----------------------------------------

class cSetupConfig
{
  public:
    static void Create();
    static void Destroy();
    static cSetupConfig & GetInstance();
    ///< singelton facility

    bool Load();
    Menus *GetMenus();
    Menus *GetMainMenus();
    const char *GetBootLinux() const;
      std::string GetCredits() const;
    bool SaveFile();            ///< Saves XML File
    bool SaveFile(const std::string);
    bool SaveSysconfig();
    //bool HasError();
    ///< returns false if error occured while parsing XML file
  private:
      cSetupConfig();
     ~cSetupConfig();
      cSetupConfig(const cSetupConfig &);
      cSetupConfig & operator=(const cSetupConfig &);


    static cSetupConfig *instance_;
    static int callUp;
    ///< increase at every call of Create() and decreas at Destroy
    ///< if callUp == 1 Destroy will realy destroy this object

    TiXmlDocument xmlDoc_;
      std::string xmlFileName_;
      std::string reBootLinux_;
      std::string credits_;
      std::string vdrLibDir_;
    Menus xmlMenus;             // weg
    Menus xmlMainMenus;         //weg


    void dumpXMLError();
    void dumpXMLError(const char *errStr);
};

// --- inline definitions --------------------

inline cSetupConfig::~cSetupConfig()
{
    cSetupPlugins::Destroy();
    cSysConfig_vdr::Destroy();
}

inline
    cSetupConfig &
cSetupConfig::GetInstance()
{
    assert(instance_);
    return *instance_;
}

inline void
cSetupConfig::Create()
{
    //dsyslog  (DBG_PREFIX " cSetupConfig::Create() callups %d", cSetupConfig::callUp);
    cSetupConfig::callUp++;

    if (!instance_)
        instance_ = new cSetupConfig;
}

inline void
cSetupConfig::Destroy()
{
    cSetupConfig::callUp--;
    if (cSetupConfig::callUp == 0)
    {
        delete instance_;
        instance_ = NULL;
    }
}

inline Menus *
cSetupConfig::GetMenus()
{
    return &xmlMenus;
}

inline Menus *
cSetupConfig::GetMainMenus()
{
    return &xmlMainMenus;
}

inline const char *
cSetupConfig::GetBootLinux() const
{
    return reBootLinux_.c_str();
}

inline bool cSetupConfig::SaveFile()
{
    return SaveFile(xmlFileName_);
}

inline std::string cSetupConfig::GetCredits() const
{
    return
    credits_;
}

#endif                          // SETUP_CONFIG_H
