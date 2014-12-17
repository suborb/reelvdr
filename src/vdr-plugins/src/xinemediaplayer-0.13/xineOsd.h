
#ifndef __XINE_OSD_H__
#define __XINE_OSD_H__

#include <vdr/osdbase.h>
#include <vdr/status.h>
#include <vdr/debugmacros.h>
#include <string>
#include <vector>
#include <time.h>
#include "Control.h"


class cFileCache;
class cTempFileCache;
class cThumbConvert;

namespace Reel{

    #include "../filebrowser/servicestructs.h"

    namespace XineMediaplayer{

        using namespace std;

        class Control; // forward declaration
        class cXineStatus;
                
        enum { NO_PLUGIN=0, FILEBROWSER_PLUGIN, MUSIC_LIBRARY /* calls filebrowser*/, SHOUTCAST_PLUGIN, IPOD_PLUGIN,
               MMFILES_PLUGIN, YOUTUBE_PLUGIN, MUSIC_BROWSER};
        void            CallFilebrowser(xineControl);    //call filebrowser plugin

        class cXineOsd : public cOsdMenu
        {
            private:
                int FunctionMode;                       // For more options, Normal/Shuffle/Repeat

                int currentlySelected;          // position of the selected mediafile in the playlist
                int currentlyPlaying;           // position of the mediafile in the playlist, that is being played
                int index;
                int total;

                bool first;

                cXineStatus     *xineStatus;
                void ShowPlayMode(bool force = false);
                
                //for displaying cover
                int previouslySelectedCover; 
                int previouslySelectedInfo;  
                cTempFileCache *cache;
                cFileCache *ID3cache;
                cThumbConvert *thumbConvert;    
                std::string cover;
                //std::string lastCover;
                int coverSize;
                static bool isShowingCover;

                bool IsImage(std::string name);
                std::string GetCoverFromDir(std::string mrl); 

                void GetImageInfoFromID3(std::string path, std::string &image, int &size);
                void ExtractImageFromID3(std::string path);
 
                void ConvertCover();
                eOSState ShowCover(bool hide); 
                eOSState ShowInfo(bool hide);
                
                std::string PlaybackMode() const;
                std::string PlaybackModeToTitle() const;

                static const char* const I3DCACHE;
                //for displaying cover

                eOSState AddToFavorites();
                void showID3TagInfo(std::string title_, std::string  artist,
                                    std::string album, std::string comment,
                                    std::string genre, int year, int track);
            public:

                // kPause was pressed
                static int kPausePressed;

                static char     *mrl;
                static bool     isPlaylist; 
                static int      browserInstance;
                static std::vector<std::string>   playListEntries; // the playlist
                static std::vector<std::string>   nameListEntries; // the display names of each playlist entry
                static bool DeletePlaylistItem(int pos) {

                    if (pos >= 0 && pos < playListEntries.size()) {

                        if (playListEntries.size() == nameListEntries.size())
                            nameListEntries.erase(nameListEntries.begin()+pos);
                        
                        playListEntries.erase(playListEntries.begin()+pos);
                        
                        if (control) {
                            
                            int curr = -1, idx = 0, total=0;
                            control->PlayerPositionInfo(curr, idx, total);
                            
                            control->SetPlayList(playListEntries);
                            
                            // if deleteing currently playing item
                            // then play the next item
                            if (pos == curr) {
                                control->PlayFileNumber(pos);
                            } // if
                            
                        } // if

                        return true;
                    } // if

                    return false;

                } // DeletePlaylistItem()

                static bool ClearPlaylist() {
                    playListEntries.clear();
                    nameListEntries.clear();

                    control->SetPlayList(playListEntries);

                    return true;
                } // ClearPlaylist()

                static Control      *control;    // pointer to the current Control
                static bool     isPlaying;   // if Not playing donot update OSD
                static bool     isOpen;      // Donot show Skins.Message() if OSD is open when playing-next file
                static bool     firstOpen;
                static bool     closeOsd;
                static bool     isNew;       // needs refresh of display?
                static int      k_blue;      // which plugin to call: filebrowser/shoutcast ?
                static std::string      openError;

                static void         ClearStaticVariables(); // clears the above variables, eg. when nomore xinemediaplayer is running

                cXineOsd();
                ~cXineOsd();

                void        ShowEntries();  // display the whole playlist
                void        UpdateEntries();    //updates  progressBar 
                static bool CallExt_Plugin();    //call External plugin: filebrowser/shoutcast
                void        SetHelpButtons();    //set help  "filebrowser"/"shoutcast"
                eOSState    ShowThumbnail(std::string item, bool hide, bool currentChanched);
                eOSState    ProcessKey(eKeys Key);
                                
                // Only filebrowser can have different modes
                bool            CanHaveDifferentModes();

                //  void        MsgSetVolume(int, bool); //cStatus

                 // override cOsdMenu::Display() so that ID3 tags can be shown in SideNote
                void Display();
                void ShowSideNoteInfo();
                void ShowReplayModeInSideNote();

                static time_t lastVolShownTime;
        };


        class cXineStatus: public cStatus
        {
            protected:
                void SetVolume(int vol, bool abs);
        };


    } //namespace xinemediaplayer
} //namespace Reel

#endif

