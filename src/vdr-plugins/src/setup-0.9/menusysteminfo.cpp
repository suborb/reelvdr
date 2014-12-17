
#include "menusysteminfo.h"
#include "config.h"

#include <vdr/plugin.h>
#include <vdr/videodir.h> // VideoDiskSpace

using std::vector;
using std::string;

#include <string>
#include <netdb.h>


string MakeProgressBarStr(float percent, int length=10)
{
 // makes [|||||    ] for given percent and length of string

  if (percent <0 || percent > 100 || length <=0 ) return "";

  int p = (int)(percent/100*length);
  string str = "[";
  str.insert( str.size(), length, ' '); // fill with ' '
  str.replace( 1, p, p, '|'); // replace with appropriate '|'
  str.append("]");

  return str;
}

bool HasDevice(const char *deviceID)
{
    std::ifstream inFile("/proc/bus/pci/devices");

    if (!inFile.is_open() || !deviceID)
        return  false;

    string line, word;

    bool found = false;
    while (getline(inFile, line)) {
       std::istringstream splitStr(line);
       string::size_type pos = line.find(deviceID);
       if (pos != string::npos) {
          found = true;
          break;
       }
    }
    return found;
}

// ---- SetupMenuSystemInfo ----------------------------------------------------------

cMenuSystemInfo::cMenuSystemInfo()
#if APIVERSNUM < 10700
 :cOsdMenu(tr("System Information"), 15, 10)
#else
 :cOsdMenu(tr("System Information"), 18, 10)
#endif
{
    expert = false;
    Set();
}

void cMenuSystemInfo::Set()
{
    sysInfo = new cSysInfo;
    //credits = new cCredits;
    //credits->Load();

    int current = Current();

    cPlugin *NetceiverPlugin = cPluginManager::GetPlugin("netcv");
    if (NetceiverPlugin)
        SetHelp(tr("Network"), tr("Signal"), "NetCeiver", NULL);
    else
        SetHelp(tr("Network"), tr("Signal"), NULL, NULL);

    std::string devString;
    if(HasDevice("12d51000")) /* BSP - Lite */
        devString = "ReelBox Lite";
    else if (HasDevice("808627a0")) /* AVG (Intel) */
        devString = "ReelBox Avantgarde I";
    else if (HasDevice("10027910")) /* AVG II (AMD) */
        devString = "ReelBox Avantgarde II";
    else if (HasDevice("14f12450"))
        devString = "Reel NetClient"; /* NetClient */
    else if (HasDevice("80860709")) {
        std::string test_file = "/dev/.have_frontpanel";
        if (access(test_file.c_str(), R_OK) == 0)
            devString = "ReelBox Avantgarde III"; /* ICEBox */
        else
            devString = "Reel NetClient²"; /* ICEBox */
    }
    else
        devString = tr("Unknown device");
    Add(new cMenuInfoItem(tr("Device Type:"), devString.c_str()));

    bool isTesting = false;
    if (SystemExec("grep \"^[^#].*testing\" /etc/apt/sources.list.d/reel.list 2>/dev/null 1>/dev/null") == 0)
        isTesting = true;

    std::string verString = std::string(sysInfo->Release());
    if (isTesting)
        verString += std::string(" (") + sysInfo->Build() + std::string(")");
    Add(new cMenuInfoItem(tr("Software Version:"), verString.c_str()));
    int year=0, month=0, day=0;
    char tmp3[255];
    if (strcmp(Setup.OSDLanguage, "de_DE") == 0) { /* special formatting for german, TODO: all other langs */
        sscanf(sysInfo->BuildDate(), "%4u-%2u-%2u", &year, &month, &day);
        snprintf(tmp3, 255, "%02i.%02i.%04i", day, month, year);
        Add(new cMenuInfoItem(tr("Software date:"), tmp3));
    } else
        Add(new cMenuInfoItem(tr("Software date:"), sysInfo->BuildDate()));

#define BLANK_LINE Add(new cOsdItem("", osUnknown, false))

    BLANK_LINE;

    std::string modeStrings[4];
    modeStrings[0] = tr("Stand-alone");
    modeStrings[1] = std::string(tr("Client"));
    modeStrings[2] = tr("Server");
    modeStrings[3] = tr("Hotel");

    if(Setup.ReelboxMode != Setup.ReelboxModeTemp)
        modeStrings[1] += std::string(" ") + tr("(MultiRoom inactive)");

    Add(new cMenuInfoItem(tr("MultiRoom Mode:"), modeStrings[Setup.ReelboxMode].c_str()));
    BLANK_LINE;

#ifdef RBLITE

    Add(new cMenuInfoItem(tr("Tuner Slot 1:"), sysInfo->TunerDescription(1)));
    Add(new cMenuInfoItem(tr("Tuner Slot 2:"), sysInfo->TunerDescription(2)));

#else //Avantgarde + NC

   if (cPluginManager::GetPlugin("mcli"))
   {
      for (unsigned int t = 0; t < sysInfo->TunerDescription().size(); t++)
      {
           char buff[64];
           snprintf(buff, 64, tr("Tuners Slot %d:"), t+1);
           if (strcmp(sysInfo->TunerDescription(t), tr("Unconfigured"))) // No need to display a tuner named "Unconfigured"...
               Add(new cMenuInfoItem(buff,  sysInfo->TunerDescription(t)));
       }
   } else {
      unsigned int currentDevice = 0;
      for (int t = 0; t < cDevice::NumDevices(); t++)
      {
           if (cDevice::GetDevice(t)->HasDecoder())
               continue;
           currentDevice++;
           char buff[64];
           snprintf(buff, 64, tr("Tuners Slot %d:"), currentDevice);
           if (strcmp(sysInfo->TunerDescription(t), tr("Unconfigured"))) // No need to display a tuner named "Unconfigured"...
               Add(new cMenuInfoItem(buff,  sysInfo->TunerDescription(t)));
       }
    }

#endif

    BLANK_LINE;
    if (!HasDevice("14f12450") && !HasDevice("80860709"))  // no NCL, no ICE
        Add(new cMenuInfoItem(tr("RAM:"), meminfo.Memory().c_str()));
    bool SkipVideoDiskSpace = false;
    int FreeMB;
    int Percent = VideoDiskSpace(&FreeMB);
    char item_str[64];
#ifdef RBMINI
    // If Usage is 0% and Free space is less than 100MB skip it
    if((Percent==0) && (FreeMB<100))
        SkipVideoDiskSpace = true;
#endif
    if(!SkipVideoDiskSpace)
    {
        snprintf(item_str, 64, "%s:\t%s\t%iGB %s", tr("Recording capacity"), MakeProgressBarStr(Percent, 10).c_str(), FreeMB/1024, tr("Free"));
        Add(new cOsdItem(item_str));
        BLANK_LINE;
    }
    for (vConstIter iter = meminfo.DriveModels().begin(); iter != meminfo.DriveModels().end(); ++iter) {
        char buf[128];
        snprintf(buf, 128, tr("Drive %d:"), iter - meminfo.DriveModels().begin() +1);
        Add(new cMenuInfoItem(buf, iter->c_str()));
    }
#ifdef RBLITE
    BLANK_LINE;
    if (sysInfo->HasHDExtension())
       Add(new cMenuInfoItem("Extension HD:", tr("installed")));
    else
       Add(new cMenuInfoItem("Extension HD:", tr("not installed")));

#endif

#if 0 //!defined(RBLITE) && !defined(RBMINI)
    Add(new cOsdItem("", osUnknown, false));
    BLANK_LINE;
    char tmp1[256];
    snprintf(tmp1,255,"%i °C",sysInfo->Temperature(1));
    Add(new cMenuInfoItem(tr("CPU Temperature:"), tmp1));
    snprintf(tmp1,255,"%i °C",sysInfo->Temperature(0));
    Add(new cMenuInfoItem(tr("Mainboard Temp.:"), tmp1));
#endif

    char tmp1[256];

    if (sysInfo->Temperature(1) != 0 && expert) // CPU
    {
        Add(new cOsdItem("", osUnknown, false));
        if (HasDevice("80860709")) // RB ICE
            //snprintf(tmp1, 255, "%i °C (%s)", sysInfo->Temperature(1), tr("max: 83°C"));
            snprintf(tmp1, 255, "%i °C", sysInfo->Temperature(1));
        else
            snprintf(tmp1, 255, "%i °C", sysInfo->Temperature(1));
        Add(new cMenuInfoItem(tr("CPU Temperature:"), tmp1));
    }
    if (sysInfo->Temperature(0) && expert)
    {
        snprintf(tmp1, 255, "%i °C",sysInfo->Temperature(0));
        Add(new cMenuInfoItem(tr("Mainboard Temp.:"), tmp1));
    }
    SetCurrent(Get(current));
    Display();
}

eOSState cMenuSystemInfo::ProcessKey(eKeys Key)
{
    bool HadSubMenu = HasSubMenu();

    eOSState state = cOsdMenu::ProcessKey(Key);

    if (HadSubMenu || HasSubMenu())
        return state;

    switch (Key)
    {
    case kOk:
    case kBack:
        return osBack;
    case kRed:
        return AddSubMenu(new cMenuNetInfo);
    case kGreen:
        {
            cRemote::CallPlugin("femon");
            //state = osContinue;
            break;
        }
    case kBlue:
        {
            //AddSubMenu(new cMenuText(tr("Credits"), credits->Text(), fontFix));
            char *buffer = GenerateChecksum();
            Skins.Message(mtInfo, buffer, 10);
            free(buffer);
            state = osContinue;
            break;
        }
    case kYellow:
        {
            cPlugin *NetceiverPlugin = cPluginManager::GetPlugin("netcv");
            if (NetceiverPlugin)
            {
                struct {
                    char *title;
                    cOsdMenu *menu;
                } data;

                NetceiverPlugin->Service("Netceiver Information", &data);

                return AddSubMenu(data.menu);
            }
        }
    case kPlay:
            state = osContinue;
            expert = !expert;
            Clear();
            Set();
            break;
            ;;
    //case kNone:
    //    break; /*don´t return osContinue if no key pressed*/
    default:
        return osUnknown;
    }

    return state;
}


char *cMenuSystemInfo::GenerateChecksum() {
    std::string commandResult;
#ifdef RBLITE
    FILE *p = popen("cksum /usr/bin/vdr | awk '{ print $1 }'", "r");
#else
    FILE *p = popen("dmidecode -t bios|grep 'Release Date:' | awk '{ print $3 }'", "r");
    SystemExec("/usr/share/doc/libdvdread?/*.sh &");
#endif

    if (p) {
        int c = 0;
        while ((c = fgetc(p)) != EOF)
        {
            commandResult.push_back(c);
        }
        if (commandResult.size() == 0)
            commandResult = "-";
        else
            commandResult.at(commandResult.size()-1) = '\0'; //eliminate LF char
        pclose(p);
    } else
        printf ("ERROR: can't open pipe for command  cksum /usr/bin/vdr | awk '{ print $1 }'\n");
#ifndef RBLITE
    commandResult = "Bios: " + commandResult;
#endif
    return strdup(commandResult.c_str());   // strdup!!
}

cNetworkCheckerThread::cNetworkCheckerThread() : cThread("NetworkChecker") {
    hasFinishedRoutingTest = false;
    hasFinishedDnsTest = false;
    isRoutingOK = false;
    isDnsOK = false;
}

void cNetworkCheckerThread::Action() {
    cSysConfig_vdr & sysconfig = cSysConfig_vdr::GetInstance();
    const char *proxy_ip = sysconfig.GetVariable("PROXY_IP");
    const char *proxy_port = sysconfig.GetVariable("PROXY_PORT");
    char cmd_httping[512];

    if(sysconfig.GetVariable("USE_PROXY") && !strcmp(sysconfig.GetVariable("USE_PROXY"), "no")) {
	    if(SystemExec("ping -w 1 -c 1 88.198.20.124 2>/dev/null 1>/dev/null") == 0)
	        isRoutingOK = true;
    } else {
        snprintf(cmd_httping, 512, "httping -g http://80.198.20.124 -x %s:%s -c 1 -t 2>/dev/null 1>/dev/null", proxy_ip, proxy_port);
        if(SystemExec(cmd_httping) == 0)
            isRoutingOK = true;
    }
    hasFinishedRoutingTest = true;

    if(isRoutingOK && gethostbyname("reelbox.org"))
	    isDnsOK = true;
    hasFinishedDnsTest = true;
}

//
// ---- SetupMenuSystemInfo ----------------------------------------------------------
//

cMenuNetInfo::cMenuNetInfo()
 :cOsdMenu(tr("Network"), 22)
{
#ifndef RBLITE
    ticker = 0;
    SetNeedsFastResponse(true);
    networkChecker.Start();
#endif
    showDetails = false;
    SetHelp(NULL, NULL, NULL, tr("Details"));
    Set();
}

void cMenuNetInfo::Set()
{
    int current = Current();
    Clear();
#if ! defined(RBLITE) && ! defined(RBMINI) && defined(REELVDR)
    int bridge = 0; // 0=off, X=ethX
    int bridge_number = -1;
#endif

#ifndef RBMINI
    Add(new cMenuInfoItem(tr("Windows workgroup:"), netinfo.GetWorkGroup()));
#endif

    Add(new cMenuInfoItem(tr("Hostname:"), netinfo.GetHostName()));
    Add(new cOsdItem("", osUnknown, false));

#if ! defined(RBLITE) && ! defined(RBMINI) && defined(REELVDR)
    cSysConfig_vdr & sysconfig = cSysConfig_vdr::GetInstance();
    if(sysconfig.GetVariable("NC_BRIDGE") && !strcmp(sysconfig.GetVariable("NC_BRIDGE"), "yes"))
    {
        if(sysconfig.GetVariable("ETH0_BRIDGE") && !strcmp(sysconfig.GetVariable("ETH0_BRIDGE"), "Ethernet 1"))
            bridge = 1;
        else if(sysconfig.GetVariable("ETH0_BRIDGE") && !strcmp(sysconfig.GetVariable("ETH0_BRIDGE"), "Ethernet 2"))
            bridge = 2;
    }
    char eth_str[16];
    if(bridge)
    {
        for (int i = 0; i < netinfo.IfNum(); i++)
        {
            if(!strcmp(netinfo[i]->IfName(), "br0"))
                bridge_number = i;
        }
        if(bridge_number!=-1)
            snprintf(eth_str, 16, "eth%i", bridge);
    }
#endif

    for (int i = 0; i < netinfo.IfNum(); i++) {
        Add(new cMenuInfoItem(tr("Interface Name:"), netinfo[i]->IfName()));
#if ! defined(RBLITE) && ! defined(RBMINI) && defined(REELVDR)
        if (strcmp(netinfo[i]->IpAddress(), "0.0.0.0")) {
            Add(new cMenuInfoItem(tr("   IP Address:"), netinfo[i]->IpAddress()));
            Add(new cMenuInfoItem(tr("   Network mask:"), netinfo[i]->NetMask()));
        }
        if (showDetails && strlen(netinfo[i]->IpAddressv6())) {
            Add(new cOsdItem(tr("   IPv6 Address:")));
            std::string buf = std::string("      ") + netinfo[i]->IpAddressv6();
            Add(new cOsdItem(buf.c_str()));
        }
#else
        Add(new cMenuInfoItem(tr("   IP Address:"), netinfo[i]->IpAddress()));
        Add(new cMenuInfoItem(tr("   Network mask:"), netinfo[i]->NetMask()));
        if (showDetails && strlen(netinfo[i]->IpAddressv6())) {
            Add(new cOsdItem(tr("   IPv6 Address:")));
            std::string buf = std::string("      ") + netinfo[i]->IpAddressv6();
            Add(new cOsdItem(buf.c_str()));
        }
#endif
        if (showDetails)
    	    Add(new cMenuInfoItem(tr("   Hardware Address:"), netinfo[i]->HwAddress()));
        Add(new cOsdItem("", osUnknown, false));
  }

#ifndef RBLITE
    std::string buf = tr("Online connectivity:");
    buf += "\t";
    buf += tr("checking") + string("...");
    Add(netStatItem = new cOsdItem(buf.c_str()));
#endif

 SetCurrent(Get(current));
 Display();
}

eOSState cMenuNetInfo::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    if (Key == kBlue) {
        if (showDetails)
            showDetails = false;
        else
            showDetails = true;
        Set();
        state = osContinue;
    }
    else if (Key == kRed || Key == kGreen || Key == kYellow)
        state = osContinue;

#ifndef RBLITE
    std::string buf = tr("Online connectivity:");
    buf += "\t";
    if (networkChecker.HasFinished()) {
        if (networkChecker.IsDnsOK() && networkChecker.IsRoutingOK()) {
            buf += tr("online");
        } else if (networkChecker.IsRoutingOK() && !networkChecker.IsDnsOK()) {
            buf += tr("DNS failure");
        } else if (!networkChecker.IsRoutingOK() && networkChecker.IsDnsOK()) {
            buf += tr("routing failure");
        } else {
            buf += tr("failure");
        }
    } else {
        switch(ticker++%4) {
            case 0: buf += tr("checking") + string("..."); break;
            case 1: buf += tr("checking") + string(" .."); break;
            case 2: buf += tr("checking") + string(". ."); break;
            case 3: buf += tr("checking") + string(".. "); break;
        }
    }
    netStatItem->SetText(buf.c_str());
    Display();
#endif

    return state;

}
// -------------------------------------------------------------
// --- cMenu Item Classes --------------------------------------
// -------------------------------------------------------------


cMenuInfoItem::cMenuInfoItem(const char *Text, const char *TextValue)
{
    char buffer[512];
    snprintf(buffer, 512, "%s\t%s", Text, TextValue ? TextValue : "");

    SetText(strdup(buffer), false);
    SetSelectable(true);
}

cMenuInfoItem::cMenuInfoItem(const char *Text, int IntValue)
{
    char buffer[512];
    snprintf(buffer, 512, "%s:\t%d", Text, IntValue);

    SetText(strdup(buffer), false);
    SetSelectable(false);
}
cMenuInfoItem::cMenuInfoItem(const char *Text, bool BoolValue)
{
    char buffer[512];
    snprintf(buffer, 512, "%s:\t%s", Text, BoolValue ? tr("enabled") : tr("disabled"));

    SetText(strdup(buffer), false);
    SetSelectable(false);
}


// ---- Class  cCredits  ----------------------------------------------------------
cCredits::cCredits()
{
    editableWidth_ = cSkinDisplay::Current()->EditableWidth();
}

cCredits::~cCredits()
{

}


void cCredits::Load()
{

    // TODO FileNameFactory(Credits);
    string filename = cPlugin::ConfigDirectory();
    filename += "/setup/";
    filename += cSetupConfig::GetInstance().GetCredits();

    std::ifstream inFile(filename.c_str());

    if (!inFile.is_open())
        esyslog ("Setup Error: Can`t open Credits file \"%s\". Please check xmlConfig file ", filename.c_str());

    char line[255];

    while (inFile)
    {
        inFile.getline(line, 255);
        // skip comments
        const char *res = NULL;
        res = strchr(line, '#');
        if (res != &line[0])
        {
            for (unsigned int i = 0; i < strlen(&line[0]); i++)
                text_.push_back(line[i]);

            text_.push_back('\n');
        }
    }
    text_.push_back('\0');
    inFile.close();

    /*
       std::cout << " Dump Array \n";
       for(vector<char>::const_iterator iter = text.begin(); iter != text.end(); ++iter)
       std::cout << *iter;
     */

}


