/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia                                 *
 *                                                                         *
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
 ***************************************************************************/

// xine_mediaplayer.c
#include "cddb.h"

#include "xinemediaplayer.h"

#include "Player.h"
#include "Control.h"
#include "services.h"
#include <vdr/remote.h>

#include <cstring>
#include <string>
#include <vector>

#include "m3u_parser.h"
#include "pls_parser.h"

#include "timecounter.h"

#include "Playlist.h"
cTimeCounter timeCounter;

//static const char *VERSION = "0.13"
#define LAST_PLAYED_PLAYLIST "last_played.playlist"

namespace Reel {

namespace XineMediaplayer {

/// Last played playlist  ------------------------------------------------------
/// Read and save playlist

LastPlayedList::LastPlayedList()
{

}

void LastPlayedList::SetFilename(std::string Filename)
{
    if (Filename.size())
    {
        filename = Filename;
        return;
    };

    const char* configDir = cPlugin::ConfigDirectory("xinemediaplayer");
    if (!configDir) {
        esyslog("(%s:%d) config Directory returns NULL. File name not set.", __FILE__, __LINE__);
        return;
    };
    
    filename = std::string(configDir)
            + std::string("/") + LAST_PLAYED_PLAYLIST;

    std::cerr << "\n\033[0;91m ############ " << filename << " ############# \033[0m"<< std::endl;

    //filename = std::string("/tmp/") + LAST_PLAYED_PLAYLIST;
};

LastPlayedList::~LastPlayedList()
{
    Save();
};

bool LastPlayedList::Read()
{
    playlist = ReadPlaylist(filename);
    return playlist.size() > 0;
};

bool LastPlayedList::Save()
{
    return SavePlaylist(filename, playlist);
};

void LastPlayedList::SetPlaylist(const std::vector<std::string> &newPlaylist)
{
    playlist = newPlaylist;
    
    // Save the changes
    Save();
};

LastPlayedList savedPlaylist;

/* when non-zero the saved playlist is used when openning "media-libraray"
 * else empty playlist is opened. */
int remember_playlist; 
/// ----------------------------------------------------------------------------

#define OPEN_XINEOSD_IF_NEEDED do { if (cXineOsd::browserInstance > -5) cRemote::CallPlugin("xinemediaplayer"); } while (0)

    // time counter : holds the duration of the time a player session was live, minus the paused duration
    namespace {
        PCSTR VersionString     =  VERSION;
        PCSTR DescriptionString = "Xine Mediaplayer plugin";
        // TODO: maybe only add supported file types?
        static const char *DEFAULT_EXTENSIONS = "! cue bin deb txt jpg jpeg bmp tif tiff xpm m3u playlist mp3 wav ogg gif png";
    };

    Plugin::Plugin() : mExtensions(DEFAULT_EXTENSIONS) {
        remember_playlist = 0; // default dont remember playlist
    }; // Plugin::Plugin

    Plugin::~Plugin() {
        SetupStore("currentPlaylistName", currentPlaylistFilename.c_str());
        SetupStore("rememberPlaylist", remember_playlist);
    };  // Plugin::~Plugin

    PCSTR Plugin::Version() {
        return VersionString;
    }; // Plugin::Version

    PCSTR Plugin::Description() {
        return DescriptionString;
    }; // Plugin::Description

    bool Plugin::SetupParse(const char *Name, const char *Value) {
        if (!strcasecmp(Name, "Extensions")) mExtensions = Value;
        else if (!strcasecmp(Name, "DelayAudio")) ; // Settings used by XineLib
        else if (!strcasecmp(Name, "DelaySPU"))   ; // Settings used by XineLib
        else if (!strcasecmp(Name, "currentPlaylistName")) {
            currentPlaylistFilename = Value;
            return true;
        } else if (!strcasecmp(Name, "rememberPlaylist"))
            remember_playlist = atoi(Value);
        else
            return false;
        return true;
    }; // Plugin::SetupParse

    bool Plugin::Initialize() { 
        // loads default file
        savedPlaylist.SetFilename();

        savedPlaylist.Read();
        return true;
    };  // Plugin::Initialize

    const char **Plugin::SVDRPHelpPages(void) {
        static const char *HelpPages[] = {
            "PLAYM3U <filename>\n"
            "    Triggers playback of playlist 'filename'.",
            NULL
        };
        return HelpPages;
    }; // Plugin::SVDRPHelpPages
    
    bool Plugin::HasSetupOptions() {
        return false;
    }; // Plugin::HasSetupOptions
    
    void print_vec(std::vector<std::string>& vec) {
        unsigned int i=0;
        for (; i<vec.size(); ++i)
            printf("%s\n",vec.at(i).empty()?"(null)":vec.at(i).c_str() );
            //std::cout<<vec.at(i)<<std::endl;
    }; // print_vec

    bool start_xinemediaplayer_with_m3u(const char* m3u_filename, int plugin_to_call) {
        std::vector<std::string> url_list, name_list;
        if ( m3u_parser( m3u_filename, url_list, name_list ) ) {
            // set names and url_list
            cXineOsd::playListEntries = url_list;
            cXineOsd::nameListEntries = name_list;
            /*
            print_vec( url_list );
            std::cout<<std::endl<<" xine playlist entries \n";
            print_vec( cXineOsd::playListEntries );*/
            // std::cout<<std::endl<<"name list \n";
            //print_vec( name_list );
            /*
            std::cout<<std::endl<<"Xine namelist entries\n";
            print_vec( cXineOsd::nameListEntries );
            */
            if ( !cXineOsd::isPlaying ) { // Donot launch new control if already playing, just add to existing playlist
                cXineOsd::control = new Control( url_list[0].c_str() , 1, url_list );
                ::cControl *control = cXineOsd::control;
                ::cControl::Launch(control);
            } else {
                //Refresh
                cXineOsd::isNew = true; 
                //LOC; printf("\n\tSetting new playlist\n");
                cXineOsd::control->SetPlayList( cXineOsd::playListEntries );
            }; // if
            cXineOsd::k_blue = plugin_to_call;

            // open xineOSD
            if (cXineOsd::k_blue != MUSIC_BROWSER)
                cXineOsd::browserInstance = 0; // anything > -5

            cXineOsd::kPausePressed = 0;
            OPEN_XINEOSD_IF_NEEDED;
            return true;
        } else
            return false;
    }; // start_xinemediaplayer_with_m3u

    cString Plugin::SVDRPCommand(const char* Command, const char *Option,  int &ReplyCode) {
        if(!strcasecmp(Command,"PLAYM3U") || !strcasecmp(Command,"PLAYM3U_SHOUTCAST") || !strcasecmp(Command,"PLAYM3U_IPOD" ) ||  !strcasecmp(Command,"PLAYM3U_FILEBROWSER") ) {
            if (!Option) {
                ReplyCode = 550;
                return "a m3u file missing";
            }; // if
            int plugin_to_start = NO_PLUGIN;
            if ( !strcasecmp(Command,"PLAYM3U") ) plugin_to_start = NO_PLUGIN;
            else if ( !strcasecmp(Command,"PLAYM3U_FILEBROWSER") ) plugin_to_start = FILEBROWSER_PLUGIN; 
            else if ( !strcasecmp(Command,"PLAYM3U_SHOUTCAST") ) plugin_to_start = SHOUTCAST_PLUGIN;
            else if ( !strcasecmp(Command,"PLAYM3U_IPOD") ) plugin_to_start = IPOD_PLUGIN;

            if ( start_xinemediaplayer_with_m3u (Option, plugin_to_start ) ) {
                ReplyCode = 900;
                return cString::sprintf("xinemediaplayer called with %s", Option );
            } else {
                ReplyCode = 550;
                return "m3u parser error";
            }; // if
        }; // if
        ReplyCode = 550;
        return NULL;
    }; // Plugin::SVDRPCommand


    void SetXinePlayInfo(Xinemediaplayer_Xine_Play_Info* info)
    {
        if (!info)
            return;

        info->isPlaying = cXineOsd::isPlaying && cXineOsd::control;

        if (!cXineOsd::control)
            return;

        int currentlyPlaying = 0, current = 0, total = 0;

        // time elapsed and duration of current mrl
        cXineOsd::control->PlayerPositionInfo(currentlyPlaying, current, total);
        info->elapsed = current / DEFAULTFRAMESPERSECOND;
        info->total   = total / DEFAULTFRAMESPERSECOND;

        // xine play mode : shuffle, random, loop, normal
        info->play_mode = cXineOsd::control->GetPlayMode();

        // the absolute path of the currently played file
        if (currentlyPlaying >= 0 && (unsigned int)currentlyPlaying < cXineOsd::playListEntries.size())
            info->mrl = cXineOsd::playListEntries.at(currentlyPlaying);
        if (currentlyPlaying >= 0 && (unsigned int)currentlyPlaying < cXineOsd::nameListEntries.size())
            info->name = cXineOsd::nameListEntries.at(currentlyPlaying);
    };


    bool Plugin::Service(char const *id, void *data) {
        if(!id) return false;
        std::cout << " ########   Service('" << id << "'" << std::endl;

        if (strcasecmp(id, "Xine Get Play Info") == 0)
        {
            Xinemediaplayer_Xine_Play_Info *info =
                    static_cast<Xinemediaplayer_Xine_Play_Info*>(data);

            if (info)
                SetXinePlayInfo(info);

            return true;
        } else
        if (strcasecmp(id, "Set Xine Mode") == 0){
            Xinemediaplayer_Set_Xine_Mode *xineMode = static_cast<Xinemediaplayer_Set_Xine_Mode *> (data);
            if ( !xineMode ){
                cXineOsd::k_blue = NO_PLUGIN;
                return true;
            }; // if
            switch (xineMode->mode) {
                case Xinemediaplayer_Mode_none: // no plugin
                    cXineOsd::k_blue = NO_PLUGIN;
                    break;
                case Xinemediaplayer_Mode_filebrowser: // filebrowser
                    cXineOsd::k_blue = FILEBROWSER_PLUGIN;
                    break;
                case Xinemediaplayer_Mode_shoutcast: // shoutcast
                    cXineOsd::k_blue = SHOUTCAST_PLUGIN;
                    break;
                case Xinemediaplayer_Mode_mmfiles: // mmfiles
                    cXineOsd::k_blue = MMFILES_PLUGIN;
                    break;
                case Xinemediaplayer_Mode_youtube: // youtube
                    cXineOsd::k_blue = YOUTUBE_PLUGIN;
                    break;
                case XineMediaplayer_Mode_musicbrowser:
                    cXineOsd::k_blue = MUSIC_BROWSER;
                    break;
                default: // no plugin
                    cXineOsd::k_blue = NO_PLUGIN;
                    break;
            }; // switch
            return true;
/*
        } else if (std::strcmp(id, "Xine Status") == 0) {
            struct XineStatus {
                bool status;
            };
            return true;
*/
#if ! RBMINI
        } else if (std::strcmp(id, "Xine Play VideoCD")==0) {
            if(data) return false;
            CdIo_t *cdio;               /* libcdio CD access structure */
            char *device = cdio_get_default_device(NULL);
            if (!device) {
                esyslog ("unable to get default CD device");
                Skins.Message(mtError, tr("Unable to get default CD device")); 
                return true;
            }; // if
            /* Load the appropriate driver and open the CD-ROM device for reading. */
            cdio = cdio_open(device, DRIVER_UNKNOWN);
            if (!cdio) {
                esyslog("unable to open CD device");
                Skins.Message(mtError, tr("Unable open CD device")); 
                return true;
            }; // if
            /* Get the track count for the CD. */
            int count = cdio_get_num_tracks(cdio); 
            cdio_destroy(cdio);
            if (count == 0) {
                esyslog("found no tracks in CD");
                Skins.Message(mtError, tr("No tracks found in CD"));
                return true;
            }; // if
            cXineOsd::playListEntries.clear();
            cXineOsd::nameListEntries.clear();
            char buffer[16];
            for (int i=1; i<= count; ++i) {
                snprintf(buffer,15, "vcd://%i", i);
                cXineOsd::playListEntries.push_back(buffer);
                snprintf(buffer,15, "Track - %i", i);
                cXineOsd::nameListEntries.push_back(buffer);
            }; // for
            cXineOsd::isNew = 1;
            if ( cXineOsd::isPlaying ) { // Donot launch new control if already playing, just replace existing playlist
                cXineOsd::control->SetPlayList( cXineOsd::playListEntries );
                cXineOsd::firstOpen = false;
            } else {
                cXineOsd::control = new Control(cXineOsd::playListEntries.at(0).c_str(), 1, cXineOsd::playListEntries);
                ::cControl *control = cXineOsd::control;
                ::cControl::Launch(control);
                cXineOsd::firstOpen = true;
            }; // if

            cXineOsd::k_blue = NO_PLUGIN;
            cXineOsd::kPausePressed = 0;
            OPEN_XINEOSD_IF_NEEDED;
            return true;
        } else if (std::strcmp(id, "Xine Play AudioCD")==0) {
            if(data) return false;
            // Get number of cd tracks
            cCdInfo&cdInfo = cCdInfo::GetInstance();
            int ret = cdInfo.AudioCdInit();
            int totalCdTracks = cdInfo.TrackCount();
            if (ret && totalCdTracks <=0 ) {
                Skins.Message(mtError, tr("No audio tracks found"));
                // shutdown splash screen
                cPlugin *p = cPluginManager::GetPlugin("mediad");
                if (p)
                    p->Service("HideSplashScreen");
                return true;
            } // if
            // build playlist
            vector <string> audioCdPlaylist;
            vector <string> trackNames;
            char buffer[32];
            for (int i=0; i<totalCdTracks; ++i) {
                // if starting track num != 1, does xine require 
                // track with track number or track index ? //TODO //XXX
                snprintf(buffer,sizeof(buffer), "cdda://%i", cdInfo.TrackNumberByIndex(i));
                audioCdPlaylist.push_back(buffer);
            } // for
            //Get Track names/titles
            string title,artist, name;
            for (int i=0; i< totalCdTracks; ++i) {
                title = cdInfo.TrackTitleByIndex(i);
                artist = cdInfo.TrackArtistByIndex(i);
                if (title.empty()) {
                    snprintf(buffer, 31, "%s - %i", tr("Track"), cdInfo.TrackNumberByIndex(i));
                    name = buffer;
                } else
                    name = title + " - " + artist;
                trackNames.push_back(name);
            } // for
            cXineOsd::playListEntries = audioCdPlaylist;
            cXineOsd::nameListEntries = trackNames;
            cXineOsd::isNew = 1;
            if ( cXineOsd::isPlaying ) {// Donot launch new control if already playing, just replace existing playlist
                cXineOsd::control->SetPlayList( cXineOsd::playListEntries );
            } else {
                cXineOsd::control = new Control(cXineOsd::playListEntries.at(0).c_str(), 1, cXineOsd::playListEntries);
                ::cControl *control = cXineOsd::control;
                ::cControl::Launch(control);
            } // if
            cXineOsd::k_blue = NO_PLUGIN;
            cXineOsd::kPausePressed = 0;
            OPEN_XINEOSD_IF_NEEDED;
            return true;
#endif /* !RBMINI */
        } else if (std::strcmp(id, "Xine Test mrl") == 0) {
            if(!data) return false;
            static_cast<Xinemediaplayer_Xine_Test_mrl *>(data)->result = checkExtension(static_cast<Xinemediaplayer_Xine_Test_mrl *>(data)->mrl);
            //syslog(LOG_INFO, "XineMediaplayer Plugin::Service Xine Test mrl %s: %s", static_cast<XineTestData *>(data)->mrl, static_cast<XineTestData *>(data)->result?"true":"false");
            return true;
        } else if (std::strcmp(id, "Xine Play m3u") == 0) {
            if(!data) return false;
            if (!start_xinemediaplayer_with_m3u(static_cast<Xinemediaplayer_Xine_Play_m3u *>(data)->m3u, NO_PLUGIN));
                printf("xine: m3u parse error OR no file/urls found in m3u\n");
            return true;
        } 
          else if (std::strcmp(id, "Xine Play mrl with name") == 0) {
            if(!data) return false;
            syslog(LOG_INFO, "XineMediaplayer Plugin::Service Xine Play mrl with name %s", static_cast<Xinemediaplayer_Xine_Play_mrl *>(data)->mrl);
            cXineOsd::nameListEntries = static_cast<Xinemediaplayer_Xine_Play_mrl_with_name *>(data)->namelistEntries;
            cXineOsd::isNew = true;
            if (static_cast<Xinemediaplayer_Xine_Play_mrl *>(data)->mrl)
                cXineOsd::mrl = strdup(static_cast<Xinemediaplayer_Xine_Play_mrl *>(data)->mrl);
            else
                cXineOsd::mrl =NULL;
            cXineOsd::isPlaylist = static_cast<Xinemediaplayer_Xine_Play_mrl_with_name *>(data)->playlist ;
            cXineOsd::browserInstance = static_cast<Xinemediaplayer_Xine_Play_mrl_with_name *>(data)->instance;
            cXineOsd::playListEntries = static_cast<Xinemediaplayer_Xine_Play_mrl_with_name*>(data)->playlistEntries;
#if 0
            Xinemediaplayer_Xine_Play_mrl xinePlayData = {
                static_cast<Xinemediaplayer_Xine_Play_mrl_with_name *>(data)->mrl,
                static_cast<Xinemediaplayer_Xine_Play_mrl *>(data)->instance,
                static_cast<Xinemediaplayer_Xine_Play_mrl_with_name *>(data)->playlist,
                static_cast<Xinemediaplayer_Xine_Play_mrl_with_name *>(data)->playlistEntries
            };
    
            return Service("Xine Play mrl", &xinePlayData);
#else
            // Donot launch new control if already playing,
            // just add to existing playlist
            if ( cXineOsd::isPlaying ) {
                cXineOsd::control->SetPlayList( cXineOsd::playListEntries );
		 if (cXineOsd::mrl) {
                    int idx = -1;
                    for (int i=0 ; i < cXineOsd::playListEntries.size(); ++i)
                        if (cXineOsd::playListEntries.at(i) == std::string(cXineOsd::mrl)) { idx = i; break; }
                    if (idx >= 0)
                        cXineOsd::control->PlayFileNumber(idx);
                }
                cXineOsd::firstOpen = false;
            } else {
            cXineOsd::control = new Control(static_cast<Xinemediaplayer_Xine_Play_mrl *>(data)->mrl,
                                            cXineOsd::isPlaylist, // always a playlist
                                            cXineOsd::playListEntries);
            ::cControl *control = cXineOsd::control;
            ::cControl::Launch(control);
            cXineOsd::firstOpen = true;
            }
            OPEN_XINEOSD_IF_NEEDED;
            return true;
#endif


        } else if (std::strcmp(id, "Xine Play mrl") == 0) {
            if(!data) return false;
            syslog(LOG_INFO, "XineMediaplayer Plugin::Service Xine Play mrl %s", static_cast<Xinemediaplayer_Xine_Play_mrl *>(data)->mrl);
            if (static_cast<Xinemediaplayer_Xine_Play_mrl *>(data)->mrl)
                cXineOsd::mrl = strdup(static_cast<Xinemediaplayer_Xine_Play_mrl *>(data)->mrl);
            else
                cXineOsd::mrl =NULL;
            cXineOsd::nameListEntries.clear();
            bool wasPlaylistEmpty = cXineOsd::playListEntries.empty();
            cXineOsd::isNew = true;
            cXineOsd::isPlaylist = static_cast<Xinemediaplayer_Xine_Play_mrl *>(data)->playlist ; 
            cXineOsd::browserInstance = static_cast<Xinemediaplayer_Xine_Play_mrl *>(data)->instance;
            cXineOsd::playListEntries = static_cast<Xinemediaplayer_Xine_Play_mrl *>(data)->playlistEntries;

            currentPlaylistFilename =  static_cast<Xinemediaplayer_Xine_Play_mrl *>(data)->playlistName;
            printf("\033[0;91mPlaylistname:'%s'\033[0m\n", currentPlaylistFilename.c_str());
            printf("\033[0;92mXine play mrl: list size %d, isPlaylist? %d\033[0m\n",
                   cXineOsd::playListEntries.size(),
                   cXineOsd::isPlaylist);
            if (cXineOsd::isPlaylist && cXineOsd::playListEntries.size() > 0)
            {
                savedPlaylist.playlist = cXineOsd::playListEntries;
                savedPlaylist.Save();
            } else if (cXineOsd::isPlaylist && cXineOsd::playListEntries.size()==0 && remember_playlist)
            {
                std::cout << " ++++++  savedPlaylist.size() := "
                          << savedPlaylist.playlist.size() << std::endl;
                cXineOsd::playListEntries = savedPlaylist.playlist;
            }

            // add mrl to playlist if playlist is empty
            //  happens when in filebrowser no playlist is created but just a "kOk" is press to play the file
            if (!cXineOsd::isPlaylist && cXineOsd::playListEntries.size() <= 0 && !isempty(cXineOsd::mrl)) {
                cXineOsd::playListEntries.push_back( cXineOsd::mrl );
                std::cout << "========== adding " << cXineOsd::mrl
                          << " --> " << std::hex
                          << cXineOsd::mrl << std::endl;
            }


            // create a vector of empty strings, these are filled in if possible by XineOsd
            if(cXineOsd::nameListEntries.size() == 0)
            {
                cXineOsd::nameListEntries.clear();
                for (unsigned int i=0; i<cXineOsd::playListEntries.size(); ++i)
                    cXineOsd::nameListEntries.push_back("");
            }
            //TODO REMOVE this
            /**** merge the playlists ****/
            std::vector<std::string>::iterator it1, it = static_cast<Xinemediaplayer_Xine_Play_mrl *>(data)->playlistEntries.begin();
            std::vector<std::string>::iterator END = static_cast<Xinemediaplayer_Xine_Play_mrl *>(data)->playlistEntries.end();
            printf("(%s:%d) MRL: '%s' %p isempty? %d\n", __FILE__,__LINE__, cXineOsd::mrl
                   , cXineOsd::mrl, isempty(cXineOsd::mrl));
            if (!isempty(cXineOsd::mrl))  {
                std::cout << "***add the mrl into cXineOsd::Playlist****" << std::endl;
                /***add the mrl into cXineOsd::Playlist****/
                std::string mrl_str = cXineOsd::mrl;
                std::string plain_mrl ; // without 'file://' like entries in playlist
                size_t found = mrl_str.find("file://");
                if ( found != std::string::npos)
                    plain_mrl = mrl_str.substr(found+7); // remove file:://
                //printf("(%s:%d) MRL: %s %s\n", __FILE__,__LINE__, mrl_str.c_str(), plain_mrl.c_str());
                it1 = cXineOsd::playListEntries.begin();
                while ( it1 != cXineOsd::playListEntries.end() ) {
                    if (*it1 == mrl_str || *it1 == plain_mrl)
                        break;
                    it1++;
                } // while
                // not found
                if (it1 ==  cXineOsd::playListEntries.end()) cXineOsd::playListEntries.push_back(mrl_str);
            } // if
            //bool isDuplicate; // no repetitions in playlist
            //playlist is handled by filebrowser, so donot add to current playlist
            /*
            for ( ; false && it != END ; ++it) {
                isDuplicate = false;
                it1 = cXineOsd::playListEntries.begin();
                while( it1 != cXineOsd::playListEntries.end() ) {
                    if ( *it == *it1 ) { // duplicater14357
                        isDuplicate = true; 
                        break;
                    } // if
                    it1++;
                } // while
                if (!isDuplicate) // then Add to playlist
                    cXineOsd::playListEntries.push_back(*it);
            } // for
            */
            cXineOsd::firstOpen = false;
            if ( cXineOsd::isPlaying ) { // Donot launch new control if already playing, just add to existing playlist
                cXineOsd::control->SetPlayList( cXineOsd::playListEntries );
            } else {
                cXineOsd::control = new Control(static_cast<Xinemediaplayer_Xine_Play_mrl *>(data)->mrl, 
                                                /*static_cast<Xinemediaplayer_Xine_Play_mrl *>(data)->playlist*/    1, // always a playlist
                                                /*static_cast<Xinemediaplayer_Xine_Play_mrl *>(data)->playlistEntries*/ 
                                                cXineOsd::playListEntries);
                ::cControl *control = cXineOsd::control;
                ::cControl::Launch(control);
                cXineOsd::firstOpen = true;
            } // if
            
            /* play the given mrl */
            if (cXineOsd::mrl) {
                int idx = -1;
                for (int i=0 ; i < cXineOsd::playListEntries.size(); ++i) {
                    printf("\033[0;92m%s\033[0m\n", cXineOsd::playListEntries.at(i).c_str());
                    if (cXineOsd::playListEntries.at(i) == std::string(cXineOsd::mrl)) 
                    { idx = i; break; }
                    
                }
                
                if (idx >= 0) {
                    cXineOsd::control->PlayFileNumber(idx);
                    cXineOsd::firstOpen = true; // allow for osd to be closed automatically
                    printf("==== lets play %d\n", idx);
                } else
                    esyslog("Could not find '%s' in playlist (size: %d)", cXineOsd::mrl, 
                            cXineOsd::playListEntries.size());
            } // if
            /* start playing the current playlist, if the prev. playlist was empty */
            else if (wasPlaylistEmpty && cXineOsd::playListEntries.size()>0) {
                cXineOsd::control->PlayFileNumber(0);
            }
            
            /*if new playlist then callback filebrowser plugin*/
            if (cXineOsd::k_blue != MUSIC_LIBRARY)
                cXineOsd::k_blue = FILEBROWSER_PLUGIN;

            cXineOsd::kPausePressed = 0;
            // donot call OSD if dvd:// is played and playlist size <= 1
            std::string mrlStr = cXineOsd::mrl;
            if ( mrlStr.find("dvd://") != std::string::npos && cXineOsd::playListEntries.size() <= 1) // playing DVD
                cXineOsd::k_blue = NO_PLUGIN;
            //Always call plugin - initial osd will be blocked later if playing video
            //          else
            OPEN_XINEOSD_IF_NEEDED;
            return true;
        } // if
        else if (std::string(id) == "Xine open Media Library") {

            //play the last played playlist

            if (savedPlaylist.playlist.size() > 0)
            {
                cXineOsd::browserInstance = -1; // new
                cXineOsd::k_blue = MUSIC_LIBRARY;

                // set playlist and namelist
                cXineOsd::nameListEntries.clear();
                cXineOsd::playListEntries.clear();
                
                if (remember_playlist) 
                {
                    std::vector<std::string> names(savedPlaylist.playlist.size(), "");
                    cXineOsd::nameListEntries = names;
                    
                    cXineOsd::playListEntries = savedPlaylist.playlist;
                }


                // if xine is running, give playlist to control
                // else start new control with playlist
                if(cXineOsd::isPlaying)
                {
                    cXineOsd::control->SetPlayList(cXineOsd::playListEntries);
                    cXineOsd::firstOpen = false;
                }
                else
                {
                    cXineOsd::control = new Control(
                                            cXineOsd::playListEntries.size()?cXineOsd::playListEntries.at(0).c_str():"",
                                1, // always a playlist
                                cXineOsd::playListEntries
                                );

                    ::cControl *control = cXineOsd::control;
                    ::cControl::Launch(control);
                    cXineOsd::firstOpen = true;
                }

#if 0
                Xinemediaplayer_Xine_Play_mrl data =
                {
                    savedPlaylist.playlist[0].c_str(),
                    cXineOsd::browserInstance, 
                    true, // a playlist
                    savedPlaylist.playlist
                };


                Service("Xine Play mrl", &data);
#endif

            } // ! empty playlist
            else
            {
                // no saved playlist. So, show message to user in Menu

                // make sure playlist is empty
                cXineOsd::playListEntries.clear();

                cXineOsd::browserInstance = -1; // new
                cXineOsd::k_blue = MUSIC_LIBRARY;
            }

            // if data was sent, the do not call MainMenuAction().
            // OSD is opened elsewhere.
            if (!data)
            {
                OPEN_XINEOSD_IF_NEEDED;
            }

            return true;

        } // if "xine open media library"
        else if (!strcmp("Xine getset playlist", id)) {
            Xinemediaplayer_Xine_GetSet_playlist *getSetPlaylist = 
                    static_cast<Xinemediaplayer_Xine_GetSet_playlist*>(data);
            
            if (!getSetPlaylist) {
                esyslog("(%s:%d) Got no 'data' with service call",
                        __FILE__, __LINE__);
                return false;
            }
            
            if (getSetPlaylist->set) {
                isyslog("replacing playlist, instance and playlistname");
                cXineOsd::browserInstance = getSetPlaylist->instance;
                cXineOsd::playListEntries = getSetPlaylist->playlistEntries;
                currentPlaylistFilename = getSetPlaylist->playlistName;
                
                if (cXineOsd::control) 
                    cXineOsd::control->SetPlayList(cXineOsd::playListEntries);
            } else {
                isyslog("loading playlist, instance and playlistname");
                getSetPlaylist->instance = cXineOsd::browserInstance;
                getSetPlaylist->playlistEntries = savedPlaylist.playlist;//cXineOsd::playListEntries;
                getSetPlaylist->playlistName = currentPlaylistFilename;
		
		printf("playlist size %d\n",  savedPlaylist.playlist.size());
		printf("getSetPlaylist->playlistEntries %d\n",  getSetPlaylist->playlistEntries.size());
		
            } // else
            return true;
        } // if "Xine getset playlist"
        else if (!strcmp("Xine handle keys", id)) // musicbrowser needs it
        {
            Xinemediaplayer_handle_keys *keys = static_cast<Xinemediaplayer_handle_keys *>(data);
            if (cXineOsd::control && keys)
            {
                keys->state = cXineOsd::control->ProcessKey(keys->Key);
            }
            return true;
        }
        return false;
    } // Plugin::Service

    cOsdObject* Plugin::MainMenuAction(){
        printf("(%s:%d) %s\n", __FILE__,__LINE__, __PRETTY_FUNCTION__);
        if(cXineOsd::control && cXineOsd::control->HasActiveVideo() && cXineOsd::firstOpen) { // Block initial osd if playing video
            cXineOsd::firstOpen = false;
            printf("... Main menu action xinemediaplayer returned null\n");
            return NULL;
        }
        else if (!cXineOsd::control && savedPlaylist.playlist.size())
        {
            // !control :=> xinemediaplayer is not playing and
            // non-empty savedplaylist available, so lets start xine with that

            cOsdMenu* menu=NULL; // means, do not use CallPlugin() in the service to open menu
            cPluginManager::CallAllServices("Xine open Media Library", &menu);

            //return NULL;
        }
        else if (!cXineOsd::control && !savedPlaylist.playlist.size())
        {
            cXineOsd::k_blue = MUSIC_LIBRARY;
        }

        return new cXineOsd;
    } // Plugin::MainMenuAction

    bool Plugin::Start() {
#ifdef RBLITE
        cSetupLine* lLine = Setup.Get("UseHdExt","reelbox");
        if(lLine && lLine->Value() && atoi(lLine->Value()))
            useOld = 0;
#else
        useOld = 0;
#endif
        return true;
    } // Plugin::Start

    bool Plugin::checkExtension(const char *fFile) {
        if(useOld) return false;
        bool lRet = true;
        if(!fFile)
            return false;
        if (!strcmp(mExtensions.c_str(), "*"))
            return true;
        const char *lExt = strrchr(fFile, '.');
        if(!lExt)
            return false;
        lExt++;
        if(!*lExt)
            return false;
        if (!strncmp(mExtensions.c_str(), "! ", 2))
            lRet = false;
        int lLen = strlen(lExt);
        const char *lPos = mExtensions.c_str();
        const char *lNext = strchr(lPos, ' ');
        while (lPos) {
            if (lNext) {
                if ((lLen == (lNext - lPos)) && !strncasecmp(lExt, lPos, lNext - lPos))
                    return lRet;
                lPos = ++lNext;
                lNext = strchr(lPos, ' ');
            } else {
                if (!strcasecmp(lExt, lPos))
                    return lRet;
                lPos = lNext;
            } // if
        } // while
        return !lRet;
    } // Plugin::checkExtension
    
    std::string currentPlaylistFilename;
} // namespace XineMediaplayer
} // namespace Reel

VDRPLUGINCREATOR(Reel::XineMediaplayer::Plugin); // Don't touch this!
