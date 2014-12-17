#ifndef EXTERNALLIB_H
#define EXTERNALLIB_H

#include <string>
#include <vector>
#include <vdr/thread.h>
#include <vdr/keys.h>

namespace Reel {

#define MAX_EVENT_BUF 1024

typedef enum {
	EXT_NONE,
	EXT_READY,
	EXT_PAUSE,
	EXT_PLAY
} ExtState;

class ExternalLib {
private:
	cMutex info_lock;
	int wfd[2];
	int rfd[2];
	int pid;
	int64_t stream_pos;
	int64_t stream_len;
	int64_t stream_div;
	double  stream_fps;
	double  stream_speed;
	cString uri;
	std::string a_info;
	std::string v_info;
	std::string t_info;
	std::string s_info;
	std::string u_info;
	bool a_auto;
	bool v_auto;
	bool t_auto;
	char event_buf[MAX_EVENT_BUF+1];
	int event_pos;
	ExtState state;
public:
	ExternalLib();
	~ExternalLib();
	void Init();
	void Exit();
	bool Send(const char *cmd, const char *descr);
	const char *GetSetupLang() const;
	int DVDTitleCount() const;
	int DVDTitleNumber() const;
	void DVDTitleSeek(int title_num);
	int DVDChapterCount() const;
	int DVDChapterNumber() const;
	void DVDChapterSeek(int chap_num);
	bool IsDVD() const;
	bool IsInDVDMenu() const;
	std::string Title()  const;
	std::string Album()  const;
	std::string Artist() const;
	void ProcessKeyMenuMode(eKeys key);
	void ProcessKey(eKeys key,bool isMusic);
	void SetPlaybackRate(double playback_rate);
	double PlaybackRate() const;
	bool GetReplayMode(bool &Play, bool &Forward, int &Speed);
	bool IsPaused() const;
	bool IsPlaying() const;
	int AudioStreamCount() const;
	int AudioStream() const;
	std::string AudioLang(int idx) const;
	void SetAudioStream(int idx);
	int VideoStreamCount() const;
	int VideoStream() const;
	void SetVideoStream(int idx);
	int SubtitleStreamCount() const;
	int SubtitleStream() const;
	void SetSubtitleStream(int idx);
	bool HandleEvents();
	bool Play(const char* mrl);
	void Stop();
	void Seek(int pos);
	bool EoS() const;
	double FramesPerSecond(void) { return stream_fps;};
	bool GetIndex(int &Current, int &Total);
	bool IsLive() const;
	bool HasError() const;
}; //class ExternalLib

} // namespace Reel
#endif /*EXTERNALLIB_H*/
