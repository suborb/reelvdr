/*
 * xmlepg2vdr.c: A plugin for the Video Disk Recorder
 *               A plugin to import EPG data from Internet into VDR
 *
 * See the README file for copyright information and how to reach the author.
 *
 *
 * $Id$
 */

#include <vdr/plugin.h>         // for cPlugin
#include <vdr/interface.h>      // for Interface->Confirm
#include <vdr/osdbase.h>        // for cOsdItem
#include <time.h>               // for time_t
#include "xmlepg2vdrthread.h"   // all work of plugin in cThreadXmlepg2vdr::Action()

static const char *VERSION = "0.0.1";
static const char *DESCRIPTION = trNOOP("A plugin to import EPG data into VDR");
static const char *MAINMENUENTRY = trNOOP("Load custom EPG");

cThreadXmlepg2vdr *threadXmlEpg; // from xmlepg2vdrthread.h

time_t lastRun;
#define FREQRUN (60*60*12)

/**** Add provider names here ******/

//#define NUMPROVIDER 2
const char *providerNames[] = {
  tr("United Kingdom (BBC+Sky)"),
  //tr("EPG from Internet"),
  //tr("EPG from Provider 1"),
  //tr("EPG from Provider 2"),
  NULL
};
 // Indices are used, so changing order of providers should be done with caution

class cProviderStatus
{
  public:
    string Name;
    int Enabled;
    int downloadManually;         // not saved by SetupStore() 
    cProviderStatus ();
};

cProviderStatus::cProviderStatus()
{
      Enabled = 0;
      downloadManually = 0;
}

std::vector < cProviderStatus > Providers;
/* list of providers
 * initiated in  cMenuSetupXmlepg2vdr::cMenuSetupXmlepg2vdr(void)
 */



/********** class cMenuSetupXmlepg2vdr ***********************/
class cMenuSetupXmlepg2vdr: public cMenuSetupPage
{
    private:
      std::vector < cProviderStatus > newProviders;
    protected:
      virtual void Store (void);
    public:
      cMenuSetupXmlepg2vdr (void);
      virtual eOSState ProcessKey (eKeys Key);
};

 // end class 

eOSState cMenuSetupXmlepg2vdr::ProcessKey (eKeys Key)
{
  char item_str[64];
  eOSState state = cOsdMenu::ProcessKey (Key); 
  //cMenuSetupPage::ProcessKey(Key); // avoiding extra "changes done" message
  // call Store() here as cMenuSetupPage::ProcessKey() is skipped where Store() is called
  const char * ItemText = Get (Current ())->Text ();

  std::vector < cProviderStatus >::iterator iter = newProviders.begin ();
  for (; iter != newProviders.end (); iter++) {
    //sprintf (item_str, "Get %s now", iter->Name.c_str ());
    sprintf (item_str, tr(" Retrieve EPG now"));
    if (!strcasecmp (ItemText, item_str)) {
      if (state == osUnknown) {
        switch (Key) {
          case kOk:
            iter->downloadManually = 1;
            // Store ();
            state = osContinue;
            break;
          default:
            break;
        }                     // end switch
      }                       // endif state
      break;                  // break out of for loop
    }                         // endif strcasestr
  }                           // end for

  if (Key == kOk) {
      Store(); // if kOk call Store() in anycase
  }

  return state;
}                             // end ProcessKey(eKeys)


cMenuSetupXmlepg2vdr::cMenuSetupXmlepg2vdr (void)
{
  SetTitle(tr("External BBC-SKY EPG"));
  SetCols(25);
  /* copy: make a tmp list */
  std::vector < cProviderStatus >::iterator iter = Providers.begin ();
  for (; iter != Providers.end (); iter++)
    newProviders.push_back (*iter);
  iter = newProviders.begin ();
  char *provStr = new char[32];
  for (; iter != newProviders.end (); iter++)
  {
    sprintf (provStr, "%s ", iter->Name.c_str ());
    Add (new cMenuEditBoolItem (provStr, &iter->Enabled, tr("Off"), tr("On")));
    sprintf (provStr, tr(" Retrieve EPG now"));
    Add (new cOsdItem (provStr));
    if (iter + 1 != newProviders.end ()) // no newline at the end
      Add (new cOsdItem ("  ")); // leave a line gap
  }
}                               // end constructor


void cMenuSetupXmlepg2vdr::Store (void)
{
  std::vector < cProviderStatus >::iterator iter = newProviders.begin (), iter2 = Providers.begin ();
 // esyslog("xepg: %s",__PRETTY_FUNCTION__);
  for (; iter != newProviders.end (); iter2++, iter++)
  {
    SetupStore (iter->Name.c_str (), iter->Enabled);
    iter2->Enabled = iter->Enabled; // Provider order should not change and no provider is added or deleted
    // not saving iter-> downloadManually

    
     /**Manual download threads here*/
    if (iter->downloadManually && !strcasecmp (iter->Name.c_str (), providerNames[0])) // Start thread here.
    {
      if (!threadXmlEpg->Active ())
      {
        /*
         * Start thread unconditionally -- no checking of lastRun/ enable
         */
        //esyslog("xmlepg2vdr: Manual download");
        Interface->Confirm (tr("Okay, Starting download in the background"), 1); // TODO change to Skins.Message(mtStatus, "message")
        lastRun = time (NULL);
        threadXmlEpg->Start ();
        SetupStore ("lastRun", lastRun);
        iter->downloadManually = 0;
      }                         // end if Active()
      else
      {
        Interface->Confirm (tr("Download already in progress!"), 1);
        // TODO change to Skins.Message(mtError, "message")
        // TODO should kill thread and start?
      }                         // end else

    }                           // endif downloadManually...
  }  // end for

}                               // end Store()

/*--------end class cMenuSetupXmlepg2vdr----------------------*/





/***************** class cPluginXmlepg2vdr *****************************
 * This class creates an object of cThreadXmlepg2vdr.
 * And initiates a thread by calling cThreadXmlepg2vdr::Start(). All work is done in this thread. 
 */
class cPluginXmlepg2vdr:public cPlugin
{

private:
  // Add any member variables or functions you may need here.
  int freqRun;                  // in Seconds. 
  // defines how often this thread runs 
  // Use Macro FREQRUN
public:

    cPluginXmlepg2vdr (void);
    virtual ~ cPluginXmlepg2vdr ();
  virtual const char *Version (void)
  {
    return VERSION;
  }
  virtual const char *Description (void)
  {
    return DESCRIPTION;
  }
  virtual const char *CommandLineHelp (void);
  virtual bool ProcessArgs (int argc, char *argv[]);
  virtual bool Initialize (void);
  virtual bool Start (void);
  virtual void Stop (void);
  virtual void Housekeeping (void);
  virtual cString Active (void);
  virtual const char *MainMenuEntry (void)
  {
    return tr(MAINMENUENTRY);       // needed for Setup Menu entry
  }
  virtual cOsdObject *MainMenuAction (void);
  virtual bool HasSetupOptions(void){ return false; } 
  virtual bool SetupParse (const char *Name, const char *Value);
};

cPluginXmlepg2vdr::cPluginXmlepg2vdr (void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
  
  //printf("cPluginXmlepg2vdr::cPluginXmlepg2vdr\n");
  
  threadXmlEpg = new cThreadXmlepg2vdr; // global object
  freqRun = FREQRUN;            // every  12 hours
  //lastRun = 1; // 1 sec after 00 hours 00 mins on 1st Jan 1970 UTC
  //lastRun got from vdr/setup.conf
  // see SetupParse()

  /*
   * initialize the Provider vector
   */
  cProviderStatus p;
  for (int i = 0; providerNames[i] != NULL; i++) // Change this number
  {
    //printf("Provider: %i\n", i);
    p.Name = providerNames[i];
    Providers.push_back (p);
  }

}

cPluginXmlepg2vdr::~cPluginXmlepg2vdr ()
{
  // Clean up after yourself!
  SetupStore ("lastRun", lastRun);

}

const char * cPluginXmlepg2vdr::CommandLineHelp (void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginXmlepg2vdr::ProcessArgs (int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginXmlepg2vdr::Initialize (void)
{
  // Initialize any background activities the plugin shall perform.

  //threadXmlEpg->setFreqRun (freqRun);
  return true;
}

bool cPluginXmlepg2vdr::Start (void)
{
  // Start any background activities the plugin shall perform.

  sleep (2);

  if (Providers[0].Enabled && !threadXmlEpg->Active () && lastRun + freqRun < time (NULL)) // Start thread only if enabled
  {
    threadXmlEpg->Start ();
    lastRun = time (NULL);
    SetupStore ("lastRun", lastRun);
  }

  /* 
   * Other provider's threads should be started here after a check
   */

  return true;
}


#if 0
cOsdObject * cPluginXmlepg2vdr::MainMenuAction (void)
{
// Perform the action when selected from the main VDR menu.
  bool ans;
  ans = Interface->Confirm ("Download EPG data now? (It could take a few minutes.) ", 5, false);

  if (ans)
  {
    if (!threadXmlEpg->Active ())
    {
      /*
       * Start thread unconditionally -- no checking of lastRun
       */
      Interface->Confirm ("Okay, Starting download.", 1); // TODO change to Skins.Message(mtStatus, "message")
      lastRun = time (NULL);
      threadXmlEpg->Start ();
    }
    else
    {
      Interface->Confirm ("Download already in progress!", 1);// TODO change to Skins.Message(mtError, "message")
      // TODO should kill thread and start?
    }
  }

  else
    Interface->Confirm ("Cancelled", 1);

  SetupStore ("lastRun", lastRun);
  return NULL;
} 
#endif

void cPluginXmlepg2vdr::Stop (void)
{
  // Stop any background activities the plugin shall perform.
  threadXmlEpg->killThread (0); // calls cThread::Cancel()
}


void cPluginXmlepg2vdr::Housekeeping (void)
{
  // Perform any cleanup or other regular tasks.
  if (!threadXmlEpg->Active () && Providers[0].Enabled)
  {
    if (lastRun + freqRun < time (NULL))
    {
      threadXmlEpg->Start ();
      lastRun = time (NULL);
      SetupStore ("lastRun", lastRun);
    }

  }                             // if Active()
}


cString cPluginXmlepg2vdr::Active (void)
{
  // Return a message string if shutdown should be postponed
  return NULL;
}

cOsdObject * cPluginXmlepg2vdr::MainMenuAction (void)

{
  // Return a setup menu in case the plugin supports one.
  return new cMenuSetupXmlepg2vdr;
}


bool cPluginXmlepg2vdr::SetupParse (const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  if (!strcasecmp ("lastRun", Name))
  {
    lastRun = (time_t) atof (Value);
    return true;
  }                             // end if

  std::vector < cProviderStatus >::iterator iter = Providers.begin ();
  for (; iter != Providers.end (); iter++)
  {
    if (!strcasecmp (Name, iter->Name.c_str ()))
    {
      iter->Enabled = atoi (Value);
      return true;
    }
  }                             // end for Provider loop

  return false;
}


VDRPLUGINCREATOR (cPluginXmlepg2vdr); // Don't touch this!
