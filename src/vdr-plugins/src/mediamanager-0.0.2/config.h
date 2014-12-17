/*
 * config.h:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef _M_CONFIG_H
#define _M_CONFIG_H

/* running as root */
// #define MOUNTCMD "/bin/mount"
// #define UMOUNTCMD "/bin/umount"

/* sudo configured */
#define MOUNTCMD "sudo /bin/mount"
#define UMOUNTCMD "sudo /bin/umount"

#endif
