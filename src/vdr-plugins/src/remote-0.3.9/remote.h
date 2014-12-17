/*
 * Remote Control plugin for the Video Disk Recorder
 *
 * remote.h: input/lirc/tty remote control
 *
 * See the README file for copyright information and how to reach the author.
 */


#ifndef __PLUGIN_REMOTE_H
#define __PLUGIN_REMOTE_H


#include <vdr/remote.h>
#include <vdr/thread.h>

//#define DEBUG

#ifdef DEBUG
#define DSYSLOG(x...)    dsyslog(x)
#else
#define DSYSLOG(x...)
#endif

#if VDRVERSNUM <= 10306
#define MSG_ERROR(x)	Interface->Error(x)
#define MSG_INFO(x)	Interface->Info(x)
#else
#define MSG_ERROR(x)	Skins.Message(mtError,x)
#define MSG_INFO(x)	Skins.Message(mtInfo,x)
#endif
				
// display error message with parameters on OSD
#define EOSD(fmt,parms...)     {  char msg[132]; \
                                  snprintf(msg, sizeof msg, fmt, parms); \
                                  MSG_ERROR(msg); \
                                  MSG_ERROR(msg); /* repeat once */ }

#if APIVERSNUM <= 10404
#define uint64_t	uint64
#endif



/*****************************************************************************/
class cRemoteGeneric : protected cRemote, protected cThread
/*****************************************************************************/
{
protected:
  static const uint64_t INVALID_KEY = (uint64_t) -1;
  int fh;
  char *device;
  int polldelay;
  int repeatdelay;
  int repeatfreq;
  int repeattimeout;
  cRemoteGeneric(const char *name, int f, char *d);
  virtual ~cRemoteGeneric();
  virtual uint64_t getKey(void) = 0;
  virtual bool keyPressed(uint64_t code) = 0;
  virtual bool Put(uint64_t Code, bool Repeat = false, bool Release = false);
  virtual void Action(void);
};



/*****************************************************************************/
class cRemoteDevInput : protected cRemoteGeneric
/*****************************************************************************/
{
private:
  bool testMode;
  uint64_t testKey;
  bool loadKeymap(const char *devname, uint32_t options);
protected:
  virtual uint64_t getKey(void);
  virtual bool keyPressed(uint64_t code);
public:
  cRemoteDevInput(const char *name, int f, char *d);
  virtual bool Initialize(void);
};



#ifdef REMOTE_FEATURE_LIRCOLD
/*****************************************************************************/
class cRemoteDevLirc : protected cRemoteGeneric
/*****************************************************************************/
{
protected:
  virtual uint64_t getKey(void);
  virtual bool keyPressed(uint64_t code);
public:
  cRemoteDevLirc(const char *name, int f, char *d)
    :cRemoteGeneric(name, f, d) { Start(); }
};
#endif



/*****************************************************************************/
class cRemoteDevTty : protected cRemoteGeneric
/*****************************************************************************/
{
private:
  struct termios tm;
  bool has_tm;
protected:
  virtual uint64_t getKey(void);
  virtual bool keyPressed(uint64_t code);
  virtual bool Put(uint64_t Code, bool Repeat = false, bool Release = false);
public:
  cRemoteDevTty(const char *name, int f, char *d);
  virtual ~cRemoteDevTty(); 
};


#endif // __PLUGIN_REMOTE_H
