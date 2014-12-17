#ifndef __SELECT_DEVICE_MENU_H
#define __SELECT_DEVICE_MENU_H

#include <vdr/menuitems.h>
#include <vdr/menu.h>
#include <vector>

#include "hal.h"
#include "mediad.h"

class deviceItem : public cOsdItem {
    private:
        device *_pDevice;
        virtual void Set(void) { SetText(_pDevice->getName(), true); };
    public:
      deviceItem(device *pDevice);
      device *getDevice(void) { return _pDevice; }
};

class selectDeviceMenu : 
    public cOsdMenu 
{
    public:
        selectDeviceMenu(const deviceSources &devices);
        virtual ~selectDeviceMenu();
        eOSState ProcessKey(eKeys Key);
        static bool triggered;
        static device* getDevice();
                static device* getDvdDevice(const deviceSources &devices); //RMM hack
    private:
        static device* _selectedDevice;
};
#endif
