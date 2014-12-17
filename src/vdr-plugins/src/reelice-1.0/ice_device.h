/**************************************************************************
*   Copyright (C) 2011 by Reel Multimedia                                 *
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

#ifndef ICE_DEVICE_SMD_H_INCLUDED
#define ICE_DEVICE_SMD_H_INCLUDED

#include "reelice.h"

class cICEDeviceSMD;

class cEventDebugThread : public cThread {
public:
	ismd_event_t event_list[EVENT_COUNT];
	cICEDeviceSMD *device;
	cEventDebugThread(cICEDeviceSMD *Device) : device(Device) {
		for(int i=0; i < EVENT_COUNT; i++)
			event_list[i] = ISMD_EVENT_HANDLE_INVALID;
//		Start();
	};
	virtual void Action();
}; // cEventDebugThread

class tICE_QueueData {
public:
	ismd_port_handle_t port;
	ismd_event_t       event;
	ismd_time_t        last_pts;
	ismd_time_t        base_pts;
	ismd_time_t        lin_start;
	bool               new_seg;
	bool               discont;
	int                follow;
	bool               write_seg;
	int                cont;
	tICE_QueueData() :
			port(ISMD_PORT_HANDLE_INVALID),
			event(ISMD_EVENT_HANDLE_INVALID) {
		Reset();
	}; // tICE_QueueData
	bool Attach() {
		if(ISMD_PORT_HANDLE_INVALID == port) return false;
		if((ISMD_EVENT_HANDLE_INVALID == event) && (ISMD_SUCCESS != ismd_event_alloc(&event))) return false;
		if(ISMD_SUCCESS == ismd_port_attach(port, event, ismd_queue_event_t(ISMD_QUEUE_EVENT_EMPTY|ISMD_QUEUE_EVENT_NOT_FULL), ISMD_QUEUE_WATERMARK_NONE)) return true;
		ismd_event_free(event);
		event = ISMD_EVENT_HANDLE_INVALID;
		return false;
	}; // Attach
	void Reset() {
		last_pts  = ISMD_NO_PTS;
		base_pts  = ISMD_NO_PTS;
		lin_start = 0;
		new_seg   = true;
		discont   = true;
		follow    = 0;
		write_seg = false;
		cont      = -1;
	} // Reset
	void CheckTs(const uchar *Data, int Length, int Pid = -1) {
		if(Data && Length >= TS_SIZE) {
			if(((Pid < 0) || (TsPid(Data)==Pid)) && TsHasPayload(Data)) {
				int cc = TsContinuityCounter(Data);
//				if((cont >= 0) && (cc != ((cont+1)&TS_CONT_CNT_MASK))) discont = true;
				cont = cc;
			}
		} else Reset();
	} // CheckTs
	virtual ~tICE_QueueData() {
		if(ISMD_EVENT_HANDLE_INVALID != event) ismd_event_free(event);
		event = ISMD_EVENT_HANDLE_INVALID;
	}; // ~tICE_QueueData
}; // tICE_QueueData

class cICEDeviceSMD : public cICEDevice, public cReelBoxBase {
friend class cPluginICE;
friend class cEventDebugThread;
protected:
	cICEDeviceSMD(cPluginICE *Plugin);
	virtual ~cICEDeviceSMD();
public:
	virtual bool HasDecoder() const { return true; };
	virtual bool SetPlayMode(ePlayMode PlayMode);
	virtual int PlayTsVideo(const uchar *Data, int Length);
	virtual int PlayTsAudio(const uchar *Data, int Length);
	virtual int PlayTs(const uchar *Data, int Length, bool VideoOnly);
	virtual int  PlayVideo(const uchar *Data, int Length);
	virtual int  PlayAudio(const uchar *Data, int Length, uchar Id);
	virtual int64_t GetSTC(void);
	virtual void SetVideoDisplayFormat(eVideoDisplayFormat VideoDisplayFormat);
	virtual void SetVideoFormat(bool VideoFormat16_9);
	virtual eVideoSystem GetVideoSystem(void);
	virtual void GetVideoSize(int &Width, int &Height, double &VideoAspect);
	virtual void GetOsdSize(int &Width, int &Height, double &PixelAspect);
	virtual bool HasIBPTrickSpeed(void);
	virtual void TrickSpeed(int Speed);
	virtual void Clear(void);
	virtual void Play(void);
	virtual void Freeze(void);
	virtual bool Flush(int TimeoutMs = 0);
	virtual void StillPicture(const uchar *Data, int Length);
	virtual bool Poll(cPoller &Poller, int TimeoutMs = 0);
	virtual int GetAudioChannelDevice(void);
	virtual void SetAudioChannelDevice(int AudioChannel);
	virtual void SetVolumeDevice(int Volume);
	virtual void SetAudioTrackDevice(eTrackType Type);
	virtual void SetDigitalAudioDevice(bool On);
	virtual bool NeedTrickSpeedDir() { return true; };
	virtual void OverflowDetected();
protected:
	virtual void Action();
#ifdef ICE_HAS_DUMMY_TUNER
public:
	virtual bool ProvidesSource(int Source) const {return true;};
	virtual bool ProvidesTransponder(const cChannel *Channel) const {return true;};
	virtual bool ProvidesChannel(const cChannel *Channel, int Priority = -1, bool *NeedsDetachReceivers = NULL) const {return true;};
	virtual int NumProvidedSystems(void) const {return 1;};
	virtual const cChannel *GetCurrentlyTunedTransponder(void) const {return channel;};
	virtual bool IsTunedToTransponder(const cChannel *Channel) {return true;};
	virtual bool MaySwitchTransponder(void) {return true;};
	virtual bool SetChannelDevice(const cChannel *Channel, bool LiveView) {channel=Channel; return true;};
	virtual bool SetPid(cPidHandle *Handle, int Type, bool On) {return true;};
	const cChannel *channel;
#endif /*ICE_HAS_DUMMY_TUNER*/
public:
	virtual void PlayPipVideo(const unsigned char *Data, int Length);
	virtual void StartPip(bool start);
	virtual bool IsPipChannel(const cChannel *channel, bool *ret);
	virtual bool SetPipVideoType(int type);
	virtual void SetPipDimensions(const uint x, const uint y, const uint width, const uint height);
	virtual bool NeedPipStartInActivate() { return true;};
	virtual void ChannelSwitchInLiveMode();
protected:
#ifdef ICE_TRIGGER_AR_ON_SET_VIDEO_FORMAT
	int SkipSetVideoFormat;
#endif
	int currVideoType, currPipType, currAudioType;
	int playRate;
	ismd_audio_processor_t aud_prc;
	tICE_QueueData queue[ICE_QUEUE_COUNT];
	ismd_codec_type_t vid_codec, pip_codec;
	ismd_viddec_stream_properties_t vid_prop;
	ismd_audio_stream_info_t aud_prop;
	ismd_audio_format_t aud_codec;
	ismd_port_handle_t vid_out;
	ismd_port_handle_t pip_out;
	ismd_port_handle_t ppr_inp;
	ismd_port_handle_t ppr_out;
	ismd_port_handle_t rnd_inp;
	ismd_port_handle_t user_data;
	ismd_dev_t aud;
	ismd_dev_t vid;
	ismd_dev_t ppr;
	ismd_dev_t rnd;
	ismd_dev_t mon;
	ismd_dev_t pip;
	ismd_dev_t pip_rnd;
	ismd_dev_t pip_ppr;
	ismd_dev_t pip_mon;
	ismd_port_handle_t pip_ppr_inp;
	ismd_port_handle_t pip_ppr_out;
	ismd_port_handle_t pip_rnd_inp;
	ismd_dev_state_t pipe_state;
	ismd_time_t base_time;
	ismd_time_t paused_time;
	ismd_event_t event_list[EVENT_COUNT];
	cEventDebugThread video_debug_thread;
	cEventDebugThread audio_debug_thread;
	clock_sync_t clk_sync;
	ismd_clock_t clk;
	ismd_clock_t pip_clk;
	ismd_event_t audio_underrun_event;
	cMutex writeLock;
	uchar *pes_buffer;
	int pes_buffer_pos;
	int WriteESData(tICE_Queue Queue, int Size, const uchar *Data, ismd_pts_t stc = ISMD_NO_PTS);
	bool WriteNewSeg(tICE_Queue Queue, ismd_time_t Start, ismd_time_t Stop, ismd_time_t position = 0);
	bool CreateAudioPipe(ismd_audio_format_t codec);
	void DestroyAudioPipe();
	bool CreateVideoPipe(ismd_codec_type_t codec);
	void DestroyVideoPipe();
	bool CreatePipPipe(ismd_codec_type_t codec);
	void DestroyPipPipe();
	bool CreatePipe();
	void DestroyPipe();
	bool SetPipe(ismd_dev_state_t state);
	void gdlEvent(gdl_app_event_t event);
	virtual void UpdateOptions();
	virtual void UpdateScreen(const gdl_tvmode_t &mode);
	virtual bool SetVideoCodec(ismd_codec_type_t codec);
	virtual bool SetPipCodec(ismd_codec_type_t codec);
	virtual bool SetAudioCodec(ismd_audio_format_t codec);
	virtual void HandleClockRecover(const uchar *Data, int Length);
	virtual void SetPlayRate(int Rate);
}; // cICEDeviceSMD

#endif /*ICE_DEVICE_SMD_H_INCLUDED*/
