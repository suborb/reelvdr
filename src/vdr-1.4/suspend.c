
#include "debug.h"
#include "suspend.h"
#include "suspend.dat"
#include "plugin.h"
#include "status.h"
#include "tools.h"

/*
 * switch on/off video output On=1 or 0
 */
void ToggleVideoOutput(int On)
{
    /*
    cPlugin *plugin = cPluginManager::GetPlugin("reelbox");
    if (plugin)
    {
        plugin->Service("ToggleVideoOutput", &On);
    }
    */
    cPluginManager::CallAllServices("ToggleVideoOutput", &On);
}

cQuickShutdownLive::cQuickShutdownLive(void)
#if VDRVERSNUM >= 10300
:cPlayer(pmVideoOnly), cThread("QuickShutdown Mode: aka Quick shutdown mode")
#endif
{
}

cQuickShutdownLive::~cQuickShutdownLive()
{
    Detach();
}

void cQuickShutdownLive::Activate(bool On)
{
    PRINTF("Activate cQuickShutdownLive %d\n", On);
    if (On)
        Start();
    else
        Stop();
}

void cQuickShutdownLive::Stop(void)
{
    if (m_Active)
    {
        m_Active = false;
        Cancel(3);
    }
}

void cQuickShutdownLive::Action(void)
{
#if VDRVERSNUM < 10300
    isyslog("VDR: QuickShutdown Live thread started (pid = %d)", getpid());
#endif

    // give time for change in player before switching OFF the HDE output
    // else at times the Video frame remain on Screen
    cCondWait::SleepMs(250);

    // show any images HERE before blanking the screen
    // eg. Screen informing the user, it is "Quick-Shutdown"

    /*switch off hdplayer */
    ToggleVideoOutput(0);

    // Call external quickshutdown to prepare frontpanel etc.
    SystemExec("quickshutdown.sh start");

    m_Active = true;
    while (m_Active)
    {
        //DeviceStillPicture(suspend_mpg, sizeof(suspend_mpg));

        // this player does nothing, so sleep
        cCondWait::SleepMs(250);
    }

#if VDRVERSNUM < 10300
    isyslog("VDR: QuickShutdown Live thread stopped");
#endif
}

bool cQuickShutdownCtl::m_Active = false;
bool cQuickShutdownCtl::runScript = true;
int cQuickShutdownCtl::afterQuickShutdown = 0; // do nothing

cQuickShutdownCtl::cQuickShutdownCtl(void):cControl(m_QuickShutdown =
                                                    new cQuickShutdownLive)
{
    m_Active = true;
    cQuickShutdownCtl::runScript = true; // can be reset in cMenuShutdown

    /*switch off hdplayer */
    //ToggleVideoOutput(0);

    cStatus::MsgOsdClear();

    cPlugin *p = cPluginManager::GetPlugin("graphlcd");
    if (p) // Set graphlcd plugin inactive, so it does not update the LCD
        p->SetupParse("PluginActive", "0");

    p = cPluginManager::GetPlugin("osdteletext");
    if (p)
        p->Service("StopReceiver");
}

cQuickShutdownCtl::~cQuickShutdownCtl()
{
    m_Active = false;
    cStatus::MsgOsdClear();
    cPlugin *p = cPluginManager::GetPlugin("graphlcd");

    /* runScript==false means, vdr going into standby/shutdown,
     *  so donot switch the video and audio back
     */
    if (p && cQuickShutdownCtl::runScript) 
    {
        p->SetupParse("PluginActive", "1");

        //LCD
        //SystemExec("/sbin/reelfpctl -clearled 1");      // only standby-led
        //SystemExec("/sbin/reelfpctl -toptext \"\" -displaymode 0");
        SystemExec("quickshutdown.sh end" );
    
        /* since hdctrld -o DVI (in script) does not send 
         * audio through HDMI, reset settings in Reelbox-plugin
         */
        cPluginManager::CallAllServices("ReInit", NULL);


        // make sure volume is "on"
        cDevice::PrimaryDevice()->SetVolume(0); 

        //Get brightness set by user from graphlcd plugin and reset to that value
        int LCDbrightness = 8;
        p->Service("Get LCDBrightness Active", &LCDbrightness);

        char buffer[128];
        sprintf(buffer, "/sbin/reelfpctl -brightness %i", LCDbrightness);
        PRINTF("%s\n", buffer);
        SystemExec(buffer);
    }

    /*switch ON hdplayer */
    ToggleVideoOutput(1);

    p = cPluginManager::GetPlugin("osdteletext");

    if (p)
        p->Service("StartReceiver");


    DELETENULL(m_QuickShutdown);
}

eOSState cQuickShutdownCtl::ProcessKey(eKeys Key)
{
    if ((!m_QuickShutdown->IsActive() && !(Key == kNone)) || Key == kBack || Key == kOk
        || Key == kUp || Key == kDown || Key == kChanUp || Key == kChanDn
        || Key == kStop)
    {
        DELETENULL(m_QuickShutdown);
        return osEnd;
    }
    return osContinue;
}
