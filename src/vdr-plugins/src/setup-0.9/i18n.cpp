/****************************************************************************
 * DESCRIPTION:
 *             Internationalisation Strings
 *
 * $Id: i18n.cpp,v 1.14 2005/10/12 13:44:14 ralf Exp $
 *
 * Contact:    ranga@vdrtools.de
 *
 * Copyright (C) 2004 by Ralf Dotzert
 ****************************************************************************/
/*
   Language support for minivdr generic menus
   written by movimax  mhahn@reel-multimedia.com
*/

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <string>

#include <tinyxml/tinyxml.h>
#include <vdr/plugin.h>
#include "i18n.h"

using std::string;
using std::map;
using std::cout;
using std::cerr;
using std::endl;
using std::pair;


const tI18nPhrase *
    Phrases = NULL;

#if 0
/// put any strings here that should be translated but are not inside the code
///  but come from the vdr-setup.xml file
//Main Menu Entries
trNOOP("TV & Radio");
trNOOP("Music & Pictures");
trNOOP("Films & DVD");
trNOOP("Internet & Extras");
trNOOP("Image Library");
trNOOP("Music Library");
trNOOP("Movie Library");
trNOOP("Additional Software");

//Setup Menu Entries
trNOOP("Installation");
trNOOP("Basic Settings");
trNOOP("Video Recorder ");
trNOOP("Background Services");
trNOOP("System Settings");

//Basic Settings
trNOOP("Video Settings");
trNOOP("Audio Settings");
trNOOP("Reception Settings");
trNOOP("OSD & Language");
trNOOP("Language");
trNOOP("Timezone & Clock");
trNOOP("EPG Services");
trNOOP("International EPG");
trNOOP("Energy Options");
trNOOP("Reset to factory defaults");

//Receiving
trNOOP("Edit Channels");

//EPG Services
trNOOP("TVTV Service");

//Network
trNOOP("Network Settings");
trNOOP("UMTS Settings");
trNOOP("Enable UMTS to connect your ReelBox to the");
trNOOP("internet using UMTS connection.");
trNOOP("Enable UMTS");
trNOOP("UMTS Provider");
trNOOP("Common Interfaces");
trNOOP("Energy Saving Mode");
trNOOP(" Activate after (h)");
trNOOP("Eject Media on Shutdown");
trNOOP("Channel Lists");
trNOOP("Global Settings");
trNOOP("Hostname");
trNOOP("Domain");
trNOOP("Gateway");
trNOOP("Nameserver");
trNOOP("Use Proxy");

//Streaming-Clients
trNOOP("Streaming Clients");
trNOOP("Setup type of external streaming devices and stream quality");
trNOOP("You can select between 'Tablet' for");
trNOOP("Tablet / Smartphone devices or 'TS' for");
trNOOP("default settings *Standard to stream");
trNOOP("LiveTV to your device.");
trNOOP("Stream Type");
trNOOP("Client Speed");
trNOOP("Select client connection type for best stream");
trNOOP("quality");
trNOOP("Ethernet Settings");
trNOOP("Enable Ethernet");
trNOOP("Enable Ethernet 1");
trNOOP("Enable Ethernet 2");
trNOOP(" Use DHCP");
trNOOP(" IP Address");
trNOOP(" Network mask");
trNOOP("NetCeiver bridge");
trNOOP("Enable Wake-on-LAN");
trNOOP("Wakeup ReelBox Avantgarde on startup");
trNOOP("Export Harddisk for Windows");
trNOOP("Workgroup");
trNOOP("Enable UPnP service");
trNOOP("Use Windows Client");
trNOOP("Windows Network Connection");
trNOOP("Windows Server");
trNOOP("Sharename");
trNOOP("DVD archive directory");
trNOOP("Use NFS Client");
trNOOP("NFS Client");
trNOOP("Expert Settings");
trNOOP("Host/IP");
trNOOP("Allow writing");
trNOOP("Use locking");
trNOOP("Buffersize");


trNOOP("Mark Blocks after Recording");
trNOOP("Playback Settings");
trNOOP("Harddisk/Media device");
trNOOP("Channel Scan");
trNOOP("Installation Wizard");
trNOOP("Use tvtv service");
trNOOP("GnuDIP service");
trNOOP("DynDNS service");
trNOOP("Use GnuDIP service");
trNOOP("Use DynDNS service");
trNOOP("DynDNS hostname");


trNOOP("Recording settings");
//Front panel
trNOOP("LEDs while operating");
trNOOP("LEDs in Standby");
trNOOP("Turn off LEDs in Standby");
trNOOP("Color");
trNOOP("Front Panel");
trNOOP("Brightness");
trNOOP("Brightness Status LEDs");
trNOOP("Display Brightness");
trNOOP("LC Display Type");
trNOOP("Black Series");
trNOOP("External Reel Display Module");
trNOOP("off");
trNOOP("blue");
trNOOP("red");
trNOOP("min");
trNOOP("low");
trNOOP("med");
trNOOP("high");
trNOOP("max");

//Installation/Software
trNOOP("Update from CD/DVD");
trNOOP("Software update");
trNOOP("Software Update");
trNOOP("Update-check on system start");
trNOOP("Manual Network Connections");
trNOOP("Dynamic DNS");
trNOOP("Remote Control / Automation");
trNOOP("Enable Control via RS232");
trNOOP("  Port speed (baud)");
trNOOP("default (115.200)");
trNOOP("Enable Control via UDP");
trNOOP("UDP Port");
trNOOP("Disable Bluetooth Support");


//ToolTips
trNOOP("Software Update and Installation, Serial Remote control");
trNOOP("Update the ReelBox software by Internet");
trNOOP("Install additional features");
trNOOP("Basic settings in just a few steps");
trNOOP("Remote control through serial port or network");
trNOOP("Set up the Reel MultiRoom");
trNOOP("Video and Audio Settings, OSD, Timezone, Front panel, Energy options");
trNOOP("Adjust automatic standby and poweroff");
trNOOP("Video output, Resolution, Screen format etc.");
trNOOP("Audio output and format, Audio/Video delay etc.");
trNOOP("Picture adjustments: Brightness, Contrast, Gamma");
trNOOP("Set your language and on-screen-display options");
trNOOP("Time zone and source for automatic setting of system time");
trNOOP("LED color and brightness, LCD brightness");
trNOOP("Adjust Energy saving options");
trNOOP("Set up local Network and Internet, connect Network storage");
trNOOP("Setup Wireless network: SSID, Encryption etc.");
trNOOP("Set up LAN ports, DHCP/IP settings");
trNOOP("Host name, Domain, Gateway, Proxy, Wake-on-LAN, Dynamic DNS Services");
trNOOP("Search and administer network shares");
trNOOP("Setup UMTS dial-up network connection");
trNOOP("Settings for Timeshift, Recordings, Playback, Media storage");
trNOOP("Settings for Timeshift");
trNOOP("Lead in & out time, VPS, Automatic marking of ads, Audio tracks");
trNOOP("Playback options for TV recordings");
trNOOP("Set up drive for Media files and Recordings");
trNOOP("Configure background services and tasks");
trNOOP("Channel scan & lists, DiSEqC settings, Common Interface, Signal information");
trNOOP("Channel lists, DiSEqC settings, Signal information");
trNOOP("Automatic or manual channel scan");
trNOOP("Choose a preconfigured channel list");
trNOOP("Receiveable satellite positions, Number of active tuners / LNBs");
trNOOP("Test and adjust your rotor");
trNOOP("Overview and configuration of Common Interface modules");
trNOOP("Details on Signal strength & quality, Video & audio bit rates etc.");
trNOOP("Advanced setup for system programs");
trNOOP("Information on Hardware, Network and System health");
trNOOP("Settings needed to serve Reel NetClients");
trNOOP("Enable NetCeiver bridge");
trNOOP(" Bridge NetCeiver to");
trNOOP("Use VLAN for NetCeiver");
trNOOP(" VLAN ID");
trNOOP("Prepare harddisk for use in the ReelBox"):

trNOOP("Can cause problems when MultiRoom");
trNOOP("is on and should not be activated.");
#endif

const tI18nPhrase
    defaultPhrases[] = {
/*
*/
    {NULL}
};

const char *
    languages[] = {
    "eng",
    "deu",
    "slv",
    "ita",
    "dut",
    "por",
    "fra",
    "nor",
    "fin",
    "pol",
    "esl",
    "ell",
    "sve",
    "rom",
    "hun",
    "cat",
    "rus",
    "hrv",
    "est",
    "dan",
    "cze",
};

struct I18nPhrStruct
{
    string
        str[I18nNumLanguages];
};

map < string, I18nPhrStruct > xmlPhrasesMap;

// -------  class LoadXmlPhrases ---------------------------

LoadXmlPhrases::LoadXmlPhrases()
{
    string xmlFileName = cPlugin::ConfigDirectory();
    cerr << "xmlFilePath: " << xmlFileName << endl;

    xmlFileName += "/setup/setup-lang.xml";

    TiXmlDocument doc(xmlFileName.c_str());

    //bool loadOkay = doc.LoadFile();
    //fprintf(stderr,"nach doc.LoadFile ReadDoc: %s \n", loadOkay?"success":"failure");

    if (!doc.LoadFile())
    {
        esyslog("Could not load file %s 'setup-lang.xml'. Error='%s' ",
                xmlFileName.c_str(), doc.ErrorDesc());
        AddDefaultPhrases();
        return;
    }

    TiXmlNode *node = NULL;
    TiXmlElement *langElement = NULL;

    int count = 0;
    for (node = doc.IterateChildren(NULL); node;
         node = doc.IterateChildren(node))
    {
        langElement = node->ToElement();
        if (langElement)
        {
            ///<  we add Language to Array
            AddLanguagePhrases(langElement);
            count++;
        }
    }
    LoadPhrases();
}

// ------------------ Methoden ---------------------------------------------------

int
LoadXmlPhrases::GetLangIndex(const char *lang_abr)
{
    for (int i = 0; i < I18nNumLanguages; i++)
    {
        if (strcmp(languages[i], lang_abr) == 0)
            return i;
    }

    return -1;
}


void
LoadXmlPhrases::PushPhrase(string entryStr, string translatedEntryStr,
                           int idx)
{

#if 0
    cerr << "PushPhrase: " << entryStr << endl;
    cerr << "MenuEntryStr :" << entryStr << endl;
    cerr << "Translatation :" << translatedEntryStr << "size: " <<
        translatedEntryStr.size() << endl;
    cerr << "for Lang idx : " << idx << endl;

    if (translatedEntryStr.size() <= 0)
    {
        cerr << "string is emtpy!\n";
        if (translatedEntryStr.empty())
            cerr << "string is emtpy()!\n";

        return;
    }
#endif

    //test if we have entryStr already in map

    if (xmlPhrasesMap.count(entryStr) == 0)
    {
        I18nPhrStruct tmp;
        tmp.str[0] = entryStr;  ///< entryStr is key and! first val, to ease the copy to Phrases
        tmp.str[idx] = translatedEntryStr;
        //  cerr << "insert to map <pair " << entryStr << ", " << tmp.str[idx] << endl;
        xmlPhrasesMap.insert(pair < string, I18nPhrStruct > (entryStr, tmp));
    }
    ///< just add  Val to Key at right position
    else
    {
        map < string, I18nPhrStruct >::iterator iter;
        iter = xmlPhrasesMap.find(entryStr);
        if (iter != xmlPhrasesMap.end())
            iter->second.str[idx] = translatedEntryStr;

    }

}

void
LoadXmlPhrases::AddDefaultPhrases()
{
    for (int i = 0; *defaultPhrases[i] != NULL; i++)
    {
        //cout << "default: " << defaultPhrases[i][0] << endl;
        I18nPhrStruct tmp;
        for (int j = 0; j < I18nNumLanguages; j++)
        {
            if (defaultPhrases[i][j])
                tmp.str[j] = defaultPhrases[i][j];
            else
                tmp.str[j] = "";


        }
        xmlPhrasesMap.insert(pair < string,
                             I18nPhrStruct > (static_cast < string >
                                              (defaultPhrases[i][0]), tmp));
    }
}

void
LoadXmlPhrases::AddLanguagePhrases(TiXmlElement * langElement)
{
    int langIndex = GetLangIndex(langElement->Attribute("language"));
    if (langIndex == -1)
    {
        esyslog(
                "setup: wrong lang ID Skiping %s take internet country code (TLD) eg. deu for Germany  ",
                langElement->Attribute("language"));
        return;
    }
    //cerr << " ++ LangElement: " << langElement->Attribute("language") << " @array: " << langIndex   << endl;

    TiXmlElement *entryElement = 0;


    for (entryElement = langElement->FirstChildElement();
         entryElement; entryElement = entryElement->NextSiblingElement())
    {
        if (entryElement)
        {
            //cerr << "GetEntry: "<<  entryElement->Attribute("entry") << "->" << entryElement->FirstChild()->Value() << endl;
            PushPhrase(entryElement->Attribute("entry"),
                       entryElement->FirstChild()->Value(), langIndex);
        }
    }
}

// -------- LoadPhrases() -------------------------------

void
LoadXmlPhrases::LoadPhrases()
{
    // cerr << "Load Phrase \n";
    int bytes = 0;
    int itemsNum = xmlPhrasesMap.size();

    for (map < string, I18nPhrStruct >::iterator iter = xmlPhrasesMap.begin();
         iter != xmlPhrasesMap.end(); ++iter)
    {
        for (int i = 0; i < I18nNumLanguages; i++)
        {
            //cout << "xmlEntry: " << iter->first  << " has " << iter->second.str[i].size() << " bytes "<< endl;
            bytes += iter->second.str[i].size() + 1;    // for string terminator '\0'
        }
    }
#if 0
    cout << "map Items: " << itemsNum << endl;
    cout << "we need to allocate " << bytes << " Bytes  of memmory \n";
    cout << " *** dump old phrases *** \n";

#endif
    // XXX
    for (int i = 0; *defaultPhrases[i] != NULL; i++)
    {
        itemsNum++;
        for (int j = 0; j < I18nNumLanguages; j++)
        {
            if (defaultPhrases[i][j])
                bytes += strlen(defaultPhrases[i][j]) + 1;
            else
                bytes += 1;
        }
    }
#if 0
    cout << "map Items +default: " << itemsNum << endl;
    cout << "we need to allocate " << bytes << " Bytes  of memmory \n";

#endif

    cout << " AddDefaultPhrases\n";
    AddDefaultPhrases();

    tI18nPhrase *phraseArr = new tI18nPhrase[itemsNum + 1];
    int i = 0;
    for (map < string, I18nPhrStruct >::iterator iter = xmlPhrasesMap.begin();
         iter != xmlPhrasesMap.end(); ++iter)
    {
        for (int j = 0; j < I18nNumLanguages; j++)
        {
            char *tP = new char[iter->second.str[j].size() + 1];
            strcpy(tP, iter->second.str[j].c_str());
            phraseArr[i][j] = tP;
        }
        i++;
    }

    //the last element items have to be NULL
    for (int j = 0; j < I18nNumLanguages; j++)
        phraseArr[itemsNum][j] = NULL;

    Phrases = phraseArr;

}

// only for debug
// ----- DumpMap() ------------------------------------

void
LoadXmlPhrases::DumpMap()
{
    cerr << " >>> I18nPhrStruct prints <<< \n\n";

    for (map < string, I18nPhrStruct >::iterator iter = xmlPhrasesMap.begin();
         iter != xmlPhrasesMap.end(); ++iter)
    {
        cout << "xmlEntry: " << iter->first << endl;

        for (int i = 0; i < I18nNumLanguages; i++)
            if (!iter->second.str[i].empty())
            {
                cout << languages[i] << ": xmlPhrases " << iter->second.
                    str[i] << " " << endl;
            }

    }

}

void
DumpPhrases()
{
    cerr << " >>> I18nPhrases dump <<< \n\n";

    for (int i = 0; *Phrases[i] != NULL; i++)
    {
        for (int j = 0; j < I18nNumLanguages; j++)
            fprintf(stderr, "phrase: %s\n", Phrases[i][j]);
    }
}
