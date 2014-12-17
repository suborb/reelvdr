#ifndef __HAL_H
#define __HAL_H

#include <vdr/thread.h>
#include <vector>
#include <dbus/dbus-glib.h>
#ifdef NEWUBUNTU
#include <hal/libhal.h>
#endif

#include "mediad.h"

using namespace std;
// mediatype defines all possible media types that can be detected
class hal : public cThread
{
	public:
		hal();
		virtual ~hal();
		void initAllKnownDevices();
		device* getDevice();
		void setDevices(deviceSources *pDevices);
		void Stop();
		void setMountScript(const char *mountScript);
		static bool triggered;
                bool hasPlayableMedia();
        static bool stop;
	protected:
		virtual void Action(void);
		static void halDeviceAdded( LibHalContext* ctx, const char* udi );
		static void halDeviceRemoved( LibHalContext* ctx, const char* udi );
	private:
		static void configureDevice(device *pDevice, const char* udi);
		static GMainLoop *loop;
		static cMutex _halMutex;
		static deviceSources *_pDevices;
		static device *_pCurDevice;
  	static LibHalContext* _halContext;
		static DBusConnection *_dbusConnection;
		static DBusError _dbusError;
		static char *_mountScript;
		// set as soon as the hal process triggers
		// media detection
};

#endif
