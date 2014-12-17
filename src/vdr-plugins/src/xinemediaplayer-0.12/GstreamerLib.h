#ifndef GSTREAMERLIB_H
#define GSTREAMERLIB_H

#include <gst/gst.h>
#include <gst/interfaces/navigation.h>
#include <string>
#include <vector>

#define GSTLIB_AUDIO_TAG    0x0001
#define GSTLIB_VIDEO_TAG    0x0002
#define GSTLIB_SUBTITLE_TAG 0x0004

namespace Reel {
    class GstLib {
    private:
        /// playbin2 element/pipeline
        GstElement *player;

        /// bus associated with playbin2; poll this for messages
        // such as EOS, ERROR, TAG etc
        GstBus     *bus;

        /// Tag-message that are sent in bus are collected in this data structure
        // eg. Title, Artist, Album etc
        // TODO in some cases refresh tags after every few minutes.
        // some case := eg.internet radio
        GstTagList *tagCache;

        /// stream info
        // current playing position of stream
        gint64 position;
        //length of stream;
        gint64 duration;// duration==0 => stream not seekable

        /// target state of gstPlayer
        // gstPlayer pipeline may be in buffering mode etc.
        // this var holds what the user/intended state player must be in
        GstState targetState;

        bool newUriWanted;

        /// Error and warning strings
        std::string errorStr;
        std::string warningStr;

        // new audio/video/subtitle tags available?
        // holds GSTLIB_VIDEO_TAG | GSTLIB_AUDIO_TAG | GSTLIB_SUBTITLE_TAG
        int new_available_tag;

    public:

        GstLib();

        ~GstLib();


        // call gst_init(). initialize player, bus, tagCache etc
        void Init();

        // sets player's state to NULL so that all elements may
        // initialize their internal state (else playback of second file stalls)
        // set 'uri' of playbin/gstPlayer and calls Play()
        void PlayMrl(const char* mrl);

        // periodically handle messages in bus
        // calls gst_bus_pop() and handles End of stream/Error/Tags etc
        // http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/GstBus.html#gst-bus-pop
        void HandleBusMessages();
        void HandleGstTags(GstTagList* tags);


        /// query duration and position
        // querys Gstreamer core and updates 'position' and 'duration'
        // member variables
        // returns true if both queries were successful
        bool QueryPositionDuration();
        int Position() const;
        int Duration() const;

        // total dvd chapters and titles
        int DVDTitleCount() const;
        int DVDChapterCount() const;

        // current DVD title and chapter number
        int DVDTitleNumber() const;
        int DVDChapterNumber() const;
        
        // calls QueryPositionDuration() and "returns" pos and length in seconds
        bool GetIndex(int &pos, int &length);


        // Pause player
        void Pause();
        // change state of player to PLAY
        void Play();
        // Stop player
        void Stop();


        /// Skip/Seek
        // jump relative to current position
        // time_secs can be +ve/-ve
        void SeekRelative(int time_secs);
        //jump to absolute position (time_secs >= 0)
        void SeekAbsolute(int time_secs);
        //jump to title
        void SeekTitle(int title_num);
        //jump to chapter
        void SeekChapter(int chap_num);

        /// fastforward/fastrewind
        // playback_rate = 1.0 Normal playback
        //               > 1.0 fastforward
        //               < -1.0 fastrewind
        void SetPlaybackRate(double playback_rate);
        // gets playback rate from pipeline
        double PlaybackRate() const;


        /// volume & mute
        bool IsMute() const;

        // mute if IsMute() is false else unmute gst
        void ToggleMute();

        // On=true => mute
        void SetMute(bool On);

        // sets volume level of gstreamer: percent between 0-100
        void SetVolume(unsigned int percent);

        // returns gst's volume [0-100]
        int Volume() const;


        /// properties from pipeline
        // look into 'n-video', 'n-audio', 'n-text' props of (only) playbin2
        int VideoStreamCount() const;
        int AudioStreamCount() const;
        int SubtitleStreamCount() const;

        // look into current-(video/audio/text) props of playbin2
        // to get current indices of video/audio/subtitle streams
        int VideoStream() const;
        int AudioStream() const;
        int SubtitleStream() const;

        // Set Video/Audio/Subtitle stream
        // select the stream based on the index (parameter)
        void SetVideoStream(int idx);
        void SetAudioStream(int idx);
        void SetSubtitleStream(int idx);
        
        // returns true if *StreamCount() > 0
        bool HasVideo() const;
        bool HasAudio() const;
        bool HasSubtitle() const;

        /// Get string from tagCache GstTagList, used to collect metadata
        //  given a tag string like "title"/GST_TAG_TITLE etc
        std::string StringFromTag(const gchar* tagStr) const;

        /// Metadata from TAGS
        //Title, Album, Artist
        std::string Title()  const;
        std::string Album()  const;
        std::string Artist() const;

        //video and audio codecs
        std::string AudioCodec() const;
        std::string VideoCodec() const;

        /// audio/subtitle language list
        std::string AudioLang(int idx) const;
        std::string SubtitleLang(int idx) const;
        std::vector<std::string> AudioLangList() const;
        std::vector<std::string> SubtitleLangList() const;

        GstElement* SourceElement() const;
        // Source klass and factory name
        // eg. source element Factory klass='Source/Network'
        std::string SourceFactoryKlass() const;
        // eg. source element Factory name='HTTP client source'
        std::string SourceFactoryName() const;


        // is player in playing mode ?
        bool IsPlaying() const;
        bool IsPaused() const;
        bool EoS() const;
        bool UriWanted() const;

        std::string ErrorString() const;
        std::string WarningString() const;
        bool IsError() const; //non-empty error string ==> error



        /// Send command to gst core
        // http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-base-libs/html/gst-plugins-base-libs-gstnavigation.html#GstNavigationCommand
        bool SendGstCommand(GstNavigationCommand cmd);

        /// DVD Menu
        bool IsInDVDMenu() const;
        /// DVD menu navigation
        /// commands while replaying DVD
        bool DVDMenu();
        bool DVDRootMenu();
        bool DVDAudioMenu();
        bool DVDAngleMenu();
        bool DVDChapterMenu();
        bool DVDSubpictureMenu();
        bool DVDTitleMenu();

        /// Change angles if possible
        bool PrevAngle();
        bool NextAngle();

        /// Navigate Left/Right Up/Down
        bool NavigateLeft();
        bool NavigateRight();
        bool NavigateUp();
        bool NavigateDown();

        /// activate selected "button"
        bool OK_key();


        /// call back handling, modifies new_available_tag variable
        // these are called by callback functions as well as application
        // to 'reset' (set have seen tags by calling the following functions
        // with false)
        void SetVideoTagsChanged(bool);
        void SetAudioTagsChanged(bool);
        void SetSubtitleTagsChanged(bool);

        // look into 'new_available_tag' to see if specific tags are avaialble
        bool VideoTagsChanged() const;
        bool AudioTagsChanged() const;
        bool SubtitleTagsChanged() const;

    }; //class GstLib



// ---------------------- call back ------------------------------------------
    // these call back functions are set in PlayMrl() to respond to
    // signals got by playbin
    void audio_tags_changed_cb(GstElement* playbin,
                            gint stream_id,
                            gpointer user_data);

    void subtitle_tags_changed_cb(GstElement* playbin,
                            gint stream_id,
                            gpointer user_data);

    void video_tags_changed_cb(GstElement* playbin,
                            gint stream_id,
                            gpointer user_data);


// ------------ helper functions ----------------------------------------------

    // get the klass and name of factory that created the elements
    // eg. source element Factory name='HTTP client source' and klass='Source/Network'
    std::string ElementFactoryName(GstElement*);
    std::string ElementFactoryKlass(GstElement*);

} // namespace Reel

#endif // GSTREAMERLIB_H

