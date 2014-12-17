#include "player.h"
#include "setup.h"
#include "socket.h"
#include <sys/socket.h>
#include <fcntl.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <netdb.h>
#include "common.h"
#include <vdr/status.h>

#define BUFSIZE 4048
#define POLLTIMEOUTS_BEFORE_DEVICECLEAR 10

cVlcPlayerFeeder::cVlcPlayerFeeder(cRemux *Remux):cThread("vlcplayerfeeder"){
  remux = Remux;
  vlcReader = NULL;
  InitStream();
  Start();
}

cVlcPlayerFeeder::~cVlcPlayerFeeder(){
  //printf("DDD: ~VlcPlayerFeeder\n");
  Cancel(3);
  delete vlcReader;
}

void cVlcPlayerFeeder::InitStream(void){
  ClientSocket.VLCStartStreaming();
  vlcReader = new cVlcReader();
  cCondWait::SleepMs(1000);
  if(!vlcReader->InitVLCDataSocket()){
    esyslog("ERROR: couldn't create data socket");
    //FLUSH();
  }
}

void cVlcPlayerFeeder::ResetStream(void){
  remux->Clear();
  vlcReader->SkipToSyncByte();
}

#define MAXPACKETSOUTOFSYNC 4

void cVlcPlayerFeeder::Action(void){
  int Length = 128*TS_SIZE;
  int nrOutOfSync = 0;          /* nr of received packets out of sync */
  uchar *b = MALLOC(uchar, Length*2);
  bool Sleep = false;
  while(Running()){
    if (Sleep) {
       cCondWait::SleepMs(10); /* this keeps the CPU load low */
       Sleep = false;
    }
    Sleep = true;
    /* input part */
    /* from network to ringbuffer */
    LOCK_THREAD;
    int r = vlcReader->Read(b, Length);
    if(r >= TS_SIZE){
      Sleep = false;
      if(b[0]==0x47) {
	  remux->Put(b, r);
        //int res = remux->Put(b, r);
	//if(res){
          //printf("DDD: VlcPlayer::Action read %i bytes of %i bytes from reader into remuxer\n", r, res);
        //}
      } else {
	  nrOutOfSync++;
	  if(nrOutOfSync >= MAXPACKETSOUTOFSYNC) {
	    nrOutOfSync = 0;
	    vlcReader->SkipToSyncByte();
	  } else
	    esyslog("ERROR: VlcPlayer: reader delivered data out of sync\n");
      }
    } else if(r>0)
	  esyslog("ERROR: VlcPlayer: reader delivered <188 bytes - shouldn't happen\n");

    cCondWait::SleepMs(10);
  }
  delete remux;
}

cVlcPlayer::cVlcPlayer(const char *Name):cThread("vlcplayer"){
  lastGetIndex = cTimeMs::Now()-5000;
  //printf("DDD: new VlcPlayer\n");
  feeder = NULL;
  const int apids[2] = {69, 0};
  const int dpids[2] = {70, 0};
  remux = new cRemux(68, 0, (const int*)&apids, (const int*)&dpids, NULL, false);
}

cVlcPlayer::~cVlcPlayer(void){
  //printf("DDD: ~cVlcPlayer\n");
  //ClientSocket.VLCStopStreaming();
  Detach();
  Cancel(3);
  delete feeder;
}

void cVlcPlayer::Action(void){
  uchar *p = NULL;
  int Result = 0;
  int PollTimeouts = 0;
  bool Sleep = false;
  feeder = new cVlcPlayerFeeder(remux);

  while (Running()){
    if (Sleep) {
      cCondWait::SleepMs(5); /* this keeps the CPU load low */
      Sleep = false;
    }
    Sleep = true;

    /* output part */
    /* get from remux */
    LOCK_THREAD;
    if(!p && remux){
      p = remux->Get(Result);
    }

    /* play */
    if(p){
      Sleep = false;
      //printf("DDD: VlcPlayer::Action got %i bytes from the remuxer\n", Result);
      cPoller Poller;
      if(DevicePoll(Poller, 100)){
	PollTimeouts = 0;
	int w = PlayPes(p, Result);
	if(w > 0) {
	  Sleep = false;
	  //printf("DDD: VlcPlayer::Action played %i bytes\n", w);
	  p += w;
          Result -= w;
          remux->Del(w);
	  if(Result <= 0)
	    p = NULL;
	}
      } else {
	PollTimeouts++;
	if (PollTimeouts == POLLTIMEOUTS_BEFORE_DEVICECLEAR) {
	  Sleep = false;
	  PollTimeouts = 0;
	  esyslog("ERROR: VlcPlayer::Action clearing remux\n");
	  remux->Clear();
	  PlayPes(NULL, 0);
	  p = NULL;
	}
      }
    }
  }
}

void cVlcPlayer::Activate(bool On){
  //printf("DDD: VlcPlayer::Activate: %i\n", On);
  if (On) {
    Start();
  } else
    Cancel(9);
}

void cVlcPlayer::Play(void){
  if(playMode == pmPause || pmStill){
    ClientSocket.VLCPauseStreaming(); /* un-pause VLC */
    feeder->ResetStream();
  }
  playMode = pmPlay;
  DevicePlay();
}

void cVlcPlayer::Forward(void){
}

void cVlcPlayer::Backward(void){
}

void cVlcPlayer::Goto(int Seconds){
  ClientSocket.VLCGoto(Seconds);
  feeder->ResetStream();
}

int cVlcPlayer::SkipFrames(int Frames){
  return 0;
}

void cVlcPlayer::SkipSeconds(int Seconds){
  ClientSocket.VLCSkipSeconds(Seconds);
  feeder->ResetStream();
}

void cVlcPlayer::Pause(void){
  printf("DDD: cVlcPlayer::Pause\n");
  if (playMode == pmPause || playMode == pmStill)
    Play();
  else {
    DeviceFreeze();
    playMode = pmPause;
    ClientSocket.VLCPauseStreaming();
  }
}

bool cVlcPlayer::GetIndex(int &Current, int &Total,  bool visible){
  uint64_t now = cTimeMs::Now();
  if((!visible) && (now - lastGetIndex > 5000)){ /* not more often than every 2. second */
    int CurrentNew = ClientSocket.GetVLCAnswer(0);
    int TotalNew = 0;
    if(Total<=1) TotalNew = ClientSocket.GetVLCAnswer(1);
    //printf("DDD: pos_old: %i pos: %i len_old: %i len: %i\n", Current, CurrentNew, Total, TotalNew);
    if (CurrentNew >= 1)
      Current = CurrentNew;
    if (TotalNew > 1)
      Total = TotalNew;
    lastGetIndex = now;
  }
  return true;
}

bool cVlcPlayer::GetReplayMode(bool &Play, bool &Forward, int &Speed){
  Play = 1;
  Forward = 1;
  Speed = -1;
  return true;
}

cVlcControl::cVlcControl(const char *Name):cControl(player = new cVlcPlayer(Name)) { 
  //printf("DDD: new VlcControl\n");
  name = Name;
  displayReplay = NULL;
  visible = modeOnly = shown = displayFrames = false;
  Current = Total = 1;
  lastCurrent = lastTotal = -1;
  lastPlay = lastForward = false;
  timeSearchActive = false;
  lastSpeed = -1;
  cStatus::MsgReplaying(this, name, NULL, true);
} 

cVlcControl::~cVlcControl(void){
  //printf("DDD: ~VlcControl()\n");
  Hide();
  cStatus::MsgReplaying(this, name, NULL, false);
  Stop();
}

void cVlcControl::Stop(void){
  //printf("DDD: cVlcControl::Stop\n");
  ClientSocket.VLCStopStreaming();
  ClientSocket.ClearVLCPlaylist();
  delete player;
  player = NULL;
}

void cVlcControl::Hide(void){
  //printf("DDD: cVlcControl::Hide\n");
  if(visible){
    delete displayReplay;
    displayReplay = NULL;
    visible = false;
    modeOnly = false;
    lastPlay = lastForward = false;
    lastSpeed = -1;
  }
}

void cVlcControl::Show(void){
  //printf("DDD: cVlcControl::Show\n");
  if(modeOnly)
    Hide();
  if(!visible)
    ShowProgress(true);
}

bool cVlcControl::ShowProgress(bool Initial){
  //printf("DDD: cVlcControl::ShowProgress, visible = %i\n", visible);

  if (GetIndex(Current, Total, visible) && Total > 0 && Current >= 0) {
    //printf("DDD: current: %i total: %i\n", Current, Total);
    if (!visible) {
      displayReplay = Skins.Current()->DisplayReplay(modeOnly);
      visible = true;
    }
    if (Initial) {
      if (name) 
	displayReplay->SetTitle(name);
      lastCurrent = lastTotal = -1;
    }
    if (Total != lastTotal) {
      displayReplay->SetTotal(IndexToHMSF(Total*FRAMESPERSEC));
      if (!Initial) 
	displayReplay->Flush();
    }
    if (Current != lastCurrent || Total != lastTotal) {
      displayReplay->SetProgress(Current, Total);
      if (!Initial)
	displayReplay->Flush();
      displayReplay->SetCurrent(IndexToHMSF(Current*FRAMESPERSEC));
      displayReplay->Flush();
      lastCurrent = Current;
    }
    lastTotal = Total;
    ShowMode();
    return true;
  }
  return false;
}

void cVlcControl::ShowMode(void){
  //printf("DDD: cVlcControl::ShowMode\n");
  if (visible || !cOsd::IsOpen()) {
    bool Play, Forward;
    int Speed; 
    if (GetReplayMode(Play, Forward, Speed) && (!visible || Play != lastPlay || Forward != lastForward || Speed != lastSpeed)) {
      bool NormalPlay = (Play && Speed == -1);
      if (!visible) {
	if (NormalPlay)
	  return; /* no need to do indicate ">" unless there was a different mode displayed before */
	visible = modeOnly = true;
	displayReplay = Skins.Current()->DisplayReplay(modeOnly);
      }
      displayReplay->SetMode(Play, Forward, Speed);
      lastPlay = Play;
      lastForward = Forward;
      lastSpeed = Speed;
    }
  }
}

bool cVlcControl::Active(void){
  return player && player->Active();
}

void cVlcControl::Pause(void){
  if (player)
     player->Pause();  
}

void cVlcControl::Play(void){
  if (player)
     player->Play();
}

void cVlcControl::Forward(void){
  if (player)
     player->Forward();
}

void cVlcControl::Backward(void){
  if (player)
     player->Backward();
}

void cVlcControl::Goto(int Seconds){
  if (player)
    player->Goto(Seconds);
}

void cVlcControl::SkipSeconds(int Seconds){
  if (player)
     player->SkipSeconds(Seconds);
}

int cVlcControl::SkipFrames(int Frames){
  if (player)
     return player->SkipFrames(Frames);
  return -1;
}

void cVlcControl::TimeSearchDisplay(void){
  char buf[64];
  strcpy(buf, tr("Jump: "));
  int len = strlen(buf);
  char h10 = '0' + (timeSearchTime >> 24);
  char h1  = '0' + ((timeSearchTime & 0x00FF0000) >> 16);
  char m10 = '0' + ((timeSearchTime & 0x0000FF00) >> 8);
  char m1  = '0' + (timeSearchTime & 0x000000FF); 
  char ch10 = timeSearchPos > 3 ? h10 : '-';
  char ch1  = timeSearchPos > 2 ? h1  : '-';
  char cm10 = timeSearchPos > 1 ? m10 : '-';
  char cm1  = timeSearchPos > 0 ? m1  : '-';
  sprintf(buf + len, "%c%c:%c%c", ch10, ch1, cm10, cm1);
  displayReplay->SetJump(buf);
}

void cVlcControl::TimeSearch(void){
  timeSearchTime = timeSearchPos = 0;
  timeSearchHide = false;
  if (modeOnly)
    Hide();
  if (!visible) {
    Show();
    if (visible)
      timeSearchHide = true;
    else
      return;
  }
  TimeSearchDisplay();
  timeSearchActive = true;
}

void cVlcControl::TimeSearchProcess(eKeys Key){
#define STAY_SECONDS_OFF_END 10
  int Seconds = (timeSearchTime >> 24) * 36000 + ((timeSearchTime & 0x00FF0000) >> 16) * 3600 + ((timeSearchTime & 0x0000FF00) >> 8) * 600 + (timeSearchTime & 0x000000FF) * 60;
  switch (Key) {
    case k0 ... k9:
      if (timeSearchPos < 4) {
	timeSearchTime <<= 8;
	timeSearchTime |= Key - k0;
	timeSearchPos++;
	TimeSearchDisplay();
      }
      break;
    case kFastRew:
    case kLeft:
    case kFastFwd:
    case kRight: {
		   int dir = ((Key == kRight || Key == kFastFwd) ? 1 : -1);
		   SkipSeconds(Seconds * dir);
		   timeSearchActive = false;
		 }
		 break;
    case kPlay:
    case kUp:
    case kPause:
    case kDown:
    case kOk:
		 Goto(Seconds);
		 timeSearchActive = false;
		 timeSearchActive = false;
		 break;
    default:
		 timeSearchActive = false;
		 break;
  }

  if (!timeSearchActive) {
    if (timeSearchHide)
      Hide();
    else
      displayReplay->SetJump(NULL);
    ShowMode();
  }
}

eOSState cVlcControl::ProcessKey(eKeys Key){
  if (!Active())
    return osEnd;
  if (visible) {
    if (modeOnly)
      ShowMode();
    else
      ShowProgress(!shown);
  }
  if (timeSearchActive && Key != kNone) {
    TimeSearchProcess(Key);
    return osContinue;
  } 
  bool DoShowMode = true;
  switch (Key) {
    // Positioning:
    case kPlay:
      Play(); break;
    case kPause:
      Pause(); break;
    case kLeft:
    case kLeft|k_Repeat:
      SkipSeconds( -5); break;
    case kRight:
    case kRight|k_Repeat:
      SkipSeconds( 5); break;
    case kRed:     TimeSearch(); break;
    case kGreen|k_Repeat:
    case kGreen:   SkipSeconds(-60); break;
    case kYellow|k_Repeat:
    case kYellow:  SkipSeconds( 60); break;
    case kStop:
    case kBlue:    Hide();
		   Stop();
		   return osEnd;
    default: {
      DoShowMode = false;
      switch(Key){
        case kOk:      if (visible && !modeOnly) {
			 Hide();
			 DoShowMode = true;
		       } else
			 Show();
		       break;
        case kBack:    return osBack;
        default:       return osContinue;
      }
    }
  } 
  if(DoShowMode)
    ShowMode(); 
  return osContinue;
}

cVlcReader::cVlcReader():cThread("vlcreader"){
  hasData = false;
  length = wanted = 0;
  buffer = NULL;
}

bool cVlcReader::InitVLCDataSocket(void){
  bool headerOk = false;
  sd = socket(AF_INET, SOCK_STREAM, 0);  
  struct sockaddr_in sin;
  struct hostent *hostent = gethostbyname(VlcClientSetup.RemoteIp);
  char *buff;

  memcpy(&sin.sin_addr.s_addr, hostent->h_addr, hostent->h_length);
  sin.sin_family = AF_INET;
  sin.sin_port = htons(PORT);
  
  if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0){
    esyslog("ERROR: VLC-Client: Couldn't connect to %s:%d: %s",VlcClientSetup.RemoteIp,PORT, strerror(errno));
  } else {
    asprintf(&buff, "GET /reelboxstream HTTP/1.0\r\nAccept: */*\r\n\r\n");
    //printf("Sent: %s\n", buff);
  
    int r = write(sd, (const void*)buff, (size_t)strlen(buff));
    if(r < 0) {
      esyslog("ERROR: VLC-Client: send() failed: %s",strerror(errno));
    } else {
      esyslog("VLC-Client: Request for Stream sent");
    }

    headerOk = ParseHeader();

    if(headerOk)
      SkipToSyncByte();
  }
  free(buff);
 
  if(headerOk){
    fcntl(sd, F_SETFL, O_NONBLOCK); // socket should be non-blocking
    cCondWait::SleepMs(1000); /* let VLC set up the stream */
    Start();
  }
  return headerOk;
}

cVlcReader::~cVlcReader(){
  //printf("DDD: ~cVlcReader\n");
  //ClientSocket.VLCStopStreaming();
  //ClientSocket.ClearVLCPlaylist();
  //Stop();
  Cancel(3);
  CloseVLCDataSocket();
  //free(buffer);
}

void cVlcReader::CloseVLCDataSocket(void){
  length = 0;
  hasData = false;
  close(sd);
}

bool cVlcReader::ParseHeader(){
  bool result = true;
  char *buff = (char*) malloc(BUFSIZE);
  char *ptr2 = buff;
  char *ptr1 = buff;

  int r = read(sd, (void*)buff, BUFSIZE);
  if(r > 0){  /* we got something */
    buff[r] = '\0';
    //printf("read %i bytes: %s\n", r, buff);
    ptr2 = buff;
    for(ptr1 = strsep(&ptr2, "\n"); ptr2; ptr1 = strsep(&ptr2, "\n")){
      if((ptr2 - ptr1) > 2){
	std::string str(ptr1, (size_t)(ptr2 - ptr1 - 1));
	//printf("1: %s\n", buff2);
	if( strncmp(str.c_str(), "Content-type:", 13) && strncmp(str.c_str(), "Cache-Control:", 14) ){ /* skip html-header */
	  if ( !strncmp(str.c_str(), "HTTP/1.0", 8) ){
	    int returnCode = atoi(str.c_str()+9);
	    if (returnCode != 200){  /* if not HTTP-reply code 200 - e.g. 404 or 403 are caught here */
	      //printf("DDD: returnCode: %i\n", returnCode);
	      result = false;
	      break;
	    }
	  } else if ( !strncmp(str.c_str(), "Content-Length:", 15) ){
	    int contentLength = atoi(str.c_str()+16);
	    if (contentLength == 0){ /* if http-body has zero-length */
	      //printf("DDD: contentLength: %i\n", contentLength);
	      result = false;
	      break;
	    }
	  }
	}
      }
    }
  } else 
    result = false;
  return result;
}

#define MAXSYNCRETRIES 9999

bool cVlcReader::SkipToSyncByte(){
    int k = 0, j = 0, m = 0;
    uchar bla[TS_SIZE*10];
    LOCK_THREAD;
    do {
      k = read(sd, (void*)&bla, TS_SIZE*10); /* read 10 times TS_SIZE */
        while(m < 7*TS_SIZE && (bla[m] != 0x47 && bla[m+TS_SIZE] != 0x47 && bla[m+2*TS_SIZE] != 0x47)){ /* search for 3 syncbytes with TS_SIZE-1 bytes between */
          m++;
        }
        if(bla[m] == 0x47 && bla[m+TS_SIZE] == 0x47 && bla[m+2*TS_SIZE] == 0x47){ /* if syncbyte found */
          read(sd, (void*)&bla, TS_SIZE-(m%TS_SIZE)); /* skip to next syncbyte */
          //printf("DDD: found syncbyte\n");
	  return true;
        }
      m = 0;
      j++;
      if(j>MAXSYNCRETRIES){ /* emergency exit - avoid endless loop */
        esyslog("ERROR: VLC-Client: giving up, no syncbyte found\n");
        break;
      }
    } while (k);
    return false;
}

void cVlcReader::Action(void){
  int r;
  while(Running()) {
    Lock();
    if(!hasData && buffer && sd) {
      //printf("DDD: cVlcReader::Action()\n");
      r = read(sd, (void*)(buffer+length), wanted - length);
      if(r > 0){ // we got something
        //printf("DDD: VlcReader::Action read %i bytes from Stream\n", r);
        length += r;
	if(length == wanted || !r) /* r == 0 means EOF */
	  hasData = true;
      } else if (r < 0 && FATALERRNO) {
         esyslog("ERROR: cVlcReader::Action() error reading from Stream\n");   
         LOG_ERROR;
      }
    }
    Unlock();
    cCondWait::SleepMs(5);
  }
}

int cVlcReader::Read(uchar *Buffer, int Length){
  if (hasData && buffer) {
     if (buffer != Buffer) {
        esyslog("ERROR: cVlcReader::Read() called with different buffer!");
        errno = EINVAL;
        return -1;
        }
     buffer = NULL;
     return length;
     }
  if (!buffer) {
     buffer = Buffer;
     wanted = Length;
     length = 0;
     hasData = false;
     }
  errno = EAGAIN;
  return -1;
}

