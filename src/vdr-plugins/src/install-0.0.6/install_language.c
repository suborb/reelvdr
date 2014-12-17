/***************************************************************************
 *   Copyright (C) 2009 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                     Based on an older Version by: Tobias Bratfisch      *
 *                 2009 parts rewritten by Tobias Bratfisch                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 ***************************************************************************
 *
 * install_language.c
 *
 ***************************************************************************/

#include "install_language.h"
#include "installmenu.h"
#include <vdr/device.h>
#include <vdr/sysconfig_vdr.h>
#include "../avahi/avahiservice.h"

cInstallPrimLang::cInstallPrimLang() :cInstallSubMenu(tr("Handling"))
{
    for (int k = 0; k < 256; k++)
        city_values[k] = NULL;

    // Language
    osdLanguageIndex = I18nCurrentLanguage();

    askTimeZone = true;
#ifdef RBMINI
    cPlugin *p = cPluginManager::GetPlugin("avahi");
    if (p)
    {
        std::vector < ReelBox_t > ReelBoxes;
        p->Service("Avahi ReelBox-List", &ReelBoxes);

        if (ReelBoxes.size() > 0)
        {
            // save changes in sysconfig variable in vdr into sysconfig file
            cSysConfig_vdr::GetInstance().Save();

            std::string cmd;
            // get the timezone from an AVG
            if (strlen(Setup.NetServerIP))
            {                   // there was already one configured, let's take this one...
                // TB: but only if it's available
                bool serverAvailable = false;
                for (unsigned int i = 0; i < ReelBoxes.size(); i++)
                    if (strcmp(Setup.NetServerIP, ReelBoxes.at(i).Ip.c_str()) == 0)
                        serverAvailable = true;
                if (serverAvailable)
                    cmd = std::string("getConfigsFromAVGServer.sh ") + Setup.NetServerIP + std::string(" sysconfig ; ");
                else
                    cmd = std::string("getConfigsFromAVGServer.sh ") + ReelBoxes.at(0).Ip + std::string(" sysconfig ; ");
            }
            else
                cmd = std::string("getConfigsFromAVGServer.sh ") + ReelBoxes.at(0).Ip + std::string(" sysconfig ; ");
            SystemExec(cmd.c_str());    // changes /etc/default/sysconfig

            cSysConfig_vdr::GetInstance().Load(SYSCONFIGFNAME); // load the changes in file into vdr
            dsyslog("(%s:%d) ... done loading sysconfig.", __FILE__, __LINE__);
        }

        // one or more reelbox avantgarde donot ask for TimeZone
        // it is got from the first avantgarde in the list
        askTimeZone = (ReelBoxes.size() == 0);
    }
#endif
    if (askTimeZone)
    {
        // Timezone
        currentCity = 0;
        nr_city_values = 1;
        DIR *zoneDir = NULL;
        struct dirent *entry = NULL;
        struct stat buf;
        std::string path = "/usr/share/zoneinfo";
        for (int k = 0; k < 256; k++)
        {
            city_values[k] = (char *)malloc(32);
            city_values[k][0] = '\0';
        }

        if ((zoneDir = opendir(path.c_str())) != NULL)
        {
            while ((entry = readdir(zoneDir)) != NULL)
            {
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
                {
                    std::string tmp = path + "/" + entry->d_name;
                    stat(tmp.c_str(), &buf);
                }
            }
            closedir(zoneDir);
        }
        else
            d(printf("Can not read directory \"%s\" because of error \"%s\"\n", path.c_str(), strerror(errno)))

        cSysConfig_vdr & sysconfig = cSysConfig_vdr::GetInstance();
        char *zoneInfo = strdup((char *)sysconfig.GetVariable("ZONEINFO"));

        char *buf2;
        char *continent = NULL;
        char *city = NULL;

        continent = strdup(strtok_r(zoneInfo, "/", &buf2)); /* go to first '/' */
        city = strdup(strtok_r(NULL, "/", &buf2));  /* get the rest */

        free(zoneInfo);

        if (city != NULL)
        {
            nr_city_values = 0;
            DIR *conDir = NULL;
            struct dirent *entry = NULL;
            struct stat buf;
            std::string path = "/usr/share/zoneinfo/Europe";

            if ((conDir = opendir(path.c_str())) != NULL)
            {
                while ((entry = readdir(conDir)) != NULL)
                {
                    std::string tmp = path + "/" + entry->d_name;
                    stat(tmp.c_str(), &buf);
                    if (S_ISREG(buf.st_mode))
                    {
                        city_values[nr_city_values] = strcpy(city_values[nr_city_values], entry->d_name);
                        nr_city_values++;
                    }
                }
                closedir(conDir);
            }
            else
                d(printf("Can not read directory \"%s\" because of error \"%s\"\n", path.c_str(), strerror(errno)))

            for (int i = 0; i < nr_city_values; i++)
                if (!strcmp(city_values[i], city))
                    currentCity = i;
        }
        free(city);
        free(continent);
    }                           // ask timezone

    Set();
}

cInstallPrimLang::~cInstallPrimLang()
{
    // Delete old images from OSD
    cPluginManager::CallAllServices("setThumb", NULL);
    for (int k = 0; k < 256; k++)
        free(city_values[k]);
}

void cInstallPrimLang::Set()
{
    int current = Current();
    Clear();

    // Language
//#ifdef RBMINI
    if (!askTimeZone)
        Add(new cOsdItem(tr("Please select a language"), osUnknown, false), false, NULL);
    else
        Add(new cOsdItem(tr("Please select a language and timezone"), osUnknown, false), false, NULL);
//#endif
    Add(new cOsdItem(" ", osUnknown, false), false, NULL);
    Add(new cMenuEditStraItem(tr("Setup.OSD$Language"), &osdLanguageIndex, I18nNumLanguagesWithLocale(), &I18nLanguages()->At(0)));

    if (askTimeZone)
    {
        // Timezone
        nr_city_values = 0;
        struct dirent *entry = NULL;
        struct stat buf;
        DIR *conDir = NULL;
        std::string path = "/usr/share/zoneinfo/Europe";
        if ((conDir = opendir(path.c_str())) != NULL)
        {
            while ((entry = readdir(conDir)) != NULL)
            {
                std::string tmp = path + "/" + entry->d_name;
                stat(tmp.c_str(), &buf);
                if (S_ISREG(buf.st_mode))
                {
                    city_values[nr_city_values] = strcpy(city_values[nr_city_values], entry->d_name);
                    nr_city_values++;
                }
            }
            closedir(conDir);
        }
        else
            d(printf("Can not read directory \"%s\" because of error \"%s\"\n", path.c_str(), strerror(errno)))

        Add(new cMenuEditStraItem(tr("City/Region"), &currentCity, nr_city_values, city_values));
    }

    Add(new cOsdItem(" ", osUnknown, false), false, NULL);
    Add(new cOsdItem(tr("Please select the prefered language using the"), osUnknown, false), false, NULL);
    Add(new cOsdItem(tr("cursor keys on the remote control."), osUnknown, false), false, NULL);
    Add(new cOsdItem(tr("Then press OK to continue."), osUnknown, false), false, NULL);

    SetCurrent(Get(current));
    SetHelp(NULL, NULL, tr("Skip"), NULL);
    /* set title again in case of language change */
    char *title;
    asprintf(&title, "%s", tr("Handling"));
    SetTitle(title);
    free(title);

    // Draw Image
    struct StructImage Image;

    Image.x = 300;
    Image.y = 270;
    Image.h = Image.w = 0;
    Image.blend = true;
    Image.slot = 0;
    snprintf(Image.path, 255, "%s/%s", "/usr/share/reel/install", "InstAssRemoteControl.png");
    cPluginManager::CallAllServices("setThumb", (void *)&Image);
    Display();
}

eOSState cInstallPrimLang::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    I18nSetLanguage(osdLanguageIndex);
    int currentchannel = cDevice::CurrentChannel();
    switch (Key)
    {
    case kLeft:
    case kRight:
        Set();                  // rebuild menu since language may be changed
        break;
    case kOk:
        Save();
        return osUser1;
    case kFastFwd:
        /*
         * "secret", undocumented feature: just loads default channellist and exits
         * this is for sellers: it will start the install-wizard on following
         * restarts again - for the end-user
         */
        //XXX nasty
        if (currentchannel > Channels.Count())
            currentchannel = 1;
        Channels.Load(CONFIGDIR "/channels/Satellite.conf");
        // workaround to trigger diseqc codes
        Channels.SwitchTo(currentchannel + 1);
        Channels.SwitchTo(currentchannel);
        return osEnd;

    default:
        break;
    }
    return state;
}

bool cInstallPrimLang::Save()
{
    strn0cpy(Setup.OSDLanguage, I18nLocale(osdLanguageIndex), sizeof(Setup.OSDLanguage));
    Setup.EPGLanguages[0] = Setup.AudioLanguages[0] = osdLanguageIndex;
    I18nSetLanguage(osdLanguageIndex);

    if (askTimeZone)
    {
        cSysConfig_vdr & sysconfig = cSysConfig_vdr::GetInstance();
        std::string buf = std::string("Europe/") + city_values[currentCity];
        sysconfig.SetVariable("ZONEINFO", buf.c_str());
        sysconfig.Save();
        if (remove("/etc/localtime"))
            esyslog("ERROR \"%s\": failed to delete \"/etc/localtime\"\n", strerror(errno));
        buf = std::string("cp /usr/share/zoneinfo/Europe/") + city_values[currentCity] + " /etc/localtime";
        if (SystemExec(buf.c_str()) == -1)
            esyslog("ERROR: failed to execute command \"%s\"\n", buf.c_str());
    }
    Setup.Save();

    return true;
}
