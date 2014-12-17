/*
 * reelchannellist.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include "favourites.h"
#include "menu.h"
#include "activelist.h"
#include "setup.h"

#include <sstream>

static const char *VERSION        = "0.0.9";
static const char *DESCRIPTION    = "Lists channels and manages favourites";
static const char *MAINMENUENTRY  = "Reel Channellist";

class cPluginReelChannellist : public cPlugin {
private:
  // Add any member variables or functions you may need here.
  void LoadFavouritesList();
public:
  cPluginReelChannellist(void);
  virtual ~cPluginReelChannellist();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return DESCRIPTION; }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual void MainThreadHook(void);
  virtual cString Active(void);
  virtual time_t WakeupTime(void);
  virtual const char *MainMenuEntry(void) { return MAINMENUENTRY; }
  virtual cOsdObject *MainMenuAction(void);
  bool HasSetupOptions() { return false;}
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  };

cPluginReelChannellist::cPluginReelChannellist(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!

    ChannelSortMode = SortByNumber;

}

cPluginReelChannellist::~cPluginReelChannellist()
{
  // Clean up after yourself!
}

const char *cPluginReelChannellist::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginReelChannellist::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginReelChannellist::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  return true;
}

void cPluginReelChannellist::LoadFavouritesList()
{
    cString confDir = cPlugin::ConfigDirectory(PLUGIN_NAME_I18N);
    cString favFile = AddDirectory(*confDir, "favourites.conf");

#ifdef REELVDR
    favourites.LoadWithDuplicates(*favFile);
#else
    favourites.Load(*favFile);
#endif
}

bool cPluginReelChannellist::Start(void)
{
  // Start any background activities the plugin shall perform.

    /* load favourites channels from favourites.conf file*/
    cString confDir = cPlugin::ConfigDirectory(PLUGIN_NAME_I18N);
    cString favFile = AddDirectory(*confDir, "favourites.conf");
    cString activeList = AddDirectory(*confDir, "activeChannelList.conf");

    printf("Loading fav channels from '%s'\n", *favFile);
#ifdef REELVDR
    favourites.LoadWithDuplicates(*favFile);
    activeChannelList.LoadWithDuplicates(*activeList);
#else
    favourites.Load(*favFile);
    activeChannelList.Load(*activeList);
#endif


    globalFilters.ClearFilters();
    favouritesFilters.ClearFilters();

    memoryOn = false;

    /* start with fav  only if favourites is not empty*/
    if (favourites.Count() <= 0)
        savedWasLastMenuFav = false;


  return true;
}

void cPluginReelChannellist::Stop(void)
{
  // Stop any background activities the plugin is performing.
    favourites.Save();
}

void cPluginReelChannellist::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

void cPluginReelChannellist::MainThreadHook(void)
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginReelChannellist::Active(void)
{
  // Return a message string if shutdown should be postponed
  return NULL;
}

time_t cPluginReelChannellist::WakeupTime(void)
{
  // Return custom wakeup time for shutdown script
  return 0;
}

cOsdObject *cPluginReelChannellist::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
    globalFilters       = savedGlobalFilters;
    favouritesFilters   = savedFavouritesFilters;
    wasLastMenuFav      = savedWasLastMenuFav;

  return new cMenuChannelList;
}

cMenuSetupPage *cPluginReelChannellist::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return new cMenuReelChannelListSetup;
}

bool cPluginReelChannellist::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
    if (!Name) return false;
    printf("----- SetupParse('%s','%s')\n", Name, Value);

    if (strcmp(Name, "ChannelSortMode") == 0) {
           ChannelSortMode = eSortModes(atoi(Value));

    } else if(strstr(Name, "GlobalFilters")) {
        savedGlobalFilters.SetupParse(Name, Value);
    } else if(strstr(Name, "FavouriteFilters")) {
        savedFavouritesFilters.SetupParse(Name, Value);
    } else if (strcmp("OpenWithFavourites", Name)==0) {
        savedWasLastMenuFav = atoi(Value);
    } else
        return false;

    return true;

}

bool cPluginReelChannellist::Service(const char *Id, void *Data)
{
    // Handle custom service requests from other plugins
    if (Id && !strcmp(Id, "reload favourites list")) {
        LoadFavouritesList();
        return true;
    }
    
    if (!Id || !Data) return false;
    else if (Id && !strcmp(Id, "get favourites list")) {
        struct Data_t {cChannels* channels;};
        Data_t *d = (Data_t*)Data;
        d->channels = &favourites;
        return true;
    } else if (!strcmp(Id, "Next channel in direction:number")) {
        struct Data_t {int currCh; int dir; int result;};
        Data_t *d = (Data_t*)Data;
        d->result = NextAvailableChannel(d->currCh, d->dir);
        return true;
    } else if (!strcmp(Id, "Next channel in direction:pointer")) {
        struct Data_t {cChannel* channel; int dir; cChannel* result;};
        Data_t *d = (Data_t*)Data;
        d->result = NextAvailableChannel(d->channel, d->dir);
        return true;
    } else if (strcmp(Id, "osChannelsHook") == 0)
    {
        printf("ignoring osChannelsHook\n");
#if 0
        struct BouquetsData
        {
            eChannellistMode Function;      /*IN*/
            cOsdMenu *pResultMenu; /*OUT*/
        } *Data;
        Data = (BouquetsData *) data;
        Data->pResultMenu = new cMenuChannelList;
        return true;
#endif
    }
#if APIVERSNUM >= 10716
    else if (strcmp(Id, "MenuMainHook-V1.0") == 0)
    {
        // forget memory, if vdr is not in zoned-channel list mode
        if(!Setup.UseZonedChannelList) {
            savedGlobalFilters.ClearFilters();
            savedFavouritesFilters.ClearFilters();
            savedWasLastMenuFav = false;
        }
        openWithFavBouquets = false;

        // select the folder that contains the current channel here due to issue #647
        cChannel *bouquet = FavCurrentBouquet();
        cMenuFavourites::lastSelectedFolder = bouquet?bouquet->Name():"";

        MenuMainHook_Data_V1_0 *mmh = (MenuMainHook_Data_V1_0 *)Data;
        switch(mmh->Function)
        {
        case osBouquets: { // open list of favourite folders
            wasLastMenuFav = true;
            openWithFavBouquets = true;

            // select the folder that contains the current channel
            cChannel *bouquet = FavCurrentBouquet();
            cMenuFavourites::lastSelectedFolder = bouquet?bouquet->Name():"";

            favouritesFilters.ClearFilters();
        }
            break;

        case osChannels:
            printf("reelchannellist plugin : osChannels\n");

        case osActiveBouquet:
            globalFilters       = savedGlobalFilters;
            favouritesFilters   = savedFavouritesFilters;
            wasLastMenuFav      = savedWasLastMenuFav;
           break;

        case osAddFavourite: printf("\033[0;91mosAddFacvorites?\033[0m\n");
        {
            // Add current channel to favourites
            int chNumber = cDevice::PrimaryDevice()->CurrentChannel();
            cChannel *channel = Channels.GetByNumber(chNumber);
            std::vector<int> channels;
            channels.push_back(chNumber);
            if (channel)
                mmh->pResultMenu = new cMenuAddChannelToFavourites(channels, true); // standalone

            return channel;
        }
            break;
        case osFavourites: printf("osAddFacvorites?\n");
            globalFilters       = savedGlobalFilters;
            favouritesFilters   = savedFavouritesFilters;
            wasLastMenuFav = true;
            break;

        default     : return false;
        }


        /* open favourites list only if current channel can be shown when it is open
           else show "All Channels"
         */
        if (wasLastMenuFav &&  FavCurrentBouquet(true))
            mmh->pResultMenu = new cMenuFavourites(!openWithFavBouquets);
        else
            mmh->pResultMenu = new cMenuChannelList;

        return true;
    }
#endif
    else if (strcmp(Id, "favourites count")==0)
    {
        int *count = static_cast<int*>(Data);
        *count = favourites.Count();
        return true;
    }
    return false;
}

const char **cPluginReelChannellist::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
    static const char *HelpPages[] = {
  "LSTC [ :groups | <number> | <name> ]\n"
  "    List channels. Without option, all channels are listed. Otherwise\n"
  "    only the given channel is listed. If a name is given, all channels\n"
  "    containing the given string as part of their name are listed.\n"
  "    If ':groups' is given, all channels are listed including group\n"
  "    separators. The channel number of a group separator is always 0.",
        NULL
    };
    return HelpPages;    
}

cString cPluginReelChannellist::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  std::stringstream sdat;
  std::string sout;

  // Process SVDRP commands this plugin implements
  if (strcasecmp(Command, "LSTC") == 0)
  {
  
  bool WithGroupSeps = strcasecmp(Option, ":groups") == 0;
  if (*Option && !WithGroupSeps) {
     if (isnumber(Option)) {
        cChannel *channel = favourites.GetByNumber(strtol(Option, NULL, 10));
        if (channel)
	{
	   sdat << channel->Number() << " " <<  *channel->ToText();
           ReplyCode = 250;
	   return sdat.str().c_str();
	} else
	{  
           sdat << "Channel " << Option << " not defined";
           ReplyCode = 501;
	   return sdat.str().c_str();	   
        }
     }   
     else {
        cChannel *next = NULL;
        for (cChannel *channel = favourites.First(); channel; channel = favourites.Next(channel)) {
            if (!channel->GroupSep()) {
               if (strcasestr(channel->Name(), Option)) {
                  if (next)
		  {
		     sdat.str(""); sdat <<  next->Number() << " " << *next->ToText() << "\n";
                     sout += sdat.str();
		  }   
                  next = channel;
                  }
               }
            }
        if (next)
	{  
	    sdat.str(""); sdat <<  next->Number() << " " << *next->ToText() << "\n";
            sout += sdat.str();
	}    
        else
	{  
	    sdat.str(""); sdat <<  "Channel " << Option << " not defined" << "\n";
            sout += sdat.str();
	}   
     }
  }
  else if (favourites.MaxNumber() >= 1) {
     for (cChannel *channel = favourites.First(); channel; channel = favourites.Next(channel)) {
         if (WithGroupSeps)
	 { 
	    sdat.str(""); sdat << (channel->GroupSep() ? 0 : channel->Number()) << " " << *channel->ToText();
            sout += sdat.str();
	 }  
         else if (!channel->GroupSep())
	 { 
	    sdat.str(""); sdat << channel->Number() << " " << *channel->ToText();
            sout += sdat.str();	    
	 }  
         }
     }
  else
     return("No channels defined");
  }
  return sout.c_str();
}

VDRPLUGINCREATOR(cPluginReelChannellist); // Don't touch this!

