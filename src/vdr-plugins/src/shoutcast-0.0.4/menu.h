#ifndef SHOUTCAST_MENU__H
#define SHOUTCAST_MENU__H

#include <vdr/osdbase.h>
//#include <vdr/tools.h>
#include <vdr/debugmacros.h>
#include <string>
#include <vector>

#include "radioStations.h"


/// shows the list containing Genres and User defined stream server list
class cShoutcastMenu : public cOsdMenu
{
    private:
        std::string selectedGenre_;
        std::vector<std::string> GenreList_;

        bool shouldCallSetupMenu();
        bool calledSetupMenu_;

    public:
        cShoutcastMenu();
        cShoutcastMenu(char*);

        ~cShoutcastMenu();

        eOSState ProcessKey(eKeys);
        void ShowEntries();
        void ReadEntries();
         
        // displays a given string on the OSD
        static void ShowMessage(std::string str);
        static bool triggerRead;
};

extern int selectedGenreNr;
extern char lastServerUrl[256];
extern char lastServerName[128];

enum { SERVER_EDIT, SERVER_ADD};

class cNewServer: public cOsdMenu
{
    int index;
 private :
     char serverName_[81];
     char serverUrl_[256];

     bool SaveUrl();
     bool SaveUrlToFile();
     
     // Add new Server Info / Edit Server Info
     int mode_;
 public:
     cNewServer(int mode, int idx =0); // index: currently selected radio station

     eOSState ProcessKey(eKeys);
};

bool PlayUrlWithXine(std::string url, std::string name);
bool SaveServerListToFile(); // overwrites the file
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
class cFunctionMenu : public cOsdMenu
{
    int index; // index of the station in the User defined station list
    public:
        cFunctionMenu(int idx = -1);
        void ShowCommands();
        eOSState ProcessKey(eKeys key);
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// shows the list of Radio stations, created by user / "favorited" by user
////////////////////////////////////////////////////////////////////////////////

class cRadioStationMenu : public cOsdMenu
{
    private:
        // needed to select the correct entry after the station list has been updated and redrawn
        int currentlySelected_;
    
    public:
        void ShowRadioStations();
        eOSState ProcessKey(eKeys Key);

        cRadioStationMenu();
};
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
