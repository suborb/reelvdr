/*
 *  Copyright (C) 2004-2006 Nathan Lutchansky <lutchann@litech.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <sys/types.h>
#include <sys/time.h>

#ifndef _EVENT_H
#define _EVENT_H

typedef struct timeval time_ref;

int time_diff(time_ref *tr_start, time_ref *tr_end);
int time_ago(time_ref *tr);
void time_now(time_ref *tr);
void time_add(time_ref *tr, int msec);
void time_future(time_ref *tr, int msec);

typedef union {
	unsigned long val;
	void *ptr;
} user_data_t;

static const user_data_t USER_DATA_NONE = { .val = 0 };

typedef void (*user_callback_t)(user_data_t data);

typedef void * timer_id_t;
#define TIMER_NONE NULL

struct ref_counter;
struct ref_counter *ref_counter_new(user_callback_t finalizer,
		user_data_t finalizer_data);
void ref_counter_inc(struct ref_counter *rc);
void ref_counter_dec(struct ref_counter *rc);

struct notifier;
struct notifier *notifier_new(void);
void notifier_set_state(struct notifier *n, int active);
int notifier_get_state(struct notifier *n);
int notifier_set_callbacks(struct notifier *n, user_callback_t toggled,
		user_callback_t removed, user_data_t data);
void notifier_del(struct notifier *n);

struct dispatcher;
struct dispatcher *dispatcher_new(void);
void dispatcher_del(struct dispatcher *d);

int dispatcher_add_fd(struct dispatcher *d, int fd, int write, int arm,
		user_callback_t callback, user_data_t data,
		struct ref_counter *obj_ref);
void dispatcher_arm_fd(struct dispatcher *d, int fd, int write, int arm);
void dispatcher_del_fd(struct dispatcher *d, int fd, int write);
void dispatcher_del_fd_sync(struct dispatcher *d, int fd, int write);

int dispatcher_add_notifier(struct dispatcher *d, struct notifier *n, int arm,
		user_callback_t callback, user_data_t data,
		struct ref_counter *obj_ref);
void dispatcher_arm_notifier(struct dispatcher *d, struct notifier *n, int arm);
void dispatcher_del_notifier(struct dispatcher *d, struct notifier *n);

timer_id_t dispatcher_add_timer(struct dispatcher *d,
		time_ref *expires, user_callback_t callback, user_data_t data,
		struct ref_counter *obj_ref);
void dispatcher_mod_timer(struct dispatcher *d, timer_id_t timer_id,
		time_ref *expires);
void dispatcher_del_timer(struct dispatcher *d, timer_id_t timer_id);

int dispatcher_poll(struct dispatcher *d, int timeout, time_ref *expired,
		user_callback_t *callback, user_data_t *data);
void dispatcher_run(struct dispatcher *d);
void dispatcher_exit(struct dispatcher *d);

#endif /* EVENT_H */
