#include "system_devices.h"
#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#ifdef NEWUBUNTU
#include <unistd.h>
#endif


/// ------------------------- cSystemDevice ------------------------------------
/// holds informaiton about (disk) devices found in system, 
/// by querying kernel via libudev
/// ----------------------------------------------------------------------------
cSystemDevice::cSystemDevice(std::string path_, long int size_) 
    : devPath(path_), size(size_), busType(""), removable(false)
{
}


void cSystemDevice::SetDevice(std::string path, long int size_) 
{ 
    devPath = path; 
    size = size_;
}
void cSystemDevice::SetBus(std::string bus) { busType = bus; }
void cSystemDevice::SetRemovable(bool rem) { removable = rem; }

int cSystemDevice::operator<(const cSystemDevice &rhs) const 
{
    return devPath.compare(rhs.Path()); 
}

void cSystemDevice::Clear() 
{
    SetDevice("", 0);
    SetBus("");
    SetRemovable(false);
}

std::string cSystemDevice::Path() const { return devPath;   }
std::string cSystemDevice::Bus()  const { return busType;   }
long int cSystemDevice::Size()    const { return size;      }
bool cSystemDevice::Removable()   const { return removable; }

void cSystemDevice::Print() const 
{ 
    std::cout << "Path:" << Path() << " Bus:" << Bus() 
              << " Size:" << Size() << " Removable:" << Removable()
              << std::endl;
} // Print()




/* --------------- system_devices ----------------------------------------------
 * query kernel via udev, information about devices attached.
 * filter the devices by the requested type.
 *----------------------------------------------------------------------------*/

cSystemDevices system_devices (eDeviceType dev_type)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev;
	
    cSystemDevices list;
	/* Create the udev object */
	udev = udev_new();
	if (!udev) {
		printf("Can't create udev\n");
		return list;
	}
	
	enumerate = udev_enumerate_new(udev);
    
    
    // filter devices according to requested type
    switch (dev_type) {
    case eDeviceNetwork: /* not fully supported yet */
        udev_enumerate_add_match_subsystem(enumerate, "net");
        break;
        
    case eDeviceDisk:
         /* return all disk devices, removable or not */
        udev_enumerate_add_match_property(enumerate, "ID_TYPE", "disk");
        udev_enumerate_add_nomatch_sysattr (enumerate, "partition", "*"); // we dont need partitions in a disk
        break;
        
    case eDeviceInternalDisk:
        udev_enumerate_add_match_property(enumerate, "ID_TYPE", "disk");
        udev_enumerate_add_nomatch_sysattr (enumerate, "partition", "*");
        udev_enumerate_add_match_property(enumerate, "ID_BUS", "ata");
        udev_enumerate_add_match_sysattr (enumerate, "removable", "0");
        break;
        
    default:
        break;
    } // switch
    
    
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);

    cSystemDevice sysDevice;
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path;
        const char* size_str;
        long int size = 0;

        sysDevice.Clear();
        
        path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);

		//printf("\nDevice Node Path: %s\n", udev_device_get_devnode(dev));

#if 0
        printf("dev devpath : %s\n", udev_device_get_devpath(dev));
        printf("dev subsystem: %s\n", udev_device_get_subsystem(dev));
        printf("dev devtype: %s\n", udev_device_get_devtype(dev));
        printf("dev syspath: %s\n", udev_device_get_syspath(dev));
        printf("Dev sysname: %s\n", udev_device_get_sysname(dev));
        printf("Dev sysnum %s\n", udev_device_get_sysnum(dev));
#endif   
        size_str = udev_device_get_sysattr_value(dev, "size");
        if (size_str) {
            size = atol(size_str);
            size = size /2/1024; // MiB
        }
        
        sysDevice.SetDevice(udev_device_get_devnode(dev), size);
        
        const char *temp = udev_device_get_sysattr_value(dev, "removable");
        
        if (temp) {
            sysDevice.SetRemovable(*temp=='1');
        }
        
        temp = udev_device_get_property_value(dev, "ID_BUS");
        sysDevice.SetBus(temp);

        
        /* sysname: sda1 sysnum: 1
           sysname: sdb  sysnum: (null)  <<<< this is what we want*/
        
        list.push_back(sysDevice);
        
        //sysDevice.Print();
	}
	/* Free the enumerator object */
	udev_enumerate_unref(enumerate);

	udev_unref(udev);

    std::sort(list.begin(), list.end());
	return list;       
}



/*  int root_device(char* buffer, size_t len)
 * reads symlink '/dev/root' and returns where it is pointing to.
 * if '/dev/root' -> 'sda1'  buffer is '/dev/sda1'
 * returns 1 for success 
 */
int root_device(char* buffer, size_t len)
{
    const char* root="/dev/root";
    char buf[32]={0};
    ssize_t size = readlink(root, buf, 31);

    /* readlink does not append '\0' */
    if (size >=0 ) { 
        buf[size] = 0;  

        if (len > 5 + size) /* len > strlen('/dev/') + strlen(buf) */
            snprintf(buffer, len, "/dev/%s", buf);
        else  
            size = -1; /* indicate error, buffer not large enough */
    } /* if  size >= 0 */

    return size>=0;
} /* root_device() */



/* 
 * If root partition is '/dev/sdd2' should 
 * return true for all paritions in 
 *        '/dev/sdd??' and '/dev/sdd'
 */
bool IsRootDisk(std::string dev) 
{
    char root_dev[64] = {0};
    int success = root_device(root_dev, 63);
    std::string rootDev;
    
    if (!success) 
        printf("(%s:%d) Error: could not find root device!",
                __FILE__, __LINE__);
    else
        rootDev = root_dev;
    
    return rootDev.find(dev) != std::string::npos;
}
