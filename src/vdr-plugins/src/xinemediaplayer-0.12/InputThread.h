#ifndef CINPUTTHREAD_H
#define CINPUTTHREAD_H
/*
* InputThread.h:
*  Defines cInputThread class that reads a mrl (filepath / URL) 
*          and writes it into a ringbuffer.
*  Also calculates the length and mime-type of the mrl content.
**/
#include <vdr/ringbuffer.h>
#include <vdr/thread.h>
#include <vdr/tools.h>
#include <string>

struct ICYHeader_s
{
    // was "ICY 200 OK" already seen
    bool isICY;
     
    std::string notice1, notice2;
    std::string name, genre;
    std::string url, content_type;
    int pub;
    int br;

    bool SetVar(const std::string, const std::string);
    void Reset();
};

enum eMrlType {eMrlUnknown, eMrlUrl, eMrlFile};

enum eInputThreadStatus { eReadingMrl, eReadError,
     eReadAbort /*to diff. btwn reading stopped or reading ended 
             in the former case start next mrl as quickly as possible
             if EOF reached then wait for the playback to complete*/,
    eReachedEOF, eWaitingForMrl, eGotMrl};
    
class cInputThread : public cThread
{
private:
    /* setting to true, indicates current mrl reading stops and 
     * the mrl_ variable already has the new MRL*/
    bool newMrl_;

    std::string mrl_;
    cRingBufferLinear* Buffer_;
    cMutex* Mutex_;

    bool readFirstByte;
    uint64_t mrlDuration_;

    eMrlType mrlType_;
    
    /* stores the ICY header information*/
    ICYHeader_s icyHeader;

    /* if mrl is a file, then use libmagic to get the MIME format 
     * of the contents of the file*/
    std::string mrlFileFormat;

    /* Input thread status, when in eReachedEOF PlayerThread 
     * should start looking for an mrl and set this status to eWaitingForMrl
     * Also, after a read error, eg. unable to open an mrl: status is eReachedEOF*/
    eInputThreadStatus Status_;
    void SetStatus(eInputThreadStatus status);
    eMrlType SetMrlType();
    void GetMrlFormatFromFile();

    // when was the last data got?
    // useful for internet streams
    cTimeMs lastDataGot;

public:
    cInputThread(std::string, cRingBufferLinear*, cMutex*);
    
    cInputThread();
    // sets mrl_ to input param and newMrl_ = true

    ~cInputThread();

    void Init(std::string, cRingBufferLinear*, cMutex*);
    // does the same as the parametrized constructor


    bool NewMrl(std::string);
    // copys mrl to mrl_ if they are different
    // starts thread if mrl_ is not empty and thread is not running

    bool StopInput();
    // stops reading data from current mrl,
    // if Running() is false or newMrl_ is set
    // so that the next mrl can be read for data

    void Action();
    // all action in thread here!

    bool HasData()const { return readFirstByte;}

    eInputThreadStatus Status()const;
    // returns status of the thread, 
    // used to signal EOF or "reading is over"

    void StopThread();
    // sets running = false, so the thread stops gracefully

    bool IsActiveThread();
    // returns Running()

    bool DoFileRead();
    // reads a file and adds to Buffer_ (ringbuffer)

    bool DoCurlRead();
    // reads an URL and adds to Buffer_ (ringbuffer)

    static size_t curl_progress_callback(void *userp, double,
        double , double, double);
    // aborts curl transfer if StopInput() == true
    // this function is called every second or so (regularly, unlike write_callback)
    // params not used

    static size_t curl_write_callback(void *buffer, size_t size,
        size_t nmemb, void *userp);
    // writes into buffer: called via DoCurlRead()
    // can abort transfer if StopInput() == true
    // Is not called regularly, Only called when curl 
    // has enough data to write so progress_callback is used to stop 
    // incase this function is not called for a while
    

    void ParseForICYHeader (const char* buffer, size_t len,  const char* str);
    // looks for string 'str' in 'buffer' and assumes starting 
    // from 'str' till "\r\n" is a ICY header

    std::string MrlFormatType()const;
    // mime type is returned
    // either from libmagic as is in the case of a file or
    // "content-type" in case of shoutcast stream (ICY header)

    uint64_t MrlDuration()const{return mrlDuration_; }
    // length of mrl played in seconds
    
    eMrlType MrlType() const {return mrlType_;}
    // URL or File ?
    
    //uint64_t BytesRead()const;
};

extern int new_mrl_opened;

#endif // CINPUTTHREAD_H
