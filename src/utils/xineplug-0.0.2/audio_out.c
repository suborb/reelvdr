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

/* audio_out.c */

#include "audio_out.h"

#include "utils.h"

#include "VdrXineMpIf.h"

#include <xine.h>
#include <xineutils.h>
#include <audio_out.h>

#define GAP_TOLERANCE 10000
    /* FIXME: Adjust this value! */

/*******************************************************************************
 * Audio driver instance.
 */

static ao_driver_t ao_driver_;      /* Single driver instance. */
static int         ao_driver_open_;
static int         num_channels_;
static int         bytes_per_frame_;

VdrXineMpIf const *VXI;

static int ao_bytes_per_frame(ao_driver_t *self)
{
    REEL_LOG("audio_out_reel: ao_bytes_per_frame()\n");
    return bytes_per_frame_;
}

static void ao_close(ao_driver_t *self)
{
    VXI->AoClose();
}

static int ao_control(ao_driver_t *self, int cmd, ...)
{
    VdrXineMpAoCtrl ctrlCmd = VdrXineMpAoCtrlNone;

    switch (cmd)
    {
    case AO_CTRL_PLAY_PAUSE:
        ctrlCmd = VdrXineMpAoCtrlPlayPause;
        break;
    
    case AO_CTRL_PLAY_RESUME:
        ctrlCmd = VdrXineMpAoCtrlPlayResume;
        break;
    
    case AO_CTRL_FLUSH_BUFFERS:
        ctrlCmd = VdrXineMpAoCtrlFlushBuffers;
        break;
    }

    return VXI->AoControl(ctrlCmd);
}

static int ao_delay(ao_driver_t *self)
{
    // REEL_LOG("audio_out_reel: ao_delay()\n");
    return VXI->AoDelay() ;
}

static void ao_exit(ao_driver_t *self)
{
    ao_close(self);

    ao_driver_open_ = 0;
}

static uint32_t ao_get_capabilities(ao_driver_t *self)
{
    uint32_t ret = AO_CAP_MODE_STEREO | AO_CAP_16BITS; /* Only 16 bit / 2-Channel-stereo supported */
    if (VXI->AoUseDigitalOut())
    {
        ret |= AO_CAP_MODE_A52;
    }
    return ret;
}

static int ao_get_gap_tolerance(ao_driver_t *self)
{
    return GAP_TOLERANCE;
}

static int ao_get_property(ao_driver_t *self, int property)
{
    REEL_LOG_1("audio_out_reel: ao_get_property() property=%d\n", property);

    return 0; /* We do not support any properties. */
}

static int ao_num_channels(ao_driver_t *self)
{
    return num_channels_;
}

static int ao_open(ao_driver_t *self, uint32_t bits, uint32_t rate, int mode)
{
    /* TODO: Support mono. */

    VdrXineMpAoCapMode capMode;

    switch (mode)
    {
    case AO_CAP_MODE_STEREO:
        num_channels_ = 2;
        bytes_per_frame_ = 4;
        capMode = VdrXineMpAoCapModeStereo;
        break;
    case AO_CAP_MODE_A52:
        num_channels_ = 2;
        bytes_per_frame_ = 4;
        capMode = VdrXineMpAoCapModeA52;
        break;
    default:
        REEL_LOG_1("audio_out_reel: ao_open() mode=%d not supported\n", mode);
        capMode = VdrXineMpAoCapModeUnsupported;
        break;
    }

    return VXI->AoOpen(bits, rate, capMode);
}

static int ao_set_property(ao_driver_t *self, int property, int value)
{
    REEL_LOG_2("audio_out_reel: ao_set_property() property=%d value=%d\n", property, value);

    return ~value; /* We do not support any properties. */
}

static int ao_write(ao_driver_t *self, int16_t *data, uint32_t num_frames)
{
    return VXI->AoWrite(data, num_frames);
}

/*******************************************************************************
 * Audio driver class.
 */

static void aocls_dispose(audio_driver_class_t *cls)
{
    REEL_LOG("audio_out_reel: aocls_dispose()\n"); // FIXME: Bug in xine: never called!

    free(cls);
}

static char *aocls_get_description (audio_driver_class_t *cls)
{
    return "reel audio output plugin";
}

static char *aocls_get_identifier (audio_driver_class_t *cls)
{
    return "audio_out_reel";
}

static ao_driver_t *aocls_open_plugin(audio_driver_class_t *cls, void const *data)
{
    if (ao_driver_open_)
    {
        /* We are unable to support multiple simultaneous drivers. */
        REEL_LOG("audio_out_reel: aocls_open_plugin() : "
                 "Driver instance is already active.\n");
        return NULL;
    }

    ao_driver_.bytes_per_frame   = ao_bytes_per_frame;
    ao_driver_.close             = ao_close;
    ao_driver_.control           = ao_control;
    ao_driver_.delay             = ao_delay;
    ao_driver_.exit              = ao_exit;
    ao_driver_.get_capabilities  = ao_get_capabilities;
    ao_driver_.get_gap_tolerance = ao_get_gap_tolerance;
    ao_driver_.get_property      = ao_get_property;
    ao_driver_.num_channels      = ao_num_channels;
    ao_driver_.open              = ao_open;
    ao_driver_.set_property      = ao_set_property;
    ao_driver_.write             = ao_write;

    ao_driver_open_ = 1;

    return &ao_driver_;
}

/*******************************************************************************
 * Plugin initialization.
 */

void *init_plugin_audio_out_reel(xine_t *xine, void *data)
{
    audio_driver_class_t *cls;
    
    VXI = (VdrXineMpIf const *)data;

    cls = (audio_driver_class_t *)xine_xmalloc(sizeof(audio_driver_class_t));
    
    cls->get_description = aocls_get_description;
    cls->get_identifier  = aocls_get_identifier;
    cls->dispose         = aocls_dispose;
    cls->open_plugin     = aocls_open_plugin;
    
    return cls;
}
