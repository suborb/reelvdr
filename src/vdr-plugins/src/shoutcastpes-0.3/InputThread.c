#include "InputThread.h"
#include <iostream>
#include <curl/curl.h>

bool MrlIsUrl(std::string mrl)
{
   return  mrl.find("http://")!=std::string::npos;
}

bool MrlIsFile(std::string mrl)
{
    //TODO check read access

    return !mrl.empty();
}


cInputThread::cInputThread()
{
    mrl_ = std::string(); 
    Buffer_ = NULL;
    Mutex_ = NULL;
    newMrl_ = false;
}

cInputThread::cInputThread(std::string mrl, cRingBufferLinear *buffer, cMutex* mutex)
{
    Init(mrl, buffer, mutex);
}

cInputThread::~cInputThread()
{
    printf("~cInputThread\n");

    if(Running())
        StopThread();
}

void cInputThread::Init(std::string mrl, cRingBufferLinear *buffer, cMutex* mutex)
{
    mrl_        = mrl;
    newMrl_     = true;
    Buffer_     = buffer;
    Mutex_      = mutex;
}

void cInputThread::StopThread()
{
    //Set running to false, so clean thread exit
    printf("StopThread: cInputThread waiting for 9sec\n");
    Cancel(9);
}

bool cInputThread::IsActiveThread()
{
    return Running();
}


bool cInputThread::NewMrl(std::string mrl)
{
    if ( !mrl.empty() && mrl_ != mrl)
    {
        mrl_    = mrl;
        newMrl_ = true;
    }
    else
        return false;

    // if thread not running, start thread
    // empty mrls are handled inside Action()
    if(!Running())
    {
        printf("Thread not running... starting\n");
        Start();
    }

    return true;
}


bool cInputThread::StopInput()
{
    return newMrl_ || !Running();
}


void cInputThread::Action()
{
    while(Running())
    {
        if (!newMrl_ || mrl_.empty())
        {
            //no more mrls to handle || invalid mrl
            // ask for new mrl
            printf("Need new mrl\n");

            //send signal here
            cCondWait::SleepMs(10);
            continue;
        }

        newMrl_ = false;

        //XXX: empty Buffer_
        Mutex_->Lock();
        printf("Clearing %i bytes from buffer\n", Buffer_->Available());
        Buffer_->Clear();
        Mutex_->Unlock();

        if(MrlIsUrl(mrl_))
        {
            std::cerr<<"mrl: '"<<mrl_ << "' is url"<<std::endl;
            DoCurlRead();
        }
        else if (MrlIsFile(mrl_))
        {
            std::cerr<<"mrl: '"<<mrl_ << "' is file"<<std::endl;
            DoFileRead();
        }
        else
        {
            std::cerr<<"Cannot handle Mrl '"<<mrl_<<"'"<<std::endl;
        }

    }//while

    std::cerr<<"cInputThread::Action() ended"<<std::endl;

}//Action()

bool cInputThread::DoFileRead()
{
    int fd = open(mrl_.c_str(), O_RDONLY);
    printf("DoFileRead() fd = %i\n", fd);

    if(fd<0)
    {
        // could not open file
        std::cerr<<"Could not open mrl: "<<mrl_ <<std::endl;
        return false;
    }

    int count = 0;
    int avail = 0;
    bool end_of_file = false;
    while(Running())
    {
        if(StopInput())
        {
            std::cerr<<"DoFileRead(): stopping read loop in"<<std::cerr;
            break;
        }

        // XXX: if Buffer_ is full cCondWait::SleepMs(10) ?

        Mutex_->Lock();
        count = Buffer_->Read(fd);
        avail = Buffer_->Available();
        //printf("Read %i bytes: Available: %i (%i)\n", count, avail, Buffer_->Free());
        if(count <= 0 && Buffer_->Free()>0)
            end_of_file = true;
        Mutex_->Unlock();

        if(end_of_file)
        {
            printf("EOF reached\n");
            break;
        }

        if(avail > 5000)
            cCondWait::SleepMs(10);
        else
            cCondWait::SleepMs(5);
    }

    close(fd);

    return true;
}

bool cInputThread::DoCurlRead()
{
    CURL* curl = curl_easy_init();
    CURLcode err;

    /** Set Curl opts */

    // URL
    err = curl_easy_setopt(curl, CURLOPT_URL, mrl_.c_str());


    // progress callback is called every 1 second and can be used to abort the
    // download quickly
    err = curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION,
                cInputThread::curl_progress_callback);
    // static function, else c++ mangled function names confuses curl lib

    err = curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
    // passed on to progress_callback so can be used call StopInput()

    // else progress callback is not called
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    // write call back
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                     cInputThread::curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);

    //curl easy perform
    err = curl_easy_perform(curl);

    //curl clean up
    curl_easy_cleanup(curl);

    return true;
}

size_t cInputThread::curl_progress_callback(void *userp, double dltotal,
        double dlnow, double ultotal, double ulnow)
{
    // this function is called periodically
    // so use this to abort transfer when required

    cInputThread *thisPtr = static_cast<cInputThread*> (userp);

    // non-zero value aborts transfer
    return thisPtr->StopInput();
}

size_t cInputThread::curl_write_callback(void *buffer, size_t size,
        size_t nmemb, void *userp)
{
    cInputThread *thisPtr = static_cast<cInputThread*> (userp);

    if(thisPtr->StopInput())
        return !(size*nmemb);

    //std::cerr<<"write callback with "<<size*nmemb<<"bytes"<<std::endl;
    //write into buffer
    thisPtr->Mutex_->Lock();
    thisPtr->Buffer_->Put((uchar*) buffer, size*nmemb);
    thisPtr->Mutex_->Unlock();

    return size*nmemb;
}

