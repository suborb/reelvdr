/***************************************************************************
 *   Copyright (C) 2008 by Reel Multimedia Vertriebs GmbH                  *
 *                         E-Mail  : info @ reel-multimedia.com            *
 *                         Internet: www.reel-multimedia.com               *
 *                                                                         *
 *   This code is free software; you can redistribute it and/or modify     *
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

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <xine.h>
// /usr/bin/xine -V hde_video -A hde-audio --stdctl         /media/002.vdr
// dvd://home/tiqq/av/movie/DVD/Findet_Nemo/
// dvd://home/tiqq/av/movie/DVD/Deamon_4x3/
// /home/tiqq/rb.dirk/testing/src/utils/xine-hde/xplayer
static void event_listener ( void *user_data, const xine_event_t *event ) {
    printf ( "event_listener\n" );
    switch ( event->type ) {
        case XINE_EVENT_UI_PLAYBACK_FINISHED:
            break;
    } // switch
} // event_listener

int main ( int argc, char **argv ) {
    if ( argc < 2 ) {
        printf ( "Usage:\n%s <filename>\n", argv[0] );
        return 1;
    } // if
//  XineLib &xineLib_=XineLib::Instance();
// xineLib_.SetParamAudioReportLevel(5);
// xineLib_.Init();
    xine_t *xine_ = 0;
    xine_audio_port_t *aoPort_ = 0;
    xine_video_port_t *voPort_ = 0;
    xine_stream_t *stream_ = 0;
    xine_event_queue_t *eventQueue_ = 0;
    char const *audioDriverId = "hde-audio";
    char const *videoDriverId = "hde-video-aa";

    printf ( "xine_new\n" );
    xine_ = xine_new();
    if ( !xine_ )
        goto cleanup;
    xine_config_load ( xine_, "./config" );
    xine_engine_set_param ( xine_, XINE_ENGINE_PARAM_VERBOSITY, 1 );

    printf ( "xine_init\n" );
    xine_init ( xine_ );
// char const *audioDriverId = "audio_out_reel";
// aoPort_ = ::xine_open_audio_driver(xine_, audioDriverId, const_cast<VdrXineMpIf *>(vxi));
    printf ( "xine_open_audio_driver\n" );
    aoPort_ = xine_open_audio_driver ( xine_, audioDriverId, 0 );
    //aoPort_ = xine_open_audio_driver ( xine_, "auto", 0 );
    if ( !aoPort_ )
        goto cleanup;
    printf ( "xine_open_video_driver\n" );
   //voPort_ = xine_open_video_driver ( xine_, "none", XINE_VISUAL_TYPE_NONE, 0 );
    voPort_ = xine_open_video_driver ( xine_, videoDriverId, XINE_VISUAL_TYPE_AA, 0 );
    if ( !voPort_ )
        goto cleanup;
    printf ( "xine_stream_new\n" );
    stream_ = xine_stream_new ( xine_, aoPort_, voPort_ );
    if ( !stream_ )
        goto cleanup;
    printf ( "xine_event_new_queue\n" );
    eventQueue_ = xine_event_new_queue ( stream_ );
    if ( !eventQueue_ )
        goto cleanup;
    xine_event_create_listener_thread ( eventQueue_, event_listener, NULL );

// xineLib_.Open(argv[1]);
    printf ( "xine_open\n" );
    xine_open ( stream_, argv[1] );

// xineLib_.SetXineConfig();
    // debug
// xineLib_.PrintXineConfig();
// DeviceFlush();
// xineLib_.Play(0, 0);
    printf ( "xine_play\n" );
    xine_play ( stream_, 0, 0 );
    {
        int restore = 0;
        struct termios raw, orig;
        if ( isatty ( STDIN_FILENO ) && !fcntl ( STDIN_FILENO, F_SETFL, O_NONBLOCK ) ) {
            tcgetattr ( STDIN_FILENO, &orig );
            raw = orig;
            raw.c_lflag &= ~ ( ICANON | ECHO );
            tcsetattr ( STDIN_FILENO, TCSAFLUSH, &raw );
            restore = 1;
        } // if
        char lKey = 0;
int chan=0;
        while ( lKey != 'q' ) {
            if ( read ( STDIN_FILENO, &lKey, 1 ) != 1 )
                usleep ( 100000 );
if (lKey == 's'){
int a = xine_get_stream_info (stream_, XINE_STREAM_INFO_MAX_AUDIO_CHANNEL);
chan++;
if(chan > a-1)chan=0;
printf("=============== set %d max %d \n",chan,a-1);
//xine_set_param (stream_,XINE_PARAM_SPEED  ,XINE_SPEED_PAUSE);
//usleep(10000);
xine_set_param (stream_,XINE_PARAM_AUDIO_CHANNEL_LOGICAL  ,chan);
//xine_set_param (stream_,XINE_PARAM_SPEED  ,XINE_SPEED_NORMAL);
lKey=0;
}
        } // while
        if ( restore )
            tcsetattr ( STDIN_FILENO, TCSAFLUSH, &orig );
    }
// xineLib_.Exit();
cleanup:
    if ( !xine_ )
        printf ( "Failed to initialize xine\n" );
    if ( !aoPort_ )
        printf ( "Failed to initialize audio\n" );
    if ( !voPort_ )
        printf ( "Failed to initialize video\n" );
    if ( !stream_ )
        printf ( "Failed to initialize stream\n" );
    if ( !eventQueue_ )
        printf ( "Failed to initialize queue\n" );

    if ( stream_ )
        xine_stop ( stream_ );
    if ( stream_ )
        xine_close ( stream_ );
    if ( eventQueue_ )
        xine_event_dispose_queue ( eventQueue_ );
    if ( stream_ )
        xine_dispose ( stream_ );
    if ( xine_ && voPort_ )
        xine_close_video_driver ( xine_, voPort_ );
    if ( xine_ && aoPort_ )
        xine_close_audio_driver ( xine_, aoPort_ );
    if ( xine_ )
        xine_exit ( xine_ );
    return 0;
} // main
