/*
 * sleeptimer.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include <vdr/interface.h>
#include <vdr/device.h>

static const char *VERSION       = "0.6";
static const char *DESCRIPTION   = trNOOP("Sleep-Timer for VDR");
static const char *MAINMENUENTRY = trNOOP("SleepTimer");
int multi = 15;
int max_minute = 360;
int sleepat = 0;
int Method = 0;
int Shutdown_Time = 0;
int Shutdown_Minutes = 0;
const char *Command = "/sbin/poweroff";

// Tools
int time_now()
{
    char buff[15];
    time_t tm;
    time(&tm);
    strftime(buff, sizeof(buff), "%s", localtime(&tm));
    return (atoi(buff));
}

int time_then(int minutes)
{
    int now;
    now = time_now();
    return (now + (minutes * 60));
}

int i2s(int tnow)
{
    time_t tme = time(NULL);
    struct tm *TM;
    char buf[15];
    int dif;
    int Std;
    int Min;

    TM = localtime(&tme);
    (*TM).tm_sec = 0;
    (*TM).tm_min = 0;
    (*TM).tm_hour = 0;
    strftime(buf, sizeof(buf), "%s", TM);
    dif = (tnow - atoi(buf));
    Std = dif / 3600;
    Min = ((dif - Std * 3600) / 60);
    return Std * 100 + Min;
}

int s2i(int &Min, int &Std, int Value)
{
    time_t tme = time(NULL);
    char buf[15];
    struct tm *TM;
    TM = localtime(&tme);
    Std = int (Value / 100);
    Min = ((Value - Std * 100) % 60);
    if (Std * 60 + Min < (*TM).tm_hour * 60 + (*TM).tm_min)
        (*TM).tm_mday++;
    (*TM).tm_sec = 0;
    (*TM).tm_min = 0;
    (*TM).tm_hour = 0;
    strftime(buf, sizeof(buf), "%s", TM);
    return atoi(buf) + Std * 3600 + Min * 60;
}

// cMenuSetupSleeptimer
class cMenuSetupSleeptimer:public cMenuSetupPage
{
  private:
    int newMethod;
  public:
      cMenuSetupSleeptimer(void);
    virtual void Store(void);
};

cMenuSetupSleeptimer::cMenuSetupSleeptimer(void)
{
    newMethod = Method;
    Add(new cMenuEditBoolItem(tr("Action"), &newMethod, tr("Shutdown"), tr("Mute")));
}

void cMenuSetupSleeptimer::Store(void)
{
    SetupStore("Method", Method = newMethod);
}


// cBackgroundSleepTimer
class cBackgroundSleepTimer:public cThread
{
  private:
    virtual void Action(void);
  public:
    cBackgroundSleepTimer(void);
     ~cBackgroundSleepTimer();
};

class MainMenu:public cOsdMenu
{
  public:
    MainMenu(void);
    eOSState ProcessKey(eKeys k);
    cBackgroundSleepTimer *dos;
};

cBackgroundSleepTimer::cBackgroundSleepTimer(void)
{
    Start();
}

cBackgroundSleepTimer::~cBackgroundSleepTimer()
{
    sleepat = 0;
}

void cBackgroundSleepTimer::Action(void)
{
    isyslog("sleeptimer: thread started (pid=%d)", getpid());
    while (sleepat)
    {
        if (sleepat <= time_now())
        {
            isyslog("sleeptimer: timeout");
            if (Method == 0)
            {
                isyslog("sleeptimer: executing \"%s\"", Command);
                if (SystemExec(Command) == -1)
                    isyslog("sleeptimer: errror while executing \"%s\"!",
                            Command);
            }
            if (Method == 1)
            {
                isyslog("sleeptimer: muting audio");
                if (!cDevice::PrimaryDevice()->IsMute())
                    cDevice::PrimaryDevice()->ToggleMute();
            }
            sleepat = 0;
        }
        else
        {
            if ((sleepat - 60) <= time_now())
            {
                // Donot make a call to osd from a background thread.
                //Interface->Confirm(tr("Turning off in about one minute"), 5, false);
                Skins.QueueMessage(mtInfo, tr("Turning off in about one minute"), 5, 5);
                isyslog("sleeptimer: Turning off in about one minute");
            }
        }
        if (sleepat)
            sleep(10);

    } //while

    // call with empty message to remove all messages belonging to this thread from message-queue
    Skins.QueueMessage(mtInfo, NULL);

    isyslog("sleeptimer: thread end (pid=%d)", getpid());
}

// cPluginSleeptimer
class cPluginSleeptimer:public cPlugin
{
  public:
    cPluginSleeptimer(void);
    virtual ~ cPluginSleeptimer();
    virtual const char *Version(void)
    {
        return VERSION;
    }
    virtual const char *Description(void)
    {
        return DESCRIPTION;
    }
    virtual const char *CommandLineHelp(void);
    virtual bool ProcessArgs(int argc, char *argv[]);
    virtual bool Start(void);
    virtual void Housekeeping(void);
    virtual const char *MainMenuEntry(void)
    {
        return MAINMENUENTRY;
    }
    virtual cOsdObject *MainMenuAction(void);
    virtual cMenuSetupPage *SetupMenu(void);
    virtual bool HasSetupOptions(void)
    {
        return false;
    }
    virtual bool SetupParse(const char *Name, const char *Value);
    //
};


cPluginSleeptimer::cPluginSleeptimer(void)
{
    // Initialize any member variables here.
    // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
    // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginSleeptimer::~cPluginSleeptimer()
{
    // Clean up after yourself!
}

const char *cPluginSleeptimer::CommandLineHelp(void)
{
    // Return a string that describes all known command line options.
    return "  -e command   shutdown command (default: /sbin/poweroff)\n";
}

bool cPluginSleeptimer::ProcessArgs(int argc, char *argv[])
{
    // Implement command line argument processing here if applicable.
    int c;
    while ((c = getopt(argc, argv, "e:")) != -1)
    {
        switch (c)
        {
        case 'e':
            Command = optarg;
            break;
        default:
            return false;
        }
    }
    return true;
}

bool cPluginSleeptimer::Start(void)
{
    return true;
}

void cPluginSleeptimer::Housekeeping(void)
{
}

cOsdObject *cPluginSleeptimer::MainMenuAction(void)
{
    return new MainMenu;
}

cMenuSetupPage *cPluginSleeptimer::SetupMenu(void)
{
    return new cMenuSetupSleeptimer;
}

bool cPluginSleeptimer::SetupParse(const char *Name, const char *Value)
{
    if (!strcasecmp(Name, "Method"))
        Method = atoi(Value);
    else
        return false;
    return true;
}

MainMenu::MainMenu(void):cOsdMenu(tr(MAINMENUENTRY), 28)
{

    if (sleepat)
    {
        Shutdown_Time = i2s(sleepat);
        if (Shutdown_Time >= 2400)
            Shutdown_Time -= 2400;      // bugfix +24
        Shutdown_Minutes = ((sleepat - time_now()) / 60);
    }
    else
    {
        Shutdown_Time = i2s(time_now());
        Shutdown_Minutes = 15;
    }
    // printf ("shutdown_time =%d\n",Shutdown_Time);
    Add(new cMenuEditIntItem(tr("Shutdown-Minutes"), &Shutdown_Minutes));
    Add(new cMenuEditTimeItem(hk(tr("Shutdown-Time")), &Shutdown_Time));
    Add(new cOsdItem(" ", osUnknown, false), false, NULL);  //blank line

    if (sleepat)
    {
        char buf[80];
        snprintf(buf, sizeof(buf), tr("Disable Shutdown in %d minutes"),
                 (sleepat - time_now()) / 60);
        Add(new cOsdItem(buf));
    }
    else
    {
        //snprintf(buf, sizeof(buf), "%s", );
        Add(new cOsdItem(tr("State:\tNot active"), osUnknown, false));  //non-selectable
    }
}

eOSState MainMenu::ProcessKey(eKeys k)
{
    int current;
    eOSState state = cOsdMenu::ProcessKey(k);
    switch (state)
    {
    case osUnknown:
        switch (k)
        {
        case kOk:
            current = Current();
            if (current == 3)
            {
                if (Interface->Confirm(tr("Abort Shutdown?")))
                {
                    sleepat = 0;
                    return (osPlugin);
                }
                else
                    return (osContinue);
            }
            else if (current == 1)
            {
                char buf[80];
                int tmp, Std, Min;
                tmp = s2i(Min, Std, Shutdown_Time);
                snprintf(buf, sizeof(buf),
                         tr("Shutdown at %i:%.2i?"), Std, Min);

                if (Interface->Confirm(buf))
                {
                    sleepat = tmp;
                    dos = new cBackgroundSleepTimer;
                    return (osEnd);
                }
            }
            else if (current == 0)
            {

                char buf[80];
                if (Shutdown_Minutes >= max_minute)
                    Shutdown_Minutes = max_minute;
                snprintf(buf, sizeof(buf),
                         tr("Shutdown in %d minutes?"),
                         Shutdown_Minutes);
                if (Interface->Confirm(buf))
                {
                    sleepat = time_then(Shutdown_Minutes);
                    dos = new cBackgroundSleepTimer;
                    return (osEnd);
                }
            }
            return (osPlugin);
            break;
        default:
            break;
        }
    case osBack:
    case osEnd:
        break;
    default:
        break;
    }
    return (state);
}

VDRPLUGINCREATOR(cPluginSleeptimer);    // Don't touch this!
