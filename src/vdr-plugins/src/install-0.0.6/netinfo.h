/*
 * csmenu.h channelscan menu part
 * writen by reel-multimedia
 *
 * See REDME for copyright information and
 * how to reach the authors.
 *
 */

#ifndef __NETINFO_H
#define __NETINFO_H


#include <string>
#include <vector>
#include <memory>
#include <net/if.h>             // don`t take <linux/if.h>
#include <cstring>

#define PATH_PROCNET_ARP     "/proc/net/arp"
#define PATH_PROCNET_DEV     "/proc/net/dev"
#define PATH_PROCNET_TCP     "/proc/net/tcp"
#define PATH_PROCNET_UDP     "/proc/net/udp"
#define PATH_PROCNET_UNIX    "/proc/net/unix"
#define PATH_PROCNET_ROUTE   "/proc/net/route"



/* pathname for the netlink device */
#define _PATH_DEV_ROUTE "/dev/route"

class cNetInterface
{
  public:
    cNetInterface(std::string ifname = "");
    const char *IfName() const;
    char *IpAddress(); /** the caller has to free the result! */
    const char *IpAddressv6();
    char *NetMask(); /** the caller has to free the result! */
    const char *HwAddress() const;
    bool IsUp() const;

  private:
    int hasIp_;
    bool isUp_;

    short flags_;               /** various flags   */
    short type_;                /** if type         */

    char hwaddr_[32];           /** HW address      */
    char name_[IFNAMSIZ];       /** interface name  */

    struct sockaddr addr_;      /** IP address      */
    char addr_v6_[128];         /** IPv6 address    */
    struct sockaddr netmask_;   /** IP network mask */
    //int metric;               /** routing metric  */

    //private functions
    char *IpAddr(sockaddr_in * sa); /** the caller has to free the result! */
    char *IpAddrv6(sockaddr_in6 * sa); /** the caller has to free the result! */
    int GetIfKernelConf();
};

class cNetInfo
{
  public:
    cNetInfo();
    ~cNetInfo();
    cNetInterface *operator[] (const int idx);
    int IfNum() const;
    const char *GetName(char *name, char *p) const;
    const char *GetWorkGroup() const;
    const char *GetHostName() const;

  private:
    int skfd_;
    std::vector < std::string > ifNames_;
    /// list of list interfaces to be probed 
    std::vector < cNetInterface * >netInterfaces_;
    ///  holds probed interface infomations 
    std::string procNetLine_;

    std::string workgroup_;
    std::string hostname_;

    //private functions
    bool ReadListProcNet(std::string & Name);
    void SetWorkGroup();
    void SetHostName();
};


// ---  inline  definitions ------

// ---  cNetInfo -----------------

inline int cNetInfo::IfNum() const
{
    return static_cast < int >(netInterfaces_.size());
}

inline const char *cNetInfo::GetWorkGroup() const
{
    return workgroup_.c_str();
}

inline const char *cNetInfo::GetHostName() const
{
    return hostname_.c_str();
}

// ---  cNetInterface -----------------
inline const char *cNetInterface::IfName() const
{
    return name_;
}

/** the caller has to free the result! */
inline char *cNetInterface::IpAddress()
{
    return IpAddr(reinterpret_cast < sockaddr_in * >(&addr_));
}

/** the caller has to free the result! */
inline const char *cNetInterface::IpAddressv6()
{
    return (const char *)&addr_v6_;
}

/** the caller has to free the result */
inline char *cNetInterface::NetMask()
{
    return IpAddr(reinterpret_cast < sockaddr_in * >(&netmask_));
}


#endif // __NETINFO_H
