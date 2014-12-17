#include <vdr/keys.h>
#include <vdr/ringbuffer.h>
#include <vdr/remote.h>
#include <vdr/status.h>

#include "mediaservice.h"
#include "cdda_player.h"

//#define DEBUG_D 1
//#define DEBUG_I 1
#include "debug.h"

#define PLAYERBUFSIZE  MEGABYTE(1)

cMediaCddaPlayer::cMediaCddaPlayer(cMediaCddaDisc *Disc, int Track)
: cPlayer(pmAudioOnly), cThread("MediaCddaPlayer")
{
	disc = Disc;
	first_track = 0;
	current_track = Track;
	last_track = disc->Count() - 1;
	current_index = 0;
	total_index = 0;
	skip_track = 0;
	currentMode = pmNone;
	ringBuffer = NULL;
	rFrame = NULL;
	pFrame = NULL;
	ringBuffer = new cRingBufferFrame(PLAYERBUFSIZE);
	cDevice::PrimaryDevice()->SetCurrentAudioTrack(ttAudio);
}

cMediaCddaPlayer::~cMediaCddaPlayer()
{
DBG_D("cMediaCddaPlayer::~cMediaCddaPlayer()")
	Detach();
	delete(ringBuffer);
}

bool cMediaCddaPlayer::Active(void)
{
	return Running();
}

void cMediaCddaPlayer::Stop(void)
{
DBG_D("*** MediaCddaPlayer: Stop %d", currentMode)
	currentMode = pmStop;
}

void cMediaCddaPlayer::Pause(void)
{
DBG_D("*** MediaCddaPlayer: Pause %d", currentMode)
	if(currentMode != pmPlay)
		Play();
	else {
		LOCK_THREAD;
		DeviceFreeze();
		currentMode = pmPause;
	}
}

void cMediaCddaPlayer::Play(void)
{
DBG_D("*** MediaCddaPlayer: Play")
	if(currentMode != pmPlay) {
		LOCK_THREAD;
		DevicePlay();
		currentMode = pmPlay;
	}
}

void cMediaCddaPlayer::PreviousTrack(void)
{
	LOCK_THREAD;
	skip_track = -1;
}

void cMediaCddaPlayer::NextTrack(void)
{
	LOCK_THREAD;
	skip_track = 1;
}

bool cMediaCddaPlayer::GetIndex(int &Current, int &Total, bool SnapToIFrame)
{
DBG_D("*** MediaCddaPlayer: GetIndex")
	Current = current_index;
	Total = total_index;
	return Total >= 0;
}

bool cMediaCddaPlayer::GetReplayMode(bool &Play, bool &Forward, int &Speed)
{
	Play = (pmPlay == currentMode);
	Forward = true;
	Speed = -1;

DBG_D("*** MediaCddaPlayer: GetReplayMode %d %d", Play, currentMode)
	return true;
}

void cMediaCddaPlayer::Activate(bool On)
{
	if(On) {
		Start();
		Play();
	} else if(Active()) {
		Cancel(3);
	}
}

#define MAX_SECTORS 24
#define BUFFERSIZE CDIO_CD_FRAMESIZE_RAW * (MAX_SECTORS + 1)
#define PACKSIZE 2048
#define PACKHEADER 6
#define PESHEADER 3
#define LPCMHEADER 7
#define MAXDATA PACKSIZE - PACKHEADER - PESHEADER - LPCMHEADER
#define DATASTART PACKHEADER + PESHEADER + LPCMHEADER

void cMediaCddaPlayer::GenerateLPCMHeader(uint8_t *Buffer, int Size, int Channels, int Freq)
{

	Buffer[0] = 0x00;
	Buffer[1] = 0x00;
	Buffer[2] = 0x01;
	Buffer[3] = 0xBD;
	Buffer[4] = Size >> 8;
	Buffer[5] = Size & 0xFF;
	Buffer[6] = 0x80;
	Buffer[7] = 0x00;
	Buffer[8] = 0x00;
	Buffer[9] = 0xA0;
	Buffer[10] = 0xFF;
	Buffer[11] = 0x00;
	Buffer[12] = 0x00;
	Buffer[13] = 0x00;
	Buffer[14] = 0x20 | 0x01;
	Buffer[15] = 0x80;
}

void cMediaCddaPlayer::Action(void)
{
	uint8_t buffer[BUFFERSIZE];
	uint8_t *b_start = buffer;
	int b_size = 0;

	cMediaCddaTrack *track = NULL;
	uint32_t cur_frame = 0;
	uint32_t max_frames = 0;
	bool new_track = true;
	
	uint8_t *fstore = NULL;
	uint8_t *fplay = NULL;
	int pc = 0;

	bool sleep = false;
	skip_track = 0;
	
DBG_I("*** MediaCddaPlayer: started")
	while(Running() && (currentMode != pmStop)) {
		if(sleep) {
			cCondWait::SleepMs(3);
			sleep = false;
		}
		cPoller Poller;
		if(DevicePoll(Poller, 100)) {
			LOCK_THREAD;
			if(skip_track) {
				if(skip_track > 0) {
					if(current_track < last_track) {
						ringBuffer->Clear();
						current_track++;
						new_track = true;
						b_size = 0;
					}
				} else if(skip_track < 0) {
					if(current_track > first_track) {
						ringBuffer->Clear();
						current_track--;
						new_track = true;
						b_size = 0;
					}
				}
				skip_track = 0;
			}
			if(new_track) {
				track = disc->Get(current_track);
				cur_frame = 0;
				max_frames = track->TrackSectors();
				current_index = 0;
				total_index = track->TrackSize();
				new_track = false;
				disc->SetLastReplayed(current_track);
			}
			if(currentMode == pmPlay) {
				if(b_size == 0 ) {
					if(cur_frame < max_frames) {
						int n = MAX_SECTORS;
						if(n > (int)(max_frames - cur_frame))
							n = max_frames - cur_frame;
						if(disc->ReadAudioSector(buffer, track->StartLsn() + cur_frame, n)) {
							cur_frame += n;
							b_size = n * CDIO_CD_FRAMESIZE_RAW;
							b_start = buffer;
/*
						printf("READ\n");
						for(int i=0; i< 20; i++)
							printf(" 0x%02X",buffer[i]);
						printf("\n");
*/
						} else {
							DBG_E("MediaCddaPlayer: read error")
							currentMode = pmStop;
							cMediaService::SetCrashed();
							break;
						}
					} else {
DBG_D("MediaCddaPlayer: available %d",ringBuffer->Available())
						if(!DeviceFlush(100))
							continue;
						current_track++;
						if(current_track > last_track) {
							currentMode = pmStop;
							break;
						}
						new_track = true;
						continue;
					} // end if(cur_frame < max_frames)
DBG_D("MediaCddaPlayer: cur_frame %d max_frames %d got %d",
									cur_frame, max_frames, n)
				} else {
					uint8_t *ptr;
					int f_size = MAXDATA;
					if(b_size < MAXDATA)
						f_size = b_size;
DBG_D("MediaCddaPlayer: fsize %d b_size %d", f_size, b_size)
					fstore = MALLOC(uint8_t, f_size + DATASTART);
					GenerateLPCMHeader(fstore, f_size + LPCMHEADER + PESHEADER, 2, 44100);
					ptr = fstore + DATASTART;
					for (int i = 0; i < f_size; i += 2) {
						ptr[i] = b_start[i+1];
						ptr[i+1] = b_start[i];
					}
/*
					printf("Store\n");
					for(int i=0; i< 20; i++)
						printf(" 0x%02X",fstore[i]);
					printf("\n");
*/
					b_start += f_size;
					b_size -= f_size;
					rFrame = new cFrame(fstore, -(f_size + DATASTART), ftUnknown);
					// hands over fstore to the ringBuffer
					fstore = NULL;
					if(rFrame) {
						if(ringBuffer->Put(rFrame))
							rFrame = NULL;
					}
				} // if(b_size == 0 )
		
				if(!pFrame) {
					pFrame = ringBuffer->Get();
					fplay = NULL;
					pc = 0;
				}
				if(pFrame) {
					if(!fplay) {
						fplay = pFrame->Data();
						pc = pFrame->Count();
					}
					if(fplay) {
						int w = 0;
DBG_D("MediaCddaPlayer: playpes plen %d", pc)
						w = PlayPes(fplay, pc, false);
						if(w > 0) {
							fplay += w;
							pc -= w;
						}
					}
					if(pc == 0) {
						current_index += (pFrame->Count() - DATASTART);
						ringBuffer->Drop(pFrame);
						pFrame = NULL;
						fplay = NULL;
DBG_D("bytesplayed: %d %d",current_index, total_index)
					}
				} else {
					sleep = true;
				} // end if(pFrame)
			} else {
				sleep = true;
			} // end if(currentMode == pmPlay)
		} else {
			sleep = true;
		} //end if(DevicePoll(Poller, 100))
	}
DBG_I("*** MediaCddaPlayer: stopped")
}

cMediaCddaControl::cMediaCddaControl(cMediaCddaDisc *Disc, int Track)
: cControl(player = new cMediaCddaPlayer(Disc, Track))
{
	disc = Disc;
	kback_key = false;
	replayMenu = NULL;
	replayTitle = strdup("");
	visible = false;
	first_track = -1;
	current_track = -1;
	last_track = -1;
	char *n = NULL;
	asprintf(&n, "%s: %s", disc->Performer(), disc->Title());
	cMediaService::SetReplaying(true);
	cStatus::MsgReplaying(this, n, NULL, true);
}

cMediaCddaControl::~cMediaCddaControl()
{
DBG_D("cMediaCddaControl::~cMediaCddaControl()")
	cStatus::MsgReplaying(this, NULL, NULL, false);
	cMediaService::SetReplaying(false);
	delete player;
	player = NULL;
	if(replayTitle)
		free(replayTitle);
	if(kback_key)
		cRemote::CallPlugin("mediamanager");
}

void cMediaCddaControl::ShowReplayMenu(bool NewTrack)
{
	if(replayMenu) {
		bool play, direction;
		int total = 0, current = 0, speed = 0;
		
		GetIndex(current, total, false);
DBG_D("UpdateReplayMenu: %d %d",current/176400, total/176400)
DBG_D("UpdateReplayMenu: %d %d",current, total)
		GetReplayMode(play, direction, speed);
		char *c = disc->TrackTimeHMS(current);

		// Add 7 Bytes to suppress the "[...] " id
		if(NewTrack) {
			char *t = disc->TrackTimeHMS(total);
			replayMenu->SetTitle(replayTitle);
			replayMenu->SetTotal(t);
			free(t);
		}
		replayMenu->SetCurrent(c);
		replayMenu->SetProgress(current/176400, total/176400);
		replayMenu->SetMode(play, direction, speed);
		replayMenu->Flush();
		free(c);
	}
}

void cMediaCddaControl::UpdateReplayMenu(void)
{
	if(replayMenu) {
		cMediaCddaTrack *track = NULL;
		bool track_changed = false;
		if(first_track == -1) {
			first_track = 0;
			last_track = disc->Count() - 1;
			current_track = player->CurrentTrack();
			track_changed = true;
		} else {
			int t = player->CurrentTrack();
			if(t != current_track) {
				current_track = t;
				track_changed = true;
			}
		}
		if(track_changed) {
			track = disc->Get(current_track);
			if(replayTitle)
				free(replayTitle);
			asprintf(&replayTitle,"%d/%d %s - %s",current_track + 1,
								last_track + 1, track->Performer(),track->Title());
		}
		if(!visible) {
			track_changed = true;
			visible = true;
		}
		ShowReplayMenu(track_changed);
	}
}

void cMediaCddaControl::Hide(void)
{
	if(replayMenu)
		DELETENULL(replayMenu);
	visible = false;
}

bool cMediaCddaControl::Active(void)
{
	return player && player->Active();
}

void cMediaCddaControl::Stop(void)
{
	if(player)
		player->Stop();
}

void cMediaCddaControl::Pause(void)
{
	if(player)
		player->Pause();
}

void cMediaCddaControl::Play(void)
{
	if(player)
		player->Play();
}

void cMediaCddaControl::PreviousTrack(void)
{
	if(player)
		player->PreviousTrack();
}

void cMediaCddaControl::NextTrack(void)
{
	if(player)
		player->NextTrack();
}

bool cMediaCddaControl::GetIndex(int &Current, int &Total, bool SnapToIFrame)
{
	if(player) {
		player->GetIndex(Current, Total, SnapToIFrame);
		return true;
	}
	return false;
}

bool cMediaCddaControl::GetReplayMode(bool &Play, bool &Forward, int &Speed)
{
	return player && player->GetReplayMode(Play, Forward, Speed);
}

eOSState cMediaCddaControl::ProcessKey(eKeys Key)
{
	eOSState state = osContinue;

	if(!Active()) {
		Hide();
		Stop();
		return osEnd;
	}

	switch(Key & ~k_Repeat) {
		case kOk:
			if(replayMenu == NULL)
				replayMenu = Skins.Current()->DisplayReplay(false);
			else
				Hide();
			break;
		case kUp:
		case kPlay:
			Play();
			break;
		case kDown:
		case kPause:
			Pause();
			break;
		case kGreen:
			PreviousTrack();
			break;
		case kYellow:
			NextTrack();
			break;
		case kStop:
		case kBlue:
			Hide();
			Stop();
			state = osEnd;
			break;
		case kBack:
			Hide();
			Stop();
			kback_key = true;
			return osEnd;
		default:
			break;
	}

	UpdateReplayMenu();
	return state;
}
