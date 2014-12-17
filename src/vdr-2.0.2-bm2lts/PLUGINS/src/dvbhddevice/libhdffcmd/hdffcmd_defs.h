/**********************************************************************
 *
 * HDFF firmware command interface library
 *
 * Copyright (C) 2011  Andreas Regel
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *********************************************************************/

#ifndef HDFFCMD_DEFS_H
#define HDFFCMD_DEFS_H

typedef enum HdffMessageType_t
{
    HDFF_MSG_TYPE_COMMAND,
    HDFF_MSG_TYPE_ANSWER,
    HDFF_MSG_TYPE_RESULT,
    HDFF_MSG_TYPE_EVENT
} HdffMessageType_t;

typedef enum HdffMessageGroup_t
{
    HDFF_MSG_GROUP_GENERIC,
    HDFF_MSG_GROUP_AV_DECODER,
    HDFF_MSG_GROUP_AV_MUX,
    HDFF_MSG_GROUP_FRONTEND,
    HDFF_MSG_GROUP_OSD,
    HDFF_MSG_GROUP_HDMI,
    HDFF_MSG_GROUP_REMOTE_CONTROL
} HdffMessageGroup_t;

typedef enum HdffMessageId_t
{
    HDFF_MSG_GEN_GET_FIRMWARE_VERSION = 0,
    HDFF_MSG_GEN_GET_INTERFACE_VERSION,
    HDFF_MSG_GEN_GET_COPYRIGHTS,

    HDFF_MSG_AV_SET_AUDIO_PID = 0,
    HDFF_MSG_AV_SET_VIDEO_PID,
    HDFF_MSG_AV_SET_PCR_PID,
    HDFF_MSG_AV_SET_TELETEXT_PID,
    HDFF_MSG_AV_SHOW_STILL_IMAGE,
    HDFF_MSG_AV_SET_VIDEO_WINDOW,
    HDFF_MSG_AV_SET_DECODER_INPUT,
    HDFF_MSG_AV_SET_DEMULTIPLEXER_INPUT,
    HDFF_MSG_AV_SET_VIDEO_FORMAT,
    HDFF_MSG_AV_SET_VIDEO_OUTPUT_MODE,
    HDFF_MSG_AV_SET_STC,
    HDFF_MSG_AV_FLUSH_BUFFER,
    HDFF_MSG_AV_ENABLE_SYNC,
    HDFF_MSG_AV_SET_VIDEO_SPEED,
    HDFF_MSG_AV_SET_AUDIO_SPEED,
    HDFF_MSG_AV_ENABLE_VIDEO_AFTER_STOP,
    HDFF_MSG_AV_GET_VIDEO_FORMAT_INFO,
    HDFF_MSG_AV_SET_AUDIO_DELAY,
    HDFF_MSG_AV_SET_AUDIO_DOWNMIX,
    HDFF_MSG_AV_SET_AUDIO_CHANNEL,
    HDFF_MSG_AV_SET_PLAY_MODE,
    HDFF_MSG_AV_SET_OPTIONS,
    HDFF_MSG_AV_MUTE_AUDIO,
    HDFF_MSG_AV_MUTE_VIDEO,

    HDFF_MSG_MUX_SET_VIDEO_OUT = 0,
    HDFF_MSG_MUX_SET_SLOW_BLANK,
    HDFF_MSG_MUX_SET_FAST_BLANK,
    HDFF_MSG_MUX_SET_VOLUME,
    HDFF_MSG_MUX_SET_AUDIO_MUTE,

    HDFF_MSG_OSD_CONFIGURE = 0,
    HDFF_MSG_OSD_RESET,
    HDFF_MSG_OSD_CREATE_DISPLAY = 10,
    HDFF_MSG_OSD_DELETE_DISPLAY,
    HDFF_MSG_OSD_ENABLE_DISPLAY,
    HDFF_MSG_OSD_SET_DISPLAY_OUTPUT_RECTANGLE,
    HDFF_MSG_OSD_SET_DISPLAY_CLIPPLING_AREA,
    HDFF_MSG_OSD_RENDER_DISPLAY,
    HDFF_MSG_OSD_SAVE_REGION,
    HDFF_MSG_OSD_RESTORE_REGION,
    HDFF_MSG_OSD_CREATE_PALETTE = 30,
    HDFF_MSG_OSD_DELETE_PALETTE,
    HDFF_MSG_OSD_SET_DISPLAY_PALETTE,
    HDFF_MSG_OSD_SET_PALETTE_COLORS,
    HDFF_MSG_OSD_CREATE_FONT_FACE = 50,
    HDFF_MSG_OSD_DELETE_FONT_FACE,
    HDFF_MSG_OSD_CREATE_FONT,
    HDFF_MSG_OSD_DELETE_FONT,
    HDFF_MSG_OSD_DRAW_PIXEL = 70,
    HDFF_MSG_OSD_DRAW_RECTANGLE,
    HDFF_MSG_OSD_DRAW_CIRCLE,
    HDFF_MSG_OSD_DRAW_ELLIPSE,
    HDFF_MSG_OSD_DRAW_SLOPE,
    HDFF_MSG_OSD_DRAW_TEXT,
    HDFF_MSG_OSD_DRAW_WIDE_TEXT,
    HDFF_MSG_OSD_DRAW_BITMAP,
    HDFF_MSG_OSD_DRAW_UTF8_TEXT,

    HDFF_MSG_HDMI_ENABLE_OUTPUT = 0,
    HDFF_MSG_HDMI_SET_VIDEO_MODE,
    HDFF_MSG_HDMI_CONFIGURE,
    HDFF_MSG_HDMI_IS_DISPLAY_CONNECTED,
    HDFF_MSG_HDMI_GET_DISPLAY_INFO,
    HDFF_MSG_HDMI_GET_VIDEO_MODE,
    HDFF_MSG_HDMI_SEND_CEC_COMMAND,
    HDFF_MSG_HDMI_SEND_RAW_CEC_COMMAND,

    HDFF_MSG_REMOTE_SET_PROTOCOL = 0,
    HDFF_MSG_REMOTE_SET_ADDRESS_FILTER,
    HDFF_MSG_REMOTE_KEY_EVENT
} HdffMessageId_t;

#endif /* HDFFCMD_DEFS_H */
