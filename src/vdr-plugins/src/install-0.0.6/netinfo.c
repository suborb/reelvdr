
#include <iostream>
#include <fstream>
#include <sstream>


#include <sys/types.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>             //??
#include <ctype.h>              //??
#include <errno.h>              //??
#include <vector>
#include <string>
#include <memory>
#include "netinfo.h"

#include <vdr/i18n.h>
#include <vdr/tools.h>

using std::string;
using std::vector;
using std::ifstream;
using std::auto_ptr;

int skfd_ = -1;

// --- Class cNetInfo -----------------------------------------------

cNetInfo::cNetInfo()
{
    ifNames_.push_back("eth0");
    ifNames_.push_back("eth1");
    ifNames_.push_back("eth2");
    ifNames_.push_back("ath0");
    ifNames_.push_back("wlan0");
    ifNames_.push_back("ppp0");
#if ! defined(RBLITE) && ! defined(RBMINI) && defined(REELVDR)
    ifNames_.push_back("br0");
#endif

    for (int i = 0; i < static_cast < int >(ifNames_.size()); i++)
        ReadListProcNet(ifNames_[i]);

    SetWorkGroup();
    SetHostName();
}

cNetInfo::~cNetInfo()
{
}

cNetInterface *cNetInfo::operator[] (const int idx)
{
    return netInterfaces_[idx];
}

bool cNetInfo::ReadListProcNet(string & IfName)
{
    //dsyslog(" ReadListProcNet(%s)",IfName.c_str());
    //printf(" ReadListProcNet(%s)\n",IfName.c_str());
    FILE *fh;
    char buf[512];
    cNetInterface *ife;

    fh = fopen(PATH_PROCNET_DEV, "r");
    if (!fh)
    {
        esyslog("Warning: cannot open %s (s). Limited output.", PATH_PROCNET_DEV);
        return false;
    }

    fgets(buf, sizeof buf, fh); /* eat line */
    fgets(buf, sizeof buf, fh);

    while (fgets(buf, sizeof buf, fh))
    {
        char name[IFNAMSIZ];
        //search proc/net/dev vor ifname and save  the rest in procNetstr
        // for further treatments in feature
        procNetLine_ = GetName(name, buf);
        if (name == IfName)
        {
            netInterfaces_.push_back(ife = new cNetInterface(IfName));
            break;              // go out of here, there is no device with the same name
        }
    }
    fclose(fh);
    return true;
}

const char *cNetInfo::GetName(char *name, char *p) const
{
    while (isspace(*p))
        p++;

    while (*p)
    {
        if (isspace(*p))
            break;
        if (*p == ':')
        {
            /* could be an alias */
            char *dot = p, *dotname = name;
            *name++ = *p++;
            while (isdigit(*p))
                *name++ = *p++;

            if (*p != ':')
            {                   /* it wasn't, backup */
                p = dot;
                name = dotname;
            }
            if (*p == '\0')
                return NULL;

            p++;
            break;
        }
        *name++ = *p++;
    }
    *name++ = '\0';
    return p;
}

void cNetInfo::SetWorkGroup()
{

    ifstream inFile("/etc/samba/smb.conf");

    if (!inFile.is_open())
    {
        workgroup_ = tr("samba not installed!");
        esyslog("Setup: No /etc/samba/smb.conf found!");
    }

    string line, word;

    while (getline(inFile, line))
    {
        if (line.find("#") == 0)
            continue;

        std::istringstream splitStr(line);

        string::size_type pos = line.find("workgroup");

        if (pos != string::npos)
        {
            //std::cout << "Out line  " << line  << std::endl;
            string::size_type pos = line.find("=");
            line = line.substr(pos + 1);
            //std::cout << "Out pos \"" << workgroup_ << std::endl;
            while (line.find(" ") == 0)
                line = line.substr(1);
            workgroup_ = line;
            //std::cout << "Out String " << workgroup_ << std::endl;
        }
        if (workgroup_.empty())
            workgroup_ = tr("not set");
    }
}


void cNetInfo::SetHostName()
{
    char hname[64 + 1] = { 0 };
    gethostname(hname, sizeof(hname));
    hostname_ = hname;
}



cNetInterface::cNetInterface(string ifname_)
{
    memset(&netmask_, 0, sizeof(sockaddr)); // initialize netmask with 0.0.0.0
    strcpy(name_, ifname_.c_str());
    addr_v6_[0] = '\0';
    //cerr << "constr cNetInterface ifname: " << ifname_ << endl;
    isUp_ = false;
    GetIfKernelConf();
}

int cNetInterface::GetIfKernelConf()
{

    int fd;
    ifreq ifr;
    int errnum;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd >= 0)
    {

        strcpy(ifr.ifr_name, name_);
        if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0)
        {
            errnum = errno;
            esyslog("get no SIOCGIFFLAGS! fd: %d; errno: %d", fd, errnum);
            return (-1);
        }
        flags_ = ifr.ifr_flags;

        if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0)
            memset(hwaddr_, 0, 32);
        else
        {
            //dsyslog("got hwaddr");
            memcpy(hwaddr_, ifr.ifr_hwaddr.sa_data, 8);
        }

        strcpy(ifr.ifr_name, name_);
        ifr.ifr_addr.sa_family = AF_INET;
        if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
        {
            hasIp_ = 1;
            addr_ = ifr.ifr_addr;

            //fprintf(stderr,"got ip address\n");
            //sockaddr_in *a  = reinterpret_cast<sockaddr_in *>(&addr_);
            //printf("address %s\n",inet_ntoa(a->sin_addr));

            strcpy(ifr.ifr_name, name_);
            if (ioctl(fd, SIOCGIFNETMASK, &ifr) < 0)
                memset(&netmask_, 0, sizeof(sockaddr));
            else
                netmask_ = ifr.ifr_netmask;
        }
        else
            memset(&addr_, 0, sizeof(sockaddr));

        /** TB: there's no method to query the IPv6 address via ioctl?! */
        FILE *process;
        std::string cmd = std::string("LANG=C  cat /proc/net/if_inet6|grep ") + name_ + "$";
        const char *strBuff;

        process = popen(cmd.c_str(), "r");
        if (process)
        {
            cReadLine readline;
            strBuff = readline.Read(process);
            while (strBuff)
            {
                int ctr_strBuff = -1, ctr = -1;
                while (strBuff[++ctr_strBuff] != ' ')
                {
                    addr_v6_[++ctr] = strBuff[ctr_strBuff];
                    if (ctr > 0 && addr_v6_[ctr - 1] == ':' && addr_v6_[ctr] == '0')
                        ctr--;  // leading 0s should be left away
                    if (ctr_strBuff % 4 == 3)
                        addr_v6_[++ctr] = ':';
                    if (addr_v6_[ctr] == ':' && ctr > 2 && addr_v6_[ctr - 1] == ':' && addr_v6_[ctr - 2] == ':')
                        ctr--;  // avoid more than two ':'
                }
                addr_v6_[ctr] = '\0';

                int netmask = -1, n0;
                int ret = sscanf(strBuff + ctr_strBuff, " %x %x ", &n0, &netmask);
                if (ret == 2)
                {
                    std::stringstream buf;
                    buf << '/' << netmask;
                    strcat((char *)&addr_v6_, buf.str().c_str());
                }
                strBuff = readline.Read(process);
            }
            pclose(process);
        }

    }
    else
    {
        esyslog("ERROR - setup plugin: no valid address family found\n");
    }

    if (close(fd) < 0)
        esyslog(" can`t close socket! errno: %d\n", errno);

    return 0;
}

const char *cNetInterface::HwAddress() const
{

    char *inet_hw = MALLOC(char, 18);
    if (inet_hw)
        sprintf(inet_hw, "%02hX:%02hX:%02hX:%02hX:%02hX:%02hX", static_cast < unsigned char >(hwaddr_[0]), static_cast < unsigned char >(hwaddr_[1]), static_cast < unsigned char >(hwaddr_[2]), static_cast < unsigned char >(hwaddr_[3]), static_cast < unsigned char >(hwaddr_[4]), static_cast < unsigned char >(hwaddr_[5]));

    dsyslog("HWAddres:  %s\n", inet_hw);
    return inet_hw;
}

char *cNetInterface::IpAddr(sockaddr_in * sa)
{
    //printf("print IpAddres: \t");
    //printf("%s\n",inet_ntoa(sa->sin_addr));
    char *tmp = NULL;
    if (sa)
        asprintf(&tmp, "%s", inet_ntoa(sa->sin_addr));
    return tmp;
}

char *cNetInterface::IpAddrv6(sockaddr_in6 * sa)
{
    //printf("print IpAddresv6: \t");
    char *tmp = (char *)malloc(256);
    if (tmp)
        inet_ntop(AF_INET6, (const void *)sa, tmp, 256);
    //puts(tmp);
    return tmp;
}
