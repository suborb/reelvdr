#include "system_devices.h"
#include <iostream>

int main()
{
    cSystemDevices devs = system_devices(eDeviceDisk);
    cSystemDevices::iterator it = devs.begin();

    char buf[32]={0};
    int success = root_device(buf, 31);

    std::cout << buf << " is a root disk ? " << IsRootDisk(buf) << std::endl << std::endl;
    while (it != devs.end()) {
        std::cout <<  " is a root disk? " << IsRootDisk(it->Path()) << "  ";
        it->Print();
        
        ++it;
    }
    
    devs.clear();

    devs = system_devices(eDeviceInternalDisk);
    std::cout << "\nInternal Devices:" << std::endl;
    it = devs.begin();
    while (it != devs.end()) {
        it->Print();
        it++;
    } // while

    return 0;
}

