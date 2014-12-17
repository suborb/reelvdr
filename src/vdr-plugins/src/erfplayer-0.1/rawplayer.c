/***************************************************************************
 *   Copyright (C) 2007 by Dirk Leber                                      *
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

#include "rawplayer.h"

#include <vdr/status.h>
#include <hdshm_user_structs.h>

#ifndef _esyslog 
#define _esyslog(a...) esyslog(a)
#endif
#ifndef _isyslog 
#define _isyslog(a...) isyslog(a)
#endif
#ifndef _output 
#define _output(a...) void()
#endif

namespace ERFPlayer {

	RawPlayer::RawPlayer()
			:mReqChan(0), mInfoChan(0), mFile(-1), mInitialized(false), mTerminate(true), mPlaying(false), mPaused(false), mSpeed(100), mPos(0), mSize(0), mLastState(-999), mAudioCount(0), mCurrentAudio(0) {
		memset(mAudioIDs, 0, sizeof(mAudioIDs));
		memset(mAudioTracks, 0, sizeof(mAudioTracks));
		if (::hd_init(0) != 0) {
			_esyslog("ERROR: Unable to open hdshm device.");
		} else {
			try {
				mCmdChan.Open(HDCH_STREAM1);
				hd_packet_erf_init_t packet;
				mCmdChan.SendPacket(HD_PACKET_ERF_INIT, packet, 0, 0); 
				mInitialized = true;
				for (int i = 0; (i < 200) && !mInfoChan; i++) {
					mInfoChan = hd_channel_open(HDCH_STREAM1_INFO);
					if (!mInfoChan)
						::usleep(50000);
				} // for
				if (!mInfoChan) {
					_esyslog("ERROR: Unable to open shm info channel.");
				} else {
					mTerminate = false;
				} // if
				for (int i = 0; (i < 200) && !mReqChan; i++) {
					mReqChan = hd_channel_open(HDCH_STREAM1_DATA);
					if (!mReqChan)
						::usleep(50000);
				} // for
				if (!mReqChan) {
					_esyslog("ERROR: Unable to open shm req channel.");
				} else {
					mTerminate = false;
				} // if
			} catch (...) {
				_esyslog("ERROR: Unable to open shm channels.");
			}
		} // if
		if(!mTerminate) {
			mThread.Start(*this, &RawPlayer::ThreadProc);
			_isyslog("RawPlayer initialized. %p", mCmdChan.ch_);
		} // if
	} // RawPlayer::RawPlayer
	RawPlayer::~RawPlayer() {
		try {
		        hd_packet_erf_done_t packet;
			int lMaxTry = 20;
			if (mInitialized) {
				mCmdChan.SendPacket(HD_PACKET_ERF_DONE, packet, 0, 0); 
				while (mLastState != ERF_STATE_DONE && lMaxTry && !mTerminate) {
				    ::usleep(500000);
				    lMaxTry--;
				} // while
			} // if
		} catch (...) {
			_esyslog("ERROR: Unable to terminate.");
		}
		mTerminate = true;
		mThread.Join();
		if (mInfoChan)
			hd_channel_close(mInfoChan);
		::hd_deinit(0); 
		_isyslog("RawPlayer deinitialized. %p", mCmdChan.ch_);
	} // RawPlayer::~RawPlayer
	bool RawPlayer::Play(const char *fFilePath, const char *fDefLang) {
		if (mTerminate)
			return false;
		int lMaxWait = 60;
		while (lMaxWait && mLastState != 1) {// Wait for init state
			::usleep(500000);
			lMaxWait--;
		} // while
		if (!lMaxWait) {
			_esyslog("RawPlayer: Timeout waiting for hde-player.");
			return false;
		} // if
		mPlaying = false;
		mPaused = false;
		mSpeed  = 100;
		mAudioCount = 0;
		if (mFile >= 0)
			close(mFile);
		mFile = -1;
		if(!strncasecmp(fFilePath, "shm:", 4))
			mFilePath = &fFilePath[4];
		else
			mFilePath = fFilePath;
		try {
		    hd_packet_erf_play_t packet;
		    memset(&packet, 0, sizeof(packet));
			strncpy(&packet.file_name[0], fFilePath, sizeof(packet.file_name)-1);
			if(fDefLang)
				strncpy(&packet.lang[0], fDefLang, sizeof(packet.lang));
			mCmdChan.SendPacket(HD_PACKET_ERF_PLAY, packet, 0, 0); 
			mPlaying = true;
			_isyslog("RawPlayer playing %s.", fFilePath);
			return true;
		} catch (...) {
			_esyslog("ERROR: Unable to Play.");
		}
		return false;
	} // RawPlayer::Play
	bool RawPlayer::Play() {
		mPaused = false;
		mSpeed = 100;
		return Cmd(ERF_CMD_REL, 0, mSpeed, 0);
	} // RawPlayer::Play
	bool RawPlayer::Stop() {
		if (mTerminate)
			return false;
		mPaused = true;
		mSpeed  = 0;
		return Cmd(ERF_CMD_ABS, 0, mSpeed, 0);
	} // RawPlayer::Stop
	bool RawPlayer::Pause() {
		if(mPaused)
			return Play();
		mPaused = true;
		mSpeed  = 0;
		return Cmd(ERF_CMD_REL, 0, mSpeed, 0);
	} // RawPlayer::Pause
	bool RawPlayer::FF() {
		if (mTerminate)
			return false;
		if (mPaused)
			mSpeed += 10;
		else if (mSpeed == -100)
			mSpeed = 100;
		else
			mSpeed += 100;
		if (mSpeed >= 100)
			mPaused = false;
		return Cmd(ERF_CMD_REL, 0, mSpeed, 0);
	} // RawPlayer::FF
	bool RawPlayer::FR() {
		if (mTerminate)
			return false;
		if (mPaused)
			mSpeed -= 10;
		else if (mSpeed == 100)
			mSpeed = -100;
		else 
			mSpeed -= 100;
		if (mSpeed <= -100)
			mPaused = false;
		return Cmd(ERF_CMD_REL, 0, mSpeed, 0);
	} // RawPlayer::FR
	bool RawPlayer::Skip(long lOffset) {
		mPaused = false;
		mSpeed  = 100;
		return Cmd(ERF_CMD_REL, ((long long)lOffset)*1000000, mSpeed, 0);
	} // RawPlayer::Skip
	bool RawPlayer::IsPlaying() {
		if (mTerminate)
			return false;
		return mPlaying;
	} // RawPlayer::IsPlaying
	bool RawPlayer::IsPaused() {
		return mPaused;
	} // RawPlayer::IsPaused
	int RawPlayer::GetSpeed() {
		return mSpeed;
	} // RawPlayer::GetSpeed
	unsigned long long RawPlayer::GetPos() {
		if (mTerminate || !mPlaying)
			return 0;
		return mPos;
	} // RawPlayer::GetPos
	unsigned long long RawPlayer::GetSize() {
		if (mTerminate || !mPlaying)
			return 0;
		return mSize;
	} // RawPlayer::GetSize
	void RawPlayer::SetCurrentAudio(int fId) {
		if (fId < 0 || fId > mAudioCount)
			return;
		Cmd(ERF_CMD_AUDIO, 0, 0, mAudioIDs[fId]);
	} // RawPlayer::SetCurrentAudio
	int RawPlayer::GetAudioCount() {
		return mAudioCount;
	} // RawPlayer::GetAudioCount
	char const *const *RawPlayer::GetAudioTracks() {
		return mAudioTracks;
	} // RawPlayer::GetAudioTracks
	int RawPlayer::GetCurrentAudio() {
		return mCurrentAudio;
	} // RawPlayer::GetCurrentAudio
	void RawPlayer::ThreadProc() {
		while (	!mTerminate) {
			void *rcv_buffer;
			int packet_size;
			if (hd_channel_read_start(mInfoChan, &rcv_buffer, &packet_size, 0))	{
				hd_packet_header_t const *packet_header = (hd_packet_header_t *)rcv_buffer;
				if (packet_header->type == HD_PACKET_ERF_REQ) {
					if (mFile < 0) 
						mFile = open(mFilePath.c_str(), O_RDONLY); 
					hd_packet_erf_req_t *req_packet = (hd_packet_erf_req_t *)rcv_buffer;
					if(mFile < 0) {
						if(req_packet->mode == ERF_REQ_SIZE) 
							req_packet->offset = 0;
						req_packet->size   = 0;
						void *snd_buffer;
						if(hd_channel_write_start(mReqChan, &snd_buffer, req_packet->header.packet_size, 0)) {
							memcpy(snd_buffer, req_packet, req_packet->header.packet_size);
							hd_channel_write_finish(mReqChan, req_packet->header.packet_size);
						} // if
					} else {
						if(req_packet->mode == ERF_REQ_SIZE) {
							lseek(mFile, 0, SEEK_END);
							req_packet->offset = lseek(mFile, 0, SEEK_CUR);
							lseek(mFile, 0, SEEK_SET); 
						} else if (req_packet->mode == ERF_REQ_DATA) {
							lseek(mFile, req_packet->offset, SEEK_SET); 
						} // if
						void *snd_buffer;
						if(hd_channel_write_start(mReqChan, &snd_buffer, HDCH_STREAM1_DATA_SIZE, 0)) {
							req_packet->size = read (mFile, ((char *)snd_buffer)+sizeof(hd_packet_erf_req_t), req_packet->size);
							req_packet->header.packet_size = sizeof(hd_packet_erf_req_t)+req_packet->size;
							memcpy(snd_buffer, req_packet, sizeof(hd_packet_erf_req_t));
							hd_channel_write_finish(mReqChan, HDCH_STREAM1_DATA_SIZE);
						} // if
					} // if
				} else if (packet_header->type == HD_PACKET_ERF_INFO) {
					hd_packet_erf_info_t const *info_packet = (hd_packet_erf_info_t *)rcv_buffer;
					_output("%d (0x%08lx) %ld %d %lld/%lld                    ", info_packet->state, info_packet->error, info_packet->flags, info_packet->speed, info_packet->pos, info_packet->size);
					if (mLastState != info_packet->state) {
						mLastState = info_packet->state;
// mError =
						_isyslog("RawPlayer: %s", GetStateText(mLastState));
					} // if
					mPlaying = info_packet->state != ERF_STATE_FINISHED;
// mSeekable = 
					mPos     = info_packet->pos;
					mSize    = info_packet->size;
				} else if (packet_header->type == HD_PACKET_ERF_SINFO) {
					hd_packet_erf_sinfo_t const *info_packet = (hd_packet_erf_sinfo_t *)rcv_buffer;
					mCurrentAudio = 0;
					mAudioCount   = 0;
					for (int i = 0; i < ERF_MAX_AUDIO; i++) {
						if(mAudioTracks[i])
							free(mAudioTracks[i]);
						mAudioIDs[i] = 0;
						if (info_packet->audio[i].flags & ERF_SINFO_SET) {
							if (info_packet->audio[i].flags & ERF_SINFO_PLAYING)
								mCurrentAudio = i;
							char lDescr[128];
							char lText [128];
							if(info_packet->audio[i].descr[0])
								strncpy(lDescr, info_packet->audio[i].descr, sizeof(lDescr)-1);
							else
								snprintf(lDescr, sizeof(lDescr)-1, "Track %d", i+1);
							if(GetFormatText(info_packet->audio[i].format)[0])
								snprintf(lText, sizeof(lText)-1, "%s (%s)", lDescr, GetFormatText(info_packet->audio[i].format));
							else
								strncpy(lText, lDescr, sizeof(lText)-1);
							mAudioTracks[i] = strdup(lText); 
							mAudioIDs[i] = info_packet->audio[i].id;
							mAudioCount++;
						} else 
							mAudioTracks[i] = 0;
					} // for
				} else 
					_isyslog("Unknown packet type: %d.", packet_header->type);
				hd_channel_read_finish(mInfoChan); 
			} // if    		
			::usleep(1);
		} // while
		if (mFile >= 0)
			close(mFile);
		mFile = -1;
	} // RawPlayer::ThreadProc
	bool RawPlayer::Cmd(int fCmd, long long fOffset, int fSpeed, unsigned long fId) {
		if (mTerminate)
			return false;
		hd_packet_erf_cmd_t packet;
		memset(&packet, 0, sizeof(packet));
		packet.cmd       = fCmd;
		packet.speed     = fSpeed;
		packet.offset    = fOffset;
		packet.id        = fId;
		mCmdChan.SendPacket(HD_PACKET_ERF_CMD, packet, 0, 0); 
		return true;
	} // RawPlayer::Cmd
	const char *gStateText[] = {"Unknown", "Init", "Stoppped","Loading", "Seek pending", "Seek ready", "Playing", "Finished", "Demux error", "Dec error", "Done"};
	const char *RawPlayer::GetStateText(int fState) {
		int lCount = sizeof(gStateText)/sizeof(gStateText[0]);
		if (fState >= 0 && fState <= lCount)
			return gStateText[fState];
		return gStateText[0]; 
	} // RawPlayer::GetStateText
	const char *RawPlayer::GetFormatText(unsigned long  fFormat) {
		switch (fFormat) {
			case 0: return "none";           //FORMAT_NONE
			case 1: return "unsupported";    //FORMAT_UNSUPPORTED
			case 2: return "metadata";       //FORMAT_METADATA
			case 0x01000000: return "";      // FORMAT_TYPE_MUX
			case 0x01000001: return "avi";   // FORMAT_AVI
			case 0x01000002: return "wav";   // FORMAT_WAV
			case 0x01000003: return "mov";   // FORMAT_MOV
			case 0x01000004: return "ts";    // FORMAT_M2TS
			case 0x01000005: return "mpg";   // FORMAT_MPG
			case 0x01000006: return "rtp";   // FORMAT_RTP
			case 0x02000000: return "";      // FORMAT_TYPE_VIDEO
			case 0x02000001: return "raw";   // FORMAT_RAWVIDEO
			case 0x02000002: return "mpg";   // FORMAT_MPV
			case 0x02000003: return "mp4";   // FORMAT_M4V
			case 0x02000004: return "h.264"; // FORMAT_H264
			case 0x02000005: return "h.263"; // FORMAT_H263
			case 0x02000006: return "jpeg";  // FORMAT_JPEG
			case 0x04000000: return "";      // FORMAT_TYPE_AUDIO
			case 0x04000001: return "raw";   // FORMAT_RAWAUDIO
			case 0x04000002: return "mpg";   // FORMAT_MPA
			case 0x04000003: return "mp4";   // FORMAT_M4A
			case 0x04000004: return "ac3";   // FORMAT_AC3
			case 0x04000005: return "amr";   // FORMAT_AMR
			default: return "?"; 
		} // switch
	} // RawPlayer::GetFormatText
} // ERFPlayer

