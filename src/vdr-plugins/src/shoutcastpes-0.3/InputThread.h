#ifndef CINPUTTHREAD_H
#define CINPUTTHREAD_H

#include <vdr/ringbuffer.h>
#include <vdr/thread.h>
#include <string>

class cInputThread : public cThread
{
private:
    bool newMrl_;
    std::string mrl_;
    cRingBufferLinear* Buffer_;
    cMutex* Mutex_;

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

    void StopThread();
    // sets running = false

    bool IsActiveThread();
    // returns Running()

    bool DoFileRead();
    // reads a file and adds to Buffer_

    bool DoCurlRead();
    // reads an URL and adds to Buffer_

    //aborts curl transfer if StopInput() == true
    //other params not used
    static size_t curl_progress_callback(void *userp, double,
        double , double, double);

    //writes into buffer: called via DoCurlRead()
    //can abort transfer if StopInput() == true
    static size_t curl_write_callback(void *buffer, size_t size,
        size_t nmemb, void *userp);

};

#endif // CINPUTTHREAD_H
