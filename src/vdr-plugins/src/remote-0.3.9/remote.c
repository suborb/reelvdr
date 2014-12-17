/*
 * Remote Control plugin for the Video Disk Recorder
 *
 * remote.c: main source file
 *
 * See the README file for copyright information and how to reach the author.
 */


#include <vdr/plugin.h>
#ifdef REMOTE_FEATURE_LIRC
#include <vdr/lirc.h>
#endif
#include <stdio.h>
#include <sys/fcntl.h>
#include <getopt.h>
#include <termios.h>
#include <linux/input.h>
#include "remote.h"
#ifdef REMOTE_FEATURE_TCPIP
#include "remotetcp.h"
#endif
#include "ttystatus.h"

/* the vdr receive the remote's key presses by default, for now*/
bool disable_remote = false;

#if REELVDR // together with XBMC
    #define DISABLE_READ_INPUT_FROM_KERNEL 1
#endif

#define NUMREMOTES      10        // maximum number of remote control devices
 
#define KEYMAP_DEVICE_AV7110   "/proc/av7110_ir"

static const char *VERSION        = "0.3.9";
static const char *DESCRIPTION    = "Remote control";



// ---------------------------------------------------------------------------
cRemoteGeneric::cRemoteGeneric(const char *name, int f, char *d)
    :cRemote(name)
// ---------------------------------------------------------------------------
{
    fh = f;
    device = d;
    polldelay     = 40;   // ms
    repeatdelay   = 350;  // ms
    repeatfreq    = 30;  // ms
    repeattimeout = 500;  // ms
}


// ---------------------------------------------------------------------------
cRemoteGeneric::~cRemoteGeneric()
// ---------------------------------------------------------------------------
{
    Cancel();
}


// ---------------------------------------------------------------------------
bool cRemoteGeneric::Put(uint64_t Code, bool Repeat, bool Release)
// ---------------------------------------------------------------------------
{
    //DDD("Code %ld, Repeat %d, Release %d", Code, Repeat, Release);
    return cRemote::Put(Code, Repeat, Release);
}


// ---------------------------------------------------------------------------
void cRemoteGeneric::Action(void)
// ---------------------------------------------------------------------------
{
#if VDRVERSNUM <= 10317
    int now, first = 0, last = 0;
#else
    cTimeMs first, rate, timeout;
#endif
    uint64_t code, lastcode = INVALID_KEY;
    bool repeat = false;

    for (;;)
    {
        if (polldelay)
            usleep(1000*polldelay);

        code = getKey();
        if (code == INVALID_KEY)
        {
            if (!disable_remote) // when remote is disabled, do not complain
            {
		    esyslog("error reading '%s'", device);
		    close(fh);
		    fh=open(device, O_RDONLY);
            }
            else if (fh != -1 && disable_remote) 
            {
               close (fh);
               fh = -1;
            }
            usleep(100000*polldelay);
            continue;
        }

#if VDRVERSNUM <= 10317
        now = time_ms();
#endif

        if (keyPressed(code))
        {
            // key down
#if VDRVERSNUM <= 10317
            if (now - last > repeattimeout)
#else
            if (timeout.TimedOut())
#endif
            {
                if (repeat)
                {
                    Put(lastcode,false,true);
                    DSYSLOG("%s: timeout %016llx\n", device, code);
                    repeat = false;
                }
                lastcode = INVALID_KEY;
            }

            if (code != lastcode)
            {
                Put(code);
                DSYSLOG("%s: press %016llx\n", device, code);
                lastcode = code;
#if VDRVERSNUM <= 10317
                last = first = now;
#else
                first.Set(repeatdelay);
                rate.Set(repeatfreq);
                timeout.Set(repeattimeout);
#endif
                repeat = false;
            }
            else
            {
#if VDRVERSNUM <= 10317
                if (now - first < repeatdelay || now - last < repeatfreq)
#else
                if (!first.TimedOut() || !rate.TimedOut())
#endif
                    continue;
                Put(code,true);
                DSYSLOG("%s: repeat %016llx\n", device, code);
#if VDRVERSNUM <= 10317
                last = now;
#else
                rate.Set(repeatfreq);
                timeout.Set(repeattimeout);
#endif
                repeat = true;
            }
        }
        else
        {
            // key up
            if (repeat)
            {
                Put(lastcode,false,true);
                DSYSLOG("%s: release %016llx\n", device, lastcode);
                repeat = false;
            }
            lastcode = INVALID_KEY;
        }
    }/* for */
}


/*****************************************************************************/


// ---------------------------------------------------------------------------
//
// Try to identify input device.
//
// input:   fh - file handle
//          name - device name
//
// returns:  0 - unknown device
//           1 - full-featured card
//           2 - other DVB card, e.g. budget receiver
//          -1 - error
//
// ---------------------------------------------------------------------------
int identifyInputDevice(const int fh, char *name)
// ---------------------------------------------------------------------------
{
    char description[256];

    // check name of input device
    if (ioctl(fh, EVIOCGNAME(sizeof(description)), description) < 0)
        return -1;

    dsyslog("device %s: %s", name, description);

    if (!strcmp(description, "DVB on-card IR receiver"))
        return 1;

    if (strstr(description, "DVB") || strstr(description, "dvb"))
        return 2;

    return 0;
}


// ---------------------------------------------------------------------------
bool cRemoteDevInput::loadKeymap(const char *devname, uint32_t options)
// ---------------------------------------------------------------------------
{
    int fh;
    uint16_t keymap[2+256];
    int n;

    fh = open(devname, O_RDWR);
    if (fh < 0)
    {
        int err = errno;
        esyslog("%s: unable to open '%s': %s", Name(), devname, strerror(err));
        EOSD(tr("%s: %s"), devname, strerror(err));
        return false;
    }

    keymap[0] = (uint16_t) options;
    keymap[1] = (uint16_t) (options >> 16);

    for (int i = 1; i <= 256; i++)
        keymap[1+i] = i;

    n = write(fh, keymap, sizeof keymap);

    close(fh);

    if (n == sizeof keymap)
    {
        dsyslog("%s: keymap loaded '%s' flags %.8x", Name(), devname, options);
        return true;
    }
    else
    {
        esyslog("%s: error uploading keymap to '%s'", Name(), devname);
        MSG_ERROR(tr("Error uploading keymap"));
        return false;
    }
}


// ---------------------------------------------------------------------------
cRemoteDevInput::cRemoteDevInput(const char *name, int f, char *d)
    :cRemoteGeneric(name, f, d)
// ---------------------------------------------------------------------------
{
    testMode = false;

    Start();

    // autorepeat
#define BITS_PER_LONG	(sizeof(unsigned long) * 8)
    unsigned long data[EV_MAX];
    memset(data, 0, sizeof data);
    ioctl(f, EVIOCGBIT(0,EV_MAX), data); 
    if ( data[EV_REP/BITS_PER_LONG] & (1 << EV_REP%BITS_PER_LONG) )
    {
        // autorepeat driver
        dsyslog("%s: autorepeat supported", name);
        polldelay = 0;
    }
    else
    {
        // non-autorepeat drivers
        polldelay = repeatdelay = repeatfreq = repeattimeout = 0;
    }

    // grab device if possible (kernel 2.6)
#ifndef EVIOCGRAB
    // required if an old /usr/include/linux/input.h is used with a new kernel :-(
#define EVIOCGRAB  _IOW('E', 0x90, int)   /* Grab/Release device */
#endif
    data[0] = 1;
    if (ioctl(f, EVIOCGRAB, data) == 0)
        dsyslog("%s: exclusive access granted", name);

    // setup keymap
    const char *setupStr = GetSetup();
    char kDevname[256]; memset(kDevname, 0, sizeof kDevname);
    uint32_t kOptions = 0;
    int kAddr = -1;

    if (setupStr)
    {
        sscanf(setupStr, "%s %x %d", kDevname, &kOptions, &kAddr);
        if (kAddr != -1)
            kOptions |= ((kAddr << 16) | 0x4000);
    }

    if (kDevname[0])
    {    
        loadKeymap(kDevname, kOptions);
    }
}


// ---------------------------------------------------------------------------
bool cRemoteDevInput::Initialize()
// ---------------------------------------------------------------------------
{
    testMode = true;

    char setupStr[256]; memset (setupStr, 0, sizeof setupStr);
		
    // load keymap for full-featured cards
    if (identifyInputDevice(fh, device) == 1)
    {
        const char *kDevname = "/proc/av7110_ir";
        uint32_t kOptions;
        int kAddr = -1;
        int i, n;

	for (n = 0; n < 2; n++)
        {
            if (n == 0)
            {
                MSG_INFO(tr("Press any key to use pre-loaded keymap"));
                for (testKey = 0, i = 0; testKey == 0 && i < 35; i++)
                    usleep(200000);
                if (testKey != 0)
                {
                    MSG_INFO(tr("User-supplied keymap will be used"));
                    break;
                }
            }

            kOptions = 0x0000;
            MSG_INFO(tr("Remote control test - press and hold down any key"));
            loadKeymap(kDevname, kOptions);
            for (testKey = 0, i = 0; testKey == 0 && i < 10; i++)
                usleep(200000);
            if (testKey != 0)
            {
                for (i = 0; i < 64; i++)
                {
                    int a = (i & 0x1f);
                    loadKeymap(kDevname, kOptions | 0x4000 | (a << 16));
                    usleep(400000);
                    testKey = 0;
                    usleep(400000);
                    if (testKey != 0)
                    {
                        kAddr = a;
			break;
		    }
		}
                MSG_INFO(tr("RC5 protocol detected"));
                sprintf (setupStr, "%s %.8x %d", kDevname, kOptions, kAddr);
                break;
            }

            kOptions = 0x8000;
            loadKeymap(kDevname, kOptions);
            for (testKey = 0, i = 0; testKey == 0 && i < 10; i++)
                usleep(200000);
            if (testKey != 0)
            {
                for (i = 0; i < 64; i++)
                {
                    int a = (i & 0x1f);
                    loadKeymap(kDevname, kOptions | 0x4000 | (a << 16));
                    usleep(400000);
                    testKey = 0;
                    usleep(400000);
                    if (testKey != 0)
                    {
                        kAddr = a;
                        break;
                    }
                }
                MSG_INFO(tr("RC5 protocol detected (inverted signal)"));
                sprintf (setupStr, "%s %.8x %d", kDevname, kOptions, kAddr);
                break;
            }

            kOptions = 0x0001;
            loadKeymap(kDevname, kOptions);
            for (testKey = 0, i = 0; testKey == 0 && i < 10; i++)
                usleep(200000);
            if (testKey != 0)
            {
                MSG_INFO(tr("RCMM protocol detected"));
                sprintf (setupStr, "%s %.8x %d", kDevname, kOptions, kAddr);
                break;
            }

            kOptions = 0x8001;
            loadKeymap(kDevname, kOptions);
            for (testKey = 0, i = 0; testKey == 0 && i < 10; i++)
                usleep(200000);
            if (testKey != 0)
            {
                MSG_INFO(tr("RCMM protocol detected (inverted signal)"));
                sprintf (setupStr, "%s %.8x %d", kDevname, kOptions, kAddr);
                break;
            }
        }/* for */

        if (testKey == 0)
        {
            MSG_ERROR(tr("No remote control detected"));
            esyslog("%s: no remote control detected", device);
            usleep(5000000);
            testMode = false;
            return false;
        }
    }/* DVB card */

    if (setupStr[0])
        PutSetup(setupStr);

    testMode = false;
    return true;
}


// ---------------------------------------------------------------------------
uint64_t cRemoteDevInput::getKey(void)
// ---------------------------------------------------------------------------
{
    struct input_event ev;
    int n;
    uint64_t code;

#if DISABLE_READ_INPUT_FROM_KERNEL
    if (!disable_remote) {
#endif
        int data = 1;
        if (ioctl(fh, EVIOCGRAB, &data) == 0)
            dsyslog("exclusive access granted");
        
    do
        n = read(fh, &ev, sizeof ev);
    while (n == sizeof ev && ev.type != 1);

#if DISABLE_READ_INPUT_FROM_KERNEL
    } // if (!disable_remote)
    
    if (disable_remote){
        /* setting n=0 sets code=INVALID_KEY
        ** If code == INVALID_KEY the Action() loop assumes error with devices
        ** and closes & opens the device. In the process it looses the exclusive grab#
        ** on the device, enabling X to receive inputs! TODO: make this cleaner.
        **/
        n = 0; // != sizeof(struct input_event)
        
        int data = 0;
        if (ioctl(fh, EVIOCGRAB, &data) == 0 && !disable_remote) // when remote is disabled dont log 
            dsyslog("exclusive access given up");
        
    }
#endif
    
    if (n == sizeof ev)
    {
        if (ev.value)
            ev.value = 1;
        code = ((uint64_t)ev.value << 32) | ((uint64_t)ev.type << 16) | (uint64_t)ev.code;
    }
    else
        code = INVALID_KEY;

    if (testMode)
    {
        testKey = code;
        code = 0ULL;
    }

    return code;
}


// ---------------------------------------------------------------------------
bool cRemoteDevInput::keyPressed(uint64_t code)
// ---------------------------------------------------------------------------
{
    return (code & 0xFFFF00000000ULL);
}


/*****************************************************************************/


#ifdef REMOTE_FEATURE_LIRCOLD
// ---------------------------------------------------------------------------
uint64_t cRemoteDevLirc::getKey(void)
// ---------------------------------------------------------------------------
{
    unsigned long code;
    int n;

    n = read(fh, &code, sizeof code);
    if (n != sizeof code)
        return INVALID_KEY;
    else
        return (uint64_t)code;
}


// ---------------------------------------------------------------------------
bool cRemoteDevLirc::keyPressed(uint64_t code)
// ---------------------------------------------------------------------------
{
    return (code & 0x80);
}
#endif // REMOTE_FEATURE_LIRCOLD


/*****************************************************************************/


// ---------------------------------------------------------------------------
cRemoteDevTty::cRemoteDevTty(const char *name, int f, char *d)
    :cRemoteGeneric(name, f, d)
// ---------------------------------------------------------------------------
{
    struct termios t;
    has_tm=false;
    if (!tcgetattr(f, &tm))
    {
        has_tm=true;
        t = tm;   
        t.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(f, TCSANOW, &t);
    }
    polldelay     = 0;
    repeattimeout = 80;
    if (f >= 0)
        Start();
}


// ---------------------------------------------------------------------------
cRemoteDevTty::~cRemoteDevTty()
// ---------------------------------------------------------------------------
{
    if(has_tm) tcsetattr(fh, TCSANOW, &tm);
}


// ---------------------------------------------------------------------------
uint64_t cRemoteDevTty::getKey(void)
// ---------------------------------------------------------------------------
{
    int n;
    uint64_t code = 0;

    n = read(fh, &code, sizeof code);
    return (n > 0) ? code : INVALID_KEY;
}


// ---------------------------------------------------------------------------
bool cRemoteDevTty::keyPressed(uint64_t code)
// ---------------------------------------------------------------------------
{
    return true;
}


// ---------------------------------------------------------------------------
bool cRemoteDevTty::Put(uint64_t Code, bool Repeat, bool Release)
// ---------------------------------------------------------------------------
{
    bool rc = cRemote::Put(Code, Repeat, Release);

    if (!rc && Code <= 0xff)
        rc = cRemote::Put(KBDKEY(Code));

    return rc;
}



/*****************************************************************************/
class cPluginRemote : public cPlugin
/*****************************************************************************/
{
private:
  int  devcnt;
  char devtyp[NUMREMOTES];
  char *devnam[NUMREMOTES];
  int  fh[NUMREMOTES];
public:
  cPluginRemote(void);
  virtual ~cPluginRemote();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Start(void);
  virtual bool HasSetupOptions(void)
  {
	return false;
  }
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  virtual bool Service(const char *Id, void *Data);
};

bool cPluginRemote::Service(const char *Id, void *Data)
{
    if (!Id) return false;
    
    if (strcmp(Id, "switch to xbmc")==0) 
        return Service("remote off", NULL);
    else if (strcmp(Id, "remote off")==0) 
    {
        disable_remote = true;
    } 
    else 
        return false;
    
    return true;
}

const char** cPluginRemote::SVDRPHelpPages()
{
    static const char* HelpPage[] = { 
        "DISR\n",
        "   Disable Remote",
        "ENBR\n",
        "   Enable remote",
        NULL
    };
    
    return HelpPage;
}

cString cPluginRemote::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    cString status;
    
    if (strncmp(Command, "DISR", 4)==0)   {
        disable_remote = true;
        status = "remote disabled.";
    }
    else if (strncmp(Command, "ENBR", 4)==0) {
        disable_remote = false;
        status = "remote enabled.";
    }
    else  {
        status = "unhandled";
    }
    
    return status;
}

// ---------------------------------------------------------------------------
cPluginRemote::cPluginRemote(void)
// ---------------------------------------------------------------------------
{
    for (int i = 0; i < NUMREMOTES; i++)
    {
        devtyp[i] = '\0';
        devnam[i] = NULL;
        fh[i] = -1;
    }
    devcnt = 0;
}


// ---------------------------------------------------------------------------
cPluginRemote::~cPluginRemote()
// ---------------------------------------------------------------------------
{
    // must not delete any remotes, see PLUGINS.html!

    for (int i = 0; i < devcnt; i++)
    {
        if (fh[i] >= 0)
            close(fh[i]);
        fh[i] = -1;
    }
    devcnt = 0;
}


// ---------------------------------------------------------------------------
const char *cPluginRemote::CommandLineHelp(void)
// ---------------------------------------------------------------------------
{
    return "  -i dev,   --input=dev    kernel input device (/dev/input/...)\n"
#ifdef REMOTE_FEATURE_LIRC
           "  -l dev,   --lirc=dev     lirc device (/dev/lircd)\n"
#endif
#ifdef REMOTE_FEATURE_LIRCOLD
           "  -l dev,   --lirc=dev     kernel lirc device (/dev/lirc)\n"
#endif
#ifdef REMOTE_FEATURE_TCPIP
           "  -p tcp:n, --port=tcp:n   listen on tcp port <n>\n"
#endif
           "  -t dev,   --tty=dev      tty device\n"
           "  -T dev,   --TTY=dev      tty device with 'OSD'\n";
}


// ---------------------------------------------------------------------------
bool cPluginRemote::ProcessArgs(int argc, char *argv[])
// ---------------------------------------------------------------------------
{
    static struct option long_options[] =
            { { "input", required_argument, NULL, 'i' },
              { "lirc",  required_argument, NULL, 'l' },
              { "port",  required_argument, NULL, 'p' },
              { "tty",   required_argument, NULL, 't' },
              { "TTY",   required_argument, NULL, 'T' },
              { NULL } };
    int c;

    while ((c = getopt_long(argc, argv, "i:l:p:t:T:", long_options, NULL)) != -1)
    {
        switch (c)
        {
          case 'i':
          case 'l':
          case 'p':
          case 't':
          case 'T':
              if (devcnt >= NUMREMOTES)
              {
                  esyslog("%s: too many remotes", Name());
                  return false;
              }
              devtyp[devcnt] = c;
              devnam[devcnt] = optarg;
	      devcnt++;
              break;

          default:
              esyslog("%s: invalid argument", Name());
              return false;
        }
    }

    return true;
}


// ---------------------------------------------------------------------------
bool cPluginRemote::Start(void)
// ---------------------------------------------------------------------------
{
    bool ok = false;

    // no device specified by the user, set default
    if (devcnt == 0)
    {
        devtyp[0] = 'i';
        devnam[0] = (char*)"autodetect";
        devcnt = 1;
    }

    /* probe eventX devices */
    for (int i = 0; i < devcnt; i++)
    {
        if (devtyp[i] == 'i' && strcmp(devnam[i], "autodetect") == 0)
        {
            char nam[80];

            for (int j = 0; ; j++)
            {
                sprintf(nam, "/dev/input/event%d", j);
                fh[i] = open(nam, O_RDONLY);
                if (fh[i] < 0)
                {
                    switch (errno)
                    {
                        case EACCES:   // permission denied: ignore device
                            continue;
                        default:       // no more devices: stop scanning
                            break;
                    }
                    break;
                }
		
                if (identifyInputDevice(fh[i], nam) >= 1)
                {
                    // found DVB card receiver
                    devnam[i] = strdup(nam);
                    close(fh[i]);
                    break;
                }

                // unknown device, try next one
                close(fh[i]);
            } // for j
        } // if autodetect

        // use default device if nothing could be identified
        if (devtyp[i] == 'i' && strcmp(devnam[i], "autodetect") == 0)
            devnam[i] = (char*)"/dev/input/event0";
    } // for i

    for (int i = 0; i < devcnt; i++)
    {
        switch (devtyp[i])
        {
#ifdef REMOTE_FEATURE_LIRC
            case 'l':
                fh[i] = access(devnam[i], R_OK);
                break;
#endif
            case 'p':
                fh[i] = 0;
                break;

            case 'T':
                fh[i] = open(devnam[i], O_RDWR);
                break;

            default:
                fh[i] = open(devnam[i], O_RDONLY);
                break;
        }

        if (fh[i] < 0)
        {
            esyslog("%s: unable to open '%s': %s",
                    Name(), devnam[i], strerror(errno));
            EOSD(tr("%s: %s"), devnam[i], strerror(errno));
            continue;
        }
	
        // at least, one device opened successfully
        ok = true;
        dsyslog("%s: using '%s'", Name(), devnam[i]);

        // build name for remote.conf
        char nam[25];
        char *cp = strrchr(devnam[i], '/');
	if (cp)
            sprintf (nam, "%s-%s", Name(), cp+1);
        else
            sprintf (nam, "%s-%s", Name(), devnam[i]);

	switch (devtyp[i])
        {
            case 'i':
                new cRemoteDevInput(nam,fh[i],devnam[i]);
                break;

#ifdef REMOTE_FEATURE_LIRC
            case 'l':
                new cLircRemote(devnam[i]); // use vdr's lirc code
                break;
#endif
#ifdef REMOTE_FEATURE_LIRCOLD
            case 'l':
                new cRemoteDevLirc(nam,fh[i],devnam[i]);
                break;
#endif

#ifdef REMOTE_FEATURE_TCPIP
            case 'p':
	        fh[i] = -1; // don't close handle 0 at destructor
                new cTcpRemote(nam, -1, devnam[i]);
                break;
#endif
            case 'T':
                new cTtyStatus(fh[i]);
                // fall thru
            case 't':
                new cRemoteDevTty(nam,fh[i],devnam[i]);
                break;
        }
    } // for
    
    if (!ok)
        esyslog("%s: fatal error - unable to open input device", Name());

    return ok;
}


/*****************************************************************************/


VDRPLUGINCREATOR(cPluginRemote); // Don't touch this!
