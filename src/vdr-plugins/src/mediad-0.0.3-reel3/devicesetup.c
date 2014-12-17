#include <vdr/debug.h>
#include <vdr/thread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "devicesetup.h"
#include "mediad.h"
#include <iostream>

using namespace std;

char *device::st_mountScript = NULL;
unsigned int device::_st_instances = 0;

device::device()
    : _hasVolume(false), _unLock(false), _name(NULL), _mountPoint(NULL), _curDevice(NULL), _curUDI(NULL), _curMediaType(unknown), _redirectedVideoDir(false), _mountPath(NULL), _mounted(false)
{ 
    if(device::_st_instances == 0)
        device::st_mountScript = strdup("pmount.sh");
    device::_st_instances++;
}

device::device(const char* name, const char* deviceName, char* mountPoint, bool unLock) 
    : _hasVolume(false), _unLock(unLock), _name(NULL), _mountPoint(NULL), _curDevice(NULL), _curUDI(NULL), _curMediaType(unknown), _redirectedVideoDir(false), _mountPath(NULL), _mounted(false)
{ 
    if(device::_st_instances == 0)
        device::st_mountScript = strdup("pmount.sh");
    device::_st_instances++;

    setDevice(deviceName);
    if(mountPoint) {
        if(_mountPoint) free(_mountPoint);
        _mountPoint = strdup(mountPoint); 
    }
    if(name)
        _name = strdup(name);
    else
        _name = NULL;
}

device::device(const device& dev) 
{ 
DDD("===> device::device");
    if(device::_st_instances == 0)
        device::st_mountScript = strdup("pmount.sh");
    device::_st_instances++;
    if(dev._name)
        _name = strdup(dev._name); 
    else
        _name = NULL;
    if(dev._deviceName)
        strcpy(_deviceName, dev._deviceName); 
    else
        _deviceName[0] = '\0';
    if(dev._physicalDeviceName)
        strcpy(_physicalDeviceName, dev._physicalDeviceName); 
    else
        _physicalDeviceName[0] = '\0';
    if(dev._mountPoint)
        _mountPoint = strdup(dev._mountPoint); 
    else 
        _mountPoint = NULL;
    if(dev._curDevice)
        _curDevice = strdup(dev._curDevice); 
    else
        _curDevice = NULL;
    if(dev._curUDI)
        _curUDI = strdup(dev._curUDI); 
    else
        _curUDI = NULL;
    if(dev._mountPath)
        _mountPath = strdup(dev._mountPath); 
    else
        _mountPath = NULL;
    _curMediaType = dev._curMediaType;
    _unLock = dev._unLock; 
    _hasVolume = dev._hasVolume;
    _redirectedVideoDir = dev._redirectedVideoDir;
DDD("device name _deviceName %s", _deviceName);
}

device& device::operator=(device const& dev) 
{ 
    if(dev._name)
        _name = strdup(dev._name); 
    else
        _name = NULL;
    if(dev._deviceName)
        strcpy(_deviceName, dev._deviceName); 
    else
        _deviceName[0] = '\0';
    if(dev._physicalDeviceName)
        strcpy(_physicalDeviceName, dev._physicalDeviceName); 
    else
        _physicalDeviceName[0] = '\0';
    if(dev._mountPoint)
        _mountPoint = strdup(dev._mountPoint); 
    else
        _mountPoint = NULL;
    if(dev._curDevice)
        _curDevice = strdup(dev._curDevice); 
    else
        _curDevice = NULL;
    if(dev._curUDI)
        _curUDI = strdup(dev._curUDI); 
    else
        _curUDI = NULL;
    if(dev._mountPath)
        _mountPath = strdup(dev._mountPath); 
    else
        _mountPath = NULL;
    _curMediaType = dev._curMediaType;
    _unLock = dev._unLock; 
    _hasVolume = dev._hasVolume;
    _redirectedVideoDir = dev._redirectedVideoDir;
    return *this; 
}

device::~device() 
{ 
    if(_mountPoint) 
        free(_mountPoint); 
    if(_curDevice) 
        free(_curDevice); 
    if(_curUDI) 
        free(_curUDI); 
    if(_name) 
        free(_name); 
    if(_mountPath) 
        free(_mountPath); 
    device::_st_instances--;
    if(device::_st_instances == 0)
        free(device::st_mountScript);
}

bool device::operator==(const char *deviceUDI) { 
    bool bRet = false;
    string deviceURL = string("/dev/") + _physicalDeviceName;
    if(deviceURL.c_str())
    {
        //cerr << deviceURL << " = " << deviceUDI << endl;
        if(_physicalDeviceName && deviceUDI) 
            bRet = (strcmp(deviceURL.c_str(), deviceUDI) == 0); 
        if(_curUDI && deviceUDI && !bRet) {
            bRet = (strcmp(_curUDI, deviceUDI) == 0); 
        }
        return bRet; 
    } else
        DERR("could not create deviceURI");
    return false;
}

bool device::Parse(char *s)
{
  char deviceName[256], name[256], mountpoint[256];
  int n;
  if((n=sscanf(s,"%255[^;];%255[^;];%255[^;];",deviceName,name,mountpoint))>=2) {
    setDevice(deviceName);
    // preset device mountpoint to be pmount conform
    asprintf(&_mountPoint, "/media/%s", _deviceName);
    if(n>=3) {
        if(_mountPoint) free(_mountPoint);
            _mountPoint = strdup(mountpoint);
    } else
        if(_name) free(_name);
            _name = strdup(name);
    return true;
   }
  return false;
}

const char* device::getMountPoint() const 
{ 
    return _mountPoint;
}

void device::setCurMediaType(const mediaType curMediaType) 
{ 
    _curMediaType = curMediaType; 
}

bool device::mount()
{
    string cmd = string(st_mountScript) + " status /dev/" + getDeviceName();
    int ret = SystemExec(cmd.c_str());
    DDD("executing: %s returns %d", cmd.c_str(), ret);

    if (WEXITSTATUS(ret) == 1) {
        cmd = string(st_mountScript) + " mount /dev/" + getDeviceName();
        DDD("search executing command %s", cmd.c_str());
        ret = SystemExec(cmd.c_str());
        if (WEXITSTATUS(ret) == 0) {
            DDD("mounted successfully");
            _mounted = true;
        }
    } else
        _mounted = true;
    // get current mountpoint
    cmd = string("cat /etc/mtab | grep ") + getDeviceName() + " | awk '{ print $2}'";
    DDD("executing: %s", cmd.c_str());
    FILE *p = popen(cmd.c_str(), "r");
    if (p) {
        char *s;
        cReadLine r;
        if ((s = r.Read(p)) != NULL) 
            _mountPoint = strdup(s);
    }
    return true;
}

bool device::umount()
{
    string cmd = string(st_mountScript) +  " unmount " + getMountPoint();
    int ret = SystemExec(cmd.c_str());
    DDD("search end executing command %s", cmd.c_str());
    if (WEXITSTATUS(ret) == 0) {
        DDD("successfully unmounted");
        _mounted = false;
        return true;
    }

    return false;
}

bool device::isMounted() const
{
    return _mounted;
}

void device::setMountScript(const char *mountScript)
{
    if(device::st_mountScript)
        free(device::st_mountScript);
    device::st_mountScript = strdup(mountScript);
}

void device::setDevice(const char* deviceName) 
{
    string buff;
    if(deviceName) {
        buff = string("/dev/") + deviceName;
        // check whether deviceName is a symbolic link
        struct stat fileStatus;
        bool got_stat=true;
        if(lstat(buff.c_str(), &fileStatus) != 0) {
            DERR("stat error for file %s with errno %d", buff.c_str(), errno);
	    got_stat=false;
	}
        if(got_stat && S_ISLNK(fileStatus.st_mode)) {
            cerr << "is symbolic link" << endl;
            if(realpath(buff.c_str(), _physicalDeviceName) == NULL) {
                cerr << "Symblic links is invalid " << buff << endl;
                strcpy(_physicalDeviceName, deviceName);
            } else {
                strcpy(_physicalDeviceName, _physicalDeviceName+5);
                strcpy(_deviceName, deviceName);
            }
            asprintf(&_mountPoint, "/media/%s", _physicalDeviceName);
        } else {
            strcpy(_physicalDeviceName, deviceName);
            strcpy(_deviceName, deviceName);
        }
    } else {
        _deviceName[0] = '\0';
        _physicalDeviceName[0] = '\0';
    }
}
// -- cFileSources --------------------------------------------------------------

bool deviceSources::Load(const char *filename, bool dummy)
{
  if(cConfig<device>::Load(filename,true)) {
    SetSource(First());
    return true;
    }
  return false;
}

device *deviceSources::FindSource(const char *dev)
{
  device *src = First();
  while(src) {
    if(*src == dev) return src;
    src=Next(src);
    }
  return 0;
}

