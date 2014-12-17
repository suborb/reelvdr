/********************************************************************************
* systeminfo.h about menu part
* writen by markus Hahn @ reel-multimedia
*
* See REDME for copyright information and
* how to reach the authors.
*
********************************************************************************/

#ifndef __SYSINFO_H
#define __SYSINFO_H


#include <string>
#include <vector>
#include <memory>
//#include <vdr/diseqc.h>
#include <vdr/device.h>


class cSysInfo
{
  public:
    cSysInfo();
    ~cSysInfo();
    const char *TunerDescription(int TunerNum) const;
    std::vector<std::string> TunerDescription() const { return tunerDescription_; };
    const char *Distribution() const;
    const char *Build() const;
    const char *BuildDate() const;
    const char *Version() const;
    const char *Release() const;
    bool HasHDExtension() const;
    const int Temperature(int mode) const;

  private:
    int fd_;
    int id1_;
    int id2_;
    const char *tunerstrings_[32];  // weg ?? 
    std::vector<std::string> tunerDescription_;
    int frontendType_[MAXDEVICES];
    bool hasHDExtension_;
    std::string build_;
    std::string release_;
    std::string version_;
    std::string buildDate_;
    std::string distribution_;
    int temps[3];

    void InitDvbInfo();
    void InitReleaseInfo();
    void InitHDExtension();

    void InitRbAvantInfo();
    int GetAvantTemperature(int mode);
    void InitAvantSystemHealth(void);
};

inline const char *cSysInfo::Distribution() const 
{
    return distribution_.c_str();
}

inline const char *cSysInfo::Release() const 
{
    return release_.c_str();
}

inline const char *cSysInfo::Version() const
{
    return version_.c_str();
}

inline const char *cSysInfo::Build() const
{
    return build_.c_str();
}

inline const char * cSysInfo::BuildDate() const
{
    return buildDate_.c_str();
}
inline bool cSysInfo::HasHDExtension() const
{
    return hasHDExtension_;
}

inline const int cSysInfo::Temperature(int mode) const
{
    if (mode>=0 && mode<=1)
	return temps[mode];
    else
	return 0;
}

#endif // __SYSINFO
