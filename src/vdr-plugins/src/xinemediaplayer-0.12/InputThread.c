#include "InputThread.h"
#include <iostream>
#include <curl/curl.h>
#include <magic.h>
#include <unistd.h>

#include <vdr/tools.h>
#include <vdr/skins.h>

int mpa_file_length(int fd, int* len); 
int new_mrl_opened = 0;

bool MrlIsUrl(std::string mrl)
{
   return  mrl.find("http://")!=std::string::npos;
}

bool MrlIsFile(std::string mrl)
{
    //TODO check read access
    std::cerr<<"Got mrl: '"<<mrl<<"' returning "<<!mrl.empty()<<std::endl;

    return !mrl.empty() && (access(mrl.c_str(), R_OK) == 0);
}

void parse_icy_response(char*p, const char**icy_cmd, const char**icy_data)
{
    if(!p || !icy_cmd || !icy_data) return;
    char *s = strchr(p, ':');
    if (s)
    {
        *s = '\0';
        if(s != p)
            *icy_cmd    = p;

        *icy_data   = s + 1;
    }
}

/* find where ICY Header ends.
   ICY Header ends with an "\r\n\r\n"
   
   See "SHOUTcast Streaming Standard"
   http://web.archive.org/web/20071013123807/http://forums.radiotoolbox.com/viewtopic.php?t=74
*/
int ParseForICYHeaderEnd(const char*p , size_t len, const char*str)
{
    size_t i = 0;
    size_t j = 0;
    size_t str_len = strlen(str);

    while (i < len)
    {
        if(p[i] == str[j]) j++;
        else j = 0;

        ++i;

        if(j == str_len)
            return i - j;

    }

    return -1;
}


// Init variable of ICYHeader_s struct
void ICYHeader_s::Reset()
{
    isICY = false;
    notice1.clear();
    notice2.clear();
    name.clear();
    genre.clear();
    url.clear();
    content_type.clear();
    pub = 0;
    br = 0;
}

// give header string and header value, set the appropriate variable
bool ICYHeader_s::SetVar(const std::string header_str, const std::string val_str)
{
    if (header_str.empty()) return false;

    if (header_str.find("notice1") != std::string::npos)
        notice1 = val_str;
    else if (header_str.find("notice2") != std::string::npos)
        notice2 = val_str;
    else if (header_str.find("name") != std::string::npos)
        name = val_str;
    else if (header_str.find("genre") != std::string::npos)
        genre = val_str;
    else if (header_str.find("url") != std::string::npos)
        url = val_str;
    else if (header_str.find("content-type") != std::string::npos)
        content_type = val_str;
    else if (header_str.find("pub") != std::string::npos)
        pub = 1;
    else if (header_str.find("br") != std::string::npos)
        br = 128;
    else if (header_str.find("ICY 200") != std::string::npos)
        isICY = true;
    else return false;

    return true;
}


///////////////////////////////////////////////////////////////////////////
// cInputThread class
// Reads given mrl and writes into ringbuffer for player to play
///////////////////////////////////////////////////////////////////////////

cInputThread::cInputThread()
{
    mrl_            = std::string(); 
    mrlDuration_    = 0;
    Buffer_         = NULL;
    Mutex_          = NULL;
    newMrl_         = false;
    icyHeader.Reset();
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
    mrl_          = mrl;
    newMrl_       = true;
    Buffer_       = buffer;
    Mutex_        = mutex;
    mrlDuration_  = 0;
    icyHeader.Reset();
}

void cInputThread::StopThread()
{
    //Set running to false, so clean thread exit
    printf("StopThread: cInputThread waiting for 9sec\n");
    esyslog("[xinemediaplayer] cInputThread::StopThread:(%s:%d) Cancel(9)", __FILE__,__LINE__);
    Cancel(9);
}

bool cInputThread::IsActiveThread()
{
    return Running();
}

void cInputThread::SetStatus(eInputThreadStatus status)
{
    Status_ = status;
}

eInputThreadStatus cInputThread::Status() const
{
    return Status_;
}

// returns mime type: eg audio/mpeg
std::string cInputThread::MrlFormatType()const
{
    switch (MrlType())
    {
        case eMrlUrl:
            return icyHeader.content_type;

        case eMrlFile:
            return mrlFileFormat; //got the opinion of libmagic

        default:
            return std::string("Unknown format");
    }

    return std::string("Unknown format");
}

bool cInputThread::NewMrl(std::string mrl)
{
    dsyslog("[xinemediaplayer] cInputThread::NewMrl(%s) (%s:%d)", mrl.empty()?"(empty)":mrl.c_str(), __FILE__,__LINE__);

    if ( !mrl.empty() && mrl_ != mrl)
    {
        mrl_    = mrl;
        newMrl_ = true;
        SetStatus(eGotMrl);
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

eMrlType cInputThread::SetMrlType()
{
    //std::cerr<<"SetMrlType() \t"<<mrl_<<std::endl;
    if (MrlIsUrl(mrl_))
    {
        mrlType_ = eMrlUrl;
    //    std::cerr<<"mrl type set to eMrlFile"<<std::endl;
    }
    else if (MrlIsFile(mrl_))
    {
        mrlType_ = eMrlFile;
    //    std::cerr<<"mrl type set to eMrlFile"<<std::endl;
    }
    else 
    {
        mrlType_ = eMrlUnknown;
     //   std::cerr<<"mrl type set to eMrlFile"<<std::endl;
    }

    return mrlType_;
}

bool cInputThread::StopInput()
{
    return newMrl_ || !Running();
}


void cInputThread::Action()
{
    while(Running())
    {
        // mrl obsolete or empty
        if (Status() == eReachedEOF || !newMrl_ || mrl_.empty())
        {
            //send signal here
            cCondWait::SleepMs(10);
            continue;
        }

        dsyslog("[xinemediaplayer] (%s:%d) Action() reading new mrl: %s", 
                __FILE__, __LINE__, mrl_.empty()?"(empty)":mrl_.c_str());

        newMrl_ = false;
        Status_ = eGotMrl;
    readFirstByte = false;

        //XXX: empty Buffer_
        Mutex_->Lock();
    int avail = Buffer_->Available();
        printf("Clearing %i bytes from buffer\n", Buffer_->Available());
        Buffer_->Clear();
    dsyslog("%i ------ Buffer Cleared ------ %i", avail, Buffer_->Available());

        new_mrl_opened = 1;
        Mutex_->Unlock();


#if 0
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
#else 
        switch (SetMrlType())
        {
            case eMrlUrl:
                std::cerr<<"mrl: '"<<mrl_ << "' is url"<<std::endl;
                DoCurlRead();
                break;

            case eMrlFile:
                std::cerr<<"mrl: '"<<mrl_ << "' is file"<<std::endl;
                DoFileRead();
                break;

            default:
            case eMrlUnknown:
                std::cerr<<"Cannot handle Mrl '"<<mrl_<<"': Unknown Type"<<std::endl;

                esyslog("[xinemediaplayer] InputThread (%s:%d) Cannot handle Mrl='%s': unknown type=%i",
                        __FILE__, __LINE__, mrl_.c_str(), MrlType());

                break;
        }// switch
#endif
        Status_ = StopInput()?eReadAbort: eReachedEOF;
        //no more mrls to handle || invalid mrl
        // ask for new mrl
        printf("**** --------- Need new mrl ---------- ****** \n\n");

        dsyslog("[xinemediaplayer] (%s:%d) Action() reading Stopped: %s. %s", 
                __FILE__, __LINE__, 
                StopInput()?"Aborted":"EOF reached",
                Running()?"Waiting for new mrl.":"Not running.");

    }//while

    std::cerr<<"cInputThread::Action() ended"<<std::endl;

}//Action()

/* use libmagic to get the mime type of the file
 * and store it in mrlfileformat
 */
void cInputThread::GetMrlFormatFromFile()
{
    magic_t cookie;
#ifdef MAGIC_MIME_TYPE // when defined gives "audio/mpeg" and MAGIC_MIME gives "audio/mpeg; charset=binary" eg. Ubuntu 10.04
    cookie = magic_open(MAGIC_MIME_TYPE); 
#else
    cookie = magic_open(MAGIC_MIME); //in Ubuntu 8.04 this gives "audio/mpeg" without the encoding type
#endif

    magic_load(cookie, "/usr/share/file/magic");
    const char* mime_str =NULL;
    mime_str = magic_file(cookie, mrl_.c_str());

    if (mime_str)
    {
        printf("Got '%s'\n", mime_str);
        mrlFileFormat = mime_str;
    dsyslog("mime-type:'%s' of mrl: '%s'", mime_str, mrl_.c_str());
    }
    else
    {
        printf("Error '%s'\n", magic_error(cookie));
        mrlFileFormat.clear();
    dsyslog("[xinemediaplayer] ERROR: mime-type of mrl: '%s' NOT found", mrl_.c_str());
    }

    magic_close(cookie);
}

bool cInputThread::DoFileRead()
{
    // get mime type of contents of mrl
    GetMrlFormatFromFile();

    // check file format/mime type
    // currently, only 'audio/mpeg' can be played
    if(mrlFileFormat != "audio/mpeg")
    {
        std::cerr<<"********\n Unplayable mime-type '"<<mrlFileFormat
            <<"'. Can play *only* 'audio/mpeg'\n*********"<<std::endl;

        Skins.QueueMessage(mtError, 
                cString::sprintf("%s '%s' %s",tr("Cannot play"), mrlFileFormat.c_str(), tr("file type")), 2,  -1);
    esyslog("[xinemediaplayer]:InputThread cannot handle mime-type ('%s') of mrl :'%s'", 
            mrlFileFormat.c_str(), mrl_.c_str() );
        return false;
    }

    int fd = open(mrl_.c_str(), O_RDONLY);
    printf("DoFileRead() fd = %i\n", fd);

    if(fd<0)
    {
        // could not open file
        std::cerr<<"Could not open mrl: "<<mrl_ <<std::endl;
        
        // thread status is set in Action() every iteration of loop
        // so no need to set it here
        return false;
    }

    SetStatus(eReadingMrl);

    printf("-------------- Calculating file length ---------------\n");
    //find file length
    int length_sec = 0;
    mpa_file_length(fd, &length_sec);
    printf("---------------- %i secs ---------------\n", length_sec);

    if(length_sec > 0)
        mrlDuration_ = (uint64_t) length_sec;
    else
        mrlDuration_ = 0;

    /* length calc traverses through the whole file,
     * so rewind to get the first frame */
    lseek(fd, SEEK_SET,0L); //rewind

    int count = 0;
    int avail = 0;
    bool end_of_file = false;
    while(Running())
    {
        if(StopInput())
        {
            std::cerr<<"DoFileRead(): stopping read loop in"<<std::endl;
            break;
        }

        // XXX: if Buffer_ is full cCondWait::SleepMs(10) ?

        Mutex_->Lock();
        count = Buffer_->Read(fd);
        avail = Buffer_->Available();
        //printf("Read %i bytes: Available: %i (%i)\n", count, avail, Buffer_->Free());
        if(count <= 0 && Buffer_->Free()>0)
            end_of_file = true;

    if (count > 0 && !readFirstByte)
    {
        readFirstByte = true;
        dsyslog("-------------- Read first BYTE ----------------");
    }

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

    SetStatus(eReadingMrl);
    icyHeader.Reset();
    lastDataGot.Set(0);

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

    if (thisPtr->lastDataGot.Elapsed() > 15000)
    {
       Skins.QueueMessage(mtInfo, tr("Waiting for server ..."), 2, -1);
    }
     else 
    {
       // remove all messages from queue
       Skins.QueueMessage(mtInfo, NULL);
    }
    

    // non-zero value aborts transfer
    return thisPtr->StopInput();
}

size_t cInputThread::curl_write_callback(void *buffer, size_t size,
        size_t nmemb, void *userp)
{
    cInputThread *thisPtr = static_cast<cInputThread*> (userp);

    if(thisPtr->StopInput())
        return !(size*nmemb);

    thisPtr->ParseForICYHeader( (const char*)buffer, size*nmemb, "icy");
    thisPtr->ParseForICYHeader( (const char*)buffer, size*nmemb, "content");
    //std::cerr<<"write callback with "<<size*nmemb<<"bytes"<<std::endl;
    //write into buffer
    thisPtr->Mutex_->Lock();
    int count = thisPtr->Buffer_->Put((uchar*) buffer, size*nmemb);
    if (count > 0 && !thisPtr->readFirstByte)
    {
        thisPtr->readFirstByte = true;
        dsyslog("[xinemediaplayer] DoCurlRead ---------- First Byte ------------- ");
    }
    thisPtr->Mutex_->Unlock();
    thisPtr->lastDataGot.Set(0);

    return size*nmemb;
}


/* look for ICY header in the given buffer
*  const char* str point to the initial few characters of a "ICY header" 
*  eg "content-type": then find this string and starting from the string 
*  to a "\r\n" character is a "ICY header"
*/
void cInputThread::ParseForICYHeader (const char* buffer, size_t len,  const char* str)
{
    size_t i = 0;
    size_t str_len = strlen(str);
    size_t j = 0;

    // loop through 'buffer' to find 'str'
    while(i < len) 
    {
        if(tolower(buffer[i]) == str[j]) j++;
        else j = 0;

        i++;

        //string found at i - str_len
        if(j == str_len)
        {
            //find end of line
            size_t k = i;
            while( k < len && buffer[k] != '\r') ++k;

            //found end of line
            if(k<len && buffer[k] == '\r')
            {
                size_t found_str_len = k - (i-str_len) ;
                char *p = new char[found_str_len+1];

                memcpy(p, buffer+(i-str_len), found_str_len);
                p[found_str_len] = 0;

                //printf("'%s'\n", p);
                const char* pre  = NULL;
                const char* data = NULL;
                parse_icy_response(p, &pre, &data);
                if (pre)
                {
                    icyHeader.SetVar(pre, data?data:std::string());
                    printf("'%s' --> '%s'\n", pre, data);
                }

                delete [] p;

                i = k;
            } //if
        } //if
    } //while

}// ParseForICYHeader()
