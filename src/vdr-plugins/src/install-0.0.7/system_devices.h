#ifndef SYSTEMDEVICES_H
#define SYSTEMDEVICES_H

#include <string>
#include <vector>
#include <iostream>

enum eDeviceType { eDeviceDisk, eDeviceInternalDisk, eDeviceWifi, eDeviceNetwork }; 

class cSystemDevice
{
    std::string devPath;            /* as '/dev/sda' */
    long int size;                  /* in MiB */
    std::string busType;            /* 'ata' or 'usb' */
    bool removable;
    
public:
    cSystemDevice(std::string path_="", long int size_=0);
    
    void SetDevice(std::string path, long int size_);
    void SetBus(std::string bus);
    void SetRemovable(bool rem);
    
    int operator<(const cSystemDevice &rhs) const ;
    
    void Clear();
    
    std::string Path() const ;
    std::string Bus()  const ;
    long int Size()    const ;
    bool Removable()   const ;
    
    void Print() const ;
    
}; // class cSystemDevice

typedef  std::vector<cSystemDevice> cSystemDevices;


/* returns a vector of attached devices according to param 
   uses libudev to query for devices from kernel.
*/
cSystemDevices system_devices (eDeviceType dev_type);


/*  int root_device(char* buffer, size_t len)
 * reads symlink '/dev/root' and returns where it is pointing to.
 * if '/dev/root' -> 'sda1'  buffer is '/dev/sda1'
 * returns 1 for success 
 */
int root_device(char* buffer, size_t len);

/* returns true if given '/dev/sda' contains the root partition pointed 
 * to by root_device() '/dev/root'*/
bool IsRootDisk(std::string dev);

#endif // SYSTEMDEVICES_H
