#include "GstreamerLib.h"

namespace Reel {
// ---------------- GstLib ------------------------------------------------


    GstLib::GstLib() {
printf("GstLib::GstLib<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
#if 0
        // init member variables
        player = NULL;
        bus    = NULL;

        newUriWanted = true;
        tagCache = NULL;

        //Init();
if (!g_thread_supported ()) {
    g_thread_init (NULL);
}
#endif
    } // GstLib()


    GstLib::~GstLib()
    {
#if 0
        Stop();

        // unref bus and player, so that they can be freed
        if (bus) {
            gst_object_unref(bus);
            bus = NULL;
        }

        if (player) {
GstStateChangeReturn ret;
ret = gst_element_set_state(player, GST_STATE_NULL);
if (ret != GST_STATE_CHANGE_SUCCESS) g_print("%s returned %d\n", __PRETTY_FUNCTION__, ret);

            gst_object_unref(player);
            player = NULL;
        }

        if (tagCache) {
            gst_tag_list_free(tagCache);
            tagCache = NULL;
        }
gst_deinit();
        
#endif
    }


    /** initialize gst, player, bus, tags
     *  player is paused, since the uri is still not set
     */

    void GstLib::Init(bool askNewUri) {
        // initialize gstreamer library
        // TODO -- try to init only once; when starting the plugin.
        gst_init(NULL, NULL);

        // create player element
        player = gst_element_factory_make ("playbin2", "playerplaybin2");
        //set output plugins here, video/audio XXX

        // get bus of the player
        bus = gst_pipeline_get_bus (GST_PIPELINE (player));

        newUriWanted = askNewUri;

    } // Init()



    /** sets pipeline state to NULL (thereby initializing all elements)
     *  and sets mrl/uri of player and calls Play()
     */

    void GstLib::PlayMrl(const char *mrl) {

        if (!player) return;

        // in preparation for new uri playback,
        // let the elements reset their internal states
        gst_element_set_state(player, GST_STATE_NULL);

        g_print("trying to play %s\n", mrl);

        // set uri of player
        g_object_set (G_OBJECT (player), "uri", mrl, NULL);

        gchar* uri;
        g_object_get(G_OBJECT(player), "uri", &uri, NULL);
        g_print("uri=%s\n", uri);

        newUriWanted = false;

        // clear warning and error strings
        warningStr.clear();
        errorStr.clear();

        // watch for new audio/video/subtitle tags
        g_signal_connect (player, "video-tags-changed",
            G_CALLBACK (video_tags_changed_cb), this);

        g_signal_connect (player, "audio-tags-changed",
            G_CALLBACK (audio_tags_changed_cb), this);

        g_signal_connect (player, "text-tags-changed",
            G_CALLBACK (subtitle_tags_changed_cb), this);

        // player can start playing
        Play();

    } // PlayMrl()



    /** get messages from bus and act on them
    //   this function is called often (every ~1 second)
    //   it should exit as quickly as possible
    */

    void GstLib::HandleBusMessages() {

        if (!bus) return;

        GstMessage *msg;

        // read bus till all messages are got
        // XXX does this take a lot of time ?
        // should try timeout ?
        while (msg = gst_bus_pop(bus)) {

            if(msg) {

                switch (GST_MESSAGE_TYPE(msg)) {

                case GST_MESSAGE_EOS: // End of stream
                    newUriWanted = true;
                    break;

                case GST_MESSAGE_WARNING: {

                    GError *error = NULL;
                    gchar *debug = NULL;

                    gst_message_parse_warning(msg, &error, &debug);

                    //store warning message
                    warningStr = GST_STR_NULL (error->message);

                    g_print("Warning message: %s [%s]", GST_STR_NULL (error->message),
                        GST_STR_NULL (debug));

                } // Warning
                     break;

                case GST_MESSAGE_ERROR: {
                    // XXX player cannot continue playing
                    newUriWanted = true;

                    GError *error = NULL;
                    gchar *debug = NULL;

                    gst_message_parse_error (msg, &error, &debug);

                    //store error message
                    errorStr = GST_STR_NULL (error->message);

                    g_print("Error message: %s [%s]", GST_STR_NULL (error->message),
                        GST_STR_NULL (debug));
                }
                    break;


                case GST_MESSAGE_TAG: //artist, title, album
                {
                    GstTagList *tags = NULL;

                    gst_message_parse_tag(msg, &tags);
                    g_print ("Got tags from element %s\n", GST_OBJECT_NAME (msg->src));

                    //cache||handle tags
                    HandleGstTags(tags);

                    gst_tag_list_free(tags);
                }
                    break;


                case GST_MESSAGE_BUFFERING:
                {
                    /// TODO
                    // if playing from streaming source and buffer < 100%
                    // should pause playing and buffering >= 100% should start
                    // playing
                    gint percent;

                    // get buffering progress from message
                    gst_message_parse_buffering(msg, &percent);

                    //g_print("MSG Buffering %d%%\n", percent);
                }
                    break;

                default:
                    break;

                } //switch msg type

            } // if msg

        } //while

    } // HandleBusMessage()



    /** handle tag list got from bus */

    void GstLib::HandleGstTags(GstTagList* tags) {

        /// Put the new tags in the "front"
        // eg. internet radio sends "TITLE" tag quite often and only the latest
        // is the "correct" title
        GstTagList* result_tag = gst_tag_list_merge(tagCache,
                                                    tags,
                                                    GST_TAG_MERGE_PREPEND);


        if (tagCache)
            gst_tag_list_free(tagCache);

        tagCache = result_tag;

    } // HandleGstTags()




    /** query player and set 'position' and 'duration' variables
    */

    bool GstLib::QueryPositionDuration() {

        if (!player) return false;

        GstFormat fmt = GST_FORMAT_TIME; // in nanoseconds

        // if either query has error, reset position and duration
        if (!gst_element_query_position(player, &fmt, &position)
                || !gst_element_query_duration(player, &fmt, &duration))
        {
            position = 0;
            duration = 0;
            
            return false;
        }
        
        return true;

    } // QueryPositionDuration()



    int GstLib::DVDTitleCount() const {

        if (!player) return 0;

        GstFormat fmt = gst_format_get_by_nick("title");

        // no such format available
        if (fmt == GST_FORMAT_UNDEFINED)
            return 0;

        gint64 titleCount = 0;
        gst_element_query_duration(player, &fmt, &titleCount);

        g_print("Title Count: %lld\n", titleCount);
        return titleCount;

    } // DVDTitleCount()


    int GstLib::DVDChapterCount() const {

        if (!player) return 0;

        GstFormat fmt = gst_format_get_by_nick("chapter");

        // no such format available
        if (fmt == GST_FORMAT_UNDEFINED)
            return 0;

        gint64 chapterCount = 0;
        gst_element_query_duration(player, &fmt, &chapterCount);

        g_print("Chapter Count: %lld\n", chapterCount);
        return chapterCount;

    } // DVDChapterCount()



    int GstLib::DVDTitleNumber() const {

        if (!player) return 0;

        GstFormat fmt = gst_format_get_by_nick("title");

        // no such format available
        if (fmt == GST_FORMAT_UNDEFINED)
            return 0;

        gint64 titleNum = 0;
        gst_element_query_position(player, &fmt, &titleNum);

        g_print("Current Title: %lld\n", titleNum);
        return titleNum;

    } // DVDTitleNumber



    int GstLib::DVDChapterNumber() const {

        if (!player) return 0;

        GstFormat fmt = gst_format_get_by_nick("chapter");

        // no such format available
        if (fmt == GST_FORMAT_UNDEFINED)
            return 0;

        gint64 chapterNum = 0;
        gst_element_query_position(player, &fmt, &chapterNum);

        g_print("Current chapter: %lld\n", chapterNum);
        return chapterNum;

    } // DVDChapterCount()





    /** send position and length in seconds
    */

    bool GstLib::GetIndex(int &pos, int &len) {

        if (!player) return false;

        QueryPositionDuration();

        // convert values from nano-seconds to seconds
        pos = position/GST_SECOND;
        len = duration/GST_SECOND;

        return true;

    } // GetIndex()



    int GstLib::Position() const {

        return position;

    } // Position()


    int GstLib::Duration() const {

        return duration;

    } // Duration()



    /** Seek to absolute position
    *   as measured from the start of the stream
    */
    void GstLib::SeekAbsolute(int time_secs) {

        if (!player) return;

        gst_element_seek (player,
                          1.0 /*playback rate*/,
                          GST_FORMAT_TIME,
                          GST_SEEK_FLAG_FLUSH
                          /* flush pipeline, non-flush might be slow */
                          //(GST_SEEK_FLAG_FLUSH|GST_SEEK_FLAG_KEY_UNIT)
                          /*seek to the nearest keyframe. faster but less accurate*/,
                          GST_SEEK_TYPE_SET, /*abs seek*/
                          time_secs * GST_SECOND,
                          GST_SEEK_TYPE_NONE,
                          GST_CLOCK_TIME_NONE);

    } // SeekAbsolute()


    /** Seek to position relative to current position
    */
    void GstLib::SeekRelative(int time_secs) {
        int pos, len;

        // first get the current position and
        // then jump to cur_pos + time_secs to do a relative seek
        QueryPositionDuration();

        GetIndex(pos, len);

        //if len == 0; unseekable
        SeekAbsolute(pos + time_secs);

    } // SeekRelative()



    // jump to a given title number
    void GstLib::SeekTitle(int title_num) {

        if (!player) return;

        GstFormat fmt = gst_format_get_by_nick("title");

        if (fmt == GST_FORMAT_UNDEFINED)
            return;

        gst_element_seek (player,
                          1.0 /*playback rate*/,
                          fmt,
                          GST_SEEK_FLAG_FLUSH,
                          GST_SEEK_TYPE_SET, /*abs seek*/
                          title_num,
                          GST_SEEK_TYPE_NONE,
                          GST_CLOCK_TIME_NONE);

    } // SeekTitle()



    // jump to a given chapter number
    void GstLib::SeekChapter(int chapter_num) {

        if (!player) return;

        GstFormat fmt = gst_format_get_by_nick("chapter");

        if (fmt == GST_FORMAT_UNDEFINED)
            return;

        gst_element_seek (player,
                          1.0 /*playback rate*/,
                          fmt,
                          GST_SEEK_FLAG_FLUSH,
                          GST_SEEK_TYPE_SET, /*abs seek*/
                          chapter_num,
                          GST_SEEK_TYPE_NONE,
                          GST_CLOCK_TIME_NONE);

    } // SeekChapter()



    /** Fast forward and fast rewind
      * playback_rate should be >= 1.0 OR <= -1.0 AND != 0.0
      */
    void GstLib::SetPlaybackRate(double playback_rate) {

        if (!player) return;

        // Gst asserts rate != 0;
        if (playback_rate == 0.0)
            return;

        // mute when not playing at normal rate
        // playback_rate == 1 ==> unmute
        // playback_rate != 1 ==> mute

        // make sure audio is played only when playback rate == 1

        // mute before playback rate != 1
        if (playback_rate != 1)
            SetMute(true);

//        gst_element_seek (player,
//                          playback_rate /*playback rate*/,
//                          GST_FORMAT_TIME,
//                          GstSeekFlags (GST_SEEK_FLAG_FLUSH /* flush pipeline, non-flush might be slow */
//                          | GST_SEEK_FLAG_SKIP) /*skip some frames*/,
//                          GST_SEEK_TYPE_NONE, /*no change in position*/
//                          GST_CLOCK_TIME_NONE,
//                          GST_SEEK_TYPE_NONE,
//                          GST_CLOCK_TIME_NONE);

        // get the current position
        QueryPositionDuration();

        // change playback rate from the current position.
        // A fix for DVD src plugin: resindvd's handling of gst_element_seek()
        gst_element_seek (player,
                          playback_rate /*playback rate*/,
                          GST_FORMAT_TIME,
                          GstSeekFlags (GST_SEEK_FLAG_FLUSH /* flush pipeline, non-flush might be slow */
                                        | GST_SEEK_FLAG_SKIP) /*skip some frames*/,
                          GST_SEEK_TYPE_SET, /*absolute*/
                          position, /* current position*/
                          GST_SEEK_TYPE_NONE,
                          GST_CLOCK_TIME_NONE);

        // unmute after setting playback rate == 1
        if (playback_rate == 1)
            SetMute(false);

    } // SetPlaybackRate()

    /** get playback rate */
    double GstLib::PlaybackRate() const {

        if (!player) return 0;

        GstQuery *query = gst_query_new_segment(GST_FORMAT_TIME);
        gdouble rate;
        GstFormat format;

        gst_element_query(GST_ELEMENT(player), query);

        gst_query_parse_segment(query, &rate, &format, NULL, NULL);
        //g_print("rate: %f format %d\n", rate, format);

        return (double) rate;

    } // PlaybackRate()

    /** Pause player
    */

    void GstLib::Pause() {

        if (!player) return;

        GstStateChangeReturn ret;
        ret = gst_element_set_state (player, GST_STATE_PAUSED);

        // note: state maybe changed async, so !SUCCESS may not imply error
        if (ret != GST_STATE_CHANGE_SUCCESS) {
            g_print("%s returned %d\n", __PRETTY_FUNCTION__, ret);
        }

    } //Pause()


    /** Set player to play
    */

    void GstLib::Play() {

        if (!player) return;

        GstStateChangeReturn ret;
        ret = gst_element_set_state(player, GST_STATE_PLAYING);

        // note: state maybe changed async, so !SUCCESS may not imply error
        if (ret != GST_STATE_CHANGE_SUCCESS) {
            g_print("%s returned %d\n", __PRETTY_FUNCTION__, ret);
        }

    } // Play()


    /** stop playing
    * difference between Pause and Stop: player can resume playing from Pause
    */

    void GstLib::Stop() {

        if (!player) return;

        GstStateChangeReturn ret;
        ret = gst_element_set_state(player, GST_STATE_NULL);

        // note: state maybe changed async, so !SUCCESS may not imply error
        if (ret != GST_STATE_CHANGE_SUCCESS) {
            g_print("%s returned %d\n", __PRETTY_FUNCTION__, ret);
#if 0 // not necessary!
            GstState state;
            GstState pending;
            GstClockTime timeout = GST_TIME_AS_MSECONDS(100);
            while (ret != GST_STATE_CHANGE_ASYNC) {
                g_print("waiting for state change\n");
                ret = gst_element_get_state(player, &state, &pending, timeout);
            } // while
#endif
        }

    } // Stop()



    /** is player playing ?
     */

    bool GstLib::IsPlaying() const {

        if (!player) return false;

        GstState state, pending;
        gst_element_get_state(player, &state, &pending, GST_CLOCK_TIME_NONE /*timeout*/);

        return state == GST_STATE_PLAYING;

    } // IsPlaying()

    bool GstLib::IsPaused() const {

        if (!player) return false;

        GstState state, pending;
        gst_element_get_state(player, &state, &pending, GST_CLOCK_TIME_NONE /*timeout*/);

        return state == GST_STATE_PAUSED;

    } // IsPlaying()


    /**  End of stream
     *   use this to check if playback ended due to EOS
     *   call UriWanted() to see if current playback is over
     */

    bool GstLib::EoS() const {

        return newUriWanted;

    } // EoS



    /** New Uri wanted ?
     *
     * is set when EoS was reached or if an Error occured
     * returns true if player has finished playing/will not play (anymore)
     * the current Uri
     */

    bool GstLib::UriWanted() const {

        return newUriWanted;

    } // UriWanted()



    /** Mute and Volume */

    // returns true if gstreamer pipeline is muted else false
    bool GstLib::IsMute() const {

        if (!player) return false;

        gboolean isMute;

        // read (GObject) pipeline property
        g_object_get(player, "mute", &isMute, NULL);

        return isMute == TRUE;

    } // IsMute()


    /** Mute or unmute pipeline */
    void GstLib::SetMute(bool On) {

        if (!player) return;

        //set pipeline property
        g_object_set(player, "mute",
                     On ? TRUE : FALSE,
                     NULL);

    } // SetMute(bool)


    /** if pipeline muted, unmute it.
      * else if pipeline not muted, mute it
      */
    void GstLib::ToggleMute() {

        if (!player) return;

        bool On = IsMute();

        //Toggle it
        SetMute(!On);

    } // ToggleMute()



    /** Get pipeline's volume
      * returns between 0-100 though volume of pipeline is between [0,1.0]
      */
    int GstLib::Volume() const {

        if (!player) return 0;

        gdouble vol;

        // get "volume" property of pipeline
        g_object_get(player, "volume", &vol, NULL);

        // 1.0 is 100%
        return vol*100;

    } // Volume()


    /** Set pipeline's volume
      * percent shoule be between 0-100*/
    void GstLib::SetVolume(unsigned int percent) {

        if (!player) return;

        // clip 'percent' at 100%
        if (percent > 100) percent = 100;

        // between [0,1.0]
        gdouble vol = (percent/100.0);

        // set pipeline's property
        g_object_set(player, "volume", vol, NULL);

    } // SetVolume(int)
    
    
    /// video, audio and subtitle streams from pipeline's properties
    bool GstLib::HasVideo() const {
      
            return VideoStreamCount() > 0;
            
    } // HasVideo()
    
    bool GstLib::HasAudio() const {
      
            return AudioStreamCount() > 0;
            
    } // HasAudio()
    
    bool GstLib::HasSubtitle() const {
      
            return SubtitleStreamCount() > 0;
            
    } // HasSubtitle()
    
    // number of available video streams
    int GstLib::VideoStreamCount() const {
      
      if (!player) return 0;

      gint nVideos = 0; // number of video streams
      g_object_get( player, "n-video", &nVideos, NULL);
      
      return nVideos;
      
    } // VideoStreamCount() 

    // number of available audio streams
    int GstLib::AudioStreamCount() const {
      
        if (!player) return 0;

      gint nAudios = 0; // number of video streams
      g_object_get( player, "n-audio", &nAudios, NULL);
      
      return nAudios;
      
    } // AudioStreamCount()
    
    // number of available subtitle streams
    int GstLib::SubtitleStreamCount() const {
      
        if (!player) return 0;

      gint nTexts = 0; // number of video streams
      g_object_get( player, "n-text", &nTexts, NULL);
      
      return nTexts;
      
    } // SubtitleStreamCount()


    /// return current video/audio/subtitle stream's indices
    int GstLib::VideoStream() const {

        if (!player) return -1;

        gint videoStream = -1;
        g_object_get( player, "current-video", &videoStream, NULL);

        return videoStream;

    } // VideoStream()


    int GstLib::AudioStream() const {

        if (!player) return -1;

        gint audioStream = -1;
        g_object_get( player, "current-audio", &audioStream, NULL);

        return audioStream;

    } // AudioStream()


    int GstLib::SubtitleStream() const {

        if (!player) return -1;

        gint subtitleStream = -1;
        g_object_get( player, "current-text", &subtitleStream, NULL);

        return subtitleStream;

    } // SubtitleStream()


    /// Set current video/audio/subtitle stream
    // given its index
    void GstLib::SetVideoStream(int idx) {

        if (!player) return;

        g_object_set(player, "current-video", idx, NULL);

    } // SetVideoStream()


    void GstLib::SetAudioStream(int idx) {

        if (!player) return;

        g_object_set(player, "current-audio", idx, NULL);

    } // SetVideoStream()


    void GstLib::SetSubtitleStream(int idx) {

        if (!player) return;

        g_object_set(player, "current-text", idx, NULL);

    } // SetVideoStream()



    /** Get string from tagCache GstTagList
     *  given a tag string like "title"/GST_TAG_TITLE etc
     */
    std::string GstLib::StringFromTag(const gchar* tagStr) const {

        std::string result = "";

        if (!tagCache)
            return result;

        const GValue *val = gst_tag_list_get_value_index(tagCache, tagStr, 0);

        if (val) {
            result = (char*) g_value_get_string(val);
        } // if

        return result;

    } //StringFromTag()



    /** Get Title, Album, Artist etc from tagCache */
    std::string GstLib::Title() const {

        return StringFromTag(GST_TAG_TITLE);

    } // Title()

    std::string GstLib::Album() const {

        return StringFromTag(GST_TAG_ALBUM);

    } // Album()

    std::string GstLib::Artist() const {

        return StringFromTag(GST_TAG_ARTIST);

    } // Artist()



    /** Error and warning strings */
    std::string GstLib::ErrorString() const {

        return errorStr;

    } // ErrorString()


    std::string GstLib::WarningString() const {

        return warningStr;

    } // WarningString()


    bool GstLib::IsError() const {

        return errorStr.size() > 0;

    } // IsError()




    /** Get Video and Audio codec from tagCache */
    std::string GstLib::AudioCodec() const {

        /*
        // try pipeline's properties first
        gint stream_num = -1;
        g_object_get(player, "current-audio", &stream_num, NULL);
        g_print("current-audio stream %d", stream_num);
        */

        // get codec from tagCache
        return StringFromTag(GST_TAG_AUDIO_CODEC);

    } // AudioCodec()

    std::string GstLib::VideoCodec() const {

        /*
        // try pipeline's properties first
        gint stream_num = -1;
        g_object_get(player, "current-video", &stream_num, NULL);
        g_print("current-video stream %d", stream_num);
        */

        // get codec from tagCache
        return StringFromTag(GST_TAG_VIDEO_CODEC);

    } // VideoCodec()


    /// Audio/subtitle langauge list
    //
    std::vector<std::string> GstLib::AudioLangList() const {

        std::vector<std::string> audioList;
        std::string lang;

        // check any audio available
        int n = AudioStreamCount();

        // iterate through stream numbers
        for (int i = 0; i<n ; ++i) {

            lang = AudioLang(i);
            audioList.push_back(lang);

        } // for

        return audioList;

    } // AudioLangList()


    std::vector<std::string> GstLib::SubtitleLangList() const {

        std::vector<std::string> subtitleList;
        std::string lang;

        // total number of subtitle available
        int n = SubtitleStreamCount();

        // iterate through stream numbers
        for (int i = 0; i<n ; ++i) {

            lang = SubtitleLang(i);
            subtitleList.push_back(lang);

        } // for

        return subtitleList;

    } // SubtitleLangList()



    /// returns langauge of 'idx'th audio stream
    std::string GstLib::AudioLang(int idx) const {

        if (!player) return "";

        std::string lang;
        GstTagList *tags = NULL;

        g_signal_emit_by_name (G_OBJECT (player), "get-audio-tags",
                               idx, &tags);

        if (tags) {
            gchar *lc = NULL, *cd = NULL;

            gst_tag_list_get_string (tags, GST_TAG_LANGUAGE_CODE, &lc);

            g_print("audio stream #%d lang:'%s'", idx, lc);
            if (lc) {
                lang = (const char*)lc;
                g_free (lc);
            }

            /* codec, use it if lang-code is NULL ? */
            gst_tag_list_get_string (tags, GST_TAG_CODEC, &cd);
            g_print(" codec:'%s'\n", cd);

            if (cd)
                g_free (cd);

            gst_tag_list_free(tags);

        } //if

        return lang;

    } // AudioStreamLang ()



    // returns langauge of 'idx'th subtitle stream
    std::string GstLib::SubtitleLang(int idx) const {

        if (!player) return "";

        std::string lang;
        GstTagList *tags = NULL;

        g_signal_emit_by_name (G_OBJECT (player), "get-text-tags",
                               idx, &tags);

        if (tags) {
            gchar *lc = NULL, *cd = NULL;

            gst_tag_list_get_string (tags, GST_TAG_LANGUAGE_CODE, &lc);

            g_print("subtitle stream #%d lang:'%s'", idx, lc);
            if (lc) {
                lang = (const char*)lc;
                g_free (lc);
            }

            /* codec, use it if lang-code is NULL ? */
            gst_tag_list_get_string (tags, GST_TAG_CODEC, &cd);
            g_print(" codec:'%s'\n", cd);

            if (cd)
                g_free (cd);

            gst_tag_list_free(tags);

        } //if

        return lang;

    } // SubtitleStreamLang ()



    GstElement* GstLib::SourceElement() const {

        if (!player) return NULL;

        GstElement *elm = NULL;

        // get source element of pipeline
        g_object_get(player, "source", &elm, NULL);

        return elm;

    } // SourceElement()



    // Get the name of the factory that made the source element
    // may be used to detect "streaming source"
    std::string GstLib::SourceFactoryName() const {

        // get source element of pipeline
        GstElement *elm = SourceElement();
        std::string name ;

        if (elm) {
            name = ElementFactoryName(elm);
            gst_object_unref(elm);
        } // if

        return name;

    } // SourceFactoryName()


    // eg. Klass = "Source/Network"
    std::string GstLib::SourceFactoryKlass() const {

        // Get source Element of pipeline
        GstElement *elm = SourceElement();

        std::string name ;

        if (elm) {
            name = ElementFactoryKlass(elm);
            gst_object_unref(elm);
        } // if

        return name;

    } // SourceFactoryKlass

    /// DVD menu navigation
    // Send command to gst core
    // http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-base-libs/html/gst-plugins-base-libs-gstnavigation.html#GstNavigationCommand
    bool GstLib::SendGstCommand(GstNavigationCommand cmd) {

        if (!player) return false;

        GstElement *navElement =
                gst_bin_get_by_interface (GST_BIN(player),
                                          GST_TYPE_NAVIGATION);

        // return value
        bool ret = false;

        if (navElement && GST_IS_NAVIGATION(navElement)) {

            GstNavigation *nav = GST_NAVIGATION (navElement);
            gst_navigation_send_command(nav, cmd);
            ret = true;

        } // if

        //XXX free/unref/do nothing? navElement
        if(navElement)
            g_object_unref(navElement);

        return ret;

    } // SendGstCommand()


    // IsInDVDMenu checks Title string for sub string "DVD Menu"
    bool GstLib::IsInDVDMenu() const {

        return Title().find("DVD Menu") != std::string::npos;

    } // IsInDVDMenu()

    /// commands while replaying DVD
    bool GstLib::DVDMenu() {

        return SendGstCommand(GST_NAVIGATION_COMMAND_DVD_MENU);

    } // DVDMenu()


    bool GstLib::DVDRootMenu() {

        return SendGstCommand(GST_NAVIGATION_COMMAND_DVD_ROOT_MENU);

    } // DVDRootMenu()


    bool GstLib::DVDAudioMenu() {

        return SendGstCommand(GST_NAVIGATION_COMMAND_DVD_AUDIO_MENU);

    } // DVDAudioMenu()


    bool GstLib::DVDAngleMenu() {

        return SendGstCommand(GST_NAVIGATION_COMMAND_DVD_ANGLE_MENU);

    } // DVDAngleMenu()


    bool GstLib::DVDChapterMenu() {

        return SendGstCommand(GST_NAVIGATION_COMMAND_DVD_CHAPTER_MENU);

    } // DVDChapterMenu()


    bool GstLib::DVDSubpictureMenu() {

        return SendGstCommand(GST_NAVIGATION_COMMAND_DVD_SUBPICTURE_MENU);

    } // DVDSubpictureMenu()


    bool GstLib::DVDTitleMenu() {

        return SendGstCommand(GST_NAVIGATION_COMMAND_DVD_TITLE_MENU);

    } // DVDTitleMenu()



    /// Change angles if possible
    bool GstLib::PrevAngle() {

        return SendGstCommand(GST_NAVIGATION_COMMAND_PREV_ANGLE);

    } // DVDPrevAngle()


    bool GstLib::NextAngle() {

        return SendGstCommand(GST_NAVIGATION_COMMAND_NEXT_ANGLE);

    } // DVDPrevAngle()



    /// Navigate Left/Right Up/Down
    bool GstLib::NavigateLeft() {

        return SendGstCommand(GST_NAVIGATION_COMMAND_LEFT);

    } // NavigateLeft()


    bool GstLib::NavigateRight() {

        return SendGstCommand(GST_NAVIGATION_COMMAND_RIGHT);

    } // NavigateRight()


    bool GstLib::NavigateUp() {

        return SendGstCommand(GST_NAVIGATION_COMMAND_UP);

    } // NavigateUp()


    bool GstLib::NavigateDown() {

        return SendGstCommand(GST_NAVIGATION_COMMAND_DOWN);

    } // NavigateDown()



    // activate selected "button"
    bool GstLib::OK_key() {
        return SendGstCommand(GST_NAVIGATION_COMMAND_ACTIVATE);
    } // OK_key()



#define UPDATE_TAG(on, tag) if (on) \
    new_available_tag |= (tag);\
    else new_available_tag &= ~(tag);


    // video tags changed
    void GstLib::SetVideoTagsChanged(bool on) {

        UPDATE_TAG(on, GSTLIB_VIDEO_TAG);

    } // SetVideoTagsChanged()


    // audio tags changed
    void GstLib::SetAudioTagsChanged(bool on) {

        UPDATE_TAG(on, GSTLIB_AUDIO_TAG);

    } // SetVideoTagsChanged()


    // subtitle tags changed
    void GstLib::SetSubtitleTagsChanged(bool on) {

        UPDATE_TAG(on, GSTLIB_SUBTITLE_TAG);

    } // SetVideoTagsChanged()


    // do we have any new tag?
    bool GstLib::VideoTagsChanged() const {

        return (new_available_tag&GSTLIB_VIDEO_TAG);

    } // VideoTagsChanged()


    bool GstLib::AudioTagsChanged() const {

        return (new_available_tag&GSTLIB_AUDIO_TAG);

    } // AudioTagsChanged()

    bool GstLib::SubtitleTagsChanged() const {

        return (new_available_tag&GSTLIB_SUBTITLE_TAG);

    } // SubtitleTagsChanged()


///-------------call backs -----------------------------------------------------
    // signal "audio-tags-changed"
    void audio_tags_changed_cb (GstElement* playbin,
                            gint stream_id,
                            gpointer user_data)

    {
        g_print("%s: playbin %p stream [%d] user_data %p\n",
                __PRETTY_FUNCTION__,
                playbin,
                stream_id,
                user_data
                );

        GstLib* gstObj = static_cast<GstLib*> (user_data);
        gstObj->SetAudioTagsChanged(true);
    }

    void video_tags_changed_cb (GstElement *playbin,
                               gint stream_id,
                               gpointer user_data)
    {
        g_print("%s: playbin %p stream [%d] user_data %p\n",
                __PRETTY_FUNCTION__,
                playbin,
                stream_id,
                user_data
                );
        GstLib* gstObj = static_cast<GstLib*> (user_data);
        gstObj->SetVideoTagsChanged(true);
    }

    void subtitle_tags_changed_cb (GstElement *playbin,
                                   gint stream_id,
                                   gpointer user_data)
    {
        g_print("%s: playbin %p stream [%d] user_data %p\n",
                __PRETTY_FUNCTION__,
                playbin,
                stream_id,
                user_data
                );
        GstLib* gstObj = static_cast<GstLib*> (user_data);
        gstObj->SetSubtitleTagsChanged(true);
    } // subtitle_tags_changed_cb



// ------------ helper functions ----------------------------------------------

    // returns the name of the factory that created the element
    std::string ElementFactoryName(GstElement* elm) {

        GstElementFactory *factory = gst_element_get_factory(elm);
        const gchar *name = gst_element_factory_get_longname(factory);

        return name ? std::string((const char*) name):std::string();

    } //Element Factory Name


    // returns the name of the klass the factory that created the element
    // belongs
    std::string ElementFactoryKlass(GstElement* elm) {

        GstElementFactory *factory = gst_element_get_factory(elm);
        const gchar *name = gst_element_factory_get_klass(factory);

        return name ? std::string((const char*) name):std::string();

    } //ElementFactoryKlass


} // namespace Reel
