#include <sstream>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <fcntl.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <netdb.h>

#include "socket.h"
#include "setup.h"
#include "remote.h"
#include "common.h"
#include "i18n.h"

cClientSocket ClientSocket;

#define VLCUSER "admin"
#define VLCPWD  "admin"
#define CLIENTNAME "vdr-vlcclient"
#define CLIENTVERSION "reel"

#define BUFSIZE KILOBYTE(128) 

#define READBUFSIZE KILOBYTE(1)

using namespace std;

int cClientSocket::GetVLCAnswer(bool mode){
  int result = csOk;
  int sd = socket(AF_INET, SOCK_STREAM, 0);  /* init socket descriptor */
  struct sockaddr_in sin;
  struct hostent *hostent = gethostbyname(VlcClientSetup.RemoteIp);
  char *buff = (char*) malloc(READBUFSIZE);
 
  string s1 = VLCUSER;
  s1 += ":";
  s1 += VLCPWD;
  string auth_string;
  cBase64Encoder b64((const unsigned char*)s1.c_str(), s1.length());
  const char *l;
  while((l=b64.NextLine())) {
    auth_string += l;
  }
	  
  memcpy(&sin.sin_addr.s_addr, hostent->h_addr, hostent->h_length);
  sin.sin_family = AF_INET;
  sin.sin_port = htons(PORT);

  if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0){
    esyslog("ERROR: Vlc-Client: Couldn't connect to %s:%d: %s",VlcClientSetup.RemoteIp,PORT, strerror(errno));
    result = csConnectFailed;
  } else {

    string url = "GET /old/admin/dboxfiles.html?";
    url += mode ? "stream_length=true" : "stream_time=true";
    url += "&dir=0123456789ABCDEFGHIJKLM HTTP/1.0\r\n"; /* some dir that doesn't exist */

    url += "User-Agent: ";
    url += CLIENTNAME;
    url += "/";
    url += CLIENTVERSION;

    url += "\r\nAuthorization: Basic ";
    url += auth_string;
    url += "\r\n\r\n";

    int r = write(sd, (const void*)url.c_str(), (size_t)url.length());
    if(r < 0) {
      esyslog("VLC-Client: send() failed: %s",strerror(errno));
      result = csSendFailed;
    } else {
      esyslog("VLC-Client: Request \"%s\" sent to VLC-Server", mode ? "stream_length=true" : "stream_time=true");

      int t = read(sd, (void*)buff, READBUFSIZE);
      while((t = read(sd, (void*)buff, READBUFSIZE))){  /* TODO: fix overlapping reads */
	if(t != -1){  /* we got something */
	  buff[t] = '\0';
          //printf("read %i bytes: %s\n", t, buff);
	  result = atoi(buff); 
	  break;
        }
      }
    }
  }
 
  free(buff);
  close(sd);
  return result;
}

#if 0
const char* cClientSocket::GetVLCInfo(void){
  int result = csOk;
  int sd = socket(AF_INET, SOCK_STREAM, 0);  /* init socket descriptor */
  struct sockaddr_in sin;
  struct hostent *hostent = gethostbyname(VlcClientSetup.RemoteIp);
  char *buff = (char*) malloc(READBUFSIZE);
  char *host = VlcClientSetup.RemoteIp;

  memcpy(&sin.sin_addr.s_addr, hostent->h_addr, hostent->h_length);
  sin.sin_family = AF_INET;
  sin.sin_port = htons(PORT);
  
  if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0){
    esyslog("ERROR: Vlc-Client: Couldn't connect to %s:%d: %s",host,PORT, strerror(errno));
    result = csConnectFailed;
  } else {
    string url = "GET /old/info.html HTTP/1.0\r\n";
    
    url += "User-Agent: ";
    url += CLIENTNAME;
    url += "/";
    url += CLIENTVERSION;
    url += "\r\n\r\n";

    int r = write(sd, (const void*)url.c_str(), (size_t)url.length());
    if(r < 0) {
      esyslog("VLC-Client: send() failed: %s",strerror(errno));
      result = csSendFailed;
    } else {
      esyslog("VLC-Client: Request for Info sent to VLC-Server");

      int t = read(sd, (void*)buff, READBUFSIZE);
      while((t = read(sd, (void*)buff, READBUFSIZE))){  /* TODO: fix overlapping reads */
        if(t != -1){  /* we got something */
          buff[t] = '\0';
          printf("read %i bytes: %s\n", t, buff);
          break;
        }
      }
    }
  }

  free(buff);
  close(sd);
  if(result==csOk)
    return "Info";
  else 
    return NULL;
}
#endif

int cClientSocket::SendVLCCommand(const char* Command){
  cCondWait::SleepMs(250);
  int result = csOk;
  int sd = socket(AF_INET, SOCK_STREAM, 0);  /* init socket descriptor */
  struct sockaddr_in sin;
  struct hostent *hostent = gethostbyname(VlcClientSetup.RemoteIp);
  char *buff = (char*) malloc(BUFSIZE);

  memcpy(&sin.sin_addr.s_addr, hostent->h_addr, hostent->h_length);
  sin.sin_family = AF_INET;
  sin.sin_port = htons(PORT);

  if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0){
    esyslog("ERROR: Vlc-Client: Couldn't connect to %s:%d: %s",VlcClientSetup.RemoteIp,PORT, strerror(errno));
    result = csConnectFailed;
  } else {
    string url = "GET /old/?";
    url += Command;
    url += " HTTP/1.0\r\n";

    url += "User-Agent: ";
    url += CLIENTNAME;
    url += "/";
    url += CLIENTVERSION;
    url += "\r\n\r\n";

    int r;

    r=write(sd, (const void*)url.c_str(), (size_t)url.length());
    if(r<0) {
      esyslog("VLC-Client: send() failed: %s",strerror(errno));
      result = csSendFailed;
    } else {
      esyslog("VLC-Client: Request \"%s\" sent to VLC-Server", Command);
    }
  }
  free(buff);
  close(sd);   
  return result;
}

int cClientSocket::AddToVLCPlaylist(const char* Name){
  string command = "control=add&mrl=";
  command += Name; 
  int len = command.length();
  for(int i = 0; i < len; i++){
    switch(command[i]){
      case ' ':
	command[i]='+';
      default:
	break;
    }
  }
  return SendVLCCommand(command.c_str());
}

int cClientSocket::ClearVLCPlaylist(void){
  return SendVLCCommand("control=empty");
}

int cClientSocket::VLCStartStreaming(void){
  return SendVLCCommand("control=play");
}

int cClientSocket::VLCStopStreaming(void){
  return SendVLCCommand("control=stop");
}

int cClientSocket::VLCPauseStreaming(void){
  return SendVLCCommand("control=pause");
}

int cClientSocket::VLCSkipSeconds(int Seconds){
  stringstream cmd;
  if(Seconds>=0)
    cmd << "%2B";  /* "+Xsec" ("%2B" is "+") */
  cmd << Seconds << "sec";
  return ClientSocket.VLCSeekInStream(cmd.str().c_str());
}

int cClientSocket::VLCGoto(int Seconds){
  stringstream cmd;
  cmd << Seconds;
  cmd << "sec";
  return ClientSocket.VLCSeekInStream(cmd.str().c_str());
}

int cClientSocket::VLCSeekInStream(const char *SeekString){
  string cmd = "control=seek&seek_value=";
  cmd += SeekString;
  return SendVLCCommand(cmd.c_str());
}

int cClientSocket::LoadVLCRecordings(cRemoteRecordings &Recordings, const char *BaseDir) {
  int result = csOk;
  int sd = socket(AF_INET, SOCK_STREAM, 0);  /* init socket descriptor */
  struct sockaddr_in sin;
  struct hostent *hostent = gethostbyname(VlcClientSetup.RemoteIp);
  char *buff = (char*) malloc(BUFSIZE);
  char *host = VlcClientSetup.RemoteIp;

  memcpy(&sin.sin_addr.s_addr, hostent->h_addr, hostent->h_length);
  sin.sin_family = AF_INET;
  sin.sin_port = htons(PORT);
  
  string s1 = VLCUSER;
  s1 += ":";
  s1 += VLCPWD;
  string auth_string;
  cBase64Encoder b64((const unsigned char*)s1.c_str(), s1.length());
  const char *l;
  while((l=b64.NextLine())) {
    auth_string += l;
  }
  
    if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0){    /* timeout problem here */
    esyslog("ERROR: Streamdev: Couldn't connect to %s:%d: %s",host,PORT, strerror(errno));
    result = false;
  } else {
    string url = "GET /old/admin/dboxfiles.html?dir=";
    url += BaseDir;
    url += " HTTP/1.0\r\n";

    url += "User-Agent: ";
    url += CLIENTNAME;
    url += "/";
    url += CLIENTVERSION;

    url += "\r\nAuthorization: Basic ";
    url += auth_string;
    url += "\r\n\r\n";

    int r;

    r=write(sd, (const void*)url.c_str(), (size_t)url.length());
    if(r<0) {
      esyslog("send() failed: %s",strerror(errno));
      result = csSendFailed;
    } else {
      esyslog("Request for VLC-Recordings sent");
      char *ptr2 = buff;
      char *ptr1 = buff;

      while((r = read(sd, (void*)buff, BUFSIZE))){  /* TODO: fix overlapping reads */
        if(r != -1){  /* we got something */
          buff[r] = '\0';
          //printf("read %i bytes: %s\n", r, buff);
          ptr2 = buff;
          for(ptr1 = strsep(&ptr2, "\n"); ptr2; ptr1 = strsep(&ptr2, "\n")){
	    if((ptr2 - ptr1) > 2){
	      string rss (ptr1, (size_t)(ptr2 - ptr1 - 1));
	      //printf("DDDDD: %s\n", rss.c_str()); 
              if( strncmp(rss.c_str(), "Content-type:", 13) && strncmp(rss.c_str(), "Cache-Control:", 14) ){ /* skip html-header */
		if ( !strncmp(rss.c_str(), "HTTP/1.0", 8) ){
		  int returnCode = atoi(rss.c_str()+9);
		  if (returnCode != 200){  /* if not HTTP-reply code 200 - e.g. 404 or 403 are caught here */
		    if(returnCode == 404)
		      esyslog(tr("VLC returned HTTP-Error 404"));
		    //FLUSH();
		    result = csReturned404;
		    break;
		  }
	        } else if ( !strncmp(rss.c_str(), "Content-Length:", 15) ){
                  int contentLength = atoi(rss.c_str()+16);
		  if (contentLength == 0){ /* if http-body has zero-length */
		    esyslog(tr("ERROR: VLC-CLient: VLC returned nothing"));
		    result = csReturnedNothing;
		    break;
		  }
		} else {
		  int len = rss.length();
		  if(rss[len-1]==0x0d) /* microsoft's "^M" */
			  rss[len-1]='\0'; /* strip off "^M" */
                  cRemoteRecording *rec = new cRemoteRecording(rss.c_str());
                  Recordings.Add(rec);
		}
	      }
	    }
          }
        }
      }
    }
  }

  //free(s1);
  free(buff);
  close(sd);
  return result;
}

int cClientSocket::SetVLCSoutMode() {
  int width;
  int height;
  switch(VlcClientSetup.Resolution){
    case 0:   width=352; height=288; break;
    case 1:   width=352; height=576; break;
    case 2:   width=480; height=480; break;
    case 3:   width=576; height=576; break;
    case 4:   width=704; height=576; break;
    case 5:   width=720; height=576; break;
    default:  width=720; height=576; break;
  }
  stringstream url;
  url << "sout=#transcode{vcodec=mp2v";
  url << ",width=" << width;
  url << ",height=" << height;
  url << ",acodec=" << (VlcClientSetup.StreamDD ? "a52" : "mpga");
  url << ",channels=" << (VlcClientSetup.StreamDD ? 6 : 2);
  url << ",fps=" << FRAMESPERSEC ;
  if(VlcClientSetup.PostProQuality)        url << ",pp-q=" << VlcClientSetup.PostProQuality ;
  if(VlcClientSetup.StreamDD)              url << ",ab=512";
  if(VlcClientSetup.Deinterlace)           url << ",deinterlace";
  if(VlcClientSetup.AudioSync)             url << ",audio-sync";
  if(VlcClientSetup.OverlaySubtitles)      url <<  ",soverlay";
  url << ",venc=ffmpeg{";
  if(VlcClientSetup.FfmpegInterlace)       url << "interlace,"; 
  if(VlcClientSetup.FfmpegHurryUp)         url << "hurry-up,";
  if(VlcClientSetup.FfmpegHQProfile)       url << "hq=simple,"; 
  if(VlcClientSetup.FfmpegInterlacedMe)    url << "interlace-me,";
  if(VlcClientSetup.FfmpegPreMe)           url << "pre-me,";
  if(VlcClientSetup.FfmpegStrict)          url << "strict=" << VlcClientSetup.FfmpegStrict << ",";
  if(VlcClientSetup.FfmpegNoiseRed)        url << "noise-reduction=" << VlcClientSetup.FfmpegNoiseRed;
  url << "}}:duplicate{dst=std{access=http,mux=ts{pid-video=68,pid-audio=";
  url << (VlcClientSetup.StreamDD ? 70 : 69);
  url << "},url=:8080/reelboxstream}}";
  return SendVLCCommand(url.str().c_str());
}

