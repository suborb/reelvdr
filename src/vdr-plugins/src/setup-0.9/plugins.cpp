/****************************************************************************
 * DESCRIPTION:
 *             Plugin Representation of XML Tree
 *
 * $Id: plugins.cpp,v 1.6 2005/10/03 14:05:20 ralf Exp $
 *
 * Contact:    ranga@teddycats.de
 *
 * Copyright (C) 2004 by Ralf Dotzert
 ****************************************************************************/

#include <algorithm>
#include <stdio.h>
#include <string.h>

#include <tinyxml/tinyxml.h>
#include <vdr/debug.h>
#include <vdr/plugin.h>

#include "plugins.h"
#include <vdr/sysconfig_vdr.h>
#include "util.h"

using std::string;
using std::vector;
using std::deque;

#define DBG ""

cSetupPlugin::cSetupPlugin(const cSetupPlugin & Plugin)
{
    name_ = Plugin.GetPluginName();
    info_ = Plugin.GetPluginInfo();
    //memmcpy
    strncpy(parameter_, Plugin.GetPluginParam(), 80);
    flags_ = Plugin.Flags();
    active_ = Plugin.HasFlag(pfActive);
    protected_ = Plugin.HasFlag(pfProtect);
}

/**
 * Construktor
 * @return empty cSetupPlugin Object
 */
cSetupPlugins::cSetupPlugins(string SysconfigKey):sysConfigKey_(SysconfigKey)
{
}

/**
 * Destructor
 * @return
 */
cSetupPlugins::cSetupPlugins()
{
}

cSetupPlugins::~cSetupPlugins()
{
}

void
cSetupPlugins::Init(TiXmlNode * XmlNode, string LibPath, string SysConfigKey)
{
    xmlNode_ = XmlNode;
    libPath_ = LibPath;
    sysConfigKey_ = SysConfigKey;
}

bool
cSetupPlugins::LoadPlugins()
{

    if (!ReadDir())
    {
        esyslog(ERR " check plugin library path \"%s\",", libPath_.c_str());
        return false;           // stops further plugin setup handling
    }
    if (!ParseXML())
    {
        esyslog(ERR
                " no cSetupPlugin configured! Overwrite \"cSetupPlugin\" section to XML file.");
        return false;
    }
    return true;
}

//bool cSetupPlugins::LoadPlugins(const string LibPath)
bool
cSetupPlugins::ReadDir()
{
    DIR *libDir;
    struct dirent *dirEntry = NULL;
    char prefix[] = "libvdr-";
    char suffix[] = ".so." VDRVERSION;

    if (!(libDir = opendir(libPath_.c_str())))
    {
        esyslog(ERR "can`t open Libdir %s", libPath_.c_str());
        return false;
    }

    while ((dirEntry = readdir(libDir)) != NULL)
    {
        char *tmp = NULL;
        if (strncmp(dirEntry->d_name, prefix, 7) == 0 &&
            (tmp = strstr(dirEntry->d_name, suffix)))
        {
            tmp[0] = '\0';
            string module(dirEntry->d_name, sizeof(prefix) - 1,
                          sizeof(dirEntry->d_name) - sizeof(prefix));
            availablePlugins_.push_back(module);
        }
    }
    closedir(libDir);

    if (availablePlugins_.empty())
    {
        esyslog(ERR "  No valid plugins found in \"%s\" ", libPath_.c_str());
        return false;
    }

    return true;
}

bool
cSetupPlugins::ParseXML()
{
    TiXmlElement *elem = NULL;
    TiXmlNode *tmpNode = xmlNode_;

    if (tmpNode && tmpNode->Type() == TiXmlNode::ELEMENT
        && (tmpNode = tmpNode->FirstChild()) != NULL)
    {
        //dsyslog(DBG "   firstChildName: %s ",tmpNode->Value());
        do
        {
            if (tmpNode->Type() == TiXmlNode::ELEMENT
                && strcmp(tmpNode->Value(), "plugin") == 0)
            {
                elem = tmpNode->ToElement();
                const char *name = elem->Attribute("name");
                const char *info = elem->Attribute("info");     // info Weg
                const char *param = elem->Attribute("param");

                const char *active = elem->Attribute("active");
                const char *protect = elem->Attribute("protected");

                /*
                   dsyslog(DBG "LOAD XML-Plugins; read:   Name %s Info %s Param %s Active %s Protected %s",
                   name, info, param , active, protect);
                 */

                if (!param)
                    param = "";
                if (!active)
                    active = "";
                if (!protect)
                    protect = "";

                int flags = pfNone;     //flags weg
                if (setup::MakeBool(active, true))
                    flags = pfActive;
                if (setup::MakeBool(protect, false))
                    flags |= pfProtect;

                //dsyslog(DBG " AddPlugin with name %s; info %s; parameter %s; flags %d;", name, info, param, flags);
                AddPlugin(name, info, param, flags);
            }
        }
        while ((tmpNode = tmpNode->NextSibling()) != NULL);
    }
    else
    {
        esyslog(ERR " Error in section plugins ");
        return false;
    }

    for (vector < string >::const_iterator iter = availablePlugins_.begin();
         iter != availablePlugins_.end(); ++iter)
    {

        //dsyslog(DBG " available Plugins %s ",iter->c_str());
        AddPlugin(*iter, "", "", pfNone);
    }

    /*dsyslog(DBG " Dump configurend Plugins ");
       for(vector<cSetupPlugin>::const_iterator iter = v_plugins_.begin(); iter != v_plugins_.end();++iter)
       {
       dsyslog(DBG " Plugin %d :  name %s info %s parameter %s flags %d", iter - v_plugins_.begin(),
       iter->GetPluginName(), iter->GetPluginInfo(), iter->GetPluginParam(), iter->Flags());
       }
     */
    return true;
}


/**
 * Save cSetupPlugin  XXX -> SaveSysConfig
 */
void
cSetupPlugins::Save()
{
    //dsyslog (DBG " Save ");

    sysConfigParamStr_ = "";
    for (vector < cSetupPlugin >::const_iterator iter = v_plugins_.begin();
         iter != v_plugins_.end(); ++iter)
    {
        cSetupPlugin p = *iter;
        /*
           //dsyslog (DBG "  Plugin %s  Active: %d;  Protect: %d ", p.GetPluginName(),*p.Active(), *p.Protected()) ;
           if (*p.Protected() == 1) dsyslog (DBG "Plugin  %s is  Protectet ",p.GetPluginName());
           if (*p.Active() == 1) dsyslog (DBG "Plugin %s is Active ",p.GetPluginName());
           //if (p.HasFlag(pfProtect) == 1) dsyslog (DBG "Plugin  %s is  Protectet ",p.GetPluginName());
           //if (p.HasFlag(pfActive) == 1) dsyslog (DBG "Plugin %s is Active ",p.GetPluginName());

           if (p.HasFlag(pfProtect) == 1 || p.HasFlag(pfActive) == 0)
           {
           char tmp[100];
           snprintf(tmp,  sizeof(tmp)-1, " \\\"-P%s \"%s\"", p.GetPluginName(), p.GetPluginParam());
           sysConfigParamStr_ += tmp;
           }
         */
    }

    cSysConfig_vdr & sysConfig = cSysConfig_vdr::GetInstance();
    sysConfig.SetVariable(sysConfigKey_.c_str(), sysConfigParamStr_.c_str());
}

TiXmlElement *
cSetupPlugins::SaveXML()
{
    isyslog(ERR " not yet implented!");
    return NULL;
}

bool
cSetupPlugins::IsValidPlugin(std::string name)
{

    vector < std::string >::const_iterator iter =
        find(availablePlugins_.begin(), availablePlugins_.end(), name);

    if (iter != availablePlugins_.end())
        return true;

    return false;
}

void
cSetupPlugins::AddPlugin(string name, string parameter, string info,
                         int flags)
{
    if (!IsValidPlugin(name))
    {
        esyslog(DBG "  Plugin \"%s\" is not in \"%s\" ", name.c_str(),
                libPath_.c_str());
    }
    else
    {
        cSetupPlugin p(name, parameter, info, flags);
        /*
           dsyslog(DBG " ->->->->-> Add Plugin \"%s\" \"%s\" \"%s\" Active: %s - Protected: %s <-<-<-<-<- ", name.c_str(), parameter.c_str(),
           info.c_str(), p.HasFlag(pfActive)?"YES":"NO", p.HasFlag(pfProtect)?"YES":"NO");
         */
        v_plugins_.push_back(p);
    }
}

/**
 * returns a pointer to the plugin with the specified name
 * or NULL if not found
 * @param name of the plugin
 * @return pointer to plugin otherwise NULL
 */
const cSetupPlugin *
cSetupPlugins::GetPluginByName(string name)
{

    for (vector < cSetupPlugin >::const_iterator iter = v_plugins_.begin();
         iter != v_plugins_.end(); ++iter)
    {
        if (strcmp(iter->GetPluginName(), name.c_str()) == 0)
            return &(*iter);
    }
    return NULL;
}
