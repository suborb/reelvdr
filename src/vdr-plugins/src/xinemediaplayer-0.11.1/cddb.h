
#ifndef _XINEMEDIAPLAYER_CDDB_H_
#define _XINEMEDIAPLAYER_CDDB_H_

#include <cddb/cddb.h>
#include <cdio/cdio.h>

#include <vector>
#include <string>

#include <vdr/tools.h> // esyslog

using namespace std;
//#define esyslog(...) ;
class cTrack;
class cCdData;
class cCdInfo;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Class holds CD track data: title,length, extended Data 
////////////////////////////////////////////////////////////////////////////////////////////////////
class cTrack
{
  friend class cCdData;
  friend class cCdInfo;
    
  private:
        int     num_;    // track number
        string  title_;
        string  artist_;
        int     length_; // seconds
        string  extData_;

    public:
        cTrack(const int n, const string t,const string a, const int len, const string  eData);
        int     Number()     const;
        int     Length()     const;
        string  Title()      const;
        string  Artist()     const;
        string  ExtData()    const;
  private:
        void    SetNumber(int num);
        void    SetLength(int length);
        void    SetTitle(string);
        void    SetExtData(string);
        void    SetArtist(string);

}; //end cTrack


////////////////////////////////////////////////////////////////////////////////////////////////////
// Class holds CD data: title, tracks, CD length, genre, extended Data 
// and the data of tracks
////////////////////////////////////////////////////////////////////////////////////////////////////
class cCdData
{
  friend class cCdInfo;
  
    private:
        string artist_;
        string title_;
        string genre_;
        string extData_;
        int trackCount_;
        int year_;
        int length_;

        vector <cTrack> tracks;
        
        void Clear();

    public:
        int TrackCount()const;
        int Year()const;
        int Length()const;

        string Title() const;
        string Artist() const;
        
        string Genre() const;
        string ExtData()const;
        
        ~cCdData();

  private:
        // set functions
        void    SetTrackCount(int);
        void    SetYear(int);
        void    SetLength(int);
        
        void    SetTitle(string);
        void    SetArtist(string);
        
        void    SetGenre(string);
        void    SetExtData(string);

        // add & query track information
        void AddTrack(const int trackNum, const string trackTitle, const string trackArtist, const int trackLen, const string trackExtData);
        

};//end cCdData


////////////////////////////////////////////////////////////////////////////////////////////////////
// singleton class that is used by xine mediaplayer to read and get Audio CD info
////////////////////////////////////////////////////////////////////////////////////////////////////
class cCdInfo
{
    private:
        cCdData cdData;
        cddb_disc_t *disc; //libcddb disc structure

    public:
        string  TrackTitleByIndex(int index);
        string  TrackArtistByIndex(int index);
        int     TrackNumberByIndex(int index);
        int     TrackCount();
        
        ~cCdInfo();
    private:
        void CreateCd(int dlength, int tcount, int *foffset);
        void ReadAudioCd(const char*dev=NULL);
        void QueryCddbServer();
        void ExtractCdInfo();
                int  ExtractCdText();
        
        cCdInfo();
        cCdInfo(const cCdInfo&);
    
    static cCdInfo *instance;
    public:
        static cCdInfo& GetInstance();
        int AudioCdInit();
};

#endif
