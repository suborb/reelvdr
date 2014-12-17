/**************************************************************************
*   Copyright (C) 2008 by Reel Multimedia                                 *
*                                                                         *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#ifndef RBMINI_H_INCLUDED
#define RBMINI_H_INCLUDED

#include <vdr/plugin.h>
#include <vdr/status.h>

#include <vector>

#include "rbm_debug.h"
#include "rbm_setup.h"

#define MAIN_DEVICE_NAME  "/dev/rbm0"
#define AUDIO_DEVICE_NAME "/dev/rbm1"
#define VIDEO_DEVICE_NAME "/dev/rbm2"
#define FB_DEVICE_NAME    "/dev/fb0"

#define PIC_INTERVAL 15
#define PIC_IMAGE_DIR "/usr/share/reel/audio/audio_%d.mpv"

#define TRIGGER_AR_ON_SET_VIDEO_FORMAT

#define MAX_BUFFER 0x100000
class cPluginRBM;

class cRBMOsdProvider : public cOsdProvider {
private:
    int fb_fd; /** the file descriptor for the framebuffer */
public:
	cRBMOsdProvider(void);
	virtual ~cRBMOsdProvider();
protected:
	virtual cOsd *CreateOsd(int Left, int Top, uint Level); //1.6
	///< Returns a pointer to a newly created cOsd object, which will be located
	///< at the given coordinates.
	virtual cOsd *CreateTrueColorOsd(int Left, int Top, uint Level);
}; // cRBMOsdProvider

class cRBMDevice;
class cRBMBgPic : public cThread {
protected:
	int nextPic;
	cRBMDevice *device;
	std::vector<uchar> Buffer;
public:
	cRBMBgPic(cRBMDevice *fDevice);
	virtual void Action();
}; // cRBMBgPic

class cRBMDevice : cDevice {
protected:
	cRBMBgPic bgPic;
	cPluginRBM *plugin;
	int main_fd;
	uchar *Buffer;
	u_int32 BufferPos;
	bool IFrameTrick;
	bool MP3Detected;
	bool OutputEnable;
#ifdef TRIGGER_AR_ON_SET_VIDEO_FORMAT
	int SkipSetVideoFormat;
#endif
	bool AddPacket(const uchar *Data, int Length);
	int ConvertTStoES(const uchar *Data, int Length, u_int32 pid);
	int ConvertPEStoES(const uchar *Data, int Length);
	int ConvertPEStoTS(const uchar *Data, int Length, uchar *Counter, uchar ID);

	cMutex dataMutex;
	int audioIndex;
	int vPid, aPid;
	rbm_dmx_mode dmx_mode;
	rbm_av_mode av_mode;
	rbm_tv_mode tv_mode;
	rbm_tv_mode setup_tv_mode;
	int lastPicTime;
	uchar VideoCounter;
	uchar AudioCounter[256];
	bool Init();
	void Done();
	bool CheckMode();
	void CheckPATPMT(uchar *PATPMT);
#if VDRVERSNUM >= 10716
	int64_t last_audio_pts;
	int64_t last_video_pts;
	uint64_t lastWrite;
	void CheckPatPmt();
	int WriteTS(const uchar *Data, int Length);
#endif
public:
	cRBMDevice(cPluginRBM *Plugin);
	virtual ~cRBMDevice();
	virtual bool HasDecoder(void) const;
	virtual int64_t GetSTC(void);
	virtual bool CanReplay(void) const;
	virtual void TrickSpeed(int Speed);
	virtual void Clear(void);
	virtual void Play(void);
	virtual void Freeze(void);
	virtual void SetVolumeDevice(int Volume);
	virtual void StillPicture(const uchar *Data, int Length);
	virtual bool SetPlayMode(ePlayMode PlayMode);
	virtual void SetAudioTrackDevice(eTrackType Type);
	virtual void SetSubtitleTrackDevice(eTrackType Type);
	virtual void SetAudioChannelDevice(int audioChannel);
	virtual int GetAudioChannelDevice();
	virtual void SetDigitalAudioDevice(bool On);
	virtual void SetVideoDisplayFormat(eVideoDisplayFormat VideoDisplayFormat);
	virtual void SetVideoFormat(bool VideoFormat16_9);
	virtual int PlayVideoTs(const uchar *Data, int Length, bool VideoOnly, uchar *PATPMT);
	virtual int PlayPes(const uchar *Data, int Length, bool VideoOnly = false);
	virtual int PlayPesPacket(const uchar *Data, int Length, bool VideoOnly = false);
	virtual int  PlayVideo(const uchar *Data, int Length);
	virtual int  PlayAudio(const uchar *Data, int Length, uchar Id);
	virtual bool Poll(cPoller &Poller, int TimeoutMs = 0);
	virtual bool Flush(int TimeoutMs = 0);
	virtual int AproxFramesInQueue();
#if VDRVERSNUM >= 10716
//	virtual void GetVideoSize (int &Width, int &Height, double &Aspect);
	virtual void GetOsdSize (int &Width, int &Height, double &Aspect);
	virtual int PlayTsVideo(const uchar *Data, int Length);
	virtual int PlayTsAudio(const uchar *Data, int Length);
	virtual int PlayTs(const uchar *Data, int Length, bool VideoOnly = false);
#endif
	virtual bool NeedDelayedTrickmode() { return true; }
	virtual bool NeedTSCheck() { return true; }
	bool Start(void);
	bool SetTvMode(rbm_tv_mode *mode, bool setup = false);
	bool GetTvMode(rbm_tv_mode *mode);
	bool SetOutput(bool enable);
	bool HasVideo() {return av_mode.uVideoPid != 0;};
}; // cRBMDevice

static const char *VERSION        = "1.0.0";
static const char *DESCRIPTION    = "Output device for reel client";

class cPluginRBM : public cPlugin {
friend class cRBMDevice;
friend class cRBMSetup;
public:
	cPluginRBM(void);
	virtual ~cPluginRBM();
	virtual const char *Version(void) { return VERSION; }
	virtual const char *Description(void) { return DESCRIPTION; }
	virtual const char *CommandLineHelp(void) { return NULL; }
	virtual bool ProcessArgs(int argc, char *argv[]) { return true; }
	virtual bool Initialize(void);
	virtual bool Start(void) { return true; }
	virtual void Housekeeping(void) {}
	virtual const char *MainMenuEntry(void) { return NULL; }
	virtual cOsdObject *MainMenuAction(void) { return NULL; }
	virtual cMenuSetupPage *SetupMenu(void) { return new cRBMSetup(this); }
	virtual bool SetupParse(const char *Name, const char *Value);
	virtual bool Service(char const *id, void *data);
#ifdef REELVDR
    bool HasSetupOptions(void) { return false; } // TB: no entry in the system-settings-menu, the settings are adjusted in the video- or audio-menu
#else
    bool HasSetupOptions(void) { return true; }
#endif
	bool Store();
protected:
	cRBMDevice *device;
	cRBMOsdProvider *osd_provider;
	rbm_tv_mode  tv_mode;
}; // cPluginRBM

#endif /*RBMINI_H_INCLUDED*/
