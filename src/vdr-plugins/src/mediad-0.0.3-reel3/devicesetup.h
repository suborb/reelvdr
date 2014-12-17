#ifndef __EPGSEARCHRCFILE_H
#define __EPGSEARCHRCFILE_H

#include <vdr/config.h>
#include <limits.h>
#include <stdlib.h>

enum mediaType {
  audio, data, videodvd, svcd, blank, unknown
};
#define MAXSTRINGLEN 256
class device : public cListObject
{
    public:
        device();
        device(const char * name, const char* deviceName, char* mountPoint, bool unLock = false);
        device(const device& dev);
        device& operator=(device const& dev);
        ~device();
        const char* getDeviceName() const { return _deviceName; }
        const char* getName() const { return _name; }
        /**
         * Checks whether device name or udi match
         */
        bool operator==(const char *deviceUDI);
        void setUDI(const char* curUDI) { _curUDI = strdup(curUDI); }
        const char* getUDI() const { return  _curUDI; }
        const char* getMountPoint() const;
        void setCurMediaType(const mediaType curMediaType);
        const mediaType getCurMediaType() const { return _curMediaType; }
        void setVolume(bool hasVolume) { _hasVolume = hasVolume; }
        bool hasVolume() const { return _hasVolume; }
        void setRedirectedVideoDir(bool redirected) { _redirectedVideoDir = redirected; }
        static char* st_mountScript;
        static void setMountScript(const char *mountScript);
        bool hasRedirectedVideoDir() { return _redirectedVideoDir; }
        bool mount();
        bool umount();
        bool isMounted() const;
        const char* getMountPath() const { return _mountPath; };
        bool Parse(char *s);
    private:
        bool _hasVolume;
        bool _unLock;
        char _deviceName[PATH_MAX];
        char _physicalDeviceName[PATH_MAX];
        char *_name;
        char *_mountPoint;
        char *_curDevice;
        char *_curUDI;
        mediaType _curMediaType;
        bool _redirectedVideoDir;
        char *_mountPath;
        bool _mounted;
        static unsigned _st_instances;
        void setDevice(const char* deviceName);
};

class deviceSources : public cConfig<device> {
private:
  device *current;
public:
  virtual bool Load(const char *filename, bool dummy=false);
  void SetSource(device *source) { current=source; }
  device *GetSource(void) { return current; }
  device *FindSource(const char *device);
};

#endif
