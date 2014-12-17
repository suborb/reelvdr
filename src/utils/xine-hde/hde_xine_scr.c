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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#define LOG_MODULE "hde-scr"
#define LOG_VERBOSE
#define LOG

#define LOG_FUNC 0
#define LOG_ERR  1
#define LOG_DIFF 1

#include "hde_xine.h"
#include "hde_io.h"

static int hde_get_priority(scr_plugin_t *scr) {
	return HDE_SCR_PRIORITY;
} // hde_get_priority

static void hde_adjust(scr_plugin_t *scr, int64_t pts) {
	hde_scr_t *this = (hde_scr_t *)scr;
	pthread_mutex_lock(&this->mutex);
	this->pts = pts;
	this->stc = 0;
	hde_io_adjust_stc(pts, &this->stc);
	this->ct  = GetCurrentTime();
	hde_io_get_speed(&this->speed);
	pthread_mutex_unlock(&this->mutex);
	llprintf(LOG_FUNC, "hde_adjust     : %lld %lld\n", pts, this->pts + ((GetCurrentTime() - this->ct) * this->speed)/1000000ll);
} // hde_adjust

int64_t last = 0;
static int64_t hde_get_current(scr_plugin_t *scr) {
	hde_scr_t *this = (hde_scr_t *)scr;
	int64_t diff = 0;
	int64_t stc = 0;
	pthread_mutex_lock(&this->mutex);
	hde_io_get_stc(&stc);
	if (!this->speed)
		diff = this->pts;
	else
		diff = this->pts + ((stc - this->stc) * this->speed) / 90000ll;
	if(diff < this->pts || !stc || !this->stc) // something wrong with stc
		diff = this->pts + ((GetCurrentTime() - this->ct) * this->speed) / 1000000ll;
	pthread_mutex_unlock(&this->mutex);
	if(diff<0) {
		diff=last;
	} else {
		last = diff;
	} // if
	return diff;
} // hde_get_current

static int hde_set_fine_speed (scr_plugin_t *scr, int speed) {
	hde_scr_t *this = (hde_scr_t *)scr;
	llprintf(LOG_FUNC, "speed start    : %d\n", speed);
	hde_adjust(scr, hde_get_current(scr)+1);
	pthread_mutex_lock(&this->mutex);
	this->speed = (90000ll*speed)/XINE_FINE_SPEED_NORMAL;
	hde_io_set_speed(this->speed);
	hde_io_get_speed(&this->speed);
	pthread_mutex_unlock(&this->mutex);
	llprintf(LOG_FUNC, "speed done     : %d\n", (int)((this->speed * XINE_FINE_SPEED_NORMAL)/90000ll));
	return (this->speed * XINE_FINE_SPEED_NORMAL)/90000ll;
} // hde_set_fine_speed

static void hde_start(scr_plugin_t *scr, int64_t pts) {
	llprintf ( LOG_FUNC, "hde_start\n" );
	hde_adjust(scr, pts);
	hde_set_fine_speed (scr, XINE_FINE_SPEED_NORMAL);
} //hde_start

static void hde_exit(scr_plugin_t *scr) {
	llprintf ( LOG_FUNC, "hde_exit\n" );
	hde_scr_t *this = (hde_scr_t *)scr;
	pthread_mutex_destroy(&this->mutex);
	free(this);
} // hde_exit

hde_scr_t *hde_scr_init(xine_t *xine) {
	llprintf ( LOG_FUNC, "hde_scr_init\n" );
	hde_scr_t *this = ( hde_scr_t * ) xine_xmalloc ( sizeof ( hde_scr_t ) );
	if ( !this ) return NULL;
	this->scr_plugin.interface_version = 3; // see xine-engine/metronom.c
	this->scr_plugin.get_priority   = hde_get_priority;
	this->scr_plugin.start          = hde_start;
	this->scr_plugin.get_current    = hde_get_current;
	this->scr_plugin.adjust         = hde_adjust;
	this->scr_plugin.set_fine_speed = hde_set_fine_speed;
	this->scr_plugin.exit           = hde_exit;
	this->xine                      = xine;
	pthread_mutex_init (&this->mutex, NULL);
	return this;
} // hde_scr_init
