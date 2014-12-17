
// To use STL in vdr
#define  __STL_CONFIG_H

#include "menu.h"
#include <stdio.h>
#include <vdr/tools.h>
#include <vdr/menuitems.h>
#include <vdr/remote.h>         // cRemote
#include <vdr/thread.h>         // SystemExec
#include <vdr/plugin.h>         // cPluginManager
#include <vdr/interface.h>      // Interface->Confirm

#include "setup.h"

#include "../xinemediaplayer/services.h"

#include <vector>
#include <string>


#define GENRELIST "/etc/vdr/plugins/shoutcast.genre"

////////////////////////////////////////////////////////////////////////////////
bool cShoutcastMenu::triggerRead = false;


cShoutcastMenu::cShoutcastMenu():cOsdMenu(tr("Internet Radio"))
{
    calledSetupMenu_ = false;

    ReadEntries();
    ShowEntries();
}

cShoutcastMenu::cShoutcastMenu(char *title):cOsdMenu(title ? tr(title) : "")
{
    calledSetupMenu_ = false;
}

////////////////////////////////////////////////////////////////////////////////
cShoutcastMenu::~cShoutcastMenu()
{
}

///////////////////////////////////////////////////////////////////////////////
bool cShoutcastMenu::shouldCallSetupMenu()      // call setup Menu only once
{
    if (calledSetupMenu_)
        return false;

    if (internetRadioParams.ShouldAskForKey())
    {
        calledSetupMenu_ = true;
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
/// Display GenreList_
/// and "Radio Station SubMenu
///
void cShoutcastMenu::ShowEntries()
{
    HERE;
    Clear();
    unsigned int i;
    char buffer[512];

    if (1 || !cRadioStation::Instance()->IsEmpty())     // TODO always show "Radio Station"
    {
        Add(new cOsdItem(tr("My Radio Stations")));
        Add(new cOsdItem("", osUnknown, false));
    }

    // Genres available at Shoutcast.com
    for (i = 0; i < GenreList_.size(); ++i)
    {
        snprintf(buffer, 511, "%d. %s", i + 1, GenreList_[i].c_str());
        Add(new cOsdItem(buffer, osUnknown, true));
    }


    if (selectedGenreNr < 0)
        SetCurrent(Get(0));
    else
        SetCurrent(Get(selectedGenreNr + 2));

    SetHelp(NULL, NULL, NULL, NULL);
    Display();
}

////////////////////////////////////////////////////////////////////////////////
eOSState cShoutcastMenu::ProcessKey(eKeys key)
{

    eOSState state = cOsdMenu::ProcessKey(key);
    static std::string str_msg;


    if (HasSubMenu())
    {
        if (state == osBack)
        {
            CloseSubMenu();
        }
        return osContinue;
    }

    // Key already acted upon
    if (state == osContinue)
        return state;

    if (internetRadioParams.KeyFileChanged() || !internetRadioParams.IsValidKey())
            internetRadioParams.ReadKey();

/*
    if (shouldCallSetupMenu())
    {
        return AddSubMenu(new cInternetRadioSetupPage(internetRadioParams, false));
    }
*/
    /// Reread the Lists if they have changed (Delete, Add, Edit)
    // Trigger should not be necessary
    if (cShoutcastMenu::triggerRead)
    {
        ReadEntries();
        ShowEntries();

        cShoutcastMenu::triggerRead = false;
    }



    selectedGenreNr = Current();
    if (1 || !cRadioStation::Instance()->IsEmpty())     //TODO always show "Radio Station"
    {
        // selected the radio station list
        if (selectedGenreNr == 0)
        {
            selectedGenreNr = -1;
        }
        else
            selectedGenreNr -= 2;
    }

    switch (key)
    {
    case kOk:

        /// selected osdItem is a Genre
        /// call scast script

        if (selectedGenreNr < 0)
        {
            AddSubMenu(new cRadioStationMenu);
            return osContinue;
        }

        /// selected a Genre

        //check if shoutcast key is right
/*
        if (!(internetRadioParams.KeyFileChanged() && internetRadioParams.ReadKey())
                && !internetRadioParams.IsValidKey())
        {
            return AddSubMenu(new cInternetRadioSetupPage(internetRadioParams, false));
        }
*/
        //printf("Current %i selectedNr %i\n", Current(), selectedGenreNr);

        // get Genre name
        selectedGenre_ = GenreList_[selectedGenreNr];
        str_msg = tr("Getting playlist for Genre: ") + selectedGenre_;
        SetStatus(str_msg.c_str());

        // kill other running scast scripts before starting to download this Genre
        //   Allow user to change his previous selection
        char buffer[128];
        snprintf(buffer, 127, "killall -9 scast ; scast \"%s\" %s",
                 selectedGenre_.c_str(), internetRadioParams.shoutcastKey);
        SystemExec(buffer);

        return osContinue;

        break;

    case kBlue:                // no function here
        break;

    default:
        break;
    }

    return state;
}

////////////////////////////////////////////////////////////////////////////////
/// Read Genres and Radio Server data from files
///
void cShoutcastMenu::ReadEntries()
{
    cReadLine rl;

    /// Read Genre List

    FILE *fp = fopen(GENRELIST, "r");
    if (!fp)
        return;                 // ERROR
    GenreList_.clear();
    char *line;
    while ((line = rl.Read(fp)) != NULL)
    {
        if (strlen(line) > 0)
            GenreList_.push_back(line);
    }
    fclose(fp);

    // User station list is got from cRadioStation singleton class
}

////////////////////////////////////////////////////////////////////////////////
void cShoutcastMenu::ShowMessage(std::string str)
{
    /// CAUTION!
    /// if mesgType other than mtStatus: there is another call using svdrp in Interface->GetKey()
    /// and svdrp command does not exit, use ONLY mtStatus

    // Draw message
    Skins.Message(mtStatus, str.c_str());

    sleep(4);

    // clear message
    Skins.Message(mtStatus, NULL);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/// this OsdMenu, Adds / Edits Internet Radio station infos.
///
/// mode_ defines ADD/EDIT
/// lastServerUrl, lastServerName remembers (through reboots) last entered
/// server data and is stored in setup.conf
///

cNewServer::cNewServer(int mode, int idx):cOsdMenu(tr("Internet Radio - Add New Server"),
                                                   12)
{
    Clear();
    mode_ = mode;
    index = idx;
    serverName_[0] = serverUrl_[0] = 0;
    //strncpy(serverUrl_, "http://38.99.68.236:8045", sizeof(serverUrl_)-1);
    if (mode_ == SERVER_EDIT)
    {
        /// copy the current Server Info
        strncpy(serverUrl_,
                cRadioStation::Instance()->StationList().at(index).Url.c_str(),
                sizeof(serverUrl_) - 1);
        strncpy(serverName_,
                cRadioStation::Instance()->StationList().at(index).Name.c_str(),
                sizeof(serverName_) - 1);

        SetTitle(tr("Internet Radio - Edit Station Info"));
    }
    else
    {
        // XXX check when is this if true ?
        if (mode_ != SERVER_ADD)
        {
            /// use last server info as template
            strncpy(serverUrl_, lastServerUrl, sizeof(serverUrl_) - 1);
            strncpy(serverName_, lastServerName, sizeof(serverName_) - 1);
        }
        else if (mode_ == SERVER_ADD)
        {
            // user default template when adding new server
            strncpy(serverUrl_, "http://", sizeof(serverUrl_) - 1);
            strncpy(serverName_, tr("New Station"), sizeof(serverName_) - 1);

        }


    }                           // else

    Add(new cMenuEditStrItem(tr("Station Name"), serverName_, 80, trVDR(FileNameChars)));
    Add(new
        cMenuEditStrItem(tr("Station Url"), serverUrl_, 120,
                         "abcdefghijklmnopqrstuvwxyz.-_:/0123456789"));

    SetHelp(tr("Save"), NULL, NULL, NULL);
    Display();
}

////////////////////////////////////////////////////////////////////////////////
eOSState cNewServer::ProcessKey(eKeys key)
{
    eOSState state = cOsdMenu::ProcessKey(key);

    if (state == osContinue)
        switch (key)
        {
        case kOk:
        case kBack:
            SetHelp(tr("Save"), NULL, NULL, NULL);
        default:
            return state;
        }

    switch (key)
    {
    case kOk:
        //printf("%s get kOk got osContinue = %i\n", __PRETTY_FUNCTION__, state == osContinue);
        //printf("%s get kOk got osUnknown = %i\n", __PRETTY_FUNCTION__, state == osUnknown);

        if (strlen(serverUrl_) == 0 || strcmp(serverUrl_, "http://") == 0)
        {
            Skins.Message(mtError, tr("No URL entered!"), 3);
            return osContinue;
        }

        switch (state)
        {
        case osContinue:       // out of edit mode
            SetHelp(tr("Save"), NULL, NULL, NULL);

            // Save Url and Name of the Server
            SaveUrl();

            break;

        case osUnknown:

            // Save Url and play
            if (SaveUrlToFile())
                Skins.Message(mtInfo, tr("Saved"), 2);
            else
            {
                Skins.Message(mtError, tr("Could not be saved!"), 3);
                return osContinue;
            }
#if 0
            // play the URL
            if (PlayUrlWithXine(serverUrl_))
                return osEnd;
#endif
            return osUser1;

            break;
        default:
            break;
        }
        break;

    case kRed | k_Repeat:
    case kRed:
        if (strlen(serverUrl_) == 0 || strcmp(serverUrl_, "http://") == 0)
        {
            Skins.Message(mtError, tr("No URL entered!"), 3);
            return osContinue;
        }

        // need a valid URL and Valid name
        if (SaveUrlToFile())
        {
            Skins.Message(mtInfo, tr("Saved"), 2);
            return osUser1;     // saved , so close and go to station list
        }
        else
            Skins.Message(mtError, tr("Could not be saved!"), 3);

        return osContinue;


    case kBack:
        return osBack;

    default:
        break;
    }
    return osContinue;
}


////////////////////////////////////////////////////////////////////////////////
/// Save Server Data to setup.conf
/// so can be used as template for the next server added
///
/// Called everytime a editing is complete!

bool cNewServer::SaveUrl()
{
    strncpy(lastServerUrl, serverUrl_, 255);
    strncpy(lastServerName, serverName_, 127);

    cPlugin *plugin = cPluginManager::GetPlugin("shoutcast");
    if (!plugin)
        return false;

    plugin->SetupStore("lastServerUrl", lastServerUrl);
    plugin->SetupStore("lastServerName", lastServerName);

    ::Setup.Save();

    return true;

}



////////////////////////////////////////////////////////////////////////////////
/// Saves the User Add Internet Radio station names amd urls to a file
///
/// Updates the  Radio Station vector and
/// Calls cRadioStation::Save() which does the real work
///

bool cNewServer::SaveUrlToFile()
{
    SaveUrl();

    cServerInfo newServerInfo(serverName_, serverUrl_);

    if (mode_ == SERVER_ADD)
    {
        if (!cRadioStation::Instance()->AddStation(serverName_, serverUrl_))
        {
            Skins.Message(mtError, tr("Already present"));
            return false;
        }
    }
    else if (mode_ == SERVER_EDIT)
    {
        if (!cRadioStation::Instance()->ReplaceStation(index, newServerInfo))
        {
            Skins.Message(mtError, tr("Couldnot save changes"));
            return false;
        }
    }
    return cRadioStation::Instance()->Save();
}



////////////////////////////////////////////////////////////////////////////////
/// Call xinemediaplayer plugin with one Url
///

bool PlayUrlWithXine(std::string url, std::string name)
{
    printf("PlayUrlWithXine, url = %s\n", url.c_str());
    // not a url
    if (url.find("http://") == std::string::npos)
    {
        Skins.Message(mtError, tr("Check URL: could not open radio-station"));
        return false;
    }

    Skins.Message(mtInfo, tr("Please wait: opening radio-station"));

    std::vector < std::string > playlistEntries;
    playlistEntries.push_back(url);

    std::vector < std::string > namelistEntries;
    namelistEntries.push_back(name);

    Xinemediaplayer_Xine_Play_mrl_with_name xinePlayData = {
        url.c_str(),
        -1,
        false,
        playlistEntries,
        namelistEntries
    };

    // Get plugin
    bool result = false;
    cPlugin *plugin = cPluginManager::GetPlugin("xinemediaplayer");

    if (plugin)
        result = plugin->Service("Xine Play mrl with name", &xinePlayData);
    if (!result)
        return false;

    Xinemediaplayer_Set_Xine_Mode xineModeData = {
        Xinemediaplayer_Mode_shoutcast  // call shoutcast plugin & no "Function" in xine
    };
    plugin->Service("Set Xine Mode", &xineModeData);
    return true;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/// Osd Menu lists "Add new" , "Edit " , "delete" on Osd (the last two are
/// omitted, when the selected item is a Genre)
/// Accordingly, it calls cNewServer( mode=Edit/add) or erases selected server
/// from cRadioStation
///

cFunctionMenu::cFunctionMenu(int idx):cOsdMenu(tr("Internet Radio Functions"), 15)
{
    index = idx;
    ShowCommands();
}

////////////////////////////////////////////////////////////////////////////////
void cFunctionMenu::ShowCommands()
{
    Clear();

    Add(new cOsdItem(tr("1  Add New Radio Station")));


    /// donot show if the selected item is a Genre

    if (index >= 0)
    {
        Add(new cOsdItem(tr("2  Edit current Radio Station")));
        Add(new cOsdItem(tr("3  Delete current  Radio Station")));

        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));

        char buffer[128];

        vector < cServerInfo > stationList = cRadioStation::Instance()->StationList();
        /// selected station info

        Add(new cOsdItem(tr("Selected Radio Station:"), osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));

        snprintf(buffer, 127, "%s:\t%s", tr("Station Name"),
                 stationList.at(index).Name.c_str());
        Add(new cOsdItem(buffer, osUnknown, false));

        snprintf(buffer, 127, "%s:\t%s", tr("Station Url"),
                 stationList.at(index).Url.c_str());
        Add(new cOsdItem(buffer, osUnknown, false));
    }

    SetCurrent(Get(0));

    Display();
}

////////////////////////////////////////////////////////////////////////////////

eOSState cFunctionMenu::ProcessKey(eKeys key)
{
    eOSState state = cOsdMenu::ProcessKey(key);

    // Keys from SubMenu
    if (HasSubMenu())
    {
        if (state == osBack)
        {
            CloseSubMenu();
        }
        else if (state == osUser1)
        {
            CloseSubMenu();
            return osUser1;
        }
        return osContinue;
    }

    if (state == osContinue)
        return state;

    // handle keys from FunctionMenu
    switch (key)
    {
    case kOk:
        switch (Current())
        {
        case 0:                // New Entry
            AddSubMenu(new cNewServer(SERVER_ADD));
            return osContinue;


        case 1:                // Edit Entry
            AddSubMenu(new cNewServer(SERVER_EDIT, index));

            return osContinue;
            break;


        case 2:                // Delete Entry

            if (cRadioStation::Instance()->DelStation(index))
            {
                if (!cRadioStation::Instance()->Save())
                    Skins.Message(mtInfo, tr("Could not save to disk"));

                return osUser1;
            }
            else
                Skins.Message(mtInfo, tr("Deletion failed"), 1);

            return osContinue;


        default:
            break;
        }
        break;

    case kBack:
        return osBack;

    default:
        return osUnknown;
    }

    return state;
}


cRadioStationMenu::cRadioStationMenu():cOsdMenu(tr("Internet Radio: My Stations"))
{
    currentlySelected_ = 0;
    ShowRadioStations();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void cRadioStationMenu::ShowRadioStations()
{
    #define MAX_LETTER_PER_LINE 50

    //printf("%s\n", __PRETTY_FUNCTION__);
    vector < cServerInfo > stationList = cRadioStation::Instance()->StationList();
    cRadioStation::Instance()->ResetDirty();

    char buffer[128];
    Clear();
    unsigned int i = 0;
    for (; i < stationList.size(); ++i)
    {
        snprintf(buffer, 127, "%i. %s", i + 1, stationList.at(i).Name.c_str());
        //printf("%s\n", buffer);
        Add(new cOsdItem(buffer));
    }
    SetHelp(NULL, NULL, NULL, tr("Button$Functions"));

    // empty station list display a message
    if (stationList.size() <= 0)
    {
        Add(new cOsdItem(tr("No Radio Stations found."), osUnknown, false));
        Add(new cOsdItem((""),osUnknown, false) );
        Add(new cOsdItem((""),osUnknown, false));
        Add(new cOsdItem((""),osUnknown, false));
        Add(new cOsdItem((""),osUnknown, false));
        Add(new cOsdItem((""),osUnknown, false));
        Add(new cOsdItem((""),osUnknown, false));
        Add(new cOsdItem((""),osUnknown, false));
        Add(new cOsdItem((""),osUnknown, false));
        Add(new cOsdItem((""),osUnknown, false));
        Add(new cOsdItem((""),osUnknown, false));
        Add(new cOsdItem(tr("Info:"),osUnknown, false));
        AddFloatingText(tr("Please use 'Function' to add a station."), MAX_LETTER_PER_LINE);
    }

    // Select the currently selected again
    if (currentlySelected_ >= (int)stationList.size())
        currentlySelected_ = stationList.size() - 1;
    if (currentlySelected_ < 0)
        currentlySelected_ = 0;

    SetCurrent(Get(currentlySelected_));
    Display();
    //printf("\033[0;91m Setting current to %i\033[0m\n", currentlySelected_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
eOSState cRadioStationMenu::ProcessKey(eKeys Key)
{

    eOSState state = cOsdMenu::ProcessKey(Key);

    // Key already acted upon
    if (state == osContinue)
        return state;

    if (HasSubMenu())
    {
        if (state == osBack)
        {
            CloseSubMenu();
        }
        else if (state == osUser1)      // indicate stationList_ has changed
        {
            CloseSubMenu();
        }

        return osContinue;
    }

    // state == osUnknown here
    currentlySelected_ = Current();
    //printf("%i / %i\n\n", currentlySelected_, Count());
    if (!SelectableItem(currentlySelected_))    // "empty" Osd
        currentlySelected_ = -1;

    switch (Key)
    {
    case kOk:
        {
            if (cRadioStation::Instance()->IsEmpty())
                break;

            vector < cServerInfo > stationList = cRadioStation::Instance()->StationList();
            PlayUrlWithXine(stationList.at(Current()).Url,
                            stationList.at(Current()).Name);
        }
        break;

    case kBlue:
        AddSubMenu(new cFunctionMenu(currentlySelected_));
        break;

    case kBack:
        return osBack;

    default:
        // Display list again only if stationList has changed
        if (cRadioStation::Instance()->Dirty())
        {
            ShowRadioStations();
            cRadioStation::Instance()->ResetDirty();
        }
        return osUnknown;
    }
    return osContinue;
}
