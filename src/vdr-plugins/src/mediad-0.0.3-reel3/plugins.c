#include "plugins.h"
#include "setup.h"
#include <vdr/recording.h>
#include <vdr/resumeDvd.h>

#include "../filebrowser/stringtools.h"

#if HAVE_XINEMEDIAPLAYER
#include "../xinemediaplayer/services.h"
#include <vector>
#endif

extern const char *VideoDirectory;
extern cRecordings Recordings;

//----------------------vdrrecording---------------------------

vdrrecording::vdrrecording() : 
    mediaplugin("vdr_rec") 
{
    addPattern(pattern(State, "001.vdr", "VDR Recording"));
    addPattern(pattern(Find, "001.vdr", "VDR Recording"));
}

bool vdrrecording::checkPluginAvailable(bool ignoreActivation)
{
    // always return true as it is a fix part of vdr
    return true;
}

bool vdrrecording::checkPluginAvailable(const cPlugin* plugin) 
{
    return mediaplugin::checkPluginAvailable(plugin);
}

void vdrrecording::TestDvdLink()
{
  struct stat buf;
  string VideoDir = VideoDirectory;
  VideoDir.append("/dvd");
  if (lstat(VideoDir.c_str(),&buf) < 0)
  {
      printf ( " DBUG VDRCD  missing Link at %s  ln -s /media/dvd  %s ", VideoDir.c_str(), VideoDir.c_str());
      symlink("/media/dvd", VideoDir.c_str());
  }
  //printf ( " DBUG VDRCD  ---------- TestDvdLink %s  ----------  ", VideoDir.c_str());
}

mediaplugin* vdrrecording::clone()
{
    return new vdrrecording(*this);
}

cOsdObject * vdrrecording::startPlugin() 
{
//  const char *oldvideodir = VideoDirectory;
    char* path;

    if (_needsUmount) {
        _st_curDevice->umount();
    }
    if (0 /*_needsDirectory*/) {
        if (_st_tmpCdrom == NULL) {
            _st_tmpCdrom = tmpnam(NULL);
            mkdir(_st_tmpCdrom,0777);
            asprintf(&path, "%s/cdrom", _st_tmpCdrom);
            mkdir(path, 0777);
            free(path);
            asprintf(&path, "%s/cdrom/%s", _st_tmpCdrom, VDRSUBDIR);
            symlink(_st_curDirectory, path);
            free(path);
        }
        VideoDirectory = _st_tmpCdrom;
    } //else
    //  VideoDirectory = _st_curDirectory;

    DDD("open recmenu in %s", VideoDirectory);

    _st_status->setCurrentDevice(_st_curDevice);
 
    Recordings.Load(); 
    cPlugin *p = cPluginManager::GetPlugin("extrecmenu");
    if(p) {
        DDD("using extrecmenu");
        cOsdObject *osd = NULL;
        TestDvdLink();
//      VideoDirectory = oldvideodir; 
        p->Service("archive", (void *) "mount"); // call services 
        osd = p->MainMenuAction();

        if (osd)
        {
          osd->ProcessKey(kOk);
        }

        _st_status->LinkedRecording(true);
        return osd;
    }  
    cOsdMenu *menu = new cMenuRecordings();
//  VideoDirectory = oldvideodir;
    _st_status->LinkedRecording(true);
 
    return menu;
}

//-------------------------dmharchivedvd---------------------------

dmharchivedvd::dmharchivedvd() : 
    mediaplugin("dmh_archive") 
{
    setSearchMode(And);
    addPattern(pattern(State, "video_ts", "DMH DVD"));
    addPattern(pattern(Find, "dvd.vdr", "DMH DVD"));
}

bool dmharchivedvd::checkPluginAvailable(bool ignoreActivation)
{
    // always return true as it is a fix part of vdr
    return true;
}

bool dmharchivedvd::checkPluginAvailable(const cPlugin* plugin) 
{
    //return mediaplugin::checkPluginAvailable(plugin);
    return false;
}

mediaplugin* dmharchivedvd::clone()
{
    return new dmharchivedvd(*this);
}

cOsdObject * dmharchivedvd::startPlugin() 
{
    const char *oldvideodir = VideoDirectory;
    char* titlePath = NULL;
    char* episodePath = NULL;
    struct stat buf;

    DDD("checking for dmh-archive-dvd");

    // TODO: OK
    //if (_st_tmpCdrom == NULL) {
    char *video_ts_dir = NULL;
    if (_needsUmount) {
        _st_curDevice->umount();
    }
    // TODO OK?
    //if(_st_tmpCdrom)
    //  free(_st_tmpCdrom);
    _st_tmpCdrom = tmpnam(NULL);
    mkdir(_st_tmpCdrom,0777);
    asprintf(&video_ts_dir, "%s/video_ts",_st_curDirectory);
    if(stat(video_ts_dir, &buf) != 0) {
            DDD("did not find %s", video_ts_dir);
        free(video_ts_dir);
        asprintf(&video_ts_dir, "%s/VIDEO_TS",_st_curDirectory);
        if(stat(video_ts_dir, &buf) != 0) {
            DDD("did not find %s", video_ts_dir);
            Skins.Message(mtError, tr("Wrong dmh-archive-dvd structure!"));
            free(video_ts_dir);
            return NULL;
        }
    }
    for(int track = 0; track <= 99; track++) {
        char *info_vdr = NULL;
        asprintf(&info_vdr, "%s/info_%.2d.vdr",video_ts_dir, track);
        if(stat(info_vdr, &buf) == 0) {
            char *index_vdr;
            char path_buf[BUF_SIZ];
            FILE* finfo;
            cReadLine ReadLine;
            char* bufinfo = NULL;
            char* title = NULL;
            char* episode = NULL;
            // create title dir
            if ((finfo = fopen(info_vdr, "r")) != (FILE *)NULL) {
                while(( bufinfo = ReadLine.Read(finfo))) {
                    if(bufinfo[0] == 'T')
                    break;
                }
                fclose(finfo);
            }
            if(bufinfo)
                asprintf(&title, "%s", bufinfo + 2);
            else
                asprintf(&title, "cdrom");
            if(titlePath)
            {
                free(titlePath);
                titlePath = NULL;
            }
            asprintf(&titlePath, "%s/%s", _st_tmpCdrom, title);
            mkdir(titlePath, 0777);
            // check whether there is an episode name
            if ((finfo = fopen(info_vdr, "r")) != (FILE *)NULL) {
                while(( bufinfo = ReadLine.Read(finfo))) {
                    if(bufinfo[0] == 'S')
                    break;
                }
                fclose(finfo);
            }
            if(bufinfo)
                asprintf(&episode, "%s", bufinfo + 2);
            else
                asprintf(&episode, "track%d",track);
            if(episodePath)
            {
                free(episodePath);
                episodePath = NULL;
            }
            asprintf(&episodePath, "%s/%s", titlePath, episode);
            DDD("Episode path %s", episodePath);
            mkdir(episodePath, 0777);

            asprintf(&index_vdr, "%s/index_%.2d.vdr",video_ts_dir,track);
            if(episodePath)
                asprintf(&index_vdr, "%s/index_%.2d.vdr",video_ts_dir,track);
            if(episodePath)
            {
                free(episodePath);
                episodePath = NULL;
            }
            asprintf(&episodePath, "%s/%s/%s/%s%.2d.rec", _st_tmpCdrom, title, episode, VDRSUBDIRARCH, track);
            mkdir(episodePath, 0777);
            sprintf(path_buf, "%s/%s/%s/%s%.2d.rec/index.vdr", _st_tmpCdrom, title, episode, VDRSUBDIRARCH, track);
            symlink(index_vdr, path_buf);
            sprintf(path_buf, "%s/%s/%s/%s%.2d.rec/info.vdr", _st_tmpCdrom, title, episode, VDRSUBDIRARCH, track);
            symlink(info_vdr, path_buf);
            for(int n = 0; n <= MAXFILESPERRECORDING; n++) {
                char vob[BUF_SIZ];
                sprintf(vob, "%s/VTS_%02d_%d.VOB", video_ts_dir, track, n);
                DDD("VOB %s", vob);
                sprintf(path_buf, "%s/%s/%s/%s%.2d.rec/%03d.vdr", _st_tmpCdrom, title, episode, VDRSUBDIRARCH, track, n);
                if(stat(vob, &buf) != 0) {
                    sprintf(vob, "%s/vts_%02d_%d.vob", video_ts_dir, track, n);
                    if(stat(vob, &buf) != 0) 
                        break;
                }
                symlink(vob, path_buf);
            }
            free(index_vdr);
            free(title);
            free(episode);
        }
        free(info_vdr);
    }
    free(video_ts_dir);
    if(titlePath)
    {
        free(titlePath);
        titlePath = NULL;
    }
    if(episodePath)
    {
        free(episodePath);
        episodePath = NULL;
    }
    //}
    VideoDirectory = _st_tmpCdrom;
    DDD("open recmenu in %s", VideoDirectory);

    _st_status->setCurrentDevice(_st_curDevice);

    Recordings.Load();
    cPlugin *p = cPluginManager::GetPlugin("extrecmenu");
    if(checkPluginAvailable(p)) {
        DDD("using extrecmenu");
        cOsdObject *osd = NULL;
        VideoDirectory = oldvideodir; // remove this if necessary
        p->Service("archive", (void *) "mount"); // call services
        osd = p->MainMenuAction();

        /*  if (osd) // why processkey once again ? 
        {
           osd->ProcessKey(kOk);
        } */

        //  VideoDirectory = oldvideodir; // add this if necessary
        
        _st_status->LinkedRecording(true);
        return osd;
    }  
    cOsdMenu *menu = new cMenuRecordings();
    VideoDirectory = oldvideodir;
    _st_status->LinkedRecording(true);

    return menu;
}

//-------------------------vcd---------------------------

vcd::vcd() : 
#if HAVE_XINEMEDIAPLAYER
    mediaplugin("xinemediaplayer")
#else
        mediaplugin("vcd")
#endif
{
    _needsUmount = true;
    addPattern(pattern(MediaType, svcd, "SVCD"));
}

mediaplugin* vcd::clone()
{
    return new vcd(*this);
}

cOsdObject * vcd::startPlugin() 
{
#if HAVE_XINEMEDIAPLAYER

    cPluginManager::CallAllServices("Xine Play VideoCD", NULL);
    return NULL;
#else
    cPlugin *p = cPluginManager::GetPlugin(getName());
    cOsdObject *osd = NULL;
    if (checkPluginAvailable(p)) {
        osd = p->MainMenuAction();

        const char *last = _st_status->Current();

        osd->ProcessKey(kDown);
        if (_st_status->Current() != last) {
            // VCD has more than one track, so leave the menu open
            osd->ProcessKey(kUp);
        } else {
            // Autostart CD and destroy menu
            osd->ProcessKey(kOk);

            delete osd;
            osd = NULL;
        }
    }
    return osd;
#endif

}

//-------------------------mplayer---------------------------

mplayer::mplayer() : 
    mediaplugin("mplayer") 
{
    addPattern( pattern(Find, "*.avi", "AVI movie disc"));
    addPattern( pattern(Find, "*.avi-????", "AVI movie disc"));
    addPattern( pattern(Find, "*.mpg", "MPG movie disc"));
    addPattern( pattern(Find, "*.mpeg", "MPG movie disc"));
}

mediaplugin* mplayer::clone()
{
    return new mplayer(*this);
}

cOsdObject * mplayer::startPlugin() 
{ 
    DDD("Looking for mplayer plugin");
    cPlugin *p = cPluginManager::GetPlugin(getName());
    cOsdObject *osd = NULL;
    if (checkPluginAvailable(p)) {
        DDD("Found mplayer plugin");
        osd = p->MainMenuAction();
        // Switch to "source" mode
        osd->ProcessKey(kYellow);
        // Move to first entry
        const char *actual = _st_status->Current();
        osd->ProcessKey(kUp);
        while (actual != _st_status->Current()) {
            osd->ProcessKey(kUp);
            actual = _st_status->Current();
        }
        // Search for /cdrom-entry
        bool mplayersource = true;
        while (strstr(actual, _st_curDirectory) == NULL) {
            osd->ProcessKey(kDown);
            if (actual == _st_status->Current()) {
                mplayersource = false;
                break;
            } else {
                actual = _st_status->Current();
            }
        }
        if (mplayersource) {
            osd->ProcessKey(kOk);

            const char *last = _st_status->Current();
            osd->ProcessKey(kDown);
            if (_st_status->Current() != last) {
                DDD("Multiple avi files");
                // CD has more than one avi-file, so leave the menu open
                osd->ProcessKey(kUp);
            } else {
                DDD("Only a single avi file");
                // eutostart CD and destroy menu
                // TODO warum do nix gehen?
                osd->ProcessKey(kRed);
            }
        } else {
            delete osd;
            osd = NULL;
            Skins.Message(mtError, tr("Drive not present in mplayersources.conf!"));
        }
    }
    return osd;
}

//-------------------------mp3---------------------------

mp3::mp3() : 
    mediaplugin("music") 
{
    addPattern( pattern(Find, "*.mp3", "MP3 player"));
    addPattern( pattern(Find, "*.ogg", "OGGVorbis player"));
    addPattern( pattern(Find, "*.wav", "Audio CD"));
}

mp3::~mp3()
{
}

mediaplugin* mp3::clone()
{
    return new mp3(*this);
}

cOsdObject* mp3::startPlugin() 
{
    DDD("Looking for mp3 plugin");
    cPlugin *p = cPluginManager::GetPlugin(getName());
    cOsdObject *osd = NULL;
    if (checkPluginAvailable(p)) {
        DDD("Found mp3 plugin");
        osd = p->MainMenuAction();

        const char *wanted = I18nTranslate("MP3", getName());

        // Switch to Browse Mode if not already done...
        if (strncmp(_st_status->Title(), wanted, strlen(wanted)) != 0)
            osd->ProcessKey(kBack);

        // Now switch into "source" mode
        osd->ProcessKey(kGreen);

        const char *last = NULL;
        while (last != _st_status->Current()) {
            if (strcmp(_st_status->Current() + strlen(_st_status->Current()) 
                    - strlen(_st_curDirectory), _st_curDirectory) == 0) {

                osd->ProcessKey(kRed);    // Select it
                osd->ProcessKey(kYellow); // Change to browse mode again

                osd->ProcessKey(kYellow); // Instant Playback
                break;
            }

            last = _st_status->Current();
            osd->ProcessKey(kDown);
        }

        /*delete osd;
        osd = NULL;
        */
        if (last == _st_status->Current())
            Skins.Message(mtError, tr("Drive not present in mp3sources.conf!"));
    }
    return osd;
}

//-------------------------ripit---------------------------

ripit::ripit() : 
    mediaplugin("ripit")
{
    _needsUmount = true;
    //addPattern( pattern(Find, "*.wav", "Audio CD for ripping"));
    addPattern( pattern(MediaType, audio, "Audio CD for ripping"));
}

mediaplugin* ripit::clone()
{
    return new ripit(*this);
}

bool ripit::matches() 
{ 
    return _found && RequestDvdAction; 
}

cOsdObject * ripit::startPlugin() 
{
    cPlugin *p = cPluginManager::GetPlugin(getName());
    cOsdObject *osd = NULL;
    if (checkPluginAvailable(p)) {
        osd = p->MainMenuAction();
        //osd->ProcessKey(kRed);
    }
    return osd;
}

//-------------------------dvd---------------------------

dvd::dvd() :
#if HAVE_XINEMEDIAPLAYER
    mediaplugin("xinemediaplayer")
#else
    mediaplugin("dvd")
#endif      
{
    _needsUmount = true;
    addPattern(pattern(State, "video_ts", "Video DVD"));
}

#if HAVE_XINEMEDIAPLAYER
cOsdObject* dvd::startPlugin()
{
    //get DVD id
    std::string id = DvdId();
    std::string dvdmrl = "dvd://";
    if ( !id.empty() && ResumeDvd.SetCurrentDvd(id))
    {
        // dvd-id in the resume list
        dvdmrl += ResumeDvd.TitleChapter();
    } 
    // else start dvd from beginning
    
    std::vector<std::string> playlistEntries; //empty list
    Xinemediaplayer_Xine_Play_mrl xinePlayData ;
    xinePlayData.mrl = dvdmrl.c_str();
    xinePlayData.instance = -1;
    xinePlayData.playlist = false;
    xinePlayData.playlistEntries = playlistEntries;
    cPluginManager::CallAllServices("Xine Play mrl", &xinePlayData);
    return NULL;
}
#endif

mediaplugin* dvd::clone()
{
    return new dvd(*this);
}

//-------------------------photocd------------------------

photocd::photocd() : 
    mediaplugin("pcd")
{
    _needsUmount = true;
    addPattern(pattern(State, "photo_cd", "Photo CD"));
}

mediaplugin* photocd::clone()
{
    return new photocd(*this);
}

//-------------------------cdda---------------------------

cdda::cdda() :
#if HAVE_XINEMEDIAPLAYER
    mediaplugin("xinemediaplayer")
#else
   mediaplugin("cdda")
#endif
{
  _needsUmount = true;
  addPattern( pattern(MediaType, audio, "Audio CD Player"));
}

mediaplugin* cdda::clone()
{
    return new cdda(*this);
}
#if HAVE_XINEMEDIAPLAYER
cOsdObject* cdda::startPlugin()
{
    cPluginManager::CallAllServices("Xine Play AudioCD", NULL);
    return NULL;
}
#endif

//-------------------------burn---------------------------

burn::burn() :
   mediaplugin("burn")
{
  _needsUmount = true;
  addPattern( pattern(MediaType, blank, "DVD creation"));
}

mediaplugin* burn::clone()
{
    return new burn(*this);
}

//-------------------------dvdswitch---------------------------

dvdswitch::dvdswitch() :
    mediaplugin("dvdswitch")
{
    _needsUmount = true;
        //TODO: correct pattern?
    addPattern(pattern(State, "video_ts", "Copy DVD"));
}

cOsdObject* dvdswitch::startPlugin()
{ 
        struct
        {
            char const *cmd;
        } dvdSwitchAction  =
        {
            "copy", 
        };
        cPluginManager::CallAllServices("Dvdswitch perform action", &dvdSwitchAction);
        //cRemote::CallPlugin("dvdswitch");
        return NULL;
}

bool dvdswitch::matches()
{
        if(RequestDvdAction) //offer dvdswitch as alternative action
        { 
                return mediaplugin::matches();
        }
        else
        {
                return false;
        }
}

mediaplugin* dvdswitch::clone()
{
    return new dvdswitch(*this);
}


//-------------------------filebrowser---------------------------

filebrowser::filebrowser() :
   mediaplugin("filebrowser")
{
  _needsUmount = true;
   addPattern( pattern(MediaType, blank, "browse data"));
  _pluginDescription = "Browse data";
}

filebrowser::~filebrowser()
{
}

mediaplugin* filebrowser::clone()
{
    return new filebrowser(*this);
}

bool filebrowser::matches() 
{   
    //printf("---------------filebrowser::matches()---------------------\n");
    _pluginDescription = "Browse data"; 
    bool mounted = _st_curDevice->isMounted();
    const char *mountPoint = _st_curDevice->getMountPoint(); 
    if(mounted && mountPoint)
    {
        path_ = mountPoint;
        std::vector<std::string>  entries;
        GetDirEntries(path_ , entries);

        //must have directories other than VIDEO_TS and AUDIO_TS
        if(entries.size() > 4) 
        {
            return true;
        }
        else
        {
            for(unsigned int i = 0; i < entries.size(); i++)
            {
                 if(GetLast(entries[i])== "VIDEO_TS")
                 {
                     return false;
                 }
            }
        }
        return true;

        //printf("------------filebrowser::matches():  entries.size() = %d---------------\n", entries.size()); 
        /*if(_st_curDevice->umount())
        { 
            printf("\n\n\n111111111111111111111111111111111111111111111111111111111111111111111111111111\n\n\n");
            _st_curDevice->mount();
            printf("\n\n\n2222222222222222222222222222222222222222222222222222222222222222222222222222222\n\n\n");
            return true;
        }*/
    }  
    return false;
} 

bool filebrowser::matches2(std::vector<mediaplugin *> matchedplugins)
{
    return matches();
} 

bool PluginMatches(std::string name, std::vector<mediaplugin *>matchedPlugins)
{
    for(unsigned int i = 0; i < matchedPlugins.size(); ++i)
    {
        if (matchedPlugins[i]->getName() == name)
        {
            return true;
        }
    }
    return false;
}

cOsdObject* filebrowser::startPlugin()
{  
    struct FileBrowserSetStartDir
    {
            char const *dir;
            char const *file;
    } startdir = { path_.c_str(), "" };
    //printf("------------------filebrowser::startPlugin, path_ = %s------------------\n", path_.c_str());
    cPluginManager::CallAllServices("Filebrowser set startdir", &startdir);
    cRemote::CallPlugin("filebrowser");
    return NULL;
}    

bool filebrowser::GetDirEntries(const std::string &dir, std::vector<std::string>  &entries)
{
    DIR *dirp;
    struct dirent64 *dp; 

    if ((dirp = opendir(dir.c_str())) == NULL) 
    {
        printf("couldn't open %s\n", dir.c_str()); 
        return false;
    }

    while((dp = readdir64(dirp)) != NULL)
    {
        std::string name = dir + "/" + dp->d_name; 
        entries.push_back(name);
    }
    closedir(dirp);
    return true;
} 

