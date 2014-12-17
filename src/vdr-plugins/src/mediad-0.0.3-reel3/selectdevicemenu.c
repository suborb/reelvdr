#include "selectdevicemenu.h"
#include "mediaplugin.h"

bool selectDeviceMenu::triggered = false;
device* selectDeviceMenu::_selectedDevice = NULL;

deviceItem::deviceItem(device *pDevice) 
{ 
    _pDevice = pDevice; 
    char *textBuffer = NULL;
        // TODO: nasty :-(
    switch(_pDevice->getCurMediaType())
    {
        case audio:
            asprintf(&textBuffer, "%s (%s)", _pDevice->getName(), tr("Audio"));
            break;
        case data:
            asprintf(&textBuffer, "%s (%s)", _pDevice->getName(), tr("Data"));
            break;
        case videodvd:
            asprintf(&textBuffer, "%s (%s)", _pDevice->getName(), tr("Video DVD"));
            break;
        case svcd:
            asprintf(&textBuffer, "%s (%s)", _pDevice->getName(), tr("SVCD"));
            break;
        case blank:
            asprintf(&textBuffer, "%s (%s)", _pDevice->getName(), tr("blank disc"));
            break;
        case unknown:
            asprintf(&textBuffer, "%s (%s)", _pDevice->getName(), tr("unknown"));
            break;
    }
    SetText(textBuffer, true);
    free(textBuffer);
};


selectDeviceMenu::selectDeviceMenu(const deviceSources &devices) :
    cOsdMenu(tr("Select device"))
{
    triggered = false;
    device *pDevice=devices.First();
    while(pDevice) {
        if(pDevice->hasVolume()) {
            Add(new deviceItem(pDevice));
        }
      pDevice=devices.Next(pDevice);
    }
    Display();
}


selectDeviceMenu::~selectDeviceMenu()
{

}

eOSState selectDeviceMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (state != osUnknown) return state;
    switch (Key) {
    
        case kOk: {
                                deviceItem *devItem = dynamic_cast<deviceItem*>(Get(Current()));
                                if(devItem)
                                {
                                    mediaplugin::setCurrentDevice(devItem->getDevice());
                                    _selectedDevice = devItem->getDevice();
                                    triggered = true;
                                }
                                return osPlugin;
                            }
                            break;
                            
                        
        default: 
                    break;
            // rmm start
                                deviceItem *devItem = dynamic_cast<deviceItem*>(Get(0));
                                            if(devItem)
                                        {
                                            mediaplugin::setCurrentDevice(devItem->getDevice());
                                            _selectedDevice = devItem->getDevice();
                                            triggered = true;
                                        }
                                        return osPlugin;
            //rmm stop              
                            break;
    }
    return osUnknown;
}

device* selectDeviceMenu::getDevice()
{
    return _selectedDevice;
}

device* selectDeviceMenu::getDvdDevice(const deviceSources &devices)
{
        device *pDevice=devices.First();
    while(pDevice) 
        {
                //printf("getDvdDevice = %s %s\n", pDevice->getDeviceName(), pDevice->getMountPoint());
        if(std::string(pDevice->getMountPoint()) == "/media/dvd/" ||
                   std::string(pDevice->getMountPoint()) == "/media/dvd")
                {
                    return  pDevice;
        } 
                pDevice=devices.Next(pDevice);
        }
        return NULL;
}

