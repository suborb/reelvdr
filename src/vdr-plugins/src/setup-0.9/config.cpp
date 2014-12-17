/****************************************************************************
 * DESCRIPTION:
 *             Read / Write cSetupConfig Data
 *
 * $Id: config.cpp,v 1.9 2005/10/03 14:05:20 ralf Exp $
 *
 * Contact:    ranga@teddycats.de
 *
 * Copyright (C) 2004 by Ralf Dotzert
 ****************************************************************************/

#include <iostream>
#include <string>

#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>

#include <vdr/debug.h>
#include <vdr/plugin.h>
#include <tinyxml/tinystr.h>

#include "config.h"
#include <vdr/sysconfig_vdr.h>
#include "plugins.h"
#include "util.h"


using std::cerr;
using std::cout;
using std::endl;
using std::string;

#define DBG ""

// Initialize static private data

cSetupPlugins *cSetupPlugins::instance_ = NULL;
int cSetupConfig::callUp = 0;


// ----- Class cSetupConfig ----------------------------------------------

cSetupConfig::cSetupConfig():xmlFileName_(cPlugin::ConfigDirectory())
{

    cSetupPlugins::Create();
    cSysConfig_vdr::Create();

    xmlFileName_ += "/setup/vdr-setup.xml";
    xmlDoc_ = TiXmlDocument(xmlFileName_.c_str());

}


/************************
* Read XML-File
*************************/
/**
 * Parse XML File
 * @return
 */
bool
cSetupConfig::Load()
{
    TiXmlElement *elem;
    const char *sysconfigFile = NULL;
    cSetupPlugins & setupPlugins = cSetupPlugins::GetInstance();

    if (xmlDoc_.LoadFile())
    {
        //printf(DBG " cSetupConfig::LoadFile LoadXml():   \n");
        elem = xmlDoc_.FirstChildElement("setup");

        sysconfigFile = elem->Attribute("sysconfigFile");

        //cout << "sysconfigFile " << sysconfigFile << endl;
        reBootLinux_ = elem->Attribute("bootLinux") ? elem->Attribute("bootLinux") : "";
        vdrLibDir_ = elem->Attribute("VDRlibDir") ? elem->Attribute("VDRlibDir") : "";
        credits_ = elem->Attribute("credits") ? elem->Attribute("credits") : "";
        //printf(DBG " cSetupConfig::LoadFile end of sysconfig attr:   \n");

        // ValidateSysconf();

        if (!cSysConfig_vdr::GetInstance().Load(sysconfigFile))
        {
            //cout << "Load sysconfig Failed : " << sysconfigFile << endl;
            return false;
        }

        elem = elem->FirstChildElement();

        for (; elem; elem = elem->NextSiblingElement())
        {
            //cout << "elem: " << elem->Value() << endl;

            if (strcmp(elem->Value(), "plugins") == 0)
            {
                ///< Get sysconfig  Plugin  variable
                //printf(DBG " cSetupConfig::LoadFile  load sysconfig: \n");
                string sysConfKey = elem->ToElement()->Attribute("sysconfig");

                //cout << "sysConfKey: " << sysConfKey << endl;

                setupPlugins.Init(elem, vdrLibDir_, sysConfKey);

            }
            //dsyslog (DBG " cSetupConfig::LoadFile LoadXml()  " );

            //printf(DBG " cSetupConfig::LoadFile  Load Main Menus\n");
            if (strcmp(elem->Value(), "mainmenus") == 0)
                xmlMainMenus.LoadXml(elem);

            //printf(DBG " cSetupConfig::LoadFile Load Menus\n");
            if (strcmp(elem->Value(), "menus") == 0)
                xmlMenus.LoadXml(elem);

        }
    }
    else
    {
        dumpXMLError("Error while Loading XML-FILE");
    }

    DDD("cSetupConfig::LoadFile success");
    //printf(DBG " cSetupConfig::LoadFile success \n");
    return true;
}

/**
 * Save the the cSetupConfigFile,
 * @param fname name of config file to write to
 * @return true if no error occured
 */
bool
cSetupConfig::SaveFile(string fname)
{

    if (xmlFileName_ != fname)
        xmlFileName_ = fname;

    cSysConfig_vdr & sysConfig = cSysConfig_vdr::GetInstance();
    cSetupPlugins & plugins = cSetupPlugins::GetInstance();

    TiXmlComment comment;
    comment.SetValue(" Mini-VDR cSetupConfiguration File\n"
                     " (c) Ralf Dotzert\n"
                     "\n\n" " for Example see vdr-menu.xml.example\n\n");

    TiXmlDocument xml = TiXmlDocument(xmlFileName_.c_str());
    TiXmlElement root("setup");
    //TODO sysconfig 
    root.SetAttribute("sysconfigFile", sysConfig.FileName());
    root.SetAttribute("bootLinux", reBootLinux_.c_str());
    root.SetAttribute("VDRlibDir", vdrLibDir_.c_str());


    TiXmlElement *xmlRootPlugins = plugins.SaveXML();

    //Change this!
    if (xmlRootPlugins != NULL &&
        root.LinkEndChild(xmlRootPlugins) != NULL &&
        xmlMainMenus.SaveXml(&root) == true &&
        xmlMenus.SaveXml(&root) == true &&
        xml.InsertEndChild(comment) != NULL &&
        xml.InsertEndChild(root) != NULL && xml.SaveFile())
    {
        return sysConfig.Save();
    }
    else
        dumpXMLError("Error writing file");

    return false;
}


// Write only sysConfig
bool
cSetupConfig::SaveSysconfig()
{
    bool ok = true;

    cSysConfig_vdr & sysConfig = cSysConfig_vdr::GetInstance();
    cSetupPlugins & setupPlugins = cSetupPlugins::GetInstance();

    ///< restore Plugins Arg list
    setupPlugins.Save();

    string oldFile = setup::FileNameFactory(sysConfig.GetVariable("CHANNELLIST"));

    sysConfig.Save();

    if (oldFile != sysConfig.GetVariable("CHANNELLIST"))
    {
        string channelDir = setup::FileNameFactory("channels");
        string channelLink = setup::FileNameFactory("link");

        //cout << "ping " << endl;
        if (unlink(channelLink.c_str()) == 0)
        {
            //cout << " ++++++++++++ deleted successfull:  " << channelLink.c_str() << endl;

            string newFile = setup::FileNameFactory(sysConfig.GetVariable("CHANNELLIST"));
            //cout << " +++++++++++ New File " << newFile << endl;

            if (!symlink(newFile.c_str(), channelLink.c_str()) < 0)
            {
                ok = false;
                DLOG("%s Can not link File %s  to %s errno=%d\n", DBG, newFile.c_str(), channelLink.c_str(), errno);
            }
        }
        else                    // !unlink
        {
            ok = false;
            DLOG("%s Can not unlink File %s  errno=%d\n", DBG, channelLink.c_str(), errno);
        }                       //if channellist
    }                           // if Chanellist Changed

    //cout << "pong " << endl;
    return ok;
}



/***************************
 * Dump XML-Error
 ***************************/
void
cSetupConfig::dumpXMLError()
{
    const char *errStr;
    int col, row;
    if (xmlDoc_.Error())
    {
        errStr = xmlDoc_.ErrorDesc();
        col = xmlDoc_.ErrorCol();
        row = xmlDoc_.ErrorRow();
        DLOG("%s: Error in %s Col=%d Row=%d :%s\n", DBG, xmlFileName_.c_str(), col, row, errStr);
    }
}

void
cSetupConfig::dumpXMLError(const char *myErrStr)
{
    const char *errStr;
    int col, row;

    if (xmlDoc_.Error())
    {
        errStr = xmlDoc_.ErrorDesc();
        col = xmlDoc_.ErrorCol();
        row = xmlDoc_.ErrorRow();
        DLOG("%s: %s: %s: Error in %s Col=%d Row=%d\n", DBG, myErrStr, errStr, xmlFileName_.c_str(), col, row);
    }
}
