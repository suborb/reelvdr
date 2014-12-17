/*
 *  Copyright (C) 2004-2007 Nathan Lutchansky <lutchann@litech.org>
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

#define NMTK_DEBUG	0
#define NMTK_VERBOSE	1
#define NMTK_INFO	2
#define NMTK_WARN	3
#define NMTK_ERR	4

void nmtk_set_log_level(int min);
void nmtk_log(int level, char *fmt, ...);
void nmtk_dump_log_buffer(int fd);
