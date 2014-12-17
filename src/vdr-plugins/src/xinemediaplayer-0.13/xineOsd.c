#include <sstream>

#include "xineOsd.h"
#include "Control.h"
#include "Player.h"
#include "timecounter.h"
#include "xinemediaplayer.h"

#include <vdr/remote.h>
#include <vdr/plugin.h>
#include <vdr/tools.h>
#include <vdr/interface.h>


//id3tags : using TagLib
#include <fileref.h>
#include <tag.h>
#include <id3v2tag.h>
#include <id3v2frame.h>
#include <attachedpictureframe.h>
//#include <taglib_export.h>
#include "tfile.h"
#include "mpegproperties.h"
//#include "taglib_export.h"
#include <mpegfile.h>

#include "../filebrowser/convert.h"
#include "../filebrowser/filecache.h"
#include "../filebrowser/filetools.h"
#include "../filebrowser/stringtools.h"
#include "../filebrowser/def.h"

#include "xineOsdInfoMenu.h"
#include "xineOsdFunction.h"

#define PROGRESSBARLEN 30

namespace Reel
{
    namespace XineMediaplayer
    {

        struct ID3Tag {
            std::string name;
            std::string value;
        };

        std::vector<struct ID3Tag> ID3Tags;

        cTimeMs osdTimeOut; // refresh Osd after x secs

        ////////////////////////////////////////////////////////////////////////////////
        //////// Static variables
        ////////////////////////////////////////////////////////////////////////////////
        char            *cXineOsd::mrl       = NULL  ;
        bool            cXineOsd::isPlaylist     = false ;
        int             cXineOsd::browserInstance     = -1 ;
        bool            cXineOsd::isNew      = true  ;
        bool            cXineOsd::firstOpen      = false ;
        bool            cXineOsd::closeOsd      = false ;

        Control         *cXineOsd::control   = NULL  ;
        bool            cXineOsd::isPlaying  = false ;
        bool            cXineOsd::isOpen     = false ;
        time_t          cXineOsd::lastVolShownTime = 0   ;
        int             cXineOsd::k_blue         = FILEBROWSER_PLUGIN;
        int             cXineOsd::kPausePressed  = 0;
        std::string     cXineOsd::openError      = "";
        bool            cXineOsd::isShowingCover = false;

        std::vector<std::string>        cXineOsd::playListEntries    ; // the playlist
        std::vector<std::string>        cXineOsd::nameListEntries    ; // the display names of each playlist entry

        ////////////////////////////////////////////////////////////////////////////////
        /////// Utility functions
        ////////////////////////////////////////////////////////////////////////////////

        inline void trim_right(std::string &source, const std::string &t) {
          source.erase(source.find_last_not_of(t)+1);
        } // trim_right
        inline void trim_left(std::string &source, const std::string &t) {
          source.erase(0, source.find_first_not_of(t));
        } // trim_left
        inline void trim(std::string &source, const std::string &t) {
          trim_left(source, t);
          trim_right(source, t);
        } // trim

        void cXineOsd::showID3TagInfo(std::string title_, std::string  artist, std::string album, std::string comment, std::string genre,
        int year, int track)
        {

            //printf("showID3TagInfo: title = %s\n", title.c_str());
            //printf("showID3TagInfo: artist = %s\n", title.c_str());
            //printf("showID3TagInfo: album = %s\n", title.c_str());
            cPlugin *skin = cPluginManager::GetPlugin("skinreel3");
            if (skin) {
                struct ID3Tag tag;
                if (!artist.empty()) {
                    tag.name = "Artist:";
                    tag.value = artist;
                    ID3Tags.push_back(tag);
                }
                if (!title_.empty()) {
                    tag.name = "Title:";
                    tag.value = title_;
                    ID3Tags.push_back(tag);
                }
                if (!album.empty()) {
                    tag.name = "Album:";
                    tag.value = album;
                    ID3Tags.push_back(tag);
                }
#if 0
                if (!comment.empty()) {
                    tag.name = "Comment";
                    tag.value = comment;
                    ID3Tags.push_back(tag);
                }
#endif
                if (!genre.empty()) {
                    tag.name = "Genre:";
                    tag.value = genre;
                    ID3Tags.push_back(tag);
                }
                if ( year != 0 ) {
                    tag.name = "Year:";
                    std::stringstream s;
                    s << year;
                    tag.value = s.str();
                    ID3Tags.push_back(tag);
                }
#if 0
                if ( track != 0 ) {
                    tag.name = "Tracknumber:";
                    std::stringstream s;
                    s << track;
                    tag.value = s.str();
                    ID3Tags.push_back(tag);
                }
#endif
#if APIVERSNUM < 10700
                skin->Service("setId3Infos", &ID3Tags);
                ID3Tags.clear();
#else
                cStringList id3tags;
                //cString tag;
                id3tags.Append(strdup("ID3Tags"));
                for (int i=0; i < ID3Tags.size(); ++i) {
                    //tag = ID3Tags.at(i).value;
                    id3tags.Append(strdup(ID3Tags.at(i).value.c_str()));
                }
                if (ID3Tags.size())
                    SideNote(&id3tags);
                //printf("Sent %d id3tags\n", id3tags.Size());
                ID3Tags.clear();
#endif
            }
        }


#if VDRVERSNUM < 10704
        const int FramesInOneHour = 3600*FRAMESPERSEC;
        const int FramesInOneDay  = 86400*FRAMESPERSEC;
#else
        const int FramesInOneHour = 3600*DEFAULTFRAMESPERSECOND;
        const int FramesInOneDay  = 86400*DEFAULTFRAMESPERSECOND;
#endif
        float ComputePercentage(int index, int total)
        {
            if (index < 0) return 0.0;
            else if (total > 0) return index*100.0/total;

            // index < 60 mins,          assume total = 60 mins
            else if (index < FramesInOneHour) return index*100.0/FramesInOneHour;

            // 60 mins <=index  < 1 day, assume total = 1 day
            else if (index < FramesInOneDay) return index*100.0/FramesInOneDay;

            // index more than 1 day,    assume percent = 80% ?
            else return 80.0;
        }

        std::string MakeProgressBarStr(float percent, int length)
        {
            // makes [|||||    ] for given percent and length of string

            if (length <=0 )
            {
                esyslog("[xinemediaplayer] (%s:%i) MakeProgressBarStr(percent=%f, length=%i)",
                __FILE__, __LINE__, percent, length);
                return "";
            }

            if (percent >100) percent = 100;
            else if (percent < 0) percent = 0;

            int p = (int)(percent/100*length);
            std::string str = "[";
            str.insert( str.size(), length, ' '); // fill with ' '

            if (p > 0)
                str.replace( 1, p, p, '|'); // replace with appropriate '|'

            str.append("]");

            return str;
        }

        /*
           Hour(if > 0) : Min : Sec format from Index
           */
        std::string HMS(int Index, bool WithHour = false)
        {
            char buffer[16];
#if VDRVERSNUM < 10704
            int s = (Index / FRAMESPERSEC);
#else
            int s = (Index / DEFAULTFRAMESPERSECOND);
#endif
            int m = s / 60 % 60;
            int h = s / 3600;
            s %= 60;

            if ( h > 0 || WithHour ) // show Hour ?
                snprintf(buffer, sizeof(buffer), "%d:%02d:%02d" , h, m, s);
            else
                snprintf(buffer, sizeof(buffer), "%02d:%02d" ,  m, s);
            return std::string(buffer);
        }
        ////////////////////////////////////////////////////////////////////////////////
        // ID3 info : wrapper functions for taglib
        ////////////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////////////
        bool Id3Info(std::string songPath, std::string& title, std::string& artist, std::string& album, std::string& comment, std::string& genre, int *year, int *track)
        {

            TagLib::FileRef f( songPath.c_str() );
            if(f.isNull() || !f.tag())
            {
                return false;
            }

            TagLib::Tag *tag = f.tag();
            title   = tag->title().stripWhiteSpace().toCString(false)   ; // unicode = false
            artist  = tag->artist().stripWhiteSpace().toCString(false)  ;
            album   = tag->album().stripWhiteSpace().toCString(false)   ;
            comment = tag->comment().stripWhiteSpace().toCString(false) ;
            genre   = tag->genre().stripWhiteSpace().toCString(false)   ;
            if (year)
                *year   = tag->year();
            if (track)
                *track  = tag->track();

            return true;
        }


        ////////////////////////////////////////////////////////////////////////////////
        // class cXineOsd
        ////////////////////////////////////////////////////////////////////////////////

        const char* const cXineOsd::I3DCACHE = "/tmp/I3Dcache";

#if APIVERSNUM < 10700
        cXineOsd::cXineOsd() : cOsdMenu(tr("Mediaplayer"),  7, 15)
#else
        cXineOsd::cXineOsd() : cOsdMenu(tr("Mediaplayer"),  8, 15)
#endif
        {
            LOC; printf("mrl: %s\n", mrl?mrl:"(null)" );
                        //DEBUG_VAR(mrl);
            //DEBUG_VAR( isPlaylist );
            //DEBUG_VAR( playListEntries.size() );

            EnableSideNote(true);
            first = true;

            //for displaying cover
            cache = new cTempFileCache;
            if(!cache || !(cache->Open(TMP_CACHE, TMP_CACHE_SIZE) && cache->Open(BROWSER_CACHE, MAX_CACHE_SIZE)))
            {
                DELETENULL(cache);
            }
            ID3cache = new cFileCache;
            if(!ID3cache || !ID3cache->Open(I3DCACHE, MAX_CACHE_SIZE))
            {
                DELETENULL(ID3cache);
            }
            thumbConvert = new cThumbConvert(cache);
            thumbConvert->SetDimensions(125, 125);
            thumbConvert->SetEnding(".thumb2.png");
            cover = "";
            coverSize = 0;
            isShowingCover = false;
            previouslySelectedCover = 0;
            previouslySelectedInfo = 0;

            //for displaying cover

            lastVolShownTime = 0;

            //if empty playlist then try adding mrl if possible:
            if ( playListEntries.size()<= 0 && !isempty(mrl))
            {
                playListEntries.push_back(mrl);
                cXineOsd::isNew = 1;
            }

            if (0 && cXineOsd::isNew == 1 )
            {
                if ( playListEntries.size() > 0 )
                    control->SetPlayList(playListEntries);
                else
                {
                    ; //printf("%s:%d %s ERROR: playlist is NEW and empty : size = %i\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, playListEntries.size() );
                }
            }

            //currentlySelected = -1; // none-selected
            currentlyPlaying = -1; // none playing
            index = 0;
            total = 0;

            if (control)
            control->PlayerPositionInfo(currentlyPlaying, index, total);
            currentlySelected = currentlyPlaying;

            ConvertCover();

            xineStatus = new cXineStatus; // for volume

            FunctionMode = 0;

            ShowEntries();
            cXineOsd::isOpen         = true ;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // ~cXineOsd
        cXineOsd::~cXineOsd()
        {
            //playListEntries.clear();
            delete xineStatus;
            cXineOsd::isOpen         = false ;

            cPlugin *skin = cPluginManager::GetPlugin("skinreel3");
            if (skin) {
                ID3Tags.clear();
                skin->Service("setId3Infos", NULL);
            }

            // Clear cover-art
            ShowCover(true);

            //for displaying cover
            DELETENULL(cache);
            ID3cache->ClearCache();
            DELETENULL(ID3cache);
            DELETENULL(thumbConvert);
            //for displaying cover

            savedPlaylist.Save();
        }

        void cXineOsd::ShowSideNoteInfo()
        {
#if APIVERSNUM >= 10700
            if(currentlySelected >= 0 && currentlySelected < (int)playListEntries.size())
            {
                std::string title_, album, artist, genre, comment;
                int year, track;
                int success = Id3Info(playListEntries[currentlySelected], title_,
                                      artist, album, comment,
                                      genre, &year, &track);

                //internet radio
                if(!success && StartsWith(playListEntries[currentlySelected], "http://"))
                {
                    //get title from player
                    std::string headerline = control->GetDisplayHeaderLine();
                    title_ = GetBeforeFirst(GetAfterLast(headerline, "]"), "-");
                    artist = GetAfterLast(GetAfterLast(headerline, "]"), "-");
                    trim(title_, " ");
                    trim(artist, " ");

                    album = comment = genre = "";
                    year = track = 0;
                    if (!title_.empty() )
                    {
                        //setTitle_str += ": " + title_;
                        printf("(%s:%d) Calling showID3TagInfo()\n", __FILE__, __LINE__);
                        //showID3TagInfo(title_, artist, "", "", "", 0, 0);
                        success = true;
                    }
                } // if

                if (success)
                showID3TagInfo(title_, artist, album, comment, genre, year, track);
            } // if

            // display replay mode such as 'Replay Mode: Shuffle' in side note area
            ShowReplayModeInSideNote();
#endif /* APIVERSNUM */
        }

        void cXineOsd::Display()
        {
#if 1 // fix title string
            std::string tmpTitle = Title();
            std::string prefix = "SHOWINGMP3COVER$";

            // Check for "SHOWINGMP3COVER$" prefix to title
            // must be appended only when showing cover
            if (isShowingCover && tmpTitle.find(prefix) == std::string::npos) {
                // no prefix found, add it
                tmpTitle = prefix + tmpTitle;
            } else if (!isShowingCover && tmpTitle.find(prefix) != std::string::npos) {
                // not showing cover but prefix found! remove it
                tmpTitle = tmpTitle.substr(prefix.size());
            }
            SetTitle(tmpTitle.c_str());

#endif // end hack

            cOsdMenu::Display();
            ShowSideNoteInfo();
        }

        ////////////////////////////////////////////////////////////////////////////////
        void cXineOsd::ClearStaticVariables()
        {
            kPausePressed   = 0    ;
            mrl         = NULL ;
            isPlaylist  = false;
            playListEntries.clear();
            nameListEntries.clear();
            control     = NULL ;
            isPlaying   = false;
            isOpen      = false;
            isNew           = true ;
            k_blue          = NO_PLUGIN;
            openError       = "";
        }

        ////////////////////////////////////////////////////////////////////////////////
        void cXineOsd::UpdateEntries() // update OSD
        {
            if( !isPlaying ) // close OSD ?
            {
                cRemote::Put(kBack);
                return;
            }

            if( !openError.length() && cXineOsd::firstOpen
                && cXineOsd::control->HasActiveVideo())
            {
                cXineOsd::closeOsd = true;
                cXineOsd::firstOpen = false;
                return;
            }

            /* no playlist items, so no update needed*/
            if(!cXineOsd::playListEntries.size())
                return;

            int tmpSelected = Current(); // prevent select-'highlight' from disappering
            if (currentlyPlaying >= 0 && tmpSelected > currentlyPlaying) tmpSelected -= 2;

            int prevCurrent=currentlyPlaying;
            int index_tmp = index, total_tmp = total;

//          printf("Waiting for player position %i\n", currentlyPlaying);
            /**** get the position in the playlist ****/
            control->PlayerPositionInfo(currentlyPlaying, index, total);
//          printf("got player position %i\n", currentlyPlaying);

            ConvertCover();

            if (currentlyPlaying < 0 || (index_tmp == index && currentlyPlaying == prevCurrent  && total == total_tmp) ) return ;

            char playedTime[64];
            sprintf(playedTime, "%10s",HMS(index).c_str() ); // right indented towards progress bar
            std::string str;
            str += playedTime;
            //str +=  std::string(" \t") + MakeProgressBarStr( total > 0 ? index*100.0/total:50 , PROGRESSBARLEN);
            str +=  std::string(" \t") + MakeProgressBarStr( ComputePercentage(index,total) , PROGRESSBARLEN);

            // if stream/ if total <=0 donot show total, just the elapsed time should be sufficient
            if (total > 0)
                str += std::string("\t") + HMS(total);

            if (prevCurrent >=0 && prevCurrent < Count())
            {
                Del( prevCurrent + 1); // remove old progress bar
                Del( prevCurrent + 1); // remove blank line
            }
            Add( new cOsdItem(  str.c_str(), osUnknown, false )  ,false, Get(currentlyPlaying)); //add new progress bar after currentPlaying
            Add( new cOsdItem("", osUnknown, false), false, Get(currentlyPlaying+1) ) ; // add new blank line


            /** donot loose the currently selected item by the user when switching to a new 'song'**/
            if ( tmpSelected == prevCurrent && currentlyPlaying > prevCurrent )
                SetCurrent( Get(currentlyPlaying) ); // to prevent highlight from disappering
            else if (tmpSelected > currentlyPlaying)
                SetCurrent(Get(tmpSelected+2));
            else
                SetCurrent(Get(tmpSelected));

            if (prevCurrent != currentlyPlaying ) // update title
            {
                std::string title, album, artist, genre, comment;
                int year, track;
                std::string setTitle_str;
                if (isShowingCover)
                    setTitle_str = "SHOWINGMP3COVER$" + string(tr("Mediaplayer"))  + PlaybackModeToTitle();
                else
                    setTitle_str = tr("Mediaplayer")  + PlaybackModeToTitle();
                if(currentlyPlaying >= 0 && currentlyPlaying < (int)playListEntries.size())
                {
                    int success = Id3Info(playListEntries[currentlyPlaying], title, artist, album, comment, genre, &year, &track);

                    // not neeeded
                    /*//internet radio
                    if(! success && StartsWith(playListEntries[currentlySelected], "http://"))
                    {
                        //get title from player
                        std::string headerline = control->GetDisplayHeaderLine();
                        title = GetBeforeFirst(GetAfterLast(headerline, "]"), "-");
                        artist = GetAfterLast(GetAfterLast(headerline, "]"), "-");
                        trim(title, " ");
                        trim(artist, " ");
                        showID3TagInfo(title, artist, "", "", "", 0, 0);
                    }
                    else
                    {
                        showID3TagInfo(title, artist, album, comment, genre, year, track);
                        Display();
                    }*/
                }

                if (!title.empty() ) setTitle_str += ": " + title;
                SetTitle(setTitle_str.c_str() );
            }
            Display();

            //some times control never reaches here,
            //so call ShowPlayMode() after calling this function

                        //ShowPlayMode();
        }


        ////////////////////////////////////////////////////////////////////////////////
        void cXineOsd::ShowEntries()
        {
            if (cXineOsd::closeOsd) return;
            if( !isPlaying && cXineOsd::control) // close OSD ?
            {
                cRemote::Put(kBack);
                printf("return %s:%d\n", __PRETTY_FUNCTION__, __LINE__);
                return;
            }

            Clear(); // clear osd

            /**** get the position in the playlist ****/
            if (control && cXineOsd::playListEntries.size()) {
                currentlyPlaying = -1;
            control->PlayerPositionInfo(currentlyPlaying, index, total);
            } else {
                AddFloatingText(tr("Media library playlist is empty.\nPlease press RED key for selecting files."), 50);

                // clear title
                SetTitle(tr("Mediaplayer"));
                SetHelpButtons();
                Display();
                printf("return %s:%d\n", __PRETTY_FUNCTION__, __LINE__);
                return;
            }


            std::vector<std::string>::iterator it = playListEntries.begin();
            size_t found;
            int i=0;

            // add playlist entries to screen
            for( ; it!=playListEntries.end(); ++i, ++it)
            {
                found = it->find_last_of('/');
                std::string str = it->substr(found+1); // TODO checks on found

                // remove '_' from file names
                unsigned int j = 0;
                while( j < str.size() )
                {
                    if (str[j] == '_') str[j] = ' ';
                    j++;
                }

                std::string Name;
                if (nameListEntries.size() ==  playListEntries.size() && !nameListEntries[i].empty()  )
                {
                    Name = nameListEntries[i];
                }
                else
                {
                  std::string title, artist, album, dummy;

                    if ( title.empty() )
                    {
                        // try id3 tag
                        Id3Info(it->c_str(), title, artist, album, dummy, dummy, NULL, NULL );
                        //printf("Got from Id3 info %s\n", title.c_str() );
                    }

                    if (!title.empty() )
                    {
                        Name  = title.substr(0, 50) ;

                        //if (!album.empty() )  Name += "-" + album;
                        if (!artist.empty() )
                            Name = title.substr(0, 25) + " - "
                                    + artist.substr(0,25);
                    }

                    if (Name.empty()) // show filename
                    {
                        //TODO : remove .mp3/.wav extn
                        Name = str;
                    }
                    Name = Name.substr(0, 50);

                    // update the title in the list, if there are 'blanks' they are filled up
                    if ( !Name.empty()  && nameListEntries.size() ==  playListEntries.size() )
                          nameListEntries[i] = Name.substr(0,100);
                }
                // add number
                char num[8]; snprintf(num, 7, "%3d.  ",i+1);
                Name = num + Name.substr(0,100);
                // display 'Song'/file  name
                Add( new cOsdItem( Name.c_str() , osUnknown, true ) );

                // currently playing
                if (currentlyPlaying == i) // currentlyplaying counts from 0...playlist.size()-1
                {
                    char playedTime[64];
                    sprintf(playedTime, "%10s",HMS(index).c_str() );
                    std::string str;
                    str += playedTime;
                    str += std::string(" \t") + MakeProgressBarStr(ComputePercentage(index,total), PROGRESSBARLEN);
                    str += std::string("\t") + HMS(total);

                    Add( new cOsdItem(  str.c_str() , osUnknown, false ) );
                    Add( new cOsdItem(  "" , osUnknown, false ) );
                }

            }

            if(currentlySelected < 0)
                currentlySelected = currentlyPlaying;

            // donot select the progressBar and the empty line that follow the currently playing song
            if (currentlyPlaying >=0 && currentlyPlaying < currentlySelected )
                SetCurrent( Get( currentlySelected + 2) );
            else
                SetCurrent( Get( currentlySelected ) );

            // update title

            std::string title, dummy;
            std::string setTitle_str;

            if (isShowingCover)
                setTitle_str = "SHOWINGMP3COVER$" + string(tr("Mediaplayer")) + PlaybackModeToTitle();
            else
                setTitle_str = tr("Mediaplayer") + PlaybackModeToTitle();
            //printf("\033[1;91m currentlyPlaying: %i\033[0m\n", currentlyPlaying);
            if (currentlyPlaying >= 0 && currentlyPlaying < (int)playListEntries.size())
                Id3Info( playListEntries[currentlyPlaying],
                         title, dummy, dummy, dummy, dummy, NULL, NULL);

            if (!title.empty() )
                setTitle_str += ": " + title;
            SetTitle(setTitle_str.c_str() );

            SetHelpButtons();
            ShowPlayMode(true);
            Display();
            cXineOsd::isNew = false;

            ConvertCover();
        }


        void cXineOsd::ShowReplayModeInSideNote()
        {
#if 0 && APIVERSNUM >= 10700
            if ( !CanHaveDifferentModes() || !cXineOsd::control) return;

            cStringList replayModes;
            replayModes.Append(strdup("STARTFROM 280"));
            std::string mode_str;

            int replay_mode = cXineOsd::control->GetPlayMode() ;
            switch( replay_mode )
            {
              case PLAY_NORMAL:
                //SetStatus(tr("Replaymode: Normal"));
                mode_str = tr("Replaymode: Normal");
                break;

              //case PLAY_REPEAT_SINGLE:
              case PLAY_REPEAT_PLAYLIST:
                //SetStatus(tr( "Replaymode: Repeat") );
                mode_str = tr( "Replaymode: Repeat") ;
                break;

              case PLAY_RANDOM:
                //SetStatus( tr("Replaymode: Shuffle") );
                mode_str = tr("Replaymode: Shuffle") ;
                break;

              default:
                //SetStatus(NULL);
                mode_str = "";
                esyslog("%s:%d unknown ReplayMode (%d) got from xine's control",
                        __FILE__,
                        __LINE__,
                        replay_mode);
                break;
            } // switch

            replayModes.Append(strdup(mode_str.c_str()));
            SideNote(&replayModes);
#endif /* APIVERSNUM */
        } // ShowReplayModeInSideNote()

                ////////////////////////////////////////////////////////////////////////////////
                void cXineOsd::ShowPlayMode(bool force)
                {
                  static int _mode = -1 ;
                  if (kPausePressed)
                  {
                      //when un-paused should replace the status message
                      _mode = -1;

                      //check if player is really paused
                      if (cXineOsd::control->IsPaused())
                      {
                          SetStatus(tr("Paused"));
                          return ;
                      }
                      else
                      {
                          // xine can be un-paused by atleast by pressing
                          // kPause and kPlay and choosing to play another file

                          // reset kPausePressed if xine is playing
                          kPausePressed = 0;
                          SetStatus(NULL);
                      }
                 }
                 else
                /*clear status if Paused had been displayed,
                 * else it is not cleared when playing internet radio.
                 * Also, don't clear status if volume bar is being shown */
                 if (_mode == -1 && !lastVolShownTime) {
                   SetStatus(NULL);
                 }
                 //Muted
                 if (cDevice::PrimaryDevice()->IsMute())
                 {
                   SetStatus(tr("Mute"));
                   return;
                 }

                 if ( !CanHaveDifferentModes() ) return;

#if APIVERSNUM < 10700
                  int mode_tmp = cXineOsd::control->GetPlayMode() ;

                  // Mode not changed
                  if ( !force && _mode == mode_tmp) return;
                  else
                  _mode = mode_tmp;

                  switch( _mode )
                  {
                    case PLAY_NORMAL:
                      SetStatus(tr("Replaymode: Normal"));
                      break;

                    //case PLAY_REPEAT_SINGLE:
                    case PLAY_REPEAT_PLAYLIST:
                      SetStatus(tr( "Replaymode: Repeat") );
                      break;

                    case PLAY_RANDOM:
                      SetStatus( tr("Replaymode: Shuffle") );
                      break;

                    default:
                      SetStatus(NULL);
                  }

#endif /* APIVERSNUM */
                  if(openError.length()) {
                     Skins.QueueMessage(mtError, openError.c_str(), Setup.OSDMessageTime*2); // Show little longer...
                     openError.clear();
                  }
                }

                ////////////////////////////////////////////////////////////////////////////////
                void cXineOsd::SetHelpButtons()
                {
                  if ( FunctionMode == 1 )
                  {
                    if ( playListEntries.size() > 2)
                        SetHelp( NULL, tr("Shuffle") ,tr("Repeat"),tr("Normal") );
                    else
                        SetHelp(NULL, NULL, tr("Repeat"),tr("Normal") );

                    return;
                  }
                  static eKeys infokeys[] = {kInfo, kNone};
                    switch(cXineOsd::k_blue)
                    {
                        case MUSIC_LIBRARY:
                        case FILEBROWSER_PLUGIN:
                            //SetHelp( NULL, "<<", ">>", tr("overview") );
                            //SetHelp( tr("Overview"), NULL, tr("Info"), tr("Options") );
                        printf("Functions keys followed by info key\n");
                        SetHelp( tr("Filelist"), NULL, NULL, tr("Functions"), infokeys);
                            break;

                        case SHOUTCAST_PLUGIN:
                          //SetHelp( tr("shoutcast"), NULL, NULL, NULL );
                          SetHelp( tr("shoutcast"), NULL, NULL, tr("Add to favs"));
                            break;

                        default:
                            SetHelp( NULL, NULL, NULL, NULL );
                            break;
                    }

                }
                ////////////////////////////////////////////////////////////////////////////////
                bool cXineOsd::CanHaveDifferentModes()
                {
                  switch(cXineOsd::k_blue)
                  {
                    case MUSIC_LIBRARY:
                    case FILEBROWSER_PLUGIN:
                      return true;
                      break;
                    case SHOUTCAST_PLUGIN:
                      return false;
                      break;
                    case YOUTUBE_PLUGIN:
                      return false;
                      break;

                    default:
                      esyslog("(%s:%d) cXineOsd::k_blue not set to a known value", __FILE__, __LINE__);
                      break;
                  }
                  return false;
                }
                ////////////////////////////////////////////////////////////////////////////////
                bool cXineOsd::CallExt_Plugin()
                {
                    bool result = true;
                    switch(cXineOsd::k_blue)
                    {
                        case MUSIC_LIBRARY:
                        case FILEBROWSER_PLUGIN:
                            CallFilebrowser(xine_browse);
                            break;
                        case SHOUTCAST_PLUGIN:
                            cRemote::CallPlugin("shoutcast");
                            break;
                        case YOUTUBE_PLUGIN:
                            cRemote::CallPlugin("youtube");
                            break;

                        case MMFILES_PLUGIN:
                            cRemote::CallPlugin("mmfiles");
                            break;
                        case MUSIC_BROWSER:
                            cRemote::CallPlugin("musicbrowser");
                            break;
                        default:
                            result = false;
                            esyslog("(%s:%d) cXineOsd::k_blue not set to a known value", __FILE__, __LINE__);
                            break;
                    }

                    return result;
                }

                ////////////////////////////////////////////////////////////////////////////////
                void CallFilebrowser(xineControl xineCtrl)
                {
                    printf("Xine: CallFilebrowser, browserInstance = %d k_blue %d\n",
                           cXineOsd::browserInstance,
                           cXineOsd::k_blue);

                    FileBrowserInstance instanceData = {
                        cXineOsd::browserInstance
                    };

                    cPluginManager::CallAllServices("Filebrowser set instance", &instanceData);


		    if(cXineOsd::browserInstance == -1)
		    {
		      Reel::FileBrowserSetPlayListEntries data;
		      data.name    = Reel::XineMediaplayer::currentPlaylistFilename;
		      data.entries = Reel::XineMediaplayer::cXineOsd::playListEntries;
		      data.instance = cXineOsd::browserInstance;
		      cPluginManager::CallAllServices("Filebrowser set playlist entries", &data);
		    }

                    FileBrowserSetCaller callerData = {
                        xine,
                        cXineOsd::browserInstance
                    };
                    cPluginManager::CallAllServices("Filebrowser set caller", &callerData);

                    FileBrowserControl ctrlData =  {
                        xineCtrl,
                        cXineOsd::browserInstance
                    };
                    cPluginManager::CallAllServices("Filebrowser xine control", &ctrlData);

                    cRemote::CallPlugin("filebrowser");
                }

                ////////////////////////////////////////////////////////////////////////////////

                eOSState cXineOsd::AddToFavorites()
                {
                   //printf("cXineOsd::AddToFavorites():\n");
                   //printf("---url = %s---\n", playListEntries[currentlySelected].c_str());
                   //printf("---name = %s---\n", nameListEntries[currentlySelected].c_str());

                    struct
                    {
                        std::string playListEntry;
                        std::string nameListEntry;
                    } data =
                    {
                        playListEntries[currentlySelected],
                        nameListEntries[currentlySelected]
                    };

                    cPluginManager::CallAllServices("Add New Radio Station", &data);
                    return osContinue;
                }

                ////////////////////////////////////////////////////////////////////////////////

        eOSState cXineOsd::ProcessKey(eKeys Key)
        {
//          printf("xineosd::ProcessKey(%i)\n",Key);
            eOSState state = osContinue;
                        if ( HasSubMenu() )
                        {

                            state = cOsdMenu::ProcessKey(Key);
                            //printf("HasSubMenu(): Key=%d, state = %d \n", Key, state);
                            if (Key == kBack && state==osUnknown)
                                CloseSubMenu();

                            if (!HasSubMenu()) {
                                printf("SubMenu closed. redrawing Menu\n");
                                first = true; // redraw id3 info and cover thumbnail

                                // if playlist is empty, remove cover-art thumbnail
                                if (playListEntries.size() == 0) {
                                    ShowCover(true); // remove cover
                                }

                                ShowEntries(); // redraw menu
                            }

                            return state;
                        }
                        else if (NORMALKEY(Key) == kBack) // do not close osd
                        {

                            if((control && !control->IsPlaying()) || playListEntries.size() == 0 || Interface->Confirm(tr("Exit mediaplayer?")))
                            {
                                // close mediaplayer if it was started
                                if (cXineOsd::control)
                                    cControl::Shutdown();
                                printf("**********    returning osBack\n");
                                return osBack;
                            }
                            else
                                return osContinue;

                        }

#if 1 // causes vdr crash ?
                        if(cXineOsd::closeOsd)
                        {
                            cXineOsd::closeOsd = false;
                            cXineOsd::firstOpen = false;

                            return osEnd; // close all menus, video is playing
                        }
#endif

                        time_t now = time(0);
                        // time counter for mode: should be used to clear mode when exceeding 4 secs
                        static cTimeCounter timeCounter_mode;

                        /* holds the time when the last volume bar was drawn*/
                        static time_t lastVolShownTime_saved = 0;

                        ///Display current Volume in MessageBar
                        if (lastVolShownTime_saved > 0 && now - lastVolShownTime_saved >= 2) // clear
                        {
                            SetStatus(NULL); // clear volume status
                            lastVolShownTime = 0;
                            lastVolShownTime_saved = 0;
                            ShowPlayMode(true);
                        }
                        else if (now - lastVolShownTime <=1 )
                        {

                            if (!cDevice::PrimaryDevice()->IsMute() &&
                                    /*lastVolShownTime_saved !=*/ lastVolShownTime)
                            {
                                std::string vol;
                                SetStatus(NULL); // clear volume status
                                vol = tr("Volume") + std::string(": ") +
                                        MakeProgressBarStr( cDevice::PrimaryDevice()
                                                            ->CurrentVolume()*100.0/255, 30  );
                                SetStatus(vol.c_str());
                                lastVolShownTime_saved = now ;
                            }
                            //donot respond to mute: Done in ShowPlayMode()
                        }

                        currentlySelected = Current();
                        if (currentlyPlaying >= 0 && currentlySelected > currentlyPlaying )
                            currentlySelected -= 2;  // account for the two additional line in OSD //progress & blank


                        if (FunctionMode == 1)
                        {
                          switch(Key)
                        {
                          case kBack:
                            break;

                          case kGreen:
                             // Shuffle/Random
                            if (playListEntries.size() > 2 )
                              cXineOsd::control->SetPlayMode(PLAY_RANDOM);
                             break;

                          case kBlue:
                              cXineOsd::control->SetPlayMode(PLAY_NORMAL); // XXX Set Mode Normal
                              break;

                          case kYellow:
                              // Repeat
                              //cXineOsd::control->SetPlayMode(PLAY_REPEAT_SINGLE);
                              cXineOsd::control->SetPlayMode(PLAY_REPEAT_PLAYLIST);
                              break;

                          case kRed:

                          case kNone:
                            // Else Player freezes after completing the current mrl!
                            if(isPlaying)
                            {
                              //printf("calling control->processKey()\n");
                              /*eOSState state = */ control->ProcessKey(Key);
                              //printf("Got state = %i\n", state);
                            }
                            break;

                          default:

                            break;

                        }

                        /*Timeout for showing function modes */
                        if ( Key != kNone || timeCounter_mode.Seconds() > 7)
                        {
                          FunctionMode = 0;
                          SetHelpButtons();
                          Key = kNone;
                        }

                       }
                        else
                          if (FunctionMode == 0)
            switch(Key)
            {
                case k0: 
                {
                    static int playbackMode = 0;
                    playbackMode = (playbackMode+1)%3;
                    
                    printf("got k0, now playbackMode = %d\n", playbackMode);
                    
                    switch(playbackMode) {
                        case 0: // normal playback
                            printf("\033[0;92mPlayback Mode: Normal\033[0m\n");
                            cXineOsd::control->SetPlayMode(PLAY_NORMAL); break;
                            
                        case 1: // shuffle / random
                            printf("\033[0;92mPlayback Mode: Random\033[0m\n");
                            cXineOsd::control->SetPlayMode(PLAY_RANDOM); break;
                            
                        case 2: // repeat playlist/loop
                            printf("\033[0;92mPlayback Mode: Repeat\033[0m\n");
                            cXineOsd::control->SetPlayMode(PLAY_REPEAT_PLAYLIST); break;
                            
                        default: // lets go back to normal playback
                            printf("\033[0;92mPlayback Mode: Normal (default)\033[0m\n");
                            cXineOsd::control->SetPlayMode(PLAY_NORMAL); break;
                    } // switch
                }
                    ShowEntries(); // update the title
                    state = osContinue;
                    break;
                case kInfo : // trap kInfo key
                printf("kInfo pressed\n");
                if ( (cXineOsd::k_blue == FILEBROWSER_PLUGIN ||
                        cXineOsd::k_blue == MUSIC_LIBRARY)&&
                    (currentlySelected >= 0 && currentlySelected < (int)playListEntries.size()))
                    AddSubMenu(new cFileInfoMenu(playListEntries.at(currentlySelected)));
                state = osContinue;
                    break;
                case kOk:
                    if (currentlySelected == currentlyPlaying) // close OSD
                    {
// Directy quit                     cRemote::Put(kBack);
                        state = osEnd;
                        break;
                    }

                                        Skins.Message(mtStatus, tr("Loading..."));
                                        // slow in playing next cd track, so some visual cue to the user

                    {
                    bool result = false;
                    // should play the correct song...
                    if (control && !(result=control->PlayFileNumber( currentlySelected )) )
                        Skins.Message(mtError, tr("Could not play file"));
                    
                    // file was played, so lets close the osd if the playlist item is a video file
                    if (result)
                        cXineOsd::firstOpen = true;
                   }
                break;

                case kRed:
                    CallExt_Plugin();
                    break;

                case kBlue:
                if (cXineOsd::k_blue == SHOUTCAST_PLUGIN)
                    return AddToFavorites();

                return AddSubMenu(new cXineOsdFunction(currentlySelected, playListEntries.size()));
                                  // different modes only for normal playlist, not for internet radio
                                  if ( CanHaveDifferentModes() )
                                  {
                                        FunctionMode = 1;
                                        SetHelpButtons();
                                        // start time out
                                        timeCounter_mode.CleanStart();
                                  }
                                  else if(cXineOsd::k_blue == SHOUTCAST_PLUGIN)
                                  {
                                        return AddToFavorites();
                                  }

                    break;

                                case kGreen: // overrides... <<
                                //break; // fall through

                                case kYellow: // overrides... >>
                                if (control) 
                                    return control->ProcessKey(Key);
                                break;
                                
                                    if (cXineOsd::k_blue == FILEBROWSER_PLUGIN &&
                                        (currentlySelected >= 0 && currentlySelected < (int)playListEntries.size()))
                                        AddSubMenu(new cFileInfoMenu(playListEntries.at(currentlySelected)));

                                    break;

                case kBack:

                    //fall thro'
                case kUp:
                case kUp|k_Repeat://prev
                    //fall thro'
                case kDown:
                case kDown|k_Repeat: // next

                    state = cOsdMenu::ProcessKey(Key);
                    break;

                case kLeft:
                case kChanUp:
                case kChanUp|k_Repeat: // page up
                    state = cOsdMenu::ProcessKey(kLeft);
                    break;

                case kRight:
                case kChanDn:
                case kChanDn|k_Repeat: // page down
                    state = cOsdMenu::ProcessKey(kRight);
                    break;

                default:
                    if (Key == kPause)
                    {
                        kPausePressed = 1;
                        // also set inside XineLib.c, in the case Osd is closed
                    }
                    else if (NORMALKEY(Key) == kStop)
                    {
                        if (control) control->StopPlayback();
                        ShowEntries(); // redraw entries without progressbar
                        state = osContinue;
                        break;
                    }

                            // fall thro'
#ifdef USE_PLAYPES
                    // RBMINI/USE_PLAYPES still cannot jump/skip,
                    // instead do the usual PgUp & PgDn
                    state = cOsdMenu::ProcessKey(Key);
#else
                //  printf("\033[1;91m ----------- cXineOsd::ProcessKey(%i) isPlaying %i------\033[0m\n", Key, isPlaying);
                    if(isPlaying)
                    {
                    //printf("Calling control->ProcessKey() line %i\n", __LINE__);
                                            if ( osEnd == control->ProcessKey(Key) )
                            cControl::Shutdown(); // now shuts down with kStop
                        else
                            if (/*Key == kLeft || Key == kRight || */ Key == kFastFwd || Key == kFastRew) // player requested to skip
                                osdTimeOut.Set(2000);  // give player time to react on the jump, so delay osd refresh
                    //printf("Out of control->ProcessKey() line %i\n", __LINE__);
                    }
#endif
                    break;
            }

                        if (state != osUnknown && Key != kNone) {
                            ShowSideNoteInfo();
                        }
            ConvertCover();
            ShowCover(false);
            ShowInfo(false);
            first = false;

            if (cXineOsd::isNew)
                ShowEntries(); // refresh
            else
                if (osdTimeOut.TimedOut())
                {
                    //printf("Calling UpdateEntries line %i\n", __LINE__);
                    UpdateEntries();
                    //printf("DONE UpdateEntries line %i\n", __LINE__);
                    ShowPlayMode();
                    osdTimeOut.Set(1000); // refresh every 1 second
                }


            // return to current song after 7 secs
            static time_t lastUserInputTime = now;
            if (Key != kNone )
                lastUserInputTime = now;
            else // Key == kNone
                if (now - lastUserInputTime > 7 && currentlyPlaying>=0) // return to current file, only if playing
                    SetCurrent(Get(currentlyPlaying));


            return state;
        }

        ////////////////////////////////////////////////////////////////////////////////

        bool cXineOsd::IsImage(std::string name)
        {
            if(IsInsideStringChain("bmp jpeg jpg tif tiff png", GetEnding(name), false))
            {
                return true;
            }
            return false;
        }

        std::string cXineOsd::GetCoverFromDir(std::string mrl)
        {
            std::string dir = RemoveName(mrl);

            if(StartsWith(dir, "http:")) //internet radio
            {
                return "";
            }

            std::vector<Filetools::dirEntryInfo> files;
            Filetools::GetDirFiles(dir, files);

            bool foundFirst = false;
            int first = 0;

            for(unsigned int i = 0; i < files.size(); ++i)
            {
                if(IsImage(files[i].name_))
                {
                    if(!foundFirst)
                    {
                        foundFirst = true;
                        first = i;
                    }

                    if(strcasecmp(RemoveEnding(GetLast(files[i].name_)).c_str(), "cover") == 0
                    || strcasecmp(RemoveEnding(GetLast(files[i].name_)).c_str(), "artist") == 0
                    || strcasecmp(RemoveEnding(GetLast(files[i].name_)).c_str(), "background") == 0
                    || strcasecmp(RemoveEnding(GetLast(files[i].name_)).c_str(), "front") == 0)
                    {
                        return files[i].name_;
                    }
                }
            }

            if(foundFirst)
            {
                return files[first].name_;
            }

            return "";
        }

        ////////////////////////////////////////////////////////////////////////////////

        void cXineOsd::GetImageInfoFromID3(std::string path, std::string &image, int &size)
        {
            TagLib::MPEG::File f(path.c_str());
            if(f.ID3v2Tag())
            {
                TagLib::ID3v2::FrameList l = f.ID3v2Tag()->frameListMap()["APIC"];
                if(!l.isEmpty())
                {
                    //TODO: look up other image formats
                    //std::string bla = l.front()->toString() ;
                    //std::string ending =
                    image = RemoveEnding(GetLast(path)) + "." + "jpg"; //ending;
                    size = l.front()->size();
                    return;
                }
            }
            image = "";
            size = 0;
        }

        ////////////////////////////////////////////////////////////////////////////////

        void cXineOsd::ExtractImageFromID3(std::string path)
        {
            TagLib::MPEG::File f(path.c_str());
            if(f.ID3v2Tag())
            {
                TagLib::ID3v2::FrameList l = f.ID3v2Tag()->frameListMap()["APIC"];
                if(!l.isEmpty())
                {
                    TagLib::ID3v2::AttachedPictureFrame *frame =  dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(l.front());
                    if(frame)
                    {
                        std::string name = RemoveEnding(GetLast(path)) + "." + "jpg"; //ending;
                        std::vector<unsigned char> buffer(frame->size());
                        memcpy(&buffer[0], frame->picture().data(), sizeof(unsigned char)*frame->size());
                        if(ID3cache)
                        {
                            ID3cache->PutInCache(name, buffer, "", true);
                        }
                    }
                }
            }
        }

        ////////////////////////////////////////////////////////////////////////////////

        void cXineOsd::ConvertCover()
        {
            if(currentlySelected < 0 || currentlySelected >= (int)playListEntries.size())
            {
                cover = ""; // remove cover-art thumbnail if invalid selection
                return;
            }

            std::string image = "";

            //try to find an image in the ID3-tag
            if(strcasecmp(GetEnding(playListEntries[currentlySelected]).c_str(), "mp3") == 0 && cache && ID3cache)
            {
                int filesize = 0;
                GetImageInfoFromID3(playListEntries[currentlySelected], image, filesize);
                if(image != "")
                {
                    cover = image;
                    coverSize = filesize;
                }
                if(image != "" && !cache->InCache(cover, filesize, thumbConvert->GetEnding()))
                {
                    if(!ID3cache->InCache(image, "", true))
                    {
                        ExtractImageFromID3(playListEntries[currentlySelected]);
                    }
                    else
                    {
                        std::string file;
                        ID3cache->FetchFromCache(image, file, "", true);
                        if(file != "")
                        {
                            thumbConvert->InsertInConversionList(file, true);
                            thumbConvert->StartConversion();
                        }
                    }
                }
                else if(image != "")
                {
                    ShowCover(false);
                }
            }

            //no image in I3D, look for appropriate image in directory
            if(image == "" && cache)
            {
                image = GetCoverFromDir(playListEntries[currentlySelected]);
                if(image != "")
                {
                    cover = image;
                    coverSize = Filetools::GetFileSize(image);
                }
                if (image != "" && !cache->InCache(image, coverSize, thumbConvert->GetEnding()))
                {
                    thumbConvert->InsertInConversionList(cover, true);
                    thumbConvert->StartConversion();
                }
                else if(image != "")
                {
                    ShowCover(false);
                }
            }

            if(image == "")
            {
                cover = image;
                ShowCover(false);
            }
        }

        ////////////////////////////////////////////////////////////////////////////////

        struct CurThumb
        {
        int x;
        int y;
        char path[255];
        bool blend;
        int slot;
        int w;
        int h;
        };

        eOSState cXineOsd::ShowCover(bool hide)
        {
            //printf("ShowCover, cover = %s, previouslySelectedCover = %d, currentlySelected = %d\n", cover.c_str(), previouslySelectedCover, currentlySelected);
            static bool dirty = false;
            if(currentlySelected != previouslySelectedCover || first)
            {
                if(cover == "")
                {
                    hide = true;
                }
                previouslySelectedCover = currentlySelected;
                dirty = true;
            }

            if(hide)
            {
                //printf("HideCover\n");
                isShowingCover = false;
                dirty = false;
                cPluginManager::CallAllServices("setThumb", NULL);
            }
            else if(dirty)
            {
                //printf("######ShowCover, dirty\n");
                std::string thumb;
                if(cache && cache->FetchFromCache(cover, thumb, coverSize, thumbConvert->GetEnding()))
                {
                    /** Hack: insert "SHOWINGMP3COVER$" in the title to tell the skin to use another banner,
                     * the skin will filter the "SHOWINGMP3COVER$" out again */

                    std::string title, dummy;
                    std::string setTitle_str = "SHOWINGMP3COVER$" + string(tr("Mediaplayer")) + PlaybackModeToTitle();

                    if (currentlyPlaying >= 0 &&
                        currentlyPlaying < (int)playListEntries.size())
                        Id3Info( playListEntries[currentlyPlaying],
                             title, dummy, dummy, dummy, dummy, NULL, NULL);

                    if (!title.empty() )
                        setTitle_str += ": " + title;
                    SetTitle(setTitle_str.c_str() );

                    isShowingCover = true;
                    dirty = false;
                    //don't let ID3cache grow
                    ID3cache->RemoveFromCache(cover, "", true);

                    //tell skinreelng that a new thumb is available
                    struct CurThumb curThumb;
                    memset(&curThumb, 0, sizeof(curThumb));
                    curThumb.x = 22;
                    curThumb.y = 315;
                    curThumb.h = curThumb.w = 125;
                    curThumb.blend = false;
                    curThumb.slot = 0;
                    strncpy(curThumb.path, thumb.c_str(), sizeof(curThumb.path)-1);
                    cPluginManager::CallAllServices("setThumb", (void*)&curThumb);
                }
            }
            return osContinue;
        }
        
        std::string cXineOsd::PlaybackMode() const
        {
            if (!control) return "";
            
            int mode = control->GetPlayMode();
            std::string modeStr = "";
            switch (mode) 
            {
                case PLAY_NORMAL: 
                    modeStr = ""; break;
                    
                case PLAY_RANDOM: 
                    modeStr = tr("Shuffle"); break;
                    
                case PLAY_REPEAT_PLAYLIST: 
                    modeStr = tr("Repeat"); break;
                    
                case PLAY_REPEAT_SINGLE:
                    modeStr = tr("Repeat single"); break;
                    
                default:
                    modeStr = ""; break;
            } // switch
            
            return modeStr;
        }
        
        std::string cXineOsd::PlaybackModeToTitle() const
        {
            if (!isPlaying) return "";
            
            std::string mode = PlaybackMode();
            
            if (!mode.empty())
                return std::string(" (") + mode + std::string(")");
            else
                return "";
        }

        eOSState cXineOsd::ShowInfo(bool hide)
        {
            static bool dirty = false;
            if(currentlySelected != previouslySelectedInfo || first)
            {
                previouslySelectedInfo = currentlySelected;
                dirty = true;
            }

            if ((currentlySelected >= 0 &&  currentlySelected < (int)playListEntries.size())
                && StartsWith(playListEntries[currentlySelected], "http://"))
            {
                static std::string last_headerline = "";
                std::string headerline = control->GetDisplayHeaderLine();
                if(headerline != last_headerline)
                {
                    last_headerline = headerline;
                    dirty = true;
                }
            }

            if(hide)
            {
                cPlugin *skin = cPluginManager::GetPlugin("skinreel3");
                ID3Tags.clear();
                if(skin) skin->Service("setId3Infos", NULL);
                dirty = false;
                Display();

            }
            else if(dirty)
            {
                    std::string title, album, artist, genre, comment;
                    int year = 0;
                    int track = 0;
                    std::string setTitle_str = "SHOWINGMP3COVER$" + string(tr("Mediaplayer")) + PlaybackModeToTitle();
                    if (currentlySelected >= 0 &&
                        currentlySelected < (int)playListEntries.size())
                    {

                        bool success = Id3Info( playListEntries[currentlySelected],
                             title, album, artist, comment, genre, &year, &track);

                        //internet radio
                        if(! success && StartsWith(playListEntries[currentlySelected], "http://"))
                        {
                            //get title from player
                            std::string headerline = control->GetDisplayHeaderLine();
                            title = GetBeforeFirst(GetAfterLast(headerline, "]"), "-");
                            artist = GetAfterLast(GetAfterLast(headerline, "]"), "-");
                            trim(title, " ");
                            trim(artist, " ");

                            if (!title.empty() )
                            {
                                setTitle_str += ": " + title;
                                showID3TagInfo(title, artist, "", "", "", 0, 0);
                            }
                        }
                        else
                        {
                            if (!title.empty() )
                            {
                                setTitle_str += ": " + title;
#if APIVERSNUM < 10700
                                showID3TagInfo(title, artist, album, comment, genre, year, track);
#endif /* APIVERSNUM */
                                Display();
                            }
                            else
                            {
                                cPlugin *skin = cPluginManager::GetPlugin("skinreel3");
                                ID3Tags.clear();
                                if(skin) skin->Service("setId3Infos", NULL);
                                Display();
                            }
                        }
                    }
                    dirty = false;
            }
            return osContinue;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //End class cXineOsd
        ////////////////////////////////////////////////////////////////////////////////


        /////////////////////////////////////////////////////////////////////////////////////////////
        // class cXineVolStatus
        /////////////////////////////////////////////////////////////////////////////////////////////

        void cXineStatus::SetVolume(int Vol, bool abs)
        {
            //printf("(%s:%d) :%s(%d/%d)\n", __FILE__,__LINE__,__PRETTY_FUNCTION__, Vol, abs);

            cXineOsd::lastVolShownTime = time(0);
            printf("ThreadID: %d  %lld\n",
                   cThread::ThreadId(),cXineOsd::lastVolShownTime
                   );
        }

    } //namespace Xinemediaplayer
} // namespace Reel
