/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia                                 *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

// XineLib.h
 
#ifndef XINE_MEDIAPLAYER_XINE_LIB_H_INCLUDED
#define XINE_MEDIAPLAYER_XINE_LIB_H_INCLUDED

#include <string>
#include <vdr/keys.h>

#include "Reel.h"
#include "XineEvent.h"
#include "VdrXineMpIf.h"



// xine struct declarations.
typedef struct xine_s             xine_t;
typedef struct xine_stream_s      xine_stream_t;
typedef struct xine_audio_port_s  xine_audio_port_t;
typedef struct xine_event_queue_s xine_event_queue_t;
typedef struct xine_video_port_s  xine_video_port_t;
//static _stream_infos_t

typedef struct  myxine_mrl_s {
    char      *origin; /* file plugin: path */
    char      *mrl;    /* <type>://<location> */
    char      *link;
    uint32_t   type;   /* see below */
    off_t      size;   /* size of this source, may be 0 */
} myxine_mrl_t;

namespace Reel {
  namespace XineMediaplayer {

    #define CATCH_THROW_STREAM(r,x...) {if(!stream_) return r;try{ return x; } catch(...){} return r;}

    extern VdrXineMpIf *vxi;
    extern int useOld;
    class Player;

    class XineLib { /* final, singleton */
      public:
        static XineLib &Instance() { return instance_;}; // Return singleton instance
        void Exit() NO_THROW; // Free all resources.
        XineEventBase *GetEvent() const THROWS; // Return a xine event.
        void Init() THROWS; // Initialize xine-lib and the xine stream. Throws XineError, in this case the XineLib will be in the disposed state.
        void SetXineConfig() THROWS; 
        void Open(char const *mrl) THROWS; // Open a stream. Throws XineError, in this case the stream will be in the closed state.
        void Close() NO_THROW; // Close the stream.
        void Play(int startPos, int startTime) THROWS; // Play the stream from a given position. Throws XineError, in this case the stream will be in the stopped state.
        void ProcessKeyMenuMode(eKeys key) THROWS; // Process a key in menu domain
        void ProcessKey(eKeys key,bool isMusic) THROWS; // Process a key.
        void Stop() NO_THROW; // Stop stream playback.
        std::string XineGetErrorStr() const NO_THROW; // Return last error of the xine stream as a string.
        bool GetReplayMode(bool &Play, bool &Forward, int &Speed) NO_THROW;
//        const char *GetTitleString();
        const char *GetAspectString() NO_THROW;
        bool GetAudioLangCode(char *lang, int track = -1) NO_THROW;
        bool GetSpuLangCode(char *lang, int track = -1) NO_THROW; // default value -1 means current track
        void SetAudioTrack(int track) NO_THROW;
        void SetSpuTrack(int track) NO_THROW;
        bool GetIndex(int &current, int &total) NO_THROW; // writes current and total time of stream. total will be 0 if xine_get_pos_length fails 
        xine_stream_t *GetStream() { return stream_;};
        const char *GetSetupLang() const NO_THROW;
        void PrintXineConfig() NO_THROW;
        void PrintXinePlugins() NO_THROW;
//        void ResetStream();

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // xine stream_info

        uint32_t GetBitrate()       {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_BITRATE));};
        uint32_t Seekable()         {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_SEEKABLE));};
        uint32_t VideoWidth()       {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_WIDTH));};
        uint32_t VideoHeight()      {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_HEIGHT));};
        uint32_t VideoRatio()       {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_RATIO));};
        uint32_t VideoChannels()    {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_CHANNELS));};
        uint32_t VideoStreams()     {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_STREAMS));};
        uint32_t VideoBitrate()     {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_BITRATE));};
        uint32_t VideoFourcc()      {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_FOURCC));};
        uint32_t VideoHandled()     {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_HANDLED));};
        uint32_t FrameDuration()    {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_FRAME_DURATION));};
        uint32_t AudioChannels()    {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_AUDIO_CHANNELS));};
        uint32_t AudioBits()        {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_AUDIO_BITS));};
        uint32_t AudioSamplerate()  {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_AUDIO_SAMPLERATE));};
        uint32_t AudioBitrate()     {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_AUDIO_BITRATE));};
        uint32_t AudioFourcc()      {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_AUDIO_FOURCC));};
        uint32_t AudioHandled()     {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_AUDIO_HANDLED));};
        uint32_t HasChapters()      {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_HAS_CHAPTERS));};
        uint32_t HasVideo()         {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_HAS_VIDEO));};
        uint32_t HasAudio()         {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_HAS_AUDIO));};
        uint32_t IgnoreVideo()      {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_IGNORE_VIDEO));};
        uint32_t IgnoreAudio()      {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_IGNORE_AUDIO));};
        uint32_t IgnoreSpu()        {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_IGNORE_SPU));};
        uint32_t VideoHasStill()    {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_HAS_STILL));};
        uint32_t MaxAudioChannel()  {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_MAX_AUDIO_CHANNEL));};
        uint32_t MaxSpuChannel()    {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_MAX_SPU_CHANNEL));};
        uint32_t AudioMode()        {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_AUDIO_MODE));};
        uint32_t SkippedFrames()    {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_SKIPPED_FRAMES));};
        uint32_t DiscardedFrames()  {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_DISCARDED_FRAMES));};
        uint32_t VideoAfd()         {CATCH_THROW_STREAM(XINE_VIDEO_AFD_NOT_PRESENT,xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_AFD));};
        uint32_t DvdTitleNumber()   {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_DVD_TITLE_NUMBER));};
        uint32_t DvdTitleCount()    {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_DVD_TITLE_COUNT));};
        uint32_t DvdChapterNumber() {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_DVD_CHAPTER_NUMBER));};
        uint32_t DvdChapterCount()  {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_DVD_CHAPTER_COUNT));};
        uint32_t DvdAngleNumber()   {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_DVD_ANGLE_NUMBER));};
        uint32_t DvdAngleCount()    {CATCH_THROW_STREAM(0,xine_get_stream_info(stream_, XINE_STREAM_INFO_DVD_ANGLE_COUNT));};

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // xine meta infos 

        const char *GetTitle() const      {CATCH_THROW_STREAM(NULL,xine_get_meta_info(stream_, XINE_META_INFO_TITLE));};
        const char *Comment() const       {CATCH_THROW_STREAM(NULL,xine_get_meta_info(stream_, XINE_META_INFO_COMMENT));};
        const char *Artist() const        {CATCH_THROW_STREAM(NULL,xine_get_meta_info(stream_, XINE_META_INFO_ARTIST));};
        const char *Genre() const         {CATCH_THROW_STREAM(NULL,xine_get_meta_info(stream_, XINE_META_INFO_GENRE));};
        const char *Album() const         {CATCH_THROW_STREAM(NULL,xine_get_meta_info(stream_, XINE_META_INFO_ALBUM));};
        const char *Year() const          {CATCH_THROW_STREAM(NULL,xine_get_meta_info(stream_, XINE_META_INFO_YEAR));};
        const char *VideoCodec() const    {CATCH_THROW_STREAM(NULL,xine_get_meta_info(stream_, XINE_META_INFO_VIDEOCODEC));};
        const char *AudioCodec() const    {CATCH_THROW_STREAM(NULL,xine_get_meta_info(stream_, XINE_META_INFO_AUDIOCODEC));};
        const char *SystemLayer() const   {CATCH_THROW_STREAM(NULL,xine_get_meta_info(stream_, XINE_META_INFO_SYSTEMLAYER));};
        const char *InputPlugin() const   {CATCH_THROW_STREAM("",xine_get_meta_info(stream_, XINE_META_INFO_INPUT_PLUGIN));};
        const char *CDIndexDiscId() const {CATCH_THROW_STREAM(NULL,xine_get_meta_info(stream_, XINE_META_INFO_CDINDEX_DISCID));};
        const char *TrackNumber() const   {CATCH_THROW_STREAM(NULL,xine_get_meta_info(stream_, XINE_META_INFO_TRACK_NUMBER));};

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // xine get stream parameter 

        int GetParamSpeed()               {CATCH_THROW_STREAM(0,xine_get_param(stream_,XINE_PARAM_SPEED));};
        int GetParamAVOffset()            {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_AV_OFFSET));};
        int GetParamAudioChannelLogical() {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_AUDIO_CHANNEL_LOGICAL));};
        int GetParamSPUChannel()          {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_SPU_CHANNEL));};
        int GetParamParamVideoChannel()   {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_VIDEO_CHANNEL));};
        int GetParamAudioVolume()         {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_AUDIO_VOLUME));};
        int GetParamAudioMute()           {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_AUDIO_MUTE));};
        int GetParamAudioComprLevel()     {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_AUDIO_COMPR_LEVEL));};
        int GetParamAudioAmpLevel()       {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_AUDIO_AMP_LEVEL));};
        int GetParamAudioReportLevel()    {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_AUDIO_REPORT_LEVEL));};
        int GetParamVerbosity()           {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_VERBOSITY));};
        int GetParamSpuOffset()           {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_SPU_OFFSET));};
        int GetParamIgnoreVideo()         {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_IGNORE_VIDEO));};
        int GetParamIgnoreAudio()         {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_IGNORE_AUDIO));};
        int GetParamIgnoreSpu()           {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_IGNORE_SPU));};
        int GetParamBroadcasterPort()     {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_BROADCASTER_PORT));};
        int GetParamMetronomPrebuffer()   {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_METRONOM_PREBUFFER));};
        int GetParamAudioAmpMute()        {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_AUDIO_AMP_MUTE));};
        int GetParamFineSpeed()           {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_FINE_SPEED));};
        int GetParamEarlyFinishedEvent()  {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_EARLY_FINISHED_EVENT));};
        int GetParamGaplessSwitch()       {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_GAPLESS_SWITCH));};
        int GetParamDeinterlace()         {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_VO_DEINTERLACE));};
        int GetParamVoAspectRatio()       {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_VO_ASPECT_RATIO));};
        int GetParamVoHue()               {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_VO_HUE));};
        int GetParamVoSaturation()        {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_VO_SATURATION));};
        int GetParamVoContrast()          {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_VO_CONTRAST));};
        int GetParamVoBrightness()        {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_VO_BRIGHTNESS));};
        int GetParamVoZoomX()             {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_VO_ZOOM_X));};
        int GetParamVoZommY()             {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_VO_ZOOM_Y));};
        int GetParamPanScan()             {CATCH_THROW_STREAM(0,xine_get_param(stream_, XINE_PARAM_VO_PAN_SCAN));};

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // xine set stream parameter 

        void SetParamSpeed(int val)               {CATCH_THROW_STREAM(,xine_set_param(stream_,XINE_PARAM_SPEED, val));};
        void SetParamAVOffset(int val)            {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_AV_OFFSET, val));};
        void SetParamAudioChannelLogical(int val) {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_AUDIO_CHANNEL_LOGICAL, val));};
        void SetParamSPUChannel(int val)          {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_SPU_CHANNEL, val));};
        void SetParamParamVideoChannel(int val)   {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_VIDEO_CHANNEL, val));};
        void SetParamAudioVolume(int val)         {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_AUDIO_VOLUME, val));};
        void SetParamAudioMute(int val)           {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_AUDIO_MUTE, val));};
        void SetParamAudioComprLevel(int val)     {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_AUDIO_COMPR_LEVEL, val));};
        void SetParamAudioAmpLevel(int val)       {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_AUDIO_AMP_LEVEL, val));};
        void SetParamAudioReportLevel(int val)    {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_AUDIO_REPORT_LEVEL, val));};
        void SetParamVerbosity(int val)           {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_VERBOSITY, val));};
        void SetParamSpuOffset(int val)           {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_SPU_OFFSET, val));};
        void SetParamIgnoreVideo(int val)         {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_IGNORE_VIDEO, val));};
        void SetParamIgnoreAudio(int val)         {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_IGNORE_AUDIO, val));};
        void SetParamIgnoreSpu(int val)           {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_IGNORE_SPU, val));};
        void SetParamBroadcasterPort(int val)     {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_BROADCASTER_PORT, val));};
        void SetParamMetronomPrebuffer(int val)   {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_METRONOM_PREBUFFER, val));};
        void SetParamAudioAmpMute(int val)        {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_AUDIO_AMP_MUTE, val));};
        void SetParamFineSpeed(int val)           {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_FINE_SPEED, val));};
        void SetParamEarlyFinishedEvent(int val)  {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_EARLY_FINISHED_EVENT, val));};
        void SetParamGaplessSwitch(int val)       {CATCH_THROW_STREAM(,xine_set_param(stream_, XINE_PARAM_GAPLESS_SWITCH, val));};

        myxine_mrl_t **GetMrls(const char *start_mrl, int *num_mrls) {
          try {
            const char *id = InputPlugin();
            if(!xine_ || !id) return NULL;
            return reinterpret_cast<myxine_mrl_t **>(xine_get_browse_mrls (xine_, id, start_mrl, num_mrls));
          } catch(...) {}
          return NULL;
        } // XineLib::GetMrls

    private:
        static XineLib           instance_;
        XineLib() NO_THROW;
        ~XineLib() NO_THROW;

        XineLib(XineLib const &); // No copying.
        XineLib &operator=(XineLib const &); // No assigning.
        void ProcessInputKeyMenuMode(eKeys key) THROWS; // PRE: Stream exists.
        void ProcessInputKey(eKeys key) THROWS; // PRE: Stream exists.
        void ProcessPlaymodeKey(eKeys key,bool isMusic) THROWS; // PRE: Stream exists.
        void ProcessSeekKey(eKeys key, bool isMusic) THROWS; // PRE: Stream exists.
    public:
        void ProcessSeek() NO_THROW;
    private:
        xine_t             *xine_;
        xine_audio_port_t  *aoPort_;
        xine_video_port_t  *voPort_;
        xine_stream_t      *stream_;
        xine_event_queue_t *eventQueue_;
        bool                streamOpen_, streamPlaying_;
        int                 seekPos;
        cTimeMs             seekTime;
    }; // XineLib

    class XineError { /* final */
      public:
        XineError(char const *msg) NO_THROW;
        // default copying, assignment and destruction do ok.
        char const *Msg() const NO_THROW;
      private:
        std::string msg_;
    }; // XineError

    void LogXineError(XineError const &ex) NO_THROW;
/*
    inline XineLib &XineLib::Instance() NO_THROW {
        return instance_;
    } // XineLib::Instance
*/
    inline XineLib::~XineLib() NO_THROW {
        Exit();
    } // XineLib::~XineLib

    inline bool XineLib::GetReplayMode(bool &Play, bool &Forward, int &Speed) NO_THROW {
      Play = true;
      Forward = false;
      Speed = -1;
      return true;
    } // XineLib::GetReplayMode

    inline bool XineLib::GetIndex(int &current, int &total) NO_THROW {
      try {
        if (!stream_)
          return false;
        int pal_ms2fps = 40; 
        //int ntfs_ms2fps = 29.97; 
        int pos;
        int pos_time;
        int length_time;
        //printf("XineLib::GetIndex() calling xine_get_pos_length()\n");
        int ret = xine_get_pos_length (stream_, &pos, &pos_time ,&length_time);
        //printf("XineLib::GetIndex() calling xine_get_pos_length() done\n");
        pos_time += seekPos;
        if(pos_time < 0) pos_time = 0;
        if(pos_time > length_time) pos_time = length_time;
        current = (int)pos_time/pal_ms2fps;
        total = (int)length_time/pal_ms2fps;
        return ret;
      } catch(...){}
      return false;
    } // XineLib::GetIndex

    inline const char *XineLib::GetAspectString() NO_THROW {
      try {
        // TODO XINE_STREAM_INFO_VIDEO_AFD
        int ret = VideoAfd();
        switch (ret) {
          case XINE_VIDEO_AFD_NOT_PRESENT:
            return "na";
          case XINE_VIDEO_AFD_RESERVED_0:
          case XINE_VIDEO_AFD_RESERVED_1:
            //printf(" Debug XineLib -----------------------------  XINE_VIDEO_AFD_RESERVED %d  ?? --------------- \n", ret);
            return "reserved";
          case XINE_VIDEO_AFD_BOX_14_9_TOP:
            //printf(" Debug XineLib -----------------------------  XINE_VIDEO_AFD_BOX_14_9_TOP ?? --------------- \n");
          case XINE_VIDEO_AFD_BOX_16_9_TOP:
          case XINE_VIDEO_AFD_BOX_GT_16_9_CENTRE:
            return "16:9";
          case XINE_VIDEO_AFD_RESERVED_5:
          case XINE_VIDEO_AFD_RESERVED_6:
          case XINE_VIDEO_AFD_RESERVED_7:
            //printf(" Debug XineLib -----------------------------  XINE_VIDEO_AFD_RESERVED %d  ?? --------------- \n", ret);
            return "reserved";
          case XINE_VIDEO_AFD_SAME_AS_FRAME:
            //printf(" Debug XineLib -----------------------------  XINE_VIDEO_AFD_SAME_AS_FRAME?? --------------- \n");
            return "saf";
          case XINE_VIDEO_AFD_4_3_CENTRE:
            return "4:3";
          case XINE_VIDEO_AFD_16_9_CENTRE:
          case XINE_VIDEO_AFD_14_9_CENTRE:
            return "16:9";
          case XINE_VIDEO_AFD_RESERVED_12:
            //printf(" Debug XineLib -----------------------------  XINE_VIDEO_AFD_RESERVED %d  ?? --------------- \n", ret);
            return "reserved";
          case XINE_VIDEO_AFD_4_3_PROTECT_14_9:
          case XINE_VIDEO_AFD_16_9_PROTECT_14_9:
          case XINE_VIDEO_AFD_16_9_PROTECT_4_3:
            //printf(" Debug XineLib -----------------------------  XINE_VIDEO_AFD_?? _PROTECT_ ?? %d --------------- \n", ret);
            return "16:9";
        } // switch
      } catch(...){}
      //printf(" Debug XineLib ------------------ No valid aspect ----------------- \n");
      return "??";
    } // XineLib::GetAspectString

    inline bool XineLib::GetAudioLangCode(char *langbuf, int track) NO_THROW {
      try {
        strcpy (langbuf, "");
        if(!stream_) return false;
        if (xine_get_audio_lang(stream_, track, langbuf)!= 0)
          return true;
      } catch(...){}
      return false;
    } // XineLib::GetAudioLangCode

    inline void XineLib::SetAudioTrack(int track) NO_THROW {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_AUDIO_CHANNEL_LOGICAL, track);
      } catch(...){}
    } // XineLib::SetAudioTrack

    inline bool XineLib::GetSpuLangCode(char *langbuf, int track) NO_THROW {
      try {
        strcpy (langbuf, "");
        if(!stream_) return false;
        if (xine_get_spu_lang(stream_, track, langbuf) != 0)
          return true;
      } catch(...){}
      return false;
    } // XineLib::GetSpuLangCode

    inline void XineLib::SetSpuTrack(int track) NO_THROW {
      try {
         if(!stream_) return;
         xine_set_param(stream_, XINE_PARAM_SPU_CHANNEL, track);
      } catch(...){}
    } // XineLib::SetSpuTrack

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // xine stream info
/*
    inline uint32_t XineLib::GetBitrate() NO_THROW {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_,XINE_STREAM_INFO_BITRATE);
      } catch(...){}
      return 0;
    } // XineLib::GetBitrate
*/
/*
    inline uint32_t XineLib::Seekable() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_SEEKABLE);
      } catch(...){}
      return 0;
    } // XineLib::Seekable
*/
/*
    inline uint32_t XineLib::VideoWidth() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_WIDTH);
      } catch(...){}
      return 0;
    } // XineLib::VideoWidth

    inline uint32_t XineLib::VideoHeight() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_HEIGHT);
      } catch(...){}
      return 0;
    } // XineLib::VideoHeight

    inline uint32_t XineLib::VideoRatio() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_RATIO);
      } catch(...){}
      return 0;
    } // XineLib::VideoRatio
*/
/*
    inline uint32_t XineLib::VideoChannels() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_CHANNELS);
      } catch(...){}
      return 0;
    } // XineLib::VideoChannels

    inline uint32_t XineLib::VideoStreams() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_STREAMS);
      } catch(...){}
      return 0;
    } // XineLib::VideoStreams

    inline uint32_t XineLib::VideoBitrate() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_BITRATE);
      } catch(...){}
      return 0;
    } // XineLib::VideoBitrate

    inline uint32_t XineLib::VideoFourcc() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_FOURCC);
      } catch(...){}
      return 0;
    } // XineLib::VideoFourcc

    inline uint32_t XineLib::VideoHandled() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_HANDLED);
      } catch(...){}
      return 0;
    } // XineLib::VideoHandled

    inline uint32_t XineLib::FrameDuration() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_FRAME_DURATION);
      } catch(...){}
      return 0;
    } // XineLib::FrameDuration

    inline uint32_t XineLib::AudioChannels() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_AUDIO_CHANNELS);
      } catch(...){}
      return 0;
    } // XineLib::AudioChannels

    inline uint32_t XineLib::AudioBits() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_AUDIO_BITS);
      } catch(...){}
      return 0;
    } // XineLib::AudioBits

    inline uint32_t XineLib::AudioSamplerate() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_AUDIO_SAMPLERATE);
      } catch(...){}
      return 0;
    } // XineLib::AudioSamplerate

    inline uint32_t XineLib::AudioBitrate() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_AUDIO_BITRATE);
      } catch(...){}
      return 0;
    } // XineLib::AudioBitrate

    inline uint32_t XineLib::AudioFourcc() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_AUDIO_FOURCC);
      } catch(...){}
      return 0;
    } // XineLib::AudioFourcc

    inline uint32_t XineLib::AudioHandled() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_AUDIO_HANDLED);
      } catch(...){}
      return 0;
    } // XineLib::AudioHandled

    inline uint32_t XineLib::HasChapters() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_HAS_CHAPTERS);
      } catch(...){}
      return 0;
    } // XineLib::HasChapters

    inline uint32_t XineLib::HasVideo() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_HAS_VIDEO);
      } catch(...){}
      return 0;
    } // XineLib::HasVideo

    inline uint32_t XineLib::HasAudio() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_HAS_AUDIO);
      } catch(...){}
      return 0;
    } // XineLib::HasAudio

    inline uint32_t XineLib::IgnoreVideo() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_IGNORE_VIDEO);
      } catch(...){}
      return 0;
    } // XineLib::IgnoreVideo

    inline uint32_t XineLib::IgnoreAudio() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_IGNORE_AUDIO);
      } catch(...){}
      return 0;
    } // XineLib::IgnoreAudio

    inline uint32_t XineLib::IgnoreSpu() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_IGNORE_SPU);
      } catch(...){}
      return 0;
    } // XineLib::IgnoreSpu

    inline uint32_t XineLib::VideoHasStill() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_HAS_STILL);
      } catch(...){}
      return 0;
    } // XineLib::VideoHasStill

    inline uint32_t XineLib::MaxAudioChannel() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_MAX_AUDIO_CHANNEL);
      } catch(...){}
      return 0;
    } // XineLib::MaxAudioChannel

    inline uint32_t XineLib::MaxSpuChannel() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_MAX_SPU_CHANNEL);
      } catch(...){}
      return 0;
    } // XineLib::MaxSpuChannel

    inline uint32_t XineLib::AudioMode() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_AUDIO_MODE);
      } catch(...){}
      return 0;
    } // XineLib::AudioMode

    inline uint32_t XineLib::SkippedFrames() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_SKIPPED_FRAMES);
      } catch(...){}
      return 0;
    } // XineLib::SkippedFrames

    inline uint32_t XineLib::DiscardedFrames() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_DISCARDED_FRAMES);
      } catch(...){}
      return 0;
    } // XineLib::DiscardedFrames

    inline uint32_t XineLib::VideoAfd() {
      try {
        if(!stream_) return XINE_VIDEO_AFD_NOT_PRESENT;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_VIDEO_AFD);
      } catch(...){}
      return 0;
    } // XineLib::VideoAfd

    inline uint32_t XineLib::DvdTitleNumber() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_DVD_TITLE_NUMBER);
      } catch(...){}
      return 0;
    } // XineLib::DvdTitleNumber

    inline uint32_t XineLib::DvdTitleCount() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_DVD_TITLE_COUNT);
      } catch(...){}
      return 0;
    } // XineLib::DvdTitleCount

    inline uint32_t XineLib::DvdChapterNumber() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_DVD_CHAPTER_NUMBER);
      } catch(...){}
      return 0;
    } // XineLib::DvdChapterNumber

    inline uint32_t XineLib::DvdChapterCount() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_DVD_CHAPTER_COUNT);
      } catch(...){}
      return 0;
    } // XineLib::DvdChapterCount

    inline uint32_t XineLib::DvdAngleNumber() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_DVD_ANGLE_NUMBER);
      } catch(...){}
      return 0;
    } // XineLib::DvdAngleNumber

    inline uint32_t XineLib::DvdAngleCount() {
      try {
        if(!stream_) return 0;
        return  xine_get_stream_info(stream_, XINE_STREAM_INFO_DVD_ANGLE_COUNT);
      } catch(...){}
      return 0;
    } // XineLib::DvdAngleCount
*/
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // xine meta infos 
/*
    inline const char *XineLib::GetTitle() const {
      try {
        if(!stream_) return NULL;
        return  xine_get_meta_info(stream_, XINE_META_INFO_TITLE);
      } catch(...){}
      return NULL;
    } // XineLib::GetTitle

    inline const char *XineLib::Comment() const {
      try {
        if(!stream_) return NULL;
        return  xine_get_meta_info(stream_, XINE_META_INFO_COMMENT);
      } catch(...){}
      return NULL;
    } // XineLib::Comment

    inline const char *XineLib::Artist() const {
      try {
        if(!stream_) return NULL;
        return  xine_get_meta_info(stream_, XINE_META_INFO_ARTIST);
      } catch(...){}
      return NULL;
    } // XineLib::Artist

    inline const char *XineLib::Genre() const {
      try {
        if(!stream_) return NULL;
        return  xine_get_meta_info(stream_, XINE_META_INFO_GENRE);
      } catch(...){}
      return NULL;
    } // XineLib::Genre

    inline const char *XineLib::Album() const {
      try {
        if(!stream_) return NULL;
        return  xine_get_meta_info(stream_, XINE_META_INFO_ALBUM);
      } catch(...){}
      return NULL;
    } // XineLib::Album

    inline const char *XineLib::Year() const {
      try {
        if(!stream_) return NULL;
        return  xine_get_meta_info(stream_, XINE_META_INFO_YEAR);
      } catch(...){}
      return NULL;
    } // XineLib::Year

    inline const char *XineLib::VideoCodec() const {
      try {
        if(!stream_) return NULL;
        return  xine_get_meta_info(stream_, XINE_META_INFO_VIDEOCODEC);
      } catch(...){}
      return NULL;
    } // XineLib::VideoCodec

    inline const char *XineLib::AudioCodec() const {
      try {
        if(!stream_) return NULL;
        return  xine_get_meta_info(stream_, XINE_META_INFO_AUDIOCODEC);
      } catch(...){}
      return NULL;
    } // XineLib::AudioCodec

    inline const char *XineLib::SystemLayer() const {
      try {
        if(!stream_) return NULL;
        return  xine_get_meta_info(stream_, XINE_META_INFO_SYSTEMLAYER);
      } catch(...){}
      return NULL;
    } // XineLib::SystemLayer

    inline const char *XineLib::InputPlugin() const {
      try {
        if(!stream_) return "";
        return  xine_get_meta_info(stream_, XINE_META_INFO_INPUT_PLUGIN);
      } catch(...){}
      return NULL;
    } // XineLib::InputPlugin

    inline const char *XineLib::CDIndexDiscId() const {
      try {
        if(!stream_) return NULL;
        return  xine_get_meta_info(stream_, XINE_META_INFO_CDINDEX_DISCID);
      } catch(...){}
      return NULL;
    } // XineLib::CDIndexDiscId

    inline const char *XineLib::TrackNumber() const {
      try {
        if(!stream_) return NULL;
        return  xine_get_meta_info(stream_, XINE_META_INFO_TRACK_NUMBER);
      } catch(...){}
      return NULL;
    } // XineLib::TrackNumber
*/
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // xine stream parameter
/*
    inline int XineLib::GetParamSpeed() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_,XINE_PARAM_SPEED);
      } catch(...){}
      return 0;
    } // XineLib::GetParamSpeed

    inline int XineLib::GetParamAVOffset() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_AV_OFFSET);
      } catch(...){}
      return 0;
    } // XineLib::GetParamAVOffset

    inline int XineLib::GetParamAudioChannelLogical() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_AUDIO_CHANNEL_LOGICAL);
      } catch(...){}
      return 0;
    } // XineLib::GetParamAudioChannelLogical

    inline int XineLib::GetParamSPUChannel() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_SPU_CHANNEL);
      } catch(...){}
      return 0;
    } // XineLib::GetParamSPUChannel

    inline int XineLib::GetParamParamVideoChannel() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_VIDEO_CHANNEL);
      } catch(...){}
      return 0;
    } // XineLib::GetParamParamVideoChannel

    inline int XineLib::GetParamAudioVolume() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_AUDIO_VOLUME);
      } catch(...){}
      return 0;
    } // XineLib::GetParamAudioVolume

    inline int XineLib::GetParamAudioMute() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_AUDIO_MUTE);
      } catch(...){}
      return 0;
    } // XineLib::GetParamAudioMute

    inline int XineLib::GetParamAudioComprLevel() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_AUDIO_COMPR_LEVEL);
      } catch(...){}
      return 0;
    } // XineLib::GetParamAudioComprLevel

    inline int XineLib::GetParamAudioAmpLevel() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_AUDIO_AMP_LEVEL);
      } catch(...){}
      return 0;
    } // XineLib::GetParamAudioAmpLevel

    inline int XineLib::GetParamAudioReportLevel() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_AUDIO_REPORT_LEVEL);
      } catch(...){}
      return 0;
    } // XineLib::GetParamAudioReportLevel

    inline int XineLib::GetParamVerbosity() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_VERBOSITY);
      } catch(...){}
      return 0;
    } // XineLib::GetParamVerbosity

    inline int XineLib::GetParamSpuOffset() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_SPU_OFFSET);
      } catch(...){}
      return 0;
    } // XineLib::GetParamSpuOffset

    inline int XineLib::GetParamIgnoreVideo() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_IGNORE_VIDEO);
      } catch(...){}
      return 0;
    } // XineLib::GetParamIgnoreVideo

    inline int XineLib::GetParamIgnoreAudio() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_IGNORE_AUDIO);
      } catch(...){}
      return 0;
    } // XineLib::GetParamIgnoreAudio

    inline int XineLib::GetParamIgnoreSpu() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_IGNORE_SPU);
      } catch(...){}
      return 0;
    } // XineLib::GetParamIgnoreSpu

    inline int XineLib::GetParamBroadcasterPort() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_BROADCASTER_PORT);
      } catch(...){}
      return 0;
    } // XineLib::GetParamBroadcasterPort

    inline int XineLib::GetParamMetronomPrebuffer() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_METRONOM_PREBUFFER);
      } catch(...){}
      return 0;
    } // XineLib::GetParamMetronomPrebuffer

    inline int XineLib::GetParamAudioAmpMute() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_AUDIO_AMP_MUTE);
      } catch(...){}
      return 0;
    } // XineLib::GetParamAudioAmpMute

    inline int XineLib::GetParamFineSpeed() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_FINE_SPEED);
      } catch(...){}
      return 0;
    } // XineLib::GetParamFineSpeed

    inline int XineLib::GetParamEarlyFinishedEvent() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_EARLY_FINISHED_EVENT);
      } catch(...){}
      return 0;
    } // XineLib::GetParamEarlyFinishedEvent

    inline int XineLib::GetParamGaplessSwitch() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_GAPLESS_SWITCH);
      } catch(...){}
      return 0;
    } // XineLib::GetParamGaplessSwitch

    inline int XineLib::GetParamDeinterlace() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_VO_DEINTERLACE);
      } catch(...){}
      return 0;
    } // XineLib::GetParamDeinterlace

    inline int XineLib::GetParamVoAspectRatio() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_VO_ASPECT_RATIO);
      } catch(...){}
      return 0;
    } // XineLib::GetParamVoAspectRatio

    inline int XineLib::GetParamVoHue() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_VO_HUE);
      } catch(...){}
      return 0;
    } // XineLib::GetParamVoHue

    inline int XineLib::GetParamVoSaturation() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_VO_SATURATION);
      } catch(...){}
      return 0;
    } // XineLib::GetParamVoSaturation

    inline int XineLib::GetParamVoContrast() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_VO_CONTRAST);
      } catch(...){}
      return 0;
    } // XineLib::GetParamVoContrast

    inline int XineLib::GetParamVoBrightness() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_VO_BRIGHTNESS);
      } catch(...){}
      return 0;
    } // XineLib::GetParamVoBrightness

    inline int XineLib::GetParamVoZoomX() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_VO_ZOOM_X);
      } catch(...){}
      return 0;
    } // XineLib::GetParamVoZoomX

    inline int XineLib::GetParamVoZommY() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_VO_ZOOM_Y);
      } catch(...){}
      return 0;
    } // XineLib::GetParamVoZommY

    inline int XineLib::GetParamPanScan() {
      try {
        if(!stream_) return 0;
        return xine_get_param(stream_, XINE_PARAM_VO_PAN_SCAN);
      } catch(...){}
      return 0;
    } // XineLib::GetParamPanScan
*/
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // xine stream parameter 
/*
    inline void XineLib::SetParamSpeed(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_,XINE_PARAM_SPEED, val);
      } catch(...) {}
    } // XineLib::SetParamSpeed
*/
/*
    inline void XineLib::SetParamAVOffset(int val) {
      try {
         if(!stream_) return;
         xine_set_param(stream_, XINE_PARAM_AV_OFFSET, val);
      } catch(...) {}
    } // XineLib::SetParamAVOffset

    inline void XineLib::SetParamAudioChannelLogical(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_AUDIO_CHANNEL_LOGICAL, val);
      } catch(...) {}
    } // XineLib::SetParamAudioChannelLogical

    inline void XineLib::SetParamSPUChannel(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_SPU_CHANNEL, val);
      } catch(...) {}
    } // XineLib::SetParamSPUChannel

    inline void XineLib::SetParamParamVideoChannel(int val) {
      try {
       if(!stream_) return;
       xine_set_param(stream_, XINE_PARAM_VIDEO_CHANNEL, val);
      } catch(...) {}
    } // XineLib::SetParamParamVideoChannel

    inline void XineLib::SetParamAudioVolume(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_AUDIO_VOLUME, val);
      } catch(...) {}
    } // XineLib::SetParamAudioVolume

    inline void XineLib::SetParamAudioMute(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_AUDIO_MUTE, val);
      } catch(...) {}
    } // XineLib::SetParamAudioMute

    inline void XineLib::SetParamAudioComprLevel(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_AUDIO_COMPR_LEVEL, val);
      } catch(...) {}
    } // XineLib::SetParamAudioComprLevel

    inline void XineLib::SetParamAudioAmpLevel(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_AUDIO_AMP_LEVEL, val);
      } catch(...) {}
    } // XineLib::SetParamAudioAmpLevel

    inline void XineLib::SetParamAudioReportLevel(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_AUDIO_REPORT_LEVEL, val);
      } catch(...) {}
    } // XineLib::SetParamAudioReportLevel

    inline void XineLib::SetParamVerbosity(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_VERBOSITY, val);
      } catch(...) {}
    } // XineLib::SetParamVerbosity

    inline void XineLib::SetParamSpuOffset(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_SPU_OFFSET, val);
      } catch(...) {}
    } // XineLib::SetParamSpuOffset

    inline void XineLib::SetParamIgnoreVideo(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_IGNORE_VIDEO, val);
      } catch(...) {}
    } // XineLib::SetParamIgnoreVideo

    inline void XineLib::SetParamIgnoreAudio(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_IGNORE_AUDIO, val);
      } catch(...) {}
    } // XineLib::SetParamIgnoreAudio

    inline void XineLib::SetParamIgnoreSpu(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_IGNORE_SPU, val);
      } catch(...) {}
    } // XineLib::SetParamIgnoreSpu

    inline void XineLib::SetParamBroadcasterPort(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_BROADCASTER_PORT, val);
      } catch(...) {}
    } // XineLib::SetParamBroadcasterPort

    inline void XineLib::SetParamMetronomPrebuffer(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_METRONOM_PREBUFFER, val);
      } catch(...) {}
    } // XineLib::SetParamMetronomPrebuffer

    inline void XineLib::SetParamAudioAmpMute(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_AUDIO_AMP_MUTE, val);
      } catch(...) {}
    } // XineLib::SetParamAudioAmpMute

    inline void XineLib::SetParamFineSpeed(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_FINE_SPEED, val);
      } catch(...) {}
    } // XineLib::SetParamFineSpeed

    inline void XineLib::SetParamEarlyFinishedEvent(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_EARLY_FINISHED_EVENT, val);
      } catch(...) {}
    } // XineLib::SetParamEarlyFinishedEvent

    inline void XineLib::SetParamGaplessSwitch(int val) {
      try {
        if(!stream_) return;
        xine_set_param(stream_, XINE_PARAM_GAPLESS_SWITCH, val);
      } catch(...) {}
    } // XineLib::SetParamGaplessSwitch

    inline myxine_mrl_t **XineLib::GetMrls(const char *start_mrl, int *num_mrls) {
      try {
         const char *id = InputPlugin();
         if(!xine_ || !id) return NULL;
         return reinterpret_cast<myxine_mrl_t **>(xine_get_browse_mrls (xine_, id, start_mrl, num_mrls));
      } catch(...) {
        return NULL;
      } // try
    } // XineLib::GetMrls
*/
  } // namespace XineMediaplayer
} // namespace Reel

#endif // XINE_MEDIAPLAYER_XINE_LIB_H_INCLUDED
