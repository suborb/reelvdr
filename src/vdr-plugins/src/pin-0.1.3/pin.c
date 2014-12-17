/*
 * pin.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * Date: 11.04.05 - 23.11.06
 */

//***************************************************************************
// Includes
//***************************************************************************

#include <vdr/interface.h>
#include <vdr/videodir.h>
#include <vdr/menu.h>

#include "pin.h"
#include "menu.h"
#include "setupmenu.h"


//***************************************************************************
// Pin
//***************************************************************************
//***************************************************************************
// Object
//***************************************************************************

cPin::cPin(void)
{
   pinSetup = *cPinPlugin::pinCode ? no : yes;  // code already configured

   if (pinSetup)
      pinTxt = tr("Setup your pin code: ");
   else
      pinTxt = tr("Pin Code: ");

   isyslog("Debug: pin is '%s', text = '%s'", cPinPlugin::pinCode, pinTxt);

   display = 0;
   clear();
}

cPin::~cPin()
{
   if (display) delete display;
}

//***************************************************************************
// Clear
//***************************************************************************

void cPin::clear()
{
   *code = 0;
   cOsd::pinValid = no;
}

//***************************************************************************
// Show
//***************************************************************************

void cPin::Show(void)
{
   display = Skins.Current()->DisplayMessage();
   display->SetMessage(mtInfo, pinTxt);
   cStatus::MsgOsdStatusMessage(pinTxt);
   display->Flush();
}

//***************************************************************************
// Process Key
//***************************************************************************

eOSState cPin::ProcessKey(eKeys Key)
{
   char txt[50+TB];
   int len = strlen(code);
   eOSState state = cOsdObject::ProcessKey(Key);
  
   if (state == osUnknown)
   {
      switch (Key)
      {
         case kBack:
         {
            return osEnd;
         }

         case kLeft:
         {
            if (len <= 0)
               break;

            len--;
            code[len] = 0;
            break;
         }

         case k0 ... k9: 
         {
            if (len >= 5)           // max 5 digits
               break;
            
            code[len++] = Key-k0+48;
            code[len] = 0;
            break;
         }
         
         case kOk:
         {
            cOsd::pinValid = no;

            if (!pinSetup || (pinSetup && *code))
            {
               if (pinSetup)
                  strcpy(cPinPlugin::pinCode, code);   // setup pin code
               
               if (strcmp(code, cPinPlugin::pinCode) == 0)
                  cOsd::pinValid = yes;
            }

            state = osEnd;
            break;
         }

         default: return state;
      }
      
      if (state == osEnd)
      {
         if (pinSetup && cOsd::pinValid)
         {
            (cPinPlugin::getObject())->StorePin();
            display->SetMessage(mtInfo, tr("Pin code successfully setup"));
            cStatus::MsgOsdStatusMessage(tr("Pin code successfully setup"));
            display->Flush();
            sleep(2);
         }
            
	 if (cOsd::pinValid && cPinPlugin::autoMenuOpen)
         {
            cRemote::CallPlugin("pin");
         }
         else
         {
            display->SetMessage(cOsd::pinValid ? mtInfo : mtWarning,
                                cOsd::pinValid ? tr("Code accepted") : tr("Invalid Code !"));
            cStatus::MsgOsdStatusMessage(cOsd::pinValid ? tr("Code accepted") : tr("Invalid Code !"));
            display->Flush();
            sleep(1);
         }
      }
      else
      {
         int i;

         strcpy(txt, pinTxt);

         for (i = 0; i < len; i++)
            txt[strlen(pinTxt)+i] = '*';

         txt[strlen(pinTxt)+i] = 0;

         display->SetMessage(mtInfo, txt);
         cStatus::MsgOsdStatusMessage(txt);
         display->Flush();
         state = osContinue;
      }
   }
   
   return state;
}

//***************************************************************************
// PIN Plugin
//***************************************************************************
//***************************************************************************
// Static Stuff
//***************************************************************************

char cPinPlugin::pinCode[] = "";
cPinPlugin* cPinPlugin::object = 0;
bool cPinPlugin::skipChannelSilent = no;
int cPinPlugin::pinResetTime = 90 /* minutes */;         // default 1,5 hour
bool cPinPlugin::autoMenuOpen = yes;
int cPinPlugin::autoProtectionMode = apmIntelligent;
bool cPinPlugin::OneTimeScriptAuth = false;

const char* cPinPlugin::autoProtectionModes[] = 
{
   "always",
   "intelligent",
   "never",

   0
};

//***************************************************************************
// Object
//***************************************************************************

cPinPlugin::cPinPlugin(void)
 : lockedChannels(ltChannels),
   lockedBroadcasts(ltBroadcasts),
   lockedPlugins(ltPlugins)
{
   statusMonitor = 0;
   lastAction = time(0);
   receiver = 0;
   cOsd::pinValid = no;            // initial sperren
}

cPinPlugin::~cPinPlugin()
{
   if (receiver) delete receiver;
   if (statusMonitor) delete statusMonitor;
}

//***************************************************************************
// Store Pin Code
//***************************************************************************

void cPinPlugin::StorePin()
{
   SetupStore("pinCode", cPinPlugin::pinCode);
}

//***************************************************************************
// Initialize
//***************************************************************************

bool cPinPlugin::Initialize(void)
{
   char* path;

   asprintf(&path, "%s/%s", cPlugin::ConfigDirectory(), "pin");

   if (!DirectoryOk(path, no))
   {
      if (mkdir(path, 0770))
      {
         // cannot create the directory

         esyslog("Creating of directory '%s' failed, errno was (%d) - '%s'", 
                 path, errno, strerror(errno)); 
         free(path);

         return true; // anyhow let vdr start
      }
   }

   free(path);

   lockedChannels.Load(AddDirectory(cPlugin::ConfigDirectory(), 
                                    "pin/channellocks.conf"), true, false);
   lockedBroadcasts.Load(AddDirectory(cPlugin::ConfigDirectory(),
                                      "pin/broadcastlocks.conf"), true, false);
   lockedPlugins.Load(AddDirectory(cPlugin::ConfigDirectory(),
                                   "pin/pluginlocks.conf"), true, false);

   return true;
}

//***************************************************************************
// Main Menu Action
//***************************************************************************

cOsdObject* cPinPlugin::MainMenuAction(void)
{
   initPluginList();

   if (cOsd::pinValid)
      //return new cPinMenu(tr(MAINMENUENTRY), &lockedChannels, &lockedBroadcasts, &lockedPlugins);
      return new cPinMenu(tr("Title$Childlock"), &lockedChannels, &lockedBroadcasts, &lockedPlugins);

   return new cPin();
}

//***************************************************************************
// Start
//***************************************************************************

bool cPinPlugin::Start(void)
{
   statusMonitor = new cPinStatusMonitor;
   object = this;

   receiver = new MessageReceiver;
   receiver->StartReceiver();

   return true;
}

//***************************************************************************
// Stop
//***************************************************************************

void cPinPlugin::Stop(void)
{
   receiver->StopReceiver();
}

//***************************************************************************
// Setup Menu
//***************************************************************************

cMenuSetupPage* cPinPlugin::SetupMenu(void)
{
   if (cOsd::pinValid)
      return new PinSetupMenu;

   Skins.Message(mtInfo, tr("Please enter pin code first"));

   return 0;
}

//***************************************************************************
// Setup Parse
//***************************************************************************

bool cPinPlugin::SetupParse(const char *Name, const char *Value)
{
   // Pase Setup Parameters
   
//    if (!strcasecmp(Name, "pinCode"))
//    {
//       strcpy(pinCode, Value);

//       // Wenn PIN Code konfiguriert initial sperren 

//       cOsd::pinValid = *pinCode ? no : yes;
//    }

   if (!strcasecmp(Name, "pinCode"))
      strcpy(pinCode, Value);
   else if (!strcasecmp(Name, "skipChannelSilent"))
      skipChannelSilent = atoi(Value);
   else if (!strcasecmp(Name, "pinResetTime"))
      pinResetTime = atoi(Value);
   else if (!strcasecmp(Name, "autoMenuOpen"))
      autoMenuOpen = atoi(Value);
   else if (!strcasecmp(Name, "autoProtectionMode"))
      autoProtectionMode = atoi(Value);
   else
      return false;

   return true;
}

struct PinSubMenuStruct{ cOsdObject* subMenuPin;};

bool cPinPlugin::Service (const char*Id, void* Data)
{

	if( Id && strcmp(Id,"CheckPIN" )==0)
	{
		if (!Data) return false;

		const char *ptr = (const char*)Data;
		return strcmp(ptr, cPinPlugin::pinCode) == 0;
	}

	if( Id && strcmp(Id,"CheckPIN_and_ADD_OneTimeScriptAuth" )==0)
	{
		if (!Data) return false;

		const char *ptr = (const char*)Data;
		if( strcmp(ptr, cPinPlugin::pinCode) == 0)
		{
			// authentication for fskcheck "script" just for one time: 
			// see MessageReceiver::wait() @ msgreceiver.c
			OneTimeScriptAuth = true; 
			printf("(%s:%d) %s : Authenticated!!\n", __FILE__, __LINE__, Id);
			return true;
		}
		else 
		{
			printf("(%s:%d) %s : Authenticated!!\n", __FILE__, __LINE__, Id);
			return false;
		}
	}

	return false;
}

//***************************************************************************
// Test if Channel Protected
//***************************************************************************

int cPinPlugin::channelProtected(const char* name, long startTime)
{
   return lockedChannels.Locked(name, startTime);
}

//***************************************************************************
// Test if Plugin Protected
//***************************************************************************

int cPinPlugin::pluginProtected(const char* name)
{
   return lockedPlugins.Locked(name);
}

//***************************************************************************
// Test if Broadcast Protected
//***************************************************************************

int cPinPlugin::broadcastProtected(const char* title)
{
   return lockedBroadcasts.Locked(title);
}

//***************************************************************************
// Check Action Time
//***************************************************************************

void cPinPlugin::checkActivity()
{
   time_t lNow = time(0);

   if (pinResetTime > 0 && lNow > lastAction+(pinResetTime*60) && cOsd::pinValid)
   {
      // keine User Aktion, Kindersicherung aktivieren ..

      isyslog("Pin: Locking pin due to no user inactivity of (%ld) seconds",
              lNow-lastAction);

      cOsd::pinValid = no;
   }

   lastAction = lNow;
}

//***************************************************************************
// Init Plugin List
//***************************************************************************

int cPinPlugin::initPluginList()
{
   static int initialized = no;

   cPlugin* plugin;
   int i = 0;
   int cnt = 0;
   cLockItem* item;

   if (initialized)
      return done;

   initialized = yes;

#ifdef __EXCL_PLUGINS
   isyslog("exclude plugins '%s' from pin protection", __EXCL_PLUGINS);
#endif

   while ((plugin = cPluginManager::GetPlugin(i)))
   {
      if (strcasecmp(plugin->Name(), "pin") != 0
#ifdef __EXCL_PLUGINS
          && !strstr(__EXCL_PLUGINS, plugin->Name()))
#endif
      {
         // nicht das pin plugin und nicht in der Exclude Liste

         if (!(item = lockedPlugins.FindByName(plugin->Name())))
         {
            lockedPlugins.Add(new cLockItem(plugin->Name(), no, 
                                            plugin->MainMenuEntry()));

            isyslog("PIN added plugin (%d) '%s' - '%s'", 
                    i, plugin->Name(), plugin->MainMenuEntry());

            cnt++;
         }
         else
         {
            item->SetTitle(plugin->MainMenuEntry());
         }
      }

      i++;
   };

   if (cnt)
      lockedPlugins.Save();

   return done;
}

//***************************************************************************
// Channel Switch
//***************************************************************************

void cPinStatusMonitor::ChannelSwitch(const cDevice *Device, int ChannelNumber)
{
   // cChannel* channel = Channels.GetByNumber(ChannelNumber);
}

//***************************************************************************
// User Action
//***************************************************************************

void cPinStatusMonitor::UserAction(const eKeys key, const cOsdObject* Interact)
{
   #ifdef __DEBUG__
      dsyslog("Pin: UserAction, key (%d)", key);
   #endif

   (cPinPlugin::getObject())->checkActivity();
}

//***************************************************************************
// Replay Protected
//***************************************************************************

bool cPinStatusMonitor::ReplayProtected(const cRecording* Recording, const char* Name,
                                        const char* Base, bool isDirectory
#if VDRVERSNUM >= 10700
                                        ,int menuview
#endif
                                        )
{
   bool fProtected;
   struct stat ifo;
   char* path = 0;
   char* base = 0;
   char* name = 0;

   if (Base)
   {
      base = strdup(Base);
      base = ExchangeChars(base, true);
   }

   if (Name)
   {
      name = strdup(Name);

      if (isDirectory)
         name = ExchangeChars(name, true);
   }

   /*
   isyslog("checking protection of %s '%s%s%s'", 
           isDirectory ? "directory" : "recording", 
           isDirectory && base ? base : "",
           isDirectory && base ? "/" : "",
           isDirectory || !Recording ? name : Recording->Name());
   */

   if (cOsd::pinValid)
      return false;

   if (isDirectory)
      asprintf(&path, "%s%s%s/%s/%s", 
               VideoDirectory, 
               base ? "/" : "",
               base ? base : "",
               name ? name : "",
               PROTECTION_FILE);
   else
      asprintf(&path, "%s/%s", 
               Recording ? Recording->FileName() : name,
               PROTECTION_FILE);

   //isyslog("checking rights, protection file is '%s'", path);

   fProtected = stat(path, &ifo) == 0;

   if (path) free(path);
   if (base) free(base);
   if (name) free(name);

   if (fProtected)
   {
      //Skins.Message(mtError, tr("Recording protected, enter pin code first"));
      return true;
   }

   return false;
}

//***************************************************************************
// Channel Protected
//***************************************************************************

bool cPinStatusMonitor::ChannelProtected(const cDevice *Device, const cChannel* Channel)
{
   char* buf;
   const cEvent* event;

   if (cOsd::pinValid)
      return false;

   if (!Channel || !Channel->Name() || Channel->GroupSep())
      return false;

   // first check channel protection
/*
   isyslog("checking protection of channel '%s', '%s'", 
           Channel->Name(),
           Channel->ShortName());
*/
   if ((cPinPlugin::getObject())->channelProtected(Channel->Name()))
   {
      isyslog("channel %d '%s' is protected", Channel->Number(), Channel->Name());

      if (cPinPlugin::skipChannelSilent)
         return true;

      asprintf(&buf, "%d %s - %s", Channel->Number(), Channel->Name(), tr("channel is protected"));
      Skins.Message(mtInfo, buf);
      
      free(buf);

      return true;
   }

   // second check broadcast protection

   if (!(event = GetEventOf(Channel)))
      return false;

   if ((cPinPlugin::getObject())->broadcastProtected(event->Title()))
   {
      if (cPinPlugin::skipChannelSilent)
         return true;

      isyslog("broadcast '%s' is protected", event->Title());
      asprintf(&buf, "%s - %s", event->Title(), tr("broadcast is protected"));
      Skins.Message(mtInfo, buf);
      
      free(buf);

      return true;
   }

   return false;
}

//***************************************************************************
// Recording
//***************************************************************************

void cPinStatusMonitor::RecordingFile(const char* FileName) 
{
   char* path = 0;
   cRecordControl* rc;
   FILE* f;

   if (!FileName || !strlen(FileName))
      return;

   rc = cRecordControls::GetRecordControl(FileName);

   //isyslog("checking if autoprotection for '%s' is needed", FileName);

   if (rc && rc->Timer() && rc->Timer()->Channel())
   {
      if (rc->Timer()->FskProtection())
      {
         asprintf(&path, "%s/%s", FileName, PROTECTION_FILE);
         
         isyslog("autoprotecting recording due to '%s'; channel '%s'; file '%s'", 
                 rc->Timer()->FskProtection() ? "timer configuration" : "of protected channel", 
                 rc->Timer()->Channel()->Name(), path);

         f = fopen(path, "w");
         if (f) fclose(f);

         free(path);
      }
   }
}

//***************************************************************************
// Get Event Of
//***************************************************************************

const cEvent* cPinStatusMonitor::GetEventOf(const cChannel* Channel)
{
   cSchedulesLock schedLock;
   const cSchedules* scheds;
   const cSchedule *sched;
   const cEvent* event;

   if (!(scheds = cSchedules::Schedules(schedLock)))
      return 0;
   
   if (!(sched = scheds->GetSchedule(Channel->GetChannelID())))
      return 0;
   
   if (!(event = sched->GetPresentEvent()))
      return 0;

   return event;
}

//***************************************************************************
// Timer Creation
//***************************************************************************

void cPinStatusMonitor::TimerCreation(cTimer* Timer, const cEvent* Event)
{
   int fsk = 0;

   if (!Timer || !Event || !Timer->Channel())
      return;

   dsyslog("Timer creation, event '%s', protection mode (%d)", 
           Event->Title(), cPinPlugin::autoProtectionMode);
   
   switch (cPinPlugin::autoProtectionMode)
   {
      case cPinPlugin::apmAlways:  fsk = 1; break;
      case cPinPlugin::apmNever:   fsk = 0; break;

      case cPinPlugin::apmIntelligent:
      {
         //dsyslog("Checking protection for channel '%s' at (%ld))",
        //         Timer->Channel()->Name(), Event->StartTime());
                 
         fsk = (cPinPlugin::getObject())->channelProtected(
            Timer->Channel()->Name(), Event->StartTime())
            || (cPinPlugin::getObject())->broadcastProtected(Event->Title());

         break;
      }
   }

   Timer->SetFskProtection(fsk);
}

//***************************************************************************
// Plugin Protected
//***************************************************************************

bool cPinStatusMonitor::PluginProtected(cPlugin* Plugin, int menuView)
{
   char* buf;

   if (cOsd::pinValid)
      return false;

   if (!Plugin)
      return false;

   // check if plugin is protected

   //isyslog("checking protection of plugin '%s'",Plugin->MainMenuEntry());

   if ((cPinPlugin::getObject())->pluginProtected(Plugin->Name()))
   {
      //dsyslog("plugin '%s' is protected", Plugin->MainMenuEntry());

      asprintf(&buf, "%s %s", Plugin->MainMenuEntry(), tr("is protected"));
      Skins.Message(mtInfo, buf);
      free(buf);

      return true;
   }

   return false;
}

//***************************************************************************

VDRPLUGINCREATOR(cPinPlugin);
