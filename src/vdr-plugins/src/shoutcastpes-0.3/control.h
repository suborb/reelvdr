#ifndef MY_CONTROL_H
#define MY_CONTROL_H

#include <vdr/player.h>
#include <vdr/thread.h>
#include <vdr/tools.h>

extern cString mrl;

class cMyPlayer: public cPlayer, cThread
{
    private:
        bool closePlayer;
    protected:
        void Activate(bool);
        void Action();
        int PlayAsPesPackets(uchar* Data, int len, uint64_t pts, float& duration);

    public:
        cMyPlayer();
        ~cMyPlayer();

        void StopPlayback();
        bool Active() {return cThread::Running();}
        bool ClosePlayer()const {return closePlayer;}
};

class cMyControl : public cControl
{
    private:
        cMyPlayer *player;
    public:
        cMyControl();
        ~cMyControl();

        eOSState ProcessKey(eKeys);
        void Hide();
};


static const int mpa_bitrate_table[2][3][15] = {
    {{0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384 },
        {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 }},
    {{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}}
};

static const int mpa_samplerate_table[] = {44100, 48000, 32000, -1};

typedef struct mpa_frame_s
{
    size_t len;
    int layer;
    int id;

    int lsf;  // low freq sampling
    int mp25; // mpeg 2.5
    
    /*bit rate index*/
    int bri;
    int bitrate;
    
    /*sample rate index*/
    int sri;
    int samplerate; 

    int samples_per_frame;

    int padding;
    int no_protection;

    float duration;
    
} mpa_frame_t;
int find_valid_mpa_frame(const uchar*, size_t, mpa_frame_t*);
#endif
