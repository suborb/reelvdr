#include "ExternalLib.h"

#include <vdr/recording.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sstream>

namespace Reel {

#define PLAY_CMD_NAME "gstplayer"
#define PLAY_CMD_FILE "/usr/sbin/"PLAY_CMD_NAME
#define PLAY_CMD_PARAM_1 "playbin2"
#define PLAY_CMD_PARAM_2 "uri=\"%s\""
#define PLAY_CMD_PARAM_3  "text-sink=fakesink"
#define PLAY_URI_FILE    "file://"
#define PLAY_URI_DVD     "dvd://"

#define GST_DVD_MENU_TITLE "DVD Menu"

std::string get_token(const std::string s, int l1, int l2) {
	std::stringstream ss1(s);
	std::string s1;
	for(int i=0; i<= l1; i++) if(ss1.eof()) s1=""; else getline(ss1, s1, ',');
	std::stringstream ss2(s1.c_str());
	std::string s2;
	for(int i=0; i<= l2; i++) if(ss2.eof()) s2=""; else getline(ss2, s2, '/');
	return s2;
} // get_token

cString encode_uri(const char * s) {
  std::string ret(s);
  for (int i = 0; i < (int)ret.length(); ++i) {
    int c = (unsigned char)ret[i];
    if (('%' == c) || ('+' == c))
      ret.replace(i, 1, cString::sprintf("%%%02x", c));
  } // for
  return cString(ret.c_str());
} // encode_uri

ExternalLib::ExternalLib() {
	memset(&wfd, -1, sizeof(wfd));
	memset(&rfd, -1, sizeof(rfd));
	pid=-1;
	stream_pos = stream_len = 0;
	stream_fps = DEFAULTFRAMESPERSECOND;
	stream_speed = 1.0;
	event_pos = 0;
	state = EXT_NONE;
	a_auto = true;
	v_auto = false;
	t_auto = true;
} // ExternalLib::ExternalLib

ExternalLib::~ExternalLib() {
	Exit();
} // ExternalLib::~ExternalLib

void ExternalLib::Init() {
} // ExternalLib::Init

void ExternalLib::Exit() {
	if(-1 != wfd[1]) close( wfd[1] ); wfd[1] = -1;
	if(-1 != rfd[0]) close( rfd[0] ); rfd[0] = -1;
	if(-1 != pid) {
		printf("KILLING: %d\n", pid);
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
	} // if
	pid=-1;
} // ExternalLib::Exit

bool ExternalLib::Send(const char *cmd, const char *descr) {
	if((-1 == pid) || (-1 == wfd[1])) return false;
	int len = strlen(cmd);
	if(len == write(wfd[1], cmd, len)) return true;
	printf("Failed sending %s (%s)\n", descr, cmd);
	Exit();
	return false;
} // ExternalLib::Send

const char *ExternalLib::GetSetupLang() const { 
	return "de";
} // ExternalLib::GetSetupLang

bool ExternalLib::IsDVD() const {
	if(!(const char *)uri) return false;
	return !strncmp((const char *)uri, PLAY_URI_DVD, strlen(PLAY_URI_DVD));
} // ExternalLib::IsDVD

bool ExternalLib::IsInDVDMenu() const {
	return !strncmp(Title().c_str(), GST_DVD_MENU_TITLE, strlen(GST_DVD_MENU_TITLE));
} // ExternalLib::IsInDVDMenu

std::string ExternalLib::Title()  const {
	if(IsDVD() && DVDTitleCount() && DVDTitleNumber())
		return (const char *)cString::sprintf("%s %d", tr("Title"), DVDTitleNumber());
	cMutexLock lock((cMutex *)&info_lock);
	return get_token(s_info, 2, 0);
} // ExternalLib::Title

std::string ExternalLib::Album()  const {
	cMutexLock lock((cMutex *)&info_lock);
	return get_token(s_info, 2, 1);
} // ExternalLib::Album

std::string ExternalLib::Artist() const {
	cMutexLock lock((cMutex *)&info_lock);
	return get_token(s_info, 2, 2);
} // ExternalLib::Artist

int ExternalLib::DVDTitleCount() const {
	cMutexLock lock((cMutex *)&info_lock);
	return atoi(get_token(s_info, 0, 1).c_str());
} // ExternalLib::DVDTitleCount

int ExternalLib::DVDTitleNumber() const {
	cMutexLock lock((cMutex *)&info_lock);
	return atoi(get_token(s_info, 0, 0).c_str());
} // ExternalLib::DVDTitleNumber

void ExternalLib::DVDTitleSeek(int title_num) {
} // ExternalLib::DVDTitleSeek

int ExternalLib::DVDChapterCount() const {
	cMutexLock lock((cMutex *)&info_lock);
	return atoi(get_token(s_info, 1, 1).c_str());
} // ExternalLib::DVDChapterCount

int ExternalLib::DVDChapterNumber() const {
	cMutexLock lock((cMutex *)&info_lock);
	return atoi(get_token(s_info, 1, 0).c_str());
} // ExternalLib::DVDChapterNumber

void ExternalLib::DVDChapterSeek(int chap_num) {
	printf("---------------------------------------------------------------DVDChapterSeek %d------------------------------------------------------\n", chap_num);
} // ExternalLib::DVDChapterSeek

void ExternalLib::ProcessKeyMenuMode(eKeys key) {
	switch(key) {
		case kOk   : Send("\r", "OK"); break;
		case kUp   : Send("\033\133\101", "UP"); break;
		case kDown : Send("\033\133\102", "DOWN"); break;
		case kRight: Send("\033\133\103", "RIGHT"); break;
		case kLeft : Send("\033\133\104", "LEFT"); break;
		case kBack : break; // in DVD Menu == > allow showing of DVD Menu, 
		default    : ProcessKey(key, false);
	} // switch
} // ExternalLib::ProcessKeyMenuMode

void ExternalLib::ProcessKey(eKeys key,bool isMusic) {
	switch(key) {
		case kPlay   : Send("s1\rp", "PLAY"); break;
		case kStop   : Stop(); break;
		case kPause  : Send("s1\r ", "PAUSE"); break;
		case kYellow|k_Repeat:
		case kYellow : Send("\033\133\62\64\176", "YELLOW(F12)"); break;
		case kGreen|k_Repeat:
		case kGreen  : Send("\033\133\62\60\176", "GREEN(F9)"); break;
		case kRight|k_Repeat:
		case kRight  : Send("\033\133\62\63\176", "RIGHT(F11)"); break;
		case kLeft|k_Repeat:
		case kLeft   : Send("\033\133\62\61\176", "LEFT(F10)"); break;
		case kFastRew: Send("r", "FREW"); break;
		case kFastFwd: Send("f", "FFWD"); break;
		case k1      : Send("1", "1"); break;
		case k2      : Send("2", "2"); break;
		case k3      : Send("3", "3"); break;
		case k4      : Send("4", "4"); break;
		case k5      : Send("5", "5"); break;
		case k6      : Send("6", "6"); break;
		case k7      : Send("7", "7"); break;
		case k8      : Send("8", "8"); break;
		case k9      : Send("9", "9"); break;
		case kUp     : Send("c", "CHAPTER UP"); break;
		case kDown   : Send("C", "CHAPTER DOWN"); break;
		default      : break;
	} // switch
} // ExternalLib::ProcessKey

void ExternalLib::SetPlaybackRate(double playback_rate) {
	Send((const char *)cString::sprintf("s%lf\r", playback_rate), "SPEED");
} // ExternalLib::SetPlaybackRate

double ExternalLib::PlaybackRate() const { 
	return stream_speed;
} // ExternalLib::PlaybackRate

bool ExternalLib::GetReplayMode(bool &Play, bool &Forward, int &Speed) {
	if(EXT_NONE == state) { printf("Invalid state\n"); return false; }
	Play    = IsPlaying();
	if(!Play) {
		Forward = true;
		Speed   = -1;
	} else {
		Forward = (stream_speed >= 0.0);
		if(fabs(stream_speed) > 4.0)
			Speed   = 3;
		else if(fabs(stream_speed) > 2.0)
			Speed   = 2;
		else if(fabs(stream_speed) > 1.0)
			Speed   = 1;
		else
			Speed   = -1;
	} // if
	return true;
} // ExternalLib::GetReplayMode

bool ExternalLib::IsPaused() const { 
	return (EXT_PAUSE == state);
} // ExternalLib::IsPaused

bool ExternalLib::IsPlaying() const { 
	return (EXT_PLAY == state);
} // ExternalLib::IsPlaying

int ExternalLib::AudioStreamCount() const {
	cMutexLock lock((cMutex *)&info_lock);
	return atoi(get_token(a_info, 0, 1).c_str());
} // ExternalLib::AudioStreamCount

int ExternalLib::AudioStream() const {
	if(a_auto) return -1;
	cMutexLock lock((cMutex *)&info_lock);
	return atoi(get_token(a_info, 0, 0).c_str());
} // ExternalLib::AudioStream

std::string ExternalLib::AudioLang(int idx) const { 
	cMutexLock lock((cMutex *)&info_lock);
	return get_token(a_info, idx+1, 0);
} // ExternalLib::AudioLang

void ExternalLib::SetAudioStream(int idx) {
	a_auto=(idx < 0);
	if(a_auto) idx = AudioStreamCount()-1;
	Send((const char *)cString::sprintf("a%d\r", idx), "AUDIOSTREAM");
} // ExternalLib::SetAudioStream

int ExternalLib::VideoStreamCount() const {
	cMutexLock lock((cMutex *)&info_lock);
	return atoi(get_token(v_info, 0, 1).c_str());
} // ExternalLib::VideoStreamCount

int ExternalLib::VideoStream() const {
	if(v_auto) return -1;
	cMutexLock lock((cMutex *)&info_lock);
	return atoi(get_token(v_info, 0, 0).c_str());
} // ExternalLib::VideoStream

void ExternalLib::SetVideoStream(int idx) {
	v_auto=(idx < 0);
	if(v_auto) idx = VideoStreamCount()-1;
	Send((const char *)cString::sprintf("v%d\r", idx), "VIDEOSTREAM");
} // ExternalLib::SetVideoStream

int ExternalLib::SubtitleStreamCount() const {
	cMutexLock lock((cMutex *)&info_lock);
	return atoi(get_token(t_info, 0, 1).c_str());
} // ExternalLib::SubtitleStreamCount

int ExternalLib::SubtitleStream() const {
	if(t_auto) return -1;
	cMutexLock lock((cMutex *)&info_lock);
	return atoi(get_token(t_info, 0, 0).c_str());
} // ExternalLib::SubtitleStream

void ExternalLib::SetSubtitleStream(int idx) {
	t_auto=(idx < 0);
	if(t_auto) idx = SubtitleStreamCount()-1;
	Send((const char *)cString::sprintf("t%d\r", idx), "TEXTSTREAM");
} // ExternalLib::SetSubtitleStream

bool ExternalLib::HandleEvents() {
	bool ret = false;
	while((event_pos < MAX_EVENT_BUF) && (1 == read(rfd[0], &event_buf[event_pos], 1))) {
		if(('\r' == event_buf[event_pos]) || ('\n' == event_buf[event_pos])) {
			if(!event_pos) continue;
			event_buf[event_pos]=0;
			char *action = NULL;
			char *a_inf = NULL;
			char *v_inf = NULL;
			char *t_inf = NULL;
			char *s_inf = NULL;
			char *u_inf = NULL;
			uint64_t pos, len, div;
			double speed = 1.0;
			int num = sscanf(event_buf, "%a[^:]:%lld/%lld:%lld:%lf:%a[^:]:%a[^:]:%a[^:]:%a[^:]:%a[^\n]", &action, &pos, &len, &div, &speed, &a_inf, &v_inf, &t_inf, &s_inf, &u_inf);
			if(10 == num) {
				cMutexLock lock((cMutex *)&info_lock);
				stream_pos   = pos;
				stream_len   = len;
				stream_div   = div;
				stream_speed = speed;
				if(HasError()) ret = true;
//ExtState old_state = state;
				if(!strcmp(action, "PLAYING")) state = EXT_PLAY;
				else if(!strcmp(action, "PAUSED")) state = EXT_PAUSE;
				else if(!strcmp(action, "READY")) state = EXT_READY;
				else if(!strcmp(action, "NONE")) state = EXT_PAUSE;
				else if(!strcmp(action, "VOID_PENDING")) state = EXT_NONE;
				else { printf("Unknown state: '%s'\n", action); state = EXT_NONE; }
//if(old_state != state) printf("++++++++++++++++++++++++++++++++++++++++++STATE CHANGE %d -> %d\n", old_state, state);
				if(strcmp(a_info.c_str(), a_inf)) { printf("a chg [%s][%s]\n", a_info.c_str(), a_inf); ret = true;}
				if(strcmp(v_info.c_str(), v_inf)) { printf("v chg [%s][%s]\n", v_info.c_str(), v_inf); ret = true;}
				if(strcmp(t_info.c_str(), t_inf)) { printf("t chg [%s][%s]\n", t_info.c_str(), t_inf); ret = true;}
				if(strcmp(s_info.c_str(), s_inf)) { printf("s chg [%s][%s]\n", s_info.c_str(), s_inf); ret = true;}
				if(strcmp(u_info.c_str(), u_inf)) { printf("u chg [%s][%s]\n", u_info.c_str(), u_inf); ret = true;}
				a_info = a_inf;
				v_info = v_inf;
				t_info = t_inf;
				s_info = s_inf;
				u_info = u_inf;
				printf("EVENT >>>%s<<< %d\n", event_buf, num);
			} else if(strncmp(event_buf, "libdvdnav:", strlen("libdvdnav:")))
				printf("IGNORED EVENT >>>%s<<< %d\n", event_buf, num);
			if(action) free(action);
			if(a_inf) free(a_inf);
			if(v_inf) free(v_inf);
			if(s_inf) free(s_inf);
			event_pos=0;
		} else
			event_pos++;
	} // while
	if(event_pos >= MAX_EVENT_BUF)
		event_pos=0;
	if((-1 != pid) && waitpid(pid, NULL, WNOHANG)) {
		Exit();
		ret = true;;
	} // if
	return ret;
} // ExternalLib::HandleEvents

bool ExternalLib::Play(const char* mrl) {
	printf("ExternalLib::Play %s\n", mrl);
	stream_pos = stream_len = 0;
	stream_speed = 1.0;
    if (!mrl || strlen(mrl) == 0) return false;
	if(strstr(mrl, "://"))
		uri = encode_uri(mrl);
	else
		uri = cString::sprintf(PLAY_URI_FILE"%s", (const char *)encode_uri(mrl));
	if((-1 != pid) && waitpid(pid, NULL, WNOHANG))
		Exit();
	if (Send((const char *)cString::sprintf("u%s\r", (const char *)uri), "NEXTFILE"))
		return true;
	cString param = cString::sprintf(PLAY_CMD_PARAM_2, (const char *)uri);
	if(-1 == pipe2(wfd, O_NONBLOCK)) return false;
	if(-1 == pipe2(rfd, O_NONBLOCK)) return false;
	pid=fork();
	if(-1 == pid) return false;
	if(0 == pid) {
		printf("ExternalLib::Play child start %s\n", (const char *)param);
		dup2( wfd[0], STDIN_FILENO);
		dup2( rfd[1], STDOUT_FILENO);
		close( wfd[0] );
		close( wfd[1] );
		close( rfd[0] );
		close( rfd[1] );
		int hdmi_opt = -1;
		int spdif_opt = -1;
		cSetupLine *line = Setup.Get("hdmi_audio", "reelice");
		if(line) hdmi_opt = atoi(line->Value());
		line = Setup.Get("spdif_audio", "reelice");
		if(line) spdif_opt = atoi(line->Value());
		if(2/*ISMD_AUDIO_OUTPUT_PASSTHROUGH*/ == hdmi_opt ) hdmi_opt  = -1; // use OUT_AUTO instread
		if(2/*ISMD_AUDIO_OUTPUT_PASSTHROUGH*/ == spdif_opt) spdif_opt = -1; // use OUT_AUTO instread
		cString extra = cString::sprintf("audio-sink=\"ismd_audio_sink audio-output-hdmi=%d audio-output-spdif=%d\"", hdmi_opt, spdif_opt);
	        if(-1 == execl(PLAY_CMD_FILE, PLAY_CMD_NAME, PLAY_CMD_PARAM_1, (const char *)param, (const char *)extra,  NULL)) _exit(-1);
			printf("ExternalLib::Play child start failed\n");
		_exit(0);
	} else {
		printf("ExternalLib::Play child running %d\n", pid);
		close( wfd[0] );
		close( rfd[1] );
	} // if
	return true;
} // ExternalLib::Play

void ExternalLib::Stop() {
	Send("\177", "STOP");
} // ExternalLib::Stop

void ExternalLib::Seek(int pos) {
	Send((const char *)cString::sprintf("jt%lld\r", (stream_div / 1000) * pos), "SEEK");
} // ExternalLib::Seek

bool ExternalLib::EoS() const {
	cMutexLock lock((cMutex *)&info_lock);
	if(!(const char *)uri || !u_info.c_str()) return false; // No stream running
	if(-1 == pid) return true;
	if(strcmp((const char *)uri, u_info.c_str())) return false; // Not current stream
	if(HasError()) return true;
	if(IsLive()) return false;
	return (EXT_PLAY == state) && (stream_pos > stream_len);
} // ExternalLib::EoS

bool ExternalLib::GetIndex(int &Current, int &Total) {
	if(-1 == pid) return false;
	if(0.0 == stream_fps) return false;
	int64_t base = stream_div / stream_fps;
	if(0 == base) return false;
	Current = HasError() ? 0 : stream_pos / base;
	Total = IsLive() ? Current : stream_len / base;
	if(Total && (Current > Total)) Current = Total;
	if(EoS()) Current = Total;
	return true;
} // ExternalLib::GetIndex

bool ExternalLib::IsLive() const {
	return stream_len < 0;
} // ExternalLib::IsLive

bool ExternalLib::HasError() const {
	return stream_pos < 0;
} // ExternalLib::HasError

} // namespace Reel

