/*
 * radioaudio.h - part of radio.c, a plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __RADIO_AUDIO_H
#define __RADIO_AUDIO_H

#include <linux/types.h>
#include <linux/dvb/video.h>
#include <vdr/player.h>
#include <vdr/device.h>
#include <vdr/audio.h>
#include <vdr/osd.h>
#include <vdr/menu.h>
#include <vdr/receiver.h>

extern char *ConfigDir;
extern char *DataDir;
extern char *ReplayFile;

//Setup-Params
extern int S_RtFunc;
extern int S_StillPic;
extern int S_RtOsdTitle;
extern int S_RtOsdTags;
extern int S_RtOsdPos;
extern int S_RtOsdRows;
extern int S_RtOsdLoop;
extern int S_RtOsdTO;
extern int S_RtSkinColor;
extern int S_RtBgCol;
extern int S_RtBgTra;
extern int S_RtFgCol;
extern int S_RtDispl;
extern int S_RtMsgItems;
extern int S_RassText;
extern int S_RockAnt;
extern uint32_t rt_color[9];
extern int S_Verbose;
//Radiotext
#define RT_MEL 65
extern char RT_Text[5][RT_MEL];
extern char RTP_Artist[RT_MEL], RTP_Title[RT_MEL];
extern int RT_Info, RT_Index, RT_PTY;
extern time_t RTP_Starttime;
extern bool RT_OsdTO, RTplus_Osd, RT_ReOpen;
extern int Radio_CA;
extern int RT_OsdTOTemp;
extern int IsRadioOrReplay;
// Info
extern bool InfoRequest;


void radioStatusMsg(void);

// seperate thread for showing RadioImages
class cRadioImage: public cThread {
private:
    char *imagepath;
    bool imageShown;
    void Show (const char *file);
    void send_pes_packet(unsigned char *data, int len, int timestamp);
protected:
    virtual void Action(void);
    void Stop(void);
public:
    cRadioImage(void);
    virtual ~cRadioImage();
    static void Init(void);
    static void Exit(void);
    void SetBackgroundImage(const char *Image);
};

// RDS-Receiver for seperate Data-Pids
class cRDSReceiver : public cReceiver {
private:
    int pid;
    bool rt_start;
    bool rt_bstuff;
protected:
    virtual void Receive(uchar *Data, int Length);
public:
    cRDSReceiver(int Pid);
    virtual ~cRDSReceiver(void);
};

class cRadioAudio : public cAudio {
private:
    bool enabled;
    int first_packets;
    int audiopid;
    bool bratefound;
    //Radiotext
    cDevice *rdsdevice;
    void RadiotextCheckPES(const uchar *Data, int Length);
    void RadiotextCheckTS(const uchar *Data, int Length);
    void AudioRecorderService(void);
    void RassDecode(uchar *Data, int Length);
protected:
    virtual void Play(const uchar *Data, int Length, uchar Id);
    virtual void PlayTs(const uchar *Data, int Length);
    virtual void Mute(bool On) {};
    virtual void Clear(void) {};
public:
    cRadioAudio(void);
    virtual ~cRadioAudio(void);
    char *bitrate;
    void EnableRadioTextProcessing(const char *Titel, int apid, bool replay = false);
    void DisableRadioTextProcessing();
    void RadiotextDecode(uchar *Data, int Length);
    void RDS_PsPtynDecode(bool PTYN, uchar *Data, int Length);
};

class cRadioTextOsd : public cOsdObject {
private:
    cOsd *osd;
    cOsd *qosd;
    cOsd *qiosd;
    const cFont *ftitel;
    const cFont *ftext;
    int fheight;
    int bheight;
    eKeys LastKey;
    cTimeMs osdtimer;
    void rtp_print(void);
    bool rtclosed;
    bool rassclosed;
    static cBitmap rds, arec, rass, radio;
    static cBitmap index, marker, page1, pages2, pages3, pages4, pageE;
    static cBitmap no0, no1, no2, no3, no4, no5, no6, no7, no8, no9, bok;
public:
    cRadioTextOsd();
    ~cRadioTextOsd();
    virtual void Hide(void);
    virtual void Show(void);
    virtual void ShowText(void);
    virtual void RTOsdClose(void);
    int RassImage(int QArchiv, int QKey, bool DirUp);
    virtual void RassOsd(void);
    virtual void RassOsdTip(void);
    virtual void RassOsdClose(void);
    virtual void RassImgSave(const char *size, int pos);
    virtual eOSState ProcessKey(eKeys Key);
    virtual bool IsInteractive(void) { return false; }
};

class cRTplusOsd : public cOsdMenu {
private:
    int bcount;
    int helpmode;
    const char *listtyp[7];
    char *btext[7];
    int rtptyp(char *btext);
    void rtp_fileprint(void);
public:
    cRTplusOsd(void);
    virtual ~cRTplusOsd();
    virtual void Load(void);
    virtual void Update(void);
    virtual eOSState ProcessKey(eKeys Key); 
};

class cRTplusList : public cOsdMenu {
private:
    int typ;
    bool refresh;
public:
    cRTplusList(int Typ = 0);
    ~cRTplusList();
    virtual void Load(void);
    virtual void Update(void);
    virtual eOSState ProcessKey(eKeys Key); 
};


// Radiotext-Memory RT+Classes 2.1
#define MAX_RTPC 50
struct rtp_classes {
    time_t start;
    char temptext[RT_MEL];
    char *radiotext[2*MAX_RTPC];
    int rt_Index;
    // Item
    bool item_New;
    char *item_Title[MAX_RTPC];     // 1
    char *item_Artist[MAX_RTPC];    // 4    
    time_t item_Start[MAX_RTPC];
    int item_Index;
    // Info
    char *info_News;                // 12
    char *info_NewsLocal;           // 13
    char *info_Stock[MAX_RTPC];     // 14
    int info_StockIndex;
    char *info_Sport[MAX_RTPC];     // 15
    int info_SportIndex;
    char *info_Lottery[MAX_RTPC];   // 16
    int info_LotteryIndex;
    char *info_DateTime;            // 24
    char *info_Weather[MAX_RTPC];   // 25
    int info_WeatherIndex;
    char *info_Traffic;             // 26
    char *info_Alarm;               // 27
    char *info_Advert;              // 28
    char *info_Url;                 // 29
    char *info_Other[MAX_RTPC];     // 30
    int info_OtherIndex;
    // Programme
    char *prog_StatShort;           // 31
    char *prog_Station;             // 32
    char *prog_Now;                 // 33
    char *prog_Next;                // 34
    char *prog_Part;                // 35
    char *prog_Host;                // 36
    char *prog_EditStaff;           // 37
    char *prog_Homepage;            // 39
    // Interactivity
    char *phone_Hotline;            // 41
    char *phone_Studio;             // 42
    char *sms_Studio;               // 44
    char *email_Hotline;            // 46
    char *email_Studio;             // 47
// to be continue...
};

// plugin audiorecorder service
struct Audiorecorder_StatusRtpChannel_v1_0 {
    const cChannel *channel;
    int status;
    /*
    * 0 = channel is unknown ...
    * 1 = no receiver is attached
    * 2 = receiver is attached
    * 3 = actual recording
    */
};
extern const cChannel *chan;

#endif //__RADIO_AUDIO_H
