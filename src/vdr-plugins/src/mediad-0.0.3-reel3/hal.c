#define DBUS_API_SUBJECT_TO_CHANGE 1

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <hal/libhal.h>
#include <glib.h>
#include <stdlib.h>
#include <vdr/remote.h>
#include <vdr/recording.h>
#include <iostream>

#include "hal.h"
#include "setup.h"
using namespace std;

cMutex hal::_halMutex;
deviceSources* hal::_pDevices = NULL;
device *hal::_pCurDevice = NULL;
GMainLoop *hal::loop = NULL;
LibHalContext* hal::_halContext = NULL;
DBusConnection *hal::_dbusConnection = NULL;
DBusError hal::_dbusError;
bool hal::triggered = false;
char* hal::_mountScript = NULL;
extern cRecordings Recordings;
extern bool switchedOn;

bool hal::stop = false;

hal::hal() : cThread()
{
}

hal::~hal() 
{
}

bool hal::hasPlayableMedia() {
     if (_pCurDevice && _pCurDevice->hasVolume() && _pCurDevice->getCurMediaType() != unknown && _pCurDevice->getCurMediaType() != blank)
         return true;
     else
         return false; 
}

void hal::Stop()
{ 
    stop = true; //callback func will execute g_main_loop_quit in loop thread

    if(loop) {
        while(g_main_loop_is_running(loop))
            {
                g_main_loop_quit( loop );
                g_main_context_wakeup(g_main_loop_get_context(loop));
            }
            loop=NULL;
    }
    if( _halContext ) {
        libhal_ctx_shutdown( _halContext, 0 );
        libhal_ctx_free( _halContext );
        _halContext = NULL;
    }
}

device* hal::getDevice(){
    return  _pCurDevice;
}

void hal::setDevices(deviceSources *pDevices)
{
    _pDevices = pDevices;
}

gboolean IdleSourceFunc(gpointer data)
{
    usleep(20000);

    if(hal::stop)
    {
        g_main_loop_quit(reinterpret_cast<GMainLoop *>(data));
        return false;
    }
    return true;
}

#if 0
void hal::Action()
{
  DBusConnection *_dbusConnection;
  dbus_error_init(&_dbusError);
  _halContext = libhal_ctx_new();
  if( !_halContext ) {
    //cerr << "mediad: not halContext" << endl;
    return ;
  }
  
  _dbusConnection = dbus_bus_get( DBUS_BUS_SYSTEM, &_dbusError );
  if( dbus_error_is_set(&_dbusError) ) {
    //cerr << "mediad: no dbus connection" << endl;
    return ;
  }

  dbus_connection_setup_with_g_main(_dbusConnection, NULL);


  if(!libhal_ctx_set_dbus_connection(_halContext, _dbusConnection))
  {
     cerr << "mediad: Couldn't connect to HAL!" << endl;
     //TB: do not shut down VDR //exit(1);
     return;
  }


  libhal_ctx_set_dbus_connection( _halContext, _dbusConnection );
  
  libhal_ctx_set_device_added( _halContext, this->halDeviceAdded );
  libhal_ctx_set_device_removed( _halContext, this->halDeviceRemoved );
  libhal_ctx_set_device_new_capability( _halContext, 0 );
  libhal_ctx_set_device_lost_capability( _halContext, 0 );
  libhal_ctx_set_device_property_modified( _halContext, 0 );
  libhal_ctx_set_device_condition( _halContext, 0 );
  
  if( !libhal_ctx_init( _halContext, &_dbusError ) ) {
    cerr << "mediad: Could not init hal" << endl;
    return;
  }

  if (!libhal_device_property_watch_all(_halContext, &_dbusError)) {
    cerr << "mediad: Failed to watch all HAL properties!" << endl << "Error: " << _dbusError.message << endl;
     //TB: do not shut down VDR //exit(1);
  }
  initAllKnownDevices();
  //cerr << "Going into mainloop" << endl;

  loop = g_main_loop_new(NULL, FALSE);
  if ( !loop ) {
     cerr << "mediad: Error creating main loop!" << endl;
     return;
  }

  //add an idle source and callback
  g_idle_add(IdleSourceFunc, loop);

  g_main_loop_run(loop);
 
  //cerr << "mediad: Main loop exited!" << endl;
}
#else

#define RETRY_SLEEP 1000
void hal::Action() {
  dbus_threads_init_default();
  DBusConnection *_dbusConnection; // DL why use same name as global variable???
  dbus_error_init(&_dbusError);

  bool Retry = FALSE;
  while(!(_halContext = libhal_ctx_new())) {
    if(!Retry) isyslog("mediad: failed to create halContext!\n");
    Retry=TRUE;
    cCondWait::SleepMs(RETRY_SLEEP);
    if(stop) return;
  }
  if(Retry) isyslog("mediad: finaly created halContext\n");

  Retry = FALSE;
  _dbusConnection = dbus_bus_get( DBUS_BUS_SYSTEM, &_dbusError );
  while( dbus_error_is_set(&_dbusError) ) {
    if(!Retry) isyslog("mediad: dbus error is set!\n");
    Retry=TRUE;
    cCondWait::SleepMs(RETRY_SLEEP);
    if(stop) return;
  } // while
  if(Retry) isyslog("mediad: finaly no dbus error set\n");

  dbus_connection_setup_with_g_main(_dbusConnection, NULL);

  Retry = FALSE;
  while(!libhal_ctx_set_dbus_connection(_halContext, _dbusConnection)) {
    if(!Retry) isyslog("mediad: couldn't connect to hal!\n");
    Retry=TRUE;
    cCondWait::SleepMs(RETRY_SLEEP);
    if(stop) return;
  } // while
  if(Retry) isyslog("mediad: finaly connected to hal\n");

  libhal_ctx_set_dbus_connection( _halContext, _dbusConnection );

  libhal_ctx_set_device_added( _halContext, this->halDeviceAdded );
  libhal_ctx_set_device_removed( _halContext, this->halDeviceRemoved );
  libhal_ctx_set_device_new_capability( _halContext, 0 );
  libhal_ctx_set_device_lost_capability( _halContext, 0 );
  libhal_ctx_set_device_property_modified( _halContext, 0 );
  libhal_ctx_set_device_condition( _halContext, 0 );

  Retry = FALSE;
  while( !libhal_ctx_init( _halContext, &_dbusError ) ) {
    if(!Retry) isyslog("mediad: couldn't init hal!\n");
    Retry=TRUE;
    cCondWait::SleepMs(RETRY_SLEEP);
    if(stop) return;
  } // while
  if(Retry) isyslog("mediad: finaly inited hal\n");

  Retry = FALSE;
  while (!libhal_device_property_watch_all(_halContext, &_dbusError)) {
    if(!Retry) isyslog("mediad: failed to watch all hal properties!\n");
    Retry=TRUE;
    cCondWait::SleepMs(RETRY_SLEEP);
    if(stop) return;
  } // while
  if(Retry) isyslog("mediad: finaly watching all hal properties\n");

  initAllKnownDevices();

  Retry = FALSE;
  loop = g_main_loop_new(NULL, FALSE);
  while ( !loop ) {
    if(!Retry) isyslog("mediad: failed to create loop!\n");
    Retry=TRUE;
    cCondWait::SleepMs(RETRY_SLEEP);
    if(stop) return;
  } // while
  if(Retry) isyslog("mediad: finaly created loop\n");

  //add an idle source and callback
  g_idle_add(IdleSourceFunc, loop);

  g_main_loop_run(loop);
} // hal::Action
#endif

void hal::halDeviceAdded( LibHalContext* ctx, const char* udi )
{
    _halMutex.Lock();
    if(libhal_device_get_property_bool (ctx, udi, "block.is_volume", 0)) {
        char *device = libhal_device_get_property_string( ctx, udi, "block.device", 0 );
        isyslog("hal::halDeviceAdded %s (%s) %d\n", device, udi, switchedOn);
        //cerr << "Device added " << device << endl;
        if (AutoStartMode) // != 0
        {
        if(switchedOn && (_pCurDevice = _pDevices->FindSource(device)) != NULL)
        {
            //cerr << "matched " << device << endl;
            configureDevice(_pCurDevice, udi);
            _pCurDevice->setVolume(true);
            triggered = true;
            cRemote::CallPlugin("mediad");
        }
        }
        else
        {
            isyslog("not calling mediad since AutoStartMode=0");
        }
        free(device);
    }
    _halMutex.Unlock();
}

void hal::halDeviceRemoved( LibHalContext* ctx, const char* udi )
{
    isyslog("hal::halDeviceRemoved %s\n", udi);
    _halMutex.Lock();
    if( (_pCurDevice = _pDevices->FindSource(udi)) != NULL)
    {
        // only reload recordings in case it has a redirected video directory attached
        if(_pCurDevice->hasRedirectedVideoDir())
        //cerr << "mediad: Removed device " << _pCurDevice->getDeviceName() << endl;
        _pCurDevice->setVolume(false);
                _pCurDevice->setCurMediaType(unknown);
        string umount = string(_mountScript) + " umount /dev/" + _pCurDevice->getDeviceName();
        SystemExec(umount.c_str());
        Recordings.Load();
    }
    _halMutex.Unlock();
}

void hal::initAllKnownDevices()
{
    _halMutex.Lock();
    int num_devices;
    char **udis = (char **) libhal_get_all_devices(_halContext, &num_devices, &_dbusError);
    if ( !udis ) {
        cerr << "mediad: Could not find any devices!" << endl;
        return;
    }
    int i = 0;
    //cerr << "mediad: Checking states for all monitored devices" << endl;
    while ( udis[i] && ( i < num_devices ) )
    {
        char *device = NULL;
        dbus_error_init (&_dbusError);
        if (libhal_device_get_property_bool (_halContext, udis[i], "block.is_volume", &_dbusError)) {
            device = (char *) libhal_device_get_property_string(_halContext, udis[i], "block.device", &_dbusError);
            //cerr << ": device " << device << endl;
            if( (_pCurDevice = _pDevices->FindSource(device)) != NULL)
            {
                //cerr << ": matched " << endl;
                configureDevice(_pCurDevice, udis[i]);              
                _pCurDevice->setVolume(true);
            }
        }
        libhal_free_string(device);
        i++;
    }
    if ( udis )
      libhal_free_string_array(udis);
    _halMutex.Unlock();
}

void hal::configureDevice(device *pDevice, const char* udi) {
    // only interested in the volume
    if( libhal_device_get_property_bool( _halContext, udi, "block.is_volume", 0) ) {
        pDevice->setUDI(udi);
        //cerr << "mediad: UDI " << udi << " " << pDevice->getDeviceName() << endl;
        if(libhal_device_get_property_bool(_halContext, udi, "volume.is_disc", 0)) {
            //cerr << "is disc" << endl;
            if(libhal_device_get_property_bool(_halContext, udi, "volume.disc.is_blank", 0)) {
                pDevice->setCurMediaType(blank);
                //cerr << "blank" << endl;
            } else if(libhal_device_get_property_bool(_halContext, udi, "volume.disc.has_audio", 0)) {
                pDevice->setCurMediaType(audio);
                //cerr << "audio" << endl;
            } else if(libhal_device_get_property_bool(_halContext, udi, "volume.disc.has_data", 0)) {
                pDevice->setCurMediaType(data);
                //cerr << "has data" << endl;
                if(libhal_device_get_property_bool(_halContext, udi, "volume.disc.is_vcd", 0) || libhal_device_get_property_bool(_halContext, udi, "volume.disc.is_svcd", 0)) {
                    pDevice->setCurMediaType(svcd);
                    //cerr << "vcd/svcd" << endl;
                } else if(libhal_device_get_property_bool(_halContext, udi, "volume.disc.is_videodvd", 0)) {
                    pDevice->setCurMediaType(videodvd);
                    //cerr << "video dvd" << endl;
                } else {
                    //cerr << "bulk data" << endl;
                    pDevice->setCurMediaType(unknown);
                }
            } else {
                //cerr << "completely unknown" << endl;
                pDevice->setCurMediaType(unknown);
            }
        } else {
            //cerr << "is data device" << endl;
            pDevice->setCurMediaType(data);
        }
    }
}

void hal::setMountScript(const char *mountScript)
{
    if(_mountScript != NULL)
        free(_mountScript);
    _mountScript = strdup(mountScript);
}
